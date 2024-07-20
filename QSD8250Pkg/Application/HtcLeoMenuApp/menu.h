#ifndef _MAIN_MENU_H_
#define _MAIN_MENU_H_
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

#include <Library/HtcLeoPlatformResetLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/DevicePathLib.h>
#include <Library/TimerLib.h>

#include <Protocol/LoadedImage.h>
#include <Resources/FbColor.h>
#include <Chipset/timer.h>
#include <Library/PrintLib.h>



typedef struct {
  UINT8   Index;
  CHAR16 *Name;
  BOOLEAN IsActive;
  void (*Function)();
} MenuEntry;

#define MAX_OPTIONS_COUNT 12
#define PRINT_CENTRE_COLUMN 20

void MainMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void RebootMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void SettingsMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void ExitMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void DrawMenu();
void ReturnToMainMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
UINTN GetActiveMenuEntryLength();

void PrepareConsole(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *Cout,
    OUT EFI_SIMPLE_TEXT_OUTPUT_MODE    *ModeToStore);
void RestoreInitialConsoleMode(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *Cout,
    IN EFI_SIMPLE_TEXT_OUTPUT_MODE     *StoredMode);
void HandleKeyInput(
    IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);

void NullFunction();
void BootDefault(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void StartTetris(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void StartShell(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void BootAndroidKernel(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void DumpMemory2Sdcard(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void DumpMemory2SdcardHelper(UINTN* hexval, CHAR16** hexstring, UINTN* length, IN EFI_SYSTEM_TABLE *SystemTable);
void DumpDmesg(void);
EFI_STATUS ReadMemoryAndWriteToFile(UINTN* MemoryAddress,UINTN Length, CHAR16 *FilePath);
CHAR16* GetHexInput(EFI_SYSTEM_TABLE *SystemTable, CHAR16* message);

#endif