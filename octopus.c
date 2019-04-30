/**
 *
 * @file    octopus
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-12-20 15:40:41
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "libae/ae.h"

#include "octopus.h"
#include "array.h"
#include "list.h"
#include "hash.h"
#include "client.h"
#include "logging.h"
#include "networking.h"
#include "common.h"
#include "ioworker_pool.h"

struct octopus_s {
    ioworker_pool_t *ioworker_pool;

    array_t         *listening_sockets;
    list_t          *clients;

    // hash: listening socket =>
    //      triple{protocol_decoder_factory_t, protocol_encoder_factory_t, octopus_t}
    // used to destory protocol context, to avoid memory leak.
    hash_t          *srv_contexts;

    aeEventLoop     *event_loop;

    // hash: protocol name => protocol_factory_t
    hash_t          *protocol_factories;

    // hash: protocol name => processor_factory_t
    hash_t          *processor_factories;
};

void cli_dealloc(void *cli) {
    client_t    *client;

    if (cli == NULL) return;

    client = (client_t *)cli;
    client_destroy(client);
    free(cli);
}

octopus_t* octopus_create() {
    octopus_t   *oct;
    int         max_clients;

    oct = (octopus_t *)calloc(1, sizeof(octopus_t));
    if (oct == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for octopus");
        return NULL;
    }

    oct->listening_sockets = array_create(10, sizeof(int), NULL);
    if (oct->listening_sockets == NULL) {
        OCTOPUS_ERROR_LOG("failed to create array for listening sockets");
        goto failed;
    }

    oct->clients = list_create(cli_dealloc);
    if (oct->clients == NULL) {
        OCTOPUS_ERROR_LOG("failed to create list for clients");
        goto failed;
    }

    oct->protocol_factories = hash_str_key_create(NULL, NULL);
    if (oct->protocol_factories == NULL) {
        OCTOPUS_ERROR_LOG("failed to create protocol factory hash");
        goto failed;
    }

    oct->srv_contexts = hash_int_key_create(free_deallocator);
    if (oct->srv_contexts == NULL) {
        OCTOPUS_ERROR_LOG("failed to create protocol context hash");
        goto failed;
    }

    oct->processor_factories = hash_str_key_create(NULL, NULL);
    if (oct->processor_factories == NULL) {
        OCTOPUS_ERROR_LOG("failed to create command processor factory hash");
        goto failed;
    }

    max_clients = 10000;
    oct->event_loop = aeCreateEventLoop(max_clients);
    if (oct->event_loop == NULL) {
        OCTOPUS_ERROR_LOG("failed to create event loop");
        goto failed;
    }

    return oct;

failed:

    octopus_destroy(oct);

    return NULL;
}

void octopus_set_ioworker_count(octopus_t *oct, int worker_count) {
    oct->ioworker_pool = ioworker_pool_create(worker_count);
}

ioworker_pool_t* octopus_ioworker_pool(octopus_t *oct) {
    return oct->ioworker_pool;
}

int octopus_register_protocol_factory(
        octopus_t *oct,
        const char *protocol_name,
        protocol_factory_t protocol_factory) {

    THREE_PTRS_NULL_CHECK(oct, protocol_name, protocol_factory);

    if (hash_exists(oct->protocol_factories, protocol_name)) {
        OCTOPUS_ERROR_LOG("protocol factory named '%s' already exist", protocol_name);
        return OCTOPUS_ERR;
    }

    if (hash_put(oct->protocol_factories, protocol_name, protocol_factory) == OCTOPUS_ERR) {
        OCTOPUS_ERROR_LOG("failed to register protocol factory, protocol name: %s", protocol_name);
        return OCTOPUS_ERR;
    }

    return OCTOPUS_OK;
}

int octopus_register_processor_factory(
        octopus_t *oct,
        const char *protocol_name,
        processor_factory_t processor_factory) {

    THREE_PTRS_NULL_CHECK(oct, protocol_name, processor_factory);

    if (hash_exists(oct->processor_factories, protocol_name)) {
        OCTOPUS_ERROR_LOG("command processor factory named '%s' already exist", protocol_name);
        return OCTOPUS_ERR;
    }

    if (hash_put(oct->processor_factories, protocol_name, processor_factory) == OCTOPUS_ERR) {
        OCTOPUS_ERROR_LOG("failed to register command processor factory, protocol name: %s",
                protocol_name);
        return OCTOPUS_ERR;
    }

    return OCTOPUS_OK;
}

void octopus_add_listening_socket(
        octopus_t *oct,
        const char *host,
        const char *port,
        const char *protocol_name) {

    struct addrinfo     hint, *res, *res0;
    int                 errno, s, added, ret;
    void                *protocol_factory, *processor_factory;
    triple_t            *srv_ctx;

    if ((protocol_factory = hash_get(oct->protocol_factories, protocol_name)) == NULL) {
        OCTOPUS_ERROR_LOG("no protocol factory for protocol named '%s'", protocol_name);
        return;
    }
    if ((processor_factory = hash_get(oct->processor_factories, protocol_name)) == NULL) {
        OCTOPUS_ERROR_LOG("no command processor factory for protocol named '%s'", protocol_name);
        return;
    }

    hint.ai_socktype = SOCK_STREAM;
    hint.ai_family = PF_INET;
    hint.ai_flags = AI_PASSIVE;

    if ((errno = getaddrinfo(host, port, &hint, &res0)) != 0) {
        OCTOPUS_ERROR_LOG("failed to get address information, err: %s", gai_strerror(errno));
        return;
    }

    added = 0;
    for (res = res0; res != NULL; res = res0->ai_next) {
        s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (s == -1) {
            OCTOPUS_ERROR_LOG("failed to create socket");
            continue;
        }

        if (tcp_nonblk_srv(s, res, 1024) == OCTOPUS_ERR) {
            OCTOPUS_ERROR_LOG("failed to set tcp non-block server");
            continue;
        }

        if ((srv_ctx = (triple_t *)malloc(sizeof(triple_t))) == NULL) {
            OCTOPUS_ERROR_LOG("failed to alloc mem for protocol context");
            return;
        }
        srv_ctx->first = protocol_factory;
        srv_ctx->second = processor_factory;
        srv_ctx->third = oct;

        ret = OCTOPUS_OK;
        do {
            // TODO: avoid leak of srv_ctx
            if (aeCreateFileEvent(oct->event_loop, s, AE_READABLE, client_connected, srv_ctx)
                    == AE_ERR) {

                OCTOPUS_ERROR_LOG("failed to add file event when creating listening socket");
                ret = OCTOPUS_ERR;
                break;
            }

            if (array_add(oct->listening_sockets, (void *)s, sizeof(s)) == OCTOPUS_ERR) {
                OCTOPUS_ERROR_LOG("failed to add socket to listening_sockets array, host:%s, "
                        "port:%s", host, port);
                ret = OCTOPUS_ERR;
                break;
            }

            if (hash_put(oct->srv_contexts, (void *)s, srv_ctx) == OCTOPUS_ERR) {
                OCTOPUS_ERROR_LOG("failed to add protocol context, host:%s, port:%s", host, port);
                ret = OCTOPUS_ERR;
            }
        } while (0);

        if (ret == OCTOPUS_ERR) {
            free(srv_ctx);
            continue;
        }

        added++;
    }

    OCTOPUS_INFO_LOG("%d sockets added for %s:%s", added, host, port);
}

int octopus_srv_start(octopus_t *oct) {
    if (hash_empty(oct->processor_factories)) {
        OCTOPUS_ERROR_LOG("protocol decoder and encoder must be both set");
        return OCTOPUS_ERR;
    }

    // If there are no listening sockets, the server will just stop.
    if (array_empty(oct->listening_sockets)) {
        OCTOPUS_ERROR_LOG("no listening socket added, server will stop...");
        return OCTOPUS_ERR;
    }

    OCTOPUS_INFO_LOG("octopus starts to run...");
    aeMain(oct->event_loop);

    return OCTOPUS_OK;
}

void octopus_srv_stop(octopus_t *oct) {
    aeStop(oct->event_loop);
    if (oct->ioworker_pool != NULL) {
        ioworker_pool_stop(oct->ioworker_pool);
    }
}

void octopus_destroy(octopus_t *oct) {
    if (oct == NULL) {
        return;
    }

    failed_destroy(oct->listening_sockets, array);
    failed_destroy(oct->clients, list);
    failed_destroy(oct->srv_contexts, hash);
    failed_destroy(oct->processor_factories, hash);
    failed_destroy(oct->protocol_factories, hash);
    failed_destroy(oct->ioworker_pool, ioworker_pool);

    if (oct->event_loop != NULL) {
        aeDeleteEventLoop(oct->event_loop);
    }

    free(oct);
}
