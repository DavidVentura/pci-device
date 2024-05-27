#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>


EFI_STATUS EFIAPI OptionRomEntry (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  ) {
    (void)ImageHandle;
	EFI_BOOT_SERVICES *gBS = SystemTable->BootServices;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
	EFI_STATUS Status;

	Status = gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&Gop);
	if (EFI_ERROR(Status)) {
		return Status;
	}

	Gop->Blt(
			Gop,
			(EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&(EFI_GRAPHICS_OUTPUT_BLT_PIXEL){0x00, 0xFF, 0xFF, 0x00}, // Blue color
			EfiBltVideoFill,
			0, 0, // Source coordinates
			0, 0, // Destination coordinates
			Gop->Mode->Info->HorizontalResolution, Gop->Mode->Info->VerticalResolution, // Width and Height
			0 // Delta
			);

	// vvv this one does nothing
	//
    DEBUG ((EFI_D_INFO, "DDDMyOptionRom loaded\n"));
    DEBUG ((EFI_D_ERROR, "EEEMyOptionRom loaded\n"));
	// vvv this one works
    Print(L"PPMyOptionRom loaded\n");

	//volatile int a = 0;
	//volatile int b = 3 / a;
	//(void)b;
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI MyOptionRomUnload (
  IN  EFI_HANDLE  ImageHandle
  ) {
    (void)ImageHandle;
    DEBUG((EFI_D_INFO, "MyOptionRom UNloaded\n"));
    Print(L"MyOptionRom UNloaded\n");

    return EFI_SUCCESS;
}

