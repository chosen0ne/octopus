/**
 *
 * @file    ioworker_pool
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2019-04-30 11:10:29
 */

#ifndef OCTOPUS_IOWORKER_POOLH
#define OCTOPUS_IOWORKER_POOLH

#include "client.h"

typedef struct ioworker_pool_s ioworker_pool_t;

ioworker_pool_t* ioworker_pool_create(int size);
int ioworker_pool_add_client(ioworker_pool_t *pool, client_t *cli);
void ioworker_pool_destroy(ioworker_pool_t *pool);
void ioworker_pool_stop(ioworker_pool_t *pool);

#endif /* ifndef OCTOPUS_IOWORKER_POOLH */
