#ifndef SIMPLE_MEMPOOL_SMEM_H
#define SIMPLE_MEMPOOL_SMEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* 内部头文件, 用户不需要包含该头文件 */

#include <stdint.h>

struct smem_heap_header;

struct smem_bucket {
	uint64_t used_chunk;
	uint64_t total_chunk;

	uint32_t heap_size;
	uint32_t chunk_size;
	uint32_t chunk_per_heap;

	uint64_t total_heap;
	struct smem_heap_header *first_heap;
};

struct smem_heap_header {
	uint32_t used_chunk;
	uint32_t bucket;
	struct smem_heap_header *next_heap;
	void *free_chunk;
};

/*
 * 	8, 16, 32, 64, 128, 256, 512, 1K, 2K, 4K, 8K, 16K, 32K, 64K,
 * 	128K, 256K, 512K
 * */
struct smem {
	char init;
	struct smem_bucket buckets[17];
};

void smem_stat();

#ifdef __cplusplus
}
#endif

#endif
