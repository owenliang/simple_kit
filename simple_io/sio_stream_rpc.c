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

#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "sio.h"
#include "sio_stream.h"
#include "slist.h"
#include "shash.h"
#include "sdeque.h"
#include "sio_stream_rpc.h"

static struct sio_stream_rpc singleton = {0};

static void _sio_stream_rpc_work_thread_init(struct sio_stream_rpc_work_thread *work_thread)
{
    work_thread->task_queue = sdeque_new();
    pthread_cond_init(&work_thread->cond, NULL);
    pthread_mutex_init(&work_thread->mutex, NULL);
}

static void *_sio_stream_rpc_work_thread_main(void *arg)
{
    struct sio_stream_rpc_work_thread *thread = arg;

    while (1) {
        pthread_mutex_lock(&thread->mutex);
        while (!(thread->task_count = sdeque_size(thread->task_queue))) {
            pthread_cond_wait(&thread->cond, &thread->mutex);
        }
        void *value;
        sdeque_front(thread->task_queue, &value);
        sdeque_pop_front(thread->task_queue);
        pthread_mutex_unlock(&thread->mutex);

        struct sio_stream_rpc_task *task = value;
        task->cb(task->arg);
        free(task);
    }
    return NULL;
}

static struct sio_stream_rpc_work_thread *_sio_stream_rpc_choose_work_thread()
{
    uint64_t min_pending = ~0;
    uint32_t min_index = 0;

    uint32_t i;
    for (i = 0; i < singleton.work_thread_count; ++i) {
        uint64_t pending = singleton.work_threads[i].task_count;
        if (pending < min_pending) {
            min_pending = pending;
            min_index = i;
        }
    }
    return singleton.work_threads + min_index;
}

static void _sio_stream_rpc_push_task(struct sio_stream_rpc_work_thread *thread, struct sio_stream_rpc_task *task)
{
    pthread_mutex_lock(&thread->mutex);
    sdeque_push_back(thread->task_queue, task);
    pthread_mutex_unlock(&thread->mutex);
    pthread_cond_signal(&thread->cond);
}

void sio_stream_rpc_run_task(sio_stream_rpc_task_callback_t cb, void *arg)
{
    struct sio_stream_rpc_work_thread *thread = _sio_stream_rpc_choose_work_thread();

    struct sio_stream_rpc_task *task = malloc(sizeof(*task));
    task->cb = cb;
    task->arg = arg;

    _sio_stream_rpc_push_task(thread, task);
}

static void _sio_stream_rpc_work_thread_run(struct sio_stream_rpc_work_thread *work_thread)
{
    pthread_create(&work_thread->tid, NULL, _sio_stream_rpc_work_thread_main, work_thread);
}

static void _sio_stream_rpc_timer_thread_init(struct sio_stream_rpc_timer_thread *timer_thread)
{
    pthread_mutex_init(&timer_thread->mutex, NULL);
    timer_thread->timer_id = 0;
    timer_thread->time_map = slist_new(32);
    timer_thread->id_map = slist_new(32);
}

static void _sio_stream_rpc_adjust_be(char *addr, uint32_t len)
{
    union {
        char c;
        int n;
    } helper;
    helper.n = 1;
    if (helper.c) {
        char *left = addr;
        char *right = left + len - 1;
        while (left < right) {
            char temp = *left;
            *left = *right;
            *right = temp;
            ++left, -- right;
        }
    }
}

static uint64_t _sio_stream_rpc_adjust_be64(const char *bytes)
{
    uint64_t ret = *(const uint64_t *)bytes;
    _sio_stream_rpc_adjust_be((char *)&ret, sizeof(ret));
    return ret;
}

static uint64_t _sio_stream_rpc_adjust_be32(const char *bytes)
{
    uint64_t ret = *(const uint64_t *)bytes;
    _sio_stream_rpc_adjust_be((char *)&ret, sizeof(ret));
    return ret;
}

static void _sio_stream_rpc_run_timer_task(void *arg)
{
    struct sio_stream_rpc_timer_task *task = arg;
    task->cb(task->id, task->arg);
    free(task);
}

