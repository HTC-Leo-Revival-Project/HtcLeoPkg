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

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/ArmLib.h>

#include <Library/PlatformResetLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Library/LKEnvLib.h>
#include <Library/MallocLib.h>
#include <Library/LcmLib.h>
#include <Library/udc.h>
#include <Library/hsusb.h>

#include "fastboot.h"

STATIC EFI_EVENT mExitBootServicesEvent;
STATIC EFI_EVENT mUsbOnlineEvent = NULL;
STATIC EFI_EVENT txn_done = NULL;

static struct udc_endpoint *in, *out;
static struct udc_endpoint *fastboot_endpoints[2];
static struct udc_request *req;
int txn_status;

STATIC VOID *DownloadBase = NULL;
STATIC UINT32 DownloadSize = 0;
STATIC UINTN DownloadPages = 0;

static unsigned mFastbootState = STATE_OFFLINE;

struct fastboot_cmd {
	struct fastboot_cmd *next;
	const char *prefix;
	unsigned prefix_len;
	void (*handle)(const char *arg, void *data, unsigned sz);
};

struct fastboot_var {
	struct fastboot_var *next;
	const char *name;
	const char *value;
};
	
static struct fastboot_cmd *CommandList;
static struct fastboot_var *VariableList;

// USB read/write
static void req_complete(struct udc_request *req, unsigned actual, int status)
{
	txn_status = status;
	req->length = actual;

	gBS->SignalEvent (txn_done);
}

static int usb_read(void *_buf, unsigned len)
{
	int r;
	unsigned xfer;
	unsigned char *buf = _buf;
	int count = 0;
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
			dprintf(INFO, "usb_read() queue failed\n");
			goto oops;
		}
        gBS->WaitForEvent (1, &txn_done, &EventIndex);

		if (txn_status < 0) {
			dprintf(INFO, "usb_read() transaction failed\n");
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

static int usb_write(void *buf, unsigned len)
{
	int r;
    UINTN EventIndex;

	if (mFastbootState == STATE_ERROR)
		goto oops;

	req->buf = buf;
	req->length = len;
	req->complete = req_complete;
	r = udc_request_queue(in, req);
	if (r < 0) {
		dprintf(INFO, "usb_write() queue failed\n");
		goto oops;
	}
    gBS->WaitForEvent (1, &txn_done, &EventIndex);
	if (txn_status < 0) {
		dprintf(INFO, "usb_write() transaction failed\n");
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
	unsigned Event)
{
	if (Event == UDC_EVENT_ONLINE) {
        gBS->SignalEvent (mUsbOnlineEvent);
	}
}

static struct udc_device surf_udc_device = {
	.vendor_id	= 0x18d1,
	.product_id	= 0x0D02,
	.version_id	= 0x0001,
	.manufacturer	= "HTC",
	.product	= "LEO EDK2",
};

static struct udc_gadget fastboot_gadget = {
	.notify		= FastbootNotify,
	.ifc_class	= 0xff,
	.ifc_subclass	= 0x42,
	.ifc_protocol	= 0x03,
	.ifc_endpoints	= 2,
	.ifc_string	= "fastboot",
	.ept		= fastboot_endpoints,
};

/* todo: give lk strtoul and nuke this */
static unsigned hex2unsigned(const char *x)
{
    unsigned n = 0;

    while(*x) {
        switch(*x) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            n = (n << 4) | (*x - '0');
            break;
        case 'a': case 'b': case 'c':
        case 'd': case 'e': case 'f':
            n = (n << 4) | (*x - 'a' + 10);
            break;
        case 'A': case 'B': case 'C':
        case 'D': case 'E': case 'F':
            n = (n << 4) | (*x - 'A' + 10);
            break;
        default:
            return n;
        }
        x++;
    }

    return n;
}

void 
FastbootAck(
	const char *Code, 
	const char *Reason
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

void 
FastbootFail(
	const char *Reason
)
{
	FastbootAck("FAIL", Reason);
}

void 
FastbootOkay(
	const char *Info
)
{
	FastbootAck("OKAY", Info);
}

void 
FastbootRegister(
	const char *Prefix,
	void (*Handle)(const char *Arg, void *Data, unsigned Size))
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

void 
FastbootPublish(
	const char *Name, 
	const char *Value
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

static void 
CommandDownload(
	const char *Arg, 
	void *Data, 
	unsigned Size
)
{
	char Response[FASTBOOT_COMMAND_MAX_LENGTH];
	unsigned Length = hex2unsigned(Arg);
	int r;

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

static void 
FastbootCommandLoop(
	void
)
{
	struct fastboot_cmd *Command;
	int r;
	dprintf(INFO,"fastboot: processing commands\n");

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

static void 
FastbootHandler(
	void *arg
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

static void 
CommandGetVar(
	const char *arg, 
	void *data, 
	unsigned sz
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


// placeholder
void 
CommandBoot(
	const char *arg, 
	void *data, 
	unsigned sz) 
{
    DEBUG((EFI_D_ERROR, "BOOT CALLED\n"));
}

void 
CommandReboot(
	const char *arg, 
	void *data, 
	unsigned sz) 
{
    ResetCold();
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