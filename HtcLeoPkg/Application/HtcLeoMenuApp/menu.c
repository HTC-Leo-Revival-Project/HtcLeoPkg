#include <Uefi.h>
#include <PiDxe.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/ArmLib.h>

#include <Library/HtcLeoPlatformResetLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/TimerLib.h>
#include <Protocol/HtcLeoMicroP.h>
#include <Protocol/HardwareInterrupt.h>

#include <Protocol/LoadedImage.h>
#include <Resources/FbColor.h>
#include <Chipset/timer.h>

#include <Device/microp.h>

#include "menu.h"
#include "CommonHeader.h"
#include "hsusb.h"
#include "udc.h"

HTCLEO_MICROP_PROTOCOL  *gMicrop;

EFI_HARDWARE_INTERRUPT_PROTOCOL *gInterrupt = NULL;

MenuEntry MenuOptions[] = 
{
    {1, L"Option 1", TRUE, &Option1Function},
    {2, L"Option 2", TRUE, &Option2Function},
    {3, L"Play Tetris", TRUE, &StartTetris},
    {4, L"Reboot Menu", TRUE, &RebootMenu},
    {5, L"Exit to Bootmgr", TRUE, &EnterBootMgr},
    {6, L"Test Hexagon", TRUE, &HexagonFunction},
    {7, L"Test Usb", TRUE, &HsUsbFunction},
    {8, L"Exit", TRUE, &ExitMenu}
};

UINTN MenuOptionCount = 0;
UINTN SelectedIndex = 0;

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

EFI_STATUS EFIAPI
ShellAppMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  DEBUG((EFI_D_ERROR, "Main\n"));
  EFI_SIMPLE_TEXT_OUTPUT_MODE InitialMode;
  EFI_STATUS Status;

  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConsoleOut = gST->ConOut;

  // Find the microp protocol.  ASSERT if not found.
  Status = gBS->LocateProtocol (&gHtcLeoMicropProtocolGuid, NULL, (VOID **)&gMicrop);
  ASSERT_EFI_ERROR (Status);

  PrepareConsole(SystemTable->ConOut, &InitialMode);

  // Loop that never exits
  while (TRUE) {
    DrawMenu(ConsoleOut);
    HandleKeyInput(ImageHandle, SystemTable);
  }

  // Restore initial console mode
  RestoreInitialConsoleMode(SystemTable->ConOut, &InitialMode);

  return EFI_SUCCESS;
}

void DrawMenu(IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConsoleOut)
{
  MenuOptionCount = GetActiveMenuEntryLength();

  // Print menu title
  gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_RED));
  SetCursorPos(15, 1);
  Print(L" HtcLeoRevivalProject EDK2 Main Menu ");

  // Print menu options
  ConsoleOut->SetAttribute(ConsoleOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK));

  for (UINTN i = 0; i < sizeof(MenuOptions) / sizeof(MenuOptions[0]); i++) {
    if (!MenuOptions[i].IsActive) {
      break;
    }
    SetCursorPos(25, 3 + i);
    if (i == SelectedIndex) {
      ConsoleOut->SetAttribute(
          ConsoleOut, EFI_TEXT_ATTR(EFI_YELLOW, EFI_BLACK));
    }
    else {
      ConsoleOut->SetAttribute(ConsoleOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK));
    }

    Print(L"%d. %s ", MenuOptions[i].Index, MenuOptions[i].Name);
  }
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
      ReturnToMainMenu(ImageHandle, SystemTable);
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
        break;
      default:
        break;
      }
      break;
    }
  }
}

