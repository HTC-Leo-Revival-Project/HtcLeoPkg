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