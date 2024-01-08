build-module:
	$(MAKE) -C $$PWD/module

build-userland:
	$(MAKE) -C $$PWD/userland

build-initramfs: build-module build-userland
	cp module/cremona.ko /initramfs/
	cp userland/cmd/cremona_daemon/cremona_daemon /initramfs/
	cp userland/cmd/driver_test/driver_test /initramfs/
	cp infra/initramfs/init /initramfs/
	( cd /initramfs ; find . -print0 | cpio -o -a0v -H newc ) | gzip > /dest/initramfs.gz

build: build-initramfs
	cp /build/linux/arch/x86/boot/bzImage /dest/

run: build
	qemu-system-x86_64 -kernel /dest/bzImage -initrd /dest/initramfs.gz -append "console=ttyS0" -nographic

debug: build
	qemu-system-x86_64 -kernel /dest/bzImage -initrd /dest/initramfs.gz -append "console=ttyS0" -nographic  -gdb tcp::12345 -S

install:
	$(MAKE) -C $$PWD/module install
	$(MAKE) -C $$PWD/userland install

clean:
	$(MAKE) -C $$PWD/module clean
	$(MAKE) -C $$PWD/userland clean
