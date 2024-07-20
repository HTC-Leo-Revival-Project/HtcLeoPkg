#include "menu.h"
#include "BootApp.h"
#include "Booter/AndroidSDDir.h"

MenuEntry MenuOptions[MAX_OPTIONS_COUNT] = {0};

UINTN MenuOptionCount = 0;
UINTN SelectedIndex = 0;
EFI_SIMPLE_TEXT_OUTPUT_MODE InitialMode;

void
FillMenu()
{
  UINTN Index = 0;
  MenuOptions[Index++] = (MenuEntry){Index, L"Boot default", TRUE, &BootDefault};
  MenuOptions[Index++] = (MenuEntry){Index, L"Boot Android", TRUE, &BootAndroidKernel};
  MenuOptions[Index++] = (MenuEntry){Index, L"Play Tetris", TRUE, &StartTetris};
  MenuOptions[Index++] = (MenuEntry){Index, L"EFI Shell", TRUE, &StartShell},
  MenuOptions[Index++] = (MenuEntry){Index, L"Dump DMESG to sdcard", TRUE, &DumpDmesg},
  MenuOptions[Index++] = (MenuEntry){Index, L"Dump Memory to sdcard", TRUE, &DumpMemory2Sdcard},
  MenuOptions[Index++] = (MenuEntry){Index, L"Reboot Menu", TRUE, &RebootMenu};
  MenuOptions[Index++] = (MenuEntry){Index, L"Settings", TRUE, &SettingsMenu};
  MenuOptions[Index++] = (MenuEntry){Index, L"Exit", TRUE, &ExitMenu};
}

void PrepareConsole(
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

void RestoreInitialConsoleMode(
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

UINTN GetActiveMenuEntryLength()
{
  UINTN MenuCount = 0;
  for (UINTN i = 0; i < sizeof(MenuOptions) / sizeof(MenuOptions[0]); i++) {
    if (MenuOptions[i].IsActive) {
      MenuCount++;
    }
  }
  return MenuCount;
}

void DrawMenu()
{
  MenuOptionCount = GetActiveMenuEntryLength();

  // Print menu title
  gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_RED, EFI_BLACK));
  gST->ConOut->SetCursorPosition( gST->ConOut, PRINT_CENTRE_COLUMN-4, 1 ); // offset by 4 so the next line will center under the name
  
  Print(L" %s \n", (CHAR16 *)PcdGetPtr(PcdFirmwareVendor));
  gST->ConOut->SetCursorPosition( gST->ConOut, PRINT_CENTRE_COLUMN, 2 );
  Print(L" EDK2 Main Menu \n");
  gST->ConOut->SetCursorPosition( gST->ConOut, PRINT_CENTRE_COLUMN, 3 );
  Print(L" Version: %s \n", (CHAR16 *)PcdGetPtr(PcdFirmwareVersionString));

  // Print menu options
  gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK));

  for (UINTN i = 0; i < sizeof(MenuOptions) / sizeof(MenuOptions[0]); i++) {
    if (!MenuOptions[i].IsActive) {
      break;
    }
    gST->ConOut->SetCursorPosition( gST->ConOut, PRINT_CENTRE_COLUMN, 5+i );
    if (i == SelectedIndex) {
      gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_YELLOW, EFI_BLACK));
    }
    else {
      gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK));
    }

    Print(L"%d. %s ", MenuOptions[i].Index, MenuOptions[i].Name);
  }
}

void MainMenu(
    IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  SelectedIndex     = 0;
  EFI_STATUS Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  ASSERT_EFI_ERROR(Status);

  FillMenu();
}

void HandleKeyInput(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_INPUT_KEY key;
  EFI_STATUS    Status;

  Status = gST->ConIn->ReadKeyStroke(gST->ConIn, &key);

  if (Status != EFI_NOT_READY) {
    ASSERT_EFI_ERROR(Status);

    switch (key.ScanCode) {
      case SCAN_HOME:
        // home button
        MainMenu(ImageHandle, SystemTable);
        break;
      case SCAN_UP:
        // volume up button
        if (SelectedIndex == 0) {
          SelectedIndex = MenuOptionCount - 1;
        }
        else {
          SelectedIndex--;
        }
        break;
      case SCAN_DOWN:
        // volume down button
        if (SelectedIndex == MenuOptionCount - 1) {
          SelectedIndex = 0;
        }
        else {
          SelectedIndex++;
        }
        break;
      case SCAN_ESC:
        // power button
        break;
      default:
        switch (key.UnicodeChar) {
        case CHAR_CARRIAGE_RETURN:
          // dial button
          if (MenuOptions[SelectedIndex].Function != NULL) {
            MenuOptions[SelectedIndex].Function(ImageHandle, SystemTable);
          }
          break;
        case CHAR_TAB:
          // windows button
          DEBUG(
              (EFI_D_ERROR, "%d Menuentries are marked as active\n",
              GetActiveMenuEntryLength()));
          DEBUG((EFI_D_ERROR, "SelectedIndex is: %d\n", SelectedIndex));
          break;
        case CHAR_BACKSPACE:
          // back button
          FallBack = TRUE;
          break;
        default:
          break;
        }
        break;
    }
  }
}

