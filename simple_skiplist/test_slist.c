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

#include "slist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <netinet/in.h>

void insert_range(struct slist *slist, int range)
{
    int i;
    for (i = 0; i < range; ++i) {
        int n = htonl(i);
        assert(slist_insert(slist, (const char *)&n, sizeof(n), NULL) == 0);
        printf("insert_range=%d\n", i);
    }
}

void iterate_erase(struct slist *slist)
{
    slist_begin_iterate(slist);
    const char *key;
    uint32_t key_len;
    void *value;
    while (slist_iterate(slist, &key, &key_len, &value) != -1) {
        int n = *(const int *)key;
        n = ntohl(n);
        printf("iterate_erase=%d\n", n);
        assert(slist_erase(slist, key, key_len) == 0);
        assert(value == NULL);
    }
    slist_end_iterate(slist);
}

void riterate_erase(struct slist *slist)
{
    slist_begin_riterate(slist);
    const char *key;
    uint32_t key_len;
    void *value;
    while (slist_riterate(slist, &key, &key_len, &value) != -1) {
        int n = *(const int *)key;
        n = ntohl(n);
        printf("riterate_erase=%d\n", n);
        assert(slist_erase(slist, key, key_len) == 0);
        assert(value == NULL);
    }
    slist_end_riterate(slist);
}

void find_valid_range(struct slist *slist, int range)
{
    int i;
    for (i = 0; i < range; ++i) {
        int n = htonl(i);
        assert(slist_find(slist, (const char *)&n, sizeof(n), NULL) == 0);
        printf("find_valid_range=%d\n", i);
    }
}

void find_invalid_range(struct slist *slist, int range)
{
    int i;
    for (i = range; i < range * 2; ++i) {
        int n = htonl(i);
        assert(slist_find(slist, (const char *)&n, sizeof(n), NULL) == -1);
        printf("find_invalid_range=%d\n", i);
    }
}

void pop_front_erase(struct slist *slist, int range)
{
    int i;
    for (i = 0; i < range; ++i) {
        const char *key;
        assert(slist_front(slist, &key, NULL, NULL) == 0);
        int n = ntohl(*(const int *)key);
        assert(slist_pop_front(slist) == 0);
        printf("pop_front_erase=%d\n", n);
    }
}

void pop_back_erase(struct slist *slist, int range)
{
    int i;
    for (i = 0; i < range; ++i) {
        const char *key;
        assert(slist_back(slist, &key, NULL, NULL) == 0);
        int n = ntohl(*(const int *)key);
        assert(slist_pop_back(slist) == 0);
        printf("pop_back_erase=%d\n", n);
    }
}

void check_iterate(struct slist *slist, int range)
{
    // 同时打开双向迭代器,验证边界行为
    slist_begin_iterate(slist);
    slist_begin_riterate(slist);
    assert(slist->iterator);
    assert(slist->riterator);
    assert(slist->iterator->levels[0].next == slist->riterator);
    int n = htonl(1);
    assert(slist_erase(slist, (const char *)&n, sizeof(n)) == 0);
    assert(slist->iterator == slist->riterator);
    n = htonl(0);
    assert(slist_erase(slist, (const char *)&n, sizeof(n)) == 0);
    assert(slist->iterator == slist->riterator && slist->iterator == NULL);
}

void test_rank()
{
    struct slist *slist = slist_new(32);

    assert(slist_insert(slist, "b", sizeof("b"), NULL) == 0);
    assert(slist_insert(slist, "a", sizeof("a"), NULL) == 0);
    assert(slist_insert(slist, "c", sizeof("c"), NULL) == 0);

    int64_t rank;
    rank = slist_rank(slist, "b", sizeof("b"));
    printf("b rank=%ld\n", rank);
    rank = slist_rank(slist, "a", sizeof("a"));
    printf("a rank=%ld\n", rank);
    rank = slist_rank(slist, "c", sizeof("c"));
    printf("c rank=%ld\n", rank);

    struct slist_node *node = slist->head;
    while (node != NULL) {
        printf("key=%s height=%d\n", node->key, node->height);
        int l;
        for (l = 0; l < node->height; ++l) {
            printf("level[%d].span=%lu\n", l, node->levels[l].span);
        }
        node = node->levels[0].next;
    }

    slist_erase(slist, "b", sizeof("b"));

    rank = slist_rank(slist, "b", sizeof("b"));
    printf("b rank=%ld\n", rank);
    rank = slist_rank(slist, "a", sizeof("a"));
    printf("a rank=%ld\n", rank);
    rank = slist_rank(slist, "c", sizeof("c"));
    printf("c rank=%ld\n", rank);
}

int main(int argc, char **argv)
{
    srand(time(NULL));

    struct slist *slist = slist_new(32);

    insert_range(slist, 20);
    iterate_erase(slist);
    assert(slist_size(slist) == 0);
    assert(slist->iterator == NULL);

    insert_range(slist, 20);
    riterate_erase(slist);
    assert(slist_size(slist) == 0);
    assert(slist->riterator == NULL);

    insert_range(slist, 20);
    find_valid_range(slist, 20);
    find_invalid_range(slist, 20);

    pop_front_erase(slist, 20);
    assert(slist_size(slist) == 0);

    insert_range(slist, 20);
    pop_back_erase(slist, 20);
    assert(slist_size(slist) == 0);

    insert_range(slist, 2);
    check_iterate(slist, 2);

    slist_free(slist);

    test_rank();
    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
