/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file slog.c
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/06/03 18:58:49
 * @version $Revision$ 
 * @brief 
 *  
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include "slog.h"

static struct slog *logger = NULL;
static const char *level_desc[] = {
    "FATAL", "ERROR", "WARN", "INFO", "DEBUG"
};

static int _slog_mkdir_recursive(const char *dir)
{
    char *dup_dir = strdup(dir);
    int len = strlen(dup_dir);
    int ret = 0;

    int i = dir[0] == '/' ? 1 : 0; /* don't create root '/' dir */
    for ( ; i <= len; ++i) {
        if (dup_dir[i] == '/' || dup_dir[i] == '\0') {
            char ch = dup_dir[i];
            dup_dir[i] = '\0';
            if (mkdir(dup_dir, 0775) == -1 && errno != EEXIST) {
                ret = -1;
                break;
            }
            dup_dir[i] = ch;
        }
    }
    free(dup_dir);
    return ret;
}

static void _slog_atfork_prepare()
{
    if (logger) {
        pthread_mutex_lock(&logger->mutex); /* fork线程先hold住mutex */
    }
}

static void _slog_atfork_parent()
{
    if (logger) {
        pthread_mutex_unlock(&logger->mutex); /* 父进程(fork线程)释放掉锁 */
    }
}

static void _slog_atfork_child()
{
    if (logger) {
        pthread_mutex_unlock(&logger->mutex); /* 子进程(fork线程)释放掉锁 */
    }
}

int slog_open(const char *dir, const char *name, enum slog_level level, uint64_t rotate_size)
{
    if (_slog_mkdir_recursive(dir) == -1)
        return -1;
    char *filename = calloc(1, strlen(dir) + strlen(name) + 2);
    sprintf(filename, "%s/%s", dir, name);
    int fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        free(filename);
        return -1;
    }
    char *lockname = calloc(1, strlen(dir) + strlen(name) + 8);
    sprintf(lockname, "%s/.%s.lock", dir, name);
    int lockfd = open(lockname, O_RDONLY | O_CREAT, 0644);
    if (lockfd < 0) {
        free(lockname);
        free(filename);
        close(fd);
        return -1;
    }
    logger = calloc(1, sizeof(*logger));
    logger->fd = fd;
    logger->filename = filename;
    logger->level = level;
    logger->lockfd = lockfd;
    logger->lockname = lockname;
    logger->rotate_size = rotate_size;
    fstat(fd, &logger->fdstat);
    pthread_mutex_init(&logger->mutex, NULL);
    /* fork是安全的, 子进程会保持mutex无锁, 同时会重新打开lock文件 */
    pthread_atfork(_slog_atfork_prepare, _slog_atfork_parent, _slog_atfork_child);
    return 0;
}

int slog_reopen()
{
    if (!logger)
        return -1;
    int lockfd = open(logger->lockname, O_RDONLY | O_CREAT, 0644);
    if (lockfd < 0)
        return -1;
    dup2(lockfd, logger->lockfd);
    close(lockfd);
    return 0;
}

void slog_close()
{
    close(logger->fd);
    close(logger->lockfd);
    free(logger->filename);
    free(logger->lockname);
    pthread_mutex_destroy(&logger->mutex);
    free(logger);
    logger = NULL;
}

static void _slog_check_file(struct timeval *tv, struct tm *tm)
{
    /* multi-process safe */
    while (flock(logger->lockfd, LOCK_EX) == -1); /* maybe interrupted by signal */
    /* first, check if file was changed */
    struct stat st;
    int ret = stat(logger->filename, &st);
    if ((ret == -1 && errno == ENOENT) ||
            (ret == 0 && (logger->fdstat.st_dev != st.st_dev || logger->fdstat.st_ino != st.st_ino))) {
        int fd = open(logger->filename, O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (fd >= 0) {
            dup2(fd, logger->fd);
            close(fd);
            fstat(logger->fd, &logger->fdstat);
        }
    }
    /* second, check if file needs rotate */
    if (ret == 0 && st.st_size >= logger->rotate_size) {
        char dest_filename[PATH_MAX];
        snprintf(dest_filename, sizeof(dest_filename), "%s.%04d%02d%02d%02d%02d%02d.%06ld",
                logger->filename, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                tm->tm_hour, tm->tm_min, tm->tm_sec, tv->tv_usec);
        rename(logger->filename, dest_filename);
        int fd = open(logger->filename, O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (fd >= 0) {
            dup2(fd, logger->fd);
            close(fd);
            fstat(logger->fd, &logger->fdstat);
        }
    }
    while (flock(logger->lockfd, LOCK_UN) == -1);
}

void slog_write(enum slog_level level, const char *file, int line, const char *function, const char *fmt, ...)
{
    if (level > logger->level)
        return;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm;
    localtime_r(&tv.tv_sec, &tm);

    /* multi-thread safe */
    pthread_mutex_lock(&logger->mutex);
    _slog_check_file(&tv, &tm);
    pthread_mutex_unlock(&logger->mutex);

    char header[1024];
    int header_len;
    if (level > SLOG_LEVEL_INFO) {
        header_len = snprintf(header, sizeof(header), "[%04d-%02d-%02d %02d:%02d:%02d.%06ld] %-5s [%ld] (%s %s:%d) ",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec,
                level_desc[level], syscall(SYS_gettid), function, file, line);
    } else {
        header_len = snprintf(header, sizeof(header), "[%04d-%02d-%02d %02d:%02d:%02d.%06ld] %-5s [%ld] ",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec,
                level_desc[level], syscall(SYS_gettid));
    }
    if (header_len >= sizeof(header)) { /* truncated */
        header_len = sizeof(header) - 1;
    }
    char body[10240];
    va_list args;
    va_start(args, fmt);
    int body_len = vsnprintf(body, sizeof(body), fmt, args);
    va_end(args);
    if (body_len >= sizeof(body)) { /* truncated */
        body_len = sizeof(body) - 1;
    } else if (body_len == 0) { /* empty body */
        return;
    }
    struct iovec vec[3];
    vec[0].iov_base = header;
    vec[0].iov_len = header_len;
    vec[1].iov_base = body;
    vec[1].iov_len = body_len;
    vec[2].iov_base = "\n";
    vec[2].iov_len = 1;
    writev(logger->fd, vec, 3);
}

static void _slog_force_rotate()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm;
    localtime_r(&tv.tv_sec, &tm);

    while (flock(logger->lockfd, LOCK_EX) == -1); /* maybe interrupted by signal */
    char dest_filename[PATH_MAX];
    snprintf(dest_filename, sizeof(dest_filename), "%s.%04d%02d%02d%02d%02d%02d.%06ld",
            logger->filename, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec, tv.tv_usec);
    rename(logger->filename, dest_filename);
    int fd = open(logger->filename, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd >= 0) {
        dup2(fd, logger->fd);
        close(fd);
        fstat(logger->fd, &logger->fdstat);
    }
    while (flock(logger->lockfd, LOCK_UN) == -1); /* maybe interrupted by signal */
}

void slog_rotate()
{
    pthread_mutex_lock(&logger->mutex);
    _slog_force_rotate();
    pthread_mutex_unlock(&logger->mutex);
}

void slog_change_level(enum slog_level level)
{
    logger->level = level;
}
/* vim: set ts=4 sw=4 sts=4 tw=100 */
