#include <linux/pci.h>

static struct pci_device_id gpu_id_tbl[] = {
	{ PCI_DEVICE(0x1234, 0x1337) },
	{ 0, },
};
MODULE_DEVICE_TABLE(pci, gpu_id_tbl);

MODULE_LICENSE("GPL");

static struct class *gpu_class;

static int gpu_probe(struct pci_dev *pdev, const struct pci_device_id *id) {
	pr_info("called probe");
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
