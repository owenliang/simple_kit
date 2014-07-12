#ifndef SIMPLE_LOG_SLOG_H
#define SIMPLE_LOG_SLOG_H

#include <stdint.h>
#include <pthread.h>
#include <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SLOG(level, fmt, ...) \
    do { \
        slog_write(SLOG_LEVEL_##level, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); \
    } while(0)

#define SLOG_GB(n) (1024ULL * 1024 * 1024 * n)
#define SLOG_MB(n) (1024ULL * 1024 * n)

enum slog_level {
    SLOG_LEVEL_FATAL,
    SLOG_LEVEL_ERROR,
    SLOG_LEVEL_WARN,
    SLOG_LEVEL_INFO,
    SLOG_LEVEL_DEBUG,
};

/* 多线程多进程日志库, 支持fork, 单例模式 */
struct slog {
    int fd; /* 当前写入的文件fd */
    char *filename; /* 当前写入的文件名 */
    struct stat fdstat; /* fd的stat结果 */

    int lockfd; /* 锁文件fd, 保证多进程安全 */
    char *lockname; /* 锁文件名 */
    pthread_mutex_t mutex; /* 保证多线程安全 */

    uint64_t rotate_size;   /* 单一文件大小限制 */
    volatile enum slog_level level; /* 当前日志级别  */
};

/**
 * @brief 初始化slog单例, 一个进程只能调用一次, fork的子进程不需重新调用
 *
 * @param [in] dir   : const char*  日志目录, 会自动创建, 需保证权限.
 * @param [in] name   : const char* 日志名称, 一般根据服务名称指定.
 * @param [in] level   : enum slog_level 日志级别, 小于给定级别的日志不会被打印
 * @param [in] rotate_size   : uint64_t 单一文件大小限制，超过限制会被转档存储
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/06/03 15:02:52
**/
int slog_open(const char *dir, const char *name, enum slog_level level, uint64_t rotate_size);
/**
 * @brief fork后的子进程必须调用，否则会导致多进程操作日志文件引起日志文件覆盖丢失
 *
 * @return  int 
 * @retval   
 * @see 因为继承的锁文件fd多进程无法互斥，所以子进程必须重新打开
 * @author liangdong
 * @date 2014/06/03 15:24:38
**/
int slog_reopen();
/**
 * @brief 关闭slog单例，一个进程只能调用一次，fork的子进程也需要调用
 *
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/06/03 15:02:56
**/
void slog_close();
/**
 * @brief 打印一条日志，用户通过宏打印日志不需要直接操作此接口
 *
 * @param [in] level   : enum slog_level 该日志的级别
 * @param [in] file   : const char* 调用者所在文件
 * @param [in] line   : int 调用者所在文件行号
 * @param [in] function   : const char* 调用者所属的函数
 * @param [in] fmt   : const char* 格式化字符串
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/06/03 15:03:00
**/
void slog_write(enum slog_level level, const char *file, int line, const char *function, const char *fmt, ...);
/**
 * @brief 立即转档当前正在写入的日志文件
 *
 * @return  void 
 * @retval   
 * @see 如果有多进程同时写日志，只需要通过一个进程调用即可。
 * @author liangdong
 * @date 2014/06/03 15:03:05
**/
void slog_rotate();
/**
 * @brief 动态修改日志级别，后续写入的日志将会根据新的级别进行过滤
 *
 * @param [in] level   : enum slog_level
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/06/03 15:06:16
**/
void slog_change_level(enum slog_level level);

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_LOG_SLOG_H */
/* vim: set ts=4 sw=4 sts=4 tw=100 */
