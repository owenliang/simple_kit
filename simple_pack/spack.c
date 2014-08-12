/*
 * Copyright (C) 2014-2015  liangdong <liangdong01@baidu.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdlib.h>
#include <string.h>
#include "spack.h"

enum spack_format {
	SPACK_FORMAT_NIL = 0xC0,
	SPACK_FORMAT_NERVER_USED = 0xC1,
	SPACK_FORMAT_FALSE = 0xC2,
	SPACK_FORMAT_TRUE = 0xC3,
	SPACK_FORMAT_BIN8 = 0xC4,
	SPACK_FORMAT_BIN16 = 0xC5,
	SPACK_FORMAT_BIN32 = 0xC6,
	SPACK_FORMAT_EXT8 = 0xC7,
	SPACK_FORMAT_EXT16 = 0xC8,
	SPACK_FORMAT_EXT32 = 0xC9,
	SPACK_FORMAT_FLOAT32 = 0xCA,
	SPACK_FORMAT_FLOAT64 = 0xCB,
	SPACK_FORMAT_UINT8 = 0xCC,
	SPACK_FORMAT_UINT16 = 0xCD,
	SPACK_FORMAT_UINT32 = 0xCE,
	SPACK_FORMAT_UINT64 = 0xCF,
	SPACK_FORMAT_INT8 = 0xD0,
	SPACK_FORMAT_INT16 = 0xD1,
	SPACK_FORMAT_INT32 = 0xD2,
	SPACK_FORMAT_INT64 = 0xD3,
	SPACK_FORMAT_FIXEXT1 = 0xD4,
	SPACK_FORMAT_FIXEXT2 = 0xD5,
	SPACK_FORMAT_FIXEXT4 = 0xD6,
	SPACK_FORMAT_FIXEXT8 = 0xD7,
	SPACK_FORMAT_FIXEXT16 = 0xD8,
	SPACK_FORMAT_STR8 = 0xD9,
	SPACK_FORMAT_STR16 = 0xDA,
	SPACK_FORMAT_STR32 = 0xDB,
	SPACK_FORMAT_ARRAY16 = 0xDC,
	SPACK_FORMAT_ARRAY32 = 0xDD,
	SPACK_FORMAT_MAP16 = 0xDE,
	SPACK_FORMAT_MAP32 = 0xDF,
};

void spack_w_init(struct spack_w *wpack, char *buf, uint64_t buf_capacity)
{
	wpack->buf = buf;
	wpack->buf_used = 0;
	wpack->buf_capacity = buf_capacity;
}

uint64_t spack_w_size(struct spack_w *wpack)
{
	return wpack->buf_used;
}

void spack_r_init(struct spack_r *rpack, const char *buf, uint64_t buf_size)
{
	rpack->buf = buf;
	rpack->buf_used = 0;
	rpack->buf_size = buf_size;
}

static void _spack_reverse_bytes(char *bytes, uint32_t len)
{
	char *end = bytes + len - 1;
	while (bytes < end) {
		char temp = *bytes;
		*bytes = *end;
		*end = temp;
		++bytes, --end;
	}
}

static void _spack_check_bigendian(char *bytes, uint32_t len)
{
	union {
		char b;
		int n;
	} u;
	u.n = 1;
	if (u.b == 1)
		_spack_reverse_bytes(bytes, len);
}

static uint64_t _spack_w_space(struct spack_w *wpack)
{
	return wpack->buf_capacity - wpack->buf_used;
}

static void _spack_w_append(struct spack_w *wpack, const char *bytes, uint64_t len)
{
	memcpy(wpack->buf + wpack->buf_used, bytes, len);
	wpack->buf_used += len;
}

static void _spack_w_put_format(struct spack_w *wpack, char format)
{
	wpack->buf[wpack->buf_used++] = format;
}

int spack_put_nil(struct spack_w *wpack)
{
	if (_spack_w_space(wpack) < 1)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_NIL);
	return 0;
}

int spack_put_boolean(struct spack_w *wpack, uint8_t is_true)
{
	if (_spack_w_space(wpack) < 1)
		return -1;

	if (is_true)
		_spack_w_put_format(wpack, SPACK_FORMAT_TRUE);
	else
		_spack_w_put_format(wpack, SPACK_FORMAT_FALSE);
	return 0;
}

static int _spack_put_bin8(struct spack_w *wpack, const char *data, uint8_t len)
{
	if (_spack_w_space(wpack) < len + 2)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_BIN8);
	_spack_w_append(wpack, &len, sizeof(len));
	_spack_w_append(wpack, data, len);
	return 0;
}

static int _spack_put_bin16(struct spack_w *wpack, const char *data, uint16_t len)
{
	if (_spack_w_space(wpack) < len + 3)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_BIN16);
	_spack_check_bigendian((char *)&len, sizeof(len));
	_spack_w_append(wpack, (const char *)&len, sizeof(len));
	_spack_w_append(wpack, data, len);
	return 0;
}

static int _spack_put_bin32(struct spack_w *wpack, const char *data, uint32_t len)
{
	if (_spack_w_space(wpack) < len + 5)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_BIN32);
	_spack_check_bigendian((char *)&len, sizeof(len));
	_spack_w_append(wpack, (const char *)&len, sizeof(len));
	_spack_w_append(wpack, data, len);
	return 0;
}

int spack_put_bin(struct spack_w *wpack, const char *data, uint32_t len)
{
	if (len <= 0xFF)
		return _spack_put_bin8(wpack, data, len);
	else if (len <= 0xFFFF)
		return _spack_put_bin16(wpack, data, len);
	return _spack_put_bin32(wpack, data, len);
}

/*
 * 8位有符号数, 分3种存法
 *
 * 1, positive fixint	0xxxxxxx	0x00 - 0x7f
 * 2, negative fixint	111xxxxx	0xe0 - 0xff
 * 3, int 8	11010000	0xd0
 * */
