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
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "sio.h"
#include "sio_stream.h"

static int _sio_stream_read(struct sio *sio, struct sio_fd *sfd, int fd, struct sio_stream *stream)
{
    char *space;
    sio_buffer_reserve(stream->inbuf, 4096); /* 4KB per read */
    sio_buffer_space(stream->inbuf, &space, NULL);

    int64_t bytes = read(fd, space, 4096);
    if (bytes == -1) {
        if (errno != EINTR && errno != EAGAIN)
            return 1;
    } else if (bytes == 0) {
        return 2;
    } else {
        sio_buffer_seek(stream->inbuf, bytes);
        stream->user_callback(sio, stream, SIO_STREAM_DATA, stream->user_arg);
    }
    return 0;
}

static int _sio_stream_write(struct sio *sio, struct sio_fd *sfd, int fd, struct sio_stream *stream)
{
    char *data;
    uint64_t size;
    sio_buffer_data(stream->outbuf, &data, &size);
    int64_t bytes = write(fd, data, size);
    if (bytes == -1) {
        if (errno != EINTR && errno != EAGAIN) 
            return 1;
    } else {
        sio_buffer_erase(stream->outbuf, bytes);
        if (bytes == size)
            sio_unwatch_write(sio, sfd);
    }
    return 0;
}

static void _sio_stream_callback(struct sio *sio, struct sio_fd *sfd, int fd, enum sio_event event, void *arg)
{
    struct sio_stream *stream = arg;
    int error = 0;
    switch (event) {
    case SIO_READ:
        error = _sio_stream_read(sio, sfd, fd, stream);
        break;
    case SIO_WRITE:
        error = _sio_stream_write(sio, sfd, fd, stream);
        break;
    case SIO_ERROR:
        error = 1;
        break;
    default:
        return;
    }
    if (error == 1) { // error
        stream->user_callback(sio, stream, SIO_STREAM_ERROR, stream->user_arg);
    } else if (error == 2) { // peer-close
        stream->user_callback(sio, stream, SIO_STREAM_CLOSE, stream->user_arg);
    }
}

static void _sio_connect_callback(struct sio *sio, struct sio_fd *sfd, int fd, enum sio_event event, void *arg)
{
    struct sio_stream *stream = arg;
    
    int ret, error;
    socklen_t optlen = sizeof(error);
    switch (event) {
    case SIO_WRITE:
        ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &optlen);
        if (ret == 0 && error == 0) {
            stream->type = SIO_STREAM_NORMAL;
            sio_watch_read(sio, sfd);
            if (!sio_buffer_length(stream->outbuf))
                sio_unwatch_write(sio, sfd); 
            sio_set(sio, sfd, _sio_stream_callback, stream);
            break;
        }
    case SIO_ERROR:
        stream->user_callback(sio, stream, SIO_STREAM_ERROR, stream->user_arg);
        break;
    default:
        break;
    }
}

static struct sio_stream *_sio_stream_new(int sock, enum sio_stream_type type, 
        sio_stream_callback_t user_callback, void *user_arg)
{
    struct sio_stream *stream = calloc(1, sizeof(*stream));
    stream->sock = sock;
    stream->type = type;
    stream->user_callback = user_callback;
    stream->user_arg = user_arg;
    stream->inbuf = sio_buffer_new();
    stream->outbuf = sio_buffer_new();
    return stream;
}

static void _sio_accept_callback(struct sio *sio, struct sio_fd *sfd, int fd, enum sio_event event, void *arg)
{
    struct sio_stream *acceptor = arg;

    int sock = accept(fd, NULL, NULL);
    if (sock == -1)
        return;
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);
    int nodelay = 1;
    setsockopt(sock, SOL_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

    struct sio_stream *stream = _sio_stream_new(sock, SIO_STREAM_NORMAL, acceptor->user_callback, acceptor->user_arg);
    stream->sfd = sio_add(sio, sock, _sio_stream_callback, stream);
    if (!stream->sfd) {
        return sio_stream_close(sio, stream);
    }
    sio_watch_read(sio, stream->sfd);
    acceptor->user_callback(sio, stream, SIO_STREAM_ACCEPT, acceptor->user_arg);
}

