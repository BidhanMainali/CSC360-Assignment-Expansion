#include "store.h"
#include "embed.h"
#include "index.h"
#include "repl.h"
#include "search.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
        "  add    <file.vdb> \"<text>\" embed text and add it to a store\n"
        "  search <file.vdb> \"<q>\" [k] [--threads N]  find similar entries\n"
        "  addfile <file.vdb> <text> add each paragraph of a text file\n"
        "  repl   <file.vdb>         open an interactive session\n"
        "  bench  <file.vdb> [nq] [threads]       benchmark search on a store\n"
        "  bench  --gen <count> <dim> [nq] [thr]  benchmark synthetic data\n"
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

static int cmd_add(int argc, char **argv) {
    VdbData     data;
    const char *path;
    const char *text;
    float      *vec;
    uint32_t    dim;

    if (argc < 2) {
        fprintf(stderr, "vecdb add: usage: add <file.vdb> \"<text>\"\n");
        return 1;
    }
    path = argv[0];
    text = argv[1];

    if (vdb_load(path, &data) != 0) {
        return 1;   /* vdb_load already reported the reason */
    }

    dim = data.hdr.dim;
    vec = malloc((size_t)dim * sizeof(float));
    if (vec == NULL) {
        fprintf(stderr, "vecdb add: out of memory\n");
        vdb_data_free(&data);
        return 1;
    }

    embed_tf(text, vec, dim, data.hdr.hash_seed);

    if (vdb_data_add(&data, vec, text) != 0) {
        fprintf(stderr, "vecdb add: out of memory\n");
        free(vec);
        vdb_data_free(&data);
        return 1;
    }
    free(vec);

    if (vdb_build_index(&data) != 0) {
        fprintf(stderr, "vecdb add: out of memory building index\n");
        vdb_data_free(&data);
        return 1;
    }

    if (vdb_write(path, &data) != 0) {
        vdb_data_free(&data);
        return 1;
    }

    printf("Added to '%s' (now %u vectors)\n", path, data.count);
    vdb_data_free(&data);
    return 0;
}

static int cmd_search(int argc, char **argv) {
    VdbData     data;
    const char *path;
    const char *query;
    uint32_t    k = 5;
    int         nthreads = 1;
    Hit        *hits;
    uint32_t    nhits = 0;
    uint32_t    i;
    int         ai;
    int         rc;

    if (argc < 2) {
        fprintf(stderr,
            "vecdb search: usage: search <file.vdb> \"<query>\" [k] [--threads N]\n");
        return 1;
    }
    path  = argv[0];
    query = argv[1];

    /* Remaining args are an optional k and/or "--threads N", in any order. */
    for (ai = 2; ai < argc; ai++) {
        if (strcmp(argv[ai], "--threads") == 0 && ai + 1 < argc) {
            nthreads = atoi(argv[ai + 1]);
            ai++;
        } else {
            long v = strtol(argv[ai], NULL, 10);
            if (v <= 0) {
                fprintf(stderr, "vecdb search: k must be a positive integer\n");
                return 1;
            }
            k = (uint32_t)v;
        }
    }
    if (nthreads < 1) {
        nthreads = 1;
    }

    if (vdb_load(path, &data) != 0) {
        return 1;
    }

    if (data.count == 0) {
        printf("Store '%s' is empty.\n", path);
        vdb_data_free(&data);
        return 0;
    }

    hits = malloc((size_t)k * sizeof(Hit));
    if (hits == NULL) {
        fprintf(stderr, "vecdb search: out of memory\n");
        vdb_data_free(&data);
        return 1;
    }

    if (nthreads > 1) {
        ThreadPool *pool = pool_create(nthreads);

        if (pool == NULL) {
            fprintf(stderr, "vecdb search: could not create thread pool\n");
            free(hits);
            vdb_data_free(&data);
            return 1;
        }
        rc = vdb_search_mt(&data, query, k, hits, &nhits, pool);
        pool_destroy(pool);
    } else {
        rc = vdb_search(&data, query, k, hits, &nhits);
    }

    if (rc != 0) {
        fprintf(stderr, "vecdb search: out of memory\n");
        free(hits);
        vdb_data_free(&data);
        return 1;
    }

    printf("Query: \"%s\"\n", query);
    printf("Top %u of %u:\n", nhits, data.count);
    for (i = 0; i < nhits; i++) {
        printf("  %.4f  %s\n", hits[i].score, data.payloads[hits[i].index]);
    }

    free(hits);
    vdb_data_free(&data);
    return 0;
}

/* Split `text` into paragraphs (runs of text separated by blank lines),
   embed each one, and append it to `data`. `text` is modified in place.
   Returns 0 on success, -1 on allocation failure; *added_out gets the number
   of paragraphs ingested. */
