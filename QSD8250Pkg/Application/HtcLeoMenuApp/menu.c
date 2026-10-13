#include "menu.h"
#include "BootApp.h"
#include "functions.h"

MenuEntry MenuOptions[MAX_OPTIONS_COUNT] = {0};

UINTN MenuOptionCount = 0;
UINTN SelectedIndex = 0;
EFI_SIMPLE_TEXT_OUTPUT_MODE *InitialMode;
EFI_WATCHDOG_TIMER_ARCH_PROTOCOL *WatchdogTimer;
HTC_DEVICE_DETECTION_PROTOCOL *DeviceDetectionProtocol;
UINT64 TimerPeriod = 120 * 10 * 1000 * 1000;
UINT64 FeedInterval = 60 * 1000 * 1000;
HtcDevice* mDevice;

VOID
FillMenu()
{
  UINTN Index = 0;
  MenuOptions[Index++] = (MenuEntry){L"Boot default", TRUE, &BootDefault};
  MenuOptions[Index++] = (MenuEntry){L"Boot Android", TRUE, &BootAndroidKernel};
  MenuOptions[Index++] = (MenuEntry){L"Fastboot", TRUE, &StartFastbootApp};
  MenuOptions[Index++] = (MenuEntry){L"Play Tetris", TRUE, &StartTetris};
  MenuOptions[Index++] = (MenuEntry){L"EFI Shell", TRUE, &StartShell},
  MenuOptions[Index++] = (MenuEntry){L"Dump DMESG to sdcard", TRUE, &DumpDmesg},
  MenuOptions[Index++] = (MenuEntry){L"Dump Memory to sdcard", TRUE, &DumpMemory2Sdcard},
  MenuOptions[Index++] = (MenuEntry){L"Reboot Menu", TRUE, &RebootMenu};
  MenuOptions[Index++] = (MenuEntry){L"Exit", TRUE, &ExitMenu};
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

VOID DrawMenu()
{
  MenuOptionCount = GetActiveMenuEntryLength();
  UINTN visibleIndex = 0;

  // Print menu title
  gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_RED, EFI_BLACK));
  gST->ConOut->SetCursorPosition( gST->ConOut, PRINT_CENTRE_COLUMN-4, 1 ); // offset by 4 so the next line will center under the name
  
  Print(L" %s \n", (CHAR16 *)PcdGetPtr(PcdFirmwareVendor));

  gST->ConOut->SetCursorPosition(gST->ConOut, PRINT_CENTRE_COLUMN, 2);
  Print(L" EDK2 Main Menu \n");

  gST->ConOut->SetCursorPosition(gST->ConOut, PRINT_CENTRE_COLUMN, 3);
  Print(L" Version: %s \n", (CHAR16 *)PcdGetPtr(PcdFirmwareVersionString));

  // Print menu options
  gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK));

  for (UINTN i = 0; i < sizeof(MenuOptions) / sizeof(MenuOptions[0]); i++) {
    if (!MenuOptions[i].IsActive) {
      continue;
    }
    gST->ConOut->SetCursorPosition(gST->ConOut,PRINT_CENTRE_COLUMN,5 + visibleIndex);
    if (visibleIndex == SelectedIndex) {
      gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_YELLOW, EFI_BLACK));
    } else {
      gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK));
    }

    Print(L"%d. %s ", visibleIndex + 1, MenuOptions[i].Name);

    visibleIndex++;
  }
}

VOID MainMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  SelectedIndex     = 0;
  EFI_STATUS Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  ASSERT_EFI_ERROR(Status);

  FillMenu();
}

UINTN GetRealIndexFromVisibleIndex(UINTN visibleIndex)
{
  UINTN count = 0;

  for (UINTN i = 0; i < MAX_OPTIONS_COUNT; i++) {
    if (!MenuOptions[i].IsActive)
      continue;

    if (count == visibleIndex)
      return i;

    count++;
  }

  return -1;
}

