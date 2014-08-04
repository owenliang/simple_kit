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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <sys/mman.h>
#include <malloc.h>
#include "smem.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static struct smem smem;

static void _smem_free()
{
	pthread_mutex_lock(&mutex);

	int i;
	for (i = 0; i < sizeof(smem.buckets) / sizeof(smem.buckets[0]); ++i) {
		struct smem_heap_header *heap = smem.buckets[i].first_heap;
		while (heap) {
			struct smem_heap_header *next = heap->next_heap;
			assert(munmap(heap, smem.buckets[i].heap_size) == 0);
			heap = next;
		}
	}

	pthread_mutex_unlock(&mutex);
	// printf("_smem_free\n");
}

static void _smem_prepare()
{
	// printf("_smem_prepare\n");
	pthread_mutex_lock(&mutex);
}

static void _smem_parent()
{
	pthread_mutex_unlock(&mutex);
}

static void _smem_child()
{
	pthread_mutex_unlock(&mutex);
}

/* 在main函数之前完成初始化操作 */
static void _smem_init() __attribute__((constructor));

static void _smem_init()
{
	if (smem.init)
		return;

	smem.init = 1;

	uint32_t chunk_size = 8;

	int i;
	for (i = 0; i < sizeof(smem.buckets) / sizeof(smem.buckets[0]); ++i) {
		uint32_t real_chunk_size = chunk_size + sizeof(void *);
		if (chunk_size < 1024)
			smem.buckets[i].chunk_per_heap =
					(1048576 - sizeof(struct smem_heap_header)) / real_chunk_size;
		else
			smem.buckets[i].chunk_per_heap =
					(2097152 - sizeof(struct smem_heap_header)) / real_chunk_size;

		smem.buckets[i].heap_size =
				 sizeof(struct smem_heap_header) + smem.buckets[i].chunk_per_heap * real_chunk_size;

		smem.buckets[i].chunk_size = chunk_size;
/*
		printf("buckets[%d] chunk_per_heap=%u heap_size=%u chunk_size=%u\n",
				i, smem.buckets[i].chunk_per_heap, smem.buckets[i].heap_size, smem.buckets[i].chunk_size);*/

		chunk_size *= 2;
	}

	pthread_atfork(_smem_prepare, _smem_parent, _smem_child);

	assert(atexit(_smem_free) == 0);

	// printf("_smem_init\n");
}

void smem_stat()
{
	pthread_mutex_lock(&mutex);

	printf("~~~~~~~~~~~smem_stat start~~~~~~~~~~~\n");
	int i;
	for (i = 0; i < sizeof(smem.buckets) / sizeof(smem.buckets[0]); ++i) {
		printf("bucket[%d] total_chunk=%lu used_chunk=%lu heap_size=%u chunk_size=%u chunk_per_heap=%u total_heap=%lu\n",
				i, smem.buckets[i].total_chunk, smem.buckets[i].used_chunk, smem.buckets[i].heap_size, smem.buckets[i].chunk_size,
				smem.buckets[i].chunk_per_heap, smem.buckets[i].total_heap);

		struct smem_heap_header *heap = smem.buckets[i].first_heap;
		uint64_t heap_index = 0;
		while (heap) {
			printf("\theap[%lu] used_chunk=%u total_chunk=%u\n", heap_index, heap->used_chunk, smem.buckets[i].chunk_per_heap);
			heap = heap->next_heap;
			heap_index++;
		}
	}
	printf("~~~~~~~~~~~smem_stat end~~~~~~~~~~~\n\n");
	pthread_mutex_unlock(&mutex);
}

static int _smem_find_buckets(size_t size)
{
	int i;
	for (i = 0; i < sizeof(smem.buckets) / sizeof(smem.buckets[0]); ++i) {
		if (size <= smem.buckets[i].chunk_size)
			return i;
	}
	return -1;
}

static void *_smem_mmap(size_t size)
{
	void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	assert(addr);
	return addr;
}

static void *_smem_alloc_large(size_t size)
{
	// printf("_smem_alloc_large=%lu\n", size);

	size_t real_size = size + sizeof(uint64_t) + sizeof(void *);

	void *addr = _smem_mmap(real_size);
	if (!addr)
		return NULL;

	*(uint64_t *)addr = real_size;
	*(void **)((uint64_t *)addr + 1) = NULL;
	return (char *)addr + sizeof(uint64_t) + sizeof(void *);
}

static void *_smem_alloc_heap(int bucket)
{
	// printf("_smem_alloc_heap=bucket:%d\n", bucket);

	struct smem_heap_header *heap = _smem_mmap(smem.buckets[bucket].heap_size);
	if (!heap)
		return NULL;

	smem.buckets[bucket].total_heap++;
	smem.buckets[bucket].total_chunk += smem.buckets[bucket].chunk_per_heap;

	heap->used_chunk = 0;
	heap->bucket = bucket;
	heap->next_heap = smem.buckets[bucket].first_heap;
	smem.buckets[bucket].first_heap = heap; /* 不需要关心第一个heap的prev */

	if (heap->next_heap)
		heap->next_heap->prev_heap = heap;

	char *cur_chunk = (char *)(heap + 1);
	uint32_t real_chunk_size = smem.buckets[bucket].chunk_size + sizeof(void *);

	heap->free_chunk = NULL;

	int i;
	for (i = 0; i < smem.buckets[bucket].chunk_per_heap; ++i) {
		*(void **)cur_chunk = heap->free_chunk;
		heap->free_chunk = cur_chunk;
		cur_chunk += real_chunk_size;
	}
	return heap;
}

