#include "menu.h"
#include "BootApp.h"

VOID RestoreInitialConsoleMode(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *Cout,
    IN EFI_SIMPLE_TEXT_OUTPUT_MODE     *StoredMode)
{
  EFI_STATUS Status;

  Status = Cout->EnableCursor(Cout, StoredMode->CursorVisible);
  ASSERT_EFI_ERROR(Status);
  Status = Cout->SetCursorPosition(Cout, StoredMode->CursorColumn, StoredMode->CursorRow);
  ASSERT_EFI_ERROR(Status);
  Status = Cout->SetAttribute(Cout, StoredMode->Attribute);
  ASSERT_EFI_ERROR(Status);
  Status = Cout->ClearScreen(Cout);
  ASSERT_EFI_ERROR(Status);
}

// Start another app
VOID StartApp(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable, CHAR16 *Description)
{
  EFI_BOOT_MANAGER_LOAD_OPTION *BootOptions;
  UINTN                         BootOptionCount;
  EFI_STATUS                    Status;
  
  EfiBootManagerRefreshAllBootOption();

  BootOptions =
      EfiBootManagerGetLoadOptions(&BootOptionCount, LoadOptionTypeBoot);
  ASSERT(BootOptionCount != -1);
  for (UINTN i = 0; i < BootOptionCount; i++) {
    if (StrCmp(Description, BootOptions[i].Description) == 0) {
      RestoreInitialConsoleMode(SystemTable->ConOut, &InitialMode);
      EfiBootManagerBoot(&BootOptions[i]);
    }
  }

  EfiBootManagerFreeLoadOptions(BootOptions, BootOptionCount);
}

VOID StartShell(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS Status = EFI_SUCCESS;

  Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  StartApp(ImageHandle, SystemTable, SHELL_APP_TITLE);
}

VOID StartTetris(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  StartApp(ImageHandle, SystemTable, TETRIS_APP_TITLE);
}

VOID SetClkBoot(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
  MmioWrite32(0x2FFB0000, FASTBOOT_MODE_MAGIC);
  MmioWrite32(0x2FFB0004, FASTBOOT_MODE_MAGIC^CLK_SIGNATURE_MAGIC);
  ResetCold();
  return;
}

VOID NullFunction()
{
  //Print(L"Feature not supported yet!");
  DEBUG((EFI_D_ERROR, "Feature not supported yet!"));
}

VOID ExitMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  ASSERT_EFI_ERROR(Status);
  SystemTable->BootServices->Exit(ImageHandle, EFI_SUCCESS, 0, NULL);
}

VOID BootDefault(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS Status = EFI_SUCCESS;

  Status = DiscoverAndBootApp(
      ImageHandle, EFI_REMOVABLE_MEDIA_FILE_NAME_ARM, NULL);

  // We shouldn't reach here if the default file is present
  if(Status) {
    //Print(L"Booting default entry failed!");
    DEBUG((EFI_D_ERROR, "Booting default entry failed!"));
  }
}

VOID PrepareConsole(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *Cout,
    OUT EFI_SIMPLE_TEXT_OUTPUT_MODE    *ModeToStore)
{
  EFI_STATUS Status;
  CopyMem(ModeToStore, Cout->Mode, sizeof(EFI_SIMPLE_TEXT_OUTPUT_MODE));

  Status = Cout->EnableCursor(Cout, FALSE);
  if (Status != EFI_UNSUPPORTED) { // workaround
    ASSERT_EFI_ERROR(Status);
  }

  Status = Cout->ClearScreen(Cout);
  ASSERT_EFI_ERROR(Status);
  Status = Cout->SetAttribute(Cout, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
  ASSERT_EFI_ERROR(Status);
  Status = Cout->SetCursorPosition(Cout, 0, 0);
  ASSERT_EFI_ERROR(Status);
}