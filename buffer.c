/**
 *
 * @file    buffer
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-05-24 16:19:57
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "common.h"
#include "buffer.h"
#include "logging.h"

#define buffer_produce(b, nbytes)       (b)->end = ((b)->end + nbytes) % (b)->size
#define buffer_consume(b, nbytes)       (b)->start = ((b)->start + nbytes) % (b)->size

typedef struct buffer_iterator_s {
    void* (*next)(void *iter);
    int (*has_next)(void *iter);
    int (*index)(void *iter);

    buffer_t    *buf;
    int         idx;
} buffer_iterator_t;

buffer_t* buffer_create(int size) {
    buffer_t    *buf;

    buf = malloc(sizeof(buffer_t));
    if (buf == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for buffer_t");
        return NULL;
    }
    bzero(buf, sizeof(buffer_t));
    buf->size = size;

    buf->buf = malloc(size);
    if (buf->buf == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for buffer");
        free(buf);
        return NULL;
    }

    return buf;
}

static inline bool space_is_continuous(buffer_t *buf, int size) {
    if (buf->end >= buf->start) {
        return buf->size - buf->end > size;
    }

    return buf->start - buf->end > size;
}

int buffer_write_from(buffer_t *buf, void *src, int wsize) {
    char    *b;
    int     first_part_len;

    if (buffer_space_remaining(buf) < wsize) {
        OCTOPUS_ERROR_LOG("no space for the write, remaining: %d, write: %d",
                buffer_space_remaining(buf), wsize);
        return OCTOPUS_ERR;
    }

    b = buf->buf + buf->end;
    if (space_is_continuous(buf, wsize)) {
        memmove(b, src, wsize);
        buffer_produce(buf, wsize);

        return OCTOPUS_OK;
    }

    first_part_len = buf->size - buf->end;
    memmove(b, src, first_part_len);
    memmove(buf->buf, src + first_part_len, wsize - first_part_len);
    buffer_produce(buf, wsize);

    return OCTOPUS_OK;
}

int buffer_write_from_fd(buffer_t *buf, int fd, int wsize) {
    char    *b;
    int     ret, left, rsize;

    if (buffer_space_remaining(buf) < wsize) {
        OCTOPUS_ERROR_LOG("no space for the write, remaining: %d, write: %d",
                buffer_space_remaining(buf), wsize);
        return OCTOPUS_ERR;
    }

    b = buf->buf + buf->end;
    if (space_is_continuous(buf, wsize)) {
        if ((ret = read(fd, b, wsize)) == -1 && errno != EAGAIN) {
            OCTOPUS_ERROR_LOG_BY_ERRNO("failed to write buffer from fd, read error, fd: %d", fd);
            return OCTOPUS_ERR;
        } else if (ret == 0) {
            OCTOPUS_DEBUG_LOG("EOF when read fd to write to buffer");
            return OCTOPUS_EOF;
        } else if (ret > 0) {
            // It's allowed that content read is less than 'wsize'.
            buffer_produce(buf, ret);
            return ret;
        } else if (errno == EAGAIN) {
            return 0;
        }
    }

    // If buffer isn't continuous, two read operations will be tried.
    left = wsize;
    while (left > 0) {
        if (left == wsize) {
            rsize = buf->size - buf->end;   // the first part
        } else {
            rsize = wsize - left;     // the second part
        }

        if ((ret = read(fd, b, rsize)) == -1 && errno != EAGAIN) {
            OCTOPUS_ERROR_LOG_BY_ERRNO("failed to write buffer from fd, read error");
            return OCTOPUS_ERR;
        } else if (ret == 0) {
            OCTOPUS_WARNING_LOG("EOF when read fd to write to buffer");
            return OCTOPUS_EOF;
        } else if (errno == EAGAIN) {
            return 0;
        }

        buffer_produce(buf, ret);
        left -= ret;

        if (ret < rsize) {
            // No more data can be read from fd, just return.
            return wsize - left;
        }
    }

    return wsize;
}

int buffer_write_from_sds(buffer_t *buf, sds s) {
    int     len, first_part_len;

    len = sdslen(s);
    if (buffer_space_remaining(buf) < len) {
        OCTOPUS_ERROR_LOG("no space for the write, remaining: %d, write: %d",
                buffer_space_remaining(buf), len);
        return OCTOPUS_ERR;
    }

    if (space_is_continuous(buf, len)) {
        memmove(buf->buf + buf->start, s, len);
        buf->end += len;
        return OCTOPUS_OK;
    }

    first_part_len = buf->size - buf->end;
    memmove(buf->buf + buf->start, s, first_part_len);
    memmove(buf->buf, s + first_part_len, len - first_part_len);
    buf->end += len;

    return OCTOPUS_OK;
}

int buffer_index_of(buffer_t *buf, char *s) {
    unsigned long   j, slen;

    slen = strlen(s);
    // 's' is a small string. So, here we use the simplest way to find
    // the first occurance of 's'.
    for (int i = buf->start; i != buf->end; i = (i + 1) % buf->size) {
        for (j = 0; j < slen; j++) {
            if (buf->buf[(i + j) % buf->size] != s[j]) {
                break;
            }
        }

        if (s[j] == '\0') return i - buf->start;
    }

    return OCTOPUS_NOT_FOUND;
}

int buffer_index_of_char(buffer_t *buf, char c) {
    for (int i = buf->start; i != buf->end; i = (i + 1) % buf->size) {
        if (buf->buf[i] == c) {
            return i - buf->start;
        }
    }

    return -1;
}

/*sds buffer_read_to_idx(buffer_t *buf, int idx) {*/
/*    sds     s;*/

