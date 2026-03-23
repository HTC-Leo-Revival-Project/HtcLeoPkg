#define KERNEL_OFFSET 0x8000
#define TAGS_OFFSET 0x100
#define RAMDISK_OFFSET 0x01000000

void BootAndroidKernel(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
