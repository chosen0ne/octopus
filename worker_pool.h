/**
 *
 * @file    worker_pool
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-05-18 17:42:11
 */

#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include "worker.h"

typedef struct {
    int         count;
    worker_t    **workers;
} worker_pool_t;

worker_pool_t* worker_pool_create(int worker_count);
void worker_pool_do(worker_pool_t *pool, job_t *job, int hash_id);
void worker_pool_destroy(worker_pool_t *pool);

#endif /* ifndef WORKER_POOL_H */
