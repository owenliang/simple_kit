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
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include "shash.h"
#include "sdeque.h"
#include "sio.h"
#include "sio_stream.h"

static volatile char server_quit = 0;

/* 客户端TCP连接 */
struct sio_stream_conn {
    uint64_t id; /* 连接唯一ID */
    struct sio_stream *stream; /* TCP连接 */
    struct sio_stream_work_thread *thread; /* 所属的线程 */
};

/* 工作线程 */
struct sio_stream_work_thread {
    uint32_t index; /* 第几个线程 */
    pthread_t tid; /* 线程ID */

    struct sdeque *conn_queue; /* 连接队列 */
    pthread_mutex_t mutex; /* 连接队列锁 */

    struct sio *sio; /* 线程事件循环 */
    volatile uint32_t conn_count; /* 线程管理的连接数 */
    struct shash *conn_hash; /* 连接哈希表 id->sio_stream_conn */
};

/* 多线程TCP服务端 */
struct sio_stream_multi_server {
    uint64_t conn_id; /* 连接ID生成器 */
    struct sio *main_sio; /* 主线程事件循环 */
    struct sio_stream *acceptor; /* 监听套接字 */
    uint32_t work_thread_count; /* 工作线程数量 */
    struct sio_stream_work_thread *work_threads; /* 工作线程池 */
};

struct sio_stream_work_thread *sio_stream_dispatch_conn(struct sio_stream_multi_server *server)
{
	uint32_t min_count = ~0;
	uint32_t min_index = 0;

	uint32_t i;
	for (i = 0; i < server->work_thread_count; ++i) {
		uint32_t count = server->work_threads[i].conn_count;
		if (count < min_count) {
			min_count = count;
			min_index = i;
		}
	}
	return server->work_threads + min_index;
}

static void sio_stream_add_conn_to_thread(struct sio_stream_work_thread *thread, struct sio_stream_conn *conn)
{
	pthread_mutex_lock(&thread->mutex);
	sdeque_push_back(thread->conn_queue, conn);
	pthread_mutex_unlock(&thread->mutex);
	sio_wakeup(thread->sio);
}

static void sio_stream_accept_callback(struct sio *sio, struct sio_stream *stream, enum sio_stream_event event, void *arg)
{
	struct sio_stream_multi_server *server = arg;

	if (event == SIO_STREAM_ACCEPT) {
		sio_stream_detach(sio, stream);

		struct sio_stream_conn *conn = malloc(sizeof(*conn));
		conn->id = server->conn_id++;
		conn->stream = stream;
		conn->thread = sio_stream_dispatch_conn(server);
		sio_stream_add_conn_to_thread(conn->thread, conn);
	}
}

static void sio_stream_close_conn(struct sio_stream_conn *conn, char attached);

static void sio_stream_handle_data(struct sio_stream_conn *conn)
{
    struct sio_buffer *input_buffer = sio_stream_buffer(conn->stream);

    char *data_ptr;
    uint64_t data_len;
    sio_buffer_data(input_buffer, &data_ptr, &data_len);

    if (sio_stream_write(conn->thread->sio, conn->stream, data_ptr, data_len) == -1)
        sio_stream_close_conn(conn, 1);
    else
        sio_buffer_erase(input_buffer, data_len);
}

static void sio_stream_conn_callback(struct sio *sio, struct sio_stream *stream, enum sio_stream_event event, void *arg)
{
    switch (event) {
    case SIO_STREAM_DATA:
        sio_stream_handle_data(arg);
        break;
    case SIO_STREAM_ERROR:
    case SIO_STREAM_CLOSE:
        sio_stream_close_conn(arg, 1);
        break;
    default:
        assert(0);
    }
}

