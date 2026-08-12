#include "search.h"
#include "embed.h"

#include <stdlib.h>

/* Insert (score, index) into a top-k list kept sorted by descending score.
   `hits` has room for `k` entries; *nhits is the current count. */
static void topk_insert(Hit *hits, uint32_t *nhits, uint32_t k,
                        float score, uint32_t index) {
    uint32_t n = *nhits;
    int      pos;

    if (n < k) {
        pos = (int)n;
        n++;
    } else if (score > hits[n - 1].score) {
        pos = (int)n - 1;   /* displace the weakest kept result */
    } else {
        return;
    }

    while (pos > 0 && hits[pos - 1].score < score) {
        hits[pos] = hits[pos - 1];
        pos--;
    }
    hits[pos].score = score;
    hits[pos].index = index;
    *nhits = n;
}

/* Score the vectors in [start, end) against `qv` and keep the best k in
   `hits` (cosine == dot product for normalized vectors). */
static void scan_range(const VdbData *data, const float *qv,
                       uint32_t start, uint32_t end, uint32_t k,
                       Hit *hits, uint32_t *nhits) {
    uint32_t dim = data->hdr.dim;
    uint32_t i;

    for (i = start; i < end; i++) {
        const float *vv    = data->vectors + (size_t)i * dim;
        float        score = 0.0f;
        uint32_t     j;

        for (j = 0; j < dim; j++) {
            score += qv[j] * vv[j];
        }
        topk_insert(hits, nhits, k, score, i);
    }
}

int vdb_search(const VdbData *data, const char *query, uint32_t k,
               Hit *hits, uint32_t *out_n) {
    uint32_t dim   = data->hdr.dim;
    uint32_t nhits = 0;
    float   *qv;

    qv = malloc((size_t)dim * sizeof(float));
    if (qv == NULL) {
        return -1;
    }

    embed_tfidf(query, qv, dim, data->hdr.hash_seed, data->idf);
    scan_range(data, qv, 0, data->count, k, hits, &nhits);

    free(qv);
    *out_n = nhits;
    return 0;
}

/* Per-worker task: score one contiguous range into a private top-k buffer. */
typedef struct {
    const VdbData *data;
    const float   *qv;
    uint32_t       start;
    uint32_t       end;
    uint32_t       k;
    Hit           *local;    /* private buffer of k hits */
    uint32_t       local_n;
} SearchTask;

static void search_task(void *arg) {
    SearchTask *t = (SearchTask *)arg;

    t->local_n = 0;
    scan_range(t->data, t->qv, t->start, t->end, t->k, t->local, &t->local_n);
}

int vdb_search_mt(const VdbData *data, const char *query, uint32_t k,
                  Hit *hits, uint32_t *out_n, ThreadPool *pool) {
    uint32_t    dim      = data->hdr.dim;
    int         nthreads = pool_size(pool);
    uint32_t    count    = data->count;
    uint32_t    nhits    = 0;
    uint32_t    base;
    uint32_t    rem;
    uint32_t    start;
    float      *qv;
    SearchTask *tasks;
    void      **args;
    Hit        *localbuf;
    int         t;

    qv       = malloc((size_t)dim * sizeof(float));
    tasks    = malloc((size_t)nthreads * sizeof(SearchTask));
    args     = malloc((size_t)nthreads * sizeof(void *));
    localbuf = malloc((size_t)nthreads * k * sizeof(Hit));

    if (qv == NULL || tasks == NULL || args == NULL || localbuf == NULL) {
        free(qv);
        free(tasks);
        free(args);
        free(localbuf);
        return -1;
    }

    embed_tfidf(query, qv, dim, data->hdr.hash_seed, data->idf);

    /* Divide [0, count) into nthreads contiguous ranges. */
    base  = count / (uint32_t)nthreads;
    rem   = count % (uint32_t)nthreads;
    start = 0;
    for (t = 0; t < nthreads; t++) {
        uint32_t len = base + ((uint32_t)t < rem ? 1u : 0u);

        tasks[t].data    = data;
        tasks[t].qv      = qv;
        tasks[t].start   = start;
        tasks[t].end     = start + len;
        tasks[t].k       = k;
        tasks[t].local   = localbuf + (size_t)t * k;
        tasks[t].local_n = 0;
        args[t]          = &tasks[t];
        start += len;
    }

    pool_run(pool, search_task, args, nthreads);

    /* Merge the per-worker top-k lists into the global top-k. */
    for (t = 0; t < nthreads; t++) {
        uint32_t r;

        for (r = 0; r < tasks[t].local_n; r++) {
            topk_insert(hits, &nhits, k, tasks[t].local[r].score,
                        tasks[t].local[r].index);
        }
    }

    free(qv);
    free(tasks);
    free(args);
    free(localbuf);
    *out_n = nhits;
    return 0;
}
