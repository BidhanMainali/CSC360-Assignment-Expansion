#ifndef VECDB_INDEX_H
#define VECDB_INDEX_H

#include "store.h"

/* Recompute the IDF weights from the dataset's payloads and re-embed every
   stored vector as TF-IDF. Replaces data->idf and data->vectors. Returns 0
   on success, -1 on allocation failure. Safe to call on an empty dataset
   (it simply clears any existing IDF). */
int vdb_build_index(VdbData *data);

#endif /* VECDB_INDEX_H */
