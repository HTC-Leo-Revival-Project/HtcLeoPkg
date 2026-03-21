#include "../menu.h"
#include "fastboot.h"
#include <Library/Lk/LKEnvLib.h>

// LK CODE START
/*
 * Copyright (c) 2009, Google Inc.
 * All rights reserved.
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

/*#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <dev/udc.h>*/
#include <Library/MallocLib.h>
#include <Library/LcmLib.h>
#include <Library/udc.h>

//void boot_linux(void *bootimg, unsigned sz);

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
	
static struct fastboot_cmd *cmdlist;

void fastboot_register(const char *prefix,
		       void (*handle)(const char *arg, void *data, unsigned sz))
{
	struct fastboot_cmd *cmd;
	cmd = malloc(sizeof(*cmd));
	if (cmd) {
		cmd->prefix = prefix;
		cmd->prefix_len = strlen(prefix);
		cmd->handle = handle;
		cmd->next = cmdlist;
		cmdlist = cmd;
	}
}

static struct fastboot_var *varlist;

void fastboot_publish(const char *name, const char *value)
{
	struct fastboot_var *var;
	var = malloc(sizeof(*var));
	if (var) {
		var->name = name;
		var->value = value;
		var->next = varlist;
		varlist = var;
	}
}


//static event_t usb_online;
EFI_EVENT usb_online = NULL;
//static event_t txn_done;
EFI_EVENT txn_done = NULL;

static unsigned char buffer[4096];
static struct udc_endpoint *in, *out;
static struct udc_request *req;
int txn_status;

static void *download_base;
static unsigned download_max;
static unsigned download_size;

#define STATE_OFFLINE	0
#define STATE_COMMAND	1
#define STATE_COMPLETE	2
#define STATE_ERROR	3

static unsigned fastboot_state = STATE_OFFLINE;

static void req_complete(struct udc_request *req, unsigned actual, int status)
{
	txn_status = status;
	req->length = actual;
	//event_signal(&txn_done, 0);

}

static int usb_read(void *_buf, unsigned len)
{
	int r;
	unsigned xfer;
	unsigned char *buf = _buf;
	int count = 0;
    UINTN EventIndex;

	if (fastboot_state == STATE_ERROR)
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
		//event_wait(&txn_done);
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
	fastboot_state = STATE_ERROR;
	return -1;
}

static int usb_write(void *buf, unsigned len)
{
	int r;
    UINTN EventIndex;

	if (fastboot_state == STATE_ERROR)
		goto oops;

	req->buf = buf;
	req->length = len;
	req->complete = req_complete;
	r = udc_request_queue(in, req);
	if (r < 0) {
		dprintf(INFO, "usb_write() queue failed\n");
		goto oops;
	}
	//event_wait(&txn_done);
    gBS->WaitForEvent (1, &txn_done, &EventIndex);
	if (txn_status < 0) {
		dprintf(INFO, "usb_write() transaction failed\n");
		goto oops;
	}
	return req->length;

oops:
	fastboot_state = STATE_ERROR;
	return -1;
}

void fastboot_ack(const char *code, const char *reason)
{
	char response[64];

	if (fastboot_state != STATE_COMMAND)
		return;

	if (reason == 0)
		reason = "";

	snprintf(response, 64, "%s%s", code, reason);
	fastboot_state = STATE_COMPLETE;

	usb_write(response, strlen(response));

}

void fastboot_fail(const char *reason)
{
	fastboot_ack("FAIL", reason);
}

void fastboot_okay(const char *info)
{
	fastboot_ack("OKAY", info);
}

int fastboot_write(void *buf, unsigned len)
{
	return usb_write(buf, len);
}

static void cmd_getvar(const char *arg, void *data, unsigned sz)
{
	struct fastboot_var *var;

	for (var = varlist; var; var = var->next) {
		if (!strcmp(var->name, arg)) {
			fastboot_okay(var->value);
			return;
		}
	}
	fastboot_okay("");
}

static void cmd_download(const char *arg, void *data, unsigned sz)
{
	char response[64];
	unsigned len = hex2unsigned(arg);
	int r;

	download_size = 0;
	if (len > download_max) {
		fastboot_fail("data too large");
		return;
	}

	sprintf(response,"DATA%08x", len);
	if (usb_write(response, strlen(response)) < 0)
		return;

	r = usb_read(download_base, len);
	if ((r < 0) || (r != len)) {
		fastboot_state = STATE_ERROR;
		return;
	}
	download_size = len;
	fastboot_okay("");
}

