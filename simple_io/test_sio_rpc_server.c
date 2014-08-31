#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include "sio.h"
#include "sio_rpc.h"

static void sio_rpc_dstream_callback(struct sio_rpc_server *server, struct sio_rpc_response *resp)
{
    uint32_t req_len;
    const char *req = sio_rpc_request(resp, &req_len);

    printf("rpc req:%.*s", req_len, req);
    sio_rpc_finish(resp, "pong\n", 5);
    printf("rpc resp:pong\n");
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

    struct sio *sio = sio_new();
    assert(sio);

    struct sio_rpc *rpc = sio_rpc_new(sio);
    assert(rpc);

    struct sio_rpc_server *server = sio_rpc_server_new(rpc, "0.0.0.0", 8989);
    assert(server);

    sio_rpc_server_add_method(server, 0, sio_rpc_dstream_callback, NULL);

    while (!server_quit) {
        sio_run(sio, 100);
    }

    sio_rpc_server_free(server);

    sio_rpc_free(rpc);
    sio_free(sio);

    return 0;
}