static void *_sio_stream_rpc_timer_thread_main(void *arg)
{
    struct sio_stream_rpc_timer_thread *thread = arg;

    while (1) {
        struct slist *tasks;
        const char *key;
        void *value;

        do {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            uint64_t cur_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

            tasks = NULL;

            pthread_mutex_lock(&thread->mutex);
            if (slist_size(thread->time_map)) {
                assert(slist_front(thread->time_map, &key, NULL, &value) == 0);
                uint64_t expire = _sio_stream_rpc_adjust_be64(key);
                if (expire <= cur_ms) { /* 最早的一批定时器过期 */
                    tasks = value;
                    assert(slist_pop_front(thread->time_map) == 0);
                    slist_begin_iterate(tasks);
                    while (slist_iterate(tasks, &key, NULL, &value) != -1) { /* 从id_map中移除, 不可以再被用户取消 */
                        slist_erase(thread->id_map, key, sizeof(uint64_t));
                    }
                    slist_end_iterate(tasks);
                }
            }
            pthread_mutex_unlock(&thread->mutex);

            if (tasks) { /* 执行这批任务 */
                while (slist_front(tasks, NULL, NULL, &value) != -1) {
                    struct sio_stream_rpc_timer_task *t = value;
                    sio_stream_rpc_run_task(_sio_stream_rpc_run_timer_task, t);
                    slist_pop_front(tasks);
                }
                slist_free(tasks);
            }
        } while (tasks);    /* 没有过期的定时器, 睡眠1ms */

        usleep(1000); /* 1ms */
    }

    return NULL;
}

uint64_t sio_stream_rpc_add_timer(sio_stream_rpc_timer_callback_t cb, void *arg, uint64_t after_ms)
{
    struct sio_stream_rpc_timer_thread *thread = singleton.timer_thread;

    struct sio_stream_rpc_timer_task *task = malloc(sizeof(*task));
    task->cb = cb;
    task->arg = arg;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    task->expire_time = tv.tv_sec * 1000 + tv.tv_usec / 1000 + after_ms;

    uint64_t expire_time = _sio_stream_rpc_adjust_be64((const char *)&task->expire_time);

    pthread_mutex_lock(&thread->mutex);

    task->id = thread->timer_id++;
    uint64_t ret = task->id;
    uint64_t id = _sio_stream_rpc_adjust_be64((const char *)&task->id);
    assert(slist_insert(thread->id_map, (const char *)&id, sizeof(id), task) == 0);

    struct slist *time_list = NULL;
    void *value = NULL;
    if (slist_find(thread->time_map, (const char *)&expire_time, sizeof(expire_time), &value) == -1) {
        time_list = slist_new(32);
        assert(slist_insert(thread->time_map, (const char *)&expire_time, sizeof(expire_time), time_list) == 0);
    } else {
        time_list = value;
    }
    slist_insert(time_list, (const char *)&id, sizeof(id), task);

    pthread_mutex_unlock(&thread->mutex);

    return ret;
}

int sio_stream_rpc_del_timer(uint64_t id)
{
    struct sio_stream_rpc_timer_thread *thread = singleton.timer_thread;

    uint64_t be_id = _sio_stream_rpc_adjust_be64((const char *)&id);

    int ret = -1;

    pthread_mutex_lock(&thread->mutex);

    void *value;
    if (slist_find(thread->id_map, (const char *)&be_id, sizeof(be_id), &value) != -1) {
        slist_erase(thread->id_map, (const char *)&be_id, sizeof(be_id)); /* 从id map中删除 */

        /* 找到timer所属的list */
        struct sio_stream_rpc_timer_task *task = value;
        uint64_t expire = _sio_stream_rpc_adjust_be64((const char *)&task->expire_time);
        assert(slist_find(thread->time_map, (const char *)&expire, sizeof(expire), &value) == 0);

        struct slist *time_list = value; /* 从list中移除这个timer */
        assert(slist_erase(time_list, (const char *)&be_id, sizeof(be_id)) == 0);
        if (!slist_size(time_list)) { /* 移除后list空, 那么从time_map中删掉expire项 */
            slist_erase(thread->time_map, (const char *)&expire, sizeof(expire));
            slist_free(time_list);
        }
        ret = 0;
        free(task);
    }

    pthread_mutex_unlock(&thread->mutex);
    return ret;
}

static void _sio_stream_rpc_timer_thread_run(struct sio_stream_rpc_timer_thread *timer_thread)
{
    pthread_create(&timer_thread->tid, NULL, _sio_stream_rpc_timer_thread_main, timer_thread);
}