unsigned ulpi_read(unsigned reg)
{
	/* initiate read operation */
	writel(ULPI_RUN | ULPI_READ | ULPI_ADDR(reg), USB_ULPI_VIEWPORT);

	/* wait for completion */
	while (readl(USB_ULPI_VIEWPORT) & ULPI_RUN) ;

	return ULPI_DATA_READ(readl(USB_ULPI_VIEWPORT));
}

void ulpi_write(unsigned val, unsigned reg)
{
	/* initiate write operation */
	writel(ULPI_RUN | ULPI_WRITE |
	       ULPI_ADDR(reg) | ULPI_DATA(val), USB_ULPI_VIEWPORT);

	/* wait for completion */
	while (readl(USB_ULPI_VIEWPORT) & ULPI_RUN) ;
}

static void fastboot_command_loop(void)
{
	struct fastboot_cmd *cmd;
	int r;
	dprintf(INFO,"fastboot: processing commands\n");
    DEBUG((EFI_D_ERROR, "fastboot: processing commands\n"));

	UINT8* Buffer = AllocateAlignedPages(ArmDataCacheLineLength(), ROUNDUP(4096, ArmDataCacheLineLength()));
    ASSERT(Buffer);

again:
	while (fastboot_state != STATE_ERROR) {
		SetMem(Buffer, 64, 0);
		InvalidateDataCacheRange(Buffer, 64);

		r = usb_read(Buffer, 64);
		if (r < 0) break;
		buffer[r] = 0;
		dprintf(INFO,"fastboot: %s\n", buffer);
        DEBUG((EFI_D_ERROR, "fastboot: %s\n", buffer));

		for (cmd = cmdlist; cmd; cmd = cmd->next) {
			if (memcmp(buffer, cmd->prefix, cmd->prefix_len))
				continue;
			fastboot_state = STATE_COMMAND;
			cmd->handle((const char*) buffer + cmd->prefix_len,
				    (void*) download_base, download_size);
			if (fastboot_state == STATE_COMMAND)
				fastboot_fail("unknown reason");
			goto again;
		}

		fastboot_fail("unknown command");
			
	}
	fastboot_state = STATE_OFFLINE;
	dprintf(INFO,"fastboot: oops!\n");
    DEBUG((EFI_D_ERROR, "fastboot: oops\n"));
}

// HERE
static int fastboot_handler(void *arg)
{
	for (;;) {
        UINTN EventIndex;

		//event_wait(&usb_online);
        DEBUG((EFI_D_ERROR, "fastboot handler: wait for event\n"));
        gBS->WaitForEvent (1, &usb_online, &EventIndex);
        DEBUG((EFI_D_ERROR, "fastboot_command_loop(): \n"));
		fastboot_command_loop();
	}
	return 0;
}

static void fastboot_notify(struct udc_gadget *gadget, unsigned event)
{
    DEBUG((EFI_D_ERROR, "fastboot_notify()\n"));
	if (event == UDC_EVENT_ONLINE) {
		//event_signal(&usb_online, 0);
        DEBUG((EFI_D_ERROR, "fastboot_notify(): usb_online!!\n"));
        gBS->SignalEvent (usb_online);
	}
}

static struct udc_endpoint *fastboot_endpoints[2];

static struct udc_gadget fastboot_gadget = {
	.notify		= fastboot_notify,
	.ifc_class	= 0xff,
	.ifc_subclass	= 0x42,
	.ifc_protocol	= 0x03,
	.ifc_endpoints	= 2,
	.ifc_string	= "fastboot",
	.ept		= fastboot_endpoints,
};

