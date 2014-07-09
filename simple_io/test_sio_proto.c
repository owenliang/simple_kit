/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 /**
 * @file test_sio_proto.c
 * @author liangdong(liangdong01@baidu.com)
 * @date 2014/07/08 10:38:07
 * @version $Revision$ 
 * @brief 
 *  
 **/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "sio_proto.h"

int main(int argc, char **argv)
{
    struct sio_proto_wmessage wmsg;

    sio_proto_wmessage_create(&wmsg);

    sio_proto_wmessage_set_id(&wmsg, 1234);
    sio_proto_wmessage_set_command(&wmsg, 2);

    sio_proto_wmessage_put_boolean(&wmsg, 1);
    sio_proto_wmessage_put_int32(&wmsg, -5);
    sio_proto_wmessage_put_uint32(&wmsg, 5);
    sio_proto_wmessage_put_int64(&wmsg, -5);
    sio_proto_wmessage_put_uint64(&wmsg, 5);
    sio_proto_wmessage_put_float(&wmsg, 2.542);
    sio_proto_wmessage_put_double(&wmsg, 2.12345);
    sio_proto_wmessage_put_string(&wmsg, "hello", 5);

    char *buf;
    uint32_t len;
    sio_proto_wmessage_serialize(&wmsg, &buf, &len);
    sio_proto_wmessage_destroy(&wmsg);

    printf("wmessage -> meta_len=%u body_len=%u total_len=%u\n",
            wmsg.header.meta_len, wmsg.header.body_len, len);

    struct sio_proto_rmessage rmsg;
    sio_proto_rmessage_create(&rmsg);
    int ret = sio_proto_rmessage_unserialize(&rmsg, buf, len);
    if (ret == 0) {
        printf("rmessage -> message not full\n");
    } else if (ret == -1) {
        printf("rmessage -> message invalid\n");
    } else {
        printf("rmessage -> meta_len=%u body_len=%u total_len=%u\n",
                rmsg.header.meta_len, rmsg.header.body_len, ret);
        uint64_t id = sio_proto_rmessage_get_id(&rmsg);
        uint32_t command = sio_proto_rmessage_get_command(&rmsg);

        uint8_t boolean;
        assert(sio_proto_rmessage_get_boolean(&rmsg, 0, &boolean) == 0);
        int32_t i32;
        assert(sio_proto_rmessage_get_int32(&rmsg, 1, &i32) == 0);
        uint32_t u32;
        assert(sio_proto_rmessage_get_uint32(&rmsg, 2, &u32) == 0);
        int64_t i64;
        assert(sio_proto_rmessage_get_int64(&rmsg, 3, &i64) == 0);
        uint64_t u64;
        assert(sio_proto_rmessage_get_uint64(&rmsg, 4, &u64) == 0);
        float f;
        assert(sio_proto_rmessage_get_float(&rmsg, 5, &f) == 0);
        double d;
        assert(sio_proto_rmessage_get_double(&rmsg, 6, &d) == 0);
        const char *string;
        uint32_t len;
        assert(sio_proto_rmessage_get_string(&rmsg, 7, &string, &len) == 0);
        printf("id=%lu command=%u boolean=%u i32=%d u32=%u i64=%ld u64=%lu "
                "f=%f d=%lf string=%s\n", id, command, boolean, i32, u32, i64, u64, f, d, string);
    }
    free(buf);
    sio_proto_rmessage_destroy(&rmsg);

    return 0;
}

/* vim: set ts=4 sw=4 sts=4 tw=100 */
