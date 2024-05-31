#include "oprom.h"
#include <Library/BaseMemoryLib.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/Pci.h>
#include <Library/UefiBootServicesTableLib.h> // gBS
#include <Library/DevicePathLib.h>

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
  EFI_TPL OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  Private = AllocateZeroPool(sizeof(MY_GPU_PRIVATE_DATA));
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  Private->Handle = NULL;

  Status = gBS->OpenProtocol (
      Controller,
      &gEfiPciIoProtocolGuid,
      (VOID **)&Private->PciIo,
      This->DriverBindingHandle,
      Controller,
      EFI_OPEN_PROTOCOL_BY_DRIVER
      );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "failed to ReadPci\n"));
    return Status;
  }
  // TODO: mb save orig attribs

  UINT64                    SupportedAttrs;
  Status = Private->PciIo->Attributes (
      Private->PciIo,
      EfiPciIoAttributeOperationSupported,
      0,
      &SupportedAttrs
      );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "failed to check attrs\n"));
    return Status;
  }
  DEBUG ((EFI_D_INFO, "sup attrs: %x\n", SupportedAttrs));
  //
  // Set new PCI attributes
  //
  Status = Private->PciIo->Attributes (
      Private->PciIo,
      EfiPciIoAttributeOperationEnable,
      EFI_PCI_DEVICE_ENABLE | EFI_PCI_IO_ATTRIBUTE_BUS_MASTER | SupportedAttrs,
      NULL
      );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "failed to enable\n"));
    return Status;
  }

  // BAR #1 = VideoMem
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Resources;
  Status = Private->PciIo->GetBarAttributes (Private->PciIo, 0, NULL, (VOID **)&Resources);
  DEBUG (( EFI_D_INFO, "iomem is at %x and is %x long\n", Resources->AddrRangeMin, Resources->AddrLen));
  Status = Private->PciIo->GetBarAttributes (Private->PciIo, 1, NULL, (VOID **)&Resources);
  DEBUG (( EFI_D_INFO, "fbmem is at %x and is %x long\n", Resources->AddrRangeMin, Resources->AddrLen));
  Private->PciFbMemBase = Resources->AddrRangeMin;
  FreePool(Resources);


  //
  // Get ParentDevicePath
  //
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  Status = gBS->HandleProtocol (
      Controller,
      &gEfiDevicePathProtocolGuid,
      (VOID **)&ParentDevicePath
      );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "failed to get parentdevicepath\n"));
    return Status;
  }
  // what even is this ACPI & why is it required? installing the proto fails otherwise
  ACPI_ADR_DEVICE_PATH      AcpiDeviceNode;
  ZeroMem (&AcpiDeviceNode, sizeof (ACPI_ADR_DEVICE_PATH));
  AcpiDeviceNode.Header.Type    = ACPI_DEVICE_PATH;
  AcpiDeviceNode.Header.SubType = ACPI_ADR_DP;
  AcpiDeviceNode.ADR            = ACPI_DISPLAY_ADR (1, 0, 0, 1, 0, ACPI_ADR_DISPLAY_TYPE_VGA, 0, 0);
  SetDevicePathNodeLength (&AcpiDeviceNode.Header, sizeof (ACPI_ADR_DEVICE_PATH));
  // what even is this
  Private->GopDevicePath = AppendDevicePathNode (ParentDevicePath, (EFI_DEVICE_PATH_PROTOCOL *)&AcpiDeviceNode);

  if (Private->GopDevicePath == NULL) {
    DEBUG ((EFI_D_ERROR, "failed to AppendDevice\n"));
    return EFI_OUT_OF_RESOURCES;
  }
  Status = gBS->InstallMultipleProtocolInterfaces (
      &Private->Handle,
      &gEfiDevicePathProtocolGuid,
      Private->GopDevicePath,
      NULL
      );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "failed to install PathProtocol\n"));
    return Status;
  }

  Status = GopSetup(Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "failed to GopSetup\n"));
    return Status;
  }

  // Install the GOP protocol
  Status = gBS->InstallMultipleProtocolInterfaces(
      &Private->Handle,
      &gEfiGraphicsOutputProtocolGuid,
      &Private->Gop,
      NULL
      );
  DEBUG ((EFI_D_INFO, "did install \n"));
  if (EFI_ERROR(Status)) {
    FreePool(Private->Gop.Mode);
    FreePool(Private);
    DEBUG ((EFI_D_INFO, "very bad\n"));
  }
  //
  // Reference parent handle from child handle.
  //
  // i assume, some refcounting shit?
  // this succeeds but then machine hangs - not just this driver
  DEBUG ((EFI_D_INFO, "about to open proto\n"));
  EFI_PCI_IO_PROTOCOL       *ChildPciIo;
  Status = gBS->OpenProtocol (
      Controller,
      &gEfiPciIoProtocolGuid,
      (VOID **)&ChildPciIo,
      This->DriverBindingHandle,
      Private->Handle,
      EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
      );
  DEBUG ((EFI_D_INFO, "done1, status=%d\n", Status));
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "failed to ref parent from child\n"));
    return Status;
  }
  //DEBUG ((EFI_D_INFO, "torestore=%d\n", OldTpl));
  gBS->RestoreTPL (OldTpl); // never returns!?
  DEBUG ((EFI_D_INFO, "done, status=%d\n", Status));
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
