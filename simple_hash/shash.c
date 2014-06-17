/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file shash.c
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/06/16 19:19:18
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include "shash.h"

struct shash *shash_new()
{
    struct shash *shash = calloc(1, sizeof(*shash));
    shash->num_buckets[0] = SHASH_INIT_NUM_BUCKETS;
    shash->buckets[0] = calloc(1ULL << SHASH_INIT_NUM_BUCKETS, sizeof(struct shash_node));
    return shash;
}

static struct shash_node *_shash_new_node(const char *key, uint32_t key_len, void *value)
{
    struct shash_node *node = calloc(1, sizeof(*node));
    node->key = malloc(key_len);
    memcpy(node->key, key, key_len);
    node->key_len = key_len;
    node->value = value;
    return node;
}

static void _shash_free_node(struct shash_node *node)
{
    free(node->key);
    free(node);
}

void shash_free(struct shash *shash)
{
    int i;
    for (i = 0; i < 2; ++i) {
        struct shash_node *buckets = shash->buckets[i];
        if (!buckets)
            break;
        uint64_t num_buckets = 1ULL << shash->num_buckets[i];
        uint64_t b;
        for (b = 0; b < num_buckets; ++b) {
            struct shash_node *node = buckets[b].next;
            while (node) {
                struct shash_node *next = node->next;
                _shash_free_node(node);
                node = next;
            }
        }
        free(buckets);
    }
    free(shash);
}

static uint64_t _shash_hash_key(const char *key, uint32_t key_len)
{
    uint64_t hash = 0;
    uint32_t i;
    for (i = 0; i < key_len; ++i)
        hash = hash * 33 + (uint8_t)key[i];
    return hash;
}

static int _shash_key_compare(const char *key1, uint32_t key1_len, const char *key2, uint32_t key2_len)
{
    int min_len = key1_len < key2_len ? key1_len : key2_len;
    int ret = memcmp(key1, key2, min_len);
    if (ret)
        return ret;
    if (key1_len == key2_len)
    	return 0;
    return key1_len < key2_len ? -1 : 1;
}

static void _shash_check_rehash(struct shash *shash)
{
	/* 只在Iterator关闭的情况下才启动rehash */
	if (!shash->buckets[1] && !shash->iterator) {
		uint64_t num_buckets = 1ULL << shash->num_buckets[0];
		if (shash->num_node >= num_buckets &&
				shash->num_buckets[0] + 1 <= SHASH_MAX_NUM_BUCKETS) {
			shash->num_buckets[1] = shash->num_buckets[0] + 1;
			shash->buckets[1] = calloc(1ULL << shash->num_buckets[1], sizeof(struct shash_node));
			shash->rehash_bucket = 0;
		}
	}
	/* 只在iterator关闭的情况下执行rehash, 否则会造成用户迭代重复数据 */
	if (shash->buckets[1] && !shash->iterator) {
		struct shash_node *head = &shash->buckets[0][shash->rehash_bucket];
		struct shash_node *node = head->next;
		uint64_t buckets_mask = (1ULL << shash->num_buckets[1]) - 1;
		while (node) {
			uint64_t hash = _shash_hash_key(node->key, node->key_len);
			uint64_t bucket_index = hash & buckets_mask;
			struct shash_node *to = &shash->buckets[1][bucket_index];
			while (to->next) {
				int ret = _shash_key_compare(node->key, node->key_len, to->next->key, to->next->key_len);
				if (ret <= 0)
					break;
				to = to->next;
			}
			struct shash_node *next = node->next;
			node->next = to->next;
			if (to->next)
				to->next->prev = node;
			to->next = node;
			node->prev = to;

			node = next;
		}
		head->next = NULL;
		/* rehash结束 */
		if (++shash->rehash_bucket == (1ULL << shash->num_buckets[0])) {
			free(shash->buckets[0]);
			shash->buckets[0] = shash->buckets[1];
			shash->buckets[1] = NULL;
			shash->num_buckets[0] = shash->num_buckets[1];
		}
	}
}

