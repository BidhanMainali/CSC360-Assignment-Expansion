#include "embed.h"
#include "util.h"

#include <ctype.h>
#include <math.h>

/* Longer tokens are truncated to this many characters, which is far beyond
   any real word and keeps the tokenizer allocation-free. */
#define TOKEN_MAX 128

void tokenize(const char *text, token_fn on_token, void *ctx) {
    char   tok[TOKEN_MAX];
    size_t len = 0;
    size_t i;

    for (i = 0; ; i++) {
        unsigned char c = (unsigned char)text[i];

        if (c != '\0' && isalnum(c)) {
            /* Accumulate lowercased characters; drop anything past the cap. */
            if (len < TOKEN_MAX - 1) {
                tok[len++] = (char)tolower(c);
            }
        } else {
            /* A delimiter (or end of string) closes the current token. */
            if (len > 0) {
                tok[len] = '\0';
                on_token(tok, len, ctx);
                len = 0;
            }
            if (c == '\0') {
                break;
            }
        }
    }
}

/* Context threaded through tokenize() while building a term-frequency
   vector: the destination vector and the parameters that control hashing. */
typedef struct {
    float   *vec;
    uint32_t dim;
    uint32_t seed;
} tf_ctx;

/* Hash one token to a bucket and add a signed unit of term frequency. The
   sign, taken from a separate bit of the hash, cancels some collision bias. */
static void tf_accumulate(const char *tok, size_t len, void *ctx) {
    tf_ctx  *c    = (tf_ctx *)ctx;
    uint32_t h    = fnv1a(tok, len, c->seed);
    uint32_t idx  = h % c->dim;
    float    sign = (h & 0x80000000u) ? -1.0f : 1.0f;

    c->vec[idx] += sign;
}

void embed_tfidf(const char *text, float *out, uint32_t dim, uint32_t seed,
                 const float *idf) {
    tf_ctx   c;
    uint32_t i;
    double   norm = 0.0;

    for (i = 0; i < dim; i++) {
        out[i] = 0.0f;
    }

    c.vec  = out;
    c.dim  = dim;
    c.seed = seed;
    tokenize(text, tf_accumulate, &c);

    /* Weight each bucket by its IDF, if provided (turning TF into TF-IDF). */
    if (idf != NULL) {
        for (i = 0; i < dim; i++) {
            out[i] *= idf[i];
        }
    }

    /* L2-normalize so a dot product between two vectors equals their
       cosine similarity. */
    for (i = 0; i < dim; i++) {
        norm += (double)out[i] * (double)out[i];
    }
    norm = sqrt(norm);

    if (norm > 0.0) {
        for (i = 0; i < dim; i++) {
            out[i] = (float)((double)out[i] / norm);
        }
    }
}

void embed_tf(const char *text, float *out, uint32_t dim, uint32_t seed) {
    embed_tfidf(text, out, dim, seed, NULL);
}
