#ifndef SIMPLE_SKIPLIST_SLIST_H
#define SIMPLE_SKIPLIST_SLIST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct slist_node;

/* 跳表分级索引 */
struct slist_level {
    struct slist_node *next; /* 当前level的后继 */
    struct slist_node *prev; /* 当前level的前驱 */
    uint64_t span; /* 向next移动会跳过多少个结点 */
};

/* 跳表中的结点 */
struct slist_node {
    char *key;    /* 节点的键,内部分配 */
    uint32_t key_len;   /* 键值的长度 */
    uint32_t height;    /* 节点的最高索引级别 */
    void *value;        /* 节点的值,用户传入 */
    struct slist_level levels[1];   /* 多级索引柔性数组,至少1级 */
};

/* 跳表数据结构: 一个多级索引且有序的高效链表 */
struct slist {
    uint32_t max_height;  /* 用户配置的最高索引层级,至少1级 */
    uint32_t cur_height;  /* 当前最高构建过的索引层级 */
    uint64_t num_nodes;   /* 总节点个数 */
    uint32_t seed; /* 生成随机数的种子 */
    struct slist_node *iterator; /* 正向迭代器 */
    struct slist_node *riterator; /* 反向迭代器 */
    struct slist_node *head;  /* 链表头节点,不含用户键值 */
    struct slist_node *tail;  /* 链表尾节点,key最大 */
    uint64_t *level_rank;   /* 用于暂存插入结点的每一级前驱的排位 */
    struct slist_node *level_node[1]; /* 用于暂存插入结点的每一级链表前驱 */
};

