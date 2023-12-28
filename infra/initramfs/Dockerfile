FROM ubuntu

RUN apt-get update && apt install -y build-essential bc bison flex libelf-dev libssl-dev libncurses5-dev git
RUN  apt install -y cpio
RUN mkdir /build && cd /build && git clone --depth 1 --branch 1_36_1  git://git.busybox.net/busybox
RUN mkdir /out
RUN mkdir /initramfs
COPY .config /build/busybox/
RUN cd /build/busybox && make -j10 && make CONFIG_PREFIX=/initramfs install
RUN mkdir /initramfs/etc && mkdir /initramfs/proc && mkdir /initramfs/dev && mkdir /initramfs/sys
COPY fstab /initramfs/etc/
COPY build.sh /
CMD [ "/build.sh" ]
COPY init /initramfs/
COPY cremona.ko /initramfs/
