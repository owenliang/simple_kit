/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file test_slog.c
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/06/03 18:59:00
 * @version $Revision$ 
 * @brief 
 *  
 **/
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include "slog.h"

static void *level_writer(void *arg)
{
    int i;
    for (i = 0; i < SLOG_MB(1); ++i) {
        if (arg == (void *)0)
            SLOG(FATAL, "%s", "a");
        else if (arg == (void *)1)
            SLOG(ERROR, "%s", "b");
        else if (arg == (void *)2)
            SLOG(WARN, "%s", "c");
        else if (arg == (void *)3)
            SLOG(INFO, "%s", "d");
        else
            SLOG(DEBUG, "%s", "e");
    }
    return NULL;
}

void rotate_handler(int signo)
{
    slog_rotate();
}

void debug_handler(int signo)
{
    static int i = 0;
    if (i == 0)
        slog_change_level(SLOG_LEVEL_INFO);
    else
        slog_change_level(SLOG_LEVEL_DEBUG);
    i = (i + 1) % 2;
}

int main(int argc, char **argv)
{
    signal(SIGUSR1, rotate_handler);
    signal(SIGUSR2, debug_handler);

    /* 父进程初始化一次slog */
    assert(slog_open("./log", "test_slog", SLOG_LEVEL_DEBUG, SLOG_MB(100)) == 0);

    int j;
    pid_t pid;
    for (j = 0; j < 8; ++j) {
        pid = fork();   /* slog支持fork模式  */
        if (pid == 0)
            break;
        else if (pid < 0)
            assert(0);
    }

    if (pid == 0) {
        assert(slog_reopen() == 0); /* 子进程重新打开lock文件 */
    }

    pthread_t tids[5];
    int i;
    for (i = 0; i < 5; ++i) {
        pthread_create(tids + i, NULL, level_writer, (void *)i);
    }
    for (i = 0; i < 5; ++i) {
        pthread_join(tids[i], NULL);
    }
    if (pid > 0) {
        while (1) {
            pid = wait(NULL);
            if (pid == (pid_t)-1 && errno == ECHILD)
                break;
        }
    }
    slog_close();
    return 0;
}
/* vim: set ts=4 sw=4 sts=4 tw=100 */
