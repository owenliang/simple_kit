#include <assert.h>
#include "sio.h"
#include "sio_rpc.h"

int main(int argc, char **argv)
{
    struct sio *sio = sio_new();
    assert(sio);

    struct sio_rpc *rpc = sio_rpc_new(sio);
    assert(rpc);

    struct sio_rpc_client *client = sio_rpc_client_new(rpc);
    sio_rpc_add_upstream(client, "127.0.0.1", 8410);
    sio_rpc_remove_upstream(client, "127.0.0.1", 8410);
    sio_rpc_client_free(client);

    sio_rpc_free(rpc);
    sio_free(sio);

    return 0;
}
