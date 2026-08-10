#include "repl.h"
#include "store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

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

    if (vdb_load(path, &data) != 0) {
        return 1;
    }

    snprintf(prompt, sizeof(prompt), "vecdb(%s)> ", path);
    printf("vecdb session on '%s' (%u vectors, dim %u)\n",
           path, data.count, data.hdr.dim);
    printf("Type 'help' for commands, 'quit' or Ctrl-D to exit.\n");

    while ((line = readline(prompt)) != NULL) {
        char *cmd = line;

        while (*cmd == ' ' || *cmd == '\t') {
            cmd++;
        }

        if (*cmd == '\0') {
            free(line);
            continue;
        }

        add_history(cmd);

        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            free(line);
            break;
        } else if (strcmp(cmd, "help") == 0) {
            print_help();
        } else {
            printf("Unknown command. Type 'help'.\n");
        }

        free(line);
    }

    vdb_data_free(&data);
    return 0;
}
