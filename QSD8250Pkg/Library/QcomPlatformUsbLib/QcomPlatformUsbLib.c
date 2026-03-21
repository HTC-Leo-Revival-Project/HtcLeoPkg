#include <Base.h>
#include <Library/LKEnvLib.h>
//#include <Library/QcomBoardLib.h>
#include <Library/QcomPlatformUsbLib.h>
#include <Library/pcom_clients.h>
//#include <Target/board.h>

/* Do target specific usb initialization */
STATIC VOID target_usb_init(target_usb_iface_t *iface)
{
  //msm_hsusb_init(&htcleo_hsusb_pdata);
}

STATIC VOID hsusb_clock_init(target_usb_iface_t *iface)
{
    pcom_enable_hsusb_clk();
}

EFI_STATUS LibQcomPlatformUsbGetInterface(target_usb_iface_t *iface)
{
  iface->controller = L"ci";
  iface->usb_init   = target_usb_init;
  iface->clock_init = hsusb_clock_init;

  return EFI_SUCCESS;
}