static int _spack_put_int8(struct spack_w *wpack, int8_t i8)
{
	uint64_t space = _spack_w_space(wpack);
	if (i8 >= 0 || i8 >= -32) {
		if (space < 1)
			return -1;
	} else {
		if (space < 2)
			return -1;
		_spack_w_put_format(wpack, SPACK_FORMAT_INT8);
	}
	_spack_w_append(wpack, (const char *)&i8, sizeof(i8));
	return 0;
}

static int _spack_put_int16(struct spack_w *wpack, int16_t i16)
{
	if (_spack_w_space(wpack) < 3)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_INT16);
	_spack_check_bigendian((char *)&i16, sizeof(i16));
	_spack_w_append(wpack, (const char *)&i16, sizeof(i16));
	return 0;
}

static int _spack_put_int32(struct spack_w *wpack, int32_t i32)
{
	if (_spack_w_space(wpack) < 5)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_INT32);
	_spack_check_bigendian((char *)&i32, sizeof(i32));
	_spack_w_append(wpack, (const char *)&i32, sizeof(i32));
	return 0;
}

static int _spack_put_int64(struct spack_w *wpack, int64_t i64)
{
	if (_spack_w_space(wpack) < 9)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_INT64);
	_spack_check_bigendian((char *)&i64, sizeof(i64));
	_spack_w_append(wpack, (const char *)&i64, sizeof(i64));
	return 0;
}

int spack_put_int(struct spack_w *wpack, int64_t i64)
{
	if (i64 >= -128 && i64 <= 127)
		return _spack_put_int8(wpack, i64);
	else if (i64 >= -32768 && i64 <= 32767)
		return _spack_put_int16(wpack, i64);
	else if (i64 >= -2147483648 && i64 <= 2147483647)
		return _spack_put_int32(wpack, i64);
	return _spack_put_int64(wpack, i64);
}

static int _spack_put_uint8(struct spack_w *wpack, uint8_t u8)
{
	uint64_t space = _spack_w_space(wpack);
	if (u8 < 0x80) {
		if (space < 1)
			return -1;
	} else {
		if (space < 2)
			return -1;
		_spack_w_put_format(wpack, SPACK_FORMAT_UINT8);
	}
	_spack_w_append(wpack, (const char *)&u8, sizeof(u8));
	return 0;
}

static int _spack_put_uint16(struct spack_w *wpack, uint16_t u16)
{
	if (_spack_w_space(wpack) < 3)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_UINT16);
	_spack_check_bigendian((char *)&u16, sizeof(u16));
	_spack_w_append(wpack, (const char *)&u16, sizeof(u16));
	return 0;
}

static int _spack_put_uint32(struct spack_w *wpack, uint32_t u32)
{
	if (_spack_w_space(wpack) < 5)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_UINT32);
	_spack_check_bigendian((char *)&u32, sizeof(u32));
	_spack_w_append(wpack, (const char *)&u32, sizeof(u32));
	return 0;
}

static int _spack_put_uint64(struct spack_w *wpack, uint64_t u64)
{
	if (_spack_w_space(wpack) < 9)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_UINT64);
	_spack_check_bigendian((char *)&u64, sizeof(u64));
	_spack_w_append(wpack, (const char *)&u64, sizeof(u64));
	return 0;
}

int spack_put_uint(struct spack_w *wpack, uint64_t u64)
{
	if (u64 <= 127)
		return _spack_put_uint8(wpack, u64);
	else if (u64 <= 32767)
		return _spack_put_uint16(wpack, u64);
	else if (u64 <= 2147483647)
		return _spack_put_uint32(wpack, u64);
	return _spack_put_uint64(wpack, u64);
}

