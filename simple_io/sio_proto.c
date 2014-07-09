/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file sio_proto.c
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/07/07 17:15:33
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sio_proto.h"

void sio_proto_wmessage_create(struct sio_proto_wmessage *msg)
{
	memset(msg, 0, sizeof(*msg));
}

void sio_proto_wmessage_destroy(struct sio_proto_wmessage *msg)
{
	struct sio_proto_meta *meta = msg->meta_head;
	while (meta) {
		struct sio_proto_meta *next = meta->next;
		if (meta->type == SIO_PROTO_FLOAT || meta->type == SIO_PROTO_DOUBLE ||
		        meta->type == SIO_PROTO_STRING)
			free(meta->value.string.buf);
		free(meta);
		meta = next;
	}
}

void sio_proto_wmessage_set_id(struct sio_proto_wmessage *msg, uint64_t id)
{
	msg->header.id = id;
}

void sio_proto_wmessage_set_command(struct sio_proto_wmessage *msg, uint32_t command)
{
	msg->header.command = command;
}

static void _sio_proto_adjust_byte_order(char *bytes, uint32_t len)
{
	union {
		char is_not_network;
		uint32_t i32;
	} helper;

	helper.i32 = 1;
	if (helper.is_not_network) {
		char *end = bytes + len - 1;
		while (bytes < end) {
			char ch = *bytes;
			*bytes = *end;
			*end = ch;
			++bytes, -- end;
		}
	}
}

void sio_proto_wmessage_serialize(struct sio_proto_wmessage *msg, char **buf, uint32_t *len)
{
    *len = 20 + msg->header.meta_len + msg->header.body_len;
    char *builder = malloc(*len);
    *buf = builder;

	/* id */
	uint64_t id = msg->header.id;
	_sio_proto_adjust_byte_order((char *)&id, sizeof(id));
	memcpy(builder, &id, sizeof(id));
	builder += sizeof(id);
	/* command */
	uint32_t command = msg->header.command;
	_sio_proto_adjust_byte_order((char *)&command, sizeof(command));
	memcpy(builder, &command, sizeof(command));
	builder += sizeof(command);
	/* meta_len */
	uint32_t meta_len = msg->header.meta_len;
	_sio_proto_adjust_byte_order((char *)&meta_len, sizeof(meta_len));
	memcpy(builder, &meta_len, sizeof(meta_len));
	builder += sizeof(meta_len);
	/* body_len */
	uint32_t body_len = msg->header.body_len;
	_sio_proto_adjust_byte_order((char *)&body_len, sizeof(body_len));
	memcpy(builder, &body_len, sizeof(body_len));
	builder += sizeof(body_len);
	/* meta & body */
	char *body_begin = builder + msg->header.meta_len;
	char *body_builder = body_begin;

	struct sio_proto_meta *meta = msg->meta_head;
	while (meta) {
		*builder++ = meta->type;
		uint32_t offset = body_builder - body_begin;
		_sio_proto_adjust_byte_order((char *)&offset, sizeof(offset));
		memcpy(builder, &offset, sizeof(offset));
		builder += sizeof(offset);
		if (meta->type == SIO_PROTO_BOOLEAN) {
			*body_builder = meta->value.boolean;
			++body_builder;
		} else if (meta->type == SIO_PROTO_INT32) {
			int32_t i32 = meta->value.i32;
			_sio_proto_adjust_byte_order((char *)&i32, sizeof(i32));
			memcpy(body_builder, &i32, sizeof(i32));
			body_builder += 4;
		} else if (meta->type == SIO_PROTO_UINT32) {
			uint32_t u32 = meta->value.u32;
			_sio_proto_adjust_byte_order((char *)&u32, sizeof(u32));
			memcpy(body_builder, &u32, sizeof(u32));
			body_builder += 4;
		} else if (meta->type == SIO_PROTO_INT64) {
			int64_t i64 = meta->value.i64;
			_sio_proto_adjust_byte_order((char *)&i64, sizeof(i64));
			memcpy(body_builder, &i64, sizeof(i64));
			body_builder += 8;
		} else if (meta->type == SIO_PROTO_UINT64) {
			int64_t u64 = meta->value.u64;
			_sio_proto_adjust_byte_order((char *)&u64, sizeof(u64));
			memcpy(body_builder, &u64, sizeof(u64));
			body_builder += 8;
		} else if (meta->type == SIO_PROTO_FLOAT || meta->type == SIO_PROTO_DOUBLE ||
				meta->type == SIO_PROTO_STRING) {
			uint32_t str_len = meta->value.string.len;
			_sio_proto_adjust_byte_order((char *)&str_len, sizeof(str_len));
			memcpy(body_builder, &str_len, sizeof(str_len));
			body_builder += 4;
			memcpy(body_builder, meta->value.string.buf, meta->value.string.len);
			body_builder += meta->value.string.len;
		}
		meta = meta->next;
	}
}

