/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
 * Copyright (c) 2011-2012, ARM Limited. All rights reserved.
 * Copyright (c) 2015, The EFIDroid Project. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the 
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <Chipset/KernelOffsets.h>
#include <Uefi.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/LKEnvLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PlatformResetLib.h>
#include <Library/udc.h>
#include <Library/hsusb.h> // must come after LKEnvLib for now
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include "Bootimg.h"
#include "Fastboot.h"

#define BASE_ADDR FixedPcdGet32(PcdSystemMemoryBase)

VOID *KernelLoadAddress = (VOID *)(BASE_ADDR + KERNEL_OFFSET);
VOID *RamdiskLoadAddress = (VOID *)(BASE_ADDR + RAMDISK_OFFSET);
UINTN *AtagsAddress = (UINTN *)(BASE_ADDR + TAGS_OFFSET);
UINTN MachType = FixedPcdGet32(PcdMachType);

STATIC EFI_EVENT mExitBootServicesEvent;
STATIC EFI_EVENT mUsbOnlineEvent = NULL;
STATIC EFI_EVENT txn_done = NULL;

STATIC struct udc_endpoint *in, *out;
STATIC struct udc_endpoint *fastboot_endpoints[2];
STATIC struct udc_request *req;
UINTN txn_status;

STATIC VOID *DownloadBase = NULL;
STATIC UINT32 DownloadSize = 0;
STATIC UINTN DownloadPages = 0;

STATIC UINTN mFastbootState = STATE_OFFLINE;

struct fastboot_cmd {
	struct fastboot_cmd *next;
	const CHAR8 *prefix;
	UINTN prefix_len;
	VOID (*handle)(const CHAR8 *arg, VOID *data, UINTN sz);
};

struct fastboot_var {
	struct fastboot_var *next;
	const CHAR8 *name;
	const CHAR8 *value;
};
	
STATIC struct fastboot_cmd *CommandList;
STATIC struct fastboot_var *VariableList;

// USB read/write
STATIC VOID req_complete(struct udc_request *req, UINTN actual, UINTN status)
{
	txn_status = status;
	req->length = actual;

	gBS->SignalEvent (txn_done);
}

STATIC UINTN usb_read(VOID *_buf, UINTN len)
{
	UINTN r;
	UINTN xfer;
	CHAR8 *buf = _buf;
	UINTN count = 0;
    UINTN EventIndex;

	if (mFastbootState == STATE_ERROR)
		goto oops;

	while (len > 0) {
		xfer = (len > 4096) ? 4096 : len;
		req->buf = buf;
		req->length = xfer;
		req->complete = req_complete;
		r = udc_request_queue(out, req);
		if (r < 0) {
			DEBUG((EFI_D_ERROR, "usb_read() queue failed\n"));
			goto oops;
		}
        gBS->WaitForEvent (1, &txn_done, &EventIndex);

		if (txn_status < 0) {
			DEBUG((EFI_D_ERROR, "usb_read() transaction failed\n"));
			goto oops;
		}

		count += req->length;
		buf += req->length;
		len -= req->length;

		/* short transfer? */
		if (req->length != xfer) break;
	}

	return count;

oops:
	mFastbootState = STATE_ERROR;
	return -1;
}

STATIC UINTN usb_write(VOID *buf, UINTN len)
{
	UINTN r;
    UINTN EventIndex;

	if (mFastbootState == STATE_ERROR)
		goto oops;

	req->buf = buf;
	req->length = len;
	req->complete = req_complete;
	r = udc_request_queue(in, req);
	if (r < 0) {
		DEBUG((EFI_D_ERROR, "usb_write() queue failed\n"));
		goto oops;
	}
    gBS->WaitForEvent (1, &txn_done, &EventIndex);
	if (txn_status < 0) {
		DEBUG((EFI_D_ERROR, "usb_write() transaction failed\n"));
		goto oops;
	}
	return req->length;

oops:
	mFastbootState = STATE_ERROR;
	return -1;
}

STATIC VOID 
FastbootNotify(
	struct udc_gadget *Gadget, 
	UINTN Event)
{
	if (Event == UDC_EVENT_ONLINE) {
        gBS->SignalEvent (mUsbOnlineEvent);
	}
}

STATIC struct udc_device surf_udc_device = {
	.vendor_id	= 0x18d1,
	.product_id	= 0x0D02,
	.version_id	= 0x0001,
	.manufacturer	= "HTC",
	.product	= "LEO EDK2",
};

STATIC struct udc_gadget fastboot_gadget = {
	.notify		= FastbootNotify,
	.ifc_class	= 0xff,
	.ifc_subclass	= 0x42,
	.ifc_protocol	= 0x03,
	.ifc_endpoints	= 2,
	.ifc_string	= "fastboot",
	.ept		= fastboot_endpoints,
};

