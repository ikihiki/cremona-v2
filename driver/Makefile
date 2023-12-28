KDIR := /build/linux
PWD := $(shell pwd)
BUILD_DIR := $(PWD)/build
SRC_DIR := $(PWD)/src
TARGET ?= $(BUILD_DIR)/cremona.ko
ccflags-y := -I$(PWD)/include -g

obj-m := cremona.o
cremona-objs := src/main.o src/cremona.o src/repeater.o src/netlink.o

$(TARGET): $(BUILD_DIR)
	make -C $(KDIR) M=$(BUILD_DIR) src=$(CURDIR) modules

$(BUILD_DIR):
	mkdir -p "$@"
	touch "$@/Makefile"


.PHONY: build all clean install
build: $(TARGET)

all: clean build

clean:
	make -C $(KDIR) M=$(BUILD_DIR) src=$(CURDIR) clean
	rm -r $(BUILD_DIR)

install:
	make -C $(KDIR) M=$(BUILD_DIR) src=$(CURDIR) modules_install

out: build
	cp $(TARGET) /out