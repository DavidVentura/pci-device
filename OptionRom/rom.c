#include "oprom.h"
#include <IndustryStandard/Pci.h>
#include <Library/UefiBootServicesTableLib.h> // gBS

/**
  Check if this device is supported.
  Yoinked straight out of QemuVideoDxe/Driver

  @param  This                   The driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The bus supports this controller.
  @retval EFI_UNSUPPORTED        This device isn't supported.

**/
EFI_STATUS
EFIAPI
GpuVideoControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  PCI_TYPE00           Pci;

  //
  // Open the PCI I/O Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Read the PCI Configuration Header from the PCI Device
  //
  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0,
                        sizeof (Pci) / sizeof (UINT32),
                        &Pci
                        );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = EFI_UNSUPPORTED;
  //if (!IS_PCI_DISPLAY (&Pci)) {
  //  goto Done;
  //}

  
  DEBUG ((DEBUG_INFO, "GpuVideo: Class: %x Vendor %x Device %x\n", Pci.Hdr.ClassCode[1], Pci.Hdr.VendorId, Pci.Hdr.DeviceId));
  if (Pci.Hdr.VendorId == 0x1234 && Pci.Hdr.DeviceId == 0x1337) {
    Status = EFI_SUCCESS;
  }

Done:
  //
  // Close the PCI I/O Protocol
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}


EFI_STATUS EFIAPI GpuVideoControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  ) {
    EFI_STATUS Status;
    MY_GPU_PRIVATE_DATA *Private;

    DEBUG ((EFI_D_INFO, "entry\n"));
    Private = AllocateZeroPool(sizeof(MY_GPU_PRIVATE_DATA));
    if (Private == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }

	//Status = ReadPci(&Private);
    if (EFI_ERROR (Status)) {
		DEBUG ((EFI_D_ERROR, "failed to ReadPci\n"));
		return Status;
    }

	Status = GopSetup(Private);
    if (EFI_ERROR (Status)) {
		DEBUG ((EFI_D_ERROR, "failed to GopSetup\n"));
		return Status;
    }


    DEBUG ((EFI_D_INFO, "good\n"));
	return Status;
}

EFI_STATUS EFIAPI GpuVideoControllerDriverStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  ) {
		DEBUG ((EFI_D_INFO, "Called STOP\n"));
	// TODO implement
	return EFI_SUCCESS;
}

EFI_DRIVER_BINDING_PROTOCOL gGpuVideoDriverBinding = {
  GpuVideoControllerDriverSupported,
  GpuVideoControllerDriverStart,
  GpuVideoControllerDriverStop,
  0x10, // version
  NULL,
  NULL
};

EFI_STATUS EFIAPI OptionRomEntry(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
) {
  EFI_STATUS Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gGpuVideoDriverBinding,
             ImageHandle,
             NULL, // name1, optional
             NULL  // name2, optional
             );
  ASSERT_EFI_ERROR (Status);
  return Status;
}
