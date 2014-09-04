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

/* rpc client的应答回调 */
typedef void (*sio_rpc_upstream_callback_t)(struct sio_rpc_client *client, char is_timeout, const char *response, uint32_t size, void *arg);
/* rpc server的请求回调 */
typedef void (*sio_rpc_dstream_callback_t)(struct sio_rpc_server *server, struct sio_rpc_response *resp, void *arg);

/* rpc框架, 需绑定到一个sio上 */
struct sio_rpc {
    struct sio *sio; /* 事件驱动 */
    uint64_t max_pending; /* 限制读写缓冲区最大容量 */
};

/* rpc请求 */
struct sio_rpc_request {
    uint64_t id; /* 请求在某个连接上的唯一ID */
    uint32_t type; /* 请求的类型 */
    char *body; /* 请求内容 */
    uint32_t bodylen; /* 请求长度 */
    uint32_t retry_count; /* 当前重试的次数 */
    uint32_t retry_times; /* 总共重试次数限制 */
    uint64_t timeout;   /* 每次重试的超时 */
    struct sio_timer timer; /* 请求超时定时器 */
    struct sio_rpc_client *client; /* 请求所属客户端 */
    struct sio_rpc_upstream *upstream; /* 请求所属连接 */
    sio_rpc_upstream_callback_t cb; /* 应答回调 */
    void *arg; /* 应答回调参数 */
};

/* 代表client中的一个上游连接 */
struct sio_rpc_upstream {
    char *ip; /* 连接ip */
    uint16_t port; /* 连接port */
    struct sio_timer timer; /* 定时检查连接状况 */
    struct sio_stream *stream; /* TCP连接 */
    uint64_t req_id; /* 自增请求ID */
    struct shash *req_status; /* 记录这条TCP连接上所有等待应答的请求 */
    struct sio_rpc_client *client; /* 连接所属client */
    time_t last_conn_time; /* 上次重连时间 */
    time_t conn_delay; /* 重连间隔, 1~256秒, 连接5秒内断开则会*2, 否则重置 */
};

/* rpc客户端 */
struct sio_rpc_client {
    struct sio_rpc *rpc; /* rpc框架 */
    uint32_t rr_stream; /* 当无活连接时, 轮转重试各个upstream */
    uint32_t upstream_count; /* upstream数组长度 */
    struct sio_rpc_upstream **upstreams; /* upstream数组, 每个元素地址各不相同 */
    struct shash *req_record; /* 记录所有请求 */
};

/* rpc应答 */
struct sio_rpc_response {
	uint64_t conn_id; /* 来自哪个dstream */
	struct sio_rpc_server *server; /* 所属server */
	struct shead req_head; /* 请求的header */
	char *request; /* 请求的body */
};

/* rpc下游,server的一个连接 */
struct sio_rpc_dstream {
    uint64_t id; /* 下游的id */
    struct sio_timer timer; /* 定时检查连接状况 */
    struct sio_stream *stream; /* TCP连接 */
    struct sio_rpc_server *server; /* 所属server */
};

/* 在server中注册的rpc方法 */
struct sio_rpc_method {
    sio_rpc_dstream_callback_t cb; /* 本地rpc方法 */
    void *arg; /* rpc参数 */
};

/* rpc服务端 */
struct sio_rpc_server {
    struct sio_rpc *rpc; /* rpc框架 */
    uint64_t conn_id; /* 自增连接ID */
    struct sio_stream *stream; /* 监听套接字 */
    struct shash *dstreams; /* 所有下游downstream */
    struct shash *methods; /* 注册的rpc方法 */
};