static void *_smem_alloc_small(int bucket)
{
	pthread_mutex_lock(&mutex);

	struct smem_heap_header *heap = smem.buckets[bucket].step_heap;

	uint64_t i;
	for (i = 0; i < smem.buckets[bucket].total_heap; ++i) {
	    if (!heap)
	        heap = smem.buckets[bucket].first_heap;
	    if (heap->free_chunk)
	        break;
	    heap = heap->next_heap;
	}

	if (i == smem.buckets[bucket].total_heap && !(heap = _smem_alloc_heap(bucket)))
		return NULL;

	smem.buckets[bucket].step_heap = heap;

	smem.buckets[bucket].used_chunk++;
	heap->used_chunk++;

	void **addr = heap->free_chunk;
	heap->free_chunk = *addr;
	*addr = heap;

	pthread_mutex_unlock(&mutex);

	// printf("_smem_alloc_small=%p heap=%p\n", addr + 1, heap);

	return addr + 1;
}

static void _smem_free_large(void *ptr)
{
	// printf("_smem_free_large=%p\n", ptr);
	char *addr = (char *)ptr - sizeof(void *) - sizeof(uint64_t);
	uint64_t size = *(uint64_t *)addr;
	assert(munmap(addr, size) == 0);
}

static void _smem_remove_heap(struct smem_heap_header *heap)
{
	struct smem_bucket *bucket = &smem.buckets[heap->bucket];
	// printf("_smem_remove_heap=%p bucket=%u\n", heap, heap->bucket);
	if (heap == bucket->first_heap) {
		bucket->first_heap = heap->next_heap; /* 不需要关心第一个heap的prev */
	} else {
		heap->prev_heap->next_heap = heap->next_heap;
		if (heap->next_heap)
			heap->next_heap->prev_heap = heap->prev_heap;
	}
	if (heap == smem.buckets[heap->bucket].step_heap)
	    smem.buckets[heap->bucket].step_heap = NULL;

	assert(munmap(heap, bucket->heap_size) == 0);
	bucket->total_chunk -= bucket->chunk_per_heap;
	bucket->total_heap--;
}

static void _smem_free_small(void *ptr)
{
	void *chunk = (void **)ptr - 1;

	struct smem_heap_header *heap = *(void **)chunk;

	// printf("_smem_free_small=%p heap=%p\n", ptr, heap);

	uint32_t bucket = heap->bucket;

	pthread_mutex_lock(&mutex);

	/* back to free chunk list */
	*(void **)chunk = heap->free_chunk;
	heap->free_chunk = chunk;

	heap->used_chunk--;
	smem.buckets[bucket].used_chunk--;

	if (!heap->used_chunk &&
			smem.buckets[bucket].total_chunk - smem.buckets[bucket].used_chunk >=
			smem.buckets[bucket].chunk_per_heap << 1)
		_smem_remove_heap(heap);

	pthread_mutex_unlock(&mutex);
}

void *malloc(size_t size)
{
	if (!size)
		return NULL;

	int bucket = _smem_find_buckets(size);
	if (bucket == -1)
		return _smem_alloc_large(size);

	return _smem_alloc_small(bucket);
}

void *calloc(size_t nmemb, size_t size)
{
	size_t total = nmemb * size;
	if (!total)
		return NULL;

	void *addr = malloc(total);
	if (!addr)
		return NULL;

	memset(addr, 0, total);
	return addr;
}

void free(void *ptr)
{
	if (!ptr)
		return;

	void *heap = *((void **)ptr - 1);
	if (!heap)
		_smem_free_large(ptr);
	else
		_smem_free_small(ptr);
}

void *realloc(void *ptr, size_t size)
{
	if (!size) { /* 等于free */
		free(ptr);
		return NULL;
	} else if (!ptr) /* 等于malloc */
		return malloc(size);

	int new_bucket = _smem_find_buckets(size);

	struct smem_heap_header *heap = *((void **)ptr - 1);

	void *new_mem;
	uint64_t old_chunk_size;
	if (heap) { /* small memory */
		old_chunk_size = smem.buckets[heap->bucket].chunk_size;
		if (new_bucket == -1) {
			new_mem = _smem_alloc_large(size);
			if (new_mem)
				_smem_free_small(ptr);
			return new_mem; /* left old memory untouched if new memory fails*/
		} else if (heap->bucket == new_bucket)  /* 新size与旧size属于同一个bucket, 不需重分配 */
			return ptr;

		new_mem = _smem_alloc_small(new_bucket);
		if (!new_mem)
			return NULL;
		if (heap->bucket < new_bucket)
			memcpy(new_mem, ptr, old_chunk_size);
		else if (heap->bucket > new_bucket)
			memcpy(new_mem, ptr, smem.buckets[new_bucket].chunk_size);
		_smem_free_small(ptr);
	} else { /* large memory */
		old_chunk_size = *((uint64_t *)heap - 1);
		if (old_chunk_size == size) /* size unchanged */
			return ptr;
		if (new_bucket == -1) /* new large memory */
			new_mem = _smem_alloc_large(size);
		else /* new small memory*/
			new_mem = _smem_alloc_small(size);
		if (!new_mem)
			return NULL;
		if (size >= old_chunk_size)
			memcpy(new_mem, ptr, old_chunk_size);
		else
			memcpy(new_mem, ptr, size);
		_smem_free_large(ptr);
	}
	return new_mem;
}
