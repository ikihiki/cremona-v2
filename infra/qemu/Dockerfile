FROM ubuntu
RUN apt-get update && apt install -y qemu-system-x86
RUN mkdir /dest
COPY run.sh /
CMD [ "/run.sh" ]