#include "linux/dma-mapping.h"
#include "driver.h"
#include "hw.h"

#define MAX_CHAR_DEVICES 2
extern wait_queue_head_t wq;
extern volatile int irq_fired;

static int gpu_fb_open(struct inode *inode, struct file *file) {
	GpuState *gpu = container_of(inode->i_cdev, struct GpuState, fbcdev);
	file->private_data = gpu;
	pr_info("fbopen hwmem %px", gpu->hwmem);
	return 0;
}

static void write_reg(GpuState* gpu, u32 val, u32 reg) {
	iowrite32(val, 	gpu->hwmem + (reg * sizeof(u32)));
}

void setup_dma(GpuState* gpu, u8 dir, u32 src, u32 dst, u32 len) {
	write_reg(gpu, dir, REG_DMA_DIR);
	write_reg(gpu, src,	REG_DMA_ADDR_SRC);
	write_reg(gpu, dst,	REG_DMA_ADDR_DST);
	write_reg(gpu, len,	REG_DMA_LEN);
	write_reg(gpu, 1, 	REG_DMA_START);
}

static struct page** pin(const char* buf, size_t count, loff_t offset) {
	unsigned long user_addr = (unsigned long)buf;
	unsigned long aligned_addr = user_addr & ~(PAGE_SIZE - 1);
	unsigned long poffset = user_addr - aligned_addr;
	int nr_pages = DIV_ROUND_UP(poffset + count, PAGE_SIZE);  // Calculate the total number of pages
	struct page **pages = kvmalloc_array(nr_pages, sizeof(struct page *), GFP_KERNEL);
	int pages_pinned = pin_user_pages_fast(aligned_addr, nr_pages, FOLL_LONGTERM, pages);
	pr_info("pinned %d\n", pages_pinned);
	return pages;
}

static ssize_t gpu_fb_read(struct file *file, char __user *buf, size_t count, loff_t *offset) {
	GpuState *gpu = (GpuState*) file->private_data;

	struct page **pages = pin(buf, count, *offset);
	dma_addr_t dma_addr = dma_map_single(&gpu->pdev->dev, buf, count, DMA_FROM_DEVICE);
	irq_fired = 0;
	setup_dma(gpu, DIR_GPU_TO_HOST, *offset, dma_addr, count);
	if (wait_event_interruptible(wq, irq_fired != 0)) {
		pr_info("interrupted");
		kvfree(pages);
		return -ERESTARTSYS;
	}
	dma_unmap_single(&gpu->pdev->dev, dma_addr, count, DMA_TO_DEVICE);
	kvfree(pages);
	return count;
}

static ssize_t gpu_fb_write(struct file *file, const char __user *buf, size_t count, loff_t *offset) {
	GpuState *gpu = (GpuState*) file->private_data;
	struct page **pages = pin(buf, count, *offset);
	dma_addr_t dma_addr;
	u8* kbuf = kmalloc(count, GFP_KERNEL);

	irq_fired = 0;

	copy_from_user(kbuf, buf, count);
	pr_info("kbuf[0] %d, kbuf[1] %d\n", kbuf[0], kbuf[1]);
	dma_addr = dma_map_single(&gpu->pdev->dev, kbuf, count, DMA_TO_DEVICE);
	pr_info("DMA addr is - 0x%llx\n", dma_addr);
	pr_info("setup DMA! - user: 0x%px, offset: 0x%llx\n", buf, *offset);
	setup_dma(gpu, DIR_HOST_TO_GPU, dma_addr, *offset, count);

	if (wait_event_interruptible(wq, irq_fired != 0)) {
		pr_info("interrupted");
		return -ERESTARTSYS;
	}
	dma_unmap_single(&gpu->pdev->dev, dma_addr, count, DMA_TO_DEVICE);
	kfree(kbuf);
	kvfree(pages);
	return count;
}

static int gpu_open(struct inode *inode, struct file *file) {
	GpuState *gpu = container_of(inode->i_cdev, struct GpuState, cdev);
	file->private_data = gpu;
	pr_info("io-open hwmem %px", gpu->hwmem);
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
	pr_info("writing to IO mem, 0x%px\n", gpu->hwmem);
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

static const struct file_operations fb_ops = {
	.owner		= THIS_MODULE,
	.open		= gpu_fb_open,
	.read		= gpu_fb_read,
	.write		= gpu_fb_write
};

static const struct file_operations fileops = {
	.owner 		= THIS_MODULE,
	.open 		= gpu_open,
	.read 		= gpu_read,
	.write 		= gpu_write
};

int setup_chardev(GpuState* gpu, struct class* class, struct pci_dev *pdev) {
	dev_t dev_num;
	dev_t major;
	alloc_chrdev_region(&dev_num, 0, MAX_CHAR_DEVICES, "gpu-chardev");
	major = MAJOR(dev_num);

	cdev_init(&gpu->cdev, &fileops);
	gpu->cdev.owner = THIS_MODULE;
	cdev_add(&gpu->cdev, MKDEV(major, 0), 1);
	device_create(class, NULL, MKDEV(major, 0), NULL, "gpu-io");

	cdev_init(&gpu->fbcdev, &fb_ops);
	gpu->fbcdev.owner = THIS_MODULE;
	cdev_add(&gpu->fbcdev, MKDEV(major, 1), 1);
	device_create(class, NULL, MKDEV(major, 1), NULL, "gpu-%d", 0);

	pr_info("character device set up");
	return 0;
}