int fastboot_init(void *base, unsigned size)
{
    EFI_STATUS Status = EFI_SUCCESS;
	//thread_t *thr;
	dprintf(INFO, "fastboot_init()\n");

    DEBUG((EFI_D_ERROR, "fastboot_init()\n"));

	download_base = base;
	download_max = size;

	//event_init(&usb_online, 0, EVENT_FLAG_AUTOUNSIGNAL);
    DEBUG((EFI_D_ERROR, "create events\n"));
    Status = gBS->CreateEvent (0, TPL_CALLBACK, NULL, NULL, &usb_online);
    ASSERT_EFI_ERROR(Status);
	//event_init(&txn_done, 0, EVENT_FLAG_AUTOUNSIGNAL);
    Status = gBS->CreateEvent (0, TPL_CALLBACK, NULL, NULL, &txn_done);
    ASSERT_EFI_ERROR(Status);

    DEBUG((EFI_D_ERROR, "alloc endpoint\n"));

	in = udc_endpoint_alloc(UDC_TYPE_BULK_IN, 512);
	if (!in)
		goto fail_alloc_in;
	out = udc_endpoint_alloc(UDC_TYPE_BULK_OUT, 512);
	if (!out)
		goto fail_alloc_out;

    DEBUG((EFI_D_ERROR, "alloc SUCCESS!\n"));

	fastboot_endpoints[0] = in;
	fastboot_endpoints[1] = out;

    DEBUG((EFI_D_ERROR, "udc_request_alloc()\n"));
	req = udc_request_alloc();
	if (!req)
		goto fail_alloc_req;

    DEBUG((EFI_D_ERROR, "udc_register_gadget()\n"));
	if (udc_register_gadget(&fastboot_gadget))
		goto fail_udc_register;

     DEBUG((EFI_D_ERROR, "fastboot_register commands\n"));
	fastboot_register("getvar:", cmd_getvar);
	fastboot_register("download:", cmd_download);
	fastboot_publish("version", "0.5");

	DEBUG((EFI_D_ERROR, "udc_start()\n"));
	udc_start();

	//thr = thread_create("fastboot", fastboot_handler, 0, DEFAULT_PRIORITY, 4096);
	//thread_resume(thr);

    DEBUG((EFI_D_ERROR, "start handler\n"));
    fastboot_handler(NULL); // we don't use threads so just loop

	return 0;

fail_udc_register:
	udc_request_free(req);
fail_alloc_req:
	udc_endpoint_free(out);	
fail_alloc_out:
	udc_endpoint_free(in);
fail_alloc_in:
    DEBUG((EFI_D_ERROR, "alloc failed\n"));
	return -1;
}
// LK CODE END

// ABOOT 
/*
void aboot_init(const struct app_descriptor *app)
{
	unsigned reboot_mode = 0;
	unsigned disp_init = 0;
	unsigned usb_init = 0;

	// Setup page size information for nand/emmc reads
	if (target_is_emmc_boot())
	{
		page_size = 2048;
		page_mask = page_size - 1;
	}
	else
	{
		page_size = flash_page_size();
		page_mask = page_size - 1;
	}

	// Display splash screen if enabled
	#if DISPLAY_SPLASH_SCREEN
	display_init();
	dprintf(INFO, "Diplay initialized\n");
	disp_init = 1;
	diplay_image_on_screen();
	#endif

	// Check if we should do something other than booting up
	if (keys_get_state(KEY_HOME) != 0)
		boot_into_recovery = 1;
	if (keys_get_state(KEY_BACK) != 0)
		goto fastboot;
	if (keys_get_state(KEY_CLEAR) != 0)
		goto fastboot;

	#if NO_KEYPAD_DRIVER
	// With no keypad implementation, check the status of USB connection.
	// If USB is connected then go into fastboot mode.
	usb_init = 1;
	udc_init(&surf_udc_device);
	if (usb_cable_status())
		goto fastboot;
	#endif

	reboot_mode = check_reboot_mode();
	if (reboot_mode == RECOVERY_MODE) {
		boot_into_recovery = 1;
	} else if(reboot_mode == FASTBOOT_MODE) {
		goto fastboot;
	}

	if (target_is_emmc_boot())
	{
		boot_linux_from_mmc();
	}
	else
	{
		recovery_init();
		boot_linux_from_flash();
	}
	dprintf(CRITICAL, "ERROR: Could not do normal boot. Reverting "
		"to fastboot mode.\n");

fastboot:
	htcleo_fastboot_init();

	if(!usb_init)
		udc_init(&surf_udc_device);

	fastboot_register("boot", cmd_boot);

	if (target_is_emmc_boot())
	{
		fastboot_register("flash:", cmd_flash_mmc);
		fastboot_register("erase:", cmd_erase_mmc);
	}
	else
	{
		fastboot_register("flash:", cmd_flash);
		fastboot_register("erase:", cmd_erase);
	}

	fastboot_register("continue", cmd_continue);
	fastboot_register("reboot", cmd_reboot);
	fastboot_register("reboot-bootloader", cmd_reboot_bootloader);
	fastboot_publish("product", TARGET(BOARD));
	fastboot_publish("kernel", "lk");

	//fastboot_init(target_get_scratch_address(), 120 * 1024 * 1024);
	fastboot_init(target_get_scratch_address(), MEMBASE - SCRATCH_ADDR - 0x00100000);
	udc_start();
	target_battery_charging_enable(1, 0);
}
*/

