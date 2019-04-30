/**
 *
 * @file    worker
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-05-23 16:02:26
 */

#ifndef WORKER_H
#define WORKER_H

#include <pthread.h>

#include "common.h"

typedef int (job_runnable_t)(void *);

typedef struct _job_t {
    void            *ctx;
    job_runnable_t  *runnable;
    // used to release the resource holded by job
    deallocator_t   dealloc;

    struct _job_t   *next;
} job_t;

typedef struct {
    job_t           job_queue;
    volatile int    stopped;
    volatile int    job_count;

    pthread_t           thread;
    pthread_mutex_t     mu;
    pthread_cond_t      wait;
} worker_t;

worker_t* worker_create();
void worker_add_job(worker_t *w, job_t *job);
void worker_stop(worker_t *w);
void worker_destroy(worker_t *w);

#endif /* ifndef WORKER_H */