static void _sio_proto_wmessage_add_meta(struct sio_proto_wmessage *msg, struct sio_proto_meta *meta)
{
	if (msg->meta_tail)
		msg->meta_tail->next = meta;
	msg->meta_tail = meta;
	if (!msg->meta_head)
		msg->meta_head = meta;

	msg->header.meta_len += 5;

	if (meta->type == SIO_PROTO_BOOLEAN)
		msg->header.body_len += 1;
	else if (meta->type == SIO_PROTO_INT32 || meta->type == SIO_PROTO_UINT32)
		msg->header.body_len += 4;
	else if (meta->type == SIO_PROTO_INT64 || meta->type == SIO_PROTO_UINT64)
		msg->header.body_len += 8;
	else if (meta->type == SIO_PROTO_FLOAT || meta->type == SIO_PROTO_DOUBLE ||
			meta->type == SIO_PROTO_STRING)
		msg->header.body_len  = msg->header.body_len + 4 + meta->value.string.len;
}

void sio_proto_wmessage_put_boolean(struct sio_proto_wmessage *msg, uint8_t boolean)
{
	struct sio_proto_meta *meta = calloc(1, sizeof(*meta));
	meta->type = SIO_PROTO_BOOLEAN;
	meta->value.boolean = boolean;
	_sio_proto_wmessage_add_meta(msg, meta);
}

void sio_proto_wmessage_put_int32(struct sio_proto_wmessage *msg, int32_t i32)
{
	struct sio_proto_meta *meta = calloc(1, sizeof(*meta));
	meta->type = SIO_PROTO_INT32;
	meta->value.i32 = i32;
	_sio_proto_wmessage_add_meta(msg, meta);
}

void sio_proto_wmessage_put_uint32(struct sio_proto_wmessage *msg, uint32_t u32)
{
	struct sio_proto_meta *meta = calloc(1, sizeof(*meta));
	meta->type = SIO_PROTO_UINT32;
	meta->value.u32 = u32;
	_sio_proto_wmessage_add_meta(msg, meta);
}

void sio_proto_wmessage_put_int64(struct sio_proto_wmessage *msg, int64_t i64)
{
	struct sio_proto_meta *meta = calloc(1, sizeof(*meta));
	meta->type = SIO_PROTO_INT64;
	meta->value.i64 = i64;
	_sio_proto_wmessage_add_meta(msg, meta);
}

void sio_proto_wmessage_put_uint64(struct sio_proto_wmessage *msg, int64_t u64)
{
	struct sio_proto_meta *meta = calloc(1, sizeof(*meta));
	meta->type = SIO_PROTO_UINT64;
	meta->value.u64 = u64;
	_sio_proto_wmessage_add_meta(msg, meta);
}

void sio_proto_wmessage_put_float(struct sio_proto_wmessage *msg, float f)
{
	char str_float[1024];
	uint32_t len = snprintf(str_float, sizeof(str_float), "%f", f);
	if (len >= sizeof(str_float))
		len = sizeof(str_float) - 1;
	char *dup = strdup(str_float);

	struct sio_proto_meta *meta = calloc(1, sizeof(*meta));
	meta->type = SIO_PROTO_FLOAT;
	meta->value.string.buf = dup;
	meta->value.string.len = len; /* 不含NULL字符 */
	_sio_proto_wmessage_add_meta(msg, meta);
}

void sio_proto_wmessage_put_double(struct sio_proto_wmessage *msg, double d)
{
	char str_double[1024];
	uint32_t len = snprintf(str_double, sizeof(str_double), "%lf", d);
	if (len >= sizeof(str_double))
		len = sizeof(str_double) - 1;
	char *dup = strdup(str_double);
	struct sio_proto_meta *meta = calloc(1, sizeof(*meta));
	meta->type = SIO_PROTO_DOUBLE;
	meta->value.string.buf = dup;
	meta->value.string.len = len;
	_sio_proto_wmessage_add_meta(msg, meta);
}

void sio_proto_wmessage_put_string(struct sio_proto_wmessage *msg, const char *string, uint32_t len)
{
	struct sio_proto_meta *meta = calloc(1, sizeof(*meta));
	meta->type = SIO_PROTO_STRING;
	char *dup = malloc(len);
	memcpy(dup, string, len);
	meta->value.string.buf = dup;
	meta->value.string.len = len;
	_sio_proto_wmessage_add_meta(msg, meta);
}

void sio_proto_rmessage_create(struct sio_proto_rmessage *msg)
{
	memset(msg, 0, sizeof(*msg));
}

