/**
 *
 * @file    worker
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-05-23 16:08:19
 */

#include <stdlib.h>
#include <string.h>

#include "worker.h"
#include "logging.h"
#include "common.h"

void* worker_run(void *arg) {
    job_t       *job;
    worker_t    *w;

    w = (worker_t *)arg;
    while (w->stopped == OCTOPUS_FALSE) {
        // Try to acquire a job from the queue.
        pthread_mutex_lock(&w->mu);
        while (w->job_queue.next == NULL) {
            // The queue is empty, wait until new jobs coming.
            pthread_cond_wait(&w->wait, &w->mu);
        }

        job = w->job_queue.next;
        w->job_queue.next = job->next;
        w->job_count--;

        pthread_mutex_unlock(&w->mu);

        // Run the job
        if (job->runnable(job->ctx) == OCTOPUS_ERR) {
            OCTOPUS_ERROR_LOG("failed to run a job");
        }

        if (job->dealloc != NULL) {
            job->dealloc(job);
        }
    }

    return NULL;
}

worker_t* worker_create() {
    worker_t *w = malloc(sizeof(worker_t));
    if (w == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for worker");
        return NULL;
    }

    bzero(w, sizeof(worker_t));
    w->stopped = OCTOPUS_FALSE;

    if (pthread_create(&w->thread, NULL, worker_run, w) != 0) {
        OCTOPUS_ERROR_LOG_BY_ERRNO("failed to create thread");
        goto failed;
    }

    if (pthread_mutex_init(&w->mu, NULL) != 0) {
        OCTOPUS_ERROR_LOG_BY_ERRNO("failed to init mutex");
        goto failed;
    }

    if (pthread_cond_init(&w->wait, NULL) != 0) {
        OCTOPUS_ERROR_LOG_BY_ERRNO("failed to init condition");
        goto failed;
    }

    return w;

failed:
    // Ignore the error here.
    pthread_cancel(w->thread);
    pthread_mutex_destroy(&w->mu);
    pthread_cond_destroy(&w->wait);
    free(w);

    return NULL;
}

void worker_add_job(worker_t *w, job_t *job) {
    pthread_mutex_lock(&w->mu);

    // TODO: add the tail of the queue
    job->next = w->job_queue.next;
    w->job_queue.next = job;

    pthread_mutex_unlock(&w->mu);
    pthread_cond_signal(&w->wait);
}

void worker_stop(worker_t *w) {
    w->stopped = OCTOPUS_TRUE;
}

void worker_destroy(worker_t *w) {
    worker_stop(w);
    pthread_mutex_destroy(&w->mu);
    pthread_cond_destroy(&w->wait);
    free(w);
}

#ifdef OCTOPUS_TEST_WORKER

#include <unistd.h>
#include <stdio.h>

int test_run(void *arg) {
    int s = (int)arg;

    printf("start to do job@%d\n", s);
    for (int i = s; i < s + 10; i++) {
        printf("do %d-%d\n", s, i);
    }
    sleep(1);
}

int main() {
    worker_t    *w;
    job_t       job;

    w = worker_create();

    for (int i = 0; i < 3; i++) {
        printf("add job %d\n", i);
        job = malloc(sizeof(job_t));
        job->runnable = test_run;
        job->ctx = (void *)i;
        job->dealloc = free_deallocator;

        worker_add_job(w, job);
    }

    sleep(5);
    worker_stop(w);
}

#endif
