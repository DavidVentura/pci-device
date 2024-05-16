#ifndef GPU_MODULE
#define GPU_MODULE
#include <linux/pci.h>
#include <linux/cdev.h>

typedef struct GpuState {
	struct pci_dev *pdev;
	u8 __iomem *hwmem;
	u8 __iomem *fbmem;
	// store the cdev in GpuState, as `cdev` is retrievable in the
	// character device's `open`, from this cdev we can calculate the 
	// offset to GpuState, which we can then pass to `file`'s `private_data`
	struct cdev cdev;
	struct cdev fbcdev;

} GpuState;

#endif
