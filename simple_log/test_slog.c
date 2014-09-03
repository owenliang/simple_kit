/*
 * Copyright (C) 2014-2015  liangdong <liangdong01@baidu.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "slog.h"

static void *level_writer(void *arg)
{
    int thread_idx = *(int *)arg;

    int i;
    for (i = 0; i < SLOG_MB(1); ++i) {
        switch (thread_idx) {
        case 0:
            SLOG(FATAL, "%s", "a");
            break;
        case 1:
            SLOG(ERROR, "%s", "b");
            break;
        case 2:    
            SLOG(WARN, "%s", "c");
            break;
        case 3:
            SLOG(INFO, "%s", "d");
            break;
        case 4:
            SLOG(DEBUG, "%s", "e");
            break;
        }
    }
    return arg;
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
        int *n = malloc(sizeof(*n));
        *n = i;
        pthread_create(tids + i, NULL, level_writer, n);
    }
    for (i = 0; i < 5; ++i) {
        void *idx;
        pthread_join(tids[i], &idx);
        free(idx);
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
