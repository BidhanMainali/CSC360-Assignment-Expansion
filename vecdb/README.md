# vecdb

A small vector search database written from scratch in C.

`vecdb` stores text as numeric vectors and answers "find the most similar
entries" queries. It has no external dependencies and talks to no network
services: the text-to-vector embedding, the on-disk format, and the search are
all implemented here in plain C.

The project reuses ideas from three systems assignments: a shell/REPL
front-end (process management), a thread pool for parallel search (threads,
mutexes, condition variables), and a custom binary on-disk format (file
systems).

## Status

Current release: **v0.2.0** — single-threaded vector store with TF-IDF ranking.

- [x] M0  on-disk `.vdb` format, `create` / `open` / `stats`
- [x] M1  hashed term-frequency embedder, `embed`
- [x] M2  single-threaded flat (exact) search: `add`, `search`, `addfile`
- [x]     IDF weighting (TF -> TF-IDF)
- [ ] M3  memory-mapped vector loads
- [ ] M4  interactive REPL, background ingest, Ctrl-C
- [ ] M5  multithreaded search + benchmarks

## Build

```
make
```

Produces a single `vecdb` executable. `make clean` removes build artifacts.
Developed and tested on Linux.

## Usage

```
./vecdb create  store.vdb 1024          # create an empty store (dimension 1024)
./vecdb add     store.vdb "some text"   # embed text and add it
./vecdb addfile store.vdb notes.txt     # add each paragraph of a text file
./vecdb search  store.vdb "a query" 5   # print the 5 most similar entries
./vecdb stats   store.vdb               # show information about the store
./vecdb embed   "some text" 16          # inspect the raw embedding vector
```

You can inspect the raw file with a hex viewer:

```
xxd store.vdb | head
```

## On-disk format

Every `.vdb` file begins with a fixed 512-byte header (a "superblock"),
followed by the vectors (a contiguous block of float32), the original text
payloads (each length-prefixed), and the embedder's IDF weights. The header
also reserves a region for an id map used by later versions. Integers are
stored big-endian so the file is easy to inspect; see `src/store.h` for the
exact layout.

## Layout

```
src/
  main.c    command-line entry point and command dispatch
  store.c   on-disk .vdb format and in-memory dataset (load/write/add)
  embed.c   tokenizer and hashing TF / TF-IDF vectorizer
  index.c   computes IDF weights and re-embeds vectors as TF-IDF
  util.c    error helpers, big-endian serialization, FNV-1a hash
```
