#ifndef SIMPLE_IO_SIO_RPC_H
#define SIMPLE_IO_SIO_RPC_H

#include <stdint.h>
#include <time.h>
#include "shead.h"

#ifdef __cplusplus
extern "C" {
#endif

struct shash;
struct sio_stream;
struct sio_rpc_upstream;
struct sio_rpc_client;
struct sio_rpc_dstream;
struct sio_rpc_server;
struct sio_rpc_response;

typedef void (*sio_rpc_upstream_callback_t)(struct sio_rpc_client *client, char is_timeout, const char *response, uint32_t size, void *arg);
typedef void (*sio_rpc_dstream_callback_t)(struct sio_rpc_server *server, struct sio_rpc_response *resp);

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
    time_t last_conn_time;
    time_t conn_delay; /* 1~256 */
};

struct sio_rpc_client {
    struct sio_rpc *rpc;
    uint32_t rr_stream;
    uint32_t upstream_count;
    struct sio_rpc_upstream **upstreams;
    struct shash *req_record;
};

struct sio_rpc_response {
	uint64_t conn_id;
	struct sio_rpc_server *server;
	struct shead req_head;
	char *request;
};

struct sio_rpc_dstream {
    uint64_t id;
    struct sio_stream *stream;
    struct sio_rpc_server *server;
};

struct sio_rpc_method {
    sio_rpc_dstream_callback_t cb;
    void *arg;
};

struct sio_rpc_server {
    struct sio_rpc *rpc;
    uint64_t conn_id;
    struct sio_stream *stream;
    struct shash *dstreams;
    struct shash *methods;
};

struct sio_rpc *sio_rpc_new(struct sio *sio);
void sio_rpc_free(struct sio_rpc *rpc);

struct sio_rpc_client *sio_rpc_client_new(struct sio_rpc *rpc);
void sio_rpc_client_free(struct sio_rpc_client *client);
void sio_rpc_add_upstream(struct sio_rpc_client *client, const char *ip, uint16_t port);
void sio_rpc_remove_upstream(struct sio_rpc_client *client, const char *ip, uint16_t port);
void sio_rpc_call(struct sio_rpc_client *client, uint32_t type, uint64_t timeout_ms, uint32_t retry_times,
        const char *request, uint32_t size, sio_rpc_upstream_callback_t cb, void *arg);

struct sio_rpc_server *sio_rpc_server_new(struct sio_rpc *rpc, const char *ip, uint16_t port);
void sio_rpc_server_free(struct sio_rpc_server *server);
void sio_rpc_server_add_method(struct sio_rpc_server *server, uint32_t type, sio_rpc_dstream_callback_t cb, void *arg);
void sio_rpc_server_remove_method(struct sio_rpc_server *server, uint32_t type);
void sio_rpc_finish(struct sio_rpc_response *resp, const char *body, uint32_t len);
const char *sio_rpc_request(struct sio_rpc_response *resp, uint32_t *len);

#ifdef __cplusplus
}
#endif

#endif
