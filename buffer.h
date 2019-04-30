/**
 *
 * @file    buffer
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-05-23 18:34:35
 */

#ifndef OCTOPUS_BUFFER_H
#define OCTOPUS_BUFFER_H

#include "sds.h"
#include "common.h"

#define buffer_space_remaining(b)   ((b)->size - ((b)->end - (b)->start) % (b)->size)
// idx is a relative index of buffer
// 0 <= idx < buffer_content_len(b)
#define buffer_at(b, idx)           (b)->buf[((b)->start + idx) % (b)->size]
#define buffer_current(b)           (b)->buf[(b)->start]

/**
 * @brief Make 'start' of buffer to advance 'step's
 */
#define buffer_advance_step(b, step)     (b)->start = ((b)->start + (step)) % (b)->size

// idx is a relative index of buffer
// 0 <= idx < buffer_content_len(b)
#define buffer_subbuf_len(b, idx)   ((idx) + 1) % (b)->size
#define buffer_content_len(b)       ((b)->end - (b)->start) % (b)->size

typedef struct {
    char    *buf;
    int     size;
    int     start;
    int     end;    // index to the position of next byte
} buffer_t;

buffer_t* buffer_create(int size);
int buffer_write_from(buffer_t *buf, void *src, int wsize);
int buffer_write_from_fd(buffer_t *buf, int fd, int wsize);
int buffer_write_from_sds(buffer_t *buf, sds s);
int buffer_read_to(buffer_t *buf, char *cbuf, int rsize);
int buffer_read_to_fd(buffer_t *buf, int fd, int rsize);

// read data from buffer and write to sds.
// sds must be expand memory when data is written to it, so here sds * is passed.
int buffer_read_to_sds(buffer_t *buf, sds *s, int rsize);

// return the relative index of string s
// 0 <= idx < buffer_content_len(buf)
int buffer_index_of(buffer_t *buf, char *s);

// return the relative index of char c
// 0 <= idx < buffer_content_len(buf)
int buffer_index_of_char(buffer_t *buf, char c);
iterator_t* buffer_iter(buffer_t *buf);
void buffer_iter_destroy(iterator_t *iter);

/**
 * @brief Read range '[buffer->start, idx]' of the buffer, and copy them to the sds.
 *      After the call, buffer->start will advance to 'idx'.
 */
// sds buffer_read_to_idx(buffer_t *buf, int idx);

void buffer_destroy(buffer_t *buf);

#endif /* ifndef OCTOPUS_BUFFER_H */
