${QEMU_PATH}/qemu-system-x86_64 -enable-kvm -kernel vmlinuz -initrd initramfs.gz \
	-chardev stdio,id=char0 -serial chardev:char0 \
	-append 'quiet console=ttyS0,115200 iomem=relaxed rdinit=/init.sh' \
	-display none -m 512 -nodefaults \
	-smp sockets=1,cores=4 \
	-device gpu
