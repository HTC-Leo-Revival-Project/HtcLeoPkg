#include "../menu.h"
#include "AndroidKernel.h"
#include <Library/FrameBufferConfigLib.h>
#include "LinuxShim.h"
#include "AndroidSDDir.h"
#include "LinuxHeader.h"
#include <Library/Atags.h>
#include <Chipset/KernelOffsets.h>

#define BASE_ADDR FixedPcdGet32(PcdSystemMemoryBase)

VOID *KernelLoadAddress = (VOID *)(BASE_ADDR + KERNEL_OFFSET);
VOID *RamdiskLoadAddress = (VOID *)(BASE_ADDR + RAMDISK_OFFSET);
UINTN *AtagsAddress = (unsigned *)(BASE_ADDR + TAGS_OFFSET);
UINTN MachType = FixedPcdGet32(PcdMachType);

unsigned* target_atag_mem(unsigned* ptr)
{
	//add atag to notify nand boot
	*ptr++ = 4;
	*ptr++ = 0x4C47414D; 	// NAND atag (MAGL :))
	*ptr++ = CLK_SIGNATURE_MAGIC; 	// cLK signature
	*ptr++ = 13;			// cLK version number

	return ptr;
}

VOID
CallExitBS(
    IN EFI_HANDLE ImageHandle, 
    IN EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;
    
    UINTN MemoryMapSize;
    EFI_MEMORY_DESCRIPTOR *MemoryMap;
    UINTN LocalMapKey;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;
    MemoryMap = NULL;
    MemoryMapSize = 0;
    
    /* Get the memory map */
    do {  
        Status = gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &LocalMapKey, &DescriptorSize,&DescriptorVersion);
        if (Status == EFI_BUFFER_TOO_SMALL){
            MemoryMap = AllocatePool(MemoryMapSize + 1);
            Status = gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &LocalMapKey, &DescriptorSize,&DescriptorVersion);      
        } else {
            /* Status is likely success - let the while() statement check success */
        }
    
    } while (Status != EFI_SUCCESS);

    DEBUG((EFI_D_INFO, "Exit BS\n"));
    gBS->ExitBootServices(ImageHandle, LocalMapKey);
}


VOID AsciiStrToUnicodeStr(CONST CHAR8 *Source, CHAR16 *Destination) {
    while (*Source != '\0') {
        *(Destination++) = (CHAR16)*(Source++);
    }
    *Destination = '\0'; // Null-terminate the Unicode string
}

/* Loads a raw image */
VOID
BootImage(
    IN EFI_HANDLE ImageHandle, 
    IN EFI_SYSTEM_TABLE *SystemTable,
    void *Kernel
)
{
	EFI_STATUS Status;
	void (*Entry)(unsigned,unsigned,unsigned*) = Kernel;

	DEBUG ((EFI_D_ERROR, "booting image @ %p\n", Kernel));

    CallExitBS(ImageHandle, SystemTable);

	//we are ready to boot the freshly loaded kernel
	PrepareForLinux();
	Entry(0, 0, 0);

    DEBUG((EFI_D_ERROR, "Failed to boot image, jump did not happen were still in uefiland \n\n"));
	//deadlock the platform, this is not recoverable
	for(;;);
}

