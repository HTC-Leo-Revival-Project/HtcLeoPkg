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

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/HardwareInterrupt.h>


#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "udc.h"

VOID boot_linux(VOID *bootimg, UINTN sz);

/* todo: give lk strtoul and nuke this */
static UINTN hex2unsigned(const char *x)
{
    UINTN n = 0;

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
	UINTN prefix_len;
	VOID (*handle)(const char *arg, VOID *data, UINTN sz);
};

struct fastboot_var {
	struct fastboot_var *next;
	const char *name;
	const char *value;
};
	
static struct fastboot_cmd *cmdlist;

VOID fastboot_register(const char *prefix,
		       VOID (*handle)(const char *arg, VOID *data, UINTN sz))
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

VOID fastboot_publish(const char *name, const char *value)
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


// static event_t usb_online;
// static event_t txn_done;
EFI_EVENT usb_online;
EFI_EVENT txn_done;
static UINTN  buffer[4096]; //used to be char remember for later
static struct udc_endpoint *in, *out;
static struct udc_request *req;
UINTN txn_status;

static VOID *download_base;
static UINTN download_max;
static UINTN download_size;

#define STATE_OFFLINE	0
#define STATE_COMMAND	1
#define STATE_COMPLETE	2
#define STATE_ERROR	3

static UINTN fastboot_state = STATE_OFFLINE;

static VOID req_complete(struct udc_request *req, UINTN actual, UINTN status)
{
	txn_status = status;
	req->length = actual;

     EFI_STATUS Status;
	//event_signal(&txn_done, 0);
                Status = gBS->SignalEvent(txn_done);
    if (EFI_ERROR(Status)) {
         DEBUG((EFI_D_ERROR, "Failed to signal event: %r\n", Status));
        gBS->CloseEvent(txn_done); // Clean up event if signaling fails
    }
		
}

