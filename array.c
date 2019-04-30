/**
 *
 * @file    array
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-12-17 18:49:34
 */

#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include "array.h"
#include "logging.h"

struct array_s {
    char    *store;     /* Binary store used to store elements */
    int     capacity;   /* Element count */
    int     size;
    int     element_size;   /* In bytes */

    // Used to release the resources belonged to the item,
    // when the item has removed.
    deallocator_t   dealloc;
};

typedef struct {
    iterator_t_implement;

    array_t     *array;
    int         idx;
} array_iterator_t;

array_t* array_create(int capacity, int element_size, deallocator_t dealloc) {
    array_t     *a;

    a = (array_t *)malloc(sizeof(array_t));
    if (a == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for array");
        return NULL;
    }

    bzero(a, sizeof(array_t));
    a->capacity = capacity;
    a->element_size = element_size;
    a->dealloc = dealloc;

    a->store = (char *)malloc(capacity * element_size);
    if (a->store == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for array.store");
        free(a);
        return NULL;
    }

    return a;
}

void void_pointer_raw_copy(char *dist, void *src, int size) {
    short   *s;
    int     *i;
    long long   *l;

    switch (size) {
    case sizeof(char):
        *dist = (char)src;
        break;
    case sizeof(short):
        s = (short *)dist;
        *s = (short)src;
        break;
    case sizeof(int):
        i = (int *)dist;
        *i = (int)src;
        break;
    case sizeof(void *):
        l = (long long *)dist;
        *l = (long)src;
        break;
    default:
        OCTOPUS_ERROR_LOG("Not support size to copy, size: %d", size);
    }
}

int array_set(array_t *a, int idx, void *val, int valsize) {
    if (idx < 0 || idx >= a->capacity) {
        OCTOPUS_ERROR_LOG("index beyond boundary, array[0:%d), idx: %d", a->capacity, idx);
        return OCTOPUS_ERR;
    }

    if (valsize != a->element_size) {
        OCTOPUS_ERROR_LOG("value size isn't expected, val size: %d, element size: %d",
                valsize, a->element_size);
        return OCTOPUS_ERR;
    }

    memmove(a->store + a->element_size * idx, val, a->element_size);

    return OCTOPUS_OK;
}

int array_remove(array_t *a, int idx) {
    if (a == NULL) {
        OCTOPUS_ERROR_LOG("invalid param");
        return OCTOPUS_ERR;
    }

    if (idx < 0 || idx >= a->capacity) {
        OCTOPUS_ERROR_LOG("index beyond boundary, array[0:%d], idx: %d", a->capacity, idx);
        return OCTOPUS_ERR;
    }

    if (a->dealloc != NULL) {
        a->dealloc(a->store + a->element_size * idx);
    }

    memmove(a->store + a->element_size * idx,
            a->store + a->element_size * (idx + 1),
            (a->size - idx - 1) * a->element_size);
    a->size--;

    return OCTOPUS_OK;
}

int array_clear(array_t *a) {
    ONE_PTR_NULL_CHECK(a);

    for (int i = a->size - 1; i >= 0; i--) {
        if (a->dealloc != NULL) {
            a->dealloc(a->store + a->element_size * i);
        }
    }

    a->size = 0;

    return OCTOPUS_OK;
}

int expand(array_t *a) {
    void    *p;

    p = realloc(a->store, (a->capacity * a->element_size) * 2);
    if (p == NULL) {
        OCTOPUS_ERROR_LOG("failed to expand");
        return OCTOPUS_ERR;
    }

    a->store = p;
    a->capacity *= 2;


    return OCTOPUS_OK;
}