STATIC UINTN hex2unsigned(const CHAR8 *x)
{
    UINTN n = 0;

    for (; *x; x++) {
        n <<= 4;

        if (*x >= '0' && *x <= '9') {
            n |= (*x - '0');
        } else if (*x >= 'a' && *x <= 'f') {
            n |= (*x - 'a' + 10);
        } else if (*x >= 'A' && *x <= 'F') {
            n |= (*x - 'A' + 10);
        } else {
            n >>= 4;
		return n;
        }

    }

    return n;
}
VOID 
FastbootAck(
	const CHAR8 *Code, 
	const CHAR8 *Reason
	//BOOLEAN ChangeState
)
{
	STACKBUF_DMA_ALIGN(Response, FASTBOOT_COMMAND_MAX_LENGTH);

	if (mFastbootState != STATE_COMMAND)
		return;

	if (Reason == 0)
		Reason = "";

	AsciiSPrint((CHAR8*)Response, FASTBOOT_COMMAND_MAX_LENGTH, "%a%a", Code, Reason);

	if( usb_write(Response, strlen((CHAR8*)Response)) < 0 ) {
		mFastbootState = STATE_ERROR;
	//else if(ChangeState)
	}else {
		mFastbootState = STATE_COMPLETE;
	}
}

VOID 
FastbootFail(
	const CHAR8 *Reason
)
{
	FastbootAck("FAIL", Reason);
}

VOID 
FastbootOkay(
	const CHAR8 *Info
)
{
	FastbootAck("OKAY", Info);
}

VOID 
FastbootRegister(
	const CHAR8 *Prefix,
	VOID (*Handle)(const CHAR8 *Arg, VOID *Data, UINTN Size))
{
	struct fastboot_cmd *Command;

	Command = AllocatePool(sizeof(*Command));
	if (Command) {
		Command->prefix = Prefix;
		Command->prefix_len = strlen(Prefix);
		Command->handle = Handle;
		Command->next = CommandList;
		CommandList = Command;
	}
}

VOID 
FastbootPublish(
	const CHAR8 *Name, 
	const CHAR8 *Value
)
{
	struct fastboot_var *Variable;

	Variable = AllocatePool(sizeof(*Variable));
	if (Variable) {
		Variable->name = Name;
		Variable->value = Value;
		Variable->next = VariableList;
		VariableList = Variable;
	}
}

STATIC VOID 
CommandDownload(
	const CHAR8 *Arg, 
	VOID *Data, 
	UINTN size
)
{
	CHAR8 Response[FASTBOOT_COMMAND_MAX_LENGTH];
	UINTN Length = hex2unsigned(Arg);;
	UINTN r;

	// free old data
  	if(DownloadBase) {
		FreeAlignedPages(DownloadBase, DownloadPages);
		DownloadBase = NULL;
		DownloadSize = 0;
		DownloadPages = 0;
  	}

	// allocate data buffer
	DownloadSize = 0;
	DownloadPages = ROUNDUP(Length, EFI_PAGE_SIZE)/EFI_PAGE_SIZE;
	DownloadBase = AllocateAlignedPages(DownloadPages, EFI_PAGE_SIZE);
	if(DownloadBase == NULL) {
		FastbootFail("data too large");
		return;
	}

	// write response
	AsciiSPrint(Response, FASTBOOT_COMMAND_MAX_LENGTH, "DATA%08x", Length);
	if(usb_write(Response, AsciiStrLen(Response))<0) {
		mFastbootState = STATE_ERROR;
		return;
	}

	// Discard the cache contents before starting the download
  	InvalidateDataCacheRange(DownloadBase, Length);

	r = usb_read(DownloadBase, Length);
	if ((r < 0) || ((UINT32)r != Length)) {
		mFastbootState = STATE_ERROR;
		return;
	}

	// set size and send OKAY
  	DownloadSize = Length;
  	FastbootOkay("");
}