VOID HandleKeyInput(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_INPUT_KEY key;
  EFI_STATUS    Status;
  UINTN realIndex;

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
          realIndex = GetRealIndexFromVisibleIndex(SelectedIndex);
          if (realIndex >= 0 && MenuOptions[realIndex].Function != NULL) {
          MenuOptions[realIndex].Function(ImageHandle, SystemTable);
         }
          break;
        case CHAR_TAB:
          //Leo - windows button
          //Passion - Trackball right
          DEBUG((EFI_D_ERROR, "SelectedIndex is %d \n", SelectedIndex));
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

VOID RebootMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  SelectedIndex     = 0;
  UINT8 Index = 0;
  EFI_STATUS Status = EFI_SUCCESS;
  
  Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  ASSERT_EFI_ERROR(Status);
  MenuOptions[Index++] = (MenuEntry){L"Reboot to CLK", mDevice->cLK, &SetClkBoot};
  MenuOptions[Index++] = (MenuEntry){L"Reboot", TRUE, &ResetCold};
  MenuOptions[Index++] = (MenuEntry){L"Shutdown", TRUE, &ResetShutdown};
  // Fill disabled options
  do {
    MenuOptions[Index++] = (MenuEntry){L"", FALSE, &NullFunction};
  }while(Index < MAX_OPTIONS_COUNT);
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

// 10 ms countdown slice, in 100 ns units.
#define TIMEOUT_SLICE_100NS  (10 * 1000 * 10)

/**
  Cancel the boot countdown timer.

  @param[in]  Event  The timer event to cancel.
**/
STATIC
VOID
CancelBootTimer (
  IN EFI_EVENT  Event
  )
{
  if (Event != NULL) {
    gBS->SetTimer (Event, TimerCancel, 0);
  }
}

EFI_STATUS EFIAPI
ShellAppMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS Status;
  EFI_INPUT_KEY key;
  UINT32 Timeout = 400; //TODO: Get from pcd
  EFI_EVENT TimerEvent;
  EFI_EVENT BootTimerEvent;
  EFI_EVENT BootEvents[2];
  UINTN      EventIndex;

    Status = gBS->LocateProtocol(&gHtcDeviceDetectionProtocolGuid, NULL, (VOID **)&DeviceDetectionProtocol);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "Failed to locate the Device Detection protocol: %r\n", Status));
        return Status;
    }

    *mDevice = DeviceDetectionProtocol->GetInfo();

    Status = gBS->LocateProtocol(&gEfiWatchdogTimerArchProtocolGuid, NULL, (VOID **)&WatchdogTimer);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "Failed to locate the Watchdog Timer protocol: %r\n", Status));
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
  #if DEVICETYPE == 4
    CONST CHAR16 *buttonString = L"Jogball";
  #else
    CONST CHAR16 *buttonString = L"Home";
  #endif

  Print(L" Press %s within %d seconds to boot to menu\n", buttonString, (Timeout / 100));
  Print(L" Back key to boot from ESP\n");
  Print(L" Power key to boot to builtin UEFI Shell\n");

  //
  // Boot countdown driven by a single timer event rather than a
  // MicroSecondDelay() spin. EVT_TIMER has to be used on its own: adding
  // EVT_NOTIFY_SIGNAL makes the core demand a non-NULL NotifyFunction and
  // CreateEvent() fails with EFI_INVALID_PARAMETER.
  //
  Status = gBS->CreateEvent (EVT_TIMER, TPL_HIGH_LEVEL, NULL, NULL, &BootTimerEvent);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "Failed to create boot timer event: %r\n", Status));
    return Status;
  }

  // Arm the whole countdown in one shot. Timeout counts 10 ms slices.
  Status = gBS->SetTimer (BootTimerEvent, TimerRelative, Timeout * TIMEOUT_SLICE_100NS);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "Failed to arm boot timer: %r\n", Status));
    gBS->CloseEvent (BootTimerEvent);
    return Status;
  }

  // Wait on the countdown and the keypad together. WaitForEvent reads
  // NumberOfEvents entries, so the array has to be exactly that long.
  BootEvents[0] = BootTimerEvent;
  BootEvents[1] = gST->ConIn->WaitForKey;
  if (BootEvents[1] == NULL) {
    DEBUG((EFI_D_ERROR, "Console input has no WaitForKey event\n"));
    gBS->CloseEvent (BootTimerEvent);
    return EFI_NOT_FOUND;
  }

  while (TRUE) {
    Status = SystemTable->ConOut->SetCursorPosition (SystemTable->ConOut, 0, 0);
    ASSERT_EFI_ERROR (Status);

    Print (L" Press %s within %d seconds to boot to menu\n", buttonString, (Timeout / 100));

    Status = gBS->WaitForEvent (2, BootEvents, &EventIndex);
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "WaitForEvent failed: %r\n", Status));
      break;
    }

    if (EventIndex == 0) {
      // Countdown expired.
      break;
    }

    // EventIndex == 1: the keypad signalled, a key is pending.

    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &key);
    if (Status == EFI_NOT_READY) {
      continue;
    }
    ASSERT_EFI_ERROR (Status);

    switch (key.ScanCode) {
      case SCAN_HOME:
        // home button
#if DEVICETYPE != 4
        CancelBootTimer (BootTimerEvent);
        goto menu;
#endif
        break;
      case CHAR_BACKSPACE:
        CancelBootTimer (BootTimerEvent);
        goto boot_esp;
        break;
      case SCAN_ESC:
        CancelBootTimer (BootTimerEvent);
        StartShell (ImageHandle, SystemTable);
        break;
      default:
        switch (key.UnicodeChar) {
          case CHAR_CARRIAGE_RETURN:
#if DEVICETYPE == 4
            CancelBootTimer (BootTimerEvent);
            goto menu;
#endif
            break;

          case CHAR_TAB:
            break;

          case CHAR_BACKSPACE:
            break;
          default:
            break;
        }
        break;
    }
  }

  // Timed out (or the wait failed): fall through to booting the ESP.
  CancelBootTimer (BootTimerEvent);
  gBS->CloseEvent (BootTimerEvent);

boot_esp:
  BootDefault(ImageHandle, SystemTable);

  // We should not get here if bootarm.efi is present, inform the user
  Print(L" Could not boot from ESP, loading menu\n");

menu:
  // Fill main menu
  FillMenu();

  PrepareConsole(SystemTable->ConOut, InitialMode);

  // Loop for key input
  while (TRUE) {
    DrawMenu();
    HandleKeyInput(ImageHandle, SystemTable);
  }

  // Restore initial console mode
  RestoreInitialConsoleMode(SystemTable->ConOut, InitialMode);

  return EFI_SUCCESS;
}