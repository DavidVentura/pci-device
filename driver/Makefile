obj-m += gpu.o
gpu-objs += gpu_pcie.o gpu_char.o
ccflags-y=-I/home/david/git/pci-device/include

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
