/**
 *
 * @file    processor
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2019-01-08 16:45:25
 */

#ifndef OCTOPUS_PROCESSOR_H
#define OCTOPUS_PROCESSOR_H

#include "common.h"

#define processor_t_implement     \
    process_t   process;    \
    processor_destroy_t     destroy

typedef struct processor_s processor_t;

typedef processor_t* (*processor_factory_t)();

/**
 * A function used to process input commands.
 * @param [IN]processor, the processor instance.
 * @param [IN]cmd_obj, object holder of input command need to process.
 * @return object_t*, object holder of output command.
 */
typedef object_t* (*process_t)(processor_t *processor, object_t *cmd);

typedef void (*processor_destroy_t)(processor_t *processor);

struct processor_s {
    process_t   process;

    processor_destroy_t     destroy;
};

#endif /* ifndef OCTOPUS_PROCESSOR_H */
