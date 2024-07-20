#include "menu.h"

#include <Device/DeviceType.h>

#define HEX_LENGTH 8
CHAR16 HexBuffer[HEX_LENGTH + 1] = L"00000000";
CHAR16* GetHexInput(EFI_SYSTEM_TABLE *SystemTable, CHAR16* message)
{
  DeviceType Device;
  CHAR16 HexChars[] = L"0123456789ABCDEF";
  EFI_INPUT_KEY Key;
  UINTN Index = 0;
  UINTN HexCharIndex = 0;
  UINTN OldHexCharIndex[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  BOOLEAN InputComplete = FALSE;
  CHAR16 EmptyHexBuffer[HEX_LENGTH + 1] = L"00000000";
#if DEVICETYPE == 1
  Device = LEO;
#endif
#if DEVICETYPE == 2
  Device = SCHUBERT;
#endif
  for (int i = 0; i < HEX_LENGTH + 1; i++){
    HexBuffer[i] = EmptyHexBuffer[i];
  }
  // Clear the screen
  SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

  // Main loop
  while (!InputComplete)
  {
    SystemTable->ConOut->SetCursorPosition(SystemTable->ConOut, 0, 0);
    Print(L"%s", message);
    SystemTable->ConOut->SetCursorPosition(SystemTable->ConOut, 0, 1);
    Print(L"Enter hex value: 0x%s", HexBuffer);

    // Read a key press (replace with actual key reading logic for the device)
    SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key);
switch(Device){
  case BRAVO:
    break;
  case PASSION:
    break;
  case LEO:
    switch (Key.ScanCode)
    {
    case SCAN_ESC: // Move to the next character
      if (Index < HEX_LENGTH - 1)
      {
        OldHexCharIndex[Index] = HexCharIndex;
        Index++;
        HexCharIndex = OldHexCharIndex[Index]; // Reset to start of hex chars
      }
      break;
    default:
      switch (Key.UnicodeChar)
      {
      case CHAR_CARRIAGE_RETURN:
        // dial button
        InputComplete = TRUE;
        break;
      case CHAR_TAB: // Cycle through 0-9, a-f
                     // windows button
        HexCharIndex = (HexCharIndex + 1) % 16;
        HexBuffer[Index] = HexChars[HexCharIndex];
        OldHexCharIndex[Index] = HexCharIndex;
        break;
      case CHAR_BACKSPACE:
        // back button
        if (Index > 0)
          Index--;
        // restore displayed hexchar to its last state
        HexBuffer[Index] = HexChars[OldHexCharIndex[Index]];
        HexCharIndex = OldHexCharIndex[Index]; // Reset to last hex char used on last position
        break;
      default:
        break;
      }
      break;
    }
    break;
  case SCHUBERT:
    switch (Key.ScanCode)
    {
    case SCAN_DOWN: // Move to the next character
      if (Index < HEX_LENGTH - 1)
      {
        OldHexCharIndex[Index] = HexCharIndex;
        Index++;
        HexCharIndex = OldHexCharIndex[Index]; // Reset to start of hex chars
      }
      break;
    case SCAN_UP:// Cycle through 0-9, a-f
        HexCharIndex = (HexCharIndex + 1) % 16;
        HexBuffer[Index] = HexChars[HexCharIndex];
        OldHexCharIndex[Index] = HexCharIndex;
      break;
    default:
      switch (Key.UnicodeChar)
      {
      case CHAR_CARRIAGE_RETURN:
        // dial button
        InputComplete = TRUE;
        break;
      default:
        break;
      }
      break;
    }
    break;  
  }
    // Null-terminate the buffer
    HexBuffer[HEX_LENGTH] = L'\0';
  }

  // Final hex value entered
 
  return HexBuffer;
}