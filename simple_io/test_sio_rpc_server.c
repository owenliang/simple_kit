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
#include <signal.h>
#include <assert.h>
#include "sio.h"
#include "sio_rpc.h"
#include "shash.h"

/* 
 * 正在异步处理中的请求, 在关闭server前需要结束这些请求, 避免内存泄露.
 *
 * key: sio_rpc_response*, value: sio_timer 
 * 
 * */
static struct shash *pending_response = NULL;

static void sio_rpc_finish_response(struct sio *sio, struct sio_timer *timer, void *arg)
{
    struct sio_rpc_response *resp = arg;
    assert(shash_erase(pending_response, (const char *)&resp, sizeof(resp)) == 0);
    free(timer);

    uint32_t req_len;
    const char *req = sio_rpc_request(resp, &req_len);

    printf("rpc req:%.*s", req_len, req);
    sio_rpc_finish(resp, "pong\n", 5);
    printf("rpc resp:pong\n");
}

static void sio_rpc_finish_response_onexit(struct sio *sio)
{
    /* 结束正在处理中的请求 */
    shash_begin_iterate(pending_response);
    const char *key;
    void *value;
    while (shash_iterate(pending_response, &key, NULL, &value) != -1) {
        struct sio_rpc_response *resp = *(struct sio_rpc_response * const *)key;
        struct sio_timer *timer = value;
        sio_rpc_finish(resp, NULL, 0);
        sio_stop_timer(sio, timer);
        free(timer);
    }
    shash_end_iterate(pending_response);
}

static void sio_rpc_dstream_callback(struct sio_rpc_server *server, struct sio_rpc_response *resp, void *arg)
{
    struct sio *sio = arg;

    /* 
     * sio_rpc_response可以异步处理, 由用户通过sio_rpc_finish结束处理.
     * 这里通过timer模拟异步请求处理, 花费100ms.
     */
    struct sio_timer *timer = malloc(sizeof(*timer));
    sio_start_timer(sio, timer, 100, sio_rpc_finish_response, resp);
    assert(shash_insert(pending_response, (const char *)&resp, sizeof(resp), timer) == 0);
}

static char server_quit = 0;

static void sio_rpc_quit_handler(int signo)
{
    server_quit = 1;
}

static void sio_rpc_server_signal()
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = sio_rpc_quit_handler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
}

int main(int argc, char **argv)
{
    sio_rpc_server_signal();

    assert(pending_response = shash_new());

    struct sio *sio = sio_new();
    assert(sio);

    struct sio_rpc *rpc = sio_rpc_new(sio, 10 * 1024 * 1024/*10MB read/write buffer limit*/);
    assert(rpc);

    struct sio_rpc_server *server = sio_rpc_server_new(rpc, "0.0.0.0", 8989);
    assert(server);

    sio_rpc_server_add_method(server, 0, sio_rpc_dstream_callback, sio);

    while (!server_quit) {
        sio_run(sio);
    }
    
    sio_rpc_finish_response_onexit(sio);

    sio_rpc_server_free(server);

    sio_rpc_free(rpc);
    sio_free(sio);

    shash_free(pending_response);

    return 0;
}
