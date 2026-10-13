#ifndef GOPBMPHELPER_H
#define GOPBMPHELPER_H

#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>

extern EFI_GUID gBackgroundImageGuid;

EFI_STATUS
LoadBmpFromFV(
  OUT VOID   **ImageData,
  OUT UINTN  *ImageSize
);

EFI_STATUS
ConvertBmpToBlt(
  IN  VOID   *ImageData,
  IN  UINTN  ImageSize,
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL **Blt,
  OUT UINTN  *Width,
  OUT UINTN  *Height
);

EFI_STATUS
DrawBackground(
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt,
  IN UINTN Width,
  IN UINTN Height
);

#endif // GOPBMPHELPER_H