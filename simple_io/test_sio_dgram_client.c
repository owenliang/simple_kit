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
#include <string.h>
#include <signal.h>
#include "sio.h"
#include "sio_dgram.h"

static void on_dgram(struct sio *sio, struct sio_dgram *sdgram, struct sockaddr_in *source, char *data, uint64_t size, void *arg)
{
    printf("on_dgram=%.*s\n", (int)size, data);
}

static void on_timer(struct sio *sio, struct sio_timer *timer, void *arg)
{
    struct sio_dgram *sdgram = arg;
    if (sio_dgram_write(sio, sdgram, "127.0.0.1", 8990, "ping\n", 5) == -1) {
        printf("sio_dgram_write=-1\n");
    }
    sio_start_timer(sio, timer, 1000, on_timer, sdgram);
}

static char client_quit = 0;

static void sio_dgram_quit_handler(int signo)
{
    client_quit = 1;
}

static void sio_dgram_client_signal()
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = sio_dgram_quit_handler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
}

int main(int argc, char **argv)
{
    sio_dgram_client_signal();

    struct sio *sio = sio_new();
    if (!sio) {
        printf("sio_new=NULL\n");
        return -1;
    }

    struct sio_dgram *sdgram = sio_dgram_open(sio, "0.0.0.0", 0, on_dgram, NULL);
    if (!sdgram) {
        printf("sio_dgram_open=NULL\n");
        sio_free(sio);
        return -1;
    }

    struct sio_timer timer;
    sio_start_timer(sio, &timer, 1000, on_timer, sdgram);

    while (!client_quit)
        sio_run(sio, 1000);

    sio_dgram_close(sio, sdgram);
    sio_stop_timer(sio, &timer);
    sio_free(sio);
    printf("dgram_client=quit\n");
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