static void _sio_stream_rpc_io_thread_init(struct sio_stream_rpc_io_thread *io_thread, uint32_t index)
{
    io_thread->index = index;
    io_thread->conn_id = 0;
    io_thread->pending_op = 0;
    assert(io_thread->service_hash = shash_new());
    assert(io_thread->server_conn_hash = shash_new());
    assert(io_thread->client_conn_hash = shash_new());
    assert(io_thread->sio = sio_new());
    assert(io_thread->op_queue = sdeque_new());
    pthread_mutex_init(&io_thread->mutex, NULL);
}

int sio_stream_rpc_init(uint32_t work_thread_count, uint32_t io_thread_count)
{
    if (!work_thread_count || !io_thread_count)
        return -1;

    singleton.timer_thread = calloc(1, sizeof(*singleton.timer_thread));
    _sio_stream_rpc_timer_thread_init(singleton.timer_thread);

    singleton.work_thread_count = work_thread_count;
    singleton.work_threads = calloc(work_thread_count, sizeof(*singleton.work_threads));

    int i;
    for (i = 0; i < work_thread_count; ++i)
        _sio_stream_rpc_work_thread_init(singleton.work_threads + i);

    singleton.io_thread_count = io_thread_count;
    singleton.io_threads = calloc(io_thread_count, sizeof(*singleton.io_threads));
    for (i = 0; i < io_thread_count; ++i)
        _sio_stream_rpc_io_thread_init(singleton.io_threads + i, i);

    return 0;
}

static struct sio_stream_rpc_io_thread *_sio_stream_rpc_choose_io_thread()
{
    uint64_t min_pending = ~0;
    uint32_t min_index = 0;

    uint32_t i;
    for (i = 0; i < singleton.io_thread_count; ++i) {
        uint64_t pending = singleton.io_threads[i].pending_op;
        if (pending < min_pending) {
            min_pending = pending;
            min_index = i;
        }
    }
    return singleton.io_threads + min_index;
}

static void _sio_stream_rpc_queue_op(struct sio_stream_rpc_io_thread *io_thread, sio_stream_rpc_task_callback_t cb, void *arg);

static void _sio_stream_rpc_free_server_conn(struct sio_stream_rpc_server_conn *server_conn, char attached)
{
    struct sio_stream_rpc_io_thread *io_thread = server_conn->io_thread;

    if (attached)
        assert(shash_erase(io_thread->server_conn_hash, (const char *)&server_conn->conn_id,
                sizeof(server_conn->conn_id)) == 0);

    sio_stream_close(io_thread->sio, server_conn->stream);
    free(server_conn);
}

static struct sio_stream_rpc_server_cloure *_sio_stream_rpc_create_server_cloure(
        struct sio_stream_rpc_server_conn *server_conn, struct sio_stream_rpc_service_item *item,
        struct shead *reqhead, char *body)
{
    struct sio_stream_rpc_server_cloure *cloure = calloc(1, sizeof(*cloure));
    cloure->conn_id = server_conn->conn_id;
    cloure->io_thread = server_conn->io_thread;
    cloure->item = item;
    memcpy(&cloure->req_head, reqhead, sizeof(*reqhead));
    if (reqhead->body_len) {
        cloure->req_body = malloc(reqhead->body_len);
        memcpy(cloure->req_body, body, reqhead->body_len);
    }
    return cloure;
}

static void _sio_stream_rpc_handle_server_cloure(void *arg)
{
    struct sio_stream_rpc_server_cloure *cloure = arg;
    cloure->item->cb(cloure, arg);
}

static void _sio_stream_rpc_finish_cloure_in_thread(void *arg)
{
    struct sio_stream_rpc_server_cloure *cloure = arg;
    struct sio_stream_rpc_io_thread *io_thread = cloure->io_thread;

    // TODO: 1, 从哈希里找到连接, 2, 如果有response则向连接发包 3, 销毁cloure
    void *value;
    if (shash_find(io_thread->server_conn_hash, (const char *)&cloure->conn_id, sizeof(cloure->conn_id), &value) == 0) {
        struct sio_stream_rpc_server_conn *server_conn = value;
        int err = 0;
        if (!cloure->no_resp) /* 有应答 */
            err = sio_stream_write(io_thread->sio, server_conn->stream, cloure->resp_packet, cloure->resp_len);
        --io_thread->pending_op; /* 处理完成了一个请求 */
        if (err)
            _sio_stream_rpc_free_server_conn(server_conn, 1);
    }
    free(cloure->req_body);
    free(cloure->resp_packet);
    free(cloure);
}

