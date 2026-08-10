#ifndef VECDB_STORE_H
#define VECDB_STORE_H

#include <stdint.h>
#include <stdio.h>

/* The header occupies a fixed 512-byte block at the start of the file,
   mirroring the "superblock" idea from a real file system and leaving
   room for the format to grow. */
#define VDB_HEADER_SIZE   512u
#define VDB_MAGIC_LEN     8
#define VDB_VERSION       1u
#define VDB_DEFAULT_DIM   1024u

/* Similarity metric recorded in the header. Only cosine is used in v1. */
typedef enum {
    VDB_METRIC_COSINE = 0
} VdbMetric;

/* Index type recorded in the header. v1 ships a flat (exact) index; the
   field is reserved so an approximate index can be added later without a
   format break. */
typedef enum {
    VDB_INDEX_FLAT = 0
} VdbIndexType;

/* In-memory view of the header. The region offsets/sizes describe where
   each part of the file lives; they are all zero for a freshly created,
   empty store. */
typedef struct {
    uint32_t version;
    uint32_t dim;          /* embedding dimension                        */
    uint32_t count;        /* number of vectors stored                   */
    uint32_t hash_seed;    /* seed for the TF-IDF hashing embedder        */
    uint16_t metric;       /* VdbMetric                                  */
    uint16_t index_type;   /* VdbIndexType                               */

    uint64_t vectors_off;
    uint64_t vectors_size;
    uint64_t payload_off;
    uint64_t payload_size;
    uint64_t idmap_off;
    uint64_t idmap_size;
    uint64_t idf_off;
    uint64_t idf_size;
} VdbHeader;

/* An open store. */
typedef struct {
    FILE      *fp;
    char      *path;
    VdbHeader  hdr;
} Vdb;

/* Create a new, empty store at `path` with the given dimension and hash
   seed. Returns 0 on success, -1 on failure (message written to stderr). */
int vdb_create(const char *path, uint32_t dim, uint32_t hash_seed);

/* Open an existing store and validate its header. On success the Vdb is
   ready to use and must be released with vdb_close. Returns 0 on success,
   -1 on failure (message written to stderr). */
int vdb_open(const char *path, Vdb *db);

/* Close a store opened with vdb_open. Safe to call on a zeroed Vdb. */
void vdb_close(Vdb *db);

/* Print a human-readable summary of the store to `out`. */
void vdb_print_stats(const Vdb *db, FILE *out);

#endif /* VECDB_STORE_H */
