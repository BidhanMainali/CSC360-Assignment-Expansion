#ifndef VECDB_EMBED_H
#define VECDB_EMBED_H

#include <stddef.h>
#include <stdint.h>

/* Callback invoked for each token found by tokenize(). `tok` is a
   NUL-terminated, lowercased copy of length `len`, valid only for the
   duration of the call. `ctx` is the caller's opaque pointer. */
typedef void (*token_fn)(const char *tok, size_t len, void *ctx);

/* Split `text` into lowercased alphanumeric tokens (delimited by any
   non-alphanumeric character) and pass each one to `on_token`. */
void tokenize(const char *text, token_fn on_token, void *ctx);

/* Embed `text` into `out` (an array of length `dim`) using a signed
   hashing-trick term-frequency vector, then L2-normalize it. `seed`
   selects the hash so a store's vectors stay reproducible. Because the
   result is normalized, the dot product of two embeddings is their cosine
   similarity. */
void embed_tf(const char *text, float *out, uint32_t dim, uint32_t seed);

#endif /* VECDB_EMBED_H */
