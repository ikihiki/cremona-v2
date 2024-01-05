#!/bin/bash

qemu-system-x86_64 -kernel /dest/bzImage -initrd /dest/initramfs.gz -append "console=ttyS0" -nographic  -gdb tcp::12345 