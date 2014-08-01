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

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sio.h"
#include "sio_dgram.h"

static void _sio_dgram_read(struct sio *sio, struct sio_dgram *sdgram)
{
    struct sockaddr_in source;
    socklen_t len = sizeof(source);
    int64_t size = recvfrom(sdgram->sock, sdgram->inbuf, 4096, 0, (struct sockaddr *)&source, &len);
    if (size > 0) {
        sdgram->user_callback(sio, sdgram, &source, sdgram->inbuf, size, sdgram->user_arg);
    }
}

static void _sio_dgram_callback(struct sio *sio, struct sio_fd *sfd, int fd, enum sio_event event, void *arg)
{
    struct sio_dgram *sdgram = arg;

    switch (event) {
    case SIO_READ:
        _sio_dgram_read(sio, sdgram);
        break;
    case SIO_WRITE:
        break;
    case SIO_ERROR:
        break;
    default:
        return;
    }
}

struct sio_dgram *sio_dgram_open(struct sio *sio, const char *ipv4, uint16_t port, sio_dgram_callback_t callback, void *arg)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
        return NULL;
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);
    
    int bufsize = 1048576;
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ipv4);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        close(sock);
        return NULL;
    }
    struct sio_dgram *sdgram = calloc(1, sizeof(*sdgram));
    sdgram->sock = sock;
    sdgram->user_callback = callback;
    sdgram->user_arg = arg;
    sdgram->sfd = sio_add(sio, sock, _sio_dgram_callback, sdgram);
    if (!sdgram->sfd) {
        sio_dgram_close(sio, sdgram);
        return NULL;
    }
    sio_watch_read(sio, sdgram->sfd);
    return sdgram;
}

void sio_dgram_close(struct sio *sio, struct sio_dgram *sdgram)
{
    if (sdgram->sfd) {
        sio_del(sio, sdgram->sfd);
    }
    close(sdgram->sock);
    free(sdgram);
}

int sio_dgram_write(struct sio *sio, struct sio_dgram *sdgram, const char *ipv4, uint16_t port, const char *data, uint64_t size)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ipv4);

    int64_t ret = sendto(sdgram->sock, data, size, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (ret > 0)
        ret = 0;
    return ret; // -1 or 0
}

void sio_dgram_set(struct sio *sio, struct sio_dgram *sdgram, sio_dgram_callback_t callback, void *arg)
{
    sdgram->user_callback = callback;
    sdgram->user_arg = arg;
}

int sio_dgram_attach(struct sio *sio, struct sio_dgram *sdgram)
{
    sdgram->sfd = sio_add(sio, sdgram->sock, _sio_dgram_callback, sdgram);
    if (!sdgram->sfd)
        return -1;
    sio_watch_read(sio, sdgram->sfd);
    return 0;
}

void sio_dgram_detach(struct sio *sio, struct sio_dgram *sdgram)
{
    sio_del(sio, sdgram->sfd);
    sdgram->sfd = NULL;
}

int sio_dgram_response(struct sio *sio, struct sio_dgram *sdgram, struct sockaddr_in *source, const char *data, uint64_t size)
{
    int64_t ret = sendto(sdgram->sock, data, size, 0, (struct sockaddr *)source, sizeof(*source));
    if (ret > 0)
        ret = 0;
    return ret; // -1 or 0
}

int sio_dgram_peer_address(struct sockaddr_in *name, char *address, uint32_t len, uint16_t *port)
{
    if (address && !inet_ntop(AF_INET, &name->sin_addr, address, len))
        return -1;
    if (port)
        *port = ntohs(name->sin_port);
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
