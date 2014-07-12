# 编译器
CC = gcc
AR = ar
CFLAGS = -g -I ./simple_hash -I ./simple_skiplist -I ./simple_deque -I ./simple_config -I ./simple_io -I ./simple_log

# 编译产出目录
OUTPUT_DIR = ./output

# 目标文件
SRC = simple_hash/shash.c simple_skiplist/slist.c simple_deque/sdeque.c \
		  simple_config/sconfig.c simple_log/slog.c simple_io/sio.c \
		  simple_io/sio_buffer.c simple_io/sio_dgram.c simple_io/sio_stream.c \
		  simple_io/sio_timer.c simple_io/sio_proto.c

# 测试程序
TEST_SRC = simple_hash/test_shash.c simple_skiplist/test_slist.c \
		   simple_deque/test_sdeque.c simple_config/test_sconfig.c \
		   simple_log/test_slog.c simple_io/test_sio.c simple_io/test_sio_dgram_client.c \
		   simple_io/test_sio_dgram_server.c simple_io/test_sio_stream_fork_server.c \
		   simple_io/test_sio_stream_client.cpp simple_io/test_sio_stream_multi_server.cpp \
		   simple_io/test_sio_stream_server.cpp

# 头文件
HEADERS = $(SRC:.c=.h)

# 目标文件
OBJECTS = $(SRC:.c=.o)

TEST_OBJECTS = $(TEST_SRC:.c=.o)

# 编译生成静态库和头文件
all:$(OBJECTS)
	mkdir -p $(OUTPUT_DIR)/include
	mkdir -p $(OUTPUT_DIR)/lib
	$(AR) -r $(OUTPUT_DIR)/lib/libskit.a $^
	cp $(HEADERS) $(OUTPUT_DIR)/include

# 生成测试程序
test:$(TEST_OBJECTS)
	mkdir -p $(OUTPUT_DIR)/test

# 令隐式推导检查.h变化
%.o:%.c %.h
	$(CC) -o $@ -c $< $(CFLAGS)

# 清理编译产物
clean:
	rm -rf output
	rm -f $(OBJECTS)
