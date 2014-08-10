#ifndef SIMPLE_PACK_SPACK_H
#define SIMPLE_PACK_SPACK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 序列化 */
struct spack_w {
	char *buf;	/* 用户提供缓冲区用于存储序列化数据 */
	uint64_t buf_used;	/* buf使用的长度 */
	uint64_t buf_capacity; /* buf的容量, 超过容量将序列化失败 */
};

/* 反序列化 */
struct spack_r {
	const char *buf; /* 用户提供待反序列化的数据 */
	uint64_t buf_used; /* 已反序列化的长度 */
	uint64_t buf_size; /* 用户提供的数据总长度 */
};

/* 序列化API */
void spack_w_init(struct spack_w *wpack, char *buf, uint64_t buf_capacity);
int spack_put_nil(struct spack_w *wpack);
int spack_put_boolean(struct spack_w *wpack, uint8_t is_true);
int spack_put_int(struct spack_w *wpack, int64_t i64);
int spack_put_uint(struct spack_w *wpack, uint64_t u64);
int spack_put_bin(struct spack_w *wpack, const char *data, uint32_t len);
int spack_put_float(struct spack_w *wpack, float f);
int spack_put_double(struct spack_w *wpack, double d);
int spack_put_ext(struct spack_w *wpack, int8_t type, const void *data, uint32_t size);
int spack_put_str(struct spack_w *wpack, const char *data, uint32_t size);
int spack_put_array(struct spack_w *wpack, uint32_t size);
int spack_put_map(struct spack_w *wpack, uint32_t size);

/* 反序列化API */
void spack_r_init(struct spack_r *rpack, const char *buf, uint64_t buf_size);
int spack_get_nil(struct spack_r *rpack);
int spack_get_boolean(struct spack_r *rpack, uint8_t *is_true);
int spack_get_int(struct spack_r *rpack, int64_t *i64);
int spack_get_uint(struct spack_r *rpack, uint64_t *u64);
int spack_get_bin(struct spack_r *rpack, const char **data, uint32_t *len);
int spack_get_float(struct spack_r *rpack, float *f);
int spack_get_double(struct spack_r *rpack, double *d);
int spack_get_ext(struct spack_r *rpack, int8_t *type, const char **data, uint32_t *size);
int spack_get_str(struct spack_r *rpack, const char **str, uint32_t *size);
int spack_get_array(struct spack_r *rpack, uint32_t *size);
int spack_get_map(struct spack_r *rpack, uint32_t *size);

#ifdef __cplusplus
}
#endif

#endif