int array_add(array_t *a, void *val, int valsize) {
    if (a == NULL) {
        OCTOPUS_ERROR_LOG("invalid param");
        return OCTOPUS_ERR;
    }

    if (valsize != a->element_size) {
        OCTOPUS_ERROR_LOG("value size isn't expected, val size: %d, element size: %d",
                valsize, a->element_size);
        return OCTOPUS_ERR;
    }

    if (a->size == a->capacity && expand(a) == OCTOPUS_ERR) {
        OCTOPUS_ERROR_LOG("failed to expand array, current capacity: %d", a->capacity);
        return OCTOPUS_ERR;
    }

    /*memmove(a->store + a->element_size * a->size, *val, a->element_size);*/
    void_pointer_raw_copy(a->store + a->element_size * a->size, val, a->element_size);

    a->size++;

    return OCTOPUS_OK;
}

void* array_get(array_t *a, int idx) {
    if (idx < 0 || idx >= a->capacity) {
        OCTOPUS_ERROR_LOG("index beyond boundary, array[0:%d), idx: %d", a->capacity, idx);
        return NULL;
    }

    return a->store + a->element_size * idx;
}

int array_size(array_t *a) {
    return a->size;
}

void* iter_next(void *iter) {
    array_iterator_t    *array_iter;
    array_t             *a;

    array_iter = (array_iterator_t *)iter;
    a = array_iter->array;

    if (array_iter->idx < 0 || array_iter->idx > a->size) {
        return NULL;
    }

    array_iter->idx++;

    return a->store + a->element_size * (array_iter->idx - 1);
}

int iter_has_next(void *iter) {
    array_iterator_t    *array_iter;

    array_iter = (array_iterator_t *)iter;

    return array_iter->idx < array_iter->array->size;
}

int iter_index(void *iter) {
    return ((array_iterator_t *)iter)->idx;
}

iterator_t* array_iter(array_t* a) {
    array_iterator_t    *iter;

    iter = malloc(sizeof(array_iterator_t));

    iter->next = iter_next;
    iter->has_next = iter_has_next;
    iter->index = iter_index;
    iter->array = a;
    iter->idx = 0;

    return (iterator_t *)iter;
}

void array_iter_destroy(iterator_t *i) {
    free(i);
}

void array_destroy(array_t *a) {
    void    *e;

    for (int i = 0; i < a->size; i++) {
        e = array_get(a, i);

        if (a->dealloc != NULL) {
            a->dealloc(e);
        }
    }

    free(a->store);
    free(a);
}

#ifdef OCTOPUS_TEST_ARRAY

#include <stdio.h>
#include <time.h>

struct tmp {
    int a, b, c;
};

void tmp_free(void *p) {
    struct tmp  **a;

    a = (struct tmp **)p;
    OCTOPUS_ERROR_LOG("tmp deallocator, free: %p", *a);
    free(*a);
}

int main(int argc, char *argv[])
{
    array_t     *array, *b;
    struct tmp  *c, *d;
    int         randval;
    iterator_t  *iter;

    array = array_create(10, sizeof(int), NULL);

    srand(time(NULL));

    for (int i = 0; i < 20; i++) {
        randval = rand();
        array_add(array, (void *)i, sizeof(int));
    }

    printf("%d: %d\n", 2, *(int *)array_get(array, 2));
    printf("%d: %d\n", 3, *(int *)array_get(array, 3));
    printf("%d: %d\n", 4, *(int *)array_get(array, 4));

    printf("%p\n", array_get(array, 2));

    iter = array_iter(array);
    while (iter->has_next(iter)) {
        printf("%d: %d\n", iter->index(iter), *(int *)iter->next(iter));
    }

    array_iter_destroy(iter);
    array_destroy(array);

    c = malloc(sizeof(struct tmp));
    d = malloc(sizeof(struct tmp));
    c->a = 10, c->b = 12, c->c = 110;
    d->a = 13, d->b = 14, d->c = 112020;

    b = array_create(10, sizeof(struct tmp *), tmp_free);
    array_add(b, c, sizeof(c));
    array_add(b, d, sizeof(d));

    for (int i = 0; i < array_size(b); i++) {
        struct tmp *e = *(struct tmp**)array_get(b, i);
        printf("struct tmp: a=%d, b=%d, c=%d, addr: %p\n", e->a, e->b, e->c, e);
    }

    array_destroy(b);

    return 0;
}

#endif
