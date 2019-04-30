/**
 *
 * @file    array
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-12-20 15:14:50
 */

#ifndef OCTOPUS_ARRAY_H
#define OCTOPUS_ARRAY_H

#include "common.h"

#define array_empty(a)  array_size(a) == 0

typedef struct array_s array_t;

array_t* array_create(int capacity, int element_size, deallocator_t dealloc);
int array_set(array_t *a, int idx, void *val, int valsize);
int array_add(array_t *a, void *val, int valsize);
int array_remove(array_t *a, int idx);
int array_clear(array_t *a);
void* array_get(array_t *a, int idx);
int array_size(array_t *a);
iterator_t* array_iter(array_t *a);
void array_iter_destroy(iterator_t *);
void array_destroy(array_t *a);

#endif /* ifndef OCTOPUS_ARRAY_H */
