/**
 *
 * @file    common
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-12-19 11:18:01
 */

#include <stdlib.h>

#include "common.h"
#include "logging.h"

/**
 * A deallocator just free the memory which allocated by malloc.
 */
void free_deallocator(void *p) {
    free(p);
}

int iter_remove_not_support(void *iter) {
    OCTOPUS_NOT_USED(iter);
    OCTOPUS_ERROR_LOG("remove of iterator not supported");

    return OCTOPUS_ERR;
}
