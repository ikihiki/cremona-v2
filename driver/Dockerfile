FROM ubuntu

RUN apt-get update && apt install -y build-essential bc bison flex libelf-dev libssl-dev libncurses5-dev git
RUN mkdir /build && cd /build && git clone --depth 1 --branch v6.6  git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
COPY .devcontainer/.config /build/linux/
RUN cd /build/linux && make
RUN mkdir /build/cremona && mkdir /build/cremona/build && mkdir /build/cremona/src && mkdir /out
COPY ./src/ /build/cremona/src
COPY ./Makefile /build/cremona/
WORKDIR /build/cremona
CMD [ "/usr/bin/make", "out" ]