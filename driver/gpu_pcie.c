#include "driver.h"

static struct pci_device_id gpu_id_tbl[] = {
	{ PCI_DEVICE(0x1234, 0x1337) },
	{ 0, },
};
MODULE_DEVICE_TABLE(pci, gpu_id_tbl);

MODULE_LICENSE("GPL");


static struct class *gpu_class;
int setup_chardev(GpuState*, struct class*, struct pci_dev*);
static int gpu_probe(struct pci_dev *pdev, const struct pci_device_id *id) {
	int bars;
	unsigned long mmio_start, mmio_len;
	GpuState* gpu = kmalloc(sizeof(struct GpuState), GFP_KERNEL);
	pr_info("called probe");


	// create a bitfield of the available BARs
	bars = pci_select_bars(pdev, IORESOURCE_MEM);
	pci_enable_device_mem(pdev);

	// claim ownership of the address space for each BAR in the bitfield
	pci_request_region(pdev, bars, "gpu-pci");

	mmio_start = pci_resource_start(pdev, 0);
	mmio_len = pci_resource_len(pdev, 0);

	// map physical address to virtual
	gpu->hwmem = ioremap(mmio_start, mmio_len);
	pr_info("mmio starts at 0x%lx; hwmem 0x%px", mmio_start, gpu->hwmem);

	mmio_start = pci_resource_start(pdev, 1);
	mmio_len = pci_resource_len(pdev, 1);

	// map physical address to virtual
	gpu->fbmem = ioremap(mmio_start, mmio_len);
	pr_info("fb starts at 0x%lx; hwmem 0x%px", mmio_start, gpu->fbmem);

	setup_chardev(gpu, gpu_class, pdev);
	return 0;
};

static struct pci_driver gpu_driver = {
	.name = "gpu",
	.id_table = gpu_id_tbl,
	.probe = gpu_probe,
};

static int __init gpu_module_init(void) {
	int err;
	gpu_class = class_create(THIS_MODULE, "gpu_pcie");
	err = pci_register_driver(&gpu_driver);
	pr_info("gpu_module_init done\n");
	return err;
}

module_init(gpu_module_init);
