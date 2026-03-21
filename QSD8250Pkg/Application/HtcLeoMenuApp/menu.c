#include "menu.h"
#include "BootApp.h"

MenuEntry MenuOptions[MAX_OPTIONS_COUNT] = {0};

UINTN MenuOptionCount = 0;
UINTN SelectedIndex = 0;
EFI_SIMPLE_TEXT_OUTPUT_MODE InitialMode;
EFI_WATCHDOG_TIMER_ARCH_PROTOCOL *WatchdogTimer;
UINT64 TimerPeriod = 120 * 10 * 1000 * 1000;
UINT64 FeedInterval = 60 * 1000 * 1000;

void
FillMenu()
{
  UINTN Index = 0;
  MenuOptions[Index++] = (MenuEntry){L"Boot default", TRUE, &BootDefault};
  MenuOptions[Index++] = (MenuEntry){L"Fastboot", TRUE, &StartFastbootApp};
  MenuOptions[Index++] = (MenuEntry){L"Play Tetris", TRUE, &StartTetris};
  MenuOptions[Index++] = (MenuEntry){L"EFI Shell", TRUE, &StartShell},
  MenuOptions[Index++] = (MenuEntry){L"Dump DMESG to sdcard", TRUE, &DumpDmesg},
  MenuOptions[Index++] = (MenuEntry){L"Dump Memory to sdcard", TRUE, &DumpMemory2Sdcard},
  MenuOptions[Index++] = (MenuEntry){L"Reboot Menu", TRUE, &RebootMenu};
  MenuOptions[Index++] = (MenuEntry){L"Exit", TRUE, &ExitMenu};
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

    Print(L"%d. %s ", i+1, MenuOptions[i].Name);
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
          break;
        case CHAR_BACKSPACE:
          // back button
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

void StartFastbootApp(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  StartApp(ImageHandle, SystemTable, FASTBOOT_APP_TITLE);
}

void RebootMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  SelectedIndex     = 0;
  UINT8 Index = 0;
  EFI_STATUS Status = EFI_SUCCESS;
  
  Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  ASSERT_EFI_ERROR(Status);
  MenuOptions[Index++] = (MenuEntry){L"Reboot to CLK", TRUE, &NullFunction};
  MenuOptions[Index++] = (MenuEntry){L"Reboot", TRUE, &ResetCold};
  MenuOptions[Index++] = (MenuEntry){L"Shutdown", TRUE, &ResetShutdown};
  // Fill disabled options
  do {
    MenuOptions[Index++] = (MenuEntry){L"", FALSE, &NullFunction};
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

/**
  Periodic timer callback function to feed the watchdog.

  @param Event   The event that triggered the callback.
  @param Context Optional context parameter, not used here.
**/
VOID
EFIAPI
FeedWatchdogCallback (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
    EFI_STATUS Status;

    Status = WatchdogTimer->SetTimerPeriod(WatchdogTimer, TimerPeriod);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "Failed to feed the watchdog: %r\n", Status));
    }
}

EFI_STATUS EFIAPI
ShellAppMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS Status;
  EFI_INPUT_KEY key;
  UINT32 Timeout = 400; //TODO: Get from pcd
   EFI_EVENT TimerEvent;

    Status = gBS->LocateProtocol(&gEfiWatchdogTimerArchProtocolGuid, NULL, (VOID **)&WatchdogTimer);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "Failed to locate Watchdog Timer protocol: %r\n", Status));
        return Status;
    }

    Status = WatchdogTimer->SetTimerPeriod(WatchdogTimer, TimerPeriod);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "Failed to set initial watchdog timer period: %r\n", Status));
        return Status;
    }


    Status = gBS->CreateEvent(
                    EVT_TIMER | EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    FeedWatchdogCallback,
                    NULL,
                    &TimerEvent
                 );
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "Failed to create periodic timer event: %r\n", Status));
        return Status;
    }

    Status = gBS->SetTimer(TimerEvent, TimerPeriodic, 60 * 10 * 1000 * 1000);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "Failed to set periodic timer: %r\n", Status));
        gBS->CloseEvent(TimerEvent);
        return Status;
    }

  Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  ASSERT_EFI_ERROR(Status);

  Print(L" Press Home within %d seconds to boot to menu\n", (Timeout / 100));
  Print(L" Back key to boot from ESP\n");
  Print(L" Power key to boot to builtin UEFI Shell\n");

  do {
    Status = SystemTable->ConOut->SetCursorPosition(SystemTable->ConOut, 0, 0);
    ASSERT_EFI_ERROR(Status);

    Print(L" Press Home within %d seconds to boot to menu\n", (Timeout / 100));

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
  } while (Timeout > 0);

boot_esp:
  BootDefault(ImageHandle, SystemTable);

  // We should not get here if bootarm.efi is present, inform the user
  Print(L" Could not boot from ESP, loading menu\n");

menu:
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