#ifndef SIMPLE_IO_SIO_H
#define SIMPLE_IO_SIO_H

#include <stdint.h>
#include <sys/epoll.h>
#include "sio_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* fd触发的事件 */
enum sio_event {
    SIO_READ,   /* 可读 */
    SIO_WRITE,   /* 可写 */
    SIO_ERROR,   /* 错误 */
};

struct sio;
struct sio_fd;

/* 事件回调函数 */
typedef void (*sio_callback_t)(struct sio *sio, struct sio_fd *sfd, enum sio_event event, void *arg);

/* 注册在sio的文件描述符 */
struct sio_fd {
    int fd;     /* 用户监听的fd */
    uint32_t watch_events;    /* 用户监听的事件 */
    sio_callback_t user_callback; /* 用户的事件回调 */
    void *user_arg; /* 用户参数 */
    char is_del; /* 被sio_del移除 */
};

/* 文件描述符管理器 */
struct sio {
    int epfd; /* epoll句柄 */
    struct epoll_event poll_events[64]; /* epoll_wait的参数 */ 
    char is_in_loop;    /* 是否正在epoll_wait事件处理循环中 */
    int deferred_count; /* 延迟待删除sio_fd个数 */
    int deferred_capacity; /* 延迟待删除数组的大小 */
    struct sio_fd **deferred_to_close; /* 延迟待删除sio_fd数组 */
    int wake_pipe[2];  /* 唤醒sio_run的管道 */
    struct sio_fd *wake_sfd; /* 注册在sio上的wake_pipe[0] */
    struct sio_timer_manager *st_mgr;         /**< 定时器管理器       */
};

/**
 * @brief 创建一个文件描述符管理器
 *
 * @return  struct sio* 
 * @retval   创建失败返回NULL
 * @see 
 * @author liangdong
 * @date 2014/03/29 17:21:37
**/
struct sio *sio_new();
/**
 * @brief 释放一个文件描述符管理器, 调用前确保取消了所有注册在其上的fd和timer, 
          否则会造成内存泄漏.
 *
 * @param [in] sio   : struct sio*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/29 17:22:25
**/
void sio_free(struct sio *sio);
/**
 * @brief 注册一个fd到管理器
 *
 * @param [in] sio   : 描述符管理器, 不能为空
 * @param [in] fd   : 注册的fd, 必须>=0
 * @param [in] callback   : 事件回调函数, 不能为空
 * @param [in] arg   : 用户参数, 可以为空
 * @return  struct sio_fd* 
 * @retval   失败返回NULL
 * @see 
 * @author liangdong
 * @date 2014/03/29 17:24:05
**/
struct sio_fd *sio_add(struct sio *sio, int fd, sio_callback_t callback, void *arg);
/**
 * @brief 更新一个sio_fd的回调函数和用户参数
 *
 * @param [in] sio   : 描述符管理器, 不能为空
 * @param [in] sfd   : 必须是sio_add返回的sio_fd
 * @param [in] callback   : 事件回调函数, 不能为空
 * @param [in] arg   : 用户参数, 可以为空
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/29 17:25:53
**/
void sio_set(struct sio *sio, struct sio_fd *sfd, sio_callback_t callback, void *arg);
/**
 * @brief 取消fd的注册，函数一旦返回sio_fd不再有效, fd仍旧保持打开
 *
 * @param [in] sio   : 描述符管理器，不能为空
 * @param [in] sfd   : 必须是sio_add返回的sio_fd
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/29 17:27:52
**/
void sio_del(struct sio *sio, struct sio_fd *sfd);
/**
 * @brief 监听sio_fd的写事件(重复调用是安全的)
 *
 * @param [in] sio   : struct sio*
 * @param [in] sfd   : struct sio_fd*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/29 17:34:10
**/
void sio_watch_write(struct sio *sio, struct sio_fd *sfd);
/**
 * @brief 取消监听sio_fd的写事件(重复调用是安全的)
 *
 * @param [in] sio   : struct sio*
 * @param [in] sfd   : struct sio_fd*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/29 17:34:31
**/
void sio_unwatch_write(struct sio *sio, struct sio_fd *sfd);
/**
 * @brief 监听sio_fd的读事件(重复调用是安全的)
 *
 * @param [in] sio   : struct sio*
 * @param [in] sfd   : struct sio_fd*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/29 17:35:06
**/
void sio_watch_read(struct sio *sio, struct sio_fd *sfd);
/**
 * @brief 取消监听sio_fd的读事件(重复调用是安全的)
 *
 * @param [in] sio   : struct sio*
 * @param [in] sfd   : struct sio_fd*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/29 17:35:24
**/
void sio_unwatch_read(struct sio *sio, struct sio_fd *sfd);
/**
 * @brief 指定超时时间,执行一次事件循环并返回
 *
 * @param [in] sio   : struct sio*
 * @param [in] timeout_ms   : epoll_wait的超时时间, 毫秒
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/29 20:20:59
**/
void sio_run(struct sio *sio, int timeout_ms);
/**
 * @brief 唤醒挂起的sio_run, 线程安全函数, 常用于与sio线程的跨线程通讯
 *
 * @param [in] sio   : struct sio*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/29 21:07:20
**/
void sio_wakeup(struct sio *sio);
/**
 * @brief 启动定时器, timer内存由用户管理
 *
 * @param [in] sio   : struct sio*
 * @param [in] timer   : struct sio_timer*
 * @param [in] timeout_ms   : uint64_t 毫秒
 * @param [in] callback   : sio_timer_callback_t
 * @param [in] arg   : void*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 10:51:26
**/
void sio_start_timer(struct sio *sio, struct sio_timer *timer, uint64_t timeout_ms, sio_timer_callback_t callback, void *arg);
/**
 * @brief 停止定时器, 停止后timer内存可以由用户释放或者重用
 *
 * @param [in] sio   : struct sio*
 * @param [in] timer   : struct sio_timer*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 10:51:52
**/
void sio_stop_timer(struct sio *sio, struct sio_timer *timer);

#ifdef __cplusplus
}
#endif

#endif  //SIMPLE_IO_SIO_H

/* vim: set ts=4 sw=4 sts=4 tw=100 */
