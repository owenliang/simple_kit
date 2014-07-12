#ifndef SIMPLE_IO_SIO_TIMER_H
#define SIMPLE_IO_SIO_TIMER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sio;
struct sio_timer;

/* 定时器回调函数 */
typedef void (*sio_timer_callback_t)(struct sio *sio, struct sio_timer *timer, void *arg);

/* 定时器 */
struct sio_timer {
    uint64_t index;       /**< 节点在堆中的数组下标       */
    uint64_t expire;          /**< 超时绝对时间       */
    sio_timer_callback_t user_callback;         /**< 用户回调       */
    void *user_arg;       /**< 用户参数       */
};

/* 定时器管理器, 一个"大根堆" */
struct sio_timer_manager {
    uint64_t heap_size;       /**< 堆数组的容量       */
    uint64_t heap_length;         /**< 定时器的数量       */
    struct sio_timer **heap_nodes;        /**< 堆数组       */
};

/**
 * @brief 创建一个定时器管理器
 *
 * @return  struct sio_timer_manager* 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 10:21:32
**/
struct sio_timer_manager *sio_timer_new();
/**
 * @brief 释放一个定时器管理器
 *
 * @param [in] st_mgr   : struct sio_timer_manager*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 10:21:44
**/
void sio_timer_free(struct sio_timer_manager *st_mgr);
/**
 * @brief 获取定时器数量
 *
 * @param [in] st_mgr   : struct sio_timer_manager*
 * @return  uint64_t 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 10:21:57
**/
uint64_t sio_timer_size(struct sio_timer_manager *st_mgr);
/**
 * @brief 插入定时器
 *
 * @param [in] st_mgr   : struct sio_timer_manager*
 * @param [in] timer   : struct sio_timer*
 * @return  void
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 10:22:11
**/
void sio_timer_insert(struct sio_timer_manager *st_mgr, struct sio_timer *timer);
/**
 * @brief 删除定时器
 *
 * @param [in] st_mgr   : struct sio_timer_manager*
 * @param [in] timer   : struct sio_timer*
 * @return  void
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 10:22:45
**/
void sio_timer_remove(struct sio_timer_manager *st_mgr, struct sio_timer *timer);
/**
 * @brief 修改定时器过期时间
 *
 * @param [in] st_mgr   : struct sio_timer_manager*
 * @param [in] timer   : struct sio_timer*
 * @return  void
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 10:23:03
**/
void sio_timer_modify(struct sio_timer_manager *st_mgr, struct sio_timer *timer);
/**
 * @brief 返回大根堆的根节点
 *
 * @param [in] st_mgr   : struct sio_timer_manager*
 * @return  struct sio_timer* 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 10:23:25
**/
struct sio_timer *sio_timer_top(struct sio_timer_manager *st_mgr);
/**
 * @brief 删除大根堆的根节点
 *
 * @param [in] st_mgr   : struct sio_timer_manager*
 * @return  struct sio_timer* 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/03/31 10:23:45
**/
struct sio_timer *sio_timer_pop(struct sio_timer_manager *st_mgr);

#ifdef __cplusplus
}
#endif

#endif  //SIMPLE_IO_SIO_TIMER_H

/* vim: set ts=4 sw=4 sts=4 tw=100 */
