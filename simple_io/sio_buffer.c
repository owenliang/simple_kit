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
#include "sio_buffer.h"

/* 初始化缓冲区尺寸16KB */
#define SIO_BUFFER_INIT_CAPACITY 16384
/* 当缓冲区尺寸超过1048576并且占用低于16KB时, 将缓冲区收缩为16KB */
#define SIO_BUFFER_SHRINK_CAPACITY 1048576

struct sio_buffer *sio_buffer_new()
{
    struct sio_buffer *sbuf = calloc(1, sizeof(*sbuf));
    sbuf->buffer = malloc(SIO_BUFFER_INIT_CAPACITY);
    sbuf->capacity = SIO_BUFFER_INIT_CAPACITY;
    return sbuf;
}

void sio_buffer_free(struct sio_buffer *sbuf)
{
    free(sbuf->buffer);
    free(sbuf);
}

void sio_buffer_reserve(struct sio_buffer *sbuf, uint64_t size)
{
    if (sbuf->capacity - sbuf->end >= size)
        return;
    if (sbuf->capacity - sbuf->end + sbuf->start >= size) {
        memmove(sbuf->buffer, sbuf->buffer + sbuf->start, sbuf->end - sbuf->start);
    } else {
        uint64_t new_capacity = sbuf->end - sbuf->start + size;
        char *new_buffer = malloc(new_capacity);
        memcpy(new_buffer, sbuf->buffer + sbuf->start, sbuf->end - sbuf->start);
        free(sbuf->buffer);
        sbuf->buffer = new_buffer;
        sbuf->capacity = new_capacity;
    }
    sbuf->end -= sbuf->start;
    sbuf->start = 0;
}

void sio_buffer_append(struct sio_buffer *sbuf, const char *data, uint64_t size)
{
    sio_buffer_reserve(sbuf, size);
    memcpy(sbuf->buffer + sbuf->end, data, size);
    sbuf->end += size;
}

void sio_buffer_seek(struct sio_buffer *sbuf, uint64_t size)
{
    sbuf->end += size;
}

void sio_buffer_erase(struct sio_buffer *sbuf, uint64_t size)
{
    sbuf->start += size;
    if (sbuf->capacity >= SIO_BUFFER_SHRINK_CAPACITY && sbuf->end - sbuf->start < SIO_BUFFER_INIT_CAPACITY) {
        char *smaller_buffer = malloc(SIO_BUFFER_INIT_CAPACITY);
        memcpy(smaller_buffer, sbuf->buffer + sbuf->start, sbuf->end - sbuf->start);
        free(sbuf->buffer);
        sbuf->buffer = smaller_buffer;
        sbuf->capacity = SIO_BUFFER_INIT_CAPACITY;
        sbuf->end -= sbuf->start;
        sbuf->start = 0;
    }
}

uint64_t sio_buffer_length(struct sio_buffer *sbuf)
{
    return sbuf->end - sbuf->start;
}

uint64_t sio_buffer_capacity(struct sio_buffer *sbuf)
{
    return sbuf->capacity;
}

char *sio_buffer_space(struct sio_buffer *sbuf, uint64_t *size)
{
    if (size)
        *size = sbuf->capacity - sbuf->end;
    return sbuf->buffer + sbuf->end;
}

char *sio_buffer_data(struct sio_buffer *sbuf, uint64_t *size)
{
    if (size)
        *size = sbuf->end - sbuf->start;
    return sbuf->buffer + sbuf->start;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
