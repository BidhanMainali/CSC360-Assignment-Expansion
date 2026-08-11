#include "search.h"
#include "embed.h"

#include <stdlib.h>

int vdb_search(const VdbData *data, const char *query, uint32_t k,
               Hit *hits, uint32_t *out_n) {
    uint32_t dim   = data->hdr.dim;
    uint32_t nhits = 0;
    uint32_t i;
    float   *qv;

    qv = malloc((size_t)dim * sizeof(float));
    if (qv == NULL) {
        return -1;
    }

    embed_tfidf(query, qv, dim, data->hdr.hash_seed, data->idf);

    /* Score every vector; keep the best k in a small array sorted by
       descending score (hits[0] is the best, hits[nhits-1] the weakest kept). */
    for (i = 0; i < data->count; i++) {
        const float *vv    = data->vectors + (size_t)i * dim;
        float        score = 0.0f;
        uint32_t     j;
        int          pos;

        for (j = 0; j < dim; j++) {
            score += qv[j] * vv[j];
        }

        if (nhits < k) {
            pos = (int)nhits;
            nhits++;
        } else if (score > hits[nhits - 1].score) {
            pos = (int)nhits - 1;   /* displace the weakest kept result */
        } else {
            continue;
        }

        while (pos > 0 && hits[pos - 1].score < score) {
            hits[pos] = hits[pos - 1];
            pos--;
        }
        hits[pos].score = score;
        hits[pos].index = i;
    }

    free(qv);
    *out_n = nhits;
    return 0;
}