static UINTN usb_read(VOID *_buf, UINTN len)
{
	UINTN r;
	UINTN xfer;
	UINTN *buf = _buf; // used to be int char or unsigned char
	UINTN count = 0;
    EFI_STATUS Status;

	if (fastboot_state == STATE_ERROR)
		goto oops;

	while (len > 0) {
		xfer = (len > 4096) ? 4096 : len;
		req->buf = buf;
		req->length = xfer;
		req->complete = req_complete;
		r = udc_request_queue(out, req);
		if (r < 0) {
			// dprintf(INFO, "usb_read() queue failed\n");
             DEBUG((EFI_D_ERROR,  "usb_read() queue failed\n"));
			goto oops;
		}
		//event_wait(&txn_done);
            // Wait for the event to be signaled
    Status = gBS->WaitForEvent(
        1,                              // Number of events to wait for
        &txn_done,                       // Array of event objects to wait for
        NULL                            // Pointer to the index of the event that was signaled
    );

		if (txn_status < 0) {
			// dprintf(INFO, "usb_read() transaction failed\n");
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
	fastboot_state = STATE_ERROR;
	return -1;
}

static UINTN usb_write(VOID *buf, UINTN len)
{
	UINTN r;
    EFI_STATUS Status;

	if (fastboot_state == STATE_ERROR)
		goto oops;

	req->buf = buf;
	req->length = len;
	req->complete = req_complete;
	r = udc_request_queue(in, req);
	if (r < 0) {
		// dprintf(INFO, "usb_write() queue failed\n");
         DEBUG((EFI_D_ERROR, "usb_write() queue failed\n"));
		goto oops;
	}
	// event_wait(&txn_done);
                // Wait for the event to be signaled
    Status = gBS->WaitForEvent(
        1,                              // Number of events to wait for
        &txn_done,                       // Array of event objects to wait for
        NULL                            // Pointer to the index of the event that was signaled
    );
	if (txn_status < 0) {
		// dprintf(INFO, "usb_write() transaction failed\n");
         DEBUG((EFI_D_ERROR, "usb_write() transaction failed\n"));
		goto oops;
	}
	return req->length;

oops:
	fastboot_state = STATE_ERROR;
	return -1;
}

VOID fastboot_ack(const char *code, const char *reason)
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

VOID fastboot_fail(const char *reason)
{
	fastboot_ack("FAIL", reason);
}

VOID fastboot_okay(const char *info)
{
	fastboot_ack("OKAY", info);
}

UINTN fastboot_write(VOID *buf, UINTN len)
{
	return usb_write(buf, len);
}

VOID fastboot_info(const char *reason)
{
	char response[512];

	if (fastboot_state != STATE_COMMAND)
		return;

	if (reason == 0)
		return;

	snprintf((char *)response, 512, "INFO%s", reason);

	usb_write(response, strlen((const char *)response));
}

static VOID getvar_all(VOID)
{
	struct fastboot_var *var;
	char getvar_all[256];

	for (var = varlist; var; var = var->next)
	{
		strlcpy((char *) getvar_all, var->name, sizeof(getvar_all));
		strlcat((char *) getvar_all, ":", sizeof(getvar_all));
		strlcat((char *) getvar_all, var->value, sizeof(getvar_all));
		fastboot_info(getvar_all);
		memset((VOID *) getvar_all, '\0', sizeof(getvar_all));
	}
	fastboot_okay("");
}

static VOID cmd_getvar(const char *arg, VOID *data, UINTN sz)
{
	struct fastboot_var *var;


	if (!strncmp("all", arg, strlen(arg)))
	{
		getvar_all();
		return;
	}

	for (var = varlist; var; var = var->next) {
		if (!strcmp(var->name, arg)) {
			fastboot_okay(var->value);
			return;
		}
	}
	fastboot_okay("");
}

static VOID cmd_download(const char *arg, VOID *data, UINTN sz)
{
	char response[64];
	UINTN len = hex2unsigned(arg);
	UINTN r;

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

static VOID fastboot_command_loop(VOID)
{
	struct fastboot_cmd *cmd;
	UINTN r;
	// dprintf(INFO,"fastboot: processing commands\n");
     DEBUG((EFI_D_ERROR, "fastboot: processing commands\n"));

again:
	while (fastboot_state != STATE_ERROR) {
		r = usb_read(buffer, 64);
		if (r < 0) break;
		buffer[r] = 0;
		// dprintf(INFO,"fastboot: %s\n", buffer);
         DEBUG((EFI_D_ERROR, "fastboot: %s\n", buffer));

		for (cmd = cmdlist; cmd; cmd = cmd->next) {
			if (memcmp(buffer, cmd->prefix, cmd->prefix_len))
				continue;
			fastboot_state = STATE_COMMAND;
			cmd->handle((const char*) buffer + cmd->prefix_len,
				    (VOID*) download_base, download_size);
			if (fastboot_state == STATE_COMMAND)
				fastboot_fail("unknown reason");
			goto again;
		}

		fastboot_fail("unknown command");
			
	}
	fastboot_state = STATE_OFFLINE;
	// dprintf(INFO,"fastboot: oops!\n");
     DEBUG((EFI_D_ERROR, "fastboot: oops!\n"));
}

static UINTN fastboot_handler(VOID *arg)
{
    EFI_STATUS Status;
		// event_wait(&usb_online);
                    // Wait for the event to be signaled
    Status = gBS->WaitForEvent(
        1,                              // Number of events to wait for
        &usb_online,                       // Array of event objects to wait for
        NULL                            // Pointer to the index of the event that was signaled
    );
		fastboot_command_loop();
	return 0;
}

static VOID fastboot_notify(struct udc_gadget *gadget, UINTN event)
{

     EFI_STATUS Status;

	if (event == UDC_EVENT_ONLINE) {
            Status = gBS->SignalEvent(usb_online);
    if (EFI_ERROR(Status)) {
         DEBUG((EFI_D_ERROR, "Failed to signal event: %r\n", Status));
        gBS->CloseEvent(usb_online); // Clean up event if signaling fails
    }
		//event_signal(&usb_online, 0);
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

VOID UsbOnlineNotifyFunction(IN EFI_EVENT Event, IN VOID *Context) {
    Print(L"UsbOnlineNotifyFunction has been called.\n");
}

VOID TxnDoneNotifyFunction(IN EFI_EVENT Event, IN VOID *Context) {
    Print(L"TxnDoneNotifyFunction has been called.\n");
}

UINTN fastboot_init(VOID *base, UINTN size)
{
	// dprintf(INFO, "fastboot_init()\n");
     DEBUG((EFI_D_ERROR, "fastboot_init()\n"));

	download_base = base;
	download_max = size;

	// event_init(&usb_online, 0, EVENT_FLAG_AUTOUNSIGNAL);
	// event_init(&txn_done, 0, EVENT_FLAG_AUTOUNSIGNAL);

        EFI_STATUS Status;

    // Create an event for usb online
    Status = gBS->CreateEvent(
        EVT_NOTIFY_SIGNAL,             // Event type
        TPL_CALLBACK,                  // Notify function's priority
        UsbOnlineNotifyFunction,         // Function to be called when the event is signaled
        NULL,                          // Context to pass to the notify function
        &usb_online                       // Event object
    );

        if (EFI_ERROR(Status)) {
        Print(L"Failed to create event: %r\n", Status);
        return Status;
    }

        // Create an event for txn_done
    Status = gBS->CreateEvent(
        EVT_NOTIFY_SIGNAL,             // Event type
        TPL_CALLBACK,                  // Notify function's priority
        TxnDoneNotifyFunction,         // Function to be called when the event is signaled
        NULL,                          // Context to pass to the notify function
        &txn_done                       // Event object
    );

        if (EFI_ERROR(Status)) {
        Print(L"Failed to create event: %r\n", Status);
        return Status;
    }

	in = udc_endpoint_alloc(UDC_TYPE_BULK_IN, 512);
	if (!in)
		goto fail_alloc_in;
	out = udc_endpoint_alloc(UDC_TYPE_BULK_OUT, 512);
	if (!out)
		goto fail_alloc_out;

	fastboot_endpoints[0] = in;
	fastboot_endpoints[1] = out;

	req = udc_request_alloc();
	if (!req)
		goto fail_alloc_req;

	if (udc_register_gadget(&fastboot_gadget))
		goto fail_udc_register;

	fastboot_register("getvar:", cmd_getvar);
	fastboot_register("download:", cmd_download);
	//fastboot_publish("version", CLK_VERSION);

	// thr = thread_create("fastboot", fastboot_handler, 0, DEFAULT_PRIORITY, 4096);
	// thread_resume(thr);
	return 0;

fail_udc_register:
	udc_request_free(req);
fail_alloc_req:
	udc_endpoint_free(out);	
fail_alloc_out:
	udc_endpoint_free(in);
fail_alloc_in:
	return -1;
}
