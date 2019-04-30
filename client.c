/**
 *
 * @file    client
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2019-01-02 19:06:59
 */
#include <stdlib.h>
#include <unistd.h>

#include "client.h"
#include "buffer.h"
#include "logging.h"

#define DEFAULT_INPUT_BUF_LEN   1024 * 1024
#define DEFAULT_OUTPUT_BUF_LEN  1024 * 1024

static void inbuf_list_deallocator(void *p) {
    buffer_destroy((buffer_t *)p);
}

static void cmd_obj_deallocator(void *p) {
    object_t    *o;

    if (p == NULL) {
        return;
    }

    o = (object_t *)p;
    o->decr(o);
}

client_t* client_create() {
    client_t    *cli;

    cli = calloc(1, sizeof(client_t));
    if (cli == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for client");
        return NULL;
    }

    cli->inbuf = buffer_create(DEFAULT_INPUT_BUF_LEN);
    if (cli->inbuf == NULL) {
        OCTOPUS_ERROR_LOG("failed to create input buf");
        goto failed;
    }

    cli->inbuf_list = list_create(inbuf_list_deallocator);
    if (cli->inbuf_list == NULL) {
        OCTOPUS_ERROR_LOG("failed to create input buf list");
        goto failed;
    }

    cli->outbuf = buffer_create(DEFAULT_OUTPUT_BUF_LEN);
    if (cli->outbuf == NULL) {
        OCTOPUS_ERROR_LOG("failed to create output buf");
        goto failed;
    }

    // TODO: need to deallocate the output commands
    cli->input_cmd_objs = list_create(cmd_obj_deallocator);
    if (cli->input_cmd_objs == NULL) {
        OCTOPUS_ERROR_LOG("failed to create output commands list");
        goto failed;
    }

    cli->input_cmd_objs_iter = list_iter(cli->input_cmd_objs);
    if (cli->input_cmd_objs_iter == NULL) {
        OCTOPUS_ERROR_LOG("failed to create output commands iterator");
        goto failed;
    }

    cli->fd = -1;

    return cli;

failed:

    failed_destroy(cli->inbuf, buffer);
    failed_destroy(cli->inbuf_list, list);
    failed_destroy(cli->outbuf, buffer);
    failed_destroy(cli->input_cmd_objs, list);
    failed_destroy(cli->input_cmd_objs_iter, list_iter);
    free(cli);

    return NULL;
}

void client_destroy(client_t *cli) {
    buffer_destroy(cli->inbuf);
    list_destroy(cli->inbuf_list);
    buffer_destroy(cli->outbuf);
    list_destroy(cli->input_cmd_objs);
    list_iter_destroy(cli->input_cmd_objs_iter);

    if (cli->protocol_obj != NULL) {
        cli->protocol_obj->decr(cli->protocol_obj);
    }
    if (cli->processor_obj != NULL) {
        cli->processor_obj->decr(cli->processor_obj);
    }

    if (cli->fd != -1) {
        close(cli->fd);
    }

    free(cli);
}
