#include "repl.h"
#include "store.h"
#include "search.h"
#include "embed.h"
#include "index.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <readline/readline.h>
#include <readline/history.h>

/* Jump target used to abort the current input line on Ctrl-C and return to a
   fresh prompt instead of terminating the session. `in_readline` gates the
   jump so a signal that arrives while a command is running is ignored. */
static sigjmp_buf            ctrlc_env;
static volatile sig_atomic_t in_readline = 0;

static void on_sigint(int sig) {
    (void)sig;
    if (in_readline) {
        siglongjmp(ctrlc_env, 1);
    }
}

static void print_help(void) {
    printf("Commands:\n"
           "  add <text...>       add a document\n"
           "  search <query...>   search for similar documents\n"
           "  k <n>               set the number of results (default 5)\n"
           "  stats               show store information\n"
           "  save                write changes back to the file\n"
           "  help                show this message\n"
           "  quit                save and exit (also Ctrl-D)\n");
}

int repl_run(const char *path) {
    VdbData  data;
    char     prompt[256];
    char    *line;
    uint32_t k = 5;
    int      dirty = 0;

    if (vdb_load(path, &data) != 0) {
        return 1;
    }

    snprintf(prompt, sizeof(prompt), "vecdb(%s)> ", path);
    printf("vecdb session on '%s' (%u vectors, dim %u)\n",
           path, data.count, data.hdr.dim);
    printf("Type 'help' for commands, 'quit' or Ctrl-D to exit.\n");

    signal(SIGINT, on_sigint);

    while (1) {
        char *cmd;
        char *word;
        char *arg;

        /* Ctrl-C during readline jumps back here: drop the line and re-prompt. */
        if (sigsetjmp(ctrlc_env, 1) != 0) {
            in_readline = 0;
            printf("\n");
        }

        in_readline = 1;
        line = readline(prompt);
        in_readline = 0;

        if (line == NULL) {   /* EOF (Ctrl-D) */
            break;
        }

        cmd = line;
        while (*cmd == ' ' || *cmd == '\t') {
            cmd++;
        }

        if (*cmd == '\0') {
            free(line);
            continue;
        }

        add_history(cmd);

        /* Split the line into a command word and the rest-of-line argument. */
        word = cmd;
        arg  = cmd;
        while (*arg != '\0' && *arg != ' ' && *arg != '\t') {
            arg++;
        }
        if (*arg != '\0') {
            *arg = '\0';
            arg++;
            while (*arg == ' ' || *arg == '\t') {
                arg++;
            }
        }

        if (strcmp(word, "quit") == 0 || strcmp(word, "exit") == 0) {
            free(line);
            break;
        } else if (strcmp(word, "help") == 0) {
            print_help();
        } else if (strcmp(word, "stats") == 0) {
            printf("Store:      %s\n", path);
            printf("Vectors:    %u\n", data.count);
            printf("Dimension:  %u\n", data.hdr.dim);
            printf("IDF:        %s\n", data.idf != NULL ? "built" : "none");
        } else if (strcmp(word, "k") == 0) {
            long v = strtol(arg, NULL, 10);

            if (v <= 0) {
                printf("k must be a positive integer.\n");
            } else {
                k = (uint32_t)v;
                printf("k = %u\n", k);
            }
        } else if (strcmp(word, "search") == 0) {
            if (*arg == '\0') {
                printf("Usage: search <query...>\n");
            } else if (data.count == 0) {
                printf("Store is empty.\n");
            } else {
                Hit     *hits  = malloc((size_t)k * sizeof(Hit));
                uint32_t nhits = 0;

                if (hits == NULL) {
                    printf("Out of memory.\n");
                } else if (vdb_search(&data, arg, k, hits, &nhits) != 0) {
                    printf("Out of memory.\n");
                    free(hits);
                } else {
                    uint32_t r;

                    printf("Top %u of %u:\n", nhits, data.count);
                    for (r = 0; r < nhits; r++) {
                        printf("  %.4f  %s\n", hits[r].score,
                               data.payloads[hits[r].index]);
                    }
                    free(hits);
                }
            }
        } else if (strcmp(word, "add") == 0) {
            if (*arg == '\0') {
                printf("Usage: add <text...>\n");
            } else {
                float *vec = malloc((size_t)data.hdr.dim * sizeof(float));

                if (vec == NULL) {
                    printf("Out of memory.\n");
                } else {
                    embed_tf(arg, vec, data.hdr.dim, data.hdr.hash_seed);

                    if (vdb_data_add(&data, vec, arg) != 0) {
                        printf("Out of memory.\n");
                    } else if (vdb_build_index(&data) != 0) {
                        printf("Out of memory building index.\n");
                    } else {
                        dirty = 1;
                        printf("Added (now %u vectors).\n", data.count);
                    }
                    free(vec);
                }
            }
        } else if (strcmp(word, "save") == 0) {
            if (!dirty) {
                printf("No changes to save.\n");
            } else if (vdb_write(path, &data) != 0) {
                printf("Save failed.\n");
            } else {
                dirty = 0;
                printf("Saved '%s'.\n", path);
            }
        } else {
            printf("Unknown command. Type 'help'.\n");
        }

        free(line);
    }

    /* Persist interactive changes on the way out (quit or Ctrl-D). */
    if (dirty) {
        if (vdb_write(path, &data) == 0) {
            printf("Saved changes to '%s'.\n", path);
        } else {
            fprintf(stderr, "vecdb: failed to save '%s'\n", path);
        }
    }

    vdb_data_free(&data);
    return 0;
}
