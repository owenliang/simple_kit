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
    if (sio_dgram_response(sio, sdgram, source, data, size) == -1) {
        printf("sio_dgram_response:-1\n");
    }
}

int main(int argc, char **argv)
{
    struct sio *sio = sio_new();
    if (!sio) {
        printf("sio_new fails\n");
        return -1;
    }

    struct sio_dgram *sdgram = sio_dgram_open(sio, "0.0.0.0", 8990, on_dgram, NULL);
    if (!sdgram) {
        printf("sio_dgram_open fails\n");
        sio_free(sio);
        return -1;
    }

    int i;
    for (i = 0; i < 20; ++i) {
        sio_run(sio, 1000);
    }
    sio_dgram_close(sio, sdgram);
    sio_free(sio);
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