/* Loads a linux image */
VOID
BootLinux(
    IN EFI_HANDLE ImageHandle, 
    IN EFI_SYSTEM_TABLE *SystemTable,
    void *kernel, 
    unsigned *tags, 
	const char *cmdline, 
    unsigned MachType,
	void *ramdisk, 
    unsigned ramdisk_size)
{
	EFI_STATUS Status;
	unsigned *ptr = tags;
	//unsigned pcount = 0;
	void (*entry)(unsigned,unsigned,unsigned*) = kernel;
	//struct ptable *ptable;
	int cmdline_len = 0;
	int have_cmdline = 0;

	/* CORE */
	*ptr++ = 2;
	*ptr++ = ATAG_CORE;

	if (ramdisk_size) {
		*ptr++ = 4;
		*ptr++ = ATAG_INITRD2;
		*ptr++ = (unsigned)ramdisk;
		*ptr++ = ramdisk_size;
	}

	ptr = target_atag_mem(ptr);
    /* ToDo: add the needed nand stuff ????*/
	// if (!target_is_emmc_boot()) {
	// 	/* Skip NAND partition ATAGS for eMMC boot */
	// 	if ((ptable = flash_get_ptable()) && (ptable->count != 0)) {
	// 		int i;
	// 		for(i=0; i < ptable->count; i++) {
	// 			struct ptentry *ptn;
	// 			ptn =  ptable_get(ptable, i);
	// 			if (ptn->type == TYPE_APPS_PARTITION)
	// 				pcount++;
	// 		}
	// 		*ptr++ = 2 + (pcount * (sizeof(struct atag_ptbl_entry) /
	// 					       sizeof(unsigned)));
	// 		*ptr++ = 0x4d534d70;
	// 		for (i = 0; i < ptable->count; ++i)
	// 			ptentry_to_tag(&ptr, ptable_get(ptable, i));
	// 	}
	// }

	if (cmdline && cmdline[0]) {
		cmdline_len = AsciiStrLen(cmdline);
		have_cmdline = 1;
	}
	if (cmdline_len > 0) {
		const char *src;
		char *dst;
		unsigned n;
		/* include terminating 0 and round up to a word multiple */
		n = (cmdline_len + 4) & (~3);
		*ptr++ = (n / 4) + 2;
		*ptr++ = ATAG_CMDLINE;
		dst = (char *)ptr;
		if (have_cmdline) {
			src = cmdline;
			while ((*dst++ = *src++));
		}
        /*J0SH1X: leo is never emmc boot, even tho sdsupport in clk would be cool*/
		// if (target_is_emmc_boot()) {
		// 	src = emmc_cmdline;
		// 	if (have_cmdline) --dst;
		// 	have_cmdline = 1;
		// 	while ((*dst++ = *src++));
		// }
        /*J0SH1X: we dont need clks charging since menu already handles charging itself*/
		// if (pause_at_bootup) {
		// 	src = battchg_pause;
		// 	if (have_cmdline) --dst;
		// 	while ((*dst++ = *src++));
		// }
		ptr += (n / 4);
	}

	/* END */
	*ptr++ = 0;
	*ptr++ = 0;
	DEBUG ((EFI_D_ERROR, "booting linux @ %p, ramdisk @ %p (%d)\n",kernel, ramdisk, ramdisk_size));
	    if (cmdline) {
        CHAR16 CmdlineUnicode[256];
        AsciiStrToUnicodeStr(cmdline, CmdlineUnicode);
        DEBUG ((EFI_D_ERROR, "cmdline: %s\n", CmdlineUnicode));
    }

    CallExitBS(ImageHandle, SystemTable);

	//we are ready to boot the freshly loaded kernel
	PrepareForLinux();
	entry(0, MachType, tags);

    DEBUG((EFI_D_ERROR, "Failed to boot Linux, jump did not happen were still in uefiland \n\n"));
	//deadlock the platform this is not recoverable
	for(;;);
}

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
        DEBUG ((EFI_D_ERROR, "Failed to locate handles for SimpleFileSystem: %r\n", Status));
        return Status;
    }

    // Assume the SD card is the first handle found
    Status = SystemTable->BootServices->HandleProtocol(
                Handles[0],
                &gEfiSimpleFileSystemProtocolGuid,
                (VOID **)&SimpleFileSystem
             );
    if (EFI_ERROR(Status)) {
        DEBUG ((EFI_D_ERROR, "Failed to handle SimpleFileSystem protocol: %r\n", Status));
        return Status;
    }

    // Open the root directory of the SD card
    Status = SimpleFileSystem->OpenVolume(SimpleFileSystem, &Root);
    if (EFI_ERROR(Status)) {
        DEBUG ((EFI_D_ERROR, "Failed to open volume: %r\n", Status));
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
        DEBUG ((EFI_D_ERROR, "Failed to open kernel file %s: %r\n", KernelFileName, Status));
        return Status;
    }

    EFI_FILE_INFO  SmallInfo;
    EFI_FILE_INFO  *FileInfo = &SmallInfo;
    UINTN          FileInfoSize = sizeof(SmallInfo);

    *KernelBuffer = NULL;
    *KernelSize = 0;

    Status = KernelFile->GetInfo(
                KernelFile,
                &gEfiFileInfoGuid,
                &FileInfoSize,
                FileInfo
             );
    if (Status == EFI_BUFFER_TOO_SMALL) {
        FileInfo = AllocatePool(FileInfoSize);
        if (FileInfo == NULL) {
            DEBUG ((EFI_D_ERROR, "Failed to allocate memory for file info\n"));
            return EFI_OUT_OF_RESOURCES;
        }

        Status = KernelFile->GetInfo(
                    KernelFile,
                    &gEfiFileInfoGuid,
                    &FileInfoSize,
                    FileInfo
                 );
    }
    if (EFI_ERROR(Status)) {
        DEBUG ((EFI_D_ERROR, "Failed to get file info: %r\n", Status));
        if (FileInfo != &SmallInfo) {
            FreePool(FileInfo);
        }
        return Status;
    }

    DEBUG ((
      EFI_D_ERROR,
      "LoadFileFromSDCard: len %u chars %04x %04x %04x %04x, size %lu, info size %lu\n",
      StrLen (KernelFileName),
      KernelFileName[0], KernelFileName[1], KernelFileName[2], KernelFileName[3],
      (UINT64) FileInfo->FileSize,
      (UINT64) FileInfoSize
      ));

    // Allocate buffer for the kernel at the specified load address
    BufferSize = (UINTN)FileInfo->FileSize;
    Buffer = LoadAddress; // Use the specified LoadAddress
    if (Buffer == NULL) {
        DEBUG ((EFI_D_ERROR, "Failed to allocate memory at load address\n"));
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
        DEBUG ((EFI_D_ERROR, "Failed to read kernel file: %r\n", Status));
        FreePool(FileInfo);
        return Status;
    }

    // Clean up and return the buffer
    if (FileInfo != &SmallInfo) {
        FreePool(FileInfo);
    }
    KernelFile->Close(KernelFile);

    *KernelBuffer = Buffer;
    *KernelSize = BufferSize;

    return EFI_SUCCESS;
}

