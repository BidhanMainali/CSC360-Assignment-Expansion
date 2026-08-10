#ifndef VECDB_EMBED_H
#define VECDB_EMBED_H

#include <stddef.h>

/* Callback invoked for each token found by tokenize(). `tok` is a
   NUL-terminated, lowercased copy of length `len`, valid only for the
   duration of the call. `ctx` is the caller's opaque pointer. */
typedef void (*token_fn)(const char *tok, size_t len, void *ctx);

/* Split `text` into lowercased alphanumeric tokens (delimited by any
   non-alphanumeric character) and pass each one to `on_token`. */
void tokenize(const char *text, token_fn on_token, void *ctx);

#endif /* VECDB_EMBED_H */
