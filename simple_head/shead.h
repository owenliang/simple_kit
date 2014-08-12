#ifndef SIMPLE_HEAD_SHEAD_H
#define SIMPLE_HEAD_SHEAD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 检查包正确性的magic_num */
#define SHEAD_MAGIC_NUM 0xF2A1C2CC
/* 序列化head的长度 */
#define SHEAD_ENCODE_SIZE 24

/* 消息头 */
struct shead {
    uint64_t id;    /* 消息唯一标识 */
    uint32_t type;  /* 消息类型 */
    uint32_t magic_num; /* 魔数, 检验包是否被合法 */
    uint32_t reserved;  /* 保留字段 */
    uint32_t body_len;  /* 包体长度 */
};

/**
 * @brief 序列化一个消息头到output中, output需要SHEAD_ENCODE_SIZE大小
 *
 * @param [in] head   : const struct shead*
 * @param [in] output   : char*
 * @param [in] len   : uint32_t
 * @return  int 失败返回-1,成功返回0
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/12 13:24:25
**/
int shead_encode(const struct shead *head, char *output, uint32_t len);
/**
 * @brief 反序列化一个消息头到input中, input至少output为SHEAD_ENCODE_SIZE大小
 *
 * @param [in] head   : struct shead*
 * @param [in] input   : const char*
 * @param [in] len   : uint32_t
 * @return  int 
 * @retval   
 * @see 
 * @author liangdong
 * @date 2014/08/12 13:25:11
**/
int shead_decode(struct shead *head, const char *input, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif
