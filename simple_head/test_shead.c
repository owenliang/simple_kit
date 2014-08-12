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

#include "shead.h"
#include <assert.h>

int main(int argc, char **argv)
{
    struct shead head;

    head.id = 555;
    head.type = 5;
    head.body_len = 10;
    head.reserved = 7;

    char encode_head[SHEAD_ENCODE_SIZE];
    assert(shead_encode(&head, encode_head, sizeof(encode_head)) == 0);

    struct shead decode_head;
    assert(shead_decode(&decode_head, encode_head, sizeof(encode_head)) == 0);
    assert(decode_head.id == head.id);
    assert(decode_head.type == head.type);
    assert(decode_head.reserved == head.reserved);
    assert(decode_head.body_len == head.body_len);

    return 0;
}
