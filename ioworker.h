/**
 *
 * @file    ioworker
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2019-04-30 10:42:19
 */

#ifndef OCTOPUS_IOWORKER_H
#define OCTOPUS_IOWORKER_H

#include "client.h"

typedef struct ioworker_s ioworker_t;

ioworker_t* ioworker_create();
int ioworker_add_client(ioworker_t *w, client_t *cli);
void ioworker_stop(ioworker_t *w);
void ioworker_destroy(ioworker_t *w);

#endif /* ifndef OCTOPUS_IOWORKER_H */
