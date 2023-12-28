$ErrorActionPreference = "Stop"
docker build -t ikihiki80/cremona-qemu  .
docker run -it --rm  ikihiki80/cremona-qemu