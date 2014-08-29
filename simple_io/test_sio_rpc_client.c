#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include "sio.h"
#include "sio_rpc.h"

static void sio_rpc_upstream_callback(struct sio_rpc_client *client, char is_timeout, const char *response, uint32_t size, void *arg)
{
    printf("is_timeout=%d\n", is_timeout);
}

static void sio_rpc_ping_timer_callback(struct sio *sio, struct sio_timer *timer, void *arg)
{
    printf("ping\n");
    struct sio_rpc_client *client = arg;
    /* 请求类型0, 请求超时100ms, 重试3次, 总共最多花费100ms * 3 = 300ms */
    sio_rpc_call(client, 0, 100, 3, "ping\n", 5, sio_rpc_upstream_callback,  NULL);
    sio_start_timer(sio, timer, 1000, sio_rpc_ping_timer_callback, client);
}

static char client_quit = 0;

static void sio_rpc_quit_handler(int signo)
{
    client_quit = 1;
}

static void sio_rpc_client_signal()
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = sio_rpc_quit_handler;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
}

int main(int argc, char **argv)
{
    sio_rpc_client_signal();

    struct sio *sio = sio_new();
    assert(sio);

    struct sio_rpc *rpc = sio_rpc_new(sio);
    assert(rpc);

    struct sio_rpc_client *client = sio_rpc_client_new(rpc);
    sio_rpc_add_upstream(client, "127.0.0.1", 8989);

    /* 每秒ping发起一次Ping rpc */
    struct sio_timer ping_timer;
    sio_start_timer(sio, &ping_timer, 1000, sio_rpc_ping_timer_callback, client);

    while (!client_quit) {
        sio_run(sio, 100);
    }

    sio_stop_timer(sio, &ping_timer);

    /*
     * 可以不调用, client_free一样会回收所有连接和请求
     *
     sio_rpc_remove_upstream(client, "127.0.0.1", 8989); */
    sio_rpc_client_free(client);

    sio_rpc_free(rpc);
    sio_free(sio);

    return 0;
}
