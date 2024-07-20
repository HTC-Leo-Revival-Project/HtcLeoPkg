#include "../menu.h"
EFI_STATUS
EFIAPI
LoadFileFromSDCard(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable,
    IN CHAR16 *KernelFileName,
    IN VOID *LoadAddress, // New parameter for custom load address
    OUT VOID **KernelBuffer,
    OUT UINTN *KernelSize
)
{
    EFI_STATUS Status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SimpleFileSystem;
    EFI_FILE_PROTOCOL *Root;
    EFI_FILE_PROTOCOL *KernelFile;
    UINTN BufferSize;
    VOID *Buffer = NULL;

    // Locate the handle for the SD card
    EFI_HANDLE *Handles;
    UINTN HandleCount;
    Status = SystemTable->BootServices->LocateHandleBuffer(
                ByProtocol,
                &gEfiSimpleFileSystemProtocolGuid,
                NULL,
                &HandleCount,
                &Handles
             );
    if (EFI_ERROR(Status)) {
        Print(L"Failed to locate handles for SimpleFileSystem: %r\n", Status);
        return Status;
    }

    // Assume the SD card is the first handle found
    Status = SystemTable->BootServices->HandleProtocol(
                Handles[0],
                &gEfiSimpleFileSystemProtocolGuid,
                (VOID **)&SimpleFileSystem
             );
    if (EFI_ERROR(Status)) {
        Print(L"Failed to handle SimpleFileSystem protocol: %r\n", Status);
        return Status;
    }

    // Open the root directory of the SD card
    Status = SimpleFileSystem->OpenVolume(SimpleFileSystem, &Root);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to open volume: %r\n", Status);
        return Status;
    }

    // Open the kernel file specified by KernelFileName
    Status = Root->Open(
                Root,
                &KernelFile,
                KernelFileName,
                EFI_FILE_MODE_READ,
                0
             );
    if (EFI_ERROR(Status)) {
        Print(L"Failed to open kernel file %s: %r\n", KernelFileName, Status);
        return Status;
    }

    // Get the file size
    EFI_FILE_INFO *FileInfo;
    UINTN FileInfoSize = sizeof(EFI_FILE_INFO);
    Status = KernelFile->GetInfo(
                KernelFile,
                &gEfiFileInfoGuid,
                &FileInfoSize,
                NULL
             );
    if (Status != EFI_BUFFER_TOO_SMALL) {
        Print(L"Failed to get file info size: %r\n", Status);
        return Status;
    }

    FileInfo = AllocatePool(FileInfoSize);
    if (FileInfo == NULL) {
        Print(L"Failed to allocate memory for file info\n");
        return EFI_OUT_OF_RESOURCES;
    }

    Status = KernelFile->GetInfo(
                KernelFile,
                &gEfiFileInfoGuid,
                &FileInfoSize,
                FileInfo
             );
    if (EFI_ERROR(Status)) {
        Print(L"Failed to get file info: %r\n", Status);
        FreePool(FileInfo);
        return Status;
    }

    // Allocate buffer for the kernel at the specified load address
    BufferSize = (UINTN)FileInfo->FileSize;
    Buffer = LoadAddress; // Use the specified LoadAddress
    if (Buffer == NULL) {
        Print(L"Failed to allocate memory at load address\n");
        FreePool(FileInfo);
        return EFI_OUT_OF_RESOURCES;
    }

    // Read the kernel file into the buffer
    Status = KernelFile->Read(
                KernelFile,
                &BufferSize,
                Buffer
             );
    if (EFI_ERROR(Status)) {
        Print(L"Failed to read kernel file: %r\n", Status);
        FreePool(FileInfo);
        return Status;
    }

    // Clean up and return the buffer
    FreePool(FileInfo);
    KernelFile->Close(KernelFile);

    *KernelBuffer = Buffer;
    *KernelSize = BufferSize;

    return EFI_SUCCESS;
}