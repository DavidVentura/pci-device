#ifndef GPU_MODULE
#define GPU_MODULE
#include <linux/pci.h>
#include <linux/cdev.h>

typedef struct GpuState {
	struct pci_dev *pdev;
} GpuState;

#endif
