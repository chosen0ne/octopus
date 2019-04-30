/**
 *
 * @file    object
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2019-01-08 17:17:46
 */

#ifndef OCTOPUS_OBJECT_H
#define OCTOPUS_OBJECT_H

#include "common.h"
#include "protocol.h"
#include "processor.h"
#include "command.h"

#define OBJECT_TYPE_COMMAND     1
#define OBJECT_TYPE_PROCESSOR   2
#define OBJECT_TYPE_PROTOCOL    3

#define object_inspect(obj, tag)    \
    OCTOPUS_INFO_LOG("refcnt: %d@%s", (obj)->refcnt, tag)

#define object_create_cmd(c)        object_create(OBJECT_TYPE_COMMAND, c)
#define object_create_protocol(p)   object_create(OBJECT_TYPE_PROTOCOL, p)
#define object_create_processor(p)  object_create(OBJECT_TYPE_PROCESSOR, p)

/**
 * A object container used to hold object, which will automatically manage memory based on
 * reference count.
 */
struct object_s {
    void (*incr)(object_t *obj);
    void (*decr)(object_t *obj);

    int     refcnt;
    int     type;
    union {
        command_t   *cmd;
        protocol_t  *protocol;
        processor_t *processor;
    } obj;
};

object_t* object_create(int type, void *obj);

#endif /* ifndef OCTOPUS_OBJECT_H */
