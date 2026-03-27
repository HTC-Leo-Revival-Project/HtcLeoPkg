#include "menu.h"
#include "BootApp.h"
#include <Library/SerialPortLib.h>

STATIC
UINTN ParseHex(CHAR8 *Str)
{
    UINTN Result = 0;

    if (Str == NULL) {
        return 0;
    }

    if (Str[0] == '0' && (Str[1] == 'x' || Str[1] == 'X')) {
        Str += 2;
    }

    while (*Str) {
        CHAR8 c = *Str++;

        Result <<= 4;

        if (c >= '0' && c <= '9') {
            Result += c - '0';
        } else if (c >= 'a' && c <= 'f') {
            Result += c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            Result += c - 'A' + 10;
        } else {
            break;
        }
    }

    return Result;
}

STATIC
CHAR8* NextArg(CHAR8 **Str)
{
    CHAR8 *Start = *Str;

    if (Start == NULL) {
        return NULL;
    }

    while (*Start == ' ') {
        Start++;
    }

    if (*Start == '\0') {
        return NULL;
    }

    CHAR8 *End = Start;

    while (*End && *End != ' ') {
        End++;
    }

    if (*End) {
        *End = '\0';
        *Str = End + 1;
    } else {
        *Str = End;
    }

    return Start;
}

STATIC
VOID CmdRead(CHAR8 *Args)
{
    CHAR8 *ptr = Args;

    CHAR8 *addrStr = NextArg(&ptr);
    CHAR8 *sizeStr = NextArg(&ptr);

    if (!addrStr) {
        SerialPortWrite((UINT8*)"Usage: read <addr> [size]\r\n", 28);
        return;
    }

    UINTN addr = ParseHex(addrStr);
    UINTN size = sizeStr ? ParseHex(sizeStr) : 1;

    UINT8 *mem = (UINT8*)addr;

    CONST UINTN ROW_SIZE = 0x10;

    for (UINTN offset = 0; offset < size; offset += ROW_SIZE) {

        CHAR8 header[32];
        AsciiSPrint(header, sizeof(header), "0x%lx> ", (UINT64)(addr + offset));
        SerialPortWrite((UINT8*)header, AsciiStrLen(header));

        //  row
        for (UINTN i = 0; i < ROW_SIZE; i++) {

            if (offset + i >= size) {
                break;
            }

            UINT8 val = mem[offset + i];

            CHAR8 byteOut[4];
            AsciiSPrint(byteOut, sizeof(byteOut), "%02x ", val);
            SerialPortWrite((UINT8*)byteOut, 3);
        }

        SerialPortWrite((UINT8*)"\r\n", 2);
    }
}

STATIC
VOID CmdWrite(CHAR8 *Args)
{
    CHAR8 *ptr = Args;

    CHAR8 *addrStr = NextArg(&ptr);
    CHAR8 *valStr  = NextArg(&ptr);

    if (!addrStr || !valStr) {
        SerialPortWrite((UINT8*)"Usage: write <addr> <value>\r\n", 30);
        return;
    }

    UINTN addr = ParseHex(addrStr);
    UINTN val  = ParseHex(valStr);

    UINT8 *mem = (UINT8*)addr;
    *mem = (UINT8)val;

    SerialPortWrite((UINT8*)"OK\r\n", 4);
}



STATIC
VOID PrintPrompt(VOID)
{
    SerialPortWrite((UINT8*)PROMPT, AsciiStrLen(PROMPT));
}

VOID TtyShell(VOID){
    UINT8 Ch;
    CHAR8 CmdBuffer[64];
    UINTN Index = 0;

    SerialPortWrite((UINT8*)"\r\n", 2);
    PrintPrompt();

    while (TRUE) {

        if (!SerialPortPoll()) {
            continue;
        }

        SerialPortRead(&Ch, 1);

        // Enter
        if (Ch == '\r' || Ch == '\n') {

            SerialPortWrite((UINT8*)"\r\n", 2);

            CmdBuffer[Index] = '\0';

            if (Index > 0) {

CHAR8 *ptr = CmdBuffer;


CHAR8 *cmd  = NextArg(&ptr);
CHAR8 *args = ptr; 

if (!cmd) {
    SerialPortWrite((UINT8*)"Empty command\r\n", 15);
    goto prompt;
}

if (AsciiStrCmp(cmd, "exit") == 0) {
    SerialPortWrite((UINT8*)"Exiting...\r\n", 13);
    break;
}
else if (AsciiStrCmp(cmd, "read") == 0) {
    CmdRead(args);
}
else if (AsciiStrCmp(cmd, "write") == 0) {
    CmdWrite(args);
}
else if (AsciiStrCmp(cmd, "reboot") == 0) {
    ResetCold();
}
else {
    SerialPortWrite((UINT8*)"Unknown command\r\n", 18);
}

prompt:
Index = 0;
PrintPrompt();
continue;
            }

            Index = 0;
            PrintPrompt();
            continue;
        }

        if (Ch < 0x20) {
            continue;
        }

        if (Index < sizeof(CmdBuffer) - 1) {
            CmdBuffer[Index++] = (CHAR8)Ch;
        }
    }
}