void sio_stream_close(struct sio *sio, struct sio_stream *stream)
{
    if (stream->sfd)
        sio_del(sio, stream->sfd);
    close(stream->sock);
    sio_buffer_free(stream->inbuf);
    sio_buffer_free(stream->outbuf);
    free(stream);
}

struct sio_stream *sio_stream_listen(struct sio *sio, const char *ipv4, uint16_t port, sio_stream_callback_t callback, void *arg)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) 
        return NULL;
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);
    int on = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ipv4);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        close(sock);
        return NULL;
    }
    listen(sock, 1024);

    struct sio_stream *stream = _sio_stream_new(sock, SIO_STREAM_LISTEN, callback, arg);
    stream->sfd = sio_add(sio, sock, _sio_accept_callback, stream);
    if (!stream->sfd) {
        sio_stream_close(sio, stream);
        return NULL;
    }
    sio_watch_read(sio, stream->sfd);
    return stream;
}

struct sio_stream *sio_stream_connect(struct sio *sio, const char *ipv4, uint16_t port, sio_stream_callback_t callback, void *arg)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        return NULL;
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);
    int nodelay = 1;
    setsockopt(sock, SOL_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ipv4);
    int ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1 && errno != EINPROGRESS) {
        close(sock);
        return NULL;
    }
    struct sio_stream *stream = _sio_stream_new(sock, ret == 0 ? SIO_STREAM_NORMAL : SIO_STREAM_CONNECT, callback, arg);
    stream->sfd = sio_add(sio, sock, ret == 0 ? _sio_stream_callback : _sio_connect_callback, stream);
    if (!stream->sfd) {
        sio_stream_close(sio, stream);
        return NULL;
    }
    if (ret == 0) 
        sio_watch_read(sio, stream->sfd);
    else
        sio_watch_write(sio, stream->sfd);
    return stream;
}

void sio_stream_detach(struct sio *sio, struct sio_stream *stream)
{
    sio_del(sio, stream->sfd);
    stream->sfd = NULL;
}

int sio_stream_attach(struct sio *sio, struct sio_stream *stream)
{
    switch (stream->type) {
    case SIO_STREAM_LISTEN:
        stream->sfd = sio_add(sio, stream->sock, _sio_accept_callback, stream);
        if (!stream->sfd)
            return -1;
        sio_watch_read(sio, stream->sfd);
        break;
    case SIO_STREAM_CONNECT:
        stream->sfd = sio_add(sio, stream->sock, _sio_connect_callback, stream);
        if (!stream->sfd)
            return -1;
        sio_watch_write(sio, stream->sfd);
        break;
    case SIO_STREAM_NORMAL:
        stream->sfd = sio_add(sio, stream->sock, _sio_stream_callback, stream);
        if (!stream->sfd)
            return -1;
        sio_watch_read(sio, stream->sfd);
        if (sio_buffer_length(stream->outbuf))
            sio_watch_write(sio, stream->sfd);
        break;
    default:
        return -1;
    }
    return 0;
}

int sio_stream_write(struct sio *sio, struct sio_stream *stream, const char *data, uint64_t size)
{
    uint64_t len = sio_buffer_length(stream->outbuf);
    if (len || stream->type == SIO_STREAM_CONNECT) {
        sio_buffer_append(stream->outbuf, data, size);
        return 0;
    }
    int64_t bytes = write(stream->sock, data, size);
    if (bytes == -1) {
        if (errno != EINTR && errno != EAGAIN)
            return -1;
        bytes = 0;
    } else if (bytes == size) {
        return 0;
    }
    sio_buffer_append(stream->outbuf, data + bytes, size - bytes);
    sio_watch_write(sio, stream->sfd);
    return 0;
}

struct sio_buffer *sio_stream_buffer(struct sio_stream *stream)
{
    return stream->inbuf;
}

void sio_stream_set(struct sio *sio, struct sio_stream *stream, sio_stream_callback_t callback, void *arg)
{
    stream->user_callback = callback;
    stream->user_arg = arg;
}

uint64_t sio_stream_pending(struct sio_stream *stream)
{
    return sio_buffer_length(stream->outbuf);
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