/**
 * @brief 创建rpc框架
 *
 * @param [in] sio   : struct sio*
 * @param [in] max_pending   : uint64_t 字节, 限制读写缓冲区最大容量
 * @return  struct sio_rpc* 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/09/03 11:27:41
**/
struct sio_rpc *sio_rpc_new(struct sio *sio, uint64_t max_pending);
/**
 * @brief 释放rpc框架
 *
 * @param [in] rpc   : struct sio_rpc*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/31 13:21:26
**/
void sio_rpc_free(struct sio_rpc *rpc);
/**
 * @brief 创建rpc客户端
 *
 * @param [in] rpc   : struct sio_rpc*
 * @return  struct sio_rpc_client* 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/31 13:21:37
**/
struct sio_rpc_client *sio_rpc_client_new(struct sio_rpc *rpc);
/**
 * @brief 释放rpc客户端
 *
 * @param [in] client   : struct sio_rpc_client*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/31 13:21:52
**/
void sio_rpc_client_free(struct sio_rpc_client *client);
/**
 * @brief 向客户端添加一个上游, 支持运行时动态添加
 *
 * @param [in] client   : struct sio_rpc_client*
 * @param [in] ip   : const char*
 * @param [in] port   : uint16_t
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/31 13:22:02
**/
void sio_rpc_add_upstream(struct sio_rpc_client *client, const char *ip, uint16_t port);
/**
 * @brief 从客户端移除一个上游, 支持运行时动态删除(不要在回调中删除)
 *
 * @param [in] client   : struct sio_rpc_client*
 * @param [in] ip   : const char*
 * @param [in] port   : uint16_t
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/31 13:22:17
**/
void sio_rpc_remove_upstream(struct sio_rpc_client *client, const char *ip, uint16_t port);
/**
 * @brief 发起RPC远程调用
 *
 * @param [in] client   : struct sio_rpc_client*
 * @param [in] type   : uint32_t 请求的类型
 * @param [in] timeout_ms   : uint64_t 请求超时时间, 总耗费时间最大timeout_ms *retry_times
 * @param [in] retry_times   : uint32_t 请求重试次数
 * @param [in] request   : const char* 请求体,可以为NULL,则只发送header
 * @param [in] size   : uint32_t 请求体长度, 如果requet为NULL, 则size必须为0
 * @param [in] cb   : sio_rpc_upstream_callback_t 应答处理回调
 * @param [in] arg   : void* 用户参数
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/31 13:22:47
**/
void sio_rpc_call(struct sio_rpc_client *client, uint32_t type, uint64_t timeout_ms, uint32_t retry_times,
        const char *request, uint32_t size, sio_rpc_upstream_callback_t cb, void *arg);
/**
 * @brief 创建RPC服务端
 *
 * @param [in] rpc   : struct sio_rpc*
 * @param [in] ip   : const char*
 * @param [in] port   : uint16_t
 * @return  struct sio_rpc_server* 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/31 13:24:28
**/
struct sio_rpc_server *sio_rpc_server_new(struct sio_rpc *rpc, const char *ip, uint16_t port);
/**
 * @brief 释放RPC服务端(释放前确保所有的response都finish, 否则会内存泄露)
 *
 * @param [in] server   : struct sio_rpc_server*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/31 13:24:42
**/
void sio_rpc_server_free(struct sio_rpc_server *server);
/**
 * @brief 注册一个RPC方法, 支持动态添加
 *
 * @param [in] server   : struct sio_rpc_server*
 * @param [in] type   : uint32_t
 * @param [in] cb   : sio_rpc_dstream_callback_t
 * @param [in] arg   : void*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/31 13:25:27
**/
void sio_rpc_server_add_method(struct sio_rpc_server *server, uint32_t type, sio_rpc_dstream_callback_t cb, void *arg);
/**
 * @brief 取消一个RPC方法, 支持动态删除(不要在回调中执行)
 *
 * @param [in] server   : struct sio_rpc_server*
 * @param [in] type   : uint32_t
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/31 13:25:52
**/
void sio_rpc_server_remove_method(struct sio_rpc_server *server, uint32_t type);
/**
 * @brief 结束一个RPC调用, 返回应答
 *
 * @param [in] resp   : struct sio_rpc_response* 
 * @param [in] body   : const char* 可以为NULL,只应答header
 * @param [in] len   : uint32_t 如果body为NULL, len必须为0
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/31 13:26:11
**/
void sio_rpc_finish(struct sio_rpc_response *resp, const char *body, uint32_t len);
/**
 * @brief 从sio_rpc_response中取出请求和长度
 *
 * @param [in] resp   : struct sio_rpc_response*
 * @param [in] len   : uint32_t* 可以为NULL
 * @return  char* 如果*len返回为0, 那么返回值是未定义的, 不要访问!
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/31 13:27:10
**/
char *sio_rpc_request(struct sio_rpc_response *resp, uint32_t *len);

#ifdef __cplusplus
}
#endif

#endif
