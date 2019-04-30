/**
 *
 * @file    hash
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-12-27 12:04:06
 */

#ifndef OCTOPUS_HASH_T
#define OCTOPUS_HASH_T

#include "common.h"

#define hash_exists(h, k)    (hash_get((h), (k)) != NULL)
#define hash_empty(h)   hash_size(h) == 0

typedef struct hash_s hash_t;

typedef int (*hash_func_t)(const void *k);
typedef int (*equal_func_t)(const void *a, const void *b);

hash_t* hash_create(hash_func_t h, equal_func_t e, deallocator_t kdealloc, deallocator_t vdealloc);
hash_t* hash_str_key_create(deallocator_t kdealloc, deallocator_t vdealloc);
hash_t* hash_int_key_create(deallocator_t vdealloc);
int hash_put(hash_t *h, const void *k, const void *v);
void* hash_get(const hash_t *h, const void *k);
void hash_remove(hash_t *h, const void *k);
iterator_t* hash_iter(hash_t *h);
void hash_iter_destroy(iterator_t *iter);
int hash_size(const hash_t *h);
void hash_destroy(hash_t *h);

#endif /* ifndef OCTOPUS_HASH_T */
