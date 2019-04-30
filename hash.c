/**
 *
 * @file    hash
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-12-27 15:40:16
 */

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "hash.h"
#include "logging.h"

#define HASH_ENTRY_INIT_SIZE    8;

typedef struct entry_s {
    int     hash_val;

    const void  *val;
    const void  *key;

    struct entry_s     *next;
} entry_t;

struct hash_s {
    int         size;
    int         entry_count;
    entry_t     **entries;

    hash_func_t     hash_func;
    equal_func_t    equal_func;
    deallocator_t   key_dealloc;
    deallocator_t   val_dealloc;
};

typedef struct {
    iterator_t_implement;

    hash_t      *hash;
    entry_t     *cur_entry;
    int         entry_index;
    int         idx;
    pair_t      pair;
    int         has_next_first;
} hash_iterator_t;

static inline int str_hash_func(const void *k) {
    int     hash_val;
    char    *s;

    hash_val = 13477;
    s = (char *)k;
    while (*s) {
        hash_val = *s + 7 * hash_val;
        s++;
    }

    return hash_val;
}

static inline int str_equal_func(const void *a, const void *b) {
    return strcmp(a, b) == 0;
}

static inline int int_hash_func(const void *k) {
    return (int)k;
}

static inline int int_equal_func(const void *a, const void *b) {
    return (int)a == (int)b;
}

hash_t* hash_create(hash_func_t h, equal_func_t e, deallocator_t kd, deallocator_t vd) {
    hash_t      *hash;

    hash = (hash_t *)malloc(sizeof(hash_t));
    if (hash == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for hash");
        return NULL;
    }
    bzero(hash, sizeof(hash_t));
    hash->entry_count = HASH_ENTRY_INIT_SIZE;

    hash->entries = (entry_t **)malloc(sizeof(entry_t *) * hash->entry_count);
    if (hash->entries == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for entries of hash");
        return NULL;
    }
    bzero(hash->entries, sizeof(entry_t *) * hash->entry_count);

    hash->hash_func = h;
    hash->equal_func = e;
    hash->key_dealloc = kd;
    hash->val_dealloc = vd;

    return hash;
}

hash_t* hash_str_key_create(deallocator_t kd, deallocator_t vd) {
    return hash_create(str_hash_func, str_equal_func, kd, vd);
}

hash_t* hash_int_key_create(deallocator_t vd) {
    return hash_create(int_hash_func, int_equal_func, NULL, vd);
}

static inline entry_t* find_entry(const hash_t *h, const void *key) {
    int         hash_val;
    entry_t     *e;

    hash_val = h->hash_func(key);
    e = h->entries[hash_val % h->entry_count];
    for (;e != NULL; e = e->next) {
        if (e->hash_val != hash_val ||
                !h->equal_func(e->key, key)) {
            continue;
        }

        return e;
    }

    return NULL;
}

int hash_put(hash_t *h, const void *key, const void *val) {
    entry_t     *e, **e_bucket;

    if (h == NULL) {
        OCTOPUS_ERROR_LOG("invalid param");
        return OCTOPUS_ERR;
    }

    if (h->hash_func == NULL || h->equal_func == NULL) {
        OCTOPUS_ERROR_LOG("hash func and equal func must be set for hash");
        return OCTOPUS_ERR;
    }

    e = find_entry(h, key);
    if (e != NULL) {
        if (h->val_dealloc != NULL) {
            h->val_dealloc((char *)e->val);
        }
        e->val = val;

        return OCTOPUS_OK;
    }

    e = calloc(1, sizeof(entry_t));
    if (e == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for entry");
        return OCTOPUS_ERR;
    }

    e->hash_val = h->hash_func(key);
    e->key = key;
    e->val = val;

    e_bucket = &h->entries[e->hash_val % h->entry_count];
    e->next = *e_bucket;
    *e_bucket = e;

    h->size++;

    return OCTOPUS_OK;
}

void* hash_get(const hash_t *h, const void *key) {
    entry_t     *e;

    if (h == NULL) {
        OCTOPUS_ERROR_LOG("invalid param");
        return NULL;
    }

    if (h->hash_func == NULL || h->equal_func == NULL) {
        OCTOPUS_ERROR_LOG("hash func and equal func must be set for hash");
        return NULL;
    }

    e = find_entry(h, key);
    if (e == NULL) {
        return NULL;
    }

    return (void *)e->val;
}

static inline void entry_free(hash_t *h, entry_t *e) {
    if (h->key_dealloc != NULL) {
        h->key_dealloc((void *)e->key);
    }
    if (h->val_dealloc != NULL) {
        h->val_dealloc((void *)e->val);
    }

    free(e);
}

void hash_remove(hash_t *h, const void *key) {
    int         hash_val;
    entry_t     *e, *prev;

    if (h == NULL) {
        OCTOPUS_ERROR_LOG("invalid param");
        return;
    }

    if (h->hash_func == NULL || h->equal_func == NULL) {
        OCTOPUS_ERROR_LOG("hash func and equal func must be set for hash");
        return;
    }

    hash_val = h->hash_func(key);
    e = h->entries[hash_val % h->entry_count];
    // find the entry and the previous entry
    for (prev = NULL; e != NULL; e = e->next, prev = e) {
        if (e->hash_val != hash_val ||
                !h->equal_func(e->key, key)) {
            continue;
        }

        // Found the entry
        if (prev == NULL) {
            h->entries[hash_val % h->entry_count] = NULL;
        } else {
            prev->next = e->next;
        }
        entry_free(h, e);
        h->size--;

        return;
    }
}

