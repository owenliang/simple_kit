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

#include <assert.h>
#include <string.h>
#include "spack.h"

static void test_nil()
{
    struct spack_w wpack;

    char wbuffer[10240];
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    int i;
    for (i = 0; i < 1000; ++i)
        assert(spack_put_nil(&wpack) == 0);

    struct spack_r rpack;
    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    for (i = 0; i < 1000; ++i)
        assert(spack_get_nil(&rpack) == 0);

    assert(rpack.buf_used == wpack.buf_used);
}

static void test_boolean()
{
    struct spack_w wpack;

    char wbuffer[10240];
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    int i;
    for (i = 0; i < 1000; ++i)
        assert(spack_put_boolean(&wpack, 1) == 0);
    for (i = 0; i < 1000; ++i)
        assert(spack_put_boolean(&wpack, 0) == 0);

    struct spack_r rpack;
    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    for (i = 0; i < 1000; ++i) {
        uint8_t boolean;
        assert(spack_get_boolean(&rpack, &boolean) == 0 && boolean);
    }
    for (i = 0; i < 1000; ++i) {
        uint8_t boolean;
        assert(spack_get_boolean(&rpack, &boolean) == 0 && !boolean);
    }
    assert(rpack.buf_used == wpack.buf_used);
}

static void test_int()
{
    struct spack_w wpack;

    char wbuffer[10240];

    /* 64位负数 */
    int64_t i;
    for (i = INT64_MIN; i < INT64_MIN + 10000; ++i) {
        spack_w_init(&wpack, wbuffer, sizeof(wbuffer));
        assert(spack_put_int(&wpack, i) == 0);
        assert(wpack.buf_used == 9);

        struct spack_r rpack;
        spack_r_init(&rpack, wbuffer, wpack.buf_used);

        int64_t n;
        assert(spack_get_int(&rpack, &n) == 0 && n == i);

        assert(rpack.buf_used == wpack.buf_used);
    }
    /* 64位正数 */
    for (i = INT64_MAX - 10000; i < INT64_MAX; ++i) {
        spack_w_init(&wpack, wbuffer, sizeof(wbuffer));
        assert(spack_put_int(&wpack, i) == 0);
        assert(wpack.buf_used == 9);

        struct spack_r rpack;
        spack_r_init(&rpack, wbuffer, wpack.buf_used);

        int64_t n;
        assert(spack_get_int(&rpack, &n) == 0 && n == i);

        assert(rpack.buf_used == wpack.buf_used);
    }
    /* 8位整数 */
    for (i = -128; i <= 127; ++i) {
        spack_w_init(&wpack, wbuffer, sizeof(wbuffer));
        assert(spack_put_int(&wpack, i) == 0);

        if (i >= 0x00 && i <= 0x7F)
            assert(wpack.buf_used == 1);
        else if (i >= (char)0xE0 && i <= (char)0xFF)
            assert(wpack.buf_used == 1);
        else
            assert(wpack.buf_used == 2);

        struct spack_r rpack;
        spack_r_init(&rpack, wbuffer, wpack.buf_used);

        int64_t n;
        assert(spack_get_int(&rpack, &n) == 0 && n == i);

        assert(rpack.buf_used == wpack.buf_used);
    }
    /* 16位整数 */
    for (i = -32768; i <= 32767; ++i) {
        spack_w_init(&wpack, wbuffer, sizeof(wbuffer));
        assert(spack_put_int(&wpack, i) == 0);

        if (i < -128 && i > 127)
            assert(wpack.buf_used == 3);

        struct spack_r rpack;
        spack_r_init(&rpack, wbuffer, wpack.buf_used);

        int64_t n;
        assert(spack_get_int(&rpack, &n) == 0 && n == i);

        assert(rpack.buf_used == wpack.buf_used);
    }
    /* 32位整数 */
    for (i = INT32_MIN; i < INT32_MIN + 10000; ++i) {
        spack_w_init(&wpack, wbuffer, sizeof(wbuffer));
        assert(spack_put_int(&wpack, i) == 0);
        assert(wpack.buf_used == 5);

        struct spack_r rpack;
        spack_r_init(&rpack, wbuffer, wpack.buf_used);

        int64_t n;
        assert(spack_get_int(&rpack, &n) == 0 && n == i);

        assert(rpack.buf_used == wpack.buf_used);
    }
    for (i = INT32_MAX - 10000; i < INT32_MAX; ++i) {
        spack_w_init(&wpack, wbuffer, sizeof(wbuffer));
        assert(spack_put_int(&wpack, i) == 0);
        assert(wpack.buf_used == 5);

        struct spack_r rpack;
        spack_r_init(&rpack, wbuffer, wpack.buf_used);

        int64_t n;
        assert(spack_get_int(&rpack, &n) == 0 && n == i);

        assert(rpack.buf_used == wpack.buf_used);
    }
}