STATIC VOID 
FastbootCommandLoop(
	VOID
)
{
	struct fastboot_cmd *Command;
	UINTN r;
	DEBUG((EFI_D_ERROR,"fastboot: processing commands\n"));

	UINT8* Buffer = AllocateAlignedPages(ArmDataCacheLineLength(), ROUNDUP(4096, ArmDataCacheLineLength()));
    ASSERT(Buffer);

again:
	while (mFastbootState != STATE_ERROR) {
		SetMem(Buffer, FASTBOOT_COMMAND_MAX_LENGTH, 0);
		InvalidateDataCacheRange(Buffer, FASTBOOT_COMMAND_MAX_LENGTH);

		r = usb_read(Buffer, FASTBOOT_COMMAND_MAX_LENGTH);
		if (r < 0)
			break;

		Buffer[r] = 0;

		mFastbootState = STATE_COMMAND;

		for (Command = CommandList; Command; Command = Command->next) {
			UINTN CmdLen = AsciiStrLen((CHAR8*)Buffer);

			if (CompareMem(Buffer, Command->prefix, Command->prefix_len))
				continue;

			CHAR8* Arg = (CHAR8*) Buffer + Command->prefix_len;
			if(Arg[0]==' ')
				Arg++;
			
			Command->handle(Arg, DownloadBase, DownloadSize);
			if (mFastbootState == STATE_STOP)
        		goto stop;
			if (mFastbootState == STATE_COMMAND)
				FastbootFail("unknown reason");
			goto again;
		}

		FastbootFail("unknown command");
			
	}

stop:
	if (mFastbootState != STATE_STOP && mFastbootState != STATE_STOPPED) {
		mFastbootState = STATE_OFFLINE;
		DEBUG((EFI_D_ERROR, "fastboot: oops!\n"));
	}
	FreeAlignedPages(Buffer, 1);

	if (DownloadBase!=NULL) {
		FreeAlignedPages(DownloadBase, DownloadPages);
		DownloadBase = NULL;
		DownloadSize = 0;
		DownloadPages = 0;
	}
}

STATIC VOID 
FastbootHandler(
	VOID *arg
)
{
	for (;;) {
        UINTN EventIndex;

        gBS->WaitForEvent (1, &mUsbOnlineEvent, &EventIndex);
		FastbootCommandLoop();
		if (mFastbootState==STATE_STOP || mFastbootState==STATE_STOPPED)
      		break;
	}
}

STATIC
VOID
EFIAPI
ExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{ 
	udc_stop();
}

STATIC VOID 
CommandGetVar(
	const CHAR8 *arg, 
	VOID *data, 
	UINTN sz
)
{
	struct fastboot_var *var;

	for (var = VariableList; var; var = var->next) {
		if (!strcmp(var->name, arg)) {
			FastbootOkay(var->value);
			return;
		}
	}
	FastbootOkay("");
}


