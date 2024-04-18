#!/bin/bash
qemu-system-x86_64 -s -S -kernel ~/oslab/linux-4.9.263/arch/x86_64/boot/bzImage -initrd ~/oslab/initramfs-busybox-x64.cpio.gz --append "nokaslr root=/dev/ram init=/init console=ttyS0 " -nographic
