#ifndef SIMPLE_CONFIG_SCONFIG_H
#define SIMPLE_CONFIG_SCONFIG_H

#include <stdint.h>

/*
    类shell风格的配置文件解析:
    1, 配置项格式:
    key=value
    2, 注释项格式:
    # note
    3, value不支持shell的"",''语法,也不会转义\r\n等字符, 但有限的支持了value内部含有空白字符, 例如:
    key=hello world
    4, key支持字母,数字,下划线混合,不得已数字与下滑线打头:
    day_1=monday
    5, key的右侧和value(value非空情况下)的左侧不能有空白字符
    6, value可以留空:
    key=
    7, 注释可以与配置项同行:
    key=hello #这是同行注释
 */

/* 使用simple_skiplist作为底层存储 */

#ifdef __cplusplus
extern "C" {
#endif

struct slist;

/* 支持类shell的配置解析与访问 */
struct sconfig {
    char *filename; /* 配置文件路径 */
    struct slist *items; /* 键值形式存储配置的跳表 */
};

/**
 * @brief 解析filename配置文件,并返回struct sconfig.
 *
 * @param [in] filename   : const char* 完整配置文件路径,不能为NULL
 * @return  struct sconfig*    解析失败返回NULL
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/31 15:38:02
**/
struct sconfig *sconfig_new(const char *filename);
/**
 * @brief 关闭配置结构
 *
 * @param [in] sconf   : struct sconfig* 不能为NULL
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/31 15:39:09
**/
void sconfig_free(struct sconfig *sconf);
/**
 * @brief 重新加载配置文件,如果加载失败不会影响当前配置.
 *
 * @param [in] sconf   : struct sconfig* 不能为NULL
 * @return  int 返回-1表示加载失败,返回0表示成功
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/31 15:39:55
**/
int sconfig_reload(struct sconfig *sconf);
/**
 * @brief 访问name配置项并以str形式赋值到*value
 *
 * @param [in] sconf   : struct sconfig* 不能为NULL
 * @param [in] name   : const char*   不能为NULL
 * @param [in] value   : const char** 不能为NULL
 * @return  int 返回-1表示失败,0表示成功
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/31 15:40:38
**/
int sconfig_read_str(struct sconfig *sconf, const char *name, const char **value);
/**
 * @brief 访问name配置项并以int32形式赋值到*value
 *
 * @param [in] sconf   : struct sconfig* 不能为NULL
 * @param [in] name   : const char*   不能为NULL
 * @param [in] value   : const char** 不能为NULL
 * @return  int 返回-1表示失败,0表示成功
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/31 15:40:38
**/
int sconfig_read_int32(struct sconfig *sconf, const char *name, int32_t *value);
/**
 * @brief 访问name配置项并以uint32形式赋值到*value
 *
 * @param [in] sconf   : struct sconfig* 不能为NULL
 * @param [in] name   : const char*   不能为NULL
 * @param [in] value   : const char** 不能为NULL
 * @return  int 返回-1表示失败,0表示成功
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/31 15:40:38
**/
int sconfig_read_uint32(struct sconfig *sconf, const char *name, uint32_t *value);
/**
 * @brief 访问name配置项并以int64形式赋值到*value
 *
 * @param [in] sconf   : struct sconfig* 不能为NULL
 * @param [in] name   : const char*   不能为NULL
 * @param [in] value   : const char** 不能为NULL
 * @return  int 返回-1表示失败,0表示成功
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/31 15:40:38
**/
int sconfig_read_int64(struct sconfig *sconf, const char *name, int64_t *value);
/**
 * @brief 访问name配置项并以uint64形式赋值到*value
 *
 * @param [in] sconf   : struct sconfig* 不能为NULL
 * @param [in] name   : const char*   不能为NULL
 * @param [in] value   : const char** 不能为NULL
 * @return  int 返回-1表示失败,0表示成功
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/31 15:40:38
**/
int sconfig_read_uint64(struct sconfig *sconf, const char *name, uint64_t *value);
/**
 * @brief 访问name配置项并以double形式赋值到*value
 *
 * @param [in] sconf   : struct sconfig* 不能为NULL
 * @param [in] name   : const char*   不能为NULL
 * @param [in] value   : const char** 不能为NULL
 * @return  int 返回-1表示失败,0表示成功
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/31 15:40:38
**/
int sconfig_read_double(struct sconfig *sconf, const char *name, double *value);
/**
 * @brief 初始化迭代器, 用于访问所有配置项
 *
 * @param [in] sconf   : struct sconfig*
 * @return  void 
 * @retval   
 * @see 在sconfig_reload后,需要重新调用该函数,否则无法迭代新的配置
 * @author liangdong
 * @date 2014/05/31 17:13:02
**/
void sconfig_begin_iterate(struct sconfig *sconf);
/**
 * @brief 发起一次迭代, 获取下一条配置项
 *
 * @param [in] sconf   : struct sconfig* 不能为NULL
 * @param [in] name   : const char**  可以为NULL
 * @param [in] value   : const char** 可以为NULL
 * @return  int 返回-1表示迭代结束, 返回0表示迭代成功
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/05/31 17:14:36
**/
int sconfig_iterate(struct sconfig *sconf, const char **name, const char **value);
/**
 * @brief 结束迭代器
 *
 * @param [in] sconf   : struct sconfig*
 * @return  void 
 * @retval   一旦结束迭代器, 接下来的sconfig_iterate会返回-1
 * @see  关于迭代器的特性,可以参考simple_skiplist迭代器.
 * @author liangdong
 * @date 2014/05/31 17:15:29
**/
void sconfig_end_iterate(struct sconfig *sconf);

#ifdef __cplusplus
}
#endif

#endif  //SIMPLE_CONFIG_SCONFIG_H

/* vim: set ts=4 sw=4 sts=4 tw=100 */
