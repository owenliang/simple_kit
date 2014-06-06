/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file test_sio_dgram_server.cpp
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/03/31 14:29:05
 * @version $Revision$ 
 * @brief 
 *  
 **/
#include <stdio.h>
#include "sio.h"
#include "sio_dgram.h"

static void on_dgram(struct sio *sio, struct sio_dgram *sdgram, struct sockaddr_in *source, char *data, uint64_t size, void *arg)
{
    printf("%.*s", (int)size, data);
}
static void on_timer(struct sio *sio, struct sio_timer *timer, void *arg)
{
    struct sio_dgram *sdgram = arg;
    if (sio_dgram_write(sio, sdgram, "127.0.0.1", 8990, "ping\n", 5) == -1) {
        printf("sio_dgram_write:-1\n");
    }
    sio_start_timer(sio, timer, 1000, on_timer, sdgram);
}
int main(int argc, char **argv)
{
    struct sio *sio = sio_new();
    if (!sio) {
        printf("sio_new fails\n");
        return -1;
    }

    struct sio_dgram *sdgram = sio_dgram_open(sio, "0.0.0.0", 0, on_dgram, NULL);
    if (!sdgram) {
        printf("sio_dgram_open fails\n");
        sio_free(sio);
        return -1;
    }

    struct sio_timer timer;
    sio_start_timer(sio, &timer, 1000, on_timer, sdgram);

    int i;
    for (i = 0; i < 20; ++i) {
        sio_run(sio, 1000);
    }
    sio_dgram_close(sio, sdgram);
    sio_stop_timer(sio, &timer);
    sio_free(sio);
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
