build-driver:
	cd driver && make all

build-driver-test:
	cd driver_test && make all

build-initramfs: build-driver build-driver-test
	cp driver/build/cremona.ko /initramfs/
	cp driver_test/driver_test /initramfs/
	cp infra/initramfs/init /initramfs/
	( cd /initramfs ; find . -print0 | cpio -o -a0v -H newc ) | gzip > /initramfs.gz

build: build-initramfs
	cp /build/linux/arch/x86/boot/bzImage /dest/
	cp /initramfs.gz /dest/