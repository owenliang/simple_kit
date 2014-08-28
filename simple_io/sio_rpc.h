#ifndef SIMPLE_IO_SIO_RPC_H
#define SIMPLE_IO_SIO_RPC_H

#include <stdint.h>
#include "shead.h"

#ifdef __cplusplus
extern "C" {
#endif

struct shash;
struct sio_stream;
struct sio_rpc_upstream;
struct sio_rpc_client;

typedef void (*sio_rpc_upstream_callback_t)(struct sio_rpc_client *client, char is_timeout, const char *response, uint32_t size);

struct sio_rpc {
    struct sio *sio;
};

struct sio_rpc_request {
    uint64_t id;
    uint32_t type;
    char *body;
    uint32_t bodylen;
    uint32_t retry_count;
    uint32_t retry_times;
    uint64_t timeout;
    struct sio_timer timer;
    struct sio_rpc_client *client;
    struct sio_rpc_upstream *upstream;
    sio_rpc_upstream_callback_t cb;
    void *arg;
};

struct sio_rpc_upstream {
    char *ip;
    uint16_t port;
    struct sio_timer timer;
    struct sio_stream *stream;
    uint64_t req_id;
    struct shash *req_status;
    struct sio_rpc_client *client;
};

struct sio_rpc_client {
    struct sio_rpc *rpc;
    uint32_t rr_stream;
    uint32_t upstream_count;
    struct sio_rpc_upstream **upstreams;
};

#ifdef __cplusplus
}
#endif

#endif