void sio_proto_rmessage_destroy(struct sio_proto_rmessage *msg)
{
    struct sio_proto_meta *meta = msg->meta_arr;

    uint32_t i;
    for (i = 0; i < msg->meta_num; ++i) {
        if (meta[i].type == SIO_PROTO_STRING)
            free(meta[i].value.string.buf);
    }
    free(msg->meta_arr);
}

uint64_t sio_proto_rmessage_get_id(struct sio_proto_rmessage *msg)
{
    return msg->header.id;
}

uint32_t sio_proto_rmessage_get_command(struct sio_proto_rmessage *msg)
{
    return msg->header.command;
}

int sio_proto_rmessage_unserialize(struct sio_proto_rmessage *msg, const char *buf, uint32_t len)
{
    if (len < 20)
        return 0; /* header不完整 */

    uint32_t meta_len;
    memcpy(&meta_len, buf + 12, 4);
    _sio_proto_adjust_byte_order((char *)&meta_len, sizeof(meta_len));
    if (meta_len % 5)
        return -1; /* meta长度不是5字节的整数倍(1字节type+4字节offset) */

    uint32_t body_len;
    memcpy(&body_len, buf + 16, 4);
    _sio_proto_adjust_byte_order((char *)&body_len, sizeof(body_len));

    uint32_t msg_len = 20 + meta_len + body_len; /* 可能溢出, 但不影响安全性 */
    if (msg_len > len)
        return 0;   /* meta + body 不完整 */

    uint64_t id;
    memcpy(&id, buf, 8);
    _sio_proto_adjust_byte_order((char *)&id, sizeof(id));

    uint32_t command;
    memcpy(&command, buf + 8, 4);
    _sio_proto_adjust_byte_order((char *)&command, sizeof(command));

    const char *meta_reader = buf + 20;
    const char *body_begin = buf + 20 + meta_len;
    const char *body_reader = body_begin;
    const char *body_end = body_reader + body_len;

    uint32_t meta_num = meta_len / 5;
    struct sio_proto_meta *meta_arr = NULL;
    if (meta_num)
        meta_arr = calloc(meta_num, sizeof(*meta_arr));
    uint32_t i;
    for (i = 0; i < meta_num; ++i) {
        uint8_t type = *meta_reader++;
        uint32_t offset;
        memcpy(&offset, meta_reader, 4);
        _sio_proto_adjust_byte_order((char *)&offset, sizeof(offset));
        if (offset != body_reader - body_begin)
            break;
        meta_reader += 4;
        if (type == SIO_PROTO_BOOLEAN) {
            if (body_reader + 1 > body_end)
                break;
            meta_arr[i].type = SIO_PROTO_BOOLEAN;
            meta_arr[i].value.boolean = *body_reader++;
        } else if (type == SIO_PROTO_INT32) {
            if (body_reader + 4 > body_end)
                break;
            meta_arr[i].type = SIO_PROTO_INT32;
            int32_t i32;
            memcpy(&i32, body_reader, 4);
            _sio_proto_adjust_byte_order((char *)&i32, sizeof(i32));
            meta_arr[i].value.i32 = i32;
            body_reader += 4;
        } else if (type == SIO_PROTO_UINT32) {
            if (body_reader + 4 > body_end)
                break;
            meta_arr[i].type = SIO_PROTO_UINT32;
            uint32_t u32;
            memcpy(&u32, body_reader, 4);
            _sio_proto_adjust_byte_order((char *)&u32, sizeof(u32));
            meta_arr[i].value.u32 = u32;
            body_reader += 4;
        } else if (type == SIO_PROTO_INT64) {
            if (body_reader + 8 > body_end)
                break;
            meta_arr[i].type = SIO_PROTO_INT64;
            int64_t i64;
            memcpy(&i64, body_reader, 8);
            _sio_proto_adjust_byte_order((char *)&i64, sizeof(i64));
            meta_arr[i].value.i64 = i64;
            body_reader += 8;
        } else if (type == SIO_PROTO_UINT64) {
            if (body_reader + 8 > body_end)
                break;
            meta_arr[i].type = SIO_PROTO_UINT64;
            uint64_t u64;
            memcpy(&u64, body_reader, 8);
            _sio_proto_adjust_byte_order((char *)&u64, sizeof(u64));
            meta_arr[i].value.u64 = u64;
            body_reader += 8;
        } else if (type == SIO_PROTO_FLOAT) {
            if (body_reader + 4 > body_end)
                break;
            meta_arr[i].type = SIO_PROTO_FLOAT;
            uint32_t str_len;
            memcpy(&str_len, body_reader, 4);
            _sio_proto_adjust_byte_order((char *)&str_len, sizeof(str_len));
            body_reader += 4;
            char str_float[1024];
            if (str_len >= 1024)
                break;
            if (body_reader + str_len > body_end)
                break;
            memcpy(str_float, body_reader, str_len);
            str_float[str_len] = '\0';
            meta_arr[i].value.f = strtof(str_float, NULL);
            body_reader += str_len;
        } else if (type == SIO_PROTO_DOUBLE) {
            if (body_reader + 4 > body_end)
                break;
            meta_arr[i].type = SIO_PROTO_DOUBLE;
            uint32_t str_len;
            memcpy(&str_len, body_reader, 4);
            _sio_proto_adjust_byte_order((char *)&str_len, sizeof(str_len));
            body_reader += 4;
            char str_double[1024];
            if (str_len >= 1024)
                break;
            if (body_reader + str_len > body_end)
                break;
            memcpy(str_double, body_reader, str_len);
            str_double[str_len] = '\0';
            meta_arr[i].value.d = strtod(str_double, NULL);
            body_reader += str_len;
        } else if (type == SIO_PROTO_STRING) {
            if (body_reader + 4 > body_end)
                break;
            meta_arr[i].type = SIO_PROTO_STRING;
            uint32_t str_len;
            memcpy(&str_len, body_reader, 4);
            _sio_proto_adjust_byte_order((char *)&str_len, sizeof(str_len));
            body_reader += 4;
            if (body_reader + str_len > body_end)
                break;
            char *str = malloc(str_len + 1);
            memcpy(str, body_reader, str_len);
            str[str_len] = '\0';
            meta_arr[i].value.string.buf = str;
            meta_arr[i].value.string.len = str_len;
            body_reader += str_len;
        }
    }
    if (i != meta_num || body_reader != body_end) {
        for (i = 0; i < meta_num; ++i) {
            if (meta_arr[i].type == SIO_PROTO_STRING)
                free(meta_arr[i].value.string.buf);
        }
        free(meta_arr);
        return -1;
    }
    msg->header.id = id;
    msg->header.command = command;
    msg->header.meta_len = meta_len;
    msg->header.body_len = body_len;
    msg->meta_num = meta_num;
    msg->meta_arr = meta_arr;
	return msg_len;
}