static int ingest_text(VdbData *data, char *text, uint32_t *added_out) {
    uint32_t dim   = data->hdr.dim;
    uint32_t added = 0;
    float   *vec   = malloc((size_t)dim * sizeof(float));
    char    *p     = text;
    int      rc    = 0;

    if (vec == NULL) {
        return -1;
    }

    while (*p != '\0') {
        char *para;
        char *q;

        /* Skip a run of blank lines and leading whitespace. */
        while (*p == '\n' || *p == '\r' || *p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '\0') {
            break;
        }

        para = p;

        /* Advance to the end of this paragraph: a blank line (a newline
           followed, after optional whitespace, by another newline) or EOF. */
        while (*p != '\0') {
            if (*p == '\n') {
                char *look = p + 1;

                while (*look == '\r' || *look == ' ' || *look == '\t') {
                    look++;
                }
                if (*look == '\n' || *look == '\0') {
                    break;
                }
            }
            p++;
        }

        if (*p != '\0') {
            *p = '\0';
            p++;
        }

        /* Fold the paragraph's internal newlines into spaces so it stores
           and prints as a single line. */
        for (q = para; *q != '\0'; q++) {
            if (*q == '\n' || *q == '\r') {
                *q = ' ';
            }
        }

        embed_tf(para, vec, dim, data->hdr.hash_seed);
        if (vdb_data_add(data, vec, para) != 0) {
            rc = -1;
            break;
        }
        added++;
    }

    free(vec);
    *added_out = added;
    return rc;
}

static int cmd_addfile(int argc, char **argv) {
    VdbData     data;
    const char *path;
    const char *textfile;
    char       *text;
    FILE       *f;
    long        len;
    uint32_t    added = 0;

    if (argc < 2) {
        fprintf(stderr, "vecdb addfile: usage: addfile <file.vdb> <textfile>\n");
        return 1;
    }
    path     = argv[0];
    textfile = argv[1];

    /* Read the whole text file into memory. */
    f = fopen(textfile, "rb");
    if (f == NULL) {
        fprintf(stderr, "vecdb addfile: cannot open '%s'\n", textfile);
        return 1;
    }
    if (fseek(f, 0, SEEK_END) != 0 || (len = ftell(f)) < 0) {
        fprintf(stderr, "vecdb addfile: cannot size '%s'\n", textfile);
        fclose(f);
        return 1;
    }
    rewind(f);

    text = malloc((size_t)len + 1);
    if (text == NULL) {
        fprintf(stderr, "vecdb addfile: out of memory\n");
        fclose(f);
        return 1;
    }
    if (fread(text, 1, (size_t)len, f) != (size_t)len) {
        fprintf(stderr, "vecdb addfile: failed to read '%s'\n", textfile);
        free(text);
        fclose(f);
        return 1;
    }
    text[len] = '\0';
    fclose(f);

    if (vdb_load(path, &data) != 0) {
        free(text);
        return 1;
    }

    if (ingest_text(&data, text, &added) != 0) {
        fprintf(stderr, "vecdb addfile: out of memory while ingesting\n");
        free(text);
        vdb_data_free(&data);
        return 1;
    }
    free(text);

    if (added == 0) {
        printf("No paragraphs found in '%s'.\n", textfile);
        vdb_data_free(&data);
        return 0;
    }

    if (vdb_build_index(&data) != 0) {
        fprintf(stderr, "vecdb addfile: out of memory building index\n");
        vdb_data_free(&data);
        return 1;
    }

    if (vdb_write(path, &data) != 0) {
        vdb_data_free(&data);
        return 1;
    }

    printf("Added %u paragraph(s) from '%s' to '%s' (now %u vectors)\n",
           added, textfile, path, data.count);
    vdb_data_free(&data);
    return 0;
}

