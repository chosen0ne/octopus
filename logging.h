/**
 *
 * @file    logging
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-05-04 16:50:09
 */

#ifndef OCTOPUS_LOGGING_H
#define OCTOPUS_LOGGING_H

#include <stdint.h>
#include <stdio.h>

typedef int octopus_log_level_t;

extern octopus_log_level_t   logging_level;

#define OCTOPUS_LOGGING_LEVEL_DEBUG      0
#define OCTOPUS_LOGGING_LEVEL_TRACE      1
#define OCTOPUS_LOGGING_LEVEL_INFO       2
#define OCTOPUS_LOGGING_LEVEL_WARNING    3
#define OCTOPUS_LOGGING_LEVEL_ERROR      4

#define OCTOPUS_DEBUG_LOG(fmt, ...)  \
    octopus_log(OCTOPUS_LOGGING_LEVEL_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define OCTOPUS_TRACE_LOG(fmt, ...)  \
    octopus_log(OCTOPUS_LOGGING_LEVEL_TRACE, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define OCTOPUS_INFO_LOG(fmt, ...)   \
    octopus_log(OCTOPUS_LOGGING_LEVEL_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define OCTOPUS_WARNING_LOG(fmt, ...)    \
    octopus_log(OCTOPUS_LOGGING_LEVEL_WARNING, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define OCTOPUS_ERROR_LOG(fmt, ...)  \
    octopus_log(OCTOPUS_LOGGING_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define OCTOPUS_ERROR_LOG_BY_ERRNO(fmt, ...)     \
    octopus_log_errno(OCTOPUS_LOGGING_LEVEL_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

int octopus_log_init(const char *logpath);
void octopus_set_log_level(octopus_log_level_t level);
void octopus_log(octopus_log_level_t level, const char *fname, int32_t lineno, const char *fmt, ...);
void octopus_log_errno(
        octopus_log_level_t level,
        const char *fname,
        int32_t lineno,
        const char *fmt, ...);
void octopus_stderr(const char *fmt, ...);

#endif /* ifndef OCTOPUS_LOGGING_H */
