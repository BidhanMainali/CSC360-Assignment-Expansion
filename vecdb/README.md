# vecdb

A small vector search database written from scratch in C — no external
dependencies, no network services. `vecdb` turns text into numeric vectors,
stores them in its own on-disk format, and answers "find the most similar
entries" queries, with an interactive shell and multithreaded search.

## How it works

`vecdb` is a complete similarity-search pipeline, all implemented in plain C:

1. **Embedding.** Each piece of text becomes a fixed-length vector via a hashing
   TF-IDF vectorizer: words are tokenized, hashed into buckets (the "hashing
   trick", with a sign bit to cancel collision bias), weighted by inverse
   document frequency so rare words count more, and the vector is L2-normalized.
   Because vectors are normalized, cosine similarity reduces to a dot product.

2. **Storage.** Vectors and their original text live in a single `.vdb` file: a
   512-byte header, a contiguous block of `float32` vectors, the length-prefixed
   text payloads, and the IDF weights. All integers are big-endian, so the file
   is portable and easy to inspect with `xxd`.

3. **Search.** A query is embedded the same way, then scored against every
   stored vector by dot product. The best *k* results are kept in a small top-k
   heap and returned with their text.

4. **Parallelism.** Search can run across a pool of worker threads: the vector
   range is split into chunks, each worker builds a *private* top-k list (no
   locks on the hot path), and the main thread merges them. This scales
   near-linearly with CPU cores.

There are no ML models and no API calls. The embeddings are purely lexical —
they match on shared words, not meaning — which keeps the system self-contained
and fast.

## Build

```
make
```

Produces a single `vecdb` executable. `make clean` removes build artifacts.
Requires the GNU readline library. Developed and tested on Linux.

## Commands

```
vecdb <command> [arguments]
```

| Command | Description |
|---------|-------------|
| `create <file.vdb> [dim]` | Create a new, empty store (dimension defaults to 1024). |
| `add <file.vdb> "<text>"` | Embed a string and add it to the store. |
| `addfile <file.vdb> <textfile>` | Add each paragraph (blank-line separated) of a text file. |
| `search <file.vdb> "<query>" [k] [--threads N]` | Print the `k` most similar entries (default 5), optionally using `N` threads. |
| `stats <file.vdb>` | Show store info (dimension, vector count, file size, ...). |
| `open <file.vdb>` | Validate that a file is a well-formed store. |
| `embed "<text>" [dim]` | Print a raw embedding vector for inspection. |
| `repl <file.vdb>` | Open an interactive session (see below). |
| `bench <file.vdb> [nq] [threads]` | Benchmark search speed on a real store. |
| `bench --gen <count> <dim> [nq] [threads]` | Benchmark on a synthetic dataset. |

Example:

```
./vecdb create notes.vdb 1024
./vecdb addfile notes.vdb README.md
./vecdb search notes.vdb "how do I build this project" 5
./vecdb search notes.vdb "how do I build this project" 5 --threads 4
```

## Interactive session (REPL)

`./vecdb repl <file.vdb>` loads the store once and keeps it in memory, so
commands run without re-reading the file each time:

```
vecdb(notes.vdb)> add machine learning is fun
Added (now 19 vectors).
vecdb(notes.vdb)> search learning
Top 5 of 19:
  0.5123  machine learning is fun
  ...
vecdb(notes.vdb)> k 3          # change the number of results
vecdb(notes.vdb)> save         # write changes back to the file
vecdb(notes.vdb)> quit         # also auto-saves on exit
```

- Command history with the up/down arrows.
- **Ctrl-C** aborts the current line and returns to a fresh prompt without
  exiting the session.
- **Ctrl-D** (or `quit`) exits, auto-saving any unsaved additions.

## Benchmarks

Multithreaded search scales near-linearly with the number of cores. Measured on
Linux with `./vecdb bench --gen 100000 512 100 8` (100,000 vectors, dim 512):

| Threads | Time (s) | Queries/sec | Speedup |
|--------:|---------:|------------:|--------:|
| 1 | 5.57 | 18 | 1.00x |
| 2 | 2.79 | 36 | 2.00x |
| 4 | 1.41 | 71 | 3.94x |
| 8 | 1.43 | 70 | 3.88x |

Speedup is essentially linear through 4 threads, then plateaus, matching the
~4 cores available to the process. Use `bench <file.vdb>` to time your own store,
or `bench --gen <count> <dim>` for a guaranteed-large synthetic dataset.

## On-disk format (`.vdb`)

```
+-----------------------------------------------------------+
| Header (512 bytes): magic "VECDBFS1", version, dimension, |
|   vector count, metric, index type, hash seed, and the    |
|   offset + size of each region below                      |
+-----------------------------------------------------------+
| Vectors:  count x dim float32, contiguous (row-major)     |
+-----------------------------------------------------------+
| Payloads: each entry's original text, length-prefixed     |
+-----------------------------------------------------------+
| IDF:      dim float32 (the embedder's weights)            |
+-----------------------------------------------------------+
```

All multi-byte integers are big-endian. The header also reserves a region for an
id map used by future features. See `src/store.h` for the exact field layout.

## Codebase

Each source file is a focused module:

| File | Responsibility |
|------|----------------|
| `src/main.c` | CLI entry point: parses the subcommand and arguments, dispatches to a handler, and implements `bench`. |
| `src/store.h` / `src/store.c` | The `.vdb` on-disk format and the in-memory `VdbData` model — read/write the header, load/save a dataset, append records. |
| `src/embed.h` / `src/embed.c` | Tokenizer and the hashing TF / TF-IDF vectorizer. |
| `src/index.h` / `src/index.c` | Computes IDF weights from the corpus and re-embeds every vector as TF-IDF. |
| `src/search.h` / `src/search.c` | Cosine scoring, the top-k heap, and both single-threaded (`vdb_search`) and parallel (`vdb_search_mt`) search. |
| `src/pool.h` / `src/pool.c` | A fixed-size worker thread pool (mutex + condition variables) that runs a batch of tasks in parallel. |
| `src/repl.h` / `src/repl.c` | The interactive session: readline loop, command parsing, Ctrl-C handling, auto-save. |
| `src/util.h` / `src/util.c` | Error/allocation helpers, big-endian serialization, and the FNV-1a hash. |

How the pieces fit together:

```
  CLI  (main.c) ─┐
                 ├─► store.c ──► embed.c + index.c   (text  → vectors)
  REPL (repl.c) ─┘      │
                        └─────► search.c ──► pool.c  (query → top-k, in parallel)
```

`main.c` (one-shot CLI commands) and `repl.c` (interactive) drive the same core:
`store.c` holds the data, `embed.c`/`index.c` produce the vectors, and `search.c`
(optionally using `pool.c`) answers queries.

## Limitations

- Embeddings are **lexical** (hashing TF-IDF): they match on shared words, not
  meaning, so synonyms won't match unless they share tokens.
- Search is **exact/flat** — every query scans every vector. That is fast and
  parallel, but linear in the number of vectors.

## Possible next steps

- An approximate nearest-neighbor index (e.g. IVF) for sub-linear search.
- Memory-mapped vector loads to avoid reading the whole file into RAM.
