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
test_sconfig:
	make test_sconfig -f $(release)
libsconfig.a:
	make libsconfig.a -f $(release)
