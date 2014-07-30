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
    printf("on_dgram=%.*s", (int)size, data);
    if (sio_dgram_response(sio, sdgram, source, "pong", 5) == -1) {
        printf("sio_dgram_response=-1\n");
    }
}
static char server_quit = 0;

static void sio_dgram_quit_handler(int signo)
{
    server_quit = 1;
}

static void sio_dgram_server_signal()
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = sio_dgram_quit_handler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
}

int main(int argc, char **argv)
{
    sio_dgram_server_signal();

    struct sio *sio = sio_new();
    if (!sio) {
        printf("sio_new=NULL\n");
        return -1;
    }

    struct sio_dgram *sdgram = sio_dgram_open(sio, "0.0.0.0", 8990, on_dgram, NULL);
    if (!sdgram) {
        printf("sio_dgram_open=NULL\n");
        sio_free(sio);
        return -1;
    }

    while (!server_quit)
        sio_run(sio, 1000);

    sio_dgram_close(sio, sdgram);
    sio_free(sio);
    printf("dgram_server=quit\n");
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
