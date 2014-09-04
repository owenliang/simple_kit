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
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include "sio_timer.h"
#include "sio.h"

/* 注册在sio的文件描述符 */
struct sio_fd {
    int fd;     /* 用户监听的fd */
    uint32_t watch_events;    /* 用户监听的事件 */
    sio_callback_t user_callback; /* 用户的事件回调 */
    void *user_arg; /* 用户参数 */
    char is_del; /* 被sio_del移除 */
};

/* 文件描述符管理器 */
struct sio {
    int epfd; /* epoll句柄 */
    struct epoll_event poll_events[64]; /* epoll_wait的参数 */
    char is_in_loop;    /* 是否正在epoll_wait事件处理循环中 */
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
    do {
        sio->epfd = epoll_create(65536);
        if (sio->epfd == -1) 
            break;
        if (pipe(sio->wake_pipe) == -1) {
            close(sio->epfd);
            break;
        }
        fcntl(sio->wake_pipe[0], F_SETFL, fcntl(sio->wake_pipe[0], F_GETFL) | O_NONBLOCK);
        fcntl(sio->wake_pipe[1], F_SETFL, fcntl(sio->wake_pipe[1], F_GETFL) | O_NONBLOCK);
        sio->wake_sfd = sio_add(sio, sio->wake_pipe[0], _sio_wakeup_callback, sio);
        if (!sio->wake_sfd) {
            close(sio->wake_pipe[0]);
            close(sio->wake_pipe[1]);
            close(sio->epfd);
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
    close(sio->epfd);
    free(sio->deferred_to_close);
    sio_timer_free(sio->st_mgr);
    free(sio);
}

struct sio_fd *sio_add(struct sio *sio, int fd, sio_callback_t callback, void *arg)
{
    struct sio_fd *sfd = calloc(1, sizeof(*sfd));
    sfd->fd = fd;
    sfd->user_callback = callback;
    sfd->user_arg = arg;
    
    struct epoll_event add_event;
    add_event.events = 0;
    add_event.data.ptr = sfd;

    if (epoll_ctl(sio->epfd, EPOLL_CTL_ADD, fd, &add_event) == -1) {
        free(sfd);
        return NULL;
    }
    return sfd;
}

void sio_set(struct sio *sio, struct sio_fd *sfd, sio_callback_t callback, void *arg)
{
    sfd->user_callback = callback;
    sfd->user_arg = arg;
}

void sio_del(struct sio *sio, struct sio_fd *sfd)
{
    epoll_ctl(sio->epfd, EPOLL_CTL_DEL, sfd->fd, NULL);
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

static void _sio_watch_events(struct sio *sio, struct sio_fd *sfd)
{
    struct epoll_event mod_event;
    mod_event.events = sfd->watch_events;
    mod_event.data.ptr = sfd;
    epoll_ctl(sio->epfd, EPOLL_CTL_MOD, sfd->fd, &mod_event);
}

void sio_watch_write(struct sio *sio, struct sio_fd *sfd)
{
    sfd->watch_events |= EPOLLOUT;
    _sio_watch_events(sio, sfd);
}

void sio_unwatch_write(struct sio *sio, struct sio_fd *sfd)
{
    sfd->watch_events &= ~EPOLLOUT;
    _sio_watch_events(sio, sfd);
}

void sio_watch_read(struct sio *sio, struct sio_fd *sfd)
{
    sfd->watch_events |= EPOLLIN;
    _sio_watch_events(sio, sfd);
}

void sio_unwatch_read(struct sio *sio, struct sio_fd *sfd)
{
    sfd->watch_events &= ~EPOLLIN;
    _sio_watch_events(sio, sfd);
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

    int event_count = epoll_wait(sio->epfd, sio->poll_events, 64, timeout);
    if (event_count <= 0)
        return;
    sio->is_in_loop = 1;
    int i;
    for (i = 0; i < event_count; ++i) {
        struct sio_fd *sfd = sio->poll_events[i].data.ptr;
        uint32_t events = sio->poll_events[i].events;
        if (sfd->is_del)
            continue;
        if ((events & EPOLLIN) && (sfd->watch_events & EPOLLIN))
            sfd->user_callback(sio, sfd, sfd->fd, SIO_READ, sfd->user_arg);
        if (!sfd->is_del && (events & EPOLLOUT) && (sfd->watch_events & EPOLLOUT))
            sfd->user_callback(sio, sfd, sfd->fd, SIO_WRITE, sfd->user_arg);
        if (!sfd->is_del && (events & (EPOLLHUP | EPOLLERR)))
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
