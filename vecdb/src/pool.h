#ifndef VECDB_POOL_H
#define VECDB_POOL_H

/* A fixed-size pool of worker threads for running a batch of independent
   tasks in parallel (a "parallel for"). */
typedef struct ThreadPool ThreadPool;

/* A task: called with one element of the `args` array passed to pool_run. */
typedef void (*task_fn)(void *arg);

/* Create a pool with `nthreads` workers (clamped to >= 1). Returns NULL on
   failure. */
ThreadPool *pool_create(int nthreads);

/* Run `count` tasks -- fn(args[0]) .. fn(args[count-1]) -- across the pool's
   workers and block until all of them have finished. */
void pool_run(ThreadPool *pool, task_fn fn, void **args, int count);

/* Number of worker threads in the pool. */
int pool_size(const ThreadPool *pool);

/* Stop the workers and free the pool. */
void pool_destroy(ThreadPool *pool);

#endif /* VECDB_POOL_H */
