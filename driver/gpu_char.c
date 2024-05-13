#include "driver.h"
#define MAX_CHAR_DEVICES 1

static int gpu_open(struct inode *inode, struct file *file) {
	pr_info("open");
    return 0;
}

static ssize_t gpu_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {
	// no data
	pr_info("read");
	return 0;
}


static ssize_t gpu_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {
	pr_info("write");
	return count; // pretend we wrote all of it
}

// initialize file_operations
static const struct file_operations fileops = {
    .owner      = THIS_MODULE,
    .open       = gpu_open,
    .read       = gpu_read,
    .write      = gpu_write
};

int setup_chardev(struct class* class, struct pci_dev *pdev) {
	dev_t dev_num;
	dev_t minor;
	dev_t major;
	// the cdev reference has to live as long as the module --
	// the ->ops pointer is dereferenced whenever doing file operations
	// so it must be allocated on the heap
	struct cdev* cdev = kmalloc(sizeof(struct cdev), GFP_KERNEL);
	alloc_chrdev_region(&dev_num, 0, 1, "gpu-chardev");
	major = MAJOR(dev_num);
	minor = MINOR(dev_num);

	cdev_init(cdev, &fileops);
	cdev->owner = THIS_MODULE;

	cdev_add(cdev, MKDEV(major, minor), 1);
	device_create(class, NULL, MKDEV(major, minor), NULL, "gpu-0");
	pr_info("character device set up");
	return 0;
}
