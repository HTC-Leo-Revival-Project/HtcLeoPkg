/*
 * Copyright (c) 2008, Google Inc.
 * All rights reserved.
 *
 * Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 * Copyright (c) 2024, J0SH1X <aljoshua.hell@gmail.com>.
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
 * OF USE, DATA, OR PROFITS; OR BUSINESS UINTNERRUPTION) HOWEVER CAUSED 
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
#include <Chipset/iomap.h>
#include <Chipset/irqs.h>
#include <Library/types.h>

#include "udc.h"

#include "hsusb.h"

/* these should probably go elsewhere */
#define GET_STATUS           0
#define CLEAR_FEATURE        1
#define SET_FEATURE          3
#define SET_ADDRESS          5
#define GET_DESCRIPTOR       6
#define SET_DESCRIPTOR       7
#define GET_CONFIGURATION    8
#define SET_CONFIGURATION    9
#define GET_INTERFACE        10
#define SET_INTERFACE        11
#define SYNCH_FRAME          12

#define TYPE_DEVICE          1
#define TYPE_CONFIGURATION   2
#define TYPE_STRING          3
#define TYPE_INTERFACE       4
#define TYPE_ENDPOINT        5

#define DEVICE_READ          0x80
#define DEVICE_WRITE         0x00
#define INTERFACE_READ       0x81
#define INTERFACE_WRITE      0x01
#define ENDPOINT_READ        0x82
#define ENDPOINT_WRITE       0x02

#define strlen(s) ((size_t)AsciiStrLen((s)))

UINTN charger_usb_disconnected(VOID);
UINTN charger_usb_i(UINTN current);
UINTN charger_usb_is_pc_connected(VOID);
UINTN charger_usb_is_charger_connected(VOID);

/* common code - factor out UINTNo a shared file */

#define writel(v, a) MmioWrite32((UINTN)(a), (UINT32)(v))

struct udc_descriptor {
	struct udc_descriptor *next;
	UINTN  tag; /* ((TYPE << 8) | NUM) */
	UINTN  len; /* total length */
	 char data[0];
};

struct udc_descriptor *udc_descriptor_alloc(UINTN type, UINTN num, UINTN len)
{
	struct udc_descriptor *desc;
	if ((len > 255) || (len < 2) || (num > 255) || (type > 255))
		return 0;

    desc = AllocatePool(sizeof(struct udc_descriptor) + len);

	desc->next = 0;
	desc->tag = (type << 8) | num;
	desc->len = len;
	desc->data[0] = len;
	desc->data[1] = type;

	return desc;
}

static struct udc_descriptor *desc_list = 0;
static UINTN next_string_id = 1;

VOID udc_descriptor_register(struct udc_descriptor *desc)
{
	desc->next = desc_list;
	desc_list = desc;
}

UINTN udc_string_desc_alloc(const char *str)
{
	UINTN len;
	struct udc_descriptor *desc;
	 char *data;

	if (next_string_id > 255)
		return 0;

	if (!str)
		return 0;

    len = strlen(str);
	desc = udc_descriptor_alloc(TYPE_STRING, next_string_id, len * 2 + 2);
	if (!desc)
		return 0;
	next_string_id++;

	/* expand ascii string to utf16 */
	data = desc->data + 2;
	while (len-- > 0) {
		*data++ = *str++;
		*data++ = 0;
	}

	udc_descriptor_register(desc);
	return desc->tag & 0xff;
}

#if 1
#define DBG(x...) do {} while(0)
#else
#define DBG(x...) dprintf(INFO, x)
#endif

//#define DBG1(x...) dprintf(INFO, x)

#define usb_status(a,b)

struct usb_request {
	struct udc_request req;
	struct ept_queue_item *item;
};
	
struct udc_endpoint
{
	struct udc_endpoint *next;
	UINTN bit;
	struct ept_queue_head *head;
	struct usb_request *req;
     char num;
     char in;
	UINTN maxpkt;
};

struct udc_endpoint *ept_list = 0;
struct ept_queue_head *epts = 0;

static UINTN usb_online = 0;
static UINTN usb_highspeed = 0;

static struct udc_device *the_device;
static struct udc_gadget *the_gadget;

struct udc_endpoint *_udc_endpoint_alloc(UINTN num, UINTN in, UINTN max_pkt)
{
	struct udc_endpoint *ept;
	UINTN cfg;
	//ept = malloc(sizeof(*ept));

