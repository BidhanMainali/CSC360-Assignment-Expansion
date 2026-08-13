# Operating Systems Projects & vecdb

This repository collects my CSC 360 (Operating Systems) course projects and
**vecdb** — a vector search database I built from scratch in C that ties their
ideas together.

## ⭐ vecdb — vector search database

[`vecdb/`](vecdb) is the highlight: a self-contained vector database in C with a
custom on-disk format, a built-in TF-IDF embedder, an interactive REPL, and
multithreaded search that scales ~3.9× on 4 cores. No external dependencies, no
network services — the embedding, storage, search, and concurrency are all
implemented from scratch.

Full documentation is in [`vecdb/README.md`](vecdb/README.md). Quick taste:

```
cd vecdb && make
./vecdb create store.vdb 1024
./vecdb addfile store.vdb notes.txt
./vecdb search store.vdb "a query" 5
```

## Course projects

| Folder | Project | Summary |
|--------|---------|---------|
| [`p1/`](p1) | SSI — Simple Shell Interpreter | A shell with foreground/background execution, `cd`, job listing, and signal handling (`fork` / `execvp` / `waitpid`). |
| [`p2/`](p2) | MTS — Multi-Thread Scheduling | A train-crossing simulator that schedules threads over a shared track using mutexes and condition variables. |
| [`p3/`](p3) | SFS — Simple File System | Utilities that read and modify a FAT-style disk image: `diskinfo`, `disklist`, `diskget`, `diskput`. |

`assignment_specs/` contains the original assignment specifications.

## How they connect

vecdb grew out of these projects and combines their core ideas into one system:

- **P1 (shell)** → vecdb's interactive REPL: a readline command loop, argument
  parsing, and Ctrl-C handling.
- **P2 (threads)** → vecdb's worker thread pool for parallel search (mutex +
  condition variables).
- **P3 (file system)** → vecdb's custom binary `.vdb` on-disk format.

## Building

Every project is written in C and builds independently with `make` in its
folder. Developed and tested on Linux (`linux.csc.uvic.ca`).
```