// ABOOT END

static struct udc_device surf_udc_device = {
	.vendor_id	= 0x18d1,
	.product_id	= 0x0D02,
	.version_id	= 0x0001,
	.manufacturer	= "HTC",
	.product	= "LEO EDK2",
};

#define MEMBASE         0x28000000
#define BASE_ADDR       0x11800000
#define TAGS_ADDR       (BASE_ADDR+0x00000100)
#define KERNEL_ADDR     (BASE_ADDR+0x00008000)
#define RAMDISK_ADDR    (BASE_ADDR+0x00a00000)
#define SCRATCH_ADDR    (BASE_ADDR+0x01400000)

// placeholder
void cmd_boot(const char *arg, void *data, unsigned sz) {
    DEBUG((EFI_D_ERROR, "BOOT CALLED\n"));
}

void cmd_continue(const char *arg, void *data, unsigned sz) {
    DEBUG((EFI_D_ERROR, "CONTINUE CALLED\n"));
	fastboot_okay("");
}

void StartFastboot(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable)
{
    // All necessary code to start listening for commands
    
    

    // TODO: Add functions
    DEBUG((EFI_D_ERROR, "register boot\n"));
    fastboot_register("boot", cmd_boot);

    DEBUG((EFI_D_ERROR, "fastboot publish\n"));
	fastboot_register("continue", cmd_continue);
	//fastboot_register("reboot", cmd_reboot);
	//fastboot_register("reboot-bootloader", cmd_reboot_bootloader);
	fastboot_publish("product", "EDK2 LEO");//fastboot_publish("product", TARGET(BOARD));
	fastboot_publish("kernel", "lk");

	// Init UDC first
    DEBUG((EFI_D_ERROR, "udc_init()\n"));
    udc_init(&surf_udc_device);

	//fastboot_init(target_get_scratch_address(), 120 * 1024 * 1024);
    DEBUG((EFI_D_ERROR, "fastboot init()\n"));
	//fastboot_init((void *)SCRATCH_ADDR, (MEMBASE - SCRATCH_ADDR - 0x00100000));
	fastboot_init((void *)SCRATCH_ADDR, 0x00040000);

    //DEBUG((EFI_D_ERROR, "udc_start()\n"));
	//udc_start();
}

#if 0
VOID
FastbootInit (
  VOID
)
{
  EFI_STATUS Status;
  INT32 r;

  MenuShowProgressDialog("Starting Fastboot", TRUE);

  mFastbootState = STATE_OFFLINE;
  Status = gBS->CreateEvent (0, TPL_CALLBACK, NULL, NULL, &mUsbOnlineEvent);
  ASSERT_EFI_ERROR (Status);

  mUsbInterface = mLKApi->usbgadget_get_interface();
  ASSERT(mUsbInterface);

  FastbootRegister("oem help", CommandHelp);
  FastbootRegister("getvar:", CommandGetVar);
  FastbootRegister("download:", CommandDownload);
  FastbootPublish("version", "0.5");

  // we use dynamic allocations, so report the highest possible value
  FastbootPublish("max-download-size", "0xffffffff");

  surf_udc_device.serialno = AsciiStrDup("EFIDroid");
  r = mUsbInterface->udc_init(mUsbInterface, &surf_udc_device);
  ASSERT(r==0);
  r= mUsbInterface->udc_register_gadget(mUsbInterface, &fastboot_gadget);
  ASSERT(r==0);
  r = mUsbInterface->udc_start(mUsbInterface);
  ASSERT(r==0);

  Status = gBS->CreateEvent (EVT_SIGNAL_EXIT_BOOT_SERVICES, TPL_NOTIFY, ExitBootServicesEvent, NULL, &mExitBootServicesEvent);
  ASSERT_EFI_ERROR (Status);

  FastbootHandler();

  if (mFastbootState!=STATE_STOPPED) {
    Status = gBS->CloseEvent(mExitBootServicesEvent);
    mUsbInterface->udc_stop(mUsbInterface);
  }
}
#endif