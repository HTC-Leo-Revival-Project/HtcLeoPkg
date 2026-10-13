#define BOOT_MAGIC_0   0x52444E41
#define BOOT_MAGIC_1   0x2144494F

typedef struct {
  UINT32  Magic0;
  UINT32  Magic1;
  UINT32  KernelSize;
  UINT32  KernelAddr;
  UINT32  RamdiskSize;
  UINT32  RamdiskAddr;
  UINT32  SecondSize;
  UINT32  SecondAddr;
  UINT32  TagsAddr;
  UINT32  PageSize;
  UINT32  HeaderVersion;
  UINT32  OsVersion;
  CHAR8   Name[16];
  CHAR8   Cmdline[512];
  UINT32  Id[8];
  CHAR8   ExtraCmdline[1024];
} BOOT_IMAGE_HEADER;

void BootAndroidKernel(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);
