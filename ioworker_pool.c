/**
 *
 * @file    ioworker_pool
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2019-04-30 11:13:21
 */

#include <stdlib.h>

#include "ioworker_pool.h"
#include "ioworker.h"
#include "logging.h"

struct ioworker_pool_s {
    ioworker_t  **workers;
    int         worker_count;
};

ioworker_pool_t* ioworker_pool_create(int size) {
    ioworker_pool_t     *pool;

    if ((pool = calloc(1, sizeof(ioworker_pool_t))) == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for ioworker pool");
        return NULL;
    }

    if ((pool->workers = calloc(size, sizeof(void *))) == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for ioworkers");
        goto failed;
    }

    pool->worker_count = size;
    for (int i = 0; i < size; i++) {
        if ((pool->workers[i] = ioworker_create()) == NULL) {
            OCTOPUS_ERROR_LOG("failed to create ioworker for pool");
            goto failed;
        }
    }

    return pool;

failed:
    if (pool->workers) {
        for (int i = 0; i < size; i++) {
            if (pool->workers[i] != NULL) {
                ioworker_destroy(pool->workers[i]);
            }
        }
        free(pool->workers);
    }
    free(pool);

    return NULL;
}

int ioworker_pool_add_client(ioworker_pool_t *pool, client_t *cli) {
    ioworker_t  *w;

    w = pool->workers[cli->fd % pool->worker_count];
    return ioworker_add_client(w, cli);
}

void ioworker_pool_destroy(ioworker_pool_t *pool) {
    for (int i = 0; i < pool->worker_count; i++) {
        ioworker_destroy(pool->workers[i]);
    }
    free(pool->workers);
    free(pool);
}

void ioworker_pool_stop(ioworker_pool_t *pool) {
    for (int i = 0; i < pool->worker_count; i++) {
        ioworker_stop(pool->workers[i]);
    }
}
