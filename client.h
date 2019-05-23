/**
 *
 * @file    client
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-12-20 15:44:18
 */

#ifndef OCTOPUS_CLIENT_H
#define OCTOPUS_CLIENT_H

#include <stdint.h>

#include "buffer.h"
#include "common.h"
#include "object.h"
#include "list.h"

typedef struct {
    int     fd;

    octopus_t   *oct;

    // object holder of protocol
    object_t    *protocol_obj;
    // object holder of processor
    object_t    *processor_obj;

    char        host[OCTOPUS_ADDR_BUF_SIZE];
    uint16_t    port;

    buffer_t    *inbuf;
    list_t      *inbuf_list;

    list_t      *input_cmd_objs;
    iterator_t  *input_cmd_objs_iter;

    buffer_t    *outbuf;
} client_t;

client_t* client_create();
void client_destroy(client_t *cli);

#endif /* ifndef OCTOPUS_CLIENT_H */