// Start another app
void StartApp(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable, CHAR16 *Description)
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

void StartShell(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS Status = EFI_SUCCESS;

  Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  StartApp(ImageHandle, SystemTable, SHELL_APP_TITLE);
}

void StartTetris(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  StartApp(ImageHandle, SystemTable, TETRIS_APP_TITLE);
}


void RebootMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  SelectedIndex     = 0;
  UINT8 Index = 0;
  EFI_STATUS Status = EFI_SUCCESS;
  
  Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  ASSERT_EFI_ERROR(Status);
  MenuOptions[Index++] = (MenuEntry){Index, L"Reboot to CLK", TRUE, &NullFunction};
  MenuOptions[Index++] = (MenuEntry){Index, L"Reboot", TRUE, &ResetCold};
  MenuOptions[Index++] = (MenuEntry){Index, L"Shutdown", TRUE, &htcleo_shutdown};
  // Fill disabled options
  do {
    MenuOptions[Index++] = (MenuEntry){Index, L"", FALSE, &NullFunction};
  }while(Index < MAX_OPTIONS_COUNT);
}

void SettingsMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  SelectedIndex     = 0;
  UINT8 Index = 0;
  EFI_STATUS Status = EFI_SUCCESS;
  
  Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  ASSERT_EFI_ERROR(Status);
  MenuOptions[Index++] = (MenuEntry){Index, L"Set AD SD dir", TRUE, &SetAndroidSdDir};
  // Fill disabled options
  do {
    MenuOptions[Index++] = (MenuEntry){Index, L"", FALSE, &NullFunction};
  }while(Index < MAX_OPTIONS_COUNT);
}

void NullFunction()
{
  //Print(L"Feature not supported yet!");
  DEBUG((EFI_D_ERROR, "Feature not supported yet!"));
}

void ExitMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  ASSERT_EFI_ERROR(Status);
  SystemTable->BootServices->Exit(ImageHandle, EFI_SUCCESS, 0, NULL);
}

void BootDefault(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
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

EFI_STATUS EFIAPI
ShellAppMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS Status;
  EFI_INPUT_KEY key;
  UINT32 Timeout = 400; //TODO: Get from pcd
  CHAR16 *LoadedDir;

  Print(L" Press Home within %d seconds to boot to menu\n", (Timeout / 100));
  Print(L" Back key to boot from ESP\n");
  Print(L" Power key to boot to builtin UEFI Shell\n");

  do {
    Status = gST->ConIn->ReadKeyStroke(gST->ConIn, &key);

    if (Status != EFI_NOT_READY) {
      ASSERT_EFI_ERROR(Status);

      switch (key.ScanCode) {
        case SCAN_HOME:
          // home button
          goto menu;
          break;
        case CHAR_BACKSPACE:
          goto boot_esp;
          break;
        case SCAN_ESC:
          StartShell(ImageHandle, SystemTable);
          break;
        default:
          break;
      }
    }
    // TODO: Use events?
    MicroSecondDelay(10000);
    Timeout--;
  }while(Timeout);

boot_esp:
  BootDefault(ImageHandle, SystemTable);

  // We should not get here if bootarm.efi is present, inform the user
  Print(L" Could not boot from ESP, loading menu\n");

menu:
      // Load previously selected directory
    Status = LoadSelectedDirFromFile(&SelectedDir);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "Failed to load selected directory from file: %r\n", Status));
        //fallback to root dir of sdcard
        CHAR16* FallBackPath = L"\\";
        FallBack = TRUE;
        *SelectedDir = *FallBackPath;
    }
  // Fill main menu
  FillMenu();

  PrepareConsole(SystemTable->ConOut, &InitialMode);

  // Loop for key input
  while (TRUE) {
    DrawMenu();
    HandleKeyInput(ImageHandle, SystemTable);
  }

  // Restore initial console mode
  RestoreInitialConsoleMode(SystemTable->ConOut, &InitialMode);

  return EFI_SUCCESS;
}