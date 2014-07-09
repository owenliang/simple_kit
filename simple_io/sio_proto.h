/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file sio_proto.h
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/07/07 17:15:08
 * @version $Revision$ 
 * @brief 
 *  
 **/
#ifndef SIMPLE_IO_SIO_PROTO_H
#define SIMPLE_IO_SIO_PROTO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 所有多字节整形字段按大端存储.
 *
 * 	header: 消息头, 定长20字节.
 * 		uint64_t id; 消息唯一标识
 * 		uint32_t command; 消息命令号
 * 		uint32_t meta_len;  元数据的长度
 * 		uint32_t body_len;  数据段的长度
 *
 * 	meta: body的元信息, 记录字段类型和字段偏移, 每个索引占用定长5字节.
 * 		enum sio_proto_field_type type; 字段类型(1字节)
 * 		uint32_t fields_offset; 字段在body中的字节偏移
 *
 * 	body: 数据段, 每个field连续紧密排列,
 * 	 SIO_PROTO_BOOLEAN: 0或1(1字节)
 * 	 SIO_PROTO_(U)INT32: 4字节
 * 	 SIO_PROTO_(U)INT64: 8字节
 * 	 SIO_PROTO_FLOAT: 按SIO_PROTO_STRING格式存储
 * 	 SIO_PROTO_DOUBLE: 按SIO_PROTO_STRING格式存储
 * 	 SIO_PROTO_STRING: 前4个字节为uint32记录字符串长度(包含NULL字符), 后接字符串内容
 * */

/* 字段类型 */
enum sio_proto_field_type {
	SIO_PROTO_NONE = 0,
	SIO_PROTO_BOOLEAN,
	SIO_PROTO_INT32,
	SIO_PROTO_UINT32,
	SIO_PROTO_INT64,
	SIO_PROTO_UINT64,
	SIO_PROTO_FLOAT,
	SIO_PROTO_DOUBLE,
	SIO_PROTO_STRING,
};

/* 消息头 */
struct sio_proto_header {
	 uint64_t id; /* 消息唯一标识 */
	 uint32_t command; /* 消息命令号 */
	 uint32_t meta_len;  /* 元数据的长度 */
	 uint32_t body_len;  /* 数据段的长度 */
};

/* 元信息 */
struct sio_proto_meta {
	enum sio_proto_field_type type;
	union {
		uint8_t boolean;
		int32_t i32;
		uint32_t u32;
		int64_t i64;
		uint64_t u64;
		float f;
		double d;
		struct {
			char *buf;
			uint32_t len;
		} string;
	} value;
	uint32_t offset; /* 在body中的偏移 */
	struct sio_proto_meta *next; /* 只在wmessage中使用 */
};

/* 生成消息 */
struct sio_proto_wmessage {
	struct sio_proto_header header;
	uint32_t meta_num;	/* field个数 */
	struct sio_proto_meta *meta_head; /* 所有field组成的链表头部 */
	struct sio_proto_meta *meta_tail;   /*  链表的尾部 */
};

/* 解析消息 */
struct sio_proto_rmessage {
	struct sio_proto_header header;
	uint32_t meta_num;
	struct sio_proto_meta *meta_arr; /* 数组, 高速下标索引 */
};

/**
 * @brief 初始化wmessage
 *
 * @param [in] msg   : struct sio_proto_wmessage*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:12:23
**/
void sio_proto_wmessage_create(struct sio_proto_wmessage *msg);
/**
 * @brief 释放wmessage资源,与create成对使用
 *
 * @param [in] msg   : struct sio_proto_wmessage*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:12:47
**/
void sio_proto_wmessage_destroy(struct sio_proto_wmessage *msg);
/**
 * @brief 设置消息ID
 *
 * @param [in] msg   : struct sio_proto_wmessage*
 * @param [in] id   : uint64_t
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:13:10
**/
void sio_proto_wmessage_set_id(struct sio_proto_wmessage *msg, uint64_t id);
/**
 * @brief 设置消息命令
 *
 * @param [in] msg   : struct sio_proto_wmessage*
 * @param [in] command   : uint32_t
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:13:25
**/
void sio_proto_wmessage_set_command(struct sio_proto_wmessage *msg, uint32_t command);
/**
 * @brief 序列化消息
 *
 * @param [in] msg   : struct sio_proto_wmessage*
 * @param [in] buf   : char** 不能为空
 * @param [in] len   : uint32_t* 不能为空
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:13:34
**/
void sio_proto_wmessage_serialize(struct sio_proto_wmessage *msg, char **buf, uint32_t *len);
/**
 * @brief 追加一条布尔数据
 *
 * @param [in] msg   : struct sio_proto_wmessage*
 * @param [in] boolean   : uint8_t
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:13:52
**/
void sio_proto_wmessage_put_boolean(struct sio_proto_wmessage *msg, uint8_t boolean);
/**
 * @brief 追加一条int32数据
 *
 * @param [in] msg   : struct sio_proto_wmessage*
 * @param [in] i32   : int32_t
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:14:09
**/
void sio_proto_wmessage_put_int32(struct sio_proto_wmessage *msg, int32_t i32);
/**
 * @brief 追加一条uin32数据
 *
 * @param [in] msg   : struct sio_proto_wmessage*
 * @param [in] u32   : uint32_t
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:14:23
**/
void sio_proto_wmessage_put_uint32(struct sio_proto_wmessage *msg, uint32_t u32);
/**
 * @brief 追加一条uint64数据
 *
 * @param [in] msg   : struct sio_proto_wmessage*
 * @param [in] i64   : int64_t
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:14:33
**/
void sio_proto_wmessage_put_int64(struct sio_proto_wmessage *msg, int64_t i64);
/**
 * @brief 追加一条float数据
 *
 * @param [in] msg   : struct sio_proto_wmessage*
 * @param [in] u64   : int64_t
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:14:42
**/
void sio_proto_wmessage_put_uint64(struct sio_proto_wmessage *msg, int64_t u64);
/**
 * @brief 追加一条float数据
 *
 * @param [in] msg   : struct sio_proto_wmessage*
 * @param [in] f   : float
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:14:55
**/
void sio_proto_wmessage_put_float(struct sio_proto_wmessage *msg, float f);
/**
 * @brief 追加一条double数据
 *
 * @param [in] msg   : struct sio_proto_wmessage*
 * @param [in] d   : double
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:15:03
**/
void sio_proto_wmessage_put_double(struct sio_proto_wmessage *msg, double d);
/**
 * @brief 追加一条字符串数据
 *
 * @param [in] msg   : struct sio_proto_wmessage*
 * @param [in] string   : char*
 * @param [in] len   : uint32_t
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:15:18
**/
void sio_proto_wmessage_put_string(struct sio_proto_wmessage *msg, const char *string, uint32_t len);