    ept = AllocatePool( sizeof( ept ) );
    
	ept->maxpkt = max_pkt;
	ept->num = num;
	ept->in = !!in;
	ept->req = 0;

	cfg = CONFIG_MAX_PKT(max_pkt) | CONFIG_ZLT;

	if(ept->in) {
		ept->bit = EPT_TX(ept->num);
	} else {
		ept->bit = EPT_RX(ept->num);
		if(num == 0) 
			cfg |= CONFIG_IOS;
	}

	ept->head = epts + (num * 2) + (ept->in);
	ept->head->config = cfg;

	ept->next = ept_list;
	ept_list = ept;
    
//	arch_clean_invalidate_cache_range(ept->head, 64);
	// DBG("ept%d %s @%p/%p max=%d bit=%x\n", 
    //         num, in ? "in":"out", ept, ept->head, max_pkt, ept->bit);

	return ept;
}

static UINTN ept_alloc_table = EPT_TX(0) | EPT_RX(0);

struct udc_endpoint *udc_endpoint_alloc(UINTN type, UINTN maxpkt)
{
	struct udc_endpoint *ept;
	UINTN n;
	UINTN in;

	if (type == UDC_TYPE_BULK_IN) {
		in = 1;
	} else if (type == UDC_TYPE_BULK_OUT) {
		in = 0;
	} else {
		return 0;
	}

	for (n = 1; n < 16; n++) {
		UINTN bit = in ? EPT_TX(n) : EPT_RX(n);
		if (ept_alloc_table & bit)
			continue;
		ept = _udc_endpoint_alloc(n, in, maxpkt);
		if (ept)
			ept_alloc_table |= bit;
		return ept;
	}
	return 0;
}

VOID udc_endpoint_free(struct udc_endpoint *ept)
{
	/* todo */
}

static VOID endpoint_enable(struct udc_endpoint *ept, UINTN yes)
{
	UINTN n = MmioRead32(USB_ENDPTCTRL(ept->num));

	if(yes) {
		if(ept->in) {
			n |= (CTRL_TXE | CTRL_TXR | CTRL_TXT_BULK);
		} else {
			n |= (CTRL_RXE | CTRL_RXR | CTRL_RXT_BULK);
		}

		if(ept->num != 0) {
			/* XXX should be more dynamic... */
			if(usb_highspeed) {
				ept->head->config = CONFIG_MAX_PKT(512) | CONFIG_ZLT;
			} else {
				ept->head->config = CONFIG_MAX_PKT(64) | CONFIG_ZLT;
			}
		}
	}
	writel(n, USB_ENDPTCTRL(ept->num));
}

struct udc_request *udc_request_alloc(VOID)
{
	struct usb_request *req;
   //req = malloc(sizeof(*req));
        UINTN Size = sizeof(*req);

    req=AllocatePool( sizeof( req ) );
    
	req->req.buf = 0;
	req->req.length = 0;
	//req->item = memalign(32, 32);
	return &req->req;
}

VOID udc_request_free(struct udc_request *req)
{
	free(req);
}

UINTN udc_request_queue(struct udc_endpoint *ept, struct udc_request *_req)
{
	struct usb_request *req = (struct usb_request *) _req;
	struct ept_queue_item *item = req->item;
	UINTN phys = (UINTN) req->req.buf;
    
	item->next = TERMINATE;
	item->info = INFO_BYTES(req->req.length) | INFO_IOC | INFO_ACTIVE;
	item->page0 = phys;
	item->page1 = (phys & 0xfffff000) + 0x1000;
    //raise tpl
	// enter_critical_section();
	ept->head->next = (UINTN) item;
	ept->head->info = 0;
	ept->req = req;

//	arch_clean_invalidate_cache_range(item, 32);
//	arch_clean_invalidate_cache_range(ept->head, 64);
//	arch_clean_invalidate_cache_range(req->req.buf, req->req.length);
	// DBG("ept%d %s queue req=%p\n",
    //         ept->num, ept->in ? "in" : "out", req);




    //todo replace with mmiowrite32 ???
	writel(ept->bit, USB_ENDPTPRIME);
	//exit_critical_section();
	return 0;
}