int spack_put_float(struct spack_w *wpack, float f)
{
	if (_spack_w_space(wpack) < 5)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_FLOAT32);
	_spack_check_bigendian((char *)&f, sizeof(f));
	_spack_w_append(wpack, (const char *)&f, sizeof(f));
	return 0;
}

int spack_put_double(struct spack_w *wpack, double d)
{
	if (_spack_w_space(wpack) < 9)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_FLOAT64);
	_spack_check_bigendian((char *)&d, sizeof(d));
	_spack_w_append(wpack, (const char *)&d, sizeof(d));
	return 0;
}

static int _spack_put_fixext1(struct spack_w *wpack, int8_t type, const char *data)
{
	if (_spack_w_space(wpack) < 3)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_FIXEXT1);
	_spack_w_append(wpack, (const char *)&type, sizeof(type));
	_spack_w_append(wpack, (const char *)data, 1);
	return 0;
}

static int _spack_put_fixext2(struct spack_w *wpack, int8_t type, const char *data)
{
	if (_spack_w_space(wpack) < 4)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_FIXEXT2);
	_spack_w_append(wpack, (const char *)&type, sizeof(type));
	_spack_w_append(wpack, (const char *)data, 2);
	return 0;
}

static int _spack_put_fixext4(struct spack_w *wpack, int8_t type, const char *data)
{
	if (_spack_w_space(wpack) < 6)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_FIXEXT4);
	_spack_w_append(wpack, (const char *)&type, sizeof(type));
	_spack_w_append(wpack, (const char *)data, 4);
	return 0;
}

static int _spack_put_fixext8(struct spack_w *wpack, int8_t type, const char *data)
{
	if (_spack_w_space(wpack) < 10)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_FIXEXT8);
	_spack_w_append(wpack, (const char *)&type, sizeof(type));
	_spack_w_append(wpack, (const char *)data, 8);
	return 0;
}

static int _spack_put_fixext16(struct spack_w *wpack, int8_t type, const char *data)
{
	if (_spack_w_space(wpack) < 18)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_FIXEXT16);
	_spack_w_append(wpack, (const char *)&type, sizeof(type));
	_spack_w_append(wpack, (const char *)data, 16);
	return 0;
}

static int _spack_put_ext8(struct spack_w *wpack, int8_t type, const char *data, uint8_t size)
{
	if (_spack_w_space(wpack) < 3 + size)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_EXT8);
	_spack_w_append(wpack, (const char *)&size, sizeof(size));
	_spack_w_append(wpack, (const char *)&type, sizeof(type));
	_spack_w_append(wpack, (const char *)data, size);
	return 0;
}

static int _spack_put_ext16(struct spack_w *wpack, int8_t type, const char *data, uint16_t size)
{
	if (_spack_w_space(wpack) < 4 + size)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_EXT16);
	_spack_check_bigendian((char *)&size, sizeof(size));
	_spack_w_append(wpack, (const char *)&size, sizeof(size));
	_spack_w_append(wpack, (const char *)&type, sizeof(type));
	_spack_w_append(wpack, (const char *)data, size);
	return 0;
}

static int _spack_put_ext32(struct spack_w *wpack, int8_t type, const char *data, uint32_t size)
{
	if (_spack_w_space(wpack) < 6 + size)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_EXT32);
	_spack_check_bigendian((char *)&size, sizeof(size));
	_spack_w_append(wpack, (const char *)&size, sizeof(size));
	_spack_w_append(wpack, (const char *)&type, sizeof(type));
	_spack_w_append(wpack, (const char *)data, size);
	return 0;
}

int spack_put_ext(struct spack_w *wpack, int8_t type, const void *data, uint32_t size)
{
	if (size == 1)
		return _spack_put_fixext1(wpack, type, data);
	else if (size == 2)
		return _spack_put_fixext2(wpack, type, data);
	else if (size == 4)
		return _spack_put_fixext4(wpack, type, data);
	else if (size == 8)
		return _spack_put_fixext8(wpack, type, data);
	else if (size == 16)
		return _spack_put_fixext16(wpack, type, data);
	else if (size <= 255)
		return _spack_put_ext8(wpack, type, data, size);
	else if (size <= 65535)
		return _spack_put_ext16(wpack, type, data, size);
	return _spack_put_ext32(wpack, type, data, size);
}

static int _spack_put_fixstr(struct spack_w *wpack, const char *data, uint8_t size)
{
	if (_spack_w_space(wpack) < 2 + size) /* 1B format + size B data + 1B NULL-terminal*/
		return -1;
	uint8_t format = 0xA0 | size;
	_spack_w_put_format(wpack, format);
	_spack_w_append(wpack, data, size);
	wpack->buf[wpack->buf_used++] = '\0';
	return 0;
}

