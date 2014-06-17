/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file shash.h
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/06/16 19:18:40
 * @version $Revision$ 
 * @brief 
 *  
 **/
#ifndef SIMPLE_HASH_SHASH_H
#define SIMPLE_HASH_SHASH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化1 << 9个buckets */
#define SHASH_INIT_NUM_BUCKETS 9
/* 最多创建1 << 63个buckets */
#define SHASH_MAX_NUM_BUCKETS 63

struct shash_node {
    char *key;  /* 哈希键, 内部分配内存 */
    uint32_t key_len;   /* 键的长度 */
    void *value;    /* 用户数据 */
    struct shash_node *prev; /* 哈希桶内的节点前驱 */
    struct shash_node *next; /* 哈希桶内的节点后继 */
};

struct shash {
    uint64_t num_node; /* 总节点个数 */
    uint64_t num_buckets[2]; /* bucket[0]和buckets[1]的桶数量指数(1 << num_buckets[i]) */
    struct shash_node *buckets[2]; /* buckets[0]是当前使用中的桶数组, buckets[1]是rehash中的桶数组 */
    uint64_t rehash_bucket; /* rehash 当前迁移到哪个buckets[0]的桶 */
    uint32_t iterator_index;  /* 正在迭代buckets[0]还是buckets[1], 只在iterator非空时有意义 */
    uint64_t iterator_bucket; /* 正在迭代当前buckets数组的第几个桶, 只在iterator非空时有意义 */
    struct shash_node *iterator; /* 迭代器指向的节点, NULL表示迭代结束 */
};

/**
 * @brief 创建哈希表
 *
 * @return  struct shash* 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/06/17 13:36:24
**/
struct shash *shash_new();
/**
 * @brief 释放哈希表
 *
 * @param [in] shash   : struct shash*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/06/17 13:36:26
**/
void shash_free(struct shash *shash);
/**
 * @brief 插入长度为key_len的键, 对应值为value
 *
 * @param [in] shash   : struct shash*
 * @param [in] key   : const char* 不能为NULL
 * @param [in] key_len   : uint32_t 不能为0
 * @param [in] value   : void* 可以为NULL
 * @return  int 返回-1表示key已存在，返回0表示插入成功
 * @retval   
 * @see 内部存储key的副本
 * @author liangdong
 * @date 2014/06/17 13:36:31
**/
int shash_insert(struct shash *shash, const char *key, uint32_t key_len, void *value);
/**
 * @brief 删除长度为key_len的key
 *
 * @param [in] shash   : struct shash*
 * @param [in] key   : const char* 不能为NULL
 * @param [in] key_len   : uint32_t 不能为0
 * @return  int 返回-1表示Key不存在，返回0表示删除成功
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/06/17 13:36:35
**/
int shash_erase(struct shash *shash, const char *key, uint32_t key_len);
/**
 * @brief 查找key_len长度的key
 *
 * @param [in] shash   : struct shash*
 * @param [in] key   : const char* 不能为NULL
 * @param [in] key_len   : uint32_t 不能为0
 * @param [in] value   : void**  返回value, 可以为NULL
 * @return  int 返回-1表示key不存在，返回0表示查找成功
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/06/17 13:36:39
**/
int shash_find(struct shash *shash, const char *key, uint32_t key_len, void **value);
/**
 * @brief 初始化迭代器
 *
 * @param [in] shash   : struct shash*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/06/17 13:36:42
**/
void shash_begin_iterate(struct shash *shash);
/**
 * @brief 迭代下一个key,value
 *
 * @param [in] shash   : struct shash*
 * @param [in] key   : const char** 可以为NULL
 * @param [in] key_len   : uint32_t* 可以为NULL
 * @param [in] value   : void** 可以为NULL
 * @return  int 返回-1表示迭代结束，返回0表示迭代成功
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/06/17 13:36:44
**/
int shash_iterate(struct shash *shash, const char **key, uint32_t *key_len, void **value);
/**
 * @brief 关闭迭代器
 *
 * @param [in] shash   : struct shash*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/06/17 13:36:48
**/
void shash_end_iterate(struct shash *shash);
/**
 * @brief 返回哈希总元素个数
 *
 * @param [in] shash   : struct shash*
 * @return  uint64_t 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/06/17 13:36:52
**/
uint64_t shash_size(struct shash *shash);

#ifdef __cplusplus
}
#endif

#endif  //SIMPLE_HASH_SHASH_H

/* vim: set ts=4 sw=4 sts=4 tw=100 */
