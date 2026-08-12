#include "pool.h"

#include <pthread.h>
#include <stdlib.h>

struct ThreadPool {
    int             nthreads;
    pthread_t      *threads;
    pthread_mutex_t mutex;
    pthread_cond_t  cond_work;   /* signaled when a new batch is posted   */
    pthread_cond_t  cond_done;   /* signaled as the batch drains          */

    task_fn         fn;          /* current batch's task function         */
    void          **args;        /* current batch's per-task arguments    */
    int             ntasks;      /* tasks in the current batch            */
    int             next;        /* next task index to claim              */
    int             active;      /* tasks not yet finished                */
    int             shutdown;    /* set by pool_destroy                   */
};

static void *worker(void *arg) {
    ThreadPool *p = (ThreadPool *)arg;

    for (;;) {
        int i;

        pthread_mutex_lock(&p->mutex);
        while (p->next >= p->ntasks && !p->shutdown) {
            pthread_cond_wait(&p->cond_work, &p->mutex);
        }
        if (p->shutdown) {
            pthread_mutex_unlock(&p->mutex);
            return NULL;
        }
        i = p->next++;
        pthread_mutex_unlock(&p->mutex);

        /* fn and args were published under the lock before the broadcast, so
           reading them here without the lock is safe for the batch's life. */
        p->fn(p->args[i]);

        pthread_mutex_lock(&p->mutex);
        p->active--;
        if (p->active == 0) {
            pthread_cond_signal(&p->cond_done);
        }
        pthread_mutex_unlock(&p->mutex);
    }
}

ThreadPool *pool_create(int nthreads) {
    ThreadPool *p;
    int         i;

    if (nthreads < 1) {
        nthreads = 1;
    }

    p = calloc(1, sizeof(*p));
    if (p == NULL) {
        return NULL;
    }

    p->threads = malloc((size_t)nthreads * sizeof(pthread_t));
    if (p->threads == NULL) {
        free(p);
        return NULL;
    }

    pthread_mutex_init(&p->mutex, NULL);
    pthread_cond_init(&p->cond_work, NULL);
    pthread_cond_init(&p->cond_done, NULL);
    p->nthreads = nthreads;

    for (i = 0; i < nthreads; i++) {
        if (pthread_create(&p->threads[i], NULL, worker, p) != 0) {
            /* Join the workers already created, then tear everything down. */
            p->nthreads = i;
            pool_destroy(p);
            return NULL;
        }
    }

    return p;
}

void pool_run(ThreadPool *p, task_fn fn, void **args, int count) {
    if (count <= 0) {
        return;
    }

    pthread_mutex_lock(&p->mutex);
    p->fn     = fn;
    p->args   = args;
    p->ntasks = count;
    p->next   = 0;
    p->active = count;
    pthread_cond_broadcast(&p->cond_work);

    while (p->active > 0) {
        pthread_cond_wait(&p->cond_done, &p->mutex);
    }
    pthread_mutex_unlock(&p->mutex);
}

int pool_size(const ThreadPool *p) {
    return p->nthreads;
}

void pool_destroy(ThreadPool *p) {
    int i;

    if (p == NULL) {
        return;
    }

    pthread_mutex_lock(&p->mutex);
    p->shutdown = 1;
    pthread_cond_broadcast(&p->cond_work);
    pthread_mutex_unlock(&p->mutex);

    for (i = 0; i < p->nthreads; i++) {
        pthread_join(p->threads[i], NULL);
    }

    pthread_mutex_destroy(&p->mutex);
    pthread_cond_destroy(&p->cond_work);
    pthread_cond_destroy(&p->cond_done);

    free(p->threads);
    free(p);
}
