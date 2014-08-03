#ifndef SIMPLE_MEMPOOL_SMEM_H
#define SIMPLE_MEMPOOL_SMEM_H

#ifdef __cplusplus
extern "C" {
#endif

/* 内部头文件, 用户不需要包含该头文件 */

#include <stdint.h>

struct smem_heap_header;

/* 存储特定尺寸的chunk的容器 */
struct smem_bucket {
	uint64_t used_chunk;	 /* bucket中已使用的chunk数 */
	uint64_t total_chunk;  /* bucket中总共分配的chunk数 */

	uint32_t heap_size; /* 一个heap是通过mmap申请的, 包换chunk_per_heap个chunk */
	uint32_t chunk_size; /* 每个chunk的尺寸, 不包含头部信息 */
	uint32_t chunk_per_heap; /* 每个heap里有多少个chunk */

	uint64_t total_heap; /* 该bucket中总共有多少个heap */
	struct smem_heap_header *first_heap; /* heap组成的双向链表 */
};

/* 一个heap的元信息, 也是heap内存的起始地址 */
struct smem_heap_header {
	uint32_t used_chunk; /* heap中总共已使用的chunk数 */
	uint32_t bucket; /* heap属于第几层bucket */
	struct smem_heap_header *prev_heap; /* 前驱heap */
	struct smem_heap_header *next_heap; /* 后继heap */
	void *free_chunk; /* heap中的空闲chunk链表 */
};

/*
 * 内存池, 维护17个bucket, 每个bucket中是若干通过mmap申请的heap,
 * 每个heap被划分成若干chunk, 空闲chunk通过双向链表的形式串联.
 *
 * 	17层bucekt的small chunk尺寸如下:
 * 	8, 16, 32, 64, 128, 256, 512, 1K, 2K, 4K, 8K, 16K, 32K, 64K,
 * 	128K, 256K, 512K
 *
 * 	chunk的数据结构为:
 * 	1, 对于small chunk(<=512K): 8 字节的struct smem_heap_header *(NOT NULL) + data segment
 * 	2, 对于larger chunk(>512K): 8字节的len记录整个内存块长度 + 8字节struct smem_heap_header *(NULL) + data segment.
 *
 * */
struct smem {
	char init; /* 只在父进程初始化一次, 之后fork子进程不再重新初始化. */
	struct smem_bucket buckets[17]; /* small chunk 的17级buckets */
};

/* 用户接口, 可以打印内存池的调试信息 */
void smem_stat();

#ifdef __cplusplus
}
#endif

#endif
