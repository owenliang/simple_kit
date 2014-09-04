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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include "sio_timer.h"
#include "sio.h"

enum sio_select_event {
    SIO_SELECT_READ  = 0x01,
    SIO_SELECT_WRITE = 0x02,
    SIO_SELECT_ERROR = 0x04,
};

/* 注册在sio的文件描述符 */
struct sio_fd {
    int fd;     /* 用户监听的fd */
    uint32_t watch_events; /* 注册的事件 */
    uint32_t revents; /* 发生的事件 */
    sio_callback_t user_callback; /* 用户的事件回调 */
    void *user_arg; /* 用户参数 */
    char is_del; /* 被sio_del移除 */
};

/* 文件描述符管理器 */
struct sio {
    fd_set rset; /* 注册读集合 */
    fd_set wset; /* 注册写集合 */
    fd_set eset; /* 注册错误集合 */
    struct sio_fd *fds[FD_SETSIZE]; /* 注册了哪些fd */
    struct sio_fd *rfds[FD_SETSIZE]; /* 本次select返回了哪些fd */
    char is_in_loop;    /* 是否正在select事件处理循环中 */
    int deferred_count; /* 延迟待删除sio_fd个数 */
    int deferred_capacity; /* 延迟待删除数组的大小 */
    struct sio_fd **deferred_to_close; /* 延迟待删除sio_fd数组 */
    int wake_pipe[2];  /* 唤醒sio_run的管道 */
    struct sio_fd *wake_sfd; /* 注册在sio上的wake_pipe[0] */
    struct sio_timer_manager *st_mgr;         /**< 定时器管理器       */
};

static void _sio_wakeup_callback(struct sio *sio, struct sio_fd *sfd, int fd, enum sio_event event, void *arg)
{
    char buffer[1024];
    read(fd, buffer, sizeof(buffer));
}

struct sio *sio_new()
{   
    /* 忽略SIPIPE信号 */
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, NULL); 
    
    struct sio *sio = calloc(1, sizeof(*sio));
    FD_ZERO(&sio->rset);
    FD_ZERO(&sio->wset);
    FD_ZERO(&sio->eset);
    do {
        if (pipe(sio->wake_pipe) == -1) {
            break;
        }
        fcntl(sio->wake_pipe[0], F_SETFL, fcntl(sio->wake_pipe[0], F_GETFL) | O_NONBLOCK);
        fcntl(sio->wake_pipe[1], F_SETFL, fcntl(sio->wake_pipe[1], F_GETFL) | O_NONBLOCK);
        sio->wake_sfd = sio_add(sio, sio->wake_pipe[0], _sio_wakeup_callback, sio);
        if (!sio->wake_sfd) {
            close(sio->wake_pipe[0]);
            close(sio->wake_pipe[1]);
            break;
        }
        sio_watch_read(sio, sio->wake_sfd);
        sio->st_mgr = sio_timer_new();
        return sio;
    } while (0);
    free(sio);
    return NULL;
}

void sio_free(struct sio *sio)
{
    sio_del(sio, sio->wake_sfd);
    close(sio->wake_pipe[0]);
    close(sio->wake_pipe[1]);
    free(sio->deferred_to_close);
    sio_timer_free(sio->st_mgr);
    free(sio);
}

struct sio_fd *sio_add(struct sio *sio, int fd, sio_callback_t callback, void *arg)
{
    if (fd < 0 || fd >= FD_SETSIZE)
        return NULL;
    struct sio_fd *sfd = calloc(1, sizeof(*sfd));
    sfd->fd = fd;
    sfd->user_callback = callback;
    sfd->user_arg = arg;
    sfd->watch_events = SIO_SELECT_ERROR;
    sio->fds[fd] = sfd;
    FD_SET(fd, &sio->eset);
    return sfd;
}

void sio_set(struct sio *sio, struct sio_fd *sfd, sio_callback_t callback, void *arg)
{
    sfd->user_callback = callback;
    sfd->user_arg = arg;
}

void sio_del(struct sio *sio, struct sio_fd *sfd)
{
    FD_CLR(sfd->fd, &sio->rset);
    FD_CLR(sfd->fd, &sio->wset);
    FD_CLR(sfd->fd, &sio->eset);
    sio->fds[sfd->fd] = NULL;
    if (sio->is_in_loop) {
        sfd->is_del = 1;
        if (sio->deferred_count == sio->deferred_capacity)
            sio->deferred_to_close = realloc(sio->deferred_to_close, 
                    ++sio->deferred_capacity * sizeof(struct sio_fd *));
        sio->deferred_to_close[sio->deferred_count++] = sfd;
        return;
    }
    free(sfd);
}

void sio_watch_write(struct sio *sio, struct sio_fd *sfd)
{
    sfd->watch_events |= SIO_SELECT_WRITE;
    FD_SET(sfd->fd, &sio->wset);
}

