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

static void _sio_proto_wmessage_fill_bytes_adjust(char **dest, char *src, uint32_t len)
{
    _sio_proto_adjust_byte_order(src, len);
    memcpy(*dest, src, len);
    _sio_proto_adjust_byte_order(src, len);
    *dest += len;
}

static void _sio_proto_wmessage_fill_bytes_not_adjust(char **dest, char *src, uint32_t len)
{
    memcpy(*dest, src, len);
    *dest += len;
}

static uint32_t _sio_proto_calculate_checksum(const char *buf, uint32_t len)
{
    uint32_t checksum = 0;
    uint32_t i;
    for (i = 0; i < len; ++i)
        checksum ^= buf[i];
    return checksum;
}

void sio_proto_wmessage_serialize(struct sio_proto_wmessage *msg, char **buf, uint32_t *len)
{
    *len = 24 + msg->header.meta_len + msg->header.body_len;
    *buf = malloc(*len);
    char *builder = *buf + 4;

    _sio_proto_wmessage_fill_bytes_adjust(&builder, (char *)&msg->header.id, sizeof(msg->header.id));
    _sio_proto_wmessage_fill_bytes_adjust(&builder, (char *)&msg->header.command, sizeof(msg->header.command));
    _sio_proto_wmessage_fill_bytes_adjust(&builder, (char *)&msg->header.meta_len, sizeof(msg->header.meta_len));
    _sio_proto_wmessage_fill_bytes_adjust(&builder, (char *)&msg->header.body_len, sizeof(msg->header.body_len));

	char *body_builder = builder + msg->header.meta_len;

	struct sio_proto_meta *meta = msg->meta_head;
	while (meta) {
		*builder++ = meta->type;

		if (meta->type == SIO_PROTO_INT8 || meta->type == SIO_PROTO_UINT8) {
		    _sio_proto_wmessage_fill_bytes_adjust(&body_builder, (char *)&meta->value, 1);
		} else if (meta->type == SIO_PROTO_INT16 || meta->type == SIO_PROTO_UINT16) {
			_sio_proto_wmessage_fill_bytes_adjust(&body_builder, (char *)&meta->value, 2);
		} else if (meta->type == SIO_PROTO_INT32 || meta->type == SIO_PROTO_UINT32) {
		    _sio_proto_wmessage_fill_bytes_adjust(&body_builder, (char *)&meta->value, 4);
		} else if (meta->type == SIO_PROTO_INT64 || meta->type == SIO_PROTO_UINT64) {
		    _sio_proto_wmessage_fill_bytes_adjust(&body_builder, (char *)&meta->value, 8);
		} else if (meta->type == SIO_PROTO_FLOAT || meta->type == SIO_PROTO_DOUBLE ||
				meta->type == SIO_PROTO_STRING) {
		    _sio_proto_wmessage_fill_bytes_adjust(&body_builder, (char *)&meta->value.string.len, 4);
		    _sio_proto_wmessage_fill_bytes_not_adjust(&body_builder, meta->value.string.buf, meta->value.string.len);
		}
		meta = meta->next;
	}
	uint32_t checksum = _sio_proto_calculate_checksum(*buf + 4, *len - 4);
	builder = *buf;
	_sio_proto_wmessage_fill_bytes_adjust(&builder, (char *)&checksum, 4);
}

static void _sio_proto_wmessage_add_meta(struct sio_proto_wmessage *msg, struct sio_proto_meta *meta)
{
	if (msg->meta_tail)
		msg->meta_tail->next = meta;
	msg->meta_tail = meta;
	if (!msg->meta_head)
		msg->meta_head = meta;

	msg->header.meta_len += 1;

	if (meta->type == SIO_PROTO_INT8 || meta->type == SIO_PROTO_UINT8)
		msg->header.body_len += 1;
	else if (meta->type == SIO_PROTO_INT16 || meta->type == SIO_PROTO_UINT16)
		msg->header.body_len += 2;
	else if (meta->type == SIO_PROTO_INT32 || meta->type == SIO_PROTO_UINT32)
		msg->header.body_len += 4;
	else if (meta->type == SIO_PROTO_INT64 || meta->type == SIO_PROTO_UINT64)
		msg->header.body_len += 8;
	else if (meta->type == SIO_PROTO_FLOAT || meta->type == SIO_PROTO_DOUBLE ||
			meta->type == SIO_PROTO_STRING)
		msg->header.body_len  += 4 + meta->value.string.len;
}

void sio_proto_wmessage_put_int8(struct sio_proto_wmessage *msg, int8_t i8)
{
    struct sio_proto_meta *meta = calloc(1, sizeof(*meta));
    meta->type = SIO_PROTO_INT8;
    meta->value.i8 = i8;
    _sio_proto_wmessage_add_meta(msg, meta);
}

void sio_proto_wmessage_put_uint8(struct sio_proto_wmessage *msg, uint8_t u8)
{
    struct sio_proto_meta *meta = calloc(1, sizeof(*meta));
    meta->type = SIO_PROTO_UINT8;
    meta->value.u8 = u8;
    _sio_proto_wmessage_add_meta(msg, meta);
}

