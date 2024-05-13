# pci-device

## Hacking

1. get a kernel, name it `vmlinuz`
2. set up qemu
	1. clone qemu
	2. apply `adapter/emulated/qemu.patch`
	3. symlink `adapter/emulated/pcie-gpu.c` to QEMU's `hw/misc/`
	4. build it
3. generate initramfs with `build-initramfs.sh`
4. run a VM
	```bash
	export QEMU_PATH=../qemu/build
	```
	run `run.sh` to start a vm