/**
 * @brief 初始化rmessage
 *
 * @param [in] msg   : struct sio_proto_rmessage*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:15:30
**/
void sio_proto_rmessage_create(struct sio_proto_rmessage *msg);
/**
 * @brief 释放rmessage资源, 与create成对使用
 *
 * @param [in] msg   : struct sio_proto_rmessage*
 * @return  void 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:15:42
**/
void sio_proto_rmessage_destroy(struct sio_proto_rmessage *msg);
/**
 * @brief 反序列化
 *
 * @param [in] msg   : struct sio_proto_rmessage*
 * @param [in] buf   : const char*
 * @param [in] len   : uint32_t
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:15:59
**/
int sio_proto_rmessage_unserialize(struct sio_proto_rmessage *msg, const char *buf, uint32_t len);
/**
 * @brief 获取消息ID
 *
 * @param [in] msg   : struct sio_proto_rmessage*
 * @return  uint64_t 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:16:09
**/
uint64_t sio_proto_rmessage_get_id(struct sio_proto_rmessage *msg);
/**
 * @brief 获取消息命令
 *
 * @param [in] msg   : struct sio_proto_rmessage*
 * @return  uint32_t 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:17:20
**/
uint32_t sio_proto_rmessage_get_command(struct sio_proto_rmessage *msg);
/**
 * @brief 获取index位置的布尔
 *
 * @param [in] msg   : struct sio_proto_rmessage*
 * @param [in] index   : uint32_t
 * @param [in] boolean   : uint8_t*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:19:49
**/
int sio_proto_rmessage_get_boolean(struct sio_proto_rmessage *msg, uint32_t index, uint8_t *boolean);
/**
 * @brief 获取index位置的int32
 *
 * @param [in] msg   : struct sio_proto_rmessage*
 * @param [in] index   : uint32_t
 * @param [in] i32   : int32_t*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:20:09
**/
int sio_proto_rmessage_get_int32(struct sio_proto_rmessage *msg, uint32_t index, int32_t *i32);
/**
 * @brief 获取index位置的uint32
 *
 * @param [in] msg   : struct sio_proto_rmessage*
 * @param [in] index   : uint32_t
 * @param [in] u32   : uint32_t*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:20:19
**/
int sio_proto_rmessage_get_uint32(struct sio_proto_rmessage *msg, uint32_t index, uint32_t *u32);
/**
 * @brief 获取index位置的int64
 *
 * @param [in] msg   : struct sio_proto_rmessage*
 * @param [in] index   : uint32_t
 * @param [in] i64   : int64_t*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:20:30
**/
int sio_proto_rmessage_get_int64(struct sio_proto_rmessage *msg, uint32_t index, int64_t *i64);
/**
 * @brief 获取index位置的uint64
 *
 * @param [in] msg   : struct sio_proto_rmessage*
 * @param [in] index   : uint32_t
 * @param [in] u64   : uint64_t*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:20:37
**/
int sio_proto_rmessage_get_uint64(struct sio_proto_rmessage *msg, uint32_t index, uint64_t *u64);
/**
 * @brief 获取index位置的float
 *
 * @param [in] msg   : struct sio_proto_rmessage*
 * @param [in] index   : uint32_t
 * @param [in] f   : float*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:20:49
**/
int sio_proto_rmessage_get_float(struct sio_proto_rmessage *msg, uint32_t index, float *f);
/**
 * @brief 获取Index位置的double
 *
 * @param [in] msg   : struct sio_proto_rmessage*
 * @param [in] index   : uint32_t
 * @param [in] d   : double*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:20:59
**/
int sio_proto_rmessage_get_double(struct sio_proto_rmessage *msg, uint32_t index, double *d);
/**
 * @brief 获取index位置的string
 *
 * @param [in] msg   : struct sio_proto_rmessage*
 * @param [in] index   : uint32_t
 * @param [in] string   : const char**
 * @param [in] len   : uint32_t*
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/07/09 14:21:07
**/
int sio_proto_rmessage_get_string(struct sio_proto_rmessage *msg, uint32_t index, const char **string, uint32_t *len);

#ifdef __cplusplus
}
#endif

#endif  //SIMPLE_IO_SIO_PROTO_H

/* vim: set ts=4 sw=4 sts=4 tw=100 */
