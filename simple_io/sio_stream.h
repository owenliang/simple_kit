/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file sio_stream.h
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/03/29 23:38:26
 * @version $Revision$ 
 * @brief 
 *  
 **/
#ifndef SIMPLE_IO_SIO_STREAM_H
#define SIMPLE_IO_SIO_STREAM_H

#include <stdint.h>
#include "sio_buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

// TCP流事件
enum sio_stream_event {
    SIO_STREAM_ACCEPT,        /**< 接收了新连接      */
    SIO_STREAM_DATA,          /**< 接收了新数据       */
    SIO_STREAM_ERROR,         /**< 连接出现错误       */
    SIO_STREAM_CLOSE,         /**< 连接被关闭      */
};

struct sio;
struct sio_fd;
struct sio_stream;
struct sio_buffer;

// 用户事件回调
typedef void (*sio_stream_callback_t)(struct sio *sio, struct sio_stream *stream, enum sio_stream_event event, void *arg);

enum sio_stream_type {
    SIO_STREAM_LISTEN,
    SIO_STREAM_CONNECT,
    SIO_STREAM_NORMAL,
};

// 封装TCP连接
struct sio_stream {
    enum sio_stream_type type;        /**< socket类型       */
    int sock;         /**<  socket描述符      */
    struct sio_fd *sfd;       /**< 注册在sio上的socekt       */
    sio_stream_callback_t user_callback;          /**< 用户回调       */
    void *user_arg;       /**< 用户参数       */
    struct sio_buffer *inbuf;         /**< 读缓冲       */
    struct sio_buffer *outbuf;        /**< 写缓冲       */
};

/**
 * @brief 启动TCP监听套接字
 *
 * @param [in] sio   : struct sio*
 * @param [in] ipv4   : const char*
 * @param [in] port   : uint16_t
 * @param [in] callback   : sio_stream_callback_t
 * @param [in] arg   : void*
 * @return  struct sio_stream* 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/30 16:12:30
**/
struct sio_stream *sio_stream_listen(struct sio *sio, const char *ipv4, uint16_t port, sio_stream_callback_t callback, void *arg);
/**
 * @brief 发起TCP异步连接
 *
 * @param [in] sio   : struct sio*
 * @param [in] ipv4   : const char*
 * @param [in] port   : uint16_t
 * @param [in] callback   : sio_stream_callback_t
 * @param [in] arg   : void*
 * @return  struct sio_stream* 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/30 16:12:47
**/
struct sio_stream *sio_stream_connect(struct sio *sio, const char *ipv4, uint16_t port, sio_stream_callback_t callback, void *arg);
/**
 * @brief 更新sio_stream的回调函数和用户参数
 *
 * @param [in] sio   : struct sio*
 * @param [in] stream   : struct sio_stream*
 * @param [in] callback   : sio_stream_callback_t
 * @param [in] arg   : void*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 12:44:46
**/
void sio_stream_set(struct sio *sio, struct sio_stream *stream, sio_stream_callback_t callback, void *arg);
/**
 * @brief 关闭TCP连接
 *
 * @param [in] sio   : struct sio*
 * @param [in] stream   : struct sio_stream*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/30 16:20:52
**/
void sio_stream_close(struct sio *sio, struct sio_stream *stream);
/**
 * @brief 从sio中取消stream的注册, 保持stream可用
 *
 * @param [in] sio   : struct sio*
 * @param [in] stream   : struct sio_stream*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/30 17:25:35
**/
void sio_stream_detach(struct sio *sio, struct sio_stream *stream);
/**
 * @brief 将stream注册到sio中, 可能失败
 *
 * @param [in] sio   : struct sio*
 * @param [in] stream   : struct sio_stream*
 * @return  int 
 * @retval   失败返回-1, 成功返回0
 * @see 
 * @author liangdong
 * @date 2014/03/30 17:39:54
**/
int sio_stream_attach(struct sio *sio, struct sio_stream *stream);
/**
 * @brief 发送数据,内置缓冲
 *
 * @param [in] sio   : struct sio*
 * @param [in] stream   : struct sio_stream*
 * @param [in] data   : const char*
 * @param [in] size   : uint64_t
 * @return  int 
 * @retval   失败返回-1, 成功返回0
 * @see 
 * @author liangdong
 * @date 2014/03/30 18:21:34
**/
int sio_stream_write(struct sio *sio, struct sio_stream *stream, const char *data, uint64_t size);
/**
 * @brief 返回读缓冲区
 *
 * @param [in] stream   : struct sio_stream*
 * @return  struct sio_buffer* 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/30 18:23:29
**/
struct sio_buffer *sio_stream_buffer(struct sio_stream *stream);
/**
 * @brief 返回写缓冲区堆积数据长度
 *
 * @param [in] stream   : struct sio_stream*
 * @return  uint64_t 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/04/21 14:42:04
**/
uint64_t sio_stream_pending(struct sio_stream *stream);

#ifdef __cplusplus
}
#endif

#endif  //SIMPLE_IO_SIO_STREAM_H

/* vim: set ts=4 sw=4 sts=4 tw=100 */
