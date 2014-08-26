#ifndef SIMPLE_IO_SIO_STREAM_RPC_H
#define SIMPLE_IO_SIO_STREAM_RPC_H

#include <stdint.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sdeque;
struct shash;

struct sio_stream_rpc_server_cloure;

/* 异步任务回调 */
typedef void (*sio_stream_rpc_task_callback_t)(void *arg);
/* 定时器回调 */
typedef void (*sio_stream_rpc_timer_callback_t)(uint64_t id, void *arg);
/* 服务回调, cloure是请求上下文, 由用户通过调用结束cloure */
typedef void (*sio_stream_rpc_service_callback_t)(struct sio_stream_rpc_server_cloure *cloure, void *arg);

/* 异步任务 */
struct sio_stream_rpc_task {
    sio_stream_rpc_task_callback_t cb;
    void *arg;
};

/* 工作线程, 执行异步任务 */
struct sio_stream_rpc_work_thread {
    struct sdeque *task_queue;
    volatile uint64_t task_count;

    pthread_t tid;
    pthread_cond_t cond;
    pthread_mutex_t mutex;
};

/* 定时器任务 */
struct sio_stream_rpc_timer_task {
    uint64_t id;
    uint64_t expire_time;
    sio_stream_rpc_timer_callback_t cb;
    void *arg;
};

/* 定时器管理线程 */
struct sio_stream_rpc_timer_thread {
    uint64_t timer_id;
    struct slist *time_map; /* key: 绝对过期时间 value: 嵌套的哈希表(key为timer_id, value为rpc_timer_task组成的slist) */
    struct slist *id_map;   /* key: timer_id, value: rpc_timer_task  */

    pthread_t tid;
    pthread_mutex_t mutex;
};

struct sio_stream_rpc_io_thread;

/* 对外发起的连接 */
struct sio_stream_rpc_client_conn {
    uint64_t conn_id; /* 连接唯一ID */
    uint64_t request_id; /* 请求唯一ID生成器 */
    struct sio_stream_rpc_io_thread *io_thread; /* 所属IO线程 */
};

struct sio_stream_rpc_service;
struct sio_stream_rpc_service_item;

/* 接收的外部连接 */
struct sio_stream_rpc_server_conn {
    uint64_t conn_id;
    struct sio_stream *stream;
    struct sio_stream_rpc_io_thread *io_thread; /* 所属IO线程 */
    struct sio_stream_rpc_service *service;
};

/* 异步处理的请求 */
struct sio_stream_rpc_server_cloure {
    struct sio_stream_rpc_service_item *item; /* 请求的服务单元(用户的回调) */

    struct sio_stream_rpc_io_thread *io_thread; /* 请求所属线程 */
    uint64_t conn_id; /* 请求所属连接ID */

    /* 用户只能访问这两个字段 */
    struct shead req_head; /* 请求头 */
    char *req_body; /* 请求内容 */

    char no_resp; /* 无应答, 用户可以指定 */
    char *resp_packet; /* 应答包(头+体) */
    uint64_t resp_len; /* 应答包长度 */
};

/* 异步处理的应答 */
struct sio_stream_rpc_client_cloure {
};


/* 注册的服务端协议 */
struct sio_stream_rpc_service_item {
    uint32_t type; /* 消息类型 */
    sio_stream_rpc_service_callback_t cb; /* 用户回调 */
    void *arg; /* 用户参数 */
};

/* 监听在某端口, 提供一系列服务 */
struct sio_stream_rpc_service {
    struct sio_stream *sfd; /* 监听套接字 */
    struct sio_stream_rpc_io_thread *io_thread; /* 属于哪个io线程 */
    struct shash *register_service; /* 注册了哪些协议号(shead中的type) */
    uint32_t dispatch_index; /* 轮转分发连接到io线程 */
};

/* 网络io线程 */
struct sio_stream_rpc_io_thread {
    uint32_t index; /* 线程下标 */
    struct sio_stream_rpc *rpc; /* rpc框架 */

    struct sio *sio; /* 事件循环 */
    uint64_t conn_id; /* 线程唯一连接ID生成器 */
    struct shash *service_hash; /* 注册在线程中的监听器 */
    struct shash *server_conn_hash; /* 注册在线程中的服务端连接  */
    struct shash *client_conn_hash;  /* 注册在线程中的客户端连接 */
    volatile uint64_t pending_op; /* 线程总共正在处理多少个操作(处理中的请求个数+等待中应答个数+监听套接字数量) */

    pthread_t tid;
    pthread_mutex_t mutex;
    struct sdeque *op_queue; /* 异步与IO交互的任务队列 */
};

/* RPC框架  */
struct sio_stream_rpc {
    char is_running;

    struct sio_stream_rpc_timer_thread *timer_thread;

    uint32_t work_thread_count;
    struct sio_stream_rpc_work_thread *work_threads;

    uint32_t io_thread_count;
    struct sio_stream_rpc_io_thread *io_threads;
};

int sio_stream_rpc_init(uint32_t work_thread_count, uint32_t io_thread_count);
struct sio_stream_rpc_service *sio_stream_rpc_add_service(const char *ip, uint16_t port);
int sio_stream_rpc_register_protocol(struct sio_stream_rpc_service *service, uint32_t type,
        sio_stream_rpc_service_callback_t cb, void *arg);
int sio_stream_rpc_run();

void sio_stream_rpc_run_task(sio_stream_rpc_task_callback_t cb, void *arg);
uint64_t sio_stream_rpc_add_timer(sio_stream_rpc_timer_callback_t cb, void *arg, uint64_t after_ms);
int sio_stream_rpc_del_timer(uint64_t id);

void sio_stream_rpc_finish_cloure(struct sio_stream_rpc_server_cloure *cloure, char no_response, const char *resp, uint32_t resp_len);

#ifdef __cplusplus
}
#endif

#endif
