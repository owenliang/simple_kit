/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file test_sconfig.c
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/05/31 10:41:41
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "slist.h"
#include "sconfig.h"

void test_case1(const char *filename)
{
    struct sconfig *sconf = sconfig_new(filename);
    assert(sconf);
    const char *user;
    assert(sconfig_read_str(sconf, "user", &user) == 0);
    assert(strcmp("hero", user) == 0);
    const char *passwd;
    assert(sconfig_read_str(sconf, "passwd", &passwd) == 0);
    assert(strcmp("unknown", passwd) == 0);
    const char *key;
    assert(sconfig_read_str(sconf, "key", &key) == -1);
    const char *nickname;
    assert(sconfig_read_str(sconf, "nickname", &nickname) == 0);
    assert(strcmp(nickname, "") == 0);
    const char *bbs_name;
    assert(sconfig_read_str(sconf, "BBS_name", &bbs_name) == 0);
    assert(strcmp(bbs_name, "value可以是中文 可以含有空格 name是区分大小写的") == 0);
    assert(sconfig_read_str(sconf, "bbs_name", &bbs_name) == -1);
    const char *allow_login;
    assert(sconfig_read_str(sconf, "allow_login", &allow_login) == 0);
    assert(strcmp(allow_login, "#这个不是注释") == 0);
    assert(slist_size(sconf->items) == 5);
    int count = 0;
    sconfig_begin_iterate(sconf);
    const char *name, *value;
    while (sconfig_iterate(sconf, &name, &value) != -1) {
        printf("%s=%s\n", name, value);
        ++count;
    }
    sconfig_end_iterate(sconf);
    assert(count == 5);
    assert(sconfig_reload(sconf) == 0);
    count = 0;
    while (sconfig_iterate(sconf, &name, &value) != -1) {
        printf("%s=%s\n", name, value);
        ++count;
    }
    assert(count == 0);
    assert(strcmp(sconf->filename, filename) == 0);
    sconfig_free(sconf);
}

void test_case2(const char *filename)
{
    struct sconfig *sconf = sconfig_new(filename);
    assert(sconf);
    const char *user;
    assert(sconfig_read_str(sconf, "user", &user) == 0);
    assert(strcmp("hero", user) == 0);
    const char *passwd;
    assert(sconfig_read_str(sconf, "passwd", &passwd) == 0);
    assert(strcmp("x101_9x-293", passwd) == 0);
    int32_t online_time;
    assert(sconfig_read_int32(sconf, "online_time", &online_time) == 0);
    assert(online_time == 235);
    uint32_t online_time_u;
    assert(sconfig_read_uint32(sconf, "online_time", &online_time_u) == 0);
    assert(online_time_u == 235);
    int64_t online_time_64;
    assert(sconfig_read_int64(sconf, "online_time", &online_time_64) == 0);
    assert(online_time_64 == 235);
    uint64_t online_time_u64;
    assert(sconfig_read_uint64(sconf, "online_time", &online_time_u64) == 0);
    assert(online_time_u64 == 235);
    int32_t gold;
    assert(sconfig_read_int32(sconf, "gold", &gold) == 0);
    assert(gold == -32);
    uint32_t gold_u;
    assert(sconfig_read_uint32(sconf, "gold", &gold_u) == 0);
    assert(gold_u == (uint32_t)-32);
    int64_t gold_64;
    assert(sconfig_read_int64(sconf, "gold", &gold_64) == 0);
    assert(gold_64 == -32);
    uint64_t gold_u64;
    assert(sconfig_read_uint64(sconf, "gold", &gold_u64) == 0);
    assert(gold_u64 == (uint64_t)-32);
    double score;
    assert(sconfig_read_double(sconf, "score", &score) == 0);
    assert(fabs(score - 45.32) < 0.001);
    assert(strcmp(sconf->filename, filename) == 0);
    uint32_t uint32_score;
    assert(sconfig_read_uint32(sconf, "score", &uint32_score) == 0);
    assert(uint32_score == 45); /* 符合strtoul截断预期 */
    uint32_t uint32_token;
    assert(sconfig_read_uint32(sconf, "token", &uint32_token) == 0);
    assert((uint64_t)uint32_token != 1234567890123ULL); /* 符合strtoul溢出预期 */
    uint64_t uint64_token;
    assert(sconfig_read_uint64(sconf, "token", &uint64_token) == 0);
    assert(uint64_token == 1234567890123ULL);
    double double_token;
    assert(sconfig_read_double(sconf, "token", &double_token) == 0);
    assert(fabs(double_token - 1234567890123ULL) < 0.001);
    assert(slist_size(sconf->items) == 6);
    sconfig_free(sconf);
}

void test_case3()
{
    struct sconfig *case3 = sconfig_new("./test_cases/test3.conf");
    assert(case3 == NULL);
    struct sconfig *case4 = sconfig_new("./test_cases/test4.conf");
    assert(case4 == NULL);
    struct sconfig *case5 = sconfig_new("./test_cases/test5.conf");
    assert(case5 == NULL);
    struct sconfig *case6 = sconfig_new("./test_cases/test6.conf");
    assert(case6 == NULL);
    struct sconfig *case7 = sconfig_new("./test_cases/test7.conf");
    assert(case7 == NULL);
    struct sconfig *case8 = sconfig_new("./test_cases/test8.conf");
    assert(case8 == NULL);
    struct sconfig *case9 = sconfig_new("./test_cases/test9.conf");
    assert(case9 == NULL);
    struct sconfig *case10 = sconfig_new("./test_cases/test10.conf");
    assert(case10 == NULL);
    struct sconfig *case11 = sconfig_new("./test_cases/test11.conf");
    assert(case11 != NULL);
    sconfig_free(case11);
    struct sconfig *case12 = sconfig_new("./test_cases/test12.conf");
    assert(case12 != NULL);
    const char *v12;
    assert(sconfig_read_str(case12, "key", &v12) == 0);
    assert(strcmp("value", v12) == 0);
    sconfig_free(case12);
    struct sconfig *case13 = sconfig_new("./test_cases/test13.conf");
    assert(case13 == NULL);
}

int main(int argc, char **argv)
{
    test_case1("./test_cases/test1.conf"); /* 测试基础逻辑 */
    test_case2("./test_cases/test2.conf"); /* 测试功能接口 */
    test_case3(); /* 测试非法输入 */
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
