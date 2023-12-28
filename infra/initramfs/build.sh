#!/bin/bash


( cd /initramfs ; find . -print0 | cpio -o -a0v -H newc ) | gzip > /out/initramfs.gz