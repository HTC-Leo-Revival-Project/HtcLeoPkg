#include <Uefi.h>

#include <Protocol/GraphicsOutput.h>

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include <Pi/PiFirmwareFile.h>

#include <Library/DxeServicesLib.h>


#include <Library/BmpSupportLib.h>

EFI_GUID gBackgroundImageGuid =
  { 0x12345678, 0x1234, 0x1234, {0x12,0x34,0x12,0x34,0x56,0x78,0x9A,0xBC} };

EFI_STATUS
LoadBmpFromFV(
  OUT VOID   **ImageData,
  OUT UINTN  *ImageSize
)
{
  return GetSectionFromAnyFv(
           &gBackgroundImageGuid,
           EFI_SECTION_RAW,
           0,
           ImageData,
           ImageSize
         );
}

EFI_STATUS
ConvertBmpToBlt(
  IN  VOID   *ImageData,
  IN  UINTN  ImageSize,
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL **Blt,
  OUT UINTN  *Width,
  OUT UINTN  *Height
)
{
  UINTN BltSize;

  return TranslateBmpToGopBlt(
           ImageData,
           ImageSize,
           Blt,
           &BltSize,
           Height,
           Width
         );
}

EFI_STATUS
DrawBackground(
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt,
  IN UINTN Width,
  IN UINTN Height
)
{
  EFI_STATUS Status;

  UINTN ScreenW = Gop->Mode->Info->HorizontalResolution;
  UINTN ScreenH = Gop->Mode->Info->VerticalResolution;

  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Black = {0, 0, 0, 0};

  Status = Gop->Blt(
                  Gop,
                  &Black,
                  EfiBltVideoFill,
                  0, 0,
                  0, 0,
                  ScreenW,
                  ScreenH,
                  0
                );
  if (EFI_ERROR(Status)) return Status;

  UINTN DestX = 0;
  UINTN DestY = 0;

  if (Width < ScreenW) {
    DestX = (ScreenW - Width) / 2;
  }

  if (Height < ScreenH) {
    DestY = (ScreenH - Height) / 2;
  }

  return Gop->Blt(
           Gop,
           Blt,
           EfiBltBufferToVideo,
           0, 0,
           DestX,
           DestY,
           Width,
           Height,
           0
         );
}

EFI_STATUS
EFIAPI
UefiMain(
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
)
{
  EFI_STATUS Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = NULL;

  VOID *ImageData = NULL;
  UINTN ImageSize = 0;

  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt = NULL;
  UINTN Width = 0;
  UINTN Height = 0;

  Print(L"Loading BMP from firmware...\n");

  Status = gBS->LocateProtocol(
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  (VOID**)&Gop
                );
  if (EFI_ERROR(Status)) {
    Print(L"Failed to locate GOP: %r\n", Status);
    return Status;
  }

  Status = LoadBmpFromFV(&ImageData, &ImageSize);
  if (EFI_ERROR(Status)) {
    Print(L"Failed to load BMP: %r\n", Status);
    return Status;
  }

  Status = ConvertBmpToBlt(ImageData, ImageSize, &Blt, &Width, &Height);
  if (EFI_ERROR(Status)) {
    Print(L"Failed to convert BMP: %r\n", Status);
    return Status;
  }

  Status = DrawBackground(Gop, Blt, Width, Height);
  if (EFI_ERROR(Status)) {
    Print(L"Failed to draw image: %r\n", Status);
    return Status;
  }

  Print(L"Done. Press any key...\n");
  gST->ConIn->Reset(gST->ConIn, FALSE);
  EFI_INPUT_KEY Key;
  gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);

  if (Blt != NULL) {
    FreePool(Blt);
  }

  return EFI_SUCCESS;
}