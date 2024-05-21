#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/units.h"
#include "hw/pci/pci.h"
#include "hw/hw.h"
#include "hw/pci/msi.h"
#include "hw/pci/msix.h"
#include "qemu/timer.h"
#include "qom/object.h"
#include "qemu/main-loop.h" /* iothread mutex */
#include "qemu/module.h"
#include "qapi/visitor.h"
#include "hw.h"

#define TYPE_PCI_GPU_DEVICE "gpu"
#define GPU_DEVICE_ID         0x1337
typedef struct GpuState GpuState;
DECLARE_INSTANCE_CHECKER(GpuState, GPU, TYPE_PCI_GPU_DEVICE)

struct GpuState {
    PCIDevice pdev;
    MemoryRegion mem;
    MemoryRegion fbmem;
    MemoryRegion msix;
	uint32_t registers[0x100000 / 32]; // 1 MiB = 32k, 32 bit registers
	unsigned char framebuffer[0x1000000]; // 16 MiB
};

static void pci_gpu_register_types(void);
static void gpu_instance_init(Object *obj);
static void gpu_class_init(ObjectClass *class, void *data);
static void pci_gpu_realize(PCIDevice *pdev, Error **errp);
static void pci_gpu_uninit(PCIDevice *pdev);

type_init(pci_gpu_register_types)

static void pci_gpu_register_types(void)
{
    static InterfaceInfo interfaces[] = {
        { INTERFACE_PCIE_DEVICE },
        { },
    };
    static const TypeInfo gpu_info = {
        .name          = TYPE_PCI_GPU_DEVICE,
        .parent        = TYPE_PCI_DEVICE,
        .instance_size = sizeof(GpuState),
        .instance_init = gpu_instance_init,
        .class_init    = gpu_class_init,
        .interfaces = interfaces,
    };

    type_register_static(&gpu_info);
}

static void gpu_instance_init(Object *obj) {
    printf("GPU instance init\n");
}

static void gpu_class_init(ObjectClass *class, void *data) {
    printf("Class init\n");
    PCIDeviceClass *k = PCI_DEVICE_CLASS(class);

    k->realize = pci_gpu_realize;
    k->exit = pci_gpu_uninit;
    k->vendor_id = PCI_VENDOR_ID_QEMU;
    k->device_id = GPU_DEVICE_ID;
    k->class_id = PCI_CLASS_OTHERS;
}

static uint64_t lower_n_bytes(uint64_t data, unsigned nbytes) {
	uint64_t result;
	if (nbytes < 8) {
		uint64_t bitcount = ((uint64_t)nbytes)<<3;
		uint64_t mask = (1ULL << bitcount)-1;
		result = data & mask;
	} else {
		result = data;
	}
	return result;
}

static uint64_t gpu_control_read(void *opaque, hwaddr addr, unsigned size) {
	GpuState *gpu = opaque;
	uint64_t index = lower_n_bytes(addr, size);
	uint32_t index_u32 = index / sizeof(uint32_t);
	uint64_t result = ((uint32_t*)gpu->registers)[index_u32];
	printf("reading idx %lu = %lu\n", index, result);
	return result;
}

static void gpu_control_write(void *opaque, hwaddr addr, uint64_t val, unsigned size) {
	GpuState *gpu = opaque;
	val = lower_n_bytes(val, size);
	uint32_t reg = addr / 4;
	printf("writing addr %ld [reg %d], size %u = %lu\n", addr, reg, size, val);
	// TODO: should write only masked bits, not whole u64
	switch (addr) {
		case REG_DMA_DIR:
		case REG_DMA_ADDR_SRC:
		case REG_DMA_ADDR_DST:
			gpu->registers[reg] = (uint32_t)val;
			// not 
			break;
		case REG_DMA_START:
			printf("supposed to start DMA now\n");
			break;
		case REG_DMA_IRQ_SET:
			printf("!supposed to set the IRQ value to %ld\n", val);
			if (val == 0) {
				msix_notify(&gpu->pdev, IRQ_TEST);
				//msix_clr_pending(&gpu->pdev, IRQ_TEST);
			} else {
				msix_notify(&gpu->pdev, IRQ_DMA_DONE);
				//msix_clr_pending(&gpu->pdev, IRQ_DMA_DONE);
			}
			break;
	}
}

static uint64_t gpu_fb_read(void *opaque, hwaddr addr, unsigned size) {
	// GpuState *gpu = opaque;
	printf("reading framebuffer %lu = %u\n", addr, size);
	return 0; // pretend we have 0 bytes to give
}

static void gpu_fb_write(void *opaque, hwaddr addr, uint64_t val, unsigned size) {
	// GpuState *gpu = opaque;
	printf("writing framebuffer, addr %ld, size %u = %lu\n", addr, size, val);
}

static const MemoryRegionOps gpu_mem_ops = {
    .read = gpu_control_read,
    .write = gpu_control_write,
	.valid = {
        .min_access_size = 0x1,
        .max_access_size = 0x1000,
    },
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static const MemoryRegionOps gpu_fb_ops = {
    .read = gpu_fb_read,
    .write = gpu_fb_write,
	.valid = {
        .min_access_size = 0x4,
        .max_access_size = 0x1000,
    },
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void pci_gpu_realize(PCIDevice *pdev, Error **errp) {
    printf("GPU Realize\n");
    GpuState *gpu = GPU(pdev);
    memory_region_init_io(&gpu->mem, OBJECT(gpu), &gpu_mem_ops, gpu, "gpu-control-mem", 1 * MiB);
    memory_region_init_io(&gpu->fbmem, OBJECT(gpu), &gpu_fb_ops, gpu, "gpu-fb-mem", 16 * MiB);
    memory_region_init(&gpu->msix, OBJECT(gpu), "gpu-msix", 16 * KiB);

    pci_register_bar(pdev, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &gpu->mem);
    pci_register_bar(pdev, 1, PCI_BASE_ADDRESS_SPACE_MEMORY, &gpu->fbmem);
    pci_register_bar(pdev, 2, PCI_BASE_ADDRESS_SPACE_MEMORY, &gpu->msix);

	int ret = msix_init(pdev, IRQ_COUNT, &gpu->msix, 2 /* table_bar_nr  = bar id */, 0x0 /* table_offset */,
						&gpu->msix, 2 /* pba_bar_nr  = bar id */, 0x2000 /* pba_offset */, 0xA0 /* cap_pos ?? */,errp);
	if (ret) {
		printf("msi init ret %d\n", ret);
	}
	for(int i = 0; i < IRQ_COUNT; i++) {
		msix_vector_use(pdev, i);
	}
}

static void pci_gpu_uninit(PCIDevice *pdev) {
    printf("GPU un-init\n");
}
