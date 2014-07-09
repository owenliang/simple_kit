#COMAKE2 edit-mode: -*- Makefile -*-
HARDWARE_PLATFORM := $(shell uname -m)
ifeq ($(HARDWARE_PLATFORM),x86_64)
    release=Makefile.64
else
    release=Makefile.32
endif
all:
	make -f $(release)
clean:
	make clean -f $(release)
dist:
	make dist -f $(release)
test_sio:
	make test_sio -f $(release)
test_sio_stream_fork_server:
	make test_sio_stream_fork_server -f $(release)
test_sio_stream_server:
	make test_sio_stream_server -f $(release)
test_sio_stream_multi_server:
	make test_sio_stream_multi_server -f $(release)
test_sio_stream_client:
	make test_sio_stream_client -f $(release)
test_sio_dgram_server:
	make test_sio_dgram_server -f $(release)
test_sio_dgram_client:
	make test_sio_dgram_client -f $(release)
libsio.a:
	make libsio.a -f $(release)
