/**
 *
 * @file    object
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2019-01-08 17:38:08
 */

#include <stdlib.h>
#include "object.h"
#include "logging.h"
#include "command.h"
#include "protocol.h"
#include "processor.h"

static inline void object_incr(object_t *obj);
static inline void object_decr(object_t *obj);

object_t* object_create(int type, void *obj) {
    object_t    *o;

    if (type != OBJECT_TYPE_COMMAND && type != OBJECT_TYPE_PROCESSOR &&
            type != OBJECT_TYPE_PROTOCOL) {
        OCTOPUS_ERROR_LOG("not support object type, type: %d", type);
        return NULL;
    }

    o = calloc(1, sizeof(object_t));
    if (o == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for object");
        return NULL;
    }

    o->type = type;
    switch (type) {
        case OBJECT_TYPE_COMMAND:
            o->obj.cmd = (command_t *)obj;
            break;
        case OBJECT_TYPE_PROTOCOL:
            o->obj.protocol = (protocol_t *)obj;
            break;
        case OBJECT_TYPE_PROCESSOR:
            o->obj.processor = (processor_t *)obj;
            break;
    }
    o->incr = object_incr;
    o->decr = object_decr;
    o->refcnt = 1;

    return o;
}

void object_incr(object_t *obj) {
    obj->refcnt++;
}

void object_decr(object_t *obj) {
    if (obj->refcnt <= 0) {
        OCTOPUS_ERROR_LOG("fatal error: obj->refcnt <= 0");
        return;
    }

    obj->refcnt--;
    if (obj->refcnt == 0) {
        switch (obj->type) {
        case OBJECT_TYPE_COMMAND:
            if (obj->obj.cmd != NULL) {
                obj->obj.cmd->destroy(obj->obj.cmd);
            }
            break;
        case OBJECT_TYPE_PROTOCOL:
            if (obj->obj.protocol != NULL) {
                obj->obj.protocol->destroy(obj->obj.protocol);
            }
            break;
        case OBJECT_TYPE_PROCESSOR:
            if (obj->obj.processor != NULL) {
                obj->obj.processor->destroy(obj->obj.processor);
            }
            break;
        }

        free(obj);
    }
}
