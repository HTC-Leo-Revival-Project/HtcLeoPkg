#include "../menu.h"
#include "AndroidKernel.h"
#include <Library/FrameBufferConfigLib.h>
#include "LinuxShim.h"
#include "AndroidSDDir.h"
#include "SdLoader.h"

unsigned* target_atag_mem(unsigned* ptr)
{

#if 0
	//MEM TAG
	*ptr++ = 4;
	*ptr++ = 0x54410002;
	//*ptr++ = 0x1e400000; //mem size from haret
	//*ptr++ = 0x1E7C0000; //mem size from kernel config
	*ptr++ = 0x1CFC0000; //mem size from kernel config with bravo dsp
	*ptr++ = 0x11800000; //mem base
#endif

	//add atag to notify nand boot
	*ptr++ = 4;
	*ptr++ = 0x4C47414D; 	// NAND atag (MAGL :))
	*ptr++ = 0x004b4c63; 	// cLK signature
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
    /* Get the memory map */
    UINTN MemoryMapSize;
    EFI_MEMORY_DESCRIPTOR *MemoryMap;
    UINTN LocalMapKey;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;
    MemoryMap = NULL;
    MemoryMapSize = 0;
    
	
    do {  
        Status = gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &LocalMapKey, &DescriptorSize,&DescriptorVersion);
        if (Status == EFI_BUFFER_TOO_SMALL){
            MemoryMap = AllocatePool(MemoryMapSize + 1);
            Status = gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &LocalMapKey, &DescriptorSize,&DescriptorVersion);      
        } else {
            /* Status is likely success - let the while() statement check success */
        }
        DEBUG((EFI_D_ERROR, "Memory loop iteration, status: %r\n", Status));
    
    } while (Status != EFI_SUCCESS);

    DEBUG((EFI_D_ERROR, "Exit BS\n"));
    gBS->ExitBootServices(ImageHandle, LocalMapKey);
}


VOID AsciiStrToUnicodeStr(CONST CHAR8 *Source, CHAR16 *Destination) {
    while (*Source != '\0') {
        *(Destination++) = (CHAR16)*(Source++);
    }
    *Destination = '\0'; // Null-terminate the Unicode string
}

void boot_linux(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable,void *kernel, unsigned *tags, 
		const char *cmdline, unsigned machtype,
		void *ramdisk, unsigned ramdisk_size)
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
	*ptr++ = 0x54410001;

	if (ramdisk_size) {
		*ptr++ = 4;
		*ptr++ = 0x54420005;
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
		*ptr++ = 0x54410009;
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
	Print(L"booting linux @ %p, ramdisk @ %p (%d)\n",kernel, ramdisk, ramdisk_size);
	    if (cmdline) {
        CHAR16 CmdlineUnicode[256];
        AsciiStrToUnicodeStr(cmdline, CmdlineUnicode);
        Print(L"cmdline: %s\n", CmdlineUnicode);
    }

    CallExitBS(ImageHandle, SystemTable);

	//we are ready to boot the freshly loaded kernel
	//DEBUG((EFI_D_INFO, "Preparing... \n"));
	PrepareForLinux();
	//DEBUG((EFI_D_INFO, "Jumping to kernel\n"));
	entry(0, machtype, tags);

    DEBUG((EFI_D_ERROR, "Failed to boot Linux, jump did not happen were still in uefiland \n\n"));
	//deadlock the platform this is not recoverable
	for(;;);
}

void BootAndroidKernel(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS Status;
    VOID *KernelBuffer;
    UINTN KernelSize;
    VOID *RamdiskBuffer;
    UINTN RamdiskSize;
    CHAR16 KernelPath[256];
    CHAR16 RamdiskPath[256];

    UINTN BaseAddr = FixedPcdGet32(PcdSystemMemoryBase);
    VOID *KernelLoadAddress = (VOID *)(BaseAddr + KERNEL_OFFSET);
    VOID *RamdiskLoadAddress = (VOID *)(BaseAddr + RAMDISK_OFFSET);
    unsigned *tags_address = (unsigned *)(BaseAddr + TAGS_OFFSET);
    const char *cmdline = "androidboot.hardware=htcleo androidboot.selinux=permissive androidboot.configfs=false";
    unsigned machtype = 0x9dc;
    
    CHAR8 *AllocatedCmdline = NULL;
    UINTN CmdlineSize;

    Status = SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    ASSERT_EFI_ERROR(Status);

    if (!FallBack) {
        // Build the paths based on SelectedDir
        UnicodeSPrint(KernelPath, sizeof(KernelPath), L"\\%s\\zImage", SelectedDir);
        UnicodeSPrint(RamdiskPath, sizeof(RamdiskPath), L"\\%s\\initrd.img", SelectedDir);
    } else {
        // Use default paths
        StrCpyS(KernelPath, sizeof(KernelPath) / sizeof(CHAR16), L"\\zImage");
        StrCpyS(RamdiskPath, sizeof(RamdiskPath) / sizeof(CHAR16), L"\\initrd.img");
    }
    //print the path that was generated
    Print(L"KernelDir is %s\n", KernelPath, Status);
    Status = LoadFileFromSDCard(ImageHandle, SystemTable, KernelPath, KernelLoadAddress, &KernelBuffer, &KernelSize);
    if (EFI_ERROR(Status)) {
        Print(L"Failed to load kernel from path %s: %r\n", KernelPath, Status);
    } else {
        Print(L"Kernel loaded successfully from path %s at address %p. Size: %d bytes\n", KernelPath, KernelBuffer, KernelSize);
        
        Status = LoadFileFromSDCard(ImageHandle, SystemTable, RamdiskPath, RamdiskLoadAddress, &RamdiskBuffer, &RamdiskSize);
        if (EFI_ERROR(Status)) {
            Print(L"Failed to load ramdisk from path %s: %r\n", RamdiskPath, Status);
        } else {
            Print(L"Ramdisk loaded successfully from path %s at address %p. Size: %d bytes\n", RamdiskPath, RamdiskLoadAddress, RamdiskSize);
        }
        // Allocate memory for cmdline
        CmdlineSize = AsciiStrLen(cmdline) + 1; // +1 for the null terminator
        Status = SystemTable->BootServices->AllocatePool(EfiLoaderData, CmdlineSize, (void **)&AllocatedCmdline);
        if (EFI_ERROR(Status)) {
            Print(L"Failed to allocate memory for cmdline: %r\n", Status);
        } else {
            // Copy cmdline to allocated memory
            AsciiStrCpyS(AllocatedCmdline, CmdlineSize, cmdline);

            // Reconfigure the FB back to RGB565
            ReconfigFb(RGB565_BPP);
            
            // Boot Linux with the allocated cmdline
            boot_linux(ImageHandle, SystemTable, KernelBuffer, tags_address, AllocatedCmdline, machtype, RamdiskBuffer, RamdiskSize);
            
            // Clean up allocated cmdline
            SystemTable->BootServices->FreePool(AllocatedCmdline);
        }
        
        // Clean up the kernel buffer when no longer needed, but we should never reach here essentially halt the platform
        FreePool(KernelBuffer);
        Print(L"Booting Linux Failed, unknown error occurred");
        for (;;) ;
    }
}