static int _spack_put_str8(struct spack_w *wpack, const char *data, uint8_t size)
{
	if (_spack_w_space(wpack) < 3 + size)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_STR8);
	_spack_w_append(wpack, &size, sizeof(size));
	_spack_w_append(wpack, data, size);
	wpack->buf[wpack->buf_used++] = '\0';
	return 0;
}

static int _spack_put_str16(struct spack_w *wpack, const char *data, uint16_t size)
{
	if (_spack_w_space(wpack) < 3 + size)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_STR16);
	_spack_check_bigendian((char *)&size, sizeof(size));
	_spack_w_append(wpack, (const char *)&size, sizeof(size));
	_spack_w_append(wpack, data, size);
	wpack->buf[wpack->buf_used++] = '\0';
	return 0;
}

static int _spack_put_str32(struct spack_w *wpack, const char *data, uint32_t size)
{
	if (_spack_w_space(wpack) < 5 + size)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_STR32);
	_spack_check_bigendian((char *)&size, sizeof(size));
	_spack_w_append(wpack, (const char *)&size, sizeof(size));
	_spack_w_append(wpack, data, size);
	wpack->buf[wpack->buf_used++] = '\0';
	return 0;
}

int spack_put_str(struct spack_w *wpack, const char *data, uint32_t size)
{
	if (size <= 31)
		return _spack_put_fixstr(wpack, data, size);
	else if (size <= 255)
		return _spack_put_str8(wpack, data, size);
	else if (size <= 65535)
		return _spack_put_str16(wpack, data, size);
	return _spack_put_str32(wpack, data, size);
}

static int _spack_put_fixarray(struct spack_w *wpack, uint8_t size)
{
	if (_spack_w_space(wpack) < 1)
		return -1;
	uint8_t format = 0x90 | size;
	_spack_w_put_format(wpack, format);
	return 0;
}

static int _spack_put_array16(struct spack_w *wpack, uint16_t size)
{
	if (_spack_w_space(wpack) < 3)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_ARRAY16);
	_spack_check_bigendian((char *)&size, sizeof(size));
	_spack_w_append(wpack, (const char *)&size, sizeof(size));
	return 0;
}

static int _spack_put_array32(struct spack_w *wpack, uint32_t size)
{
	if (_spack_w_space(wpack) < 5)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_ARRAY32);
	_spack_check_bigendian((char *)&size, sizeof(size));
	_spack_w_append(wpack, (const char *)&size, sizeof(size));
	return 0;
}

int spack_put_array(struct spack_w *wpack, uint32_t size)
{
	if (size <= 15)
		return _spack_put_fixarray(wpack, size);
	else if (size <= 65535)
		return _spack_put_array16(wpack, size);
	return _spack_put_array32(wpack, size);
}

static int _spack_put_fixmap(struct spack_w *wpack, uint8_t size)
{
	if (_spack_w_space(wpack) < 1)
		return -1;
	uint8_t format = 0x80 | size;
	_spack_w_put_format(wpack, format);
	return 0;
}

static int _spack_put_map16(struct spack_w *wpack, uint16_t size)
{
	if (_spack_w_space(wpack) < 3)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_MAP16);
	_spack_check_bigendian((char *)&size, sizeof(size));
	_spack_w_append(wpack, (const char *)&size, sizeof(size));
	return 0;
}

static int _spack_put_map32(struct spack_w *wpack, uint32_t size)
{
	if (_spack_w_space(wpack) < 5)
		return -1;
	_spack_w_put_format(wpack, SPACK_FORMAT_MAP32);
	_spack_check_bigendian((char *)&size, sizeof(size));
	_spack_w_append(wpack, (const char *)&size, sizeof(size));
	return 0;
}

int spack_put_map(struct spack_w *wpack, uint32_t size)
{
	if (size <= 15)
		return _spack_put_fixmap(wpack, size);
	else if (size <= 65535)
		return _spack_put_map16(wpack, size);
	return _spack_put_map32(wpack, size);
}

static uint64_t _spack_r_space(struct spack_r *rpack)
{
	return rpack->buf_size - rpack->buf_used;
}

static uint8_t _spack_r_get_format(struct spack_r *rpack)
{
	return rpack->buf[rpack->buf_used++];
}

int spack_get_nil(struct spack_r *rpack)
{
	if (_spack_r_space(rpack) < 1)
		return -1;
	if (_spack_r_get_format(rpack) != SPACK_FORMAT_NIL)
		return -1;
	return 0;
}

int spack_get_boolean(struct spack_r *rpack, uint8_t *is_true)
{
	if (_spack_r_space(rpack) < 1)
		return -1;
	uint8_t format = _spack_r_get_format(rpack);
	if (format == SPACK_FORMAT_TRUE)
		*is_true = 1;
	else if (format == SPACK_FORMAT_FALSE)
		*is_true = 0;
	else
		return -1;
	return 0;
}

