#include "embed.h"

#include <ctype.h>

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
