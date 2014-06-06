/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file sdeque.h
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/05/27 20:32:30
 * @version $Revision$ 
 * @brief 
 *  
 **/
#ifndef SIMPLE_DEQUE_SDEQUE_H
#define SIMPLE_DEQUE_SDEQUE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 双向链表的节点 */
struct sdeque_node {
    void *value;    /* 用户数据 */
    struct sdeque_node *next;    /* 前驱 */
    struct sdeque_node *prev; /* 后继 */
};

/* 双向链表 */
struct sdeque {
    uint64_t num_nodes; /* 总结点个数 */
    struct sdeque_node *head; /*头指针*/
    struct sdeque_node *tail;    /*尾指针*/
};

/**
 * @brief 创建双向链表
 *
 * @return  struct sdeque* 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/29 21:54:23
**/
struct sdeque *sdeque_new();
/**
 * @brief 释放双向链表
 *
 * @param [in] sdeque   : struct sdeque*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/29 21:54:37
**/
void sdeque_free(struct sdeque *sdeque);
/**
 * @brief 向链表头部插入数据value
 *
 * @param [in] sdeque   : struct sdeque*
 * @param [in] value   : void* 可以为NULL
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/29 21:54:52
**/
void sdeque_push_front(struct sdeque *sdeque, void *value);
/**
 * @brief 从链表头部弹出一个value
 *
 * @param [in] sdeque   : struct sdeque* 
 * @return  int 返回-1表示没有元素,0表示成功
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/29 21:55:21
**/
int sdeque_pop_front(struct sdeque *sdeque);
/**
 * @brief 获取链表头部的value
 *
 * @param [in] sdeque   : struct sdeque*
 * @param [in] value   : void** , *value被赋值为用户数据
 * @return  int 返回-1表示没有元素,0表示成功
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/29 21:55:51
**/
int sdeque_front(struct sdeque *sdeque, void **value);
/**
 * @brief 向链表尾部插入一个value
 *
 * @param [in] sdeque   : struct sdeque*
 * @param [in] value   : void* 可以为NULL
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/29 21:56:41
**/
void sdeque_push_back(struct sdeque *sdeque, void *value);
/**
 * @brief 从链表尾部弹出一个value
 *
 * @param [in] sdeque   : struct sdeque*
 * @return  int 返回-1表示没有元素,0表示成功
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/29 21:57:07
**/
int sdeque_pop_back(struct sdeque *sdeque);
/**
 * @brief 获取链表尾部的value
 *
 * @param [in] sdeque   : struct sdeque*
 * @param [in] value   : void**, *value被赋值为用户数据
 * @return  int 返回-1表示没有元素,0表示成功
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/29 21:57:48
**/
int sdeque_back(struct sdeque *sdeque, void **value);
/**
 * @brief 获取链表元素个数
 *
 * @param [in] sdeque   : struct sdeque*
 * @return  uint64_t 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/29 21:59:08
**/
uint64_t sdeque_size(struct sdeque *sdeque);

#ifdef __cplusplus
}
#endif

#endif  //SIMPLE_DEQUE_SDEQUE_H

/* vim: set ts=4 sw=4 sts=4 tw=100 */