EFI_STATUS
OpenFileOnSdCard (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable,
  IN  CHAR16            *FileName,
  OUT EFI_FILE_PROTOCOL  **File,
  OUT EFI_FILE_PROTOCOL  **Root
  )
{
    EFI_STATUS                     Status;
    EFI_HANDLE                     *Handles;
    UINTN                          HandleCount;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SimpleFileSystem;

    *File  = NULL;
    *Root  = NULL;

    Status = SystemTable->BootServices->LocateHandleBuffer (
                        ByProtocol,
                        &gEfiSimpleFileSystemProtocolGuid,
                        NULL,
                        &HandleCount,
                        &Handles
                        );
    if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "No simple file system: %r\n", Status));
        return Status;
    }

    Status = SystemTable->BootServices->HandleProtocol (
                        Handles[0],
                        &gEfiSimpleFileSystemProtocolGuid,
                        (VOID **) &SimpleFileSystem
                        );
    if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "No file system protocol: %r\n", Status));
        FreePool (Handles);
        return Status;
    }

    Status = SimpleFileSystem->OpenVolume (SimpleFileSystem, Root);
    if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "Cannot open the card volume: %r\n", Status));
        FreePool (Handles);
        return Status;
    }

    Status = (*Root)->Open (*Root, File, FileName, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "Cannot open %s: %r\n", FileName, Status));
        (*Root)->Close (*Root);
        *Root = NULL;
    }

    FreePool (Handles);

    return Status;
}

EFI_STATUS
ReadFileRange (
  IN  EFI_FILE_PROTOCOL  *File,
  IN  UINT64             Offset,
  IN  UINTN              Size,
  OUT VOID               *Dest
  )
{
    EFI_STATUS  Status;
    UINTN       Read = Size;

    Status = File->SetPosition (File, Offset);
    if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_ERROR, "Cannot seek to %lu: %r\n", Offset, Status));
        return Status;
    }

    Status = File->Read (File, &Read, Dest);
    if (EFI_ERROR (Status) || Read != Size) {
        DEBUG ((
            EFI_D_ERROR,
            "Cannot read %lu bytes at %lu: %r (read %lu)\n",
            Size, Offset, Status, Read
            ));
        return EFI_DEVICE_ERROR;
    }

    return EFI_SUCCESS;
}

