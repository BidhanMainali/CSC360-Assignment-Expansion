#ifndef VECDB_SEARCH_H
#define VECDB_SEARCH_H

#include "store.h"

/* One scored search result: a similarity score and the record it refers to. */
typedef struct {
    float    score;
    uint32_t index;
} Hit;

/* Score `query` against every vector in `data` (cosine similarity, using the
   dataset's stored IDF weights) and fill `hits` with the best `k`, sorted by
   descending score. `hits` must have room for `k` entries. The number actually
   written -- min(k, data->count) -- is stored in *out_n. Returns 0 on success,
   -1 on allocation failure. */
int vdb_search(const VdbData *data, const char *query, uint32_t k,
               Hit *hits, uint32_t *out_n);

#endif /* VECDB_SEARCH_H */
