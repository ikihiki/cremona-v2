# KDIR := /build/linux
# PWD := $(shell pwd)
# BUILD_DIR := $(PWD)/build
# SRC_DIR := $(PWD)/src
# TARGET ?= $(BUILD_DIR)/cremona.ko
# ccflags-y := -I$(PWD)/include -g

obj-m := cremona.o
cremona-objs := main.o cremona_main.o repeater.o netlink.o command_buffer.o

ifneq ($(KERNELRELEASE),)
KVER ?= $(shell uname -r)
endif
KDIR ?= /usr/lib/modules/$(KVER)/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean
install:
	$(MAKE) -C $(KDIR) M=$$PWD  modules_install