EFI_STATUS
BootAndroidBootImage (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE  *SystemTable,
    IN CHAR16            *FileName
    )
{
    EFI_STATUS           Status;
    EFI_FILE_PROTOCOL    *File;
    EFI_FILE_PROTOCOL    *Root;
    BOOT_IMAGE_HEADER    HeaderData;
    BOOT_IMAGE_HEADER    *Header = &HeaderData;
    UINTN                PageSize;
    UINTN                Offset;
    CHAR8                Cmdline[1600];
    UINTN                CmdlineLen;
    VOID                 *Kernel;
    VOID                 *Ramdisk;

    Status = OpenFileOnSdCard (ImageHandle, SystemTable, FileName, &File, &Root);
    if (EFI_ERROR (Status)) {
        return EFI_NOT_FOUND;
    }

    Status = ReadFileRange (File, 0, sizeof (BOOT_IMAGE_HEADER), Header);
    if (EFI_ERROR (Status)) {
        File->Close (File);
        Root->Close (Root);
        return Status;
    }

    if ((Header->Magic0 != BOOT_MAGIC_0) || (Header->Magic1 != BOOT_MAGIC_1)) {
        DEBUG ((
            EFI_D_ERROR,
            "%s is not a boot image (magic %08x %08x)\n",
            FileName, Header->Magic0, Header->Magic1
            ));
        File->Close (File);
        Root->Close (Root);
        return EFI_NOT_FOUND;
    }

    DEBUG ((
        EFI_D_ERROR,
        "Boot image %s: kernel %u bytes at 0x%08x, ramdisk %u bytes at 0x%08x, tags 0x%08x\n",
        FileName, Header->KernelSize, Header->KernelAddr,
        Header->RamdiskSize, Header->RamdiskAddr, Header->TagsAddr
        ));

    PageSize = (Header->PageSize != 0) ? Header->PageSize : 2048;
    Kernel   = (VOID *) (UINTN) Header->KernelAddr;
    Ramdisk  = (VOID *) (UINTN) Header->RamdiskAddr;

    //
    // Kernel first, then the ramdisk: both are page aligned in the file.
    //
    Offset = (sizeof (BOOT_IMAGE_HEADER) + PageSize - 1) & ~(PageSize - 1);
    Status = ReadFileRange (File, Offset, Header->KernelSize, Kernel);
    if (EFI_ERROR (Status)) {
        File->Close (File);
        Root->Close (Root);
        return Status;
    }

    Offset = (Offset + Header->KernelSize + PageSize - 1) & ~(PageSize - 1);
    Status = ReadFileRange (File, Offset, Header->RamdiskSize, Ramdisk);
    if (EFI_ERROR (Status)) {
        File->Close (File);
        Root->Close (Root);
        return Status;
    }

    File->Close (File);
    Root->Close (Root);

    Header->Cmdline[sizeof (Header->Cmdline) - 1] = 0;
    if (Header->HeaderVersion > 0) {
        Header->ExtraCmdline[sizeof (Header->ExtraCmdline) - 1] = 0;
        CmdlineLen = AsciiStrLen (Header->Cmdline);
        if ((CmdlineLen > 0) && (Header->Cmdline[CmdlineLen - 1] != ' ')) {
            Header->Cmdline[CmdlineLen++] = ' ';
            Header->Cmdline[CmdlineLen]   = 0;
        }
        AsciiStrCatS (Cmdline, sizeof (Cmdline), Header->Cmdline);
        AsciiStrCatS (Cmdline, sizeof (Cmdline), Header->ExtraCmdline);
    } else {
        AsciiStrCpyS (Cmdline, sizeof (Cmdline), Header->Cmdline);
    }

    DEBUG ((
        EFI_D_ERROR,
        "Boot image command line: %a\n",
        Cmdline
        ));

    BootLinux (
        ImageHandle,
        SystemTable,
        Kernel,
        (UINTN *) (UINTN) Header->TagsAddr,
        Cmdline,
        MachType,
        Ramdisk,
        Header->RamdiskSize
        );

    return EFI_SUCCESS;
}

static
VOID
BuildSdPath (
  OUT CHAR16  *Buffer,
  IN  UINTN   BufferSize,
  IN  CHAR16  *FileName
  )
{
  CHAR16  Dir[128];
  UINTN   Len = 0;

  if (!FallBack && SelectedDir != NULL) {
    while (SelectedDir[Len] != L'\0' && Len < ARRAY_SIZE (Dir) - 1) {
      Dir[Len] = SelectedDir[Len];
      Len++;
    }
  }
  Dir[Len] = L'\0';

  while (Len > 0 && (Dir[Len - 1] == L'\\' || Dir[Len - 1] == L'/')) {
    Dir[--Len] = L'\0';
  }

  if (Len == 0) {
    UnicodeSPrint (Buffer, BufferSize, L"\\%s", FileName);
  } else {
    UnicodeSPrint (Buffer, BufferSize, L"\\%s\\%s", Dir, FileName);
  }

  DEBUG ((EFI_D_ERROR, "BuildSdPath: %s\n", Buffer));
}

