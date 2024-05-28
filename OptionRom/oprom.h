#include <Uefi.h>
#include <Protocol/PciIo.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <stddef.h>

typedef struct {
    EFI_PCI_IO_PROTOCOL             *PciIo;

    EFI_GRAPHICS_OUTPUT_PROTOCOL Gop;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION Info;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *InfoPtr;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL *FrameBuffer;
} MY_GPU_PRIVATE_DATA;

EFI_STATUS EFIAPI GopSetup(IN OUT MY_GPU_PRIVATE_DATA *Private);
