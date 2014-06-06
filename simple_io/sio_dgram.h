/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file sio_dgram.h
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/03/31 13:32:45
 * @version $Revision$ 
 * @brief 
 *  
 **/
#ifndef SIMPLE_IO_SIO_DGRAM_H
#define SIMPLE_IO_SIO_DGRAM_H

#include <stdint.h>
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sio;
struct sio_fd;
struct sio_dgram;

/* UDP收包回调 */
typedef void (*sio_dgram_callback_t)(struct sio *sio, struct sio_dgram *sdgram,
        struct sockaddr_in *addr, char *data, uint64_t size, void *arg);

struct sio_dgram {
    int sock;
    struct sio_fd *sfd;
    sio_dgram_callback_t user_callback;
    void *user_arg;
    char inbuf[4096];
};

/**
 * @brief 创建udp socket, 绑定到ipv4和port, 
        如果udp socket作为客户端使用不关心ip或port,可以ip填写0.0.0.0,port填写0.
 *
 * @param [in] sio   : struct sio*
 * @param [in] ipv4   : const char*
 * @param [in] port   : uint16_t
 * @param [in] callback   : sio_dgram_callback_t
 * @param [in] arg   : void*
 * @return  struct sio_dgram* 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 13:55:27
**/
struct sio_dgram *sio_dgram_open(struct sio *sio, const char *ipv4, uint16_t port, sio_dgram_callback_t callback, void *arg);
/**
 * @brief 关闭udp socket
 *
 * @param [in] sio   : struct sio*
 * @param [in] sdgram   : struct sio_dgram*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 13:56:55
**/
void sio_dgram_close(struct sio *sio, struct sio_dgram *sdgram);
/**
 * @brief 通过udp socket向ipv4,port地址发送数据
 *
 * @param [in] sio   : struct sio*
 * @param [in] sdgram   : struct sio_dgram*
 * @param [in] ipv4   : const char*
 * @param [in] port   : uint16_t
 * @param [in] data   : const char*
 * @param [in] size   : uint64_t
 * @return  int 返回0表示成功, -1表示失败
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 13:58:15
**/
int sio_dgram_write(struct sio *sio, struct sio_dgram *sdgram, const char *ipv4, uint16_t port, const char *data, uint64_t size);
/**
 * @brief 通过udp socket向源地址发送应答
 *
 * @param [in] sio   : struct sio*
 * @param [in] sdgram   : struct sio_dgram*
 * @param [in] addr   : struct sockaddr_in*
 * @param [in] data   : const char*
 * @param [in] size   : uint64_t
 * @return  int 返回0表示成功, -1表示失败
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 15:19:22
**/
int sio_dgram_response(struct sio *sio, struct sio_dgram *sdgram, struct sockaddr_in *source, const char *data, uint64_t size);
/**
 * @brief 更新sio_dgram的回调函数和参数
 *
 * @param [in] sio   : struct sio*
 * @param [in] sdgram   : struct sio_dgram*
 * @param [in] callback   : sio_dgram_callback_t
 * @param [in] arg   : void*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 13:58:57
**/
void sio_dgram_set(struct sio *sio, struct sio_dgram *sdgram, sio_dgram_callback_t callback, void *arg);
/**
 * @brief 注册sio_dgram到sio
 *
 * @param [in] sio   : struct sio*
 * @param [in] sdgram   : struct sio_dgram*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 14:05:15
**/
int sio_dgram_attach(struct sio *sio, struct sio_dgram *sdgram);
/**
 * @brief 取消sio_dgram到sio的注册
 *
 * @param [in] sio   : struct sio*
 * @param [in] sdgram   : struct sio_dgram*
 * @return  void
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 14:05:40
**/
void sio_dgram_detach(struct sio *sio, struct sio_dgram *sdgram);

#ifdef __cplusplus
}
#endif

#endif  //SIMPLE_IO_SIO_DGRAM_H

/* vim: set ts=4 sw=4 sts=4 tw=100 */