void BootAndroidKernel(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS Status;
    VOID *KernelBuffer;
    UINTN KernelSize;
    VOID *RamdiskBuffer;
    UINTN RamdiskSize;
    CHAR16 KernelPath[256];
    CHAR16 RamdiskPath[256];

    const char *cmdline = "";// = "androidboot.hardware=htcleo androidboot.selinux=permissive androidboot.configfs=false";
    
    CHAR8 *AllocatedCmdline = NULL;
    UINTN CmdlineSize;

    Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    ASSERT_EFI_ERROR(Status);

    BuildSdPath (KernelPath, sizeof (KernelPath), L"boot.img");
    Status = BootAndroidBootImage (ImageHandle, SystemTable, KernelPath);
    if (Status != EFI_NOT_FOUND) {
        return;
    }

    BuildSdPath (KernelPath, sizeof (KernelPath), L"zImage");
    BuildSdPath (RamdiskPath, sizeof (RamdiskPath), L"initrd.img");
    // Print the path that was generated
    DEBUG((EFI_D_INFO, "KernelDir is %s\n", KernelPath, Status));

    Status = LoadFileFromSDCard(ImageHandle, SystemTable, KernelPath, KernelLoadAddress, &KernelBuffer, &KernelSize);
    if (EFI_ERROR(Status)) {
        DEBUG ((EFI_D_ERROR, "Failed to load kernel from path %s: %r\n", KernelPath, Status));
    } else {
        DEBUG ((EFI_D_ERROR, "Kernel loaded successfully from path %s at address %p. Size: %d bytes\n", KernelPath, KernelBuffer, KernelSize));
        
        /* Check if the image is a linux kernel 
         * source: https://github.com/ARM-software/u-boot/blob/master/arch/arm/lib/zimage.c
         */
        struct arm_z_header *zi = (struct arm_z_header *)KernelBuffer;
        if (zi->zi_magic != LINUX_ARM_ZIMAGE_MAGIC) {
            DEBUG((EFI_D_INFO, "Bad Linux ARM zImage magic, booting only the image\n"));

            // Reconfigure the FB back to RGB565
            ReconfigFb(RGB565_BPP);
            
            // Boot the image
            BootImage(ImageHandle, SystemTable, KernelBuffer);
        }

        Status = LoadFileFromSDCard(ImageHandle, SystemTable, RamdiskPath, 
                                RamdiskLoadAddress, &RamdiskBuffer, &RamdiskSize);
        if (EFI_ERROR(Status)) {
            DEBUG ((EFI_D_ERROR, "Failed to load ramdisk from path %s: %r\n", RamdiskPath, Status));
        } else {
            DEBUG ((EFI_D_ERROR, "Ramdisk loaded successfully from path %s at address %p. Size: %d bytes\n", RamdiskPath, RamdiskLoadAddress, RamdiskSize));
        }
        // Allocate memory for cmdline
        CmdlineSize = AsciiStrLen(cmdline) + 1; // +1 for the null terminator
        Status = SystemTable->BootServices->AllocatePool(EfiLoaderData, CmdlineSize, (void **)&AllocatedCmdline);
        if (EFI_ERROR(Status)) {
            DEBUG ((EFI_D_ERROR, "Failed to allocate memory for cmdline: %r\n", Status));
        } else {
            // Copy cmdline to allocated memory
            AsciiStrCpyS(AllocatedCmdline, CmdlineSize, cmdline);

            // Reconfigure the FB back to RGB565
            ReconfigFb(RGB565_BPP);
            
            // Boot Linux with the allocated cmdline
            BootLinux(ImageHandle, SystemTable, KernelBuffer, AtagsAddress, 
                    AllocatedCmdline, MachType, RamdiskBuffer, RamdiskSize);
            // Clean up allocated cmdline
            SystemTable->BootServices->FreePool(AllocatedCmdline);
        }
        
        // Clean up the kernel buffer when no longer needed, but we should never reach here essentially halt the platform
        FreePool(KernelBuffer);
        DEBUG ((EFI_D_ERROR, "Booting Linux Failed, unknown error occurred"));
        for (;;) ;
    }
}