static double now_seconds(void) {
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/* Run the timing table over an already-populated dataset. */
static void run_benchmark(VdbData *data, uint32_t nqueries, int maxthreads) {
    const uint32_t k     = 5;
    const char    *query = "vector database benchmark query terms";
    Hit           *hits;
    double         base_time = 0.0;
    int            t;

    hits = malloc((size_t)k * sizeof(Hit));
    if (hits == NULL) {
        fprintf(stderr, "vecdb bench: out of memory\n");
        return;
    }

    printf("Benchmark: %u vectors, dim %u, %u queries per run\n\n",
           data->count, data->hdr.dim, nqueries);
    printf("%-9s %-9s %-10s %-12s %s\n",
           "Threads", "Queries", "Time(s)", "QPS", "Speedup");

    for (t = 1; t <= maxthreads; t *= 2) {
        ThreadPool *pool = NULL;
        double      t0;
        double      elapsed;
        uint32_t    q;
        uint32_t    nh;

        if (t > 1) {
            pool = pool_create(t);
            if (pool == NULL) {
                fprintf(stderr, "vecdb bench: could not create thread pool\n");
                break;
            }
        }

        t0 = now_seconds();
        for (q = 0; q < nqueries; q++) {
            if (pool != NULL) {
                vdb_search_mt(data, query, k, hits, &nh, pool);
            } else {
                vdb_search(data, query, k, hits, &nh);
            }
        }
        elapsed = now_seconds() - t0;

        if (t == 1) {
            base_time = elapsed;
        }

        printf("%-9d %-9u %-10.3f %-12.0f %.2fx\n",
               t, nqueries, elapsed,
               elapsed > 0.0 ? (double)nqueries / elapsed : 0.0,
               elapsed > 0.0 ? base_time / elapsed : 0.0);

        if (pool != NULL) {
            pool_destroy(pool);
        }
    }

    free(hits);
}

/* Build a synthetic in-memory dataset of random vectors for benchmarking. */
static int build_synthetic(VdbData *data, uint32_t count, uint32_t dim) {
    size_t total = (size_t)count * dim;
    size_t idx;

    memset(data, 0, sizeof(*data));
    data->hdr.dim        = dim;
    data->hdr.metric     = VDB_METRIC_COSINE;
    data->hdr.index_type = VDB_INDEX_FLAT;
    data->count = count;
    data->cap   = count;

    data->vectors  = malloc(total * sizeof(float));
    data->payloads = calloc(count, sizeof(char *));

    if (data->vectors == NULL || data->payloads == NULL) {
        free(data->vectors);
        free(data->payloads);
        return -1;
    }

    srand(12345u);
    for (idx = 0; idx < total; idx++) {
        data->vectors[idx] = (float)rand() / (float)RAND_MAX * 2.0f - 1.0f;
    }

    return 0;
}

static int cmd_bench(int argc, char **argv) {
    VdbData  data;
    uint32_t nqueries   = 200;
    int      maxthreads = 8;

    if (argc >= 1 && strcmp(argv[0], "--gen") == 0) {
        /* Synthetic:  bench --gen <count> <dim> [nqueries] [threads] */
        uint32_t count = 50000;
        uint32_t dim   = 256;

        if (argc >= 2) { long v = strtol(argv[1], NULL, 10); if (v > 0) count = (uint32_t)v; }
        if (argc >= 3) { long v = strtol(argv[2], NULL, 10); if (v > 0) dim = (uint32_t)v; }
        if (argc >= 4) { long v = strtol(argv[3], NULL, 10); if (v > 0) nqueries = (uint32_t)v; }
        if (argc >= 5) { long v = strtol(argv[4], NULL, 10); if (v > 0) maxthreads = (int)v; }

        if (build_synthetic(&data, count, dim) != 0) {
            fprintf(stderr, "vecdb bench: out of memory (try a smaller count/dim)\n");
            return 1;
        }
    } else if (argc >= 1) {
        /* Real store:  bench <file.vdb> [nqueries] [threads] */
        if (argc >= 2) { long v = strtol(argv[1], NULL, 10); if (v > 0) nqueries = (uint32_t)v; }
        if (argc >= 3) { long v = strtol(argv[2], NULL, 10); if (v > 0) maxthreads = (int)v; }

        if (vdb_load(argv[0], &data) != 0) {
            return 1;
        }
        if (data.count == 0) {
            printf("Store '%s' is empty; nothing to benchmark.\n", argv[0]);
            vdb_data_free(&data);
            return 0;
        }
    } else {
        fprintf(stderr,
            "vecdb bench: usage:\n"
            "  bench <file.vdb> [nqueries] [threads]\n"
            "  bench --gen <count> <dim> [nqueries] [threads]\n");
        return 1;
    }

    run_benchmark(&data, nqueries, maxthreads);
    vdb_data_free(&data);
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
    if (strcmp(cmd, "add") == 0) {
        return cmd_add(argc - 2, argv + 2);
    }
    if (strcmp(cmd, "search") == 0) {
        return cmd_search(argc - 2, argv + 2);
    }
    if (strcmp(cmd, "addfile") == 0) {
        return cmd_addfile(argc - 2, argv + 2);
    }
    if (strcmp(cmd, "repl") == 0) {
        if (argc < 3) {
            fprintf(stderr, "vecdb repl: usage: repl <file.vdb>\n");
            return 1;
        }
        return repl_run(argv[2]);
    }
    if (strcmp(cmd, "bench") == 0) {
        return cmd_bench(argc - 2, argv + 2);
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