static VOID handle_ept_complete(struct udc_endpoint *ept)
{
	struct ept_queue_item *item;
	UINTN actual;
	UINTN status;
	struct usb_request *req;
    
	// DBG("ept%d %s complete req=%p\n",
    //         ept->num, ept->in ? "in" : "out", ept->req);
    
	req = ept->req;
	if(req) {
		ept->req = 0;
        
		item = req->item;

		/* For some reason we are getting the notification for
		 * transfer completion before the active bit has cleared.
		 * HACK: wait for the ACTIVE bit to clear:
		 */
		while (MmioRead32((UINTN)&(item->info)) & INFO_ACTIVE);

//		arch_clean_invalidate_cache_range(item, 32);
//		arch_clean_invalidate_cache_range(req->req.buf, req->req.length);
		
		if(item->info & 0xff) {
			actual = 0;
			status = -1;
			// dprintf(INFO, "EP%d/%s FAIL nfo=%x pg0=%x\n",
			// 	ept->num, ept->in ? "in" : "out", item->info, item->page0);
		} else {
			actual = req->req.length - ((item->info >> 16) & 0x7fff);
			status = 0;
		}
		if(req->req.complete)
			req->req.complete(&req->req, actual, status);
	}
}

static const char *reqname(UINTN r)
{
	switch(r) {
	case GET_STATUS: return "GET_STATUS";
	case CLEAR_FEATURE: return "CLEAR_FEATURE";
	case SET_FEATURE: return "SET_FEATURE";
	case SET_ADDRESS: return "SET_ADDRESS";
	case GET_DESCRIPTOR: return "GET_DESCRIPTOR";
	case SET_DESCRIPTOR: return "SET_DESCRIPTOR";
	case GET_CONFIGURATION: return "GET_CONFIGURATION";
	case SET_CONFIGURATION: return "SET_CONFIGURATION";
	case GET_INTERFACE: return "GET_INTERFACE";
	case SET_INTERFACE: return "SET_UINTNERFACE";
	default: return "*UNKNOWN*";
	}
}

static struct udc_endpoint *ep0in, *ep0out;
static struct udc_request *ep0req;

static VOID setup_ack(VOID)
{
	ep0req->complete = 0;
	ep0req->length = 0;
	udc_request_queue(ep0in, ep0req);
}

static VOID ep0in_complete(struct udc_request *req, UINTN actual, UINTN status)
{
	// DBG("ep0in_complete %p %d %d\n", req, actual, status);
	if(status == 0) {
		req->length = 0;
		req->complete = 0;
		udc_request_queue(ep0out, req);
	}
}

static VOID setup_tx(VOID *buf, UINTN len)
{
	// DBG("setup_tx %p %d\n", buf, len);
	memcpy(ep0req->buf, buf, len);
	ep0req->complete = ep0in_complete;
	ep0req->length = len;
	udc_request_queue(ep0in, ep0req);
}

static UINTN usb_config_value = 0;

#define SETUP(type,request) (((type) << 8) | (request))

