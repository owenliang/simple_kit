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
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include "sio.h"    /* 基础事件循环 */
#include "sio_stream.h" /* TCP框架 */
#include "shash.h"  /* 哈希表 */

struct sio_stream_client;

struct sio_stream_conn {
    uint64_t index; /* 第几个连接 */
    struct sio_stream_client *client; /* client */
    struct sio_stream *stream;  /* TCP连接 */
    struct sio_timer timer;  /* 连接检查定时器 */
};

struct sio_stream_client {
    struct sio *sio;
    uint64_t conn_count;
    struct sio_stream_conn *conns;
};

static const char *ip = "127.0.0.1";
static uint16_t port = 8989;

static void sio_stream_finish_conn(struct sio_stream_conn *conn, char end);

static void sio_stream_conn_handle_data(struct sio_stream_conn *conn)
{
    struct sio_buffer *input_buffer = sio_stream_buffer(conn->stream);

    uint64_t data_len;
    char *data_ptr = sio_buffer_data(input_buffer, &data_len);

    if (sio_stream_write(conn->client->sio, conn->stream, data_ptr, data_len) == -1)
        sio_stream_finish_conn(conn, 0);
    else
        sio_buffer_erase(input_buffer, data_len);
}

static void sio_stream_conn_callback(struct sio *sio, struct sio_stream *stream, enum sio_stream_event event, void *arg)
{
    struct sio_stream_conn *conn = arg;
    switch (event) {
    case SIO_STREAM_DATA:
        sio_stream_conn_handle_data(conn);
        break;
    case SIO_STREAM_ERROR:
    case SIO_STREAM_CLOSE:
        sio_stream_finish_conn(conn, 0);
        break;
    default:
        assert(0);
    }
}

static void sio_stream_conn_burst_ping(struct sio *sio, struct sio_stream_conn *conn);

static void sio_timer_conn_callback(struct sio *sio, struct sio_timer *timer, void *arg)
{
    struct sio_stream_conn *conn = arg;
    if (!conn->stream)
        sio_stream_conn_burst_ping(sio, conn);
    sio_start_timer(sio, timer, 1000, sio_timer_conn_callback, conn);
}

static void sio_stream_finish_conn(struct sio_stream_conn *conn, char end)
{
    if (conn->stream) {
        printf("sio_stream_finish_conn=index:%lu\n", conn->index);
        sio_stream_close(conn->client->sio, conn->stream);
        conn->stream = NULL;
    }
    if (end)
        sio_stop_timer(conn->client->sio, &conn->timer);
}

static void sio_stream_conn_burst_ping(struct sio *sio, struct sio_stream_conn *conn)
{
    if (!conn->stream)
        conn->stream = sio_stream_connect(sio, ip, port, sio_stream_conn_callback, conn);

    if (conn->stream) {
        if (sio_stream_write(sio, conn->stream, "ping", 4) == -1)
            sio_stream_finish_conn(conn, 0);
    }
}

static void sio_stream_client_init(struct sio_stream_client *client, uint64_t conn_count)
{
    assert(client->sio = sio_new());
    client->conn_count = conn_count;
    client->conns = calloc(conn_count, sizeof(*client->conns));
    int i;
    for (i = 0; i < conn_count; ++i) {
        struct sio_stream_conn *conn = client->conns + i;
        conn->index = i;
        conn->client = client;
        sio_start_timer(client->sio, &conn->timer, 1000, sio_timer_conn_callback, conn);
        sio_stream_conn_burst_ping(client->sio, conn);
    }
}

static void sio_stream_client_free(struct sio_stream_client *client)
{
    int i;
    for (i = 0; i < client->conn_count; ++i) {
        sio_stream_finish_conn(client->conns + i, 1);
    }
    sio_free(client->sio);
    free(client->conns);
}

static char client_quit = 0;

static void sio_stream_quit_handler(int signo)
{
    client_quit = 1;
}

static void sio_stream_client_signal()
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = sio_stream_quit_handler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
}

int main(int argc, char **argv)
{
    sio_stream_client_signal();

    struct sio_stream_client client;
    sio_stream_client_init(&client, 1200);

    while (!client_quit)
        sio_run(client.sio, 1000);

    sio_stream_client_free(&client);
    printf("sio_stream_client=quit\n");
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