EFI_STATUS StartAnotherApp(
    IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable,
    IN EFI_GUID *AppGuid)
{
  DEBUG((EFI_D_ERROR, "Start another app\n"));
  MicroSecondDelay(3000);

  EFI_STATUS Status;

  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  EFI_HANDLE                 AppImageHandle;

  // Get the LoadedImage protocol of the current application
  DEBUG((EFI_D_ERROR, "Assigning loaded image protocol guid\n"));
  MicroSecondDelay(3000);
  Status = gBS->HandleProtocol(
      ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **)&LoadedImage);
  if (EFI_ERROR(Status)) {

    return Status;
  }
  DEBUG((EFI_D_ERROR, "Starting application\n"));
  MicroSecondDelay(3000);
  // Load the EFI application image by its GUID
  Status = gBS->LoadImage(TRUE, ImageHandle, AppGuid, NULL, 0, AppImageHandle);
  if (EFI_ERROR(Status)) {

    return Status;
  }

  // Start the EFI application
  Status = gBS->StartImage(AppImageHandle, NULL, NULL);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR, "You selected Option 4\n"));
    MicroSecondDelay(3000);
    // Handle the error condition if needed
  }

  return Status;
}





INTN htcleo_panel_status = 0;
static void htcleo_panel_bkl_pwr(INTN enable)
{
	htcleo_panel_status = enable;
	UINT8 data[1];
	data[0] = !!enable;
	gMicrop->Write(MICROP_I2C_WCMD_BL_EN, data, 1);
}

static void htcleo_panel_bkl_level(UINT8 value)
{
	UINT8 data[1];
	data[0] = value << 4;
	gMicrop->Write(MICROP_I2C_WCMD_LCM_BL_MANU_CTL, data, 1);
}

void htcleo_panel_set_brightness(INTN val)
{
	if (val > 9) val = 9;
	if (val < 1) {
		if (htcleo_panel_status != 0)
			htcleo_panel_bkl_pwr(0);
		return;
	} else {
		if (htcleo_panel_status == 0)
			htcleo_panel_bkl_pwr(1);
		htcleo_panel_bkl_level((UINT8)val);
		return;
	}
}

void Option1Function(
    IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  DEBUG((EFI_D_ERROR, "You selected Option 1\n"));
  htcleo_panel_set_brightness(1);
}

void Option2Function(
    IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  DEBUG((EFI_D_ERROR, "You selected Option 2\n"));
  htcleo_panel_set_brightness(9);
}

#define HexagonMemStart 0x10000000
#define HexagonMemSize 0x01800000

VOID
TestHexagonCopy(VOID)
{
  UINT32 WriteVal = 0x43454345;

  MmioWrite32(HexagonMemStart, WriteVal);
  UINT32 ReadVal = MmioRead32(HexagonMemStart);

  if(ReadVal == WriteVal) {
      DEBUG((EFI_D_ERROR, "Hexagon memory write success\n"));
  }
  else {
      DEBUG((EFI_D_ERROR, "Hexagon memory write failed\n"));
  }
}

#define DISABLE TRUE
#define ENABLE FALSE

VOID
ConfigureMpu(BOOLEAN Disable)
{
  ArmDisableDataCache ();
  // Invalidate instruction cache
  ArmInvalidateInstructionCache ();

  asm(" MRC p15, 0, R1, c1, c0, 0");     //read CP15 register 1
  if(Disable) {
    asm(" BIC R1, R1, #0x1"); // Disable MPU
  }
  else {
    asm(" ORR R1, R1, #0x1"); // Enable MPU
  }

  ArmDataSynchronizationBarrier();
  asm(" MCR p15, 0, R1, c1, c0, 0");  //write CP15 register 1
  ArmInstructionSynchronizationBarrier();
}

extern UINTN udc_start();

static struct udc_device surf_udc_device = {
	.vendor_id	= 0x18d1,
	.product_id	= 0x0D02,
	.version_id	= 0x0001,
	.manufacturer	= "google",
	.product	= "EDK2",
};

extern UINTN udc_init(struct udc_device *dev);

