#ifndef SIMPLE_IO_SIO_PROTO_H
#define SIMPLE_IO_SIO_PROTO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum sio_proto_field_type {
	SIO_PROTO_NONE = 0,
	SIO_PROTO_INT8,
	SIO_PROTO_UINT8,
	SIO_PROTO_INT16,
	SIO_PROTO_UINT16,
	SIO_PROTO_INT32,
	SIO_PROTO_UINT32,
	SIO_PROTO_INT64,
	SIO_PROTO_UINT64,
	SIO_PROTO_FLOAT,
	SIO_PROTO_DOUBLE,
	SIO_PROTO_STRING,
};

struct sio_proto_header {
	 uint64_t id;
	 uint32_t command;
	 uint32_t meta_len;
	 uint32_t body_len;
};

struct sio_proto_meta {
	enum sio_proto_field_type type;
	union {
	    int8_t i8;
		uint8_t u8;
		int16_t i16;
		uint16_t u16;
		int32_t i32;
		uint32_t u32;
		int64_t i64;
		uint64_t u64;
		float f;
		double d;
		struct {
			char *buf;
			uint32_t len;
		} string;
	} value;
	struct sio_proto_meta *next;
};

struct sio_proto_wmessage {
	struct sio_proto_header header;
	struct sio_proto_meta *meta_head;
	struct sio_proto_meta *meta_tail;
};

struct sio_proto_rmessage {
	struct sio_proto_header header;
	struct sio_proto_meta *meta_arr;
};

void sio_proto_wmessage_create(struct sio_proto_wmessage *msg);
void sio_proto_wmessage_destroy(struct sio_proto_wmessage *msg);
void sio_proto_wmessage_set_id(struct sio_proto_wmessage *msg, uint64_t id);
void sio_proto_wmessage_set_command(struct sio_proto_wmessage *msg, uint32_t command);
void sio_proto_wmessage_serialize(struct sio_proto_wmessage *msg, char **buf, uint32_t *len);
void sio_proto_wmessage_put_int8(struct sio_proto_wmessage *msg, int8_t i8);
void sio_proto_wmessage_put_uint8(struct sio_proto_wmessage *msg, uint8_t u8);
void sio_proto_wmessage_put_int16(struct sio_proto_wmessage *msg, int16_t i16);
void sio_proto_wmessage_put_uint16(struct sio_proto_wmessage *msg, uint16_t u16);
void sio_proto_wmessage_put_int32(struct sio_proto_wmessage *msg, int32_t i32);
void sio_proto_wmessage_put_uint32(struct sio_proto_wmessage *msg, uint32_t u32);
void sio_proto_wmessage_put_int64(struct sio_proto_wmessage *msg, int64_t i64);
void sio_proto_wmessage_put_uint64(struct sio_proto_wmessage *msg, int64_t u64);
void sio_proto_wmessage_put_float(struct sio_proto_wmessage *msg, float f);
void sio_proto_wmessage_put_double(struct sio_proto_wmessage *msg, double d);
void sio_proto_wmessage_put_string(struct sio_proto_wmessage *msg, const char *string, uint32_t len);

void sio_proto_rmessage_create(struct sio_proto_rmessage *msg);
void sio_proto_rmessage_destroy(struct sio_proto_rmessage *msg);
int sio_proto_rmessage_unserialize(struct sio_proto_rmessage *msg, const char *buf, uint32_t len);
uint64_t sio_proto_rmessage_get_id(struct sio_proto_rmessage *msg);
uint32_t sio_proto_rmessage_get_command(struct sio_proto_rmessage *msg);

int sio_proto_rmessage_get_int8(struct sio_proto_rmessage *msg, uint32_t index, int8_t *i8);
int sio_proto_rmessage_get_uint8(struct sio_proto_rmessage *msg, uint32_t index, uint8_t *u8);
int sio_proto_rmessage_get_int16(struct sio_proto_rmessage *msg, uint32_t index, int16_t *i16);
int sio_proto_rmessage_get_uint16(struct sio_proto_rmessage *msg, uint32_t index, uint16_t *u16);
int sio_proto_rmessage_get_int32(struct sio_proto_rmessage *msg, uint32_t index, int32_t *i32);
int sio_proto_rmessage_get_uint32(struct sio_proto_rmessage *msg, uint32_t index, uint32_t *u32);
int sio_proto_rmessage_get_int64(struct sio_proto_rmessage *msg, uint32_t index, int64_t *i64);
int sio_proto_rmessage_get_uint64(struct sio_proto_rmessage *msg, uint32_t index, uint64_t *u64);
int sio_proto_rmessage_get_float(struct sio_proto_rmessage *msg, uint32_t index, float *f);
int sio_proto_rmessage_get_double(struct sio_proto_rmessage *msg, uint32_t index, double *d);
int sio_proto_rmessage_get_string(struct sio_proto_rmessage *msg, uint32_t index, const char **string, uint32_t *len);

#ifdef __cplusplus
}
#endif

#endif  //SIMPLE_IO_SIO_PROTO_H

/* vim: set ts=4 sw=4 sts=4 tw=100 */