int sio_proto_rmessage_get_boolean(struct sio_proto_rmessage *msg, uint32_t index, uint8_t *boolean)
{
    if (index >= msg->meta_num || msg->meta_arr[index].type != SIO_PROTO_BOOLEAN)
        return -1;
    *boolean = msg->meta_arr[index].value.boolean;
    return 0;
}

int sio_proto_rmessage_get_int32(struct sio_proto_rmessage *msg, uint32_t index, int32_t *i32)
{
    if (index >= msg->meta_num || msg->meta_arr[index].type != SIO_PROTO_INT32)
        return -1;
    *i32 = msg->meta_arr[index].value.i32;
    return 0;
}

int sio_proto_rmessage_get_uint32(struct sio_proto_rmessage *msg, uint32_t index, uint32_t *u32)
{
    if (index >= msg->meta_num || msg->meta_arr[index].type != SIO_PROTO_UINT32)
        return -1;
    *u32 = msg->meta_arr[index].value.u32;
    return 0;
}

int sio_proto_rmessage_get_int64(struct sio_proto_rmessage *msg, uint32_t index, int64_t *i64)
{
    if (index >= msg->meta_num || msg->meta_arr[index].type != SIO_PROTO_INT64)
        return -1;
    *i64 = msg->meta_arr[index].value.i64;
    return 0;
}

int sio_proto_rmessage_get_uint64(struct sio_proto_rmessage *msg, uint32_t index, uint64_t *u64)
{
    if (index >= msg->meta_num || msg->meta_arr[index].type != SIO_PROTO_UINT64)
        return -1;
    *u64 = msg->meta_arr[index].value.u64;
    return 0;
}

int sio_proto_rmessage_get_float(struct sio_proto_rmessage *msg, uint32_t index, float *f)
{
    if (index >= msg->meta_num || msg->meta_arr[index].type != SIO_PROTO_FLOAT)
        return -1;
    *f = msg->meta_arr[index].value.f;
    return 0;
}

int sio_proto_rmessage_get_double(struct sio_proto_rmessage *msg, uint32_t index, double *d)
{
    if (index >= msg->meta_num || msg->meta_arr[index].type != SIO_PROTO_DOUBLE)
        return -1;
    *d = msg->meta_arr[index].value.d;
    return 0;
}

int sio_proto_rmessage_get_string(struct sio_proto_rmessage *msg, uint32_t index, const char **string, uint32_t *len)
{
    if (index >= msg->meta_num || msg->meta_arr[index].type != SIO_PROTO_STRING)
        return -1;
    *string = msg->meta_arr[index].value.string.buf;
    *len = msg->meta_arr[index].value.string.len;
    return 0;
}


/* vim: set ts=4 sw=4 sts=4 tw=100 */
