FROM ubuntu

RUN apt-get update && apt install -y build-essential bc bison flex libelf-dev libssl-dev libncurses5-dev git
RUN mkdir /build && cd /build && git clone --depth 1 --branch v6.6  git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git 
RUN mkdir /out
COPY build.sh /
CMD [ "/build.sh" ]
COPY .config /build/linux/