static void _spack_r_drain(struct spack_r *rpack, char *bytes, uint64_t len)
{
	memcpy(bytes, rpack->buf + rpack->buf_used, len);
	rpack->buf_used += len;
}

static int _spack_get_int8(struct spack_r *rpack, int64_t *i64, char check_positive)
{
	if (_spack_r_space(rpack) < 1)
		return -1;
	int8_t i8;
	_spack_r_drain(rpack, &i8, sizeof(i8));
	if (check_positive && i8 < 0)
		return -1;
	*i64 = i8;
	return 0;
}

static int _spack_get_int16(struct spack_r *rpack, int64_t *i64, char check_positive)
{
	if (_spack_r_space(rpack) < 2)
		return -1;
	int16_t i16;
	_spack_r_drain(rpack, (char *)&i16, sizeof(i16));
	_spack_check_bigendian((char *)&i16, sizeof(i16));
	if (check_positive && i16 < 0)
		return -1;
	*i64 = i16;
	return 0;
}

static int _spack_get_int32(struct spack_r *rpack, int64_t *i64, char check_positive)
{
	if (_spack_r_space(rpack) < 4)
		return -1;
	int32_t i32;
	_spack_r_drain(rpack, (char *)&i32, sizeof(i32));
	_spack_check_bigendian((char *)&i32, sizeof(i32));
	if (check_positive && i32 < 0)
		return -1;
	*i64 = i32;
	return 0;
}

static int _spack_get_int64(struct spack_r *rpack, int64_t *i64, char check_positive)
{
	if (_spack_r_space(rpack) < 8)
		return -1;
	_spack_r_drain(rpack, (char *)i64, sizeof(*i64));
	_spack_check_bigendian((char *)i64, sizeof(*i64));
	if (check_positive && *i64 < 0)
		return -1;
	return 0;
}

int spack_get_int(struct spack_r *rpack, int64_t *i64)
{
	if (_spack_r_space(rpack) < 1)
		return -1;
	uint8_t format = _spack_r_get_format(rpack);
	if (format <= 0x7F) /* positive fix int */
		*i64 = format;
	else if (format >= 0xE0) /* negative fix int */
		*i64 = (int8_t)format;
	else if (format == SPACK_FORMAT_INT8)
		return _spack_get_int8(rpack, i64, 0);
	else if (format == SPACK_FORMAT_INT16)
		return _spack_get_int16(rpack, i64, 0);
	else if (format == SPACK_FORMAT_INT32)
		return _spack_get_int32(rpack, i64, 0);
	else if (format == SPACK_FORMAT_INT64)
		return _spack_get_int64(rpack, i64, 0);
	else if (format == SPACK_FORMAT_UINT8)
		return _spack_get_int8(rpack, i64, 1);
	else if (format == SPACK_FORMAT_UINT16)
		return _spack_get_int16(rpack, i64, 1);
	else if (format == SPACK_FORMAT_UINT32)
		return _spack_get_int32(rpack, i64, 1);
	else if (format == SPACK_FORMAT_UINT64)
		return _spack_get_int64(rpack, i64, 1);
	else
		return -1;
	return 0;
}

static int _spack_get_uint8(struct spack_r *rpack, uint64_t *u64, char check_positive)
{
	if (_spack_r_space(rpack) < 1)
		return -1;
	uint8_t u8;
	_spack_r_drain(rpack, &u8, sizeof(u8));
	if (check_positive && (int8_t)u8 < 0)
		return -1;
	*u64 = u8;
	return 0;
}

static int _spack_get_uint16(struct spack_r *rpack, uint64_t *u64, char check_positive)
{
	if (_spack_r_space(rpack) < 2)
		return -1;
	uint16_t u16;
	_spack_r_drain(rpack, (char *)&u16, sizeof(u16));
	_spack_check_bigendian((char *)&u16, sizeof(u16));
	if (check_positive && (int16_t)u16 < 0)
		return -1;
	*u64 = u16;
	return 0;
}

static int _spack_get_uint32(struct spack_r *rpack, uint64_t *u64, char check_positive)
{
	if (_spack_r_space(rpack) < 4)
		return -1;
	uint32_t u32;
	_spack_r_drain(rpack, (char *)&u32, sizeof(u32));
	_spack_check_bigendian((char *)&u32, sizeof(u32));
	if (check_positive && (int32_t)u32 < 0)
		return -1;
	*u64 = u32;
	return 0;
}

static int _spack_get_uint64(struct spack_r *rpack, uint64_t *u64, char check_positive)
{
	if (_spack_r_space(rpack) < 8)
		return -1;
	_spack_r_drain(rpack, (char *)u64, sizeof(*u64));
	_spack_check_bigendian((char *)u64, sizeof(*u64));
	if (check_positive && (int64_t)(*u64) < 0)
		return -1;
	return 0;
}