static VOID handle_setup(struct udc_endpoint *ept)
{
	struct setup_packet s;
    
	memcpy(&s, ept->head->setup_data, sizeof(s));

    

	writel(ept->bit, USB_ENDPTSETUPSTAT);

#if 0
	// DBG("handle_setup type=0x%02x req=0x%02x val=%d idx=%d len=%d (%s)\n",
    //         s.type, s.request, s.value, s.index, s.length,
    //         reqname(s.request));
#endif
	switch (SETUP(s.type,s.request)) {
	case SETUP(DEVICE_READ, GET_STATUS): {
		UINTN zero = 0;
		if (s.length == 2) {
			setup_tx(&zero, 2);
			return;
		}
		break;
	}
	case SETUP(DEVICE_READ, GET_DESCRIPTOR): {
		struct udc_descriptor *desc;
		/* usb_highspeed? */
		for (desc = desc_list; desc; desc = desc->next) {
			if (desc->tag == s.value) {
				UINTN len = desc->len;
				if (len > s.length) len = s.length;
				setup_tx(desc->data, len);
				return;
			}
		}
		break;
	}
	case SETUP(DEVICE_READ, GET_CONFIGURATION):
		/* disabling this causes data transaction failures on OSX. Why? */
		if ((s.value == 0) && (s.index == 0) && (s.length == 1)) {
			setup_tx(&usb_config_value, 1);
			return;
		}
		break;
	case SETUP(DEVICE_WRITE, SET_CONFIGURATION):
		if (s.value == 1) {
			struct udc_endpoint *ept;
			/* enable endpoints */
			for (ept = ept_list; ept; ept = ept->next){
				if (ept->num == 0) 
					continue;
				endpoint_enable(ept, s.value);
			}
			usb_config_value = 1;
			the_gadget->notify(the_gadget, UDC_EVENT_ONLINE);
		} else {
            
			writel(0, USB_ENDPTCTRL(1));
			usb_config_value = 0;
			the_gadget->notify(the_gadget, UDC_EVENT_OFFLINE);
		}
		setup_ack();
		usb_online = s.value ? 1 : 0;
		usb_status(s.value ? 1 : 0, usb_highspeed);
		return;
	case SETUP(DEVICE_WRITE, SET_ADDRESS):
		/* write address delayed (will take effect
		** after the next IN txn)
		*/
        
		writel((s.value << 25) | (1 << 24), USB_DEVICEADDR);
		setup_ack();
		return;
	case SETUP(INTERFACE_WRITE, SET_INTERFACE):
		/* if we ack this everything hangs */
		/* per spec, STALL is valid if there is not alt func */
		goto stall;
	case SETUP(0x02, CLEAR_FEATURE): {
		struct udc_endpoint *ept;
		UINTN num = s.index & 15;
		UINTN in = !!(s.index & 0x80);
        
		if ((s.value == 0) && (s.length == 0)) {
			// DBG("clr feat %d %d\n", num, in);
			for (ept = ept_list; ept; ept = ept->next) {
				if ((ept->num == num) && (ept->in == in)) {
					endpoint_enable(ept, 1);
					setup_ack();
					return;
				}
			}
		}
		break;
	}
	}

	// dprintf(INFO, "STALL %s %d %d %d %d %d\n",
	// 	reqname(s.request),
	// 	s.type, s.request, s.value, s.index, s.length);

 stall:
     
	writel((1<<16) | (1 << 0), USB_ENDPTCTRL(ept->num));    
    Print("test");
}

UINTN ulpi_read(UINTN reg)
{
        /* initiate read operation */
	writel(ULPI_RUN | ULPI_READ | ULPI_ADDR(reg),
               USB_ULPI_VIEWPORT);

        /* wait for completion */
         
	 while(MmioRead32(USB_ULPI_VIEWPORT) & ULPI_RUN) ;
    
	return ULPI_DATA_READ(MmioRead32(USB_ULPI_VIEWPORT));
}

VOID ulpi_write(UINTN val, UINTN reg)
{
        /* initiate write operation */

 

	writel(ULPI_RUN | ULPI_WRITE | ULPI_ADDR(reg) | ULPI_DATA(val),USB_ULPI_VIEWPORT);

        /* wait for completion */
         
	while(MmioRead32(USB_ULPI_VIEWPORT) & ULPI_RUN) ;
}

#define USB_CLK             0x00902910
#define USB_PHY_CLK         0x00902E20
#define CLK_RESET_ASSERT    0x1
#define CLK_RESET_DEASSERT  0x0
#define CLK_RESET(x,y)  writel((y), (x));

static UINTN msm_otg_xceiv_reset()
{
	CLK_RESET(USB_CLK, CLK_RESET_ASSERT);
	CLK_RESET(USB_PHY_CLK, CLK_RESET_ASSERT);
	MicroSecondDelay(2000);
	CLK_RESET(USB_PHY_CLK, CLK_RESET_DEASSERT);
	CLK_RESET(USB_CLK, CLK_RESET_DEASSERT);
	MicroSecondDelay(2000);

	/* select ULPI phy */
     
	writel(0x81000000, USB_PORTSC);
	return 0;
}

VOID board_usb_init(VOID);
VOID board_ulpi_init(VOID);

