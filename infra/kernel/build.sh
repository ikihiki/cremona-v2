#!/bin/bash

cd /build/linux
make -j10 bzImage
cp -f arch/x86/boot/bzImage /out/