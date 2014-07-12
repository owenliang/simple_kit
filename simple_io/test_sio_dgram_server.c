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