/*    if (idx >= buf->start) {*/
/*        s = sdsnewlen(buf->buf + buf->start, idx - buf->start + 1);*/
/*    } else {*/
/*        s = sdsempty();*/
/*        sdscatlen(s, buf->buf + buf->start, buf->size - buf->start);*/
/*        sdscatlen(s, buf->buf, idx);*/
/*    }*/

/*    buf->start = idx + 1;*/

/*    return s;*/
/*}*/

void buffer_destroy(buffer_t *buf) {
    free(buf->buf);
    free(buf);
}

int buffer_read_to_fd(buffer_t *buf, int fd, int rsize) {
    int     ret, w, wsize;

    if (buffer_content_len(buf) < rsize) {
        OCTOPUS_ERROR_LOG("no enough data to read, content len: %d, read size: %d",
                buffer_content_len(buf), rsize);
        return OCTOPUS_ERR;
    }

    if (space_is_continuous(buf, rsize)) {
        // If the socket buffer is full, will 'write' block?
        // No. If the socket is in nonblock mode, 'write' will return error when the buffer
        // is full, and the error is EAGAIN.
        if ((ret = write(fd, buf->buf + buf->start, rsize)) == -1 && errno == EAGAIN) {
            // buffer is full, retry later
            return ret;
        } else if (errno == ECONNRESET) {
            OCTOPUS_ERROR_LOG_BY_ERRNO("failed to write socket, fd: %d", fd);
            return OCTOPUS_RESET;
        } else if (ret == -1) {
            OCTOPUS_ERROR_LOG("failed to write socket, fd: %d", fd);
            return OCTOPUS_ERR;
        }

        buffer_consume(buf, ret);

        return ret;
    }

    // If buffer isn't continuous, two write operations will be tried.
    w = rsize;
    while (w > 0) {
        if (w == rsize) {   // The first write
            wsize = buf->size - buf->start;
        } else {            // The second write
            wsize = rsize - w;
        }

        if ((ret = write(fd, buf->buf + buf->start, wsize)) == -1 && errno == EAGAIN) {
            // buffer is full, retry later
            return rsize - w + ret;
        } else if (errno == ECONNRESET) {
            OCTOPUS_ERROR_LOG_BY_ERRNO("failed to write socket, fd: %d", fd);
            return OCTOPUS_RESET;
        } else if (ret == -1) {
            OCTOPUS_ERROR_LOG("failed to write socket, fd: %d", fd);
            return OCTOPUS_ERR;
        }

        w -= ret;
        buffer_consume(buf, ret);
    }

    return rsize;
}

int buffer_read_to(buffer_t *buf, char *cbuf, int rsize) {
    int     to_read, first_part_len;

    if (buffer_content_len(buf) < rsize) {
        rsize = buffer_content_len(buf);
    }

    if (space_is_continuous(buf, rsize)) {
        memmove(cbuf, buf->buf + buf->start, rsize);
        buffer_consume(buf, rsize);

        return rsize;
    }

    // [start, size)
    first_part_len = buf->size - buf->start;
    memmove(cbuf, buf->buf + buf->start, first_part_len);
    to_read = rsize - first_part_len;
    buffer_consume(buf, first_part_len);

    // [0, end)
    memmove(cbuf + first_part_len, buf->buf + buf->start, to_read);
    buffer_consume(buf, to_read);

    return rsize;
}

int buffer_read_to_sds(buffer_t *buf, sds *s, int rsize) {
    int     first_part_len;

    if (space_is_continuous(buf, rsize)) {
        *s = sdscatlen(*s, buf->buf + buf->start, rsize);
        buffer_consume(buf, rsize);
        return rsize;
    }

    first_part_len = buf->size - buf->start;
    *s = sdscatlen(*s, buf->buf + buf->start, first_part_len);
    buffer_consume(buf, first_part_len);
    *s = sdscatlen(*s, buf->buf + buf->start, rsize - first_part_len);
    buffer_consume(buf, rsize - first_part_len);

    return rsize;
}

void* buffer_iter_next(void *iter) {
    buffer_iterator_t   *buf_iter;
    char        *c;
    buffer_t    *buf;

    buf_iter = (buffer_iterator_t *)iter;
    buf = buf_iter->buf;
    c = buf->buf + buf->start;
    buf->start = (buf->start + 1) % buf->size;
    buf_iter->idx++;

    return (void *)c;
}

int buffer_iter_has_next(void *iter) {
    buffer_iterator_t   *buf_iter;
    buffer_t    *buf;

    buf_iter = (buffer_iterator_t *)iter;
    buf = buf_iter->buf;

    return buffer_content_len(buf) > 0;
}

int buffer_iter_index(void *iter) {
    buffer_iterator_t   *buf_iter;

    buf_iter = (buffer_iterator_t *)iter;

    return buf_iter->idx;
}

iterator_t* buffer_iter(buffer_t *buf) {
    buffer_iterator_t   *buf_iter;

    buf_iter = calloc(1, sizeof(buffer_iterator_t));
    if (buf_iter == NULL) {
        OCTOPUS_ERROR_LOG("failed to alloc mem for buffer iterator");
        return NULL;
    }

    buf_iter->next = buffer_iter_next;
    buf_iter->has_next = buffer_iter_has_next;
    buf_iter->index = buffer_iter_index;
    buf_iter->buf = buf;

    return (iterator_t *)buf_iter;
}

void buffer_iter_destroy(iterator_t *iter) {
    free(iter);
}
