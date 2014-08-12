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

#include <string.h>
#include "shead.h"

static void _shead_reverse_bytes(char *bytes, uint32_t len)
{
    char *end = bytes + len - 1;
    while (bytes < end) {
        char temp = *bytes;
        *bytes = *end;
        *end = temp;
        ++bytes, --end;
    }
}

static void _shead_check_bigendian(char *bytes, uint32_t len)
{
    union {
        char b;
        int n;
    } u;
    u.n = 1;
    if (u.b == 1)
        _shead_reverse_bytes(bytes, len);
}

int shead_encode(const struct shead *head, char *output, uint32_t len)
{
    if (len < SHEAD_ENCODE_SIZE)
        return -1;

    uint64_t id = head->id;
    _shead_check_bigendian((char *)&id, sizeof(id));
    memcpy(output, &id, sizeof(id));

    uint32_t type = head->type;
    _shead_check_bigendian((char *)&type, sizeof(type));
    memcpy(output + 8, &type, sizeof(type));

    uint32_t magic_num = SHEAD_MAGIC_NUM;
    _shead_check_bigendian((char *)&magic_num, sizeof(magic_num));
    memcpy(output + 12, &magic_num, sizeof(magic_num));

    uint32_t reserved = head->reserved;
    _shead_check_bigendian((char *)&reserved, sizeof(reserved));
    memcpy(output + 16, &reserved, sizeof(reserved));

    uint32_t body_len = head->body_len;
    _shead_check_bigendian((char *)&body_len, sizeof(body_len));
    memcpy(output + 20, &body_len, sizeof(body_len));

    return 0;
}

int shead_decode(struct shead *head, const char *input, uint32_t len)
{
    if (len < SHEAD_ENCODE_SIZE)
        return -1;

    uint32_t magic_num;
    memcpy(&magic_num, input + 12, sizeof(magic_num));
    _shead_check_bigendian((char *)&magic_num, sizeof(magic_num));
    if (magic_num != SHEAD_MAGIC_NUM)
        return -1;

    head->magic_num = magic_num;

    memcpy(&head->id, input, sizeof(head->id));
    _shead_check_bigendian((char *)&head->id, sizeof(head->id));

    memcpy(&head->type, input + 8, sizeof(head->type));
    _shead_check_bigendian((char *)&head->type, sizeof(head->type));

    memcpy(&head->reserved, input + 16, sizeof(head->reserved));
    _shead_check_bigendian((char *)&head->reserved, sizeof(head->reserved));

    memcpy(&head->body_len, input + 20, sizeof(head->body_len));
    _shead_check_bigendian((char *)&head->body_len, sizeof(head->body_len));

    return 0;
}
