#include "oprom.h"
#include <Protocol/GraphicsOutput.h>
#include <Library/UefiBootServicesTableLib.h> // gBS

EFI_STATUS EFIAPI MyGpuBlt(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer,
    IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
    IN UINTN SourceX, IN UINTN SourceY,
    IN UINTN DestinationX, IN UINTN DestinationY,
    IN UINTN Width, IN UINTN Height,
    IN UINTN Delta
) {
    DEBUG ((EFI_D_INFO, "Blit\n"));
    // Implement Blt function
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI MyGpuSetMode(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    IN UINT32 ModeNumber
) {
    DEBUG ((EFI_D_INFO, "setmode to %d\n", ModeNumber));
    // Implement SetMode function
    return EFI_SUCCESS;
}

//#define MY_GPU_PRIVATE_DATA_FROM_THIS(a) CR(a, MY_GPU_PRIVATE_DATA, Gop, SIGNATURE_32('g','o','p','d'))
#define MY_GPU_PRIVATE_DATA_FROM_THIS(a) BASE_CR(a, MY_GPU_PRIVATE_DATA, Gop)
EFI_STATUS EFIAPI MyGpuQueryMode(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    IN UINT32 ModeNumber,
    OUT UINTN *SizeOfInfo,
    OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info
) {
    MY_GPU_PRIVATE_DATA *Private = MY_GPU_PRIVATE_DATA_FROM_THIS(This);
	DEBUG ((EFI_D_INFO, "in querymode for mode=%d\n", ModeNumber));

    if (ModeNumber >= This->Mode->MaxMode) {
		DEBUG ((EFI_D_INFO, "badparam\n"));
        return EFI_INVALID_PARAMETER;
    }
	// Info must be a newly allocated pool

    *SizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
	*Info = AllocateCopyPool (*SizeOfInfo, &Private->Info);

    //*Info = &Private->Info;
    DEBUG ((EFI_D_INFO, "donequery hr %d vr %d\n", (*Info)->HorizontalResolution, (*Info)->VerticalResolution));
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI GopSetup(IN OUT MY_GPU_PRIVATE_DATA *Private) {
    EFI_STATUS Status;

    // Initialize the GOP protocol
    Private->Gop.QueryMode = MyGpuQueryMode;
    Private->Gop.SetMode = MyGpuSetMode;
    Private->Gop.Blt = MyGpuBlt;

    // Fill in the mode information
    Private->Info.Version = 0;
    Private->Info.HorizontalResolution = 1024;
    Private->Info.VerticalResolution = 768;
    Private->Info.PixelFormat = PixelBlueGreenRedReserved8BitPerColor;
    Private->Info.PixelsPerScanLine = Private->Info.HorizontalResolution;

    Private->InfoPtr = &Private->Info;
    Private->Gop.Mode = AllocateZeroPool(sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE));
    if (Private->Gop.Mode == NULL) {
        FreePool(Private);
        return EFI_OUT_OF_RESOURCES;
    }
    Private->Gop.Mode->MaxMode = 1;
    Private->Gop.Mode->Mode = 0;
    Private->Gop.Mode->Info = Private->InfoPtr;
    Private->Gop.Mode->SizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
	UINT32 FbSize = Private->Info.HorizontalResolution * Private->Info.VerticalResolution * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    Private->Gop.Mode->FrameBufferBase = (EFI_PHYSICAL_ADDRESS)(UINTN)AllocatePages(EFI_SIZE_TO_PAGES(FbSize));
    Private->Gop.Mode->FrameBufferSize = FbSize;

    Status = Private->Gop.SetMode(&Private->Gop, 0);
    if (EFI_ERROR (Status)) {
		DEBUG ((EFI_D_ERROR, "failed to setmode\n"));
		return Status;
    }
    DEBUG ((EFI_D_INFO, "installing handle, with private at %p\n", Private));
	return Status;
}