void sio_stream_rpc_finish_cloure(struct sio_stream_rpc_server_cloure *cloure, const char *resp, uint32_t resp_len)
{
    cloure->no_resp = 0;
    cloure->resp_len = SHEAD_ENCODE_SIZE + resp_len;
    cloure->resp_packet = malloc(cloure->resp_len);
    cloure->req_head.body_len = resp_len;
    assert(shead_encode(&cloure->req_head, cloure->resp_packet, SHEAD_ENCODE_SIZE) == 0);
    memcpy(cloure->resp_packet + SHEAD_ENCODE_SIZE, resp, resp_len);

    _sio_stream_rpc_queue_op(cloure->io_thread, _sio_stream_rpc_finish_cloure_in_thread, cloure);
}

void sio_stream_rpc_terminate_cloure(struct sio_stream_rpc_server_cloure *cloure)
{
    cloure->no_resp = 1;
    _sio_stream_rpc_queue_op(cloure->io_thread, _sio_stream_rpc_finish_cloure_in_thread, cloure);
}

static void _sio_stream_rpc_handle_server_conn_data(struct sio_stream_rpc_server_conn *server_conn)
{
    struct sio_stream *stream = server_conn->stream;

    struct sio_buffer *input = sio_stream_buffer(stream);

    char *data;
    uint64_t data_len;
    sio_buffer_data(input, &data, &data_len);

    uint64_t used = 0;
    while (used < data_len) {
        uint64_t left = data_len - used;
        if (left < SHEAD_ENCODE_SIZE) /* 头部不完整 */
            break;
        struct shead reqhead;
        if (shead_decode(&reqhead, data + used, left) == -1) /* 头部解析失败 */
            return _sio_stream_rpc_free_server_conn(server_conn, 1);
        if (left - SHEAD_ENCODE_SIZE < reqhead.body_len) /* 包体不完整 */
            break;
        /* TODO:得到完整的一个包 */
        void *value;
        if (shash_find(server_conn->service->register_service, (const char *)&reqhead.type, sizeof(reqhead.type), &value) == 0) {
            /* 请求的协议被服务端支持 */
            ++server_conn->io_thread->pending_op; /* 处理中的请求+1 */
            struct sio_stream_rpc_server_cloure *cloure = _sio_stream_rpc_create_server_cloure(server_conn, value, &reqhead, data + used + SHEAD_ENCODE_SIZE);
            sio_stream_rpc_run_task(_sio_stream_rpc_handle_server_cloure, cloure);
        }
        used += SHEAD_ENCODE_SIZE + reqhead.body_len;
    }
    sio_buffer_erase(input, used);
}

/* TODO: 解析header, 判断type在service中, 生成cloure, 交给work thread, 回调service cb, 用户done cloure, 返回到对应io thread */
static void _sio_stream_rpc_sio_handle_server_conn(struct sio *sio, struct sio_stream *stream, enum sio_stream_event event, void *arg)
{
    struct sio_stream_rpc_server_conn *server_conn = arg;

    switch (event) {
    case SIO_STREAM_DATA:
        _sio_stream_rpc_handle_server_conn_data(server_conn);
        break;
    case SIO_STREAM_ERROR:
    case SIO_STREAM_CLOSE:
        _sio_stream_rpc_free_server_conn(server_conn, 1);
        break;
    default:
        assert(0);
    }
}

static void _sio_stream_rpc_attach_server_conn(void *arg)
{
    struct sio_stream_rpc_server_conn *conn = arg;
    struct sio_stream_rpc_io_thread *io_thread = conn->io_thread;

    if (sio_stream_attach(io_thread->sio, conn->stream) == -1) {
        _sio_stream_rpc_free_server_conn(conn, 0);
        return;
    }
    sio_stream_set(io_thread->sio, conn->stream, _sio_stream_rpc_sio_handle_server_conn, conn);

    conn->conn_id = io_thread->conn_id++;
    assert(shash_insert(io_thread->server_conn_hash, (const char *)&conn->conn_id, sizeof(conn->conn_id), conn) == 0);
}