int shash_insert(struct shash *shash, const char *key, uint32_t key_len, void *value)
{
	_shash_check_rehash(shash);
    uint64_t hash = _shash_hash_key(key, key_len);

    struct shash_node *buckets = shash->buckets[0];
    uint64_t bucket_index = hash & ((1ULL << shash->num_buckets[0]) - 1);

    if (shash->buckets[1] && bucket_index < shash->rehash_bucket) {
        buckets = shash->buckets[1];
        bucket_index = hash & ((1ULL << shash->num_buckets[1]) - 1);
    }

    struct shash_node *node = &buckets[bucket_index];
    while (node->next) {
    	int ret = _shash_key_compare(key, key_len, node->next->key, node->next->key_len);
    	if (ret == 0)
    		return -1;
    	else if (ret < 0)
    		break;
    	node = node->next;
    }

    struct shash_node *new_node = _shash_new_node(key, key_len, value);
    new_node->next = node->next;
    if (node->next)
    	node->next->prev = new_node;
    node->next = new_node;
    new_node->prev = node;
    ++shash->num_node;
    return 0;
}

static struct shash_node *_shash_find(struct shash *shash, const char *key, uint32_t key_len)
{
    uint64_t hash = _shash_hash_key(key, key_len);
    struct shash_node *buckets = shash->buckets[0];
    uint64_t bucket_index = hash & ((1ULL << shash->num_buckets[0]) - 1);

    if (shash->buckets[1] && bucket_index < shash->rehash_bucket) {
        buckets = shash->buckets[1];
        bucket_index = hash & ((1ULL << shash->num_buckets[1]) - 1);
    }

    struct shash_node *node = &buckets[bucket_index];
    while (node->next) {
    	int ret = _shash_key_compare(key, key_len, node->next->key, node->next->key_len);
    	if (ret == 0)
    		return node->next;
    	else if (ret < 0)
    		break;
    	node = node->next;
    }
    return NULL;
}

int shash_erase(struct shash *shash, const char *key, uint32_t key_len)
{
	_shash_check_rehash(shash);
	struct shash_node *node = _shash_find(shash, key, key_len);
	if (!node)
		return -1;
	/* 删除的节点是迭代器指向的节点 */
	if (node == shash->iterator) {
		shash_iterate(shash, NULL, NULL, NULL);
	}
	if (node->next)
		node->next->prev = node->prev;
	node->prev->next = node->next;
	_shash_free_node(node);
	--shash->num_node;
	return 0;
}

int shash_find(struct shash *shash, const char *key, uint32_t key_len, void **value)
{
	_shash_check_rehash(shash);
	struct shash_node *node = _shash_find(shash, key, key_len);
	if (!node)
		return -1;
	if (value)
		*value = node->value;
	return 0;
}

void shash_begin_iterate(struct shash *shash)
{
	int i;
	for (i = 0; i < 2; ++i) {
		if (!shash->buckets[i])
			break;
		struct shash_node *buckets = shash->buckets[i];
		uint64_t num_buckets = 1ULL << shash->num_buckets[i];
		uint64_t b;
		for (b = 0; b < num_buckets; ++b) {
			if (buckets[b].next) {
				shash->iterator_index = i;
				shash->iterator_bucket = b;
				shash->iterator = buckets[b].next;
				return;
			}
		}
	}
	shash->iterator = NULL;
}

int shash_iterate(struct shash *shash, const char **key, uint32_t *key_len, void **value)
{
	if (!shash->iterator)
		return -1;
	if (key)
	    *key = shash->iterator->key;
	if (key_len)
	    *key_len = shash->iterator->key_len;
	if (value)
	    *value = shash->iterator->value;

	shash->iterator = shash->iterator->next;
	if (!shash->iterator) {
	    ++shash->iterator_bucket;
	    uint64_t num_buckets = 1ULL << shash->num_buckets[shash->iterator_index];
	    int i;
	    for (i = shash->iterator_index; i < 2; ++i) {
	        struct shash_node *buckets = shash->buckets[i];
	        if (!buckets)
	            break;
	        uint64_t b;
	        for (b = shash->iterator_bucket; b < num_buckets; ++b) {
	            if (buckets[b].next) {
	                shash->iterator_bucket = b;
	                shash->iterator = buckets[b].next;
	                shash->iterator_index = i;
	                return 0;
	            }
	        }
	        if (i == 0) {
	            shash->iterator_bucket = 0;
	        }
	    }
	}
	return 0;
}

void shash_end_iterate(struct shash *shash)
{
	shash->iterator = NULL;
}

uint64_t shash_size(struct shash *shash)
{
    return shash->num_node;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