STATIC
VOID
CommandFlashEsp(
    const CHAR8 *Arg,
    VOID *Data,
    UINTN Size
)
{
    EFI_STATUS Status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;
    EFI_FILE_PROTOCOL *Root;
    EFI_FILE_PROTOCOL *File;
    EFI_HANDLE *HandleBuffer;
    UINTN HandleCount;
    CHAR16 FileName[256];
	CHAR16 FullPath[300];

	if (Arg == NULL || Arg[0] == '\0') {
    FastbootFail("no filename");
    return;
	}

    if (DownloadBase == NULL || DownloadSize == 0) {
        FastbootFail("no image");
        return;
    }

	AsciiStrToUnicodeStrS(Arg,FileName,sizeof(FileName) / sizeof(CHAR16));

	UnicodeSPrint(FullPath, sizeof(FullPath), L"\\%s", FileName);

    Status = gBS->LocateHandleBuffer(
        ByProtocol,
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        &HandleCount,
        &HandleBuffer
    );

    if (EFI_ERROR(Status) || HandleCount == 0) {
        FastbootFail("no fs");
        return;
    }

    Status = gBS->HandleProtocol(
        HandleBuffer[0],
        &gEfiSimpleFileSystemProtocolGuid,
        (VOID**)&Fs
    );

    if (EFI_ERROR(Status)) {
        FastbootFail("fs error");
        return;
    }

    Status = Fs->OpenVolume(Fs, &Root);
    if (EFI_ERROR(Status)) {
        FastbootFail("openvol fail");
        return;
    }

    Status = Root->Open(
        Root,
        &File,
        FullPath,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
        0
    );

    if (!EFI_ERROR(Status)) {
        File->Delete(File);
    }

    Status = Root->Open(
        Root,
        &File,
        FullPath,
        EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
        0
    );

    if (EFI_ERROR(Status)) {
        FastbootFail("create fail");
        return;
    }

    UINTN WriteSize = DownloadSize;

    Status = File->Write(File, &WriteSize, DownloadBase);
    if (EFI_ERROR(Status) || WriteSize != DownloadSize) {
        File->Close(File);
        FastbootFail("write fail");
        return;
    }

    File->Close(File);

    FastbootOkay("");
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



VOID 
CommandBoot(
    const CHAR8 *arg, 
    VOID *data, 
    UINTN sz) 
{
    DEBUG((EFI_D_ERROR, "BOOT CALLED, size=%u\n", sz));

    // Check magic
    if (CompareMem(data, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0) {
        DEBUG((EFI_D_ERROR, "Not an Android boot image!\n"));
        return;
    }

    ANDROID_BOOT_IMG_HDR *hdr = (ANDROID_BOOT_IMG_HDR *)data;
	DEBUG((EFI_D_ERROR, "Boot Image Header:\n"));
    DEBUG((EFI_D_ERROR, "Kernel size: %u\n", hdr->kernel_size));
    DEBUG((EFI_D_ERROR, "Ramdisk size: %u\n", hdr->ramdisk_size));
    DEBUG((EFI_D_ERROR, "Kernel addr: 0x%x\n", hdr->kernel_addr));
    DEBUG((EFI_D_ERROR, "Ramdisk addr: 0x%x\n", hdr->ramdisk_addr));
    DEBUG((EFI_D_ERROR, "Tags addr: 0x%x\n", hdr->tags_addr));
    DEBUG((EFI_D_ERROR, "Page size: %u\n", hdr->page_size));
    DEBUG((EFI_D_ERROR, "Cmdline: %a\n", hdr->cmdline));

    UINTN page_size = hdr->page_size;

    UINTN kernel_offset = page_size;
    UINTN kernel_size   = hdr->kernel_size;

    UINT8 *buf = (UINT8 *)data;
    UINT8 *kernel_src = buf + kernel_offset;

    UINT8 *kernel_dst = (UINT8 *)KernelLoadAddress;

    DEBUG((EFI_D_ERROR, "Copying kernel to 0x%p\n", kernel_dst));

    CopyMem(kernel_dst, kernel_src, kernel_size);

    // Optional: dump first bytes of kernel
    DEBUG((EFI_D_ERROR, "First 25 bytes of loaded kernel:\n"));
    for (UINTN i = 0; i < 25 && i < kernel_size; i++) {
        DEBUG((EFI_D_ERROR, "%02x ", kernel_dst[i]));
    }
    DEBUG((EFI_D_ERROR, "\n"));

	// Ack before jumping
	FastbootOkay("");

    VOID (*Entry)(UINTN, UINTN, UINTN*) =
        (VOID (*)(UINTN, UINTN, UINTN*))kernel_dst;

	CallExitBS(gImageHandle, gST);

    DEBUG((EFI_D_ERROR, "Jumping to kernel at 0x%p\n", Entry));

    // Jump (ARM Linux convention)
    Entry(0, 0, (UINTN*)hdr->tags_addr);
}

VOID 
CommandReboot(
	const CHAR8 *arg, 
	VOID *data, 
	UINTN sz) 
{
	FastbootOkay("");
    ResetCold();
}

VOID CommandOEMHandler(const CHAR8 *arg, VOID* data, UINTN size){
    if (arg == NULL || arg[0] == '\0') {
        FastbootFail("no oem command");
        return;
    }

    // --- exit ---
    if (!AsciiStrCmp(arg, "exit")) {
        FastbootOkay("bye");

        mFastbootState = STATE_STOP;
		udc_stop();
    	gBS->Exit(gImageHandle, EFI_SUCCESS, 0, NULL);
    }

    // --- unknown ---
    FastbootFail("unknown oem cmd");
}

EFI_STATUS 
EFIAPI
StartFastboot(
	IN EFI_HANDLE ImageHandle, 
	IN EFI_SYSTEM_TABLE *SystemTable)
{
	INT32 r;

	EFI_STATUS Status = gBS->CreateEvent (0, TPL_CALLBACK, NULL, NULL, &mUsbOnlineEvent);
	ASSERT_EFI_ERROR (Status);

    FastbootRegister("boot", CommandBoot);
	FastbootRegister("reboot", CommandReboot);
	FastbootPublish("product", "EDK2 LEO");
	FastbootPublish("kernel", "lk");

	// Init UDC first
    r = udc_init(&surf_udc_device);
	ASSERT(r==0);

    Status = gBS->CreateEvent (0, TPL_CALLBACK, NULL, NULL, &txn_done);
    ASSERT_EFI_ERROR(Status);

	in = udc_endpoint_alloc(UDC_TYPE_BULK_IN, 512);
	ASSERT(in==0);
	out = udc_endpoint_alloc(UDC_TYPE_BULK_OUT, 512);
	ASSERT(out==0);

	fastboot_endpoints[0] = in;
	fastboot_endpoints[1] = out;

	req = udc_request_alloc();
	ASSERT(req==0);

	r = udc_register_gadget(&fastboot_gadget);
	ASSERT(r==0);

	FastbootRegister("getvar:", CommandGetVar);
	FastbootRegister("download:", CommandDownload);
	FastbootRegister("flash:esp:", CommandFlashEsp);
	FastbootRegister("oem", CommandOEMHandler);
	FastbootPublish("version", "0.5");

	r = udc_start();
	ASSERT(r==0);

    FastbootHandler(NULL); // we don't use threads so just loop

	if (mFastbootState!=STATE_STOPPED) {
    	Status = gBS->CloseEvent(mExitBootServicesEvent);
    	udc_stop();
  	}

	return EFI_SUCCESS;
}