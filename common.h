/**
 *
 * @file    common
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-05-04 17:25:38
 */

#ifndef OCTOPUS_COMMON_H
#define OCTOPUS_COMMON_H

#include <stdlib.h>

// retrun value of functions
#define OCTOPUS_OK          0
#define OCTOPUS_ERR         -1
#define OCTOPUS_EOF         -2
#define OCTOPUS_RESET       -3
#define OCTOPUS_AGAIN       -4
#define OCTOPUS_NOT_FOUND   -5

// boolean values
#define OCTOPUS_TRUE     1
#define OCTOPUS_FALSE    0

#define OCTOPUS_ADDR_BUF_SIZE  100

#define OCTOPUS_NOT_USED(p)  ((void)(p))

#define ONE_PTR_NULL_CHECK(p)   \
    if ((p) == NULL) {  \
        OCTOPUS_ERROR_LOG("Invalid param, " #p "is NULL."); \
        return OCTOPUS_ERR; \
    }

#define TWO_PTRS_NULL_CHECK(p1, p2)     \
    ONE_PTR_NULL_CHECK(p1); \
    ONE_PTR_NULL_CHECK(p2);

#define THREE_PTRS_NULL_CHECK(p1, p2, p3)   \
    ONE_PTR_NULL_CHECK(p1); \
    ONE_PTR_NULL_CHECK(p2); \
    ONE_PTR_NULL_CHECK(p3);

#define failed_destroy(r, t)  if ((r) != NULL) { t##_destroy((r)); }

#define iterator_t_implement    \
    void* (*next)(void *iter);  \
    int (*has_next)(void *iter);    \
    int (*remove)(void *iter);  \
    int (*index)(void *iter)

typedef struct octopus_s octopus_t;

typedef struct {
    void    *first;
    void    *second;
} pair_t;

typedef struct {
    void    *first;
    void    *second;
    void    *third;
} triple_t;

typedef struct {
    void* (*next)(void *iter);
    int (*has_next)(void *iter);
    int (*remove)(void *iter);
    int (*index)(void *iter);
} iterator_t;

typedef struct object_s object_t;

/**
 * A function pointer used to deallocate resource.
 */
typedef void (* deallocator_t)(void *p);

/**
 * A deallocator just free the memory which allocated by malloc.
 */
void free_deallocator(void *p);

int iter_remove_not_support(void *iter);

#endif /* ifndef OCTOPUS_COMMON_H */
