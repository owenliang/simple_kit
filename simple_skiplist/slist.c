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
#include <time.h>
#include "slist.h"

static struct slist_node *_slist_new_node(char *key, uint32_t key_len, uint32_t height, void *value)
{
    struct slist_node *node = calloc(1, sizeof(*node) + sizeof(struct slist_level) * (height - 1));
    node->key = key;
    node->key_len = key_len;
    node->height = height;
    node->value = value;
    return node;
}

void _slist_free_node(struct slist_node *node)
{
    free(node->key);
    free(node);
}

struct slist *slist_new(uint32_t height)
{
    struct slist *slist = calloc(1, sizeof(*slist) + sizeof(struct slist_node *) * height);
    slist->max_height = height + 1;
    slist->cur_height = 1;
    slist->seed = time(NULL);
    slist->head = _slist_new_node(NULL, 0, slist->max_height,NULL);
    slist->level_rank = malloc(sizeof(uint64_t) * height);
    return slist;
}

void slist_free(struct slist *slist)
{
    struct slist_node *node = slist->head;
    while (node) {
        struct slist_node *next = node->levels[0].next;
        _slist_free_node(node);
        node = next;
    }
    free(slist->level_rank);
    free(slist);
}

static int _slist_random(struct slist *slist)
{
    int n = rand_r(&slist->seed);
    n &= 0xFFFF;
    if (n > 0.5 * 0xFFFF)
        return 1;
    return 0;
}

static uint32_t _slist_choose_height(struct slist *slist)
{
    uint32_t height = 1;
    while (height < slist->max_height && _slist_random(slist))
        ++height;
    return height;
}

static int _slist_key_compare(const char *key1, uint32_t key_len1, const char *key2, uint32_t key_len2)
{
    uint32_t min_len = key_len1 < key_len2 ? key_len1 : key_len2;
    int ret = memcmp(key1, key2, min_len);
    if (ret)
        return ret;
    if (key_len1 == key_len2)
        return 0;
    return key_len1 < key_len2 ? -1 : 1;
}

int slist_insert(struct slist *slist, const char *key, uint32_t key_len, void *value)
{
    struct slist_node *node = slist->head;
    uint32_t h;
    for (h = slist->cur_height; h >= 1; --h) {
        uint32_t l = h - 1;
        slist->level_rank[l] = h == slist->cur_height ? 0 : slist->level_rank[l + 1];
        while (node->levels[l].next && 
                _slist_key_compare(key, key_len, node->levels[l].next->key, node->levels[l].next->key_len) > 0) {
            slist->level_rank[l] += node->levels[l].span;
            node = node->levels[l].next;
        }
        slist->level_node[l] = node;
    }
    if (node->levels[0].next && 
            _slist_key_compare(key, key_len, node->levels[0].next->key, node->levels[0].next->key_len) == 0)
        return -1;

    uint32_t height = _slist_choose_height(slist);
    if (height > slist->cur_height) {
        for (h = height; h > slist->cur_height; --h) {
            uint32_t l = h - 1;
            slist->level_node[l] = slist->head;
            slist->level_rank[l] = 0;
        }
    }
    
    char *dup_key = malloc(key_len);
    memcpy(dup_key, key, key_len);
    struct slist_node *new_node = _slist_new_node(dup_key, key_len, height, value);
    for (h = height; h >= 1; --h) {
        uint32_t l = h - 1;
        node = slist->level_node[l];
        new_node->levels[l].next = node->levels[l].next;
        new_node->levels[l].prev = node;
        node->levels[l].next = new_node;
        new_node->levels[l].span = node->levels[l].span - (slist->level_rank[0] - slist->level_rank[l] + 1) + 1; // 当没有后继时,span值是无效的
        node->levels[l].span = slist->level_rank[0] - slist->level_rank[l] + 1;
        if (new_node->levels[l].next)
            new_node->levels[l].next->levels[l].prev = new_node;
    }
    if (height > slist->cur_height) {
        slist->cur_height = height;
    } else {
        for (h = height + 1; h <= slist->cur_height; ++h) {
            uint32_t l = h - 1;
            ++slist->level_node[l]->levels[l].span;
        }
    }
    if (!new_node->levels[0].next)
        slist->tail = new_node;
    ++slist->num_nodes;
    return 0;
}

static struct slist_node *_slist_find_node(struct slist *slist, const char *key, uint32_t key_len)
{
    struct slist_node *node = slist->head;
    uint32_t h;
    for (h = slist->cur_height; h >= 1; --h) {
        uint32_t l = h - 1;
        while (node->levels[l].next) {
            int ret = _slist_key_compare(key, key_len, node->levels[l].next->key, node->levels[l].next->key_len);
            if (ret < 0)
                break;
            else if (ret == 0)
                return node->levels[l].next;
            node = node->levels[l].next;
        }
    }
    return NULL;
}