UINTN udc_init(struct udc_device *dev) 
{

	epts = AllocatePool(4096);

	//dprintf(INFO, "USB init ept @ %p\n", epts);
	memset(epts, 0, 32 * sizeof(struct ept_queue_head));

	//dprintf(INFO, "USB ID %08x\n", MmioRead32(USB_ID));
//    board_usb_init();

        /* select ULPI phy */
    //todo replace with mmiowrite32
	writel(0x81000000, USB_PORTSC);

        /* RESET */
//todo replace with mmiowrite32
writel(0x00080002, USB_USBCMD);



//tofo replace with mmioread32 ?=???    
	writel((UINTN) epts, MSM_USB_BASE + 0x0158);

        /* select DEVICE mode */
	writel(0x02, USB_USBMODE);

	writel(0xffffffff, USB_ENDPTFLUSH);

	ep0out = _udc_endpoint_alloc(0, 0, 64);
	ep0in = _udc_endpoint_alloc(0, 1, 64);
	ep0req = udc_request_alloc();
	//ep0req->buf = malloc(4096);
   ep0req->buf=  AllocatePool( sizeof(  ep0req->buf ) );
	{
		/* create and register a language table descriptor */
		/* language 0x0409 is US English */
		struct udc_descriptor *desc = udc_descriptor_alloc(TYPE_STRING, 0, 4);
		desc->data[2] = 0x09;
		desc->data[3] = 0x04;
		udc_descriptor_register(desc);
	}
	
	the_device = dev;
	return 0;
}

enum handler_return udc_interrupt(VOID *arg)
{
	struct udc_endpoint *ept;
	UINTN ret = 0;
	UINTN n = MmioRead32(USB_USBSTS);
	//tofo replace with mmioread32 ?=???
    writel(n, USB_USBSTS);
    
	n &= (STS_SLI | STS_URI | STS_PCI | STS_UI | STS_UEI);

	if (n == 0)
		return INT_NO_RESCHEDULE;

	if (n & STS_URI) {
		writel(MmioRead32(USB_ENDPTCOMPLETE), USB_ENDPTCOMPLETE);
		writel(MmioRead32(USB_ENDPTSETUPSTAT), USB_ENDPTSETUPSTAT);
		writel(0xffffffff, USB_ENDPTFLUSH);
		writel(0, USB_ENDPTCTRL(1));
		//DBG1("-- reset --\n");
		usb_online = 0;
		usb_config_value = 0;
		the_gadget->notify(the_gadget, UDC_EVENT_OFFLINE);

		/* error out any pending reqs */
		for (ept = ept_list; ept; ept = ept->next) {
			/* ensure that ept_complete considers
			 * this to be an error state
			 */
			if (ept->req) {
				ept->req->item->info = INFO_HALTED;
				handle_ept_complete(ept);
			}
		}
		usb_status(0, usb_highspeed);
	}
	if (n & STS_SLI) {
		//DBG1("-- suspend --\n");
	}
	if (n & STS_PCI) {
		//DBG1("-- portchange --\n");
		UINTN spd = (MmioRead32(USB_PORTSC) >> 26) & 3;
		if(spd == 2) {
			usb_highspeed = 1;
		} else {
			usb_highspeed = 0;
		}
	}
	if (n & STS_UEI) {
		// dprintf(INFO, "<UEI %x>\n", MmioRead32(USB_ENDPTCOMPLETE));
	}
#if 0
	DBG("STS: ");
	if (n & STS_UEI) DBG("ERROR ");
	if (n & STS_SLI) DBG("SUSPEND ");
	if (n & STS_URI) DBG("RESET ");
	if (n & STS_PCI) DBG("PORTCHANGE ");
	if (n & STS_UI) DBG("USB ");
	DBG("\n");
#endif
	if ((n & STS_UI) || (n & STS_UEI)) {
		n = MmioRead32(USB_ENDPTSETUPSTAT);
		if (n & EPT_RX(0)) {
			handle_setup(ep0out);
			//ret = INT_RESCHEDULE;
		}

		n = MmioRead32(USB_ENDPTCOMPLETE);
		if (n != 0) {
			writel(n, USB_ENDPTCOMPLETE);
		}

		for (ept = ept_list; ept; ept = ept->next){
			if (n & ept->bit) {
				handle_ept_complete(ept);
				//ret = INT_RESCHEDULE;
			}
		}
	}
	return INT_NO_RESCHEDULE;
}

UINTN udc_register_gadget(struct udc_gadget *gadget)
{
    
    DEBUG((EFI_D_ERROR, "udc_register_gadget()\n"));
	if (the_gadget) {
		// dprintf(CRITICAL, "only one gadget supported\n");
         DEBUG((EFI_D_ERROR, "only one gadget supported\n"));
		return -1;
	}
	the_gadget = gadget;
	return 0;
}

