FROM ubuntu as kernel
RUN apt-get update && apt install -y build-essential bc bison flex libelf-dev libssl-dev libncurses5-dev git
RUN mkdir /build && cd /build && git clone --depth 1 --branch v6.6  git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git 
COPY infra/kernel/.config /build/linux/
RUN cd /build/linux && make -j10
RUN cd /build/linux && make scripts_gdb

FROM kernel as cremona
RUN mkdir /build/cremona
COPY module/ /build/cremona
RUN cd /build/cremona && make

FROM ubuntu AS busybox
RUN apt-get update && apt install -y build-essential bc bison flex libelf-dev libssl-dev libncurses5-dev git
RUN mkdir /build && cd /build && git clone --depth 1 --branch 1_36_1  git://git.busybox.net/busybox
RUN mkdir /initramfs
COPY infra/initramfs/.config /build/busybox/
RUN cd /build/busybox && make -j10 && make CONFIG_PREFIX=/initramfs install

FROM golang AS userland
RUN mkdir /build
COPY userland/ /build
RUN cd /build && make build

FROM ubuntu AS initramfs
RUN  apt-get update && apt install -y cpio
RUN mkdir /initramfs
COPY --from=busybox /initramfs/ /initramfs
RUN mkdir /initramfs/etc && mkdir /initramfs/proc && mkdir /initramfs/dev && mkdir /initramfs/sys
COPY infra/initramfs/fstab /initramfs/etc/
COPY infra/initramfs/mdev.conf /initramfs/etc/
COPY infra/initramfs/init /initramfs/
COPY infra/initramfs/cremona.json /initramfs/

FROM initramfs AS build_initramfs
COPY --from=cremona  /build/cremona/cremona.ko /initramfs/
COPY --from=userland /build/userland/cmd/cremona_daemon/cremona_daemon /initramfs/
COPY --from=userland /build/userland/cmd/driver_test /initramfs/
RUN ( cd /initramfs ; find . -print0 | cpio -o -a0v -H newc ) | gzip > /initramfs.gz

FROM ubuntu AS qemu
RUN apt-get update && apt install -y qemu-system-x86
RUN apt install -y tmux gdb
RUN mkdir /dest /build /build/cremona /build/linux
COPY infra/qemu/run.sh /
CMD [ "/run.sh" ]
COPY --from=kernel /build/linux/ /build/linux/
COPY --from=cremona /build/cremona/ /build/cremona/
COPY --from=kernel /build/linux/arch/x86/boot/bzImage /dest
COPY --from=initramfs /initramfs.gz /dest


FROM kernel AS devcontainer
RUN apt-get update && apt install -y qemu-system-x86
RUN apt install -y tmux gdb cpio
COPY --from=initramfs /initramfs /initramfs
RUN mkdir -p /dest /root/.config/gdb/
RUN echo "set auto-load safe-path /build" > /root/.config/gdb/gdbinit