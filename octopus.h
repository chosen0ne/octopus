/**
 *
 * @file    octopus
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-12-20 15:29:31
 */

#ifndef OCTOPUS_H
#define OCTOPUS_H

#include "protocol.h"
#include "processor.h"
#include "ioworker_pool.h"

octopus_t* octopus_create();

void octopus_set_ioworker_count(octopus_t *oct, int worker_count);

ioworker_pool_t* octopus_ioworker_pool(octopus_t *oct);

int octopus_register_protocol_factory(
        octopus_t *oct,
        const char *protocol_name,
        protocol_factory_t protocol_factory);

int octopus_register_processor_factory(
        octopus_t *oct,
        const char *protocol_name,
        processor_factory_t processor_factory);

void octopus_add_listening_socket(
        octopus_t *oct,
        const char *host,
        const char *port,
        const char *protocol_name);

int octopus_srv_start(octopus_t *oct);

void octopus_srv_stop(octopus_t *oct);

void octopus_destroy(octopus_t *oct);

#endif /* ifndef OCTOPUS_H */
