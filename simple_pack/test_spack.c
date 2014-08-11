/*
 * test_spack.c
 *
 *  Created on: 2014年8月11日
 *      Author: liangdong01
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

    int64_t i;
    for (i = INT64_MIN; i < INT64_MIN + 10000; ++i) {
        spack_w_init(&wpack, wbuffer, sizeof(wbuffer));
        assert(spack_put_int(&wpack, i) == 0);

        struct spack_r rpack;
        spack_r_init(&rpack, wbuffer, wpack.buf_used);

        int64_t n;
        assert(spack_get_int(&rpack, &n) == 0 && n == i);

        assert(rpack.buf_used == wpack.buf_used);
    }
    for (i = INT64_MAX - 10000; i < INT64_MAX; ++i) {
        spack_w_init(&wpack, wbuffer, sizeof(wbuffer));
        assert(spack_put_int(&wpack, i) == 0);

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

    char wbuffer[10240];

    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    char bin8[0xFF] = {1};
    assert(spack_put_bin(&wpack, bin8, sizeof(bin8)) == 0 && wpack.buf_used == 2 + 0xFF);

    struct spack_r rpack;
    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    const char *rbin8;
    uint32_t rlen8;
    assert(spack_get_bin(&rpack, &rbin8, &rlen8) == 0 && rlen8 == 0xFF && memcmp(bin8, rbin8, rlen8) == 0);
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
}

static void test_ext()
{
    struct spack_w wpack;

    char wbuffer[10240];

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
}

static void test_str()
{
    struct spack_w wpack;

    char wbuffer[10240];

    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

    const char *str = "hello world";
    assert(spack_put_str(&wpack, str, strlen(str)) == 0);

    struct spack_r rpack;
    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    const char *rstr = NULL;
    uint32_t len;
    assert(spack_get_str(&rpack, &rstr, &len) == 0 && strcmp(rstr, "hello world") == 0 && len == 11);
}

static void test_array()
{
    struct spack_w wpack;

    char wbuffer[10240];

    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

	assert(spack_put_array(&wpack, 5) == 0);

    struct spack_r rpack;
    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    uint32_t size;
    assert(spack_get_array(&rpack, &size) == 0 && size == 5);
}

static void test_map()
{
    struct spack_w wpack;

    char wbuffer[10240];

    spack_w_init(&wpack, wbuffer, sizeof(wbuffer));

	assert(spack_put_map(&wpack, 5) == 0);

    struct spack_r rpack;
    spack_r_init(&rpack, wbuffer, wpack.buf_used);

    uint32_t size;
    assert(spack_get_map(&rpack, &size) == 0 && size == 5);
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
