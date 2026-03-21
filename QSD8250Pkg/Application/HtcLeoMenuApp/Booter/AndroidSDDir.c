#include "AndroidSDDir.h"
#include "../menu.h"

//by default the selected dir is / (root of sdcard)

CHAR16 *SelectedDir = L"\\";
BOOLEAN FallBack = FALSE;

EFI_STATUS ListDirectories(EFI_HANDLE *HandleBuffer, UINTN HandleCount, CHAR16 ***DirList, UINTN *DirCount) {
    EFI_STATUS Status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    EFI_FILE_PROTOCOL *Root;
    EFI_FILE_INFO *FileInfo;
    UINTN BufferSize;
    CHAR16 **Directories;
    UINTN DirectoryCount = 0;

    Directories = AllocateZeroPool(HandleCount * sizeof(CHAR16*));

    for (UINTN Index = 0; Index < HandleCount; Index++) {
        Status = gBS->HandleProtocol(HandleBuffer[Index], &gEfiSimpleFileSystemProtocolGuid, (VOID **)&FileSystem);
        if (EFI_ERROR(Status)) {
            continue;
        }

        Status = FileSystem->OpenVolume(FileSystem, &Root);
        if (EFI_ERROR(Status)) {
            continue;
        }

        BufferSize = SIZE_OF_EFI_FILE_INFO + 256 * sizeof(CHAR16);
        FileInfo = AllocateZeroPool(BufferSize);

        while (Root->Read(Root, &BufferSize, FileInfo) == EFI_SUCCESS && BufferSize > 0) {
            if (FileInfo->Attribute & EFI_FILE_DIRECTORY) {
                Directories[DirectoryCount] = AllocateCopyPool(StrSize(FileInfo->FileName), FileInfo->FileName);
                DirectoryCount++;
            }
            BufferSize = SIZE_OF_EFI_FILE_INFO + 256 * sizeof(CHAR16);
            ZeroMem(FileInfo, BufferSize);
        }

        FreePool(FileInfo);
        Root->Close(Root);
    }

    *DirList = Directories;
    *DirCount = DirectoryCount;

    return EFI_SUCCESS;
}

EFI_STATUS DisplayAndSelectDirectory(CHAR16 **DirList, UINTN DirCount, CHAR16 **SelectedDir) {
    EFI_INPUT_KEY Key;
    UINTN Index = 0;
    UINTN MaxColumn, MaxRow, CentreColumn;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut = gST->ConOut;

    ConOut->QueryMode(ConOut, ConOut->Mode->Mode, &MaxColumn, &MaxRow);
    CentreColumn = MaxColumn / 2;

    while (TRUE) {
        ConOut->ClearScreen(ConOut);
        ConOut->SetCursorPosition(ConOut, CentreColumn - 10, 1); // Adjust the x-axis position as needed
        ConOut->SetAttribute(ConOut, EFI_TEXT_ATTR(EFI_RED, EFI_BLACK));
        Print(L"Select a directory:\n");
        ConOut->SetAttribute(ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK));
        ConOut->SetCursorPosition(ConOut, CentreColumn - 10, 3);
        for (UINTN i = 0; i < DirCount; i++) { // Changed loop starting point to 0
            ConOut->SetCursorPosition(ConOut, CentreColumn - 10, i + 3); // Adjust the x-axis position as needed
            if (i == Index) {
                ConOut->SetAttribute(ConOut, EFI_TEXT_ATTR(EFI_YELLOW, EFI_BLACK));
                Print(L"%s\n", DirList[i]);
                ConOut->SetAttribute(ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK));
            } else {
                Print(L"%s\n", DirList[i]);
            }
        }

        gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, NULL);
        gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);

        if (Key.ScanCode == SCAN_UP) {
            if (Index > 0) {
                Index--;
            }
        } else if (Key.ScanCode == SCAN_DOWN) {
            if (Index < DirCount - 1) {
                Index++;
            }
        } else if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
            *SelectedDir = DirList[Index];
            break;
        }
    }
    ConOut->ClearScreen(ConOut);
    return EFI_SUCCESS;
}

EFI_STATUS SaveSelectedDirToFile(CHAR16 *DirPath) {
    EFI_STATUS Status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    EFI_FILE_PROTOCOL *Root, *File;
    EFI_HANDLE *HandleBuffer;
    UINTN HandleCount;
    UINTN BufferSize = StrSize(DirPath);
    
    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &HandleBuffer);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = gBS->HandleProtocol(HandleBuffer[0], &gEfiSimpleFileSystemProtocolGuid, (VOID **)&FileSystem);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = FileSystem->OpenVolume(FileSystem, &Root);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = Root->Open(Root, &File, CONFIG_FILE_PATH, EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status)) {
        Root->Close(Root);
        return Status;
    }

    Status = File->Write(File, &BufferSize, DirPath);
    File->Close(File);
    Root->Close(Root);

    return Status;
}

EFI_STATUS LoadSelectedDirFromFile(CHAR16 **DirPath) {
    EFI_STATUS Status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
    EFI_FILE_PROTOCOL *Root, *File;
    EFI_HANDLE *HandleBuffer;
    UINTN HandleCount;
    UINTN BufferSize;
    CHAR16 *Buffer;

    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &HandleBuffer);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = gBS->HandleProtocol(HandleBuffer[0], &gEfiSimpleFileSystemProtocolGuid, (VOID **)&FileSystem);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = FileSystem->OpenVolume(FileSystem, &Root);
    if (EFI_ERROR(Status)) {
        return Status;
    }

    Status = Root->Open(Root, &File, CONFIG_FILE_PATH, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(Status)) {
        Root->Close(Root);
        return Status;
    }

    BufferSize = 256 * sizeof(CHAR16);
    Buffer = AllocateZeroPool(BufferSize);

    Status = File->Read(File, &BufferSize, Buffer);
    if (EFI_ERROR(Status)) {
        FreePool(Buffer);
        File->Close(File);
        Root->Close(Root);
        return Status;
    }

    *DirPath = Buffer;
    File->Close(File);
    Root->Close(Root);

    return Status;
}

void SetAndroidSdDir(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS Status;
    EFI_HANDLE *HandleBuffer;
    UINTN HandleCount;
    CHAR16 **DirList;
    UINTN DirCount;

    Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    ASSERT_EFI_ERROR(Status);

    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &HandleBuffer);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "Failed to locate handles for Simple File System Protocol: %r\n", Status));
    }

    Status = ListDirectories(HandleBuffer, HandleCount, &DirList, &DirCount);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "Failed to list directories: %r\n", Status));
    }

    Status = DisplayAndSelectDirectory(DirList, DirCount, &SelectedDir);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "Failed to select directory: %r\n", Status));
    }

    DEBUG((EFI_D_ERROR, "Failed to select directory: %r\n", Status));
    //we are done here return to main menu
    Status = SaveSelectedDirToFile(SelectedDir);
    if (EFI_ERROR(Status)) {
        DEBUG((EFI_D_ERROR, "Failed to save selected directory to file: %r\n", Status));
    }
    MainMenu(ImageHandle, SystemTable);
}