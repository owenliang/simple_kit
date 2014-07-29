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

/* TCP服务端 */
struct sio_stream_server {
    struct sio *sio; /* 事件管理器 */
    struct sio_stream *acceptor;    /* 监听套接字 */
    uint64_t conn_id; /* 连接唯一ID标识 */
    struct shash *conn_hash; /* 记录客户端连接 */
};

/* TCP连接 */
struct sio_stream_conn {
	uint64_t id;
	struct sio_stream *stream;
	struct sio_stream_server *server;
};

static void sio_stream_conn_callback(struct sio *sio, struct sio_stream *stream, enum sio_stream_event event, void *arg);

static void sio_stream_handle_accept(struct sio *sio, struct sio_stream *stream, struct sio_stream_server *server)
{
	struct sio_stream_conn *conn = malloc(sizeof(*conn));
	conn->id = server->conn_id++;
	conn->stream = stream;
	conn->server = server;
	assert(shash_insert(server->conn_hash,(const char *)&conn->id, sizeof(conn->id), conn) == 0);
	sio_stream_set(sio, stream, sio_stream_conn_callback, conn);
	printf("sio_stream_handle_accept=id:%lu\n", conn->id);
}

static void sio_stream_close_conn(struct sio_stream_conn *conn)
{
	printf("sio_stream_close_conn=id:%lu\n", conn->id);
	sio_stream_close(conn->server->sio, conn->stream);
	assert(shash_erase(conn->server->conn_hash, (const char *)&conn->id, sizeof(conn->id)) == 0);
	free(conn);
}

static void sio_stream_handle_close(struct sio_stream_conn *conn)
{
	sio_stream_close_conn(conn);
}

static void sio_stream_handle_data(struct sio_stream_conn *conn)
{
	struct sio_buffer *input_buffer = sio_stream_buffer(conn->stream);

	char *data_ptr;
	uint64_t data_len;
	sio_buffer_data(input_buffer, &data_ptr, &data_len);

	if (sio_stream_write(conn->server->sio, conn->stream, data_ptr, data_len) == -1)
		sio_stream_close_conn(conn);
	else
		sio_buffer_erase(input_buffer, data_len);
}

static void sio_stream_conn_callback(struct sio *sio, struct sio_stream *stream, enum sio_stream_event event, void *arg)
{
	switch (event) {
	case SIO_STREAM_ACCEPT:
		sio_stream_handle_accept(sio, stream, arg);
		break;
	case SIO_STREAM_DATA:
		sio_stream_handle_data(arg);
		break;
	case SIO_STREAM_ERROR:
	case SIO_STREAM_CLOSE:
		sio_stream_handle_close(arg);
		break;
	default:
		assert(0);
	}
}

static void sio_stream_server_init(struct sio_stream_server *server)
{
    assert(server->sio = sio_new());
    assert(server->acceptor = sio_stream_listen(server->sio, "0.0.0.0", 8989, sio_stream_conn_callback, server));
    server->conn_id = 0;
    assert(server->conn_hash = shash_new());
}

static void sio_stream_server_free(struct sio_stream_server *server)
{
	shash_begin_iterate(server->conn_hash);

	const char *key;
	uint32_t key_len;
	void *value;
	while (shash_iterate(server->conn_hash, &key, &key_len, &value) != -1) {
		assert(key_len == sizeof(uint64_t));
		uint64_t id = *(const uint64_t *)key;
		struct sio_stream_conn *conn = value;
		assert(id == conn->id);
		sio_stream_close_conn(conn);
	}
	shash_free(server->conn_hash);
	sio_stream_close(server->sio, server->acceptor);
	sio_free(server->sio);
}

static char server_quit = 0;

static void sio_stream_quit_handler(int signo)
{
	server_quit = 1;
}

static void sio_stream_server_signal()
{
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_handler = sio_stream_quit_handler;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
}

int main(int argc, char **argv)
{
	sio_stream_server_signal();

	struct sio_stream_server server;

	sio_stream_server_init(&server);

	while (!server_quit) {
		sio_run(server.sio, 1000);
	}

	sio_stream_server_free(&server);
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
