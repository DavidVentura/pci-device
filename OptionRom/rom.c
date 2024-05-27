#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>


EFI_STATUS EFIAPI OptionRomEntry (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  ) {
    (void)ImageHandle;
    (void)SystemTable;

	// vvv this one does nothing
	//
    DEBUG((EFI_D_INFO, "DDDMyOptionRom loaded\n"));
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

