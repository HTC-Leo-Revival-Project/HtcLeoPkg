#include "menu.h"

#define HEX_LENGTH 8

void GetHexInput(UINTN* hexval, CHAR16** hexstring,EFI_SYSTEM_TABLE *SystemTable)
{
  CHAR16 HexChars[] = L"0123456789ABCDEF";
  CHAR16 HexBuffer[HEX_LENGTH + 1] = L"00000000";
  EFI_INPUT_KEY Key;
  UINTN Index = 0;
  UINTN HexCharIndex = 0;
  UINTN OldHexCharIndex[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  BOOLEAN InputComplete = FALSE;

  // Clear the screen
  SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

  // Main loop
  while (!InputComplete)
  {
    // Print the current state of the hex buffer
    SystemTable->ConOut->SetCursorPosition(SystemTable->ConOut, 0, 0);
    Print(L"Enter hex value: 0x%s", HexBuffer);

    // Read a key press (replace with actual key reading logic for the device)
    SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key);

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

    // Null-terminate the buffer
    HexBuffer[HEX_LENGTH] = L'\0';
  }

  // Final hex value entered
  SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
  /* replace next 4 lines with "return StrHexToUintn(&HexBuffer)" */
  UINTN HexValue = StrHexToUintn(&HexBuffer);
  SystemTable->ConOut->SetCursorPosition(SystemTable->ConOut, 0, 1);
 
  *hexval = HexValue;
  *hexstring = HexBuffer;
  Print(L"Final hex as string value: 0x%s\n", hexstring);
  Print(L"Final hex as uint value: 0x%x\n", hexval);
  DEBUG((EFI_D_ERROR, "Final hex as uint value: 0x%x\n", hexval));
  MicroSecondDelay(50000);
}