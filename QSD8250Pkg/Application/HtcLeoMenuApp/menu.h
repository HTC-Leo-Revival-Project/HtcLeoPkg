#include <Uefi.h>
#include <PiDxe.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/BootAppLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/ArmLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>

#include <Library/PlatformResetLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/DevicePathLib.h>
#include <Library/TimerLib.h>

#include <Protocol/LoadedImage.h>
#include <Resources/FbColor.h>
#include <Chipset/timer.h>
#include <Library/PrintLib.h>
#include <Protocol/WatchdogTimer.h>
#include "Booter/AndroidKernel.h"
#include "bootreason.h"
#include <Chipset/iomap.h>
#include <Device/DeviceType.h>
#include <Protocol/HtcDeviceDetection.h>

#ifndef _MAIN_MENU_H_
#define _MAIN_MENU_H_

typedef struct {
  CHAR16 *Name;
  BOOLEAN IsActive;
  void (*Function)(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable);
} MenuEntry;

#define MAX_OPTIONS_COUNT 12
#define PRINT_CENTRE_COLUMN 20

void RebootMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void DrawMenu();
void MainMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
UINTN GetActiveMenuEntryLength();
void HandleKeyInput(
    IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);


void DumpMemory2Sdcard(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void DumpMemory2SdcardHelper(UINTN* hexval, CHAR16** hexstring, UINTN* length, IN EFI_SYSTEM_TABLE *SystemTable);
void DumpDmesg(void);
EFI_STATUS ReadMemoryAndWriteToFile(UINTN* MemoryAddress,UINTN Length, CHAR16 *FilePath);
CHAR16* GetHexInput(EFI_SYSTEM_TABLE *SystemTable, CHAR16* message);
VOID
EFIAPI
FeedWatchdogCallback (IN EFI_EVENT Event,IN VOID      *Context);

extern EFI_SIMPLE_TEXT_OUTPUT_MODE *InitialMode;
extern HtcDevice* mDevice;

#endif