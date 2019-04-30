/**
 *
 * @file    list
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-12-20 15:52:34
 */

#ifndef OCTOPUS_LIST_H
#define OCTOPUS_LIST_H

#include "common.h"

typedef struct list_s list_t;

list_t* list_create(deallocator_t dealloc);
int list_push(list_t *list, void *val);
int list_pop(list_t *list);
void* list_head(list_t *list);
void* list_tail(list_t *list);
void list_remove(list_t *list, void* val);
iterator_t* list_iter(list_t *list);
int list_iter_init(list_t *list, iterator_t *iter);
void list_iter_destroy(iterator_t *iter);
int list_size(list_t *list);
void list_destroy(list_t *list);

#endif /* ifndef OCTOPUS_LIST_H */
