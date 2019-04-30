/**
 *
 * @file    list
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-12-20 16:53:58
 */

#include <stdlib.h>
#include <strings.h>

#include "list.h"
#include "logging.h"

#define head(list)  (list->sentinel.next)
#define tail(list)  (list->sentinel.prev)

typedef struct list_node_s {
    void    *val;
    struct list_node_s  *prev, *next;
} list_node_t;

struct list_s {
    int     size;
    list_node_t     sentinel;
    deallocator_t   dealloc;
};

typedef struct {
    iterator_t_implement;

    list_t          *list;
    list_node_t     *cur_node;
    int             idx;
} list_iterator_t;

static inline void node_destroy(list_t *list, list_node_t *n) {
    if (list->dealloc != NULL) {
        list->dealloc(n->val);
    }

    free(n);
}

static inline int node_remove(list_t *list, list_node_t *node) {
    TWO_PTRS_NULL_CHECK(list, node);

    node->prev->next = node->next;
    node->next->prev = node->prev;
    node_destroy(list, node);

    return OCTOPUS_OK;
}

list_t* list_create(deallocator_t dealloc) {
    list_t  *li;

    li = malloc(sizeof(list_t));
    if (li == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for list");
        return NULL;
    }
    bzero(li, sizeof(list_t));

    li->dealloc = dealloc;
    li->sentinel.prev = li->sentinel.next = &li->sentinel;

    return li;
}

int list_push(list_t *list, void *val) {
    list_node_t     *node;

    TWO_PTRS_NULL_CHECK(list, val);

    node = malloc(sizeof(list_node_t));
    if (node == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc list node");
        return OCTOPUS_ERR;
    }

    node->next = tail(list)->next;
    node->prev = tail(list);
    tail(list)->next = node;
    tail(list) = node;

    node->val = val;

    list->size++;

    return OCTOPUS_OK;
}

int list_pop(list_t *list) {
    list_node_t     *node;

    ONE_PTR_NULL_CHECK(list);

    if (list->size == 0) {
        return OCTOPUS_OK;
    }

    node = head(list);
    if (list->dealloc != NULL) {
        list->dealloc(node->val);
    }

    head(list)->next->prev = &list->sentinel;
    head(list) = head(list)->next;
    list->size--;

    free(node);

    return OCTOPUS_OK;
}

void* list_head(list_t *list) {
    return head(list) == NULL ? NULL : head(list)->val;
}

void* list_tail(list_t *list) {
    return tail(list) == NULL ? NULL : tail(list)->val;
}

void list_remove(list_t *list, void *val) {
    list_node_t     *n;

    for (n = head(list); n != &list->sentinel; n = n->next) {
        if (n->val == val) {
            n->prev->next = n->next;
            n->next->prev = n->prev;

            node_destroy(list, n);
            list->size--;
        }
    }
}

void* list_iter_next(void *iter) {
    list_iterator_t     *list_iter;

    list_iter = (list_iterator_t *)iter;
    list_iter->cur_node = list_iter->cur_node->next;
    list_iter->idx++;

    return list_iter->cur_node->val;
}

int list_iter_has_next(void *iter) {
    list_iterator_t     *list_iter;

    list_iter = (list_iterator_t *)iter;

    return list_iter->cur_node->next != &list_iter->list->sentinel;
}

int list_iter_remove(void *iter) {
    list_iterator_t     *list_iter;
    list_node_t         *to_rm;

    ONE_PTR_NULL_CHECK(iter);

    list_iter = (list_iterator_t *)iter;
    to_rm = list_iter->cur_node;
    list_iter->cur_node = to_rm->prev;
    if (node_remove(list_iter->list, to_rm) == OCTOPUS_ERR) {
        return OCTOPUS_ERR;
    }
    list_iter->list->size--;

    return OCTOPUS_OK;
}

int list_iter_index(void *iter) {
    return ((list_iterator_t *)iter)->idx;
}

iterator_t* list_iter(list_t *list) {
    list_iterator_t     *list_iter;

    list_iter = malloc(sizeof(list_iterator_t));
    if (list_iter == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for list iter");
        return NULL;
    }

    list_iter_init(list, (iterator_t *)list_iter);

    return (iterator_t *)list_iter;
}

int list_iter_init(list_t *list, iterator_t *iter) {
    list_iterator_t     *list_iter;

    ONE_PTR_NULL_CHECK(iter);

    list_iter = (list_iterator_t *)iter;
    list_iter->next = list_iter_next;
    list_iter->has_next = list_iter_has_next;
    list_iter->remove = list_iter_remove;
    list_iter->index = list_iter_index;
    list_iter->list = list;
    list_iter->idx = 0;
    list_iter->cur_node = &list->sentinel;

    return OCTOPUS_OK;
}

int list_size(list_t *list) {
    return list->size;
}

void list_iter_destroy(iterator_t *iter) {
    free(iter);
}

void list_destroy(list_t *list) {
    list_node_t     *n, *next;

    for (n = head(list); n != &list->sentinel; n = next) {
        next = n->next;
        if (list->dealloc) {
            list->dealloc(n->val);
        }
        free(n);
    }

    free(list);
}

#ifdef OCTOPUS_TEST_LIST

#include <stdio.h>

struct tmp {
    int a, b, c;
};

void tmp_free(void *p) {
    OCTOPUS_ERROR_LOG("tmp free, addr: %p", p);
    free(p);
}

int main(int argc, char *argv[]) {
    list_t      *li, *list_b;
    iterator_t  *iter;
    list_node_t *n;
    int         v;

    li = list_create(NULL);

    for (int i = 0; i < 10; i++) {
        list_push(li, (void *)i);
    }

    for (n = head(li); n != &li->sentinel; n = n->next) {
        printf("%d\n", (int)(n->val));
    }

    printf("size: %d\n", list_size(li));

    for (iter = list_iter(li); iter->has_next(iter);) {
        v = (int)iter->next(iter);
        printf("%d: %d\n", iter->index(iter), v);
        if (v == 1 || v == 9) {
            iter->remove(iter);
        }
    }

    for (list_iter_init(li, iter); iter->has_next(iter);) {
        v = (int)iter->next(iter);
        printf("%d: %d\n", iter->index(iter), v);
    }

    list_iter_destroy(iter);
    list_destroy(li);

    struct tmp  *a, *b, *c;
    a = malloc(sizeof(*a));
    b = malloc(sizeof(*b));
    a->a = 12, a->b = 123, a->c = 144;
    b->a = 34, b->b = 33, b->c = 54;

    list_b = list_create(tmp_free);
    list_push(list_b, a);
    list_push(list_b, b);

    for (iter = list_iter(list_b); iter->has_next(iter);) {
        c = (struct tmp *)iter->next(iter);
        printf("%d: a=%d, b=%d, c=%d\n", iter->index(iter), c->a, c->b, c->c);
    }

    list_iter_destroy(iter);
    list_destroy(list_b);

    return 0;
}

#endif