/**
 * @brief 创建一个高度不超过height的跳表
 *
 * @param [in] height   : uint32_t, 如果为0则蜕化为普通链表, 建议值不超过32.
 * @return  struct slist* 
 * @retval   返回NULL表示创建失败
 * @see 
 * @author liangdong
 * @date 2014/05/26 18:02:07
**/
struct slist *slist_new(uint32_t height);
/**
 * @brief 释放跳表
 *
 * @param [in] slist   : struct slist*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/26 18:02:22
**/
void slist_free(struct slist *slist);
/**
 * @brief 插入一个长度key_len的key, 其值为value, 接口会存储key的副本而不是自身.
 *
 * @param [in] slist   : struct slist*
 * @param [in] key   : const char*  不能为NULL
 * @param [in] key_len   : uint32_t 不能为0
 * @param [in] value   : void*      可以为NULL
 * @return  int 返回0表示插入成功,返回-1表示key已存在
 * @retval
 * @see 
 * @author liangdong
 * @date 2014/05/26 18:02:38
**/
int slist_insert(struct slist *slist, const char *key, uint32_t key_len, void *value);
/**
 * @brief 删除key对应的结点
 *
 * @param [in] slist   : struct slist*
 * @param [in] key   : const char*  不能为NULL
 * @param [in] key_len   : uint32_t 不能为0
 * @return  int 返回0表示删除成功,返回-1表示key不存在
 * @retval
 * @see 
 * @author liangdong
 * @date 2014/05/26 18:06:03
**/
int slist_erase(struct slist *slist, const char *key, uint32_t key_len);
/**
 * @brief 查找key对应的value
 *
 * @param [in] slist   : struct slist*
 * @param [in] key   : const char*  不能为NULL
 * @param [in] key_len   : uint32_t 不能为0
 * @param [in] value   : void**     不为NULL则赋值*value为对应数据
 * @return  int 返回0表示查找成功,返回-1表示key不存在
 * @retval
 * @see 
 * @author liangdong
 * @date 2014/05/26 18:06:56
**/
int slist_find(struct slist *slist, const char *key, uint32_t key_len, void **value);
/**
 * @brief 返回跳表中最小的key和value
 *
 * @param [in] slist   : struct slist*
 * @param [in] key   : const char**  非NULL则赋值*key,注意*key指向节点内部内存.
 * @param [in] key_len   : uint32_t* 非NULL则赋值*key_len为key长度
 * @param [in] value   : void**      非NULL则赋值*value为对应数据
 * @return  int 返回0表示得到头结点,返回-1表示链表为空
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/26 18:09:32
**/
int slist_front(struct slist *slist, const char **key, uint32_t *key_len, void **value);
/**
 * @brief 弹出跳表中最小的key
 *
 * @param [in] slist   : struct slist*
 * @return  int 返回0表示成功,返回-1表示链表为空
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/26 18:11:24
**/
int slist_pop_front(struct slist *slist);
/**
 * @brief 返回跳表中最大的key和value
 *
 * @param [in] slist   : struct slist*
 * @param [in] key   : const char**  非NULL则赋值*key
 * @param [in] key_len   : uint32_t* 非NULL则赋值*key_len为key长度
 * @param [in] value   : void**      非NULL则赋值*value为对应数据
 * @return  int 返回0表示得到头结点,返回-1表示链表为空
 * @retval   注意*key指向内部使用的内存, 在节点删除后内存无效.
 * @see 
 * @author liangdong
 * @date 2014/05/26 19:26:46
**/
int slist_back(struct slist *slist, const char **key, uint32_t *key_len, void **value);
/**
 * @brief 弹出跳表中最大的key
 *
 * @param [in] slist   : struct slist*
 * @return  int 
 * @retval  返回0表示成功,返回-1表示链表为空
 * @see 
 * @author liangdong
 * @date 2014/05/26 19:32:07
**/
int slist_pop_back(struct slist *slist);
/**
 * @brief 
 *
 * @param [in] slist   : struct slist*
 * @return  uint64_t 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/26 18:13:14
**/
uint64_t slist_size(struct slist *slist);
/**
 * @brief 计算key的排名
 *
 * @param [in] slist   : struct slist*
 * @param [in] key   : const char*  不能为NULL
 * @param [in] key_len   : uint32_t 不能为0
 * @return  int 返回非0表示成功,返回-1表示key不存在
 * @retval
 * @see 
 * @author liangdong
 * @date 2019/02/13 10:16:56
**/
uint64_t slist_rank(struct slist *slist, const char* key, uint32_t key_len);
/**
 * @brief 重置正向迭代器(key从小到大)
 *
 * @param [in] slist   : struct slist*
 * @return  void 
 * @retval   
 * @see 内部实现仅仅将迭代器指向head之后的第一个节点,所以在任何情况下调用都是安全的.
 * @author liangdong
 * @date 2014/05/27 14:45:53
**/
void slist_begin_iterate(struct slist *slist);
/**
 * @brief 返回迭代器当前指向的key,value,并令迭代器正向移动一步
 *
 * @param [in] slist   : struct slist*
 * @param [in] key   : const char**
 * @param [in] key_len   : uint32_t*
 * @param [in] value   : void**
 * @return  int 
 * @retval   返回-1表示迭代结束,返回0表示迭代成功
 * @see 正向迭代器和反向迭代器是彼此独立的,可以同时打开与移动.
 * @author liangdong
 * @date 2014/05/27 14:46:21
**/
int slist_iterate(struct slist *slist, const char **key, uint32_t *key_len, void **value);
/**
 * @brief 关闭正向迭代器
 *
 * @param [in] slist   : struct slist*
 * @return  void 
 * @retval   
 * @see 内部实现仅仅将迭代器指向的节点置为NULL,所以在任何情况下调用都是安全的.
 * @author liangdong
 * @date 2014/05/27 14:47:24
**/
void slist_end_iterate(struct slist *slist);
/**
 * @brief 重置反向迭代器(key从大到小)
 *
 * @param [in] slist   : struct slist*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/27 14:49:19
**/
void slist_begin_riterate(struct slist *slist);
/**
 * @brief 返回迭代器当前指向的key,value,并令迭代器反向移动一步
 *
 * @param [in] slist   : struct slist*
 * @param [in] key   : const char**
 * @param [in] key_len   : uint32_t*
 * @param [in] value   : void**
 * @return  int 
 * @retval   返回-1表示迭代结束,返回0表示迭代成功
 * @see 内部实现仅仅将迭代器指向的节点置为NULL,所以在任何情况下调用都是安全的.
 * @author liangdong
 * @date 2014/05/27 14:49:32
**/
int slist_riterate(struct slist *slist, const char **key, uint32_t *key_len, void **value);
/**
 * @brief 关闭反向迭代器
 *
 * @param [in] slist   : struct slist*
 * @return  void 
 * @retval   
 * @see 内部实现仅仅将迭代器指向的节点置为NULL,所以在任何情况下调用都是安全的.
 * @author liangdong
 * @date 2014/05/27 14:50:39
**/
void slist_end_riterate(struct slist *slist);

#ifdef __cplusplus
}
#endif

#endif  //SIMPLE_SKIPLIST_SLIST_H

/* vim: set ts=4 sw=4 sts=4 tw=100 */
