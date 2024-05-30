#include <Uefi.h>
#include <Protocol/PciIo.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FrameBufferBltLib.h>
#include <stddef.h>

typedef struct {
  EFI_HANDLE Handle;
  EFI_PCI_IO_PROTOCOL             *PciIo;
  EFI_DEVICE_PATH_PROTOCOL        *GopDevicePath;

  FRAME_BUFFER_CONFIGURE          *FrameBufferBltConfigure;
  UINTN                           FrameBufferBltConfigureSize;

  EFI_GRAPHICS_OUTPUT_PROTOCOL Gop;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION Info;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *FrameBuffer;
} MY_GPU_PRIVATE_DATA;

EFI_STATUS EFIAPI GopSetup(IN OUT MY_GPU_PRIVATE_DATA *Private);

EFI_STATUS EFIAPI DoBusMasterWrite (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT8                *HostAddress,
  IN UINTN                 Length
  );
