FROM ubuntu as kernel
RUN apt-get update && apt install -y build-essential bc bison flex libelf-dev libssl-dev libncurses5-dev git
RUN mkdir /build && cd /build && git clone --depth 1 --branch v6.6  git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git 
COPY infra/kernel/.config /build/linux/
RUN cd /build/linux && make -j10
RUN cd /build/linux && make scripts_gdb

FROM kernel as cremona
RUN mkdir /build/cremona && mkdir /build/cremona/build && mkdir /build/cremona/src && mkdir /out
COPY driver/src/ /build/cremona/src
COPY driver/Makefile /build/cremona/
RUN cd /build/cremona && make  all

FROM ubuntu as busybox
RUN apt-get update && apt install -y build-essential bc bison flex libelf-dev libssl-dev libncurses5-dev git
RUN mkdir /build && cd /build && git clone --depth 1 --branch 1_36_1  git://git.busybox.net/busybox
RUN mkdir /initramfs
COPY infra/initramfs/.config /build/busybox/
RUN cd /build/busybox && make -j10 && make CONFIG_PREFIX=/initramfs install

FROM golang as driver_test
RUN mkdir /build
COPY driver_test/ /build
RUN cd /build && ./build.sh

FROM ubuntu as initramfs
RUN  apt-get update && apt install -y cpio
RUN mkdir /initramfs
COPY --from=busybox /initramfs/ /initramfs
RUN mkdir /initramfs/etc && mkdir /initramfs/proc && mkdir /initramfs/dev && mkdir /initramfs/sys
COPY infra/initramfs/fstab /initramfs/etc/
COPY infra/initramfs/init /initramfs/
COPY --from=cremona /build/cremona/build/cremona.ko /initramfs/
COPY --from=driver_test /build/driver_test /initramfs/
RUN ( cd /initramfs ; find . -print0 | cpio -o -a0v -H newc ) | gzip > /initramfs.gz

FROM ubuntu as qemu
RUN apt-get update && apt install -y qemu-system-x86
RUN apt install -y tmux gdb
RUN mkdir /dest /build /build/cremona /build/linux
COPY infra/qemu/run.sh /
CMD [ "/run.sh" ]
COPY --from=kernel /build/linux/ /build/linux/
COPY --from=cremona /build/cremona/ /build/cremona/
COPY --from=kernel /build/linux/arch/x86/boot/bzImage /dest
COPY --from=initramfs /initramfs.gz /dest

