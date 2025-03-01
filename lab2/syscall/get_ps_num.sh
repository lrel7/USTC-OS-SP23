#!/bin/bash
gcc -static -o get_ps_num get_ps_num.c
sudo cp get_ps_num ~/oslab/busybox-1.32.1/_install/
cd ~/oslab/busybox-1.32.1/_install/
find . -print0 | cpio --null -ov --format=newc | gzip -9 > ~/oslab/initramfs-busybox-x64.cpio.gz
cd ~/oslab/linux-4.9.263
make -j $((`nproc`-1))
bash ~/oslab/qemu_nogdb.sh