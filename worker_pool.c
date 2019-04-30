/**
 *
 * @file    worker_pool
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-05-18 17:53:51
 */

#include <stdlib.h>

#include "worker_pool.h"
#include "logging.h"

worker_pool_t* worker_pool_create(int worker_count) {
    worker_pool_t *pool = malloc(sizeof(worker_pool_t));
    if (pool == NULL) {
        OCTOPUS_ERROR_LOG("failed to malloc for worker pool");
        return NULL;
    }

    pool->count = worker_count;
    pool->workers = malloc(sizeof(worker_t *) * worker_count);
    if (pool->workers == NULL) {
        OCTOPUS_ERROR_LOG("failed to malloc for workers");
        free(pool);
        return NULL;
    }

    for (int i = 0; i < worker_count; i++) {
        pool->workers[i] = worker_create();
        if (pool->workers[i] == NULL) {
            OCTOPUS_ERROR_LOG("failed to create worker");
            goto failed;
        }
    }

    return pool;

failed:
    for (int i = 0; i < worker_count; i++) {
        if (pool->workers[i] != NULL) {
            worker_destroy(pool->workers[i]);
        }
    }

    free(pool->workers);
    free(pool);

    return NULL;
}

void worker_pool_do(worker_pool_t *pool, job_t *job, int hash_id) {
    int     worker_id;

    worker_id = hash_id % pool->count;
    worker_t *w = pool->workers[worker_id];

    worker_add_job(w, job);
}

void worker_pool_destroy(worker_pool_t *pool) {
    for (int i = 0; i < pool->count; i++) {
        worker_stop(pool->workers[i]);
        worker_destroy(pool->workers[i]);
    }

    free(pool->workers);
    free(pool);
}

#ifdef OCTOPUS_TEST_WORKER_POOL

#include <stdio.h>
#include <unistd.h>

#include "common.h"

int test_run(void *arg) {
    int s = (int)arg;

    printf("start to do job@%d\n", s);
    for (int i = s; i < s + 10; i++) {
        printf("do %d-%d\n", s, i);
        sleep(1);
    }
}

int main(int argc, char *argv[])
{
    worker_pool_t   *pool;

    job_t       *job;

    pool = worker_pool_create(4);

    for (int i = 0; i < 3; ++i) {
        printf("add job %d\n", i);

        job = malloc(sizeof(job_t));
        job->runnable = test_run;
        job->ctx = (void *)i;
        job->dealloc = free_deallocator;

        worker_pool_do(pool, job, i);
    }

    sleep(10);

    worker_pool_destroy(pool);

    return 0;
}
#endif