void sio_proto_wmessage_put_int16(struct sio_proto_wmessage *msg, int16_t i16)
{
    struct sio_proto_meta *meta = calloc(1, sizeof(*meta));
    meta->type = SIO_PROTO_INT16;
    meta->value.i16 = i16;
    _sio_proto_wmessage_add_meta(msg, meta);
}

void sio_proto_wmessage_put_uint16(struct sio_proto_wmessage *msg, uint16_t u16)
{
    struct sio_proto_meta *meta = calloc(1, sizeof(*meta));
    meta->type = SIO_PROTO_UINT16;
    meta->value.u16 = u16;
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
	meta->value.string.len = len; /* 不含\0字符 */
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
    for (i = 0; i < msg->header.meta_len; ++i) {
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

static void _sio_proto_rmessage_extract_bytes_adjust(const char **src, char *dest, uint32_t len)
{
    memcpy(dest, *src, len);
    _sio_proto_adjust_byte_order(dest, len);
    *src += len;
}

static int _sio_proto_rmessage_extract_string(const char **reader, uint32_t *len, uint32_t *left)
{
	_sio_proto_rmessage_extract_bytes_adjust(reader, (char *)len, 4);
	*left -= 4;
	if (*len <= *left && *len > 0)
		return 0;
	return -1;
}

static int _sio_proto_rmessage_build_with_meta(uint32_t meta_len, struct sio_proto_meta *meta_arr,
		const char *reader, uint32_t body_len)
{
	uint32_t i;
	for (i = 0; i < meta_len; ++i) {
		if ((meta_arr[i].type == SIO_PROTO_INT8 || meta_arr[i].type == SIO_PROTO_UINT8) && body_len >= 1) {
			_sio_proto_rmessage_extract_bytes_adjust(&reader, (char *)&meta_arr[i].value, 1);
			body_len -= 1;
		} else if ((meta_arr[i].type == SIO_PROTO_INT16 || meta_arr[i].type == SIO_PROTO_UINT16) && body_len >= 2) {
			_sio_proto_rmessage_extract_bytes_adjust(&reader, (char *)&meta_arr[i].value, 2);
			body_len -= 2;
		} else if ((meta_arr[i].type == SIO_PROTO_INT32 || meta_arr[i].type == SIO_PROTO_UINT32) && body_len >= 4) {
			_sio_proto_rmessage_extract_bytes_adjust(&reader, (char *)&meta_arr[i].value, 4);
			body_len -= 4;
		} else if ((meta_arr[i].type == SIO_PROTO_INT64 || meta_arr[i].type == SIO_PROTO_UINT64) && body_len >= 8) {
			_sio_proto_rmessage_extract_bytes_adjust(&reader, (char *)&meta_arr[i].value, 8);
			body_len -= 8;
		} else if ((meta_arr[i].type == SIO_PROTO_FLOAT || meta_arr[i].type == SIO_PROTO_DOUBLE || meta_arr[i].type == SIO_PROTO_STRING ) && body_len >= 4) {
			uint32_t str_len;
			if (_sio_proto_rmessage_extract_string(&reader, &str_len, &body_len) == -1)
				break;
			if (meta_arr[i].type == SIO_PROTO_FLOAT && str_len < 1024) {
				char strf[1024] = {0};
				memcpy(strf, reader, str_len);
				meta_arr[i].value.f = strtod(reader, NULL); /* strtof only exists in C99 */
			} else if (meta_arr[i].type == SIO_PROTO_DOUBLE && str_len < 1024) {
				char strd[1024] = {0};
				memcpy(strd, reader, str_len);
				meta_arr[i].value.d = strtod(reader, NULL);
			} else if (meta_arr[i].type == SIO_PROTO_STRING) {
				meta_arr[i].value.string.len = str_len;
				meta_arr[i].value.string.buf = malloc(str_len);
				memcpy(meta_arr[i].value.string.buf, reader, str_len);
			} else {
				break;
			}
			reader += str_len;
			body_len -= str_len;
		} else {
			break;
		}
	}
	if (i != meta_len || body_len != 0)
		return -1;
	return 0;
}

int sio_proto_rmessage_unserialize(struct sio_proto_rmessage *msg, const char *buf, uint32_t len)
{
    if (len < 24)
        return 0;

    const char *reader = buf;

    uint32_t checksum;
    _sio_proto_rmessage_extract_bytes_adjust(&reader, (char *)&checksum, sizeof(checksum));
    uint64_t id;
    _sio_proto_rmessage_extract_bytes_adjust(&reader, (char *)&id, sizeof(id));
    uint32_t command;
    _sio_proto_rmessage_extract_bytes_adjust(&reader, (char *)&command, sizeof(command));
    uint32_t meta_len;
    _sio_proto_rmessage_extract_bytes_adjust(&reader, (char *)&meta_len, sizeof(meta_len));
    uint32_t body_len;
    _sio_proto_rmessage_extract_bytes_adjust(&reader, (char *)&body_len, sizeof(body_len));

    uint32_t msg_len = 24 + meta_len + body_len;
    if (msg_len > len)
        return 0;

    uint32_t calc_checksum = _sio_proto_calculate_checksum(buf + 4, msg_len - 4);
    if (calc_checksum != checksum)
        return -1;

    struct sio_proto_meta *meta_arr = calloc(meta_len, sizeof(*meta_arr));
    uint32_t meta_ndx;
    for (meta_ndx = 0; meta_ndx < meta_len; ++meta_ndx) {
    	meta_arr[meta_ndx].type = *reader++;
    }

    if (_sio_proto_rmessage_build_with_meta(meta_len, meta_arr, reader, body_len) == -1) {
    	for (meta_ndx = 0; meta_ndx < meta_len; ++meta_ndx) {
            if (meta_arr[meta_ndx].type == SIO_PROTO_STRING)
                free(meta_arr[meta_ndx].value.string.buf); /* calloc保证初始化为NULL */
        }
        free(meta_arr);
        return -1;
    }
    msg->header.id = id;
    msg->header.command = command;
    msg->header.meta_len = meta_len;
    msg->header.body_len = body_len;
    msg->meta_arr = meta_arr;
    return msg_len;
}

int sio_proto_rmessage_get_int8(struct sio_proto_rmessage *msg, uint32_t index, int8_t *i8)
{
    if (index >= msg->header.meta_len || msg->meta_arr[index].type != SIO_PROTO_INT8)
        return -1;
    *i8 = msg->meta_arr[index].value.i8;
    return 0;
}
int sio_proto_rmessage_get_uint8(struct sio_proto_rmessage *msg, uint32_t index, uint8_t *u8)
{
    if (index >= msg->header.meta_len || msg->meta_arr[index].type != SIO_PROTO_UINT8)
        return -1;
    *u8 = msg->meta_arr[index].value.u8;
    return 0;
}
int sio_proto_rmessage_get_int16(struct sio_proto_rmessage *msg, uint32_t index, int16_t *i16)
{
    if (index >= msg->header.meta_len || msg->meta_arr[index].type != SIO_PROTO_INT16)
        return -1;
    *i16 = msg->meta_arr[index].value.i16;
    return 0;
}
int sio_proto_rmessage_get_uint16(struct sio_proto_rmessage *msg, uint32_t index, uint16_t *u16)
{
    if (index >= msg->header.meta_len || msg->meta_arr[index].type != SIO_PROTO_UINT16)
        return -1;
    *u16 = msg->meta_arr[index].value.u16;
    return 0;
}

int sio_proto_rmessage_get_int32(struct sio_proto_rmessage *msg, uint32_t index, int32_t *i32)
{
    if (index >= msg->header.meta_len || msg->meta_arr[index].type != SIO_PROTO_INT32)
        return -1;
    *i32 = msg->meta_arr[index].value.i32;
    return 0;
}

int sio_proto_rmessage_get_uint32(struct sio_proto_rmessage *msg, uint32_t index, uint32_t *u32)
{
    if (index >= msg->header.meta_len || msg->meta_arr[index].type != SIO_PROTO_UINT32)
        return -1;
    *u32 = msg->meta_arr[index].value.u32;
    return 0;
}

int sio_proto_rmessage_get_int64(struct sio_proto_rmessage *msg, uint32_t index, int64_t *i64)
{
    if (index >= msg->header.meta_len || msg->meta_arr[index].type != SIO_PROTO_INT64)
        return -1;
    *i64 = msg->meta_arr[index].value.i64;
    return 0;
}

int sio_proto_rmessage_get_uint64(struct sio_proto_rmessage *msg, uint32_t index, uint64_t *u64)
{
    if (index >= msg->header.meta_len || msg->meta_arr[index].type != SIO_PROTO_UINT64)
        return -1;
    *u64 = msg->meta_arr[index].value.u64;
    return 0;
}

int sio_proto_rmessage_get_float(struct sio_proto_rmessage *msg, uint32_t index, float *f)
{
    if (index >= msg->header.meta_len|| msg->meta_arr[index].type != SIO_PROTO_FLOAT)
        return -1;
    *f = msg->meta_arr[index].value.f;
    return 0;
}

int sio_proto_rmessage_get_double(struct sio_proto_rmessage *msg, uint32_t index, double *d)
{
    if (index >= msg->header.meta_len || msg->meta_arr[index].type != SIO_PROTO_DOUBLE)
        return -1;
    *d = msg->meta_arr[index].value.d;
    return 0;
}

int sio_proto_rmessage_get_string(struct sio_proto_rmessage *msg, uint32_t index, const char **string, uint32_t *len)
{
    if (index >= msg->header.meta_len || msg->meta_arr[index].type != SIO_PROTO_STRING)
        return -1;
    *string = msg->meta_arr[index].value.string.buf;
    *len = msg->meta_arr[index].value.string.len;
    return 0;
}


/* vim: set ts=4 sw=4 sts=4 tw=100 */
