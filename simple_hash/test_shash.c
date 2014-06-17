/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file test_shash.c
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/06/16 19:21:35
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include <stdio.h>
#include <assert.h>
#include "shash.h"

void insert_range(struct shash *shash, int range)
{
    char buffer[32];
    int i;
    for (i = 0; i < range; ++i) {
        size_t len = snprintf(buffer, sizeof(buffer), "%d", i);
        if (len >= sizeof(buffer))
            len = sizeof(buffer) - 1;
        assert(0 == shash_insert(shash, buffer, len + 1, NULL));
    }
}

void find_range(struct shash *shash, int range)
{
    char buffer[32];
    int i;
    for (i = 0; i < range; ++i) {
        size_t len = snprintf(buffer, sizeof(buffer), "%d", i);
        if (len >= sizeof(buffer))
            len = sizeof(buffer) - 1;
        assert(0 == shash_find(shash, buffer, len + 1, NULL));
    }
}

void erase_iterate(struct shash *shash, int range)
{
    int count = 0;
    int size = shash_size(shash);
    assert(size == range);

    shash_begin_iterate(shash);
    const char *key;
    uint32_t key_len;
    while (shash_iterate(shash, &key, &key_len, NULL) != -1) {
        assert(shash_erase(shash, key, key_len) == 0);
        ++count;
    }
    assert(shash_iterate(shash, NULL, NULL, NULL) == -1);
    assert(shash_size(shash) == 0);
    assert(count == range);
    shash_end_iterate(shash);
}

void begin_iterate_when_rehash()
{
    struct shash *shash = shash_new();
    int i;
    char buffer[32];
    struct shash_node *rehash_backup = NULL;
    for (i = 0; i < 102400; ++i) {
        size_t len = snprintf(buffer, sizeof(buffer), "%d", i);
        if (len >= sizeof(buffer))
            len = sizeof(buffer) - 1;
        assert(0 == shash_insert(shash, buffer, len + 1, NULL));
        if (shash->buckets[1]) {
            shash_begin_iterate(shash);
            rehash_backup = shash->buckets[1];
        }
    }
    assert(rehash_backup == shash->buckets[1]);
    shash_end_iterate(shash);
    find_range(shash, 102400);
    find_range(shash, 102400);
    assert(shash->buckets[1] == NULL);
    shash_free(shash);
}

int main(int argc, char **argv)
{
    struct shash *shash = shash_new();
    insert_range(shash, 102400);
    find_range(shash, 102400);
    erase_iterate(shash, 102400);
    shash_free(shash);
    begin_iterate_when_rehash();
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
