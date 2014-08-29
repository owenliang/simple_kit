# 编译器
CC = gcc
CPP = g++
AR = ar
CFLAGS = -g -Wall -I ./simple_hash -I ./simple_skiplist -I ./simple_deque -I ./simple_config -I ./simple_io -I ./simple_log -I ./simple_spack -I ./simple_head
LDFLAGS = -lrt -pthread -I./output/include -L./output/lib -lskit

# 编译产出目录
OUTPUT_DIR = ./output

# 目标文件
SRC = simple_hash/shash.c simple_skiplist/slist.c simple_deque/sdeque.c \
		  simple_config/sconfig.c simple_log/slog.c simple_io/sio.c simple_io/sio_rpc.c \
		  simple_io/sio_buffer.c simple_io/sio_dgram.c simple_io/sio_stream.c simple_io/sio_rpc.c \
		  simple_io/sio_timer.c simple_pack/spack.c simple_head/shead.c

# 测试程序
TEST_SRC_C = simple_hash/test_shash.c simple_skiplist/test_slist.c \
		   simple_deque/test_sdeque.c simple_config/test_sconfig.c \
		   simple_log/test_slog.c simple_io/test_sio.c simple_io/test_sio_dgram_client.c \
		   simple_io/test_sio_dgram_server.c simple_io/test_sio_stream_fork_server.c \
		   simple_io/test_sio_stream_server.c simple_io/test_sio_stream_client.c simple_io/test_sio_rpc.c \
		   simple_io/test_sio_stream_multi_server.c simple_pack/test_spack.c simple_head/test_shead.c

TEST_SRC_CPP = 

# 测试用例
TEST_CASE = simple_config/test_cases

# 头文件
HEADERS = $(SRC:.c=.h)

# 目标文件
OBJECTS = $(SRC:.c=.o)

# 测试程序目标文件
TEST_OBJECTS_C = $(TEST_SRC_C:.c=.o)
TEST_OBJECTS_CPP = $(TEST_SRC_CPP:.cpp=.o)

# 测试程序二进制
TEST_BIN_C = $(TEST_OBJECTS_C:.o=)
TEST_BIN_CPP = $(TEST_OBJECTS_CPP:.o=)

# 编译生成静态库和头文件
all:$(OBJECTS)
	mkdir -p $(OUTPUT_DIR)/include
	mkdir -p $(OUTPUT_DIR)/lib
	$(AR) -r $(OUTPUT_DIR)/lib/libskit.a $^
	cp -f $(HEADERS) $(OUTPUT_DIR)/include

# 生成测试程序
test:$(TEST_BIN_C) $(TEST_BIN_CPP)
	mkdir -p $(OUTPUT_DIR)/test
	cp -rf $(TEST_CASE) $(TEST_BIN_C) $(TEST_BIN_CPP) $(OUTPUT_DIR)/test

$(TEST_BIN_C) $(TEST_BIN_CPP):all

# 令隐式推导检查.h变化
%.o:%.c %.h
	$(CC) -o $@ -c $< $(CFLAGS)

# C测试程序
%:%.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS)

# CPP测试程序
%:%.cpp
	$(CPP) -o $@ $< $(CFLAGS) $(LDFLAGS)

# 清理编译产物
clean:
	rm -rf output
	rm -f $(OBJECTS)
	rm -f $(TEST_OBJECTS_C)
	rm -f $(TEST_OBJECTS_CPP)
	rm -f $(TEST_BIN_C)
	rm -f $(TEST_BIN_CPP)
