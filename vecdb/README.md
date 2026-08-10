# vecdb

A small vector search database written from scratch in C.

`vecdb` stores text as numeric vectors and answers "find the most similar
entries" queries. It has no external dependencies and talks to no network
services: the text-to-vector embedding, the on-disk format, the search, and
the concurrency are all implemented here in plain C.

The project reuses ideas from three systems assignments: a shell/REPL
front-end (process management), a thread pool for parallel search (threads,
mutexes, condition variables), and a custom binary on-disk format (file
systems).

## Status

Under active development. Current milestone: **M0 — storage skeleton**.

- [x] M0  on-disk `.vdb` format, `create` / `open` / `stats`
- [ ] M1  hashed TF-IDF embedder
- [ ] M2  single-threaded flat (exact) search
- [ ] M3  persistence of vectors + payloads, `mmap`
- [ ] M4  interactive REPL, background ingest, Ctrl-C
- [ ] M5  multithreaded search + benchmarks

## Build

```
make
```

Produces a single `vecdb` executable. `make clean` removes build artifacts.
Developed and tested on Linux.

## Usage (M0)

```
./vecdb create store.vdb 1024   # create an empty store with dimension 1024
./vecdb stats  store.vdb        # show information about the store
./vecdb open   store.vdb        # verify the file is a valid store
```

You can inspect the raw file with a hex viewer:

```
xxd store.vdb | head
```

## On-disk format

Every `.vdb` file begins with a fixed 512-byte header (a "superblock"),
followed by regions for the vectors, the original text payloads, an id map,
and the embedder's IDF weights. All integers are stored big-endian so the
file is portable and easy to inspect. See `src/store.h` for the exact layout.

## Layout

```
src/
  main.c    command-line entry point
  store.c   on-disk .vdb format (header read/write, create/open/stats)
  util.c    error helpers and big-endian serialization
```
