/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file test_sio.c
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/03/29 20:09:29
 * @version $Revision$ 
 * @brief 
 *  
 **/

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
