#ifndef SIMPLE_PACK_SPACK_H
#define SIMPLE_PACK_SPACK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *
 * 	这是一个MessagePack协议的实现(https://github.com/msgpack/msgpack/blob/master/spec.md),
 * 	实现比较粗糙, 但保证基本的安全性, 可以用于网络通讯中安全的序列化与反序列化.
 *
 * 	序列化, 反序列化操作必须按顺序调用, 没有对数据进行抽象, 所以也没有什么便捷的按key或者index访问的方式.
 *
 * 	以下API成功返回0, 失败返回-1, 一旦失败不要再操作序列化/反序列化对象了.
 *
 * 	同时, 这些API不会分配任何临时内存, 所有操作都是基于用户提供buffer的.
 * */

/* 序列化器 */
struct spack_w {
	char *buf;	/* 用户提供缓冲区用于存储序列化数据 */
	uint64_t buf_used;	/* buf使用的长度 */
	uint64_t buf_capacity; /* buf的容量, 超过容量将序列化失败 */
};

/* 反序列化器 */
struct spack_r {
	const char *buf; /* 用户提供待反序列化的数据 */
	uint64_t buf_used; /* 已反序列化的长度 */
	uint64_t buf_size; /* 用户提供的数据总长度 */
};

/**
 * @brief 初始化序列化, 数据将被写入buf_capacity大小的buf中
 *
 * @param [in] wpack   : struct spack_w*
 * @param [in] buf   : char*
 * @param [in] buf_capacity   : uint64_t
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:34:04
**/
void spack_w_init(struct spack_w *wpack, char *buf, uint64_t buf_capacity);
/**
 * @brief 返回序列化后数据长度
 *
 * @param [in] wpack   : struct spack_w*
 * @return  uint64_t 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:57:16
**/
uint64_t spack_w_size(struct spack_w *wpack);
/**
 * @brief 序列化一个单字节的NIL
 *
 * @param [in] wpack   : struct spack_w*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:34:50
**/
int spack_put_nil(struct spack_w *wpack);
/**
 * @brief 序列化一个单字节的布尔变量
 *
 * @param [in] wpack   : struct spack_w*
 * @param [in] is_true   : uint8_t
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:35:50
**/
int spack_put_boolean(struct spack_w *wpack, uint8_t is_true);
/**
 * @brief 序列化一个int变量, 最大支持64位int.
 *
 * 内部会根据int实际大小压缩为fixint/int8/int16/int32/int64.
 *
 * @param [in] wpack   : struct spack_w*
 * @param [in] i64   : int64_t
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:36:19
**/
int spack_put_int(struct spack_w *wpack, int64_t i64);
/**
 * @brief 序列化一个uint变量, 最大支持64位uint.
 *
 * 内部会根据uint实际大小压缩为fixuint/uint8/uint16/uint32/uint64
 *  
 * @param [in] wpack   : struct spack_w*
 * @param [in] u64   : uint64_t
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:37:23
**/
int spack_put_uint(struct spack_w *wpack, uint64_t u64);
/**
 * @brief 序列化一段二进制数据, 最大支持uint32位长度
 * 
 * 内部会根据uint实际大小压缩为uint8/uint16/uint32
 * 
 * @param [in] wpack   : struct spack_w*
 * @param [in] data   : const char*
 * @param [in] len   : uint32_t
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:38:13
**/
int spack_put_bin(struct spack_w *wpack, const char *data, uint32_t len);
/**
 * @brief 序列化一个float
 *
 * @param [in] wpack   : struct spack_w*
 * @param [in] f   : float
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:39:44
**/
int spack_put_float(struct spack_w *wpack, float f);
/**
 * @brief 序列化一个double
 *
 * @param [in] wpack   : struct spack_w*
 * @param [in] d   : double
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:40:21
**/
int spack_put_double(struct spack_w *wpack, double d);
/**
 * @brief 序列化一个自定义对象, 类型为type, 数据最长为32位uint
 *
 * 内部会根据uint实际大小压缩为fixext-1/fixext-2/fixext-4/fixext-8/fixext-16/ext8/ext16/ext32
 *
 * @param [in] wpack   : struct spack_w*
 * @param [in] type   : int8_t
 * @param [in] data   : const void*
 * @param [in] size   : uint32_t
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:40:34
**/
int spack_put_ext(struct spack_w *wpack, int8_t type, const void *data, uint32_t size);
/**
 * @brief 序列化一个长度为size字符串(不含0),
 *
 * 内部会在末尾追加0字节, 并会根据uint实际大小压缩为fixstr/str8/str16/str32
 *
 * @param [in] wpack   : struct spack_w*
 * @param [in] data   : const char*
 * @param [in] size   : uint32_t
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:42:19
**/
int spack_put_str(struct spack_w *wpack, const char *data, uint32_t size);
/**
 * @brief 序列化一个有size个元素的array,
 *
 * 实际上只是写入数组长度size, 会根据uint实际大小压缩为fixarray/array16/array32
 *
 * @param [in] wpack   : struct spack_w*
 * @param [in] size   : uint32_t
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:44:12
**/
int spack_put_array(struct spack_w *wpack, uint32_t size);
/**
 * @brief 序列化一个有size个k,v对的map字典,
 *
 * 实际上只写入map的k,v对个数size, 会根据uint实际大小压缩为fixmap/map16/map32
 *
 * @param [in] wpack   : struct spack_w*
 * @param [in] size   : uint32_t
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:44:44
**/
int spack_put_map(struct spack_w *wpack, uint32_t size);


/**
 * @brief 初始化反序列化, 传入待解析缓冲区
 *
 * @param [in] rpack   : struct spack_r*
 * @param [in] buf   : const char*
 * @param [in] buf_size   : uint64_t
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:48:00
**/
void spack_r_init(struct spack_r *rpack, const char *buf, uint64_t buf_size);
/**
 * @brief 反序列化一个NIL
 *
 * @param [in] rpack   : struct spack_r*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:49:39
**/
int spack_get_nil(struct spack_r *rpack);
/**
 * @brief 反序列化一个布尔
 *
 * @param [in] rpack   : struct spack_r*
 * @param [in] is_true   : uint8_t*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:49:49
**/
int spack_get_boolean(struct spack_r *rpack, uint8_t *is_true);
/**
 * @brief 反序列化一个int
 *
 * @param [in] rpack   : struct spack_r*
 * @param [in] i64   : int64_t*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:50:02
**/
int spack_get_int(struct spack_r *rpack, int64_t *i64);
/**
 * @brief 反序列化一个uint.
 *
 * 特别: 如果序列化时使用int类型, 并且>=0, 那么反序列化能够解析为uint, 否则返回-1.
 *
 * @param [in] rpack   : struct spack_r*
 * @param [in] u64   : uint64_t*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:50:10
**/
int spack_get_uint(struct spack_r *rpack, uint64_t *u64);
/**
 * @brief 反序列化一段二进制
 *
 * @param [in] rpack   : struct spack_r*
 * @param [in] data   : const char**
 * @param [in] len   : uint32_t*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:53:21
**/
int spack_get_bin(struct spack_r *rpack, const char **data, uint32_t *len);
/**
 * @brief 反序列化一个float
 *
 * @param [in] rpack   : struct spack_r*
 * @param [in] f   : float*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:53:33
**/
int spack_get_float(struct spack_r *rpack, float *f);
/**
 * @brief 反序列化一个double
 *
 * @param [in] rpack   : struct spack_r*
 * @param [in] d   : double*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:53:47
**/
int spack_get_double(struct spack_r *rpack, double *d);
/**
 * @brief 反序列化一个自定义对象
 *
 * @param [in] rpack   : struct spack_r*
 * @param [in] type   : int8_t*
 * @param [in] data   : const char**
 * @param [in] size   : uint32_t*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:53:54
**/
int spack_get_ext(struct spack_r *rpack, int8_t *type, const char **data, uint32_t *size);
/**
 * @brief 反序列化一个字符串, 接口保证str是0结尾的
 *
 * @param [in] rpack   : struct spack_r*
 * @param [in] str   : const char**
 * @param [in] size   : uint32_t*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:54:04
**/
int spack_get_str(struct spack_r *rpack, const char **str, uint32_t *size);
/**
 * @brief 反序列化一个数组, 实际上是返回数组的大小
 *
 * @param [in] rpack   : struct spack_r*
 * @param [in] size   : uint32_t*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:54:28
**/
int spack_get_array(struct spack_r *rpack, uint32_t *size);
/**
 * @brief 反序列化一个字典, 实际上是返回字典的大小
 *
 * @param [in] rpack   : struct spack_r*
 * @param [in] size   : uint32_t*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/11 23:54:47
**/
int spack_get_map(struct spack_r *rpack, uint32_t *size);

#ifdef __cplusplus
}
#endif

#endif
