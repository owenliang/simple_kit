#COMAKE2 edit-mode: -*- Makefile -*-
HARDWARE_PLATFORM := $(shell uname -m)
ifeq ($(HARDWARE_PLATFORM),x86_64)
    release=Makefile.64
else
    release=Makefile.32
endif
ifeq ($(MAC),ARM32)
    release=Makefile.arm32
endif
all:
	make -f $(release)
clean:
	make clean -f $(release)
dist:
	make dist -f $(release)
test_slog:
	make test_slog -f $(release)
libslog.a:
	make libslog.a -f $(release)