static VOID udc_ept_desc_fill(struct udc_endpoint *ept, char *data)
{
	data[0] = 7;
	data[1] = 5;
	data[2] = ept->num | (ept->in ? 0x80 : 0x00);
	data[3] = 0x02; /* bulk -- the only kind we support */
	data[4] = ept->maxpkt;
	data[5] = ept->maxpkt >> 8;
	data[6] = ept->in ? 0x00 : 0x01;
}

static UINTN udc_ifc_desc_size(struct udc_gadget *g)
{
	return 9 + g->ifc_endpoints * 7;
}

static VOID udc_ifc_desc_fill(struct udc_gadget *g, char *data)
{
	UINTN n;

	data[0] = 0x09;
	data[1] = TYPE_INTERFACE;
	data[2] = 0x00; /* ifc number */
	data[3] = 0x00; /* alt number */
	data[4] = g->ifc_endpoints;
	data[5] = g->ifc_class;
	data[6] = g->ifc_subclass;
	data[7] = g->ifc_protocol;
	data[8] = udc_string_desc_alloc(g->ifc_string);

	data += 9;
	for (n = 0; n < g->ifc_endpoints; n++) {
		udc_ept_desc_fill(g->ept[n], data);
		data += 7;
	}
}

UINTN udc_start(EFI_HARDWARE_INTERRUPT_PROTOCOL *gInterupt)
{
	struct udc_descriptor *desc;
	 char *data;
	UINTN size;

	// dprintf(ALWAYS, "udc_start()\n");
    DEBUG((EFI_D_ERROR, "udc_start()\n"));

	if (!the_device) {
		// dprintf(CRITICAL, "udc cannot start before init\n");
        DEBUG((EFI_D_ERROR, "udc cannot start before init\n"));
		return -1;
	}
	
	if (!the_gadget) {
	// 	// dprintf(CRITICAL, "udc has no gadget registered\n");
         DEBUG((EFI_D_ERROR, "udc has no gadget registered\n"));
	 	return -1;
	}

	/* create our device descriptor */
	desc = udc_descriptor_alloc(TYPE_DEVICE, 0, 18);
	data = desc->data;
	data[2] = 0x10; /* usb spec rev 2.10 */
	data[3] = 0x02;
	data[4] = 0x00; /* class */
	data[5] = 0x00; /* subclass */
	data[6] = 0x00; /* protocol */
	data[7] = 0x40; /* max packet size on ept 0 */
	memcpy(data + 8, &the_device->vendor_id, sizeof(short));
	memcpy(data + 10, &the_device->product_id, sizeof(short));
	memcpy(data + 12, &the_device->version_id, sizeof(short));
	data[14] = udc_string_desc_alloc(the_device->manufacturer);
	data[15] = udc_string_desc_alloc(the_device->product);
	// char* deviceSerial = generate_serial_from_cid(device_cid);
	// dprintfr(INFO, "device serial %s", deviceSerial);
	// data[16] = udc_string_desc_alloc(deviceSerial);
	data[17] = 1; /* number of configurations */
	udc_descriptor_register(desc);

	/* create our configuration descriptor */
	size = 9 + udc_ifc_desc_size(the_gadget);
	desc = udc_descriptor_alloc(TYPE_CONFIGURATION, 0, size);
	data = desc->data;
	data[0] = 0x09;
	data[2] = size;
	data[3] = size >> 8;
	data[4] = 0x01; /* number of UINTNerfaces */
	data[5] = 0x01; /* configuration value */
	data[6] = 0x00; /* configuration string */
	data[7] = 0x80; /* attributes */
	data[8] = 0x80; /* max power (250ma) -- todo fix this */
	udc_ifc_desc_fill(the_gadget, data + 9);
	udc_descriptor_register(desc);

        /* go to RUN mode (D+ pullup enable) */
	MmioWrite32(USB_USBCMD, 0x00080001);
    gInterupt->RegisterInterruptSource(gInterupt,INT_USB_HS,(VOID*) 0);
	//register_int_handler(INT_USB_HS, udc_interrupt, (VOID*) 0);
	//unmask_interrupt(INT_USB_HS);
    gInterupt->EnableInterruptSource(gInterupt, INT_USB_HS);
	writel(STS_URI | STS_SLI | STS_UI | STS_PCI, USB_USBINTR);
	return 0;
}