void sio_unwatch_write(struct sio *sio, struct sio_fd *sfd)
{
    sfd->watch_events &= ~SIO_SELECT_WRITE;
    FD_CLR(sfd->fd, &sio->wset);
}

void sio_watch_read(struct sio *sio, struct sio_fd *sfd)
{
    sfd->watch_events |= SIO_SELECT_READ;
    FD_SET(sfd->fd, &sio->rset);
}

void sio_unwatch_read(struct sio *sio, struct sio_fd *sfd)
{
    sfd->watch_events &= ~SIO_SELECT_READ;
    FD_CLR(sfd->fd, &sio->rset);
}

static uint64_t _sio_cur_time_ms()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static void _sio_timer_run(struct sio *sio)
{
    uint64_t now = _sio_cur_time_ms();
    
    /* 防止用户循环投递超时为0的timer造成死循环 */
    uint64_t max_times = sio_timer_size(sio->st_mgr);
    uint64_t cur_times = 0;

    while (sio_timer_size(sio->st_mgr) && cur_times++ < max_times) {
        struct sio_timer *timer = sio_timer_top(sio->st_mgr);
        if (timer->expire <= now) {
            sio_timer_pop(sio->st_mgr);
            timer->user_callback(sio, timer, timer->user_arg);
        } else {
            break;
        }   
    }
}

static int _sio_calc_timeout(struct sio *sio)
{
    if (!sio_timer_size(sio->st_mgr))
        return 1000; /* 默认1s */
    
    uint64_t now = _sio_cur_time_ms();
    struct sio_timer *timer = sio_timer_top(sio->st_mgr);
    if (timer->expire <= now)
        return 0; /* 已经有任务超时 */
    uint64_t period = timer->expire - now; 
    if (period >= 1000) /* 最多挂起1s */
        return 1000;
    return period; /* 否则挂起到最近一个任务超时时间 */
}

void sio_run(struct sio *sio)
{
    _sio_timer_run(sio);

    int timeout = _sio_calc_timeout(sio);

    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    
    fd_set rset = sio->rset;
    fd_set wset = sio->wset;
    fd_set eset = sio->eset;
    int event_count = select(FD_SETSIZE, &rset, &wset, &eset, &tv); 

    if (event_count <= 0)
        return;
    /* 遍历所有fd, 先记录下select发生事件的sfd以及事件 */
    int r_idx = 0;
    int fd;
    for (fd = 0; fd < FD_SETSIZE; ++fd) {
        sio->rfds[r_idx] = sio->fds[fd];
        if (!sio->rfds[r_idx])
            continue;
        sio->rfds[r_idx]->revents = 0;
        if (FD_ISSET(fd, &rset))
            sio->rfds[r_idx]->revents |= SIO_SELECT_READ;
        if (FD_ISSET(fd, &wset))
            sio->rfds[r_idx]->revents |= SIO_SELECT_WRITE;
        if (FD_ISSET(fd, &eset))
            sio->rfds[r_idx]->revents |= SIO_SELECT_ERROR;
        if (sio->rfds[r_idx]->revents)
            r_idx++;
    }
    sio->is_in_loop = 1;
    int i; 
    for (i = 0; i < r_idx; ++i) {
        struct sio_fd *sfd = sio->rfds[i];
        if (sfd->is_del)
            continue;
        uint32_t events = sfd->revents;
        if ((events & SIO_SELECT_READ) && (sfd->watch_events & SIO_SELECT_READ))
            sfd->user_callback(sio, sfd, sfd->fd, SIO_READ, sfd->user_arg);
        if (!sfd->is_del && (events & SIO_SELECT_WRITE) && (sfd->watch_events & SIO_SELECT_WRITE))
            sfd->user_callback(sio, sfd, sfd->fd, SIO_WRITE, sfd->user_arg);
        if (!sfd->is_del && (events & SIO_SELECT_ERROR))
            sfd->user_callback(sio, sfd, sfd->fd, SIO_ERROR, sfd->user_arg);
    }
    sio->is_in_loop = 0;
    for (i = 0; i < sio->deferred_count; ++i) 
        free(sio->deferred_to_close[i]);
    sio->deferred_count = 0;
}

void sio_wakeup(struct sio *sio)
{
    while (write(sio->wake_pipe[1], "", 1) == -1 && errno == EINTR);
}

void sio_start_timer(struct sio *sio, struct sio_timer *timer, uint64_t timeout_ms, sio_timer_callback_t callback, void *arg)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    uint64_t expire = tv.tv_sec * 1000 + tv.tv_usec / 1000 + timeout_ms;

    timer->expire = expire;
    timer->user_callback = callback; 
    timer->user_arg = arg;
    sio_timer_insert(sio->st_mgr, timer);
}

void sio_stop_timer(struct sio *sio, struct sio_timer *timer)
{
    sio_timer_remove(sio->st_mgr, timer);
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