static void sio_stream_fetch_conn(struct sio_stream_work_thread *thread)
{
    struct sdeque *conn_queue = NULL;

	pthread_mutex_lock(&thread->mutex);
    if (sdeque_size(thread->conn_queue)) {
        conn_queue = thread->conn_queue;
        assert(thread->conn_queue = sdeque_new());
    }
	pthread_mutex_unlock(&thread->mutex);

    if (!conn_queue)
        return;
	
    struct sio_stream_conn *conn = NULL;

	while (sdeque_front(conn_queue, (void **)&conn) != -1) {
		sdeque_pop_front(conn_queue);
		
        if (sio_stream_attach(thread->sio, conn->stream) == -1) {
		    sio_stream_close_conn(conn, 0);
			return;
		}
		sio_stream_set(thread->sio, conn->stream, sio_stream_conn_callback, conn);
		assert(shash_insert(thread->conn_hash, (const char *)&conn->id, sizeof(conn->id), conn) == 0);
		thread->conn_count++;

	    char ipv4[32];
	    uint16_t port;
	    assert(sio_stream_peer_address(conn->stream, ipv4, sizeof(ipv4), &port) == 0);
	    printf("[Thread-%u]sio_stream_fetch_conn=id:%lu ipv4=%s port=%u conn_count=%u\n",
	            thread->index, conn->id, ipv4, port, thread->conn_count);
    }

    sdeque_free(conn_queue);
}

static void *sio_steram_work_thread_main(void *arg)
{
	struct sio_stream_work_thread *thread = arg;

	while (!server_quit) {
		sio_stream_fetch_conn(thread);
		sio_run(thread->sio, 1000);
	}
    return NULL;
}

static void sio_stream_close_conn(struct sio_stream_conn *conn, char attached)
{
    printf("sio_stream_close_conn=id:%lu\n", conn->id);
    sio_stream_close(conn->thread->sio, conn->stream);
    if (attached) {
        assert(shash_erase(conn->thread->conn_hash, (const char *)&conn->id, sizeof(conn->id)) == 0);
        conn->thread->conn_count--;
    }
    free(conn);
}

static void sio_stream_work_thread_free(struct sio_stream_work_thread *thread)
{
    pthread_join(thread->tid, NULL);
    pthread_mutex_destroy(&thread->mutex);

    struct sio_stream_conn *detached_conn = NULL;
    while (sdeque_front(thread->conn_queue, (void **)&detached_conn) != -1) {
        sdeque_pop_front(thread->conn_queue);
        sio_stream_close_conn(detached_conn, 0);
    }
    sdeque_free(thread->conn_queue);

    shash_begin_iterate(thread->conn_hash);

    const char *key;
    uint32_t key_len;
    void *value;
    while (shash_iterate(thread->conn_hash, &key, &key_len, &value) != -1) {
        assert(key_len == sizeof(uint64_t));
        uint64_t id = *(const uint64_t *)key;
        struct sio_stream_conn *conn = value;
        assert(id == conn->id);
        sio_stream_close_conn(conn, 1);
    }
    shash_free(thread->conn_hash);
    sio_free(thread->sio);
}

static void sio_stream_work_thread_init(struct sio_stream_work_thread *thread, uint32_t index)
{
    thread->index = index;
    assert(thread->sio = sio_new());
    thread->conn_count = 0;
    assert(thread->conn_hash = shash_new());
    assert(thread->conn_queue = sdeque_new());
    pthread_mutex_init(&thread->mutex, NULL);
    pthread_create(&thread->tid, NULL, sio_steram_work_thread_main, thread);
}

static void sio_stream_multi_server_init(struct sio_stream_multi_server *server, uint32_t work_thread_count)
{
    assert(server->main_sio = sio_new());
    assert(server->acceptor = sio_stream_listen(server->main_sio, "0.0.0.0", 8989, sio_stream_accept_callback, server));
    assert(server->work_thread_count = work_thread_count);
    server->conn_id = 0;
    server->work_threads = calloc(work_thread_count, sizeof(*server->work_threads));
    uint32_t i;
    for (i = 0; i < work_thread_count; ++i)
        sio_stream_work_thread_init(server->work_threads + i, i);
}

static void sio_stream_multi_server_free(struct sio_stream_multi_server *server)
{
    sio_stream_close(server->main_sio, server->acceptor);
    sio_free(server->main_sio);
    uint32_t i;
    for (i = 0; i < server->work_thread_count; ++i)
        sio_stream_work_thread_free(server->work_threads + i);
    free(server->work_threads);
}

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

    struct sio_stream_multi_server server;

    sio_stream_multi_server_init(&server, 8);
    while (!server_quit)
    	sio_run(server.main_sio, 1000);
    sio_stream_multi_server_free(&server);

    printf("sio_stream_multi_server=quit\n");

    return 0;
}