int spack_get_uint(struct spack_r *rpack, uint64_t *u64)
{
	if (_spack_r_space(rpack) < 1)
		return -1;
	uint8_t format = _spack_r_get_format(rpack);
	if (format <= 0x7F) /* positive fix int*/
		*u64 = format;
	else if (format == SPACK_FORMAT_UINT8)
		return _spack_get_uint8(rpack, u64, 0);
	else if (format == SPACK_FORMAT_UINT16)
		return _spack_get_uint16(rpack, u64, 0);
	else if (format == SPACK_FORMAT_UINT32)
		return _spack_get_uint32(rpack, u64, 0);
	else if (format == SPACK_FORMAT_UINT64)
		return _spack_get_uint64(rpack, u64, 0);
	/* must be negative
	else if (format == SPACK_FORMAT_INT8)
		return _spack_get_uint8(rpack, u64, 1); */
	else if (format == SPACK_FORMAT_INT16)
		return _spack_get_uint16(rpack, u64, 1);
	else if (format == SPACK_FORMAT_INT32)
		return _spack_get_uint32(rpack, u64, 1);
	else if (format == SPACK_FORMAT_INT64)
		return _spack_get_uint64(rpack, u64, 1);
	else
		return -1;
	return 0;
}

static int _spack_get_bin8(struct spack_r *rpack, const char **data, uint32_t *len)
{
	uint64_t space = _spack_r_space(rpack);
	if (space < 1)
		return -1;
	uint8_t data_len;
	_spack_r_drain(rpack, &data_len, sizeof(data_len));
	if (space < 1 + data_len)
		return -1;
	*len = data_len;
	*data = rpack->buf + rpack->buf_used;
	rpack->buf_used += data_len;
	return 0;
}

static int _spack_get_bin16(struct spack_r *rpack, const char **data, uint32_t *len)
{
	uint64_t space = _spack_r_space(rpack);
	if (space < 2)
		return -1;
	uint16_t data_len;
	_spack_r_drain(rpack, (char *)&data_len, sizeof(data_len));
	if (space < 2 + data_len)
		return -1;
	*len = data_len;
	*data = rpack->buf + rpack->buf_used;
	rpack->buf_used += data_len;
	return 0;
}

static int _spack_get_bin32(struct spack_r *rpack, const char **data, uint32_t *len)
{
	uint64_t space = _spack_r_space(rpack);
	if (space < 4)
		return -1;
	_spack_r_drain(rpack, (char *)len, sizeof(*len));
	if (space < 4 + *len)
		return -1;
	*data = rpack->buf + rpack->buf_used;
	rpack->buf_used += *len;
	return 0;
}

int spack_get_bin(struct spack_r *rpack, const char **data, uint32_t *len)
{
	if (_spack_r_space(rpack) < 1)
		return -1;
	uint8_t format = _spack_r_get_format(rpack);
	if (format == SPACK_FORMAT_BIN8)
		return _spack_get_bin8(rpack, data, len);
	else if (format == SPACK_FORMAT_BIN16)
		return _spack_get_bin16(rpack, data, len);
	else if (format == SPACK_FORMAT_BIN32)
		return _spack_get_bin32(rpack, data, len);
	return -1;
}

int spack_get_float(struct spack_r *rpack, float *f)
{
	if (_spack_r_space(rpack) < 5)
		return -1;
	uint8_t format = _spack_r_get_format(rpack);
	if (format != SPACK_FORMAT_FLOAT32)
		return -1;
	_spack_r_drain(rpack, (char *)f, sizeof(*f));
	_spack_check_bigendian((char *)f, sizeof(*f));
	return 0;
}

int spack_get_double(struct spack_r *rpack, double *d)
{
	if (_spack_r_space(rpack) < 9)
		return -1;
	uint8_t format = _spack_r_get_format(rpack);
	if (format != SPACK_FORMAT_FLOAT64)
		return -1;
	_spack_r_drain(rpack, (char *)d, sizeof(*d));
	_spack_check_bigendian((char *)d, sizeof(*d));
	return 0;
}

static int _spack_get_fixext1(struct spack_r *rpack, int8_t *type, const char **data, uint32_t *size)
{
	uint64_t space = _spack_r_space(rpack);
	if (space < 2)
		return -1;
	_spack_r_drain(rpack, (char *)type, sizeof(*type));
	*data = rpack->buf + rpack->buf_used;
	rpack->buf_used += 1;
	return 0;
}

static int _spack_get_fixext2(struct spack_r *rpack, int8_t *type, const char **data, uint32_t *size)
{
	uint64_t space = _spack_r_space(rpack);
	if (space < 3)
		return -1;
	_spack_r_drain(rpack, (char *)type, sizeof(*type));
	*data = rpack->buf + rpack->buf_used;
	rpack->buf_used += 2;
	return 0;
}

