#ifndef VECDB_REPL_H
#define VECDB_REPL_H

/* Run an interactive session on the store at `path`: load it once, then read
   and execute commands until end of input (Ctrl-D) or `quit`. Returns 0 on
   success, non-zero if the store could not be opened. */
int repl_run(const char *path);

#endif /* VECDB_REPL_H */
