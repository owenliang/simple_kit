/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file test_sio_stream_fork_server.c
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/06/01 10:26:53
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include "sio.h"
#include "sio_stream.h"
static void sio_stream_callback(struct sio *sio, struct sio_stream *stream, enum sio_stream_event event, void *arg)
{
    printf("%u accept\n", getpid());
    sio_stream_close(sio, stream);
}

int main(int argc, char **argv)
{
    struct sio *sio = sio_new();
    assert(sio);
    struct sio_stream *stream = sio_stream_listen(sio, "0.0.0.0", 8989, sio_stream_callback, NULL);
    assert(stream);
    sio_stream_detach(sio, stream);

    int i;
    for (i = 0; i < 8; ++i) {
        pid_t pid = fork();
        if (pid > 0) {
            printf("fork children[%d]\n", i);
        } else if (pid == 0) {
            sio_free(sio);
            sio = sio_new();
            assert(sio);
            assert(sio_stream_attach(sio, stream) == 0);
            break;
        } else {
            assert(0);
        }
    }
    if (i < 8) {
        int j;
        for (j = 0; j < 20; ++j)
            sio_run(sio, 1000);
    } else {
        while (1) {
            pid_t pid = wait(NULL);
            if (pid == (pid_t)-1 && errno == ECHILD)
                break;
        }
    }
    sio_stream_close(sio, stream);
    sio_free(sio);
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
