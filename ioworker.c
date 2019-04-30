/**
 *
 * @file    ioworker
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2019-04-30 10:43:43
 */

#include <stdlib.h>
#include <pthread.h>
#include <sys/errno.h>

#include "ioworker.h"
#include "logging.h"
#include "common.h"
#include "networking.h"
#include "client.h"

#include "libae/ae.h"

#define CLIENT_MAX_COUNT 100000

struct ioworker_s {
    aeEventLoop     *event_loop;

    pthread_t       thread;
};

void* ioworker_run(void *arg) {
    ioworker_t  *w;

    w = (ioworker_t *)arg;
    aeMain(w->event_loop);

    return NULL;
}

ioworker_t* ioworker_create() {
    ioworker_t  *w;
    int         err;

    if ((w = calloc(1, sizeof(ioworker_t))) == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for ioworker");
        return NULL;
    }

    if ((w->event_loop = aeCreateEventLoop(CLIENT_MAX_COUNT)) == NULL) {
        OCTOPUS_ERROR_LOG("failed to create event loop for ioworker");
        goto failed;
    }

    if ((err = pthread_create(&w->thread, NULL, ioworker_run, w)) != 0) {
        errno = err;
        OCTOPUS_ERROR_LOG_BY_ERRNO("failed to create pthread");
        goto failed;
    }

    return w;

failed:
    if (w->event_loop != NULL) {
        aeDeleteEventLoop(w->event_loop);
    }
    free(w);

    return NULL;
}

int ioworker_add_client(ioworker_t *w, client_t *cli) {
    ONE_PTR_NULL_CHECK(w);

    if (aeCreateFileEvent(w->event_loop, cli->fd, AE_READABLE, process_input_bytestream, cli)
            == AE_ERR) {
        OCTOPUS_ERROR_LOG("failed to add new client");
        return OCTOPUS_ERR;
    }

    return OCTOPUS_OK;
}

void ioworker_stop(ioworker_t *w) {
    aeStop(w->event_loop);
}

void ioworker_destroy(ioworker_t *w) {
    if (w == NULL) return;

    aeDeleteEventLoop(w->event_loop);
    free(w);
}