UINTN udc_stop(EFI_HARDWARE_INTERRUPT_PROTOCOL *gInterupt)
{
	UINTN val;
    writel(0, USB_USBINTR);
	//mask_interrupt(INT_USB_HS);
    gInterupt->DisableInterruptSource(gInterupt,INT_USB_HS);

        /* disable pullup */
	writel(0x00080000, USB_USBCMD);

	return 0;
}

VOID usb_stop_charging(UINTN stop_charging)
{
    ENABLE_CHARGING = !stop_charging;
}

static inline UINTN is_usb_charging(VOID)
{
    return ENABLE_CHARGING;
}

VOID usb_charger_reset(VOID)
{
    usb_stop_charging(TRUE);
    charger_usb_disconnected();
}

/* Charger detection code
 * Set global flags WALL_CHARGER and
 * RETURN: type of charger connected
 * CHG_WALL
 * CHG_HOST_PC
 * */
UINTN usb_chg_detect_type(VOID)
{
    UINTN ret = CHG_UNDEFINED;

    if ((MmioRead32(USB_PORTSC) & PORTSC_LS) == PORTSC_LS)
    {
        if(charger_usb_is_charger_connected() == TRUE) {
            WALL_CHARGER = TRUE;
            HOST_CHARGER = FALSE;
            charger_usb_i(1500);
            ret = CHG_WALL;
        }
    }
    else
    {
        if(charger_usb_is_pc_connected() == TRUE) {
            WALL_CHARGER = FALSE;
            HOST_CHARGER = TRUE;
            ret = CHG_HOST_PC;
        }
    }
    return ret;
}

/* check if USB cable is connected
 *
 * RETURN: If cable connected return 1
 * If cable disconnected return 0
 */
UINTN is_usb_cable_connected(VOID)
{
    /*Verify B Session Valid Bit to verify vbus status*/
    if (B_SESSION_VALID & MmioRead32(USB_OTGSC)) {
        return 1;
    } else {
        return 0;
    }
}

/* check for USB connection assuming USB is not pulled up.
 * It looks for suspend state bit in PORTSC register.
 *
 * RETURN: If cable connected return 1
 * If cable disconnected return 0
 */

UINTN usb_cable_status(EFI_HARDWARE_INTERRUPT_PROTOCOL *gInterupt)
{
    UINTN ret = 0;
    /*Verify B Session Valid Bit to verify vbus status*/
    writel(0x00080001, USB_USBCMD);

    /*Check reset value of suspend state bit*/
    if (!((1<<7) & MmioRead32(USB_PORTSC))) {
        ret=1;
    }
    udc_stop(gInterupt);
    return ret;
}

VOID usb_charger_change_state(VOID)
{
    UINTN usb_connected;

   //User might have switched from host pc to wall charger. So keep checking
   //every time we are in the loop

   if(ENABLE_CHARGING == TRUE)
   {
      usb_connected = is_usb_cable_connected();

      if(usb_connected && !charger_connected)
      {
	//mdelay(20);

         /* go to RUN mode (D+ pullup enable) */
         writel(0x00080001, USB_USBCMD);
         //mdelay(10);

         usb_chg_detect_type();
         charger_connected = TRUE;
      }
      else if(!usb_connected && charger_connected)
      {
         /* disable D+ pull-up */
         writel(0x00080000, USB_USBCMD);

         /* Applicable only for 8k target */
         /*USB Spoof Disconnect Failure
           Symptoms:
           In USB peripheral mode, writing '0' to Run/Stop bit of the
           USBCMD register doesn't cause USB disconnection (spoof disconnect).
           The PC host doesn't detect the disconnection and the phone remains
           active on Windows device manager.

           Suggested Workaround:
           After writing '0' to Run/Stop bit of USBCMD, also write 0x48 to ULPI
           "Function Control" register. This can be done via the ULPI VIEWPORT
           register (offset 0x170) by writing a value of 0x60040048.
          */
         ulpi_write(0x48, 0x04);
	 //usb_charger_reset();
         WALL_CHARGER = FALSE;
         HOST_CHARGER = FALSE;
         charger_usb_i(0);
         charger_usb_disconnected();
         charger_connected = FALSE;
      }
      if(WALL_CHARGER == TRUE || HOST_CHARGER == TRUE){
	//battery_charging_image();
      }
   }
   else if ((MmioRead32(USB_USBCMD) & 0x01) == 0){
      writel(0x00080001, USB_USBCMD);
   }
}