static int _spack_get_fixext4(struct spack_r *rpack, int8_t *type, const char **data, uint32_t *size)
{
	uint64_t space = _spack_r_space(rpack);
	if (space < 5)
		return -1;
	_spack_r_drain(rpack, (char *)type, sizeof(*type));
	*data = rpack->buf + rpack->buf_used;
	rpack->buf_used += 4;
	return 0;
}

static int _spack_get_fixext8(struct spack_r *rpack, int8_t *type, const char **data, uint32_t *size)
{
	uint64_t space = _spack_r_space(rpack);
	if (space < 9)
		return -1;
	_spack_r_drain(rpack, (char *)type, sizeof(*type));
	*data = rpack->buf + rpack->buf_used;
	rpack->buf_used += 8;
	return 0;
}

static int _spack_get_fixext16(struct spack_r *rpack, int8_t *type, const char **data, uint32_t *size)
{
	uint64_t space = _spack_r_space(rpack);
	if (space < 17)
		return -1;
	_spack_r_drain(rpack, (char *)type, sizeof(*type));
	*data = rpack->buf + rpack->buf_used;
	rpack->buf_used += 16;
	return 0;
}

static int _spack_get_ext8(struct spack_r *rpack, int8_t *type, const char **data, uint32_t *size)
{
	uint64_t space = _spack_r_space(rpack);
	if (space < 2)
		return -1;
	uint8_t data_len;
	_spack_r_drain(rpack, (char *)&data_len, sizeof(data_len));
	_spack_r_drain(rpack, (char *)type, sizeof(*type));
	if (space < 2 + data_len)
		return -1;
	*data = rpack->buf + rpack->buf_used;
	*size = data_len;
	rpack->buf_used += data_len;
	return 0;
}

static int _spack_get_ext16(struct spack_r *rpack, int8_t *type, const char **data, uint32_t *size)
{
	uint64_t space = _spack_r_space(rpack);
	if (space < 3)
		return -1;
	uint16_t data_len;
	_spack_r_drain(rpack, (char *)&data_len, sizeof(data_len));
	_spack_check_bigendian((char *)&data_len, sizeof(data_len));
	_spack_r_drain(rpack, (char *)type, sizeof(*type));
	if (space < 3 + data_len)
		return -1;
	*data = rpack->buf + rpack->buf_used;
	*size = data_len;
	rpack->buf_used += data_len;
	return 0;
}

static int _spack_get_ext32(struct spack_r *rpack, int8_t *type, const char **data, uint32_t *size)
{
	uint64_t space = _spack_r_space(rpack);
	if (space < 5)
		return -1;
	uint32_t data_len;
	_spack_r_drain(rpack, (char *)&data_len, sizeof(data_len));
	_spack_check_bigendian((char *)&data_len, sizeof(data_len));
	_spack_r_drain(rpack, (char *)type, sizeof(*type));
	if (space < 5 + data_len)
		return -1;
	*data = rpack->buf + rpack->buf_used;
	*size = data_len;
	rpack->buf_used += data_len;
	return 0;
}

int spack_get_ext(struct spack_r *rpack, int8_t *type, const char **data, uint32_t *size)
{
	if (_spack_r_space(rpack) < 1)
		return -1;
	uint8_t format = _spack_r_get_format(rpack);
	if (format == SPACK_FORMAT_FIXEXT1)
		return _spack_get_fixext1(rpack, type, data, size);
	else if (format == SPACK_FORMAT_FIXEXT2)
		return _spack_get_fixext2(rpack, type, data, size);
	else if (format == SPACK_FORMAT_FIXEXT4)
		return _spack_get_fixext4(rpack, type, data, size);
	else if (format == SPACK_FORMAT_FIXEXT8)
		return _spack_get_fixext8(rpack, type, data, size);
	else if (format == SPACK_FORMAT_FIXEXT16)
		return _spack_get_fixext16(rpack, type, data, size);
	else if (format == SPACK_FORMAT_EXT8)
		return _spack_get_ext8(rpack, type, data, size);
	else if (format == SPACK_FORMAT_EXT16)
		return _spack_get_ext16(rpack, type, data, size);
	else if (format == SPACK_FORMAT_EXT32)
		return _spack_get_ext32(rpack, type, data, size);
	return -1;
}

static int _spack_get_fixstr(struct spack_r *rpack, const char **str, uint32_t *size)
{
	if (_spack_r_space(rpack) < *size + 1) /* NULL terminal */
		return -1;
	*str = rpack->buf + rpack->buf_used;
	rpack->buf_used += *size;
	if (rpack->buf[rpack->buf_used++] != '\0')
		return -1;
	return 0;
}