static void _sio_stream_rpc_sio_handle_service(struct sio *sio, struct sio_stream *stream, enum sio_stream_event event, void *arg)
{
    struct sio_stream_rpc_service *service = arg;

    sio_stream_detach(sio, stream);

    struct sio_stream_rpc_server_conn *conn = calloc(1, sizeof(*conn));
    conn->io_thread = &singleton.io_threads[service->dispatch_index++ % singleton.io_thread_count]; /* 轮转分发连接 */
    conn->stream = stream;
    conn->service = service;
    _sio_stream_rpc_queue_op(conn->io_thread, _sio_stream_rpc_attach_server_conn, conn);
}

struct sio_stream_rpc_service *sio_stream_rpc_add_service(const char *ip, uint16_t port)
{
    struct sio_stream_rpc_io_thread *io_thread = _sio_stream_rpc_choose_io_thread();

    struct sio_stream *sfd = sio_stream_listen(io_thread->sio, ip, port, _sio_stream_rpc_sio_handle_service, NULL);
    if (!sfd)
        return NULL;

    struct sio_stream_rpc_service *service = calloc(1, sizeof(*service));
    service->io_thread = io_thread;
    service->sfd = sfd;
    service->dispatch_index = 0;
    assert(service->register_service = shash_new());
    sio_stream_set(io_thread->sio, sfd, _sio_stream_rpc_sio_handle_service, service);

    assert(shash_insert(service->register_service, (const char *)&io_thread->conn_id, sizeof(io_thread->conn_id), service) == 0);
    io_thread->conn_id++;
    io_thread->pending_op++;
    return service;
}

int sio_stream_rpc_register_protocol(struct sio_stream_rpc_service *service, uint32_t type,
        sio_stream_rpc_service_callback_t cb, void *arg)
{
    void *value = NULL;
    if (shash_find(service->register_service, (const char *)&type, sizeof(type), &value) == 0)
        return -1; /* 已经注册过type */

    struct sio_stream_rpc_service_item *item = calloc(1, sizeof(*item));
    item->cb = cb;
    item->type = type;
    item->arg = arg;
    assert(shash_insert(service->register_service, (const char *)&type, sizeof(type), item) == 0);
    return 0;
}

static void _sio_stream_rpc_io_handle_op(struct sio_stream_rpc_io_thread *io_thread)
{
    struct sdeque *op_queue = NULL;

    pthread_mutex_lock(&io_thread->mutex);
    if (sdeque_size(io_thread->op_queue)) {
        op_queue = io_thread->op_queue;
        assert(io_thread->op_queue = sdeque_new());
    }
    pthread_mutex_unlock(&io_thread->mutex);

    if (!op_queue)
        return;

    void *value;
    while (sdeque_front(op_queue, &value) != -1) {
        struct sio_stream_rpc_task *task = value;
        sdeque_pop_front(op_queue);
        task->cb(task->arg);
        free(task);
    }
    sdeque_free(op_queue);
}

static void _sio_stream_rpc_queue_op(struct sio_stream_rpc_io_thread *io_thread, sio_stream_rpc_task_callback_t cb, void *arg)
{
    struct sio_stream_rpc_task *task = malloc(sizeof(*task));
    task->cb = cb;
    task->arg = arg;

    pthread_mutex_lock(&io_thread->mutex);
    sdeque_push_back(io_thread->op_queue, task);
    pthread_mutex_unlock(&io_thread->mutex);

    sio_wakeup(io_thread->sio);
}

static void *_sio_stream_rpc_io_thread_main(void *arg)
{
    struct sio_stream_rpc_io_thread *io_thread = arg;

    while (1) {
        _sio_stream_rpc_io_handle_op(io_thread);
        sio_run(io_thread->sio, 1);
    }
    return NULL;
}

static void _sio_stream_rpc_io_thread_run(struct sio_stream_rpc_io_thread *io_thread)
{
    pthread_create(&io_thread->tid, NULL, _sio_stream_rpc_io_thread_main, io_thread);
}

int sio_stream_rpc_run()
{
    if (singleton.is_running || !singleton.work_thread_count || !singleton.io_thread_count)
        return -1;

    singleton.is_running = 1;

    _sio_stream_rpc_timer_thread_run(singleton.timer_thread);

    int i;
    for (i = 0; i < singleton.work_thread_count; ++i)
        _sio_stream_rpc_work_thread_run(singleton.work_threads + i);
    for (i = 0; i < singleton.io_thread_count; ++i)
        _sio_stream_rpc_io_thread_run(singleton.io_threads + i);
    return 0;
}
