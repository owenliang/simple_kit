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
#include "sio.h"

static void stdin_callback(struct sio *sio, struct sio_fd *sfd, enum sio_event event, void *arg)
{
    switch (event) {
    case SIO_READ:
        fprintf(stderr, "SIO_READ\n");
        break;
    case SIO_WRITE:
        fprintf(stderr, "SIO_WRITE\n");
        break;
    case SIO_ERROR:
        fprintf(stderr, "SIO_ERROR\n");
        break;
    default:
        fprintf(stderr, "unknown event\n");
        break;
    }
}

static void timer_callback(struct sio *sio, struct sio_timer *timer, void *arg)
{
    fprintf(stderr, "timer_callback\n");
    sio_start_timer(sio, timer, 2000, timer_callback, NULL);
}

int main(int argc, char **argv)
{
    struct sio *sio = sio_new();
    if (!sio) {
        fprintf(stderr, "sio_new returns NULL\n");
        return -1;
    }
    struct sio_fd *sfd = sio_add(sio, 0, stdin_callback, NULL);
    if (!sfd) {
        fprintf(stderr, "sio_add returns NULL\n");
        return -1;
    }
    sio_watch_read(sio, sfd);
    struct sio_timer timer;
    sio_start_timer(sio, &timer, 2000, timer_callback, NULL);
    int i;
    for (i = 0; i < 10; ++i) 
        sio_run(sio, 1000);
    sio_del(sio, sfd);
    sio_stop_timer(sio, &timer);
    sio_free(sio);
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