static void test_uint()
{
    struct spack_w wpack;

    char wbuffer[10240];

    uint64_t i;
    for (i = 0; i < 100000; ++i) {
        spack_w_init(&wpack, wbuffer, sizeof(wbuffer));
        assert(spack_put_uint(&wpack, i) == 0);

        struct spack_r rpack;
        spack_r_init(&rpack, wbuffer, wpack.buf_used);

        uint64_t n;
        assert(spack_get_uint(&rpack, &n) == 0 && n == i);

        assert(rpack.buf_used == wpack.buf_used);
    }
    for (i = UINT64_MAX - 10000; i < UINT64_MAX; ++i) {
        spack_w_init(&wpack, wbuffer, sizeof(wbuffer));
        assert(spack_put_uint(&wpack, i) == 0);

        struct spack_r rpack;
        spack_r_init(&rpack, wbuffer, wpack.buf_used);

        uint64_t n;
        assert(spack_get_uint(&rpack, &n) == 0 && n == i);

        assert(rpack.buf_used == wpack.buf_used);
    }
}

static void test_bin()
{
    struct spack_w wpack;

    char wbuffer[102400]; /* 100 K, 支持bin8和bin16 */

    /* bin8 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    char bin8[0xFF] = {1};
    assert(spack_put_bin(&wpack, bin8, sizeof(bin8)) == 0 && wpack.buf_used == 2 + 0xFF);

    struct spack_r rpack;
    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    const char *rbin8;
    uint32_t rlen8;
    assert(spack_get_bin(&rpack, &rbin8, &rlen8) == 0 && rlen8 == 0xFF && memcmp(bin8, rbin8, rlen8) == 0);

    assert(rpack.buf_used == wpack.buf_used);

    /* bin16 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    char bin16[0xFFFF] = {1};
    assert(spack_put_bin(&wpack, bin16, sizeof(bin16)) == 0 && wpack.buf_used == 3 + 0xFFFF);

    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    const char *rbin16;
    uint32_t rlen16;
    assert(spack_get_bin(&rpack, &rbin16, &rlen16) == 0 && rlen16 == 0xFFFF && memcmp(bin16, rbin16, rlen16) == 0);

    assert(rpack.buf_used == wpack.buf_used);

    /* bin32 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    char bin32[0x10000] = {1};
    assert(spack_put_bin(&wpack, bin32, sizeof(bin32)) == 0 && wpack.buf_used == 5 + 0x10000);

    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    const char *rbin32;
    uint32_t rlen32;
    assert(spack_get_bin(&rpack, &rbin32, &rlen32) == 0 && rlen32 == 0x10000 && memcmp(bin32, rbin32, rlen32) == 0);

    assert(rpack.buf_used == wpack.buf_used);
}

static void test_float()
{
    struct spack_w wpack;

    char wbuffer[10240];

    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    float f = 2.3;
    assert(spack_put_float(&wpack, f) == 0);

    struct spack_r rpack;
    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    float rf;
    assert(spack_get_float(&rpack, &rf) == 0 && rf == f);

    assert(rpack.buf_used == wpack.buf_used);
}

static void test_double()
{
    struct spack_w wpack;

    char wbuffer[10240];

    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    double d= 2.3;
    assert(spack_put_double(&wpack, d) == 0);

    struct spack_r rpack;
    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    double rd;
    assert(spack_get_double(&rpack, &rd) == 0 && rd == d);

    assert(rpack.buf_used == wpack.buf_used);
}

static void test_ext()
{
    struct spack_w wpack;

    char wbuffer[102400]; /*100K*/

    /* ext8 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    char ext8[0xFF] = {1};
    assert(spack_put_ext(&wpack, 8, ext8, sizeof(ext8)) == 0 && wpack.buf_used == 3 + 0xFF);

    struct spack_r rpack;
    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    const char *rext8;
    int8_t type;
    uint32_t rlen8;
    assert(spack_get_ext(&rpack, &type, &rext8, &rlen8) == 0);
    assert(type == 8 && rlen8 == 0xFF && memcmp(ext8, rext8, rlen8) == 0);
    assert(rpack.buf_used == wpack.buf_used);

    /* ext16 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    char ext16[0xFFFF] = {1};
    assert(spack_put_ext(&wpack, 8, ext16, sizeof(ext16)) == 0 && wpack.buf_used == 4 + 0xFFFF);

    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    const char *rext16;
    uint32_t rlen16;
    assert(spack_get_ext(&rpack, &type, &rext16, &rlen16) == 0);
    assert(type == 8 && rlen16 == 0xFFFF && memcmp(ext16, rext16, rlen16) == 0);
    assert(rpack.buf_used == wpack.buf_used);

    /* ext32 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    char ext32[0x10000] = {1};
    assert(spack_put_ext(&wpack, 8, ext32, sizeof(ext32)) == 0 && wpack.buf_used == 6 + 0x10000);

    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    const char *rext32;
    uint32_t rlen32;
    assert(spack_get_ext(&rpack, &type, &rext32, &rlen32) == 0);
    assert(type == 8 && rlen32 == 0x10000 && memcmp(ext32, rext32, rlen32) == 0);
    assert(rpack.buf_used == wpack.buf_used);

    /* fixext1 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    char fixext1[1] = {1};
    assert(spack_put_ext(&wpack, 8, fixext1, sizeof(fixext1)) == 0 && wpack.buf_used == 3);

    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    const char *rfixext1;
    uint32_t fix_rlen1;
    assert(spack_get_ext(&rpack, &type, &rfixext1, &fix_rlen1) == 0);
    assert(type == 8 && fix_rlen1 == 1 && memcmp(rfixext1, fixext1, fix_rlen1) == 0);
    assert(rpack.buf_used == wpack.buf_used);

    /* fixext2 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    char fixext2[2] = {1};
    assert(spack_put_ext(&wpack, 8, fixext2, sizeof(fixext2)) == 0 && wpack.buf_used == 4);

    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    const char *rfixext2;
    uint32_t fix_rlen2;
    assert(spack_get_ext(&rpack, &type, &rfixext2, &fix_rlen2) == 0);
    assert(type == 8 && fix_rlen2 == 2 && memcmp(rfixext2, fixext2, fix_rlen2) == 0);
    assert(rpack.buf_used == wpack.buf_used);

    /* fixext4 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    char fixext4[4] = {1};
    assert(spack_put_ext(&wpack, 8, fixext4, sizeof(fixext4)) == 0 && wpack.buf_used == 6);

    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    const char *rfixext4;
    uint32_t fix_rlen4;
    assert(spack_get_ext(&rpack, &type, &rfixext4, &fix_rlen4) == 0);
    assert(type == 8 && fix_rlen4 == 4 && memcmp(rfixext4, fixext4, fix_rlen4) == 0);
    assert(rpack.buf_used == wpack.buf_used);

    /* fixext8 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    char fixext8[8] = {1};
    assert(spack_put_ext(&wpack, 8, fixext8, sizeof(fixext8)) == 0 && wpack.buf_used == 10);

    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    const char *rfixext8;
    uint32_t fix_rlen8;
    assert(spack_get_ext(&rpack, &type, &rfixext8, &fix_rlen8) == 0);
    assert(type == 8 && fix_rlen8 == 8 && memcmp(rfixext8, fixext8, fix_rlen8) == 0);
    assert(rpack.buf_used == wpack.buf_used);

    /* fixext16 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    char fixext16[16] = {1};
    assert(spack_put_ext(&wpack, 8, fixext16, sizeof(fixext16)) == 0 && wpack.buf_used == 18);

    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    const char *rfixext16;
    uint32_t fix_rlen16;
    assert(spack_get_ext(&rpack, &type, &rfixext16, &fix_rlen16) == 0);
    assert(type == 8 && fix_rlen16 == 16 && memcmp(rfixext16, fixext16, fix_rlen16) == 0);
    assert(rpack.buf_used == wpack.buf_used);
}

static void test_str()
{
    struct spack_w wpack;

    char wbuffer[102400];/* 100K */

    /* fixstr */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    char fixstr[31] = {1};
    assert(spack_put_str(&wpack, fixstr, sizeof(fixstr)) == 0);

    struct spack_r rpack;
    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    const char *rfixstr = NULL;
    uint32_t rfixlen;
    assert(spack_get_str(&rpack, &rfixstr, &rfixlen) == 0 && rfixlen == sizeof(fixstr)  && memcmp(fixstr, rfixstr, rfixlen) == 0
            && rfixlen == 31 && wpack.buf_used == 33);
    assert(rpack.buf_used == wpack.buf_used);

    /* str8 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    char str8[0xFF] = {1};
    assert(spack_put_str(&wpack, str8, sizeof(str8)) == 0);

    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    const char *rstr8 = NULL;
    uint32_t rlen8;
    assert(spack_get_str(&rpack, &rstr8, &rlen8) == 0 && rlen8 == sizeof(str8)  && memcmp(str8, rstr8, rlen8) == 0
            && rlen8 == 0xFF && wpack.buf_used == 3 + 0xFF);
    assert(rpack.buf_used == wpack.buf_used);

    /* str16 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    char str16[0xFFFF] = {1};
    assert(spack_put_str(&wpack, str16, sizeof(str16)) == 0);

    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    const char *rstr16 = NULL;
    uint32_t rlen16;
    assert(spack_get_str(&rpack, &rstr16, &rlen16) == 0 && rlen16 == sizeof(str16)  && memcmp(str16, rstr16, rlen16) == 0
            && rlen16 == 0xFFFF && wpack.buf_used == 4 + 0xFFFF);
    assert(rpack.buf_used == wpack.buf_used);

    /* str32 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    char str32[0x10000] = {1};
    assert(spack_put_str(&wpack, str32, sizeof(str32)) == 0);

    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    const char *rstr32 = NULL;
    uint32_t rlen32;
    assert(spack_get_str(&rpack, &rstr32, &rlen32) == 0 && rlen32 == sizeof(str32)  && memcmp(str32, rstr32, rlen32) == 0
            && rlen32 == 0x10000 && wpack.buf_used == 6 + 0x10000);
    assert(rpack.buf_used == wpack.buf_used);
}

static void test_array()
{
    struct spack_w wpack;

    char wbuffer[10240];

    /* fixarray */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

	assert(spack_put_array(&wpack, 5) == 0);
    assert(wpack.buf_used == 1);

    struct spack_r rpack;
    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    uint32_t size;
    assert(spack_get_array(&rpack, &size) == 0 && size == 5);
    assert(rpack.buf_used == wpack.buf_used);

    /* array16 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    assert(spack_put_array(&wpack, 0xFF) == 0);
    assert(wpack.buf_used == 3);

    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    assert(spack_get_array(&rpack, &size) == 0 && size == 0xFF);
    assert(rpack.buf_used == wpack.buf_used);

    /* array32 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    assert(spack_put_array(&wpack, 0xFFFFFFFF) == 0);
    assert(wpack.buf_used == 5);

    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    assert(spack_get_array(&rpack, &size) == 0 && size == 0xFFFFFFFF);
    assert(rpack.buf_used == wpack.buf_used);
}

static void test_map()
{
    struct spack_w wpack;

    char wbuffer[10240];

    /* fixmap */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

	assert(spack_put_map(&wpack, 5) == 0);
    assert(wpack.buf_used == 1);

    struct spack_r rpack;
    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    uint32_t size;
    assert(spack_get_map(&rpack, &size) == 0 && size == 5);
    assert(rpack.buf_used == wpack.buf_used);

    /* map16 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    assert(spack_put_map(&wpack, 0xFF) == 0);
    assert(wpack.buf_used == 3);

    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    assert(spack_get_map(&rpack, &size) == 0 && size == 0xFF);
    assert(rpack.buf_used == wpack.buf_used);

    /* map32 */
    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    assert(spack_put_map(&wpack, 0xFFFFFF) == 0);
    assert(wpack.buf_used == 5);

    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    assert(spack_get_map(&rpack, &size) == 0 && size == 0xFFFFFF);
    assert(rpack.buf_used == wpack.buf_used);
}

int main(int argc, char **argv)
{
    test_nil();
    test_boolean();
    test_int();
    test_uint();
    test_bin();
    test_float();
    test_double();
    test_ext();
    test_str();
    test_array();
    test_map();
    return 0;
}