void HsUsbFunction(
    IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS  Status;

  // Find the interrupt controller protocol.  ASSERT if not found.
  Status = gBS->LocateProtocol (&gHardwareInterruptProtocolGuid, NULL, (VOID **)&gInterrupt);
  ASSERT_EFI_ERROR (Status);

  DEBUG((EFI_D_ERROR, "Starting udc\n"));
  udc_init(&surf_udc_device);
  udc_start(gInterrupt);
}

void HexagonFunction(
    IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  DEBUG((EFI_D_ERROR, "----------Hexagon Test-------------\n"));
  ConfigureMpu(DISABLE);
  TestHexagonCopy();

  /* TODO :
   * Find the sequence to get the Hexagon out of reset (thanks to Jon Pry for mentioning it)
   * Get some test code to run 
   *
   * Current idea : 
   * (1) Disable MPU (done)
   * (2) Copy code to hexagon mem
   * (3) Bring the hexagon out of reset, halt the AP (thanks to minecrell for suggestions)
   */

  //ConfigureMpu(ENABLE);
  DEBUG((EFI_D_ERROR, "----------Hexagon Test End-------------\n"));
}

void EnterBootMgr(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable){
  EFI_STATUS                   Status;
  EFI_BOOT_MANAGER_LOAD_OPTION BootManagerMenu;
  Status = EfiBootManagerGetBootManagerMenu(&BootManagerMenu);
    if (!EFI_ERROR(Status)) {
    EfiBootManagerBoot(&BootManagerMenu);
  }
  else {
    ResetCold();
  }
}

void RebootMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  SelectedIndex     = 0;
  EFI_STATUS Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  ASSERT_EFI_ERROR(Status);
  MenuOptions[0] = (MenuEntry){1, L"Reboot to CLK", TRUE, &NullFunction};
  MenuOptions[1] = (MenuEntry){2, L"Reboot", TRUE, &ResetCold};
  MenuOptions[2] = (MenuEntry){3, L"Shutdown", TRUE, &htcleo_shutdown};
  MenuOptions[3] = (MenuEntry){4, L"", FALSE, &NullFunction};
  MenuOptions[4] = (MenuEntry){5, L"", FALSE, &NullFunction};
  MenuOptions[5] = (MenuEntry){6, L"", FALSE, &NullFunction};
}

void ReturnToMainMenu(
    IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  SelectedIndex     = 0;
  EFI_STATUS Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  ASSERT_EFI_ERROR(Status);

  MenuOptions[0] = (MenuEntry){1, L"Option 1", TRUE, &Option1Function};
  MenuOptions[1] = (MenuEntry){2, L"Option 2", TRUE, &Option2Function};
  MenuOptions[2] = (MenuEntry){3, L"Play Tetris", TRUE, &StartTetris};
  MenuOptions[3] = (MenuEntry){4, L"Reboot Menu", TRUE, &RebootMenu};
  MenuOptions[4] = (MenuEntry){5, L"Exit to Bootmgr", TRUE, &EnterBootMgr},
  MenuOptions[5] = (MenuEntry){6, L"Test Hexagon", TRUE, &HexagonFunction},
  MenuOptions[6] = (MenuEntry){6, L"Test Usb", TRUE, &HsUsbFunction},
  MenuOptions[7] = (MenuEntry){7, L"Exit", TRUE, &ExitMenu};
}

void NullFunction()
{
  DEBUG((EFI_D_ERROR, "This feature is not supported yet\n"));
}

void ExitMenu(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_STATUS Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  ASSERT_EFI_ERROR(Status);
  SystemTable->BootServices->Exit(ImageHandle, EFI_SUCCESS, 0, NULL);
}

void StartTetris(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{

  EFI_GUID TetrisAppGuid = {
      0xD7A59C0A,
      0x899E,
      0x4E31,
      {0xA1, 0xC3, 0x3D, 0x55, 0x1D, 0x7A, 0x53, 0x81}};

  // ...
  //MicroSecondDelay(1000);
  EFI_STATUS Status = StartAnotherApp(ImageHandle, SystemTable, &TetrisAppGuid);
}
