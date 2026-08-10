#include "index.h"
#include "embed.h"

#include <math.h>
#include <stdlib.h>

int vdb_build_index(VdbData *data) {
    uint32_t  dim  = data->hdr.dim;
    uint32_t  n    = data->count;
    uint32_t  seed = data->hdr.hash_seed;
    uint32_t  i;
    uint32_t  j;
    float    *idf;
    uint32_t *df;
    float    *tmp;

    if (n == 0) {
        free(data->idf);
        data->idf = NULL;
        return 0;
    }

    idf = malloc((size_t)dim * sizeof(float));
    df  = calloc((size_t)dim, sizeof(uint32_t));
    tmp = malloc((size_t)dim * sizeof(float));

    if (idf == NULL || df == NULL || tmp == NULL) {
        free(idf);
        free(df);
        free(tmp);
        return -1;
    }

    /* Document frequency: for each document, count the buckets it touches.
       Whether a bucket is nonzero is unaffected by normalization, so the
       plain TF embedding is enough to detect it. */
    for (i = 0; i < n; i++) {
        embed_tf(data->payloads[i], tmp, dim, seed);
        for (j = 0; j < dim; j++) {
            if (tmp[j] != 0.0f) {
                df[j]++;
            }
        }
    }

    /* Smoothed inverse document frequency (always >= 1). */
    for (j = 0; j < dim; j++) {
        idf[j] = (float)(log((double)(n + 1) / ((double)df[j] + 1.0)) + 1.0);
    }

    /* Re-embed every document as a TF-IDF vector. */
    for (i = 0; i < n; i++) {
        embed_tfidf(data->payloads[i], data->vectors + (size_t)i * dim, dim,
                    seed, idf);
    }

    free(data->idf);
    data->idf = idf;

    free(df);
    free(tmp);

    return 0;
}
