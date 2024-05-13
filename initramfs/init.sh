#!/busybox sh
/busybox mkdir /bin
/busybox --install /bin
export PATH=/bin
mkdir /sys
mkdir /proc
mount -t proc null /proc
mount -t sysfs null /sys
mknod /dev/mem c 1 1
mdev -s
/lspci -nn -vvv -d 1234:1337
insmod gpu_pcie.ko
mdev -s
dmesg | tail -n 10
exec /bin/sh
