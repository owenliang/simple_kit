/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file sio_buffer.h
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/03/30 16:24:16
 * @version $Revision$ 
 * @brief 
 *  
 **/
#ifndef SIMPLE_IO_SIO_BUFFER_H
#define SIMPLE_IO_SIO_BUFFER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 支持动态收缩的缓冲区
struct sio_buffer {
    char *buffer;         /**< 缓冲区       */
    uint64_t start;       /**< 数据左端偏移       */
    uint64_t end;         /**< 数据右端偏移       */
    uint64_t capacity;    /**< 缓冲区总大小       */
};

/**
 * @brief 创建sio_buffer
 *
 * @return  struct sio_buffer* 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/30 16:30:25
**/
struct sio_buffer *sio_buffer_new();
/**
 * @brief 释放sio_buffer
 *
 * @param [in] sbuf   : struct sio_buffer*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/30 16:30:40
**/
void sio_buffer_free(struct sio_buffer *sbuf);
/**
 * @brief 向缓冲区尾部追加数据
 *
 * @param [in] sbuf   : struct sio_buffer*
 * @param [in] data   : const char*
 * @param [in] size   : uint64_t
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/30 16:43:43
**/
void sio_buffer_append(struct sio_buffer *sbuf, const char *data, uint64_t size);
/**
 * @brief 将缓冲区尾指针向后移动size字节
 *
 * @param [in] sbuf   : struct sio_buffer*
 * @param [in] size   : uint64_t
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/30 17:12:08
**/
void sio_buffer_seek(struct sio_buffer *sbuf, uint64_t size);
/**
 * @brief 从缓冲区头部删除size字节数据
 *
 * @param [in] sbuf   : struct sio_buffer*
 * @param [in] size   : uint64_t
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/30 16:43:21
**/
void sio_buffer_erase(struct sio_buffer *sbuf, uint64_t size);
/**
 * @brief 确保缓冲区尾部留有至少size字节的空闲空间
 *
 * @param [in] sbuf   : struct sio_buffer*
 * @param [in] size   : uint64_t
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/30 16:43:05
**/
void sio_buffer_reserve(struct sio_buffer *sbuf, uint64_t size);
/**
 * @brief 返回缓冲区数据段长度
 *
 * @param [in] sbuf   : struct sio_buffer*
 * @return  uint64_t 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/30 17:00:36
**/
uint64_t sio_buffer_length(struct sio_buffer *sbuf);
/**
 * @brief 返回缓冲区总大小
 *
 * @param [in] sbuf   : struct sio_buffer*
 * @return  uint64_t 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/30 17:00:51
**/
uint64_t sio_buffer_capacity(struct sio_buffer *sbuf);
/**
 * @brief 返回空闲区域的地址和长度, 需要用户调用sio_buffer_reserve预分配
 *
 * @param [in] sbuf   : struct sio_buffer*
 * @param [in] space   : 如果不需要可以传入NULL
 * @param [in] size   : 如果不需要可以传入NULL
 * @return  void 
 * @retval  *space为空闲区域首地址, *size为空闲区域长度 
 * @see 
 * @author liangdong
 * @date 2014/03/30 17:05:01
**/
void sio_buffer_space(struct sio_buffer *sbuf, char **space, uint64_t *size);
/**
 * @brief 返回数据段的首地址和长度
 *
 * @param [in] sbuf   : struct sio_buffer*
 * @param [in] data   : 不需要可以传入NULL
 * @param [in] size   :  不需要可以传入NULL
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/30 17:07:51
**/
void sio_buffer_data(struct sio_buffer *sbuf, char **data, uint64_t *size);

#ifdef __cplusplus
}
#endif

#endif  //SIMPLE_IO_SIO_BUFFER_H

/* vim: set ts=4 sw=4 sts=4 tw=100 */