static int _spack_get_str8(struct spack_r *rpack, const char **str, uint32_t *size)
{
	uint64_t space = _spack_r_space(rpack);
	if (space < 2)
		return -1;
	uint8_t str_size;
	_spack_r_drain(rpack, (char *)&str_size, sizeof(str_size));
	if (space < 2 + str_size) /* NULL terminal */
		return -1;
	*size = str_size;
	*str = rpack->buf + rpack->buf_used;
	rpack->buf_used += str_size;
	if (rpack->buf[rpack->buf_used++] != '\0')
		return -1;
	return 0;
}

static int _spack_get_str16(struct spack_r *rpack, const char **str, uint32_t *size)
{
	uint64_t space = _spack_r_space(rpack);
	if (space < 3)
		return -1;
	uint16_t str_size;
	_spack_r_drain(rpack, (char *)&str_size, sizeof(str_size));
	_spack_check_bigendian((char *)&str_size, sizeof(str_size));
	if (space < 3 + str_size)
		return -1;
	*size = str_size;
	*str = rpack->buf + rpack->buf_used;
	rpack->buf_used += str_size;
	if (rpack->buf[rpack->buf_used++] != '\0')
		return -1;
	return 0;
}

static int _spack_get_str32(struct spack_r *rpack, const char **str, uint32_t *size)
{
	uint64_t space = _spack_r_space(rpack);
	if (space < 5)
		return -1;
	_spack_r_drain(rpack, (char *)size, sizeof(*size));
	_spack_check_bigendian((char *)size, sizeof(*size));
	if (space < 5 + *size)
		return -1;
	*str = rpack->buf + rpack->buf_used;
	rpack->buf_used += *size;
	if (rpack->buf[rpack->buf_used++] != '\0')
		return -1;
	return 0;
}

int spack_get_str(struct spack_r *rpack, const char **str, uint32_t *size)
{
	if (_spack_r_space(rpack) < 1)
		return -1;
	uint8_t format = _spack_r_get_format(rpack);
	if (format >= 0xA0 && format <= 0xBF) {
		*size = format & 0x1F;
		return _spack_get_fixstr(rpack, str, size);
	} else if (format == SPACK_FORMAT_STR8)
		return _spack_get_str8(rpack, str, size);
	else if (format == SPACK_FORMAT_STR16)
		return _spack_get_str16(rpack, str, size);
	else if (format == SPACK_FORMAT_STR32)
		return _spack_get_str32(rpack, str, size);
	return -1;
}

static int _spack_get_array16(struct spack_r *rpack, uint32_t *size)
{
	if (_spack_r_space(rpack) < 2)
		return -1;
	uint16_t array_size;
	_spack_r_drain(rpack, (char *)&array_size, sizeof(array_size));
	_spack_check_bigendian((char *)&array_size, sizeof(array_size));
	*size = array_size;
	return 0;
}

static int _spack_get_array32(struct spack_r *rpack, uint32_t *size)
{
	if (_spack_r_space(rpack) < 4)
		return -1;
	_spack_r_drain(rpack, (char *)size, sizeof(*size));
	_spack_check_bigendian((char *)size, sizeof(*size));
	return 0;
}

int spack_get_array(struct spack_r *rpack, uint32_t *size)
{
	if (_spack_r_space(rpack) < 1)
		return -1;
	uint8_t format = _spack_r_get_format(rpack);
	if (format >= 0x90 && format <= 0x9F) {
		*size = format & 0x0F;
		return 0;
	} else if (format == SPACK_FORMAT_ARRAY16)
		return _spack_get_array16(rpack, size);
	else if (format == SPACK_FORMAT_ARRAY32)
		return _spack_get_array32(rpack, size);
	return -1;
}

static int _spack_get_map16(struct spack_r *rpack, uint32_t *size)
{
	if (_spack_r_space(rpack) < 2)
		return -1;
	uint16_t map_size;
	_spack_r_drain(rpack, (char *)&map_size, sizeof(map_size));
	_spack_check_bigendian((char *)&map_size, sizeof(map_size));
	*size = map_size;
	return 0;
}

static int _spack_get_map32(struct spack_r *rpack, uint32_t *size)
{
	if (_spack_r_space(rpack) < 4)
		return -1;
	_spack_r_drain(rpack, (char *)size, sizeof(*size));
	_spack_check_bigendian((char *)size, sizeof(*size));
	return 0;
}

int spack_get_map(struct spack_r *rpack, uint32_t *size)
{
	if (_spack_r_space(rpack) < 1)
		return -1;
	uint8_t format = _spack_r_get_format(rpack);
	if (format >= 0x80 && format <= 0x8F) {
		*size = format & 0x0F;
		return 0;
	} else if (format == SPACK_FORMAT_MAP16)
		return _spack_get_map16(rpack, size);
	else if (format == SPACK_FORMAT_MAP32)
		return _spack_get_map32(rpack, size);
	return -1;
}