static inline int find_next_nonnull_entry(hash_iterator_t *hash_iter) {
    hash_t              *h;

    h = hash_iter->hash;
    // Find next non-null entry
    do {
        hash_iter->entry_index++;
    } while (hash_iter->entry_index < h->entry_count &&
            h->entries[hash_iter->entry_index] == NULL);

    if (hash_iter->entry_index >= h->entry_count) {
        return 0;
    }

    return 1;
}

void* hash_iter_next(void *iter) {
    hash_iterator_t     *hash_iter;
    entry_t             *e;

    hash_iter = (hash_iterator_t *)iter;

    e = hash_iter->cur_entry;
    if (e == NULL) {
        return NULL;
    }

    if (e->next != NULL) {
        hash_iter->cur_entry = e->next;
    } else {
        // Move to next entry bucket
        if (find_next_nonnull_entry(hash_iter)) {
            hash_iter->cur_entry = hash_iter->hash->entries[hash_iter->entry_index];
        } else {
            hash_iter->cur_entry = NULL;
        }
    }

    hash_iter->idx++;
    hash_iter->pair.first = (void *)e->key;
    hash_iter->pair.second = (void *)e->val;

    return &hash_iter->pair;
}

int hash_iter_has_next(void *iter) {
    hash_iterator_t     *hash_iter;

    hash_iter = (hash_iterator_t *)iter;

    return hash_iter->cur_entry != NULL;
}

int hash_iter_index(void *iter) {
    return ((hash_iterator_t *)iter)->idx;
}

iterator_t* hash_iter(hash_t *h) {
    hash_iterator_t     *hash_iter;

    hash_iter = (hash_iterator_t *)calloc(1, sizeof(hash_iterator_t));
    if (hash_iter == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for hash iterator");
        return NULL;
    }

    hash_iter->next = hash_iter_next;
    hash_iter->has_next = hash_iter_has_next;
    hash_iter->index = hash_iter_index;

    hash_iter->hash = h;
    // Set to -1, because entry_index will advance preferentially in find_next_nonnull_entry
    hash_iter->entry_index = -1;
    if (find_next_nonnull_entry(hash_iter)) {
        hash_iter->cur_entry = h->entries[hash_iter->entry_index];
    } else {
        hash_iter->cur_entry = NULL;
    }
    hash_iter->idx = 0;
    hash_iter->has_next_first = 1;

    return (iterator_t *)hash_iter;
}

void hash_iter_destroy(iterator_t *iter) {
    free(iter);
}

int hash_size(const hash_t *h) {
    return h->size;
}

void hash_destroy(hash_t *h) {
    entry_t     *e, *f;

    if (h == NULL) {
        return;
    }

    for (int i = 0; i < h->entry_count; i++) {
        e = h->entries[i];

        while (e != NULL) {
            f = e;
            e = e->next;

            if (h->key_dealloc != NULL) {
                h->key_dealloc((void *)e->key);
            }
            if (h->val_dealloc != NULL) {
                h->val_dealloc((void *)e->val);
            }

            free(f);
        }
    }

    free(h->entries);
    free(h);
}

#ifdef OCTOPUS_TEST_HASH

#include <stdio.h>

#define test_join(h, a, b) a##_##b(h)

int main(int argc, char *argv[])
{
    hash_t      *h, *int_hash;
    iterator_t  *iter;
    pair_t      *p;

    h = hash_str_key_create(NULL, NULL);
    hash_put(h, "abc", "dff");
    hash_put(h, "fff", "ddd");
    hash_put(h, "fff", "xxxddd");
    hash_put(h, "xfff", "x111xxddd");

    printf("%s -> %s\n", "abc", (char *)hash_get(h, "abc"));
    printf("%s -> %s\n", "fff", (char *)hash_get(h, "fff"));

    hash_remove(h, "abc");
    printf("%s -> %s\n", "abc", (char *)hash_get(h, "abc"));

    for (iter = hash_iter(h); iter->has_next(iter);) {
        p = (pair_t *)iter->next(iter);
        printf("%d: %s -> %s\n", iter->index(iter), (char *)p->first, (char *)p->second);
    }
    hash_iter_destroy(iter);

    int_hash = hash_int_key_create(NULL);
    for (int i = 1000; i < 1010; i++) {
        hash_put(int_hash, (void *)i, (void *)(i - 888));
    }

    printf("%d: %d\n", 1000, (int)hash_get(int_hash, 1000));
    printf("%d: %d\n", 1001, (int)hash_get(int_hash, 1001));

    printf("size: %d\n", test_join(h, hash, size));

    for (iter = hash_iter(int_hash); iter->has_next(iter);) {
        p = (pair_t *)iter->next(iter);
        printf("%d: %d->%d\n", iter->index(iter), (int)p->first, (int)p->second);
    }
    hash_iter_destroy(iter);

    return 0;
}

#endif
