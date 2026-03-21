#ifndef _SD_LOADER_H_
#define __SD_LOADER_H_


EFI_STATUS EFIAPI LoadFileFromSDCard(IN EFI_HANDLE ImageHandle,IN EFI_SYSTEM_TABLE *SystemTable,IN CHAR16 *KernelFileName,IN VOID *LoadAddress,OUT VOID **KernelBuffer,OUT UINTN *KernelSize);
#endif