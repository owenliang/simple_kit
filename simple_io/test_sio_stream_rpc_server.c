#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include "sio_stream_rpc.h"

static void run_task_per_sec(void *arg)
{
    printf("%s\n", __FUNCTION__);
}

static void run_timer(uint64_t id, void *arg)
{
    printf("timer id: %lu\n", id);
    sio_stream_rpc_add_timer(run_timer, NULL, 500);
}

static void handle_rpc_type5(struct sio_stream_rpc_server_cloure *cloure, void *arg)
{
    sio_stream_rpc_terminate_cloure(cloure); /* Ã»ÓÐÓ¦´ð */
}

int main(int argc, char **argv)
{
    sio_stream_rpc_init(8, 8);

    struct sio_stream_rpc_service *service = sio_stream_rpc_add_service("0.0.0.0", 8989);
    assert(service);

    assert(sio_stream_rpc_register_protocol(service, 5, handle_rpc_type5, NULL) == 0);

    sio_stream_rpc_run();

    sio_stream_rpc_add_timer(run_timer, NULL, 500);

    while (1) {
        sio_stream_rpc_run_task(run_task_per_sec, NULL);
        sleep(1);
    }
    return 0;
}
