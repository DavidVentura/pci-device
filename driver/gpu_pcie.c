#include <linux/pci.h>

static struct pci_device_id gpu_id_tbl[] = {
	{ PCI_DEVICE(0x1234, 0x1337) },
	{ 0, },
};
MODULE_DEVICE_TABLE(pci, gpu_id_tbl);

MODULE_LICENSE("GPL");

static struct class *gpu_class;

typedef struct GpuState {
	struct pci_dev *pdev;
} GpuState;

static int gpu_probe(struct pci_dev *pdev, const struct pci_device_id *id) {
	int bars;
	unsigned long mmio_start, mmio_len;
	static u8 __iomem *hwmem; /* Memory pointer for the I/O operations */
	pr_info("called probe");


	// create a bitfield of the available BARs
	bars = pci_select_bars(pdev, IORESOURCE_MEM);
	pci_enable_device_mem(pdev);

	// claim ownership of the address space for each BAR in the bitfield
	pci_request_region(pdev, bars, "gpu-pci");

	mmio_start = pci_resource_start(pdev, 0);
	mmio_len = pci_resource_len(pdev, 0);

	// map physical address to virtual
	hwmem = ioremap(mmio_start, mmio_len);
	pr_info("mmio starts at 0x%lx; hwmem 0x%px", mmio_start, hwmem);
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
