#include "store.h"
#include "embed.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VECDB_VERSION_STRING "vecdb 0.1 (M0)"

static void usage(FILE *out, const char *prog) {
    fprintf(out,
        "Usage: %s <command> [arguments]\n"
        "\n"
        "Commands:\n"
        "  create <file.vdb> [dim]   create a new, empty store\n"
        "                            (dim defaults to %u)\n"
        "  stats  <file.vdb>         print information about a store\n"
        "  open   <file.vdb>         check that a store opens and is valid\n"
        "  embed  \"<text>\" [dim]      embed text and print a vector summary\n"
        "  help                      show this message\n"
        "  version                   show the version\n",
        prog, VDB_DEFAULT_DIM);
}

static int cmd_create(int argc, char **argv) {
    const char *path;
    uint32_t    dim = VDB_DEFAULT_DIM;

    if (argc < 1) {
        fprintf(stderr, "vecdb create: missing file argument\n");
        return 1;
    }
    path = argv[0];

    if (argc >= 2) {
        long v = strtol(argv[1], NULL, 10);
        if (v <= 0) {
            fprintf(stderr, "vecdb create: dimension must be a positive integer\n");
            return 1;
        }
        dim = (uint32_t)v;
    }

    if (vdb_create(path, dim, 0) != 0) {
        return 1;
    }

    printf("Created '%s' (dim=%u, 0 vectors)\n", path, dim);
    return 0;
}

static int cmd_stats(int argc, char **argv) {
    Vdb db;

    if (argc < 1) {
        fprintf(stderr, "vecdb stats: missing file argument\n");
        return 1;
    }

    if (vdb_open(argv[0], &db) != 0) {
        return 1;
    }

    vdb_print_stats(&db, stdout);
    vdb_close(&db);
    return 0;
}

static int cmd_open(int argc, char **argv) {
    Vdb db;

    if (argc < 1) {
        fprintf(stderr, "vecdb open: missing file argument\n");
        return 1;
    }

    if (vdb_open(argv[0], &db) != 0) {
        return 1;
    }

    printf("'%s' is a valid vecdb store (version %u)\n", argv[0], db.hdr.version);
    vdb_close(&db);
    return 0;
}

static int cmd_embed(int argc, char **argv) {
    const uint32_t PRINT_CAP = 20;
    const char    *text;
    uint32_t       dim = VDB_DEFAULT_DIM;
    float         *vec;
    uint32_t       i;
    uint32_t       nonzero = 0;
    uint32_t       printed = 0;
    double         norm = 0.0;

    if (argc < 1) {
        fprintf(stderr, "vecdb embed: missing text argument\n");
        return 1;
    }
    text = argv[0];

    if (argc >= 2) {
        long v = strtol(argv[1], NULL, 10);
        if (v <= 0) {
            fprintf(stderr, "vecdb embed: dimension must be a positive integer\n");
            return 1;
        }
        dim = (uint32_t)v;
    }

    vec = malloc((size_t)dim * sizeof(float));
    if (vec == NULL) {
        fprintf(stderr, "vecdb embed: out of memory\n");
        return 1;
    }

    embed_tf(text, vec, dim, 0);

    for (i = 0; i < dim; i++) {
        if (vec[i] != 0.0f) {
            nonzero++;
        }
        norm += (double)vec[i] * (double)vec[i];
    }

    printf("Text:     \"%s\"\n", text);
    printf("Dim:      %u\n", dim);
    printf("Nonzero:  %u\n", nonzero);
    printf("L2 norm:  %.6f\n", sqrt(norm));
    printf("Buckets (up to %u shown):\n", PRINT_CAP);

    for (i = 0; i < dim && printed < PRINT_CAP; i++) {
        if (vec[i] != 0.0f) {
            printf("  [%u] %.6f\n", i, vec[i]);
            printed++;
        }
    }

    free(vec);
    return 0;
}

int main(int argc, char **argv) {
    const char *cmd;

    if (argc < 2) {
        usage(stderr, argv[0]);
        return 1;
    }

    cmd = argv[1];

    if (strcmp(cmd, "create") == 0) {
        return cmd_create(argc - 2, argv + 2);
    }
    if (strcmp(cmd, "stats") == 0) {
        return cmd_stats(argc - 2, argv + 2);
    }
    if (strcmp(cmd, "open") == 0) {
        return cmd_open(argc - 2, argv + 2);
    }
    if (strcmp(cmd, "embed") == 0) {
        return cmd_embed(argc - 2, argv + 2);
    }
    if (strcmp(cmd, "help") == 0) {
        usage(stdout, argv[0]);
        return 0;
    }
    if (strcmp(cmd, "version") == 0) {
        printf("%s\n", VECDB_VERSION_STRING);
        return 0;
    }

    fprintf(stderr, "vecdb: unknown command '%s'\n\n", cmd);
    usage(stderr, argv[0]);
    return 1;
}
