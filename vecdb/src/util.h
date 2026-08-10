#ifndef VECDB_UTIL_H
#define VECDB_UTIL_H

#include <stddef.h>
#include <stdint.h>

/* Print a formatted message to stderr and exit with a failure status. */
void die(const char *fmt, ...);

/* malloc/calloc wrappers that abort the program on allocation failure. */
void *xmalloc(size_t n);
void *xcalloc(size_t count, size_t size);

/* Big-endian (network order) serialization helpers.
   The .vdb on-disk format stores all integers big-endian so the file is
   portable across architectures and easy to inspect with a hex viewer. */
void     put_be16(uint8_t *p, uint16_t v);
void     put_be32(uint8_t *p, uint32_t v);
void     put_be64(uint8_t *p, uint64_t v);
uint16_t get_be16(const uint8_t *p);
uint32_t get_be32(const uint8_t *p);
uint64_t get_be64(const uint8_t *p);

/* Seeded 32-bit FNV-1a hash. The embedder uses this to map a token to a
   bucket in the embedding vector (the "hashing trick"); the seed comes
   from the store header so a file's vectors are reproducible. */
uint32_t fnv1a(const void *data, size_t len, uint32_t seed);

#endif /* VECDB_UTIL_H */
