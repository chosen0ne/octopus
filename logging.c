/**
 *
 * @file    logging
 * @author  chosen0ne(louzhenlin86@126.com)
 * @date    2018-05-04 17:09:10
 */

#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "logging.h"
#include "common.h"

#define OCTOPUS_MAX_LOG_LINE     10240

FILE    *log_fp = NULL;

octopus_log_level_t   logging_level;

int octopus_log_init(const char *logpath) {
    if (log_fp != NULL || logpath == NULL) {
        return OCTOPUS_ERR;
    }

    if (strlen(logpath) == 0) {
        // No logging file specified, and the logs will be outputed to stdout.
        log_fp = NULL;
        return OCTOPUS_OK;
    }

    log_fp = fopen(logpath, "a");
    if (!log_fp) {
        octopus_stderr("failed to open log file, path: %s", logpath);
        return OCTOPUS_ERR;
    }

    return OCTOPUS_OK;
}

int fill_time(char *buf, int buflen) {
    time_t      now;
    struct tm   now_tm;

    now = time(NULL);
    localtime_r(&now, &now_tm);

    return strftime(buf, buflen, "%Y-%m-%d %H:%M:%S", &now_tm);
}

const char* level_name(octopus_log_level_t lv) {
    switch (lv) {
    case OCTOPUS_LOGGING_LEVEL_DEBUG: return "DEBUG"; break;
    case OCTOPUS_LOGGING_LEVEL_TRACE: return "TRACE"; break;
    case OCTOPUS_LOGGING_LEVEL_INFO: return "INFO"; break;
    case OCTOPUS_LOGGING_LEVEL_WARNING: return "WARNING"; break;
    case OCTOPUS_LOGGING_LEVEL_ERROR: return "ERROR"; break;
    default: return "UNKNOWN"; break;
    }
}

void octopus_set_log_level(octopus_log_level_t level) {
    const char  *lv_name;

    lv_name = level_name(level);
    if (strcmp(lv_name, "UNKNOWN") == 0) {
        OCTOPUS_ERROR_LOG("unsupport logging level, level: %d", level);
        return;
    }

    logging_level = level;
}

void octopus_log(
        octopus_log_level_t level,
        const char *fname,
        int32_t lineno,
        const char *fmt, ...) {
    char        buf[OCTOPUS_MAX_LOG_LINE];
    FILE        *out = log_fp;
    size_t      time_size;
    int         pos_size;
    va_list     va;
    pthread_t   tid;

    if (level < logging_level) return;

    time_size = fill_time(buf, OCTOPUS_MAX_LOG_LINE);

    tid = pthread_self();
    // position infomation
    pos_size = snprintf(buf + time_size, OCTOPUS_MAX_LOG_LINE - time_size, " %s %p [%s:%d] ",
            level_name(level),
            tid,
            fname,
            lineno);

    va_start(va, fmt);
    vsnprintf(
            buf + time_size + pos_size,
            OCTOPUS_MAX_LOG_LINE - time_size - pos_size,
            fmt,
            va);
    va_end(va);

    if (!out) {
        out = stdout;
    }

    fputs(buf, out);
    fputs("\n", out);
}

void octopus_log_errno(
        octopus_log_level_t level,
        const char *fname,
        int32_t lineno,
        const char *fmt, ...) {
    char        errmsg[1024], logbuf[OCTOPUS_MAX_LOG_LINE];
    va_list     va;

    if (level < logging_level) return;

    strerror_r(errno, errmsg, 1024);

    va_start(va, fmt);
    vsnprintf(logbuf, OCTOPUS_MAX_LOG_LINE, fmt, va);
    va_end(va);

    octopus_log(level, fname, lineno, "%s, system err: %s", logbuf, errmsg);
}

void octopus_stderr(const char *fmt, ...) {
    va_list     va;
    char        buf[OCTOPUS_MAX_LOG_LINE];

    fill_time(buf, OCTOPUS_MAX_LOG_LINE);
    fputs(buf, stdout);
    fputs(" ", stdout);

    va_start(va, fmt);
    vfprintf(stdout, fmt, va);
    va_end(va);
    fputs("\n", stdout);
}

#ifdef OCTOPUS_TEST_LOGGING

int main() {
    octopus_stderr("test stderr, err: %s, xx: %d", "abc", 10);
    octopus_stderr("test stderr, err: %s, xx: %d", "abc", 10);

    OCTOPUS_ERROR_LOG("error log test, err: %s, xx: %d", "fdff", 100);
    OCTOPUS_ERROR_LOG("error log test, err: %s, xx: %d", "fdff", 100);

    OCTOPUS_ERROR_LOG_BY_ERRNO("errno log test, msg: %s", "xxxx");
}

#endif