static void _slist_erase_node(struct slist *slist, struct slist_node *node)
{
    uint32_t h;
    for (h = node->height; h >= 1; --h) {
        uint32_t l = h - 1;
        node->levels[l].prev->levels[l].span += node->levels[l].span - 1;
        node->levels[l].prev->levels[l].next = node->levels[l].next;
        if (node->levels[l].next)
            node->levels[l].next->levels[l].prev = node->levels[l].prev;
    }
    if (node == slist->tail) {
        if (slist->head == node->levels[0].prev)
            slist->tail = NULL;
        else
            slist->tail = node->levels[0].prev;
    }
    for (h = slist->cur_height; slist->cur_height > 1 && h >= 1; --h) {
        if (!slist->head->levels[h - 1].next)
            --slist->cur_height;
        else
            break;
    }
    if (node == slist->iterator)
        slist->iterator = node->levels[0].next;
    if (node == slist->riterator) {
        slist->riterator = node->levels[0].prev;
        if (slist->riterator == slist->head)
            slist->riterator = NULL;
    }
    _slist_free_node(node);
    --slist->num_nodes;
}

int slist_erase(struct slist *slist, const char *key, uint32_t key_len)
{
    struct slist_node *node = _slist_find_node(slist, key, key_len);
    if (!node)
        return -1;
    _slist_erase_node(slist, node);
    return 0;
}

int slist_find(struct slist *slist, const char *key, uint32_t key_len, void **value)
{
    struct slist_node *node = _slist_find_node(slist, key, key_len);
    if (!node)
        return -1;
    if (value)
        *value = node->value;
    return 0;
}

int slist_front(struct slist *slist, const char **key, uint32_t *key_len, void **value)
{
    struct slist_node *node = slist->head->levels[0].next;
    if (!node)
        return -1;
    if (key)
        *key = node->key;
    if (key_len)
        *key_len = node->key_len;
    if (value)
        *value = node->value;
    return 0;
}

int slist_pop_front(struct slist *slist)
{
    struct slist_node *node = slist->head->levels[0].next;
    if (!node)
        return -1;
    _slist_erase_node(slist, node);
    return 0;
}

int slist_back(struct slist *slist, const char **key, uint32_t *key_len, void **value)
{
    struct slist_node *node = slist->tail;
    if (!node)
        return -1;
    if (key)
        *key = node->key;
    if (key_len)
        *key_len = node->key_len;
    if (value)
        *value = node->value;
    return 0;
}

int slist_pop_back(struct slist *slist)
{
    struct slist_node *node = slist->tail;
    if (!node)
        return -1;
    _slist_erase_node(slist, node);
    return 0;
}

uint64_t slist_size(struct slist *slist)
{
    return slist->num_nodes;
}

int64_t slist_rank(struct slist *slist, const char* key, uint32_t key_len)
{
    struct slist_node *node = slist->head;
    uint32_t h;
    uint64_t rank = 0;

    for (h = slist->cur_height; h >= 1; --h) {
        uint32_t l = h - 1;
        while (node->levels[l].next) {
            int ret = _slist_key_compare(key, key_len, node->levels[l].next->key, node->levels[l].next->key_len);
            if (ret < 0) {
                break;
            } else if (ret == 0) {
                rank += node->levels[l].span;
                return rank;
            }
            rank += node->levels[l].span;
            node = node->levels[l].next;
        }
    }
    return -1;
}

void slist_begin_iterate(struct slist *slist)
{
    slist->iterator = slist->head->levels[0].next;
}
int slist_iterate(struct slist *slist, const char **key, uint32_t *key_len, void **value)
{
    if (!slist->iterator)
        return -1;
    if (key)
        *key = slist->iterator->key;
    if (key_len)
        *key_len = slist->iterator->key_len;
    if (value)
        *value = slist->iterator->value;
    slist->iterator = slist->iterator->levels[0].next;
    return 0;
}
void slist_end_iterate(struct slist *slist)
{
    slist->iterator = NULL;
}
void slist_begin_riterate(struct slist *slist)
{
    slist->riterator = slist->tail;
}
int slist_riterate(struct slist *slist, const char **key, uint32_t *key_len, void **value)
{
    if (!slist->riterator)
        return -1;
    if (key)
        *key = slist->riterator->key;
    if (key_len)
        *key_len = slist->riterator->key_len;
    if (value)
        *value = slist->riterator->value;
    slist->riterator = slist->riterator->levels[0].prev;
    if (slist->riterator == slist->head)
        slist->riterator = NULL;
    return 0;
}
void slist_end_riterate(struct slist *slist)
{
    slist->riterator = NULL;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
