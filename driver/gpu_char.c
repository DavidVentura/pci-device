#include "driver.h"
#define MAX_CHAR_DEVICES 2

static int gpu_open(struct inode *inode, struct file *file) {
	GpuState *gpu = container_of(inode->i_cdev, struct GpuState, cdev);
	file->private_data = gpu;
	pr_info("open");
    return 0;
}

static ssize_t gpu_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {
	GpuState *gpu = (GpuState*) file->private_data;
	uint32_t read_val = ioread32(gpu->hwmem + *offset);
	copy_to_user(buf, &read_val, 4);
	*offset += 4;
	return 4;
}


static ssize_t gpu_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {
	GpuState *gpu = (GpuState*) file->private_data;
	u32 n;
	size_t written = 0;
	while((written + 4) <= count) {
		copy_from_user(&n, buf + *offset + written, 4);
		iowrite32(n, gpu->hwmem + *offset + written);
		written += 4;
	}
	while(written < count) {
		iowrite8(*(buf + written), gpu->hwmem + *offset + written);
		written += 1;
	}
	*offset += written;
	return written;
}

// initialize file_operations
static const struct file_operations fileops = {
    .owner      = THIS_MODULE,
    .open       = gpu_open,
    .read       = gpu_read,
    .write      = gpu_write
};

int setup_chardev(GpuState* gpu, struct class* class, struct pci_dev *pdev) {
	dev_t dev_num;
	dev_t major;
	alloc_chrdev_region(&dev_num, 0, MAX_CHAR_DEVICES, "gpu-chardev");
	major = MAJOR(dev_num);

	cdev_init(&gpu->cdev, &fileops);
	gpu->cdev.owner = THIS_MODULE;
	cdev_add(&gpu->cdev, MKDEV(major, 0), 1);
	device_create(class, NULL, MKDEV(major, 0), NULL, "gpu-%d", 0);

	cdev_init(&gpu->fbcdev, &fileops);
	gpu->fbcdev.owner = THIS_MODULE;
	cdev_add(&gpu->fbcdev, MKDEV(major, 1), 1);
	device_create(class, NULL, MKDEV(major, 1), NULL, "gpu-%d", 1);

	pr_info("character device set up");
	return 0;
}
