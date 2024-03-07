#ifndef _MAIN_MENU_H_
#define _MAIN_MENU_H_
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

typedef struct {
  UINT8   Index;
  CHAR16 *Name;
  BOOLEAN IsActive;
  void (*Function)();
} MenuEntry;

void Option1Function(
    IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void RebootMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void Option2Function(
    IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void ExitMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void StartTetris(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void HexagonFunction(
    IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void PrepareConsole(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *Cout,
    OUT EFI_SIMPLE_TEXT_OUTPUT_MODE    *ModeToStore);
void RestoreInitialConsoleMode(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *Cout,
    IN EFI_SIMPLE_TEXT_OUTPUT_MODE     *StoredMode);
void HandleKeyInput(
    IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);

EFI_STATUS StartAnotherApp(
    IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable,
    IN EFI_GUID *AppGuid);
void DrawMenu(IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConsoleOut);

void HsUsbFunction(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);

void  NullFunction();
UINTN GetActiveMenuEntryLength();
void ReturnToMainMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
void EnterBootMgr(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);

#endif