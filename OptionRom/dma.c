#include <Uefi.h>
#include <Protocol/PciIo.h>
#include <Library/UefiLib.h>

EFI_STATUS EFIAPI DoBusMasterWrite (
  IN EFI_PCI_IO_PROTOCOL  *PciIo,
  IN UINT8                *HostAddress,
  IN UINTN                Length
  ) {
  EFI_STATUS              Status;
  UINTN                   NumberOfBytes;
  EFI_PHYSICAL_ADDRESS    DeviceAddress;
  VOID                    *Mapping;
  UINT32                  DmaStartAddress;
  //UINT64                  ControllerStatus;
  
  //
  // Loop until the entire buffer specified by HostAddress and
  // Length has been written by the PCI DMA bus master
  // do {
    //
    // Call Map() to retrieve the DeviceAddress to use for the bus
    // master write operation. The Map() function may not support
    // performing a DMA operation for the entire length, so it may
    // be broken up into smaller DMA operations.
    //
    NumberOfBytes = Length;
    Status = PciIo->Map (
                    PciIo,                            // This
                    EfiPciIoOperationBusMasterWrite,  // Operation
                    (VOID *)HostAddress,              // HostAddress
                    &NumberOfBytes,                   // NumberOfBytes
                    &DeviceAddress,                   // DeviceAddress
                    &Mapping                          // Mapping
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

	UINTN dir = 0; // host_to_gpu
	// dir
    Status = PciIo->Mem.Write (
                        PciIo,                        // This
                        EfiPciIoWidthUint32,          // Width
                        0,                            // BarIndex
                        0x0 * 4,                         // dir 
                        1,                            // Count
                        &dir                // Buffer
                        );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Write the DMA start address to MMIO Register 0x20 of Bar #1
    //
    DmaStartAddress = (UINT32)DeviceAddress;
    Status = PciIo->Mem.Write (
                        PciIo,                        // This
                        EfiPciIoWidthUint32,          // Width
                        0,                            // BarIndex
                        0x1 * 4,                         // Offset = reg 1 = src addr (host)
                        1,                            // Count
                        &DmaStartAddress              // Buffer
                        );
    if (EFI_ERROR (Status)) {
      return Status;
    }

	UINTN dst_addr = 0; // dst = 0
	// dst
    Status = PciIo->Mem.Write (
                        PciIo,                        // This
                        EfiPciIoWidthUint32,          // Width
                        0,                            // BarIndex
                        0x2 * 4,                         // dst 
                        1,                            // Count
                        &dst_addr                // Buffer
                        );
    if (EFI_ERROR (Status)) {
      return Status;
    }

  
    //
    // Write the length of the DMA to MMIO Register 0x24 of Bar #1
    // This write operation also starts the DMA transaction
    //
    Status = PciIo->Mem.Write (
                        PciIo,                        // This
                        EfiPciIoWidthUint32,          // Width
                        0,                            // BarIndex
                        0x3 * 4,                         // len 
                        1,                            // Count
                        &NumberOfBytes                // Buffer
                        );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  
	UINTN go = 1; // token value
	// go
    Status = PciIo->Mem.Write (
                        PciIo,                        // This
                        EfiPciIoWidthUint32,          // Width
                        0,                            // BarIndex
                        0xf00*4,                         // cmd start 
                        1,                            // Count
                        &go                // Buffer
                        );
    if (EFI_ERROR (Status)) {
      return Status;
    }

  
    //
    // Call Flush() to flush all write transactions to system memory
    //
    Status = PciIo->Flush (PciIo);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  
    //
    // Call Unmap() to complete the bus master write operation
    //
    Status = PciIo->Unmap (PciIo, Mapping);
    if (EFI_ERROR (Status)) {
    return Status;
    }
  
    //
    // Update the HostAddress and Length remaining based upon the
    // number of bytes transferred
    //
    HostAddress += NumberOfBytes;
    //*Length -= NumberOfBytes;
//  } while (*Length != 0);
  return Status;
}
