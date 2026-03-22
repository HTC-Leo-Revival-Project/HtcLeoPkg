#include <PiDxe.h>
#include <Uefi.h>

#include <Library/LKEnvLib.h>

#include <Library/KeypadDeviceHelperLib.h>
#include <Library/KeypadDeviceImplLib.h>
#include <Library/UefiLib.h>
#include <Protocol/KeypadDevice.h>
#include <Protocol/GpioTlmm.h>

#include <Device/Gpio.h>
#include <Device/DeviceType.h>

#if KP_LED_ENABLE_METHOD == 2

#include <Device/microp.h>
#include <Protocol/HtcLeoMicroP.h>

// Cached copy of the MicroP protocol instance
HTCLEO_MICROP_PROTOCOL *gMicroP = NULL;

#endif

// Cached copy of the Hardware Gpio protocol instance
TLMM_GPIO *gGpio = NULL;

// Global variables
EFI_EVENT m_CallbackTimer = NULL;
EFI_EVENT EfiExitBootServicesEvent = (EFI_EVENT)NULL;
BOOLEAN timerRunning = FALSE;

typedef enum {
  KEY_DEVICE_TYPE_UNKNOWN,
  KEY_DEVICE_TYPE_LEGACY,
  KEY_DEVICE_TYPE_KEYMATRIX,
  KEY_DEVICE_TYPE_TLMM,
  KEY_DEVICE_TYPE_PM8X41,
  KEY_DEVICE_TYPE_PM8X41_PON,
} KEY_DEVICE_TYPE;

typedef struct {
  KEY_CONTEXT     EfiKeyContext;
  BOOLEAN         IsValid;
  KEY_DEVICE_TYPE DeviceType;

  // gpio
  UINT8   Gpio;
  BOOLEAN ActiveLow;

  // gpio keymatrix
  UINT8 GpioIn;
  UINT8 GpioOut;

  // pon
  BOOLEAN EnableKeyPadLed;
} KEY_CONTEXT_PRIVATE;

STATIC KEY_CONTEXT_PRIVATE KeyContextPower;
STATIC KEY_CONTEXT_PRIVATE KeyContextVolumeUp;
STATIC KEY_CONTEXT_PRIVATE KeyContextVolumeDown;
STATIC KEY_CONTEXT_PRIVATE KeyContextBack;
STATIC KEY_CONTEXT_PRIVATE KeyContextWindows;
STATIC KEY_CONTEXT_PRIVATE KeyContextHome;
STATIC KEY_CONTEXT_PRIVATE KeyContextDial;

STATIC KEY_CONTEXT_PRIVATE *KeyList[] = {
    &KeyContextPower, &KeyContextVolumeUp, &KeyContextVolumeDown,
    &KeyContextBack,  &KeyContextWindows,  &KeyContextHome,
    &KeyContextDial};

STATIC
VOID KeypadInitializeKeyContextPrivate(KEY_CONTEXT_PRIVATE *Context)
{
  Context->IsValid     = FALSE;
  Context->Gpio        = 0;
  Context->GpioOut     = 0;
  Context->GpioIn      = 0;
  Context->DeviceType  = KEY_DEVICE_TYPE_UNKNOWN;
  Context->ActiveLow   = FALSE;
  Context->EnableKeyPadLed = FALSE;
}

STATIC
KEY_CONTEXT_PRIVATE *KeypadKeyCodeToKeyContext(UINT32 KeyCode)
{
  if (KeyCode == 114)
    return &KeyContextVolumeDown;
  else if (KeyCode == 115)
    return &KeyContextVolumeUp;
  else if (KeyCode == 116)
    return &KeyContextPower;
  else if (KeyCode == 117)
    return &KeyContextBack;
  else if (KeyCode == 118)
    return &KeyContextWindows;
  else if (KeyCode == 119)
    return &KeyContextHome;
  else if (KeyCode == 120)
    return &KeyContextDial;
  else
    return NULL;
}

VOID
EFIAPI
ExitBootServicesEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  // Make sure the LED is disabled
#if KP_LED_ENABLE_METHOD == 2
  gMicroP->KpLedSetBrightness(0);
#else
  gGpio->Set(HTCLEO_GPIO_KP_LED, 0);
#endif
}

RETURN_STATUS
EFIAPI
KeypadDeviceImplConstructor(VOID)
{
  UINTN                Index;
  EFI_STATUS           Status = EFI_SUCCESS;
  KEY_CONTEXT_PRIVATE *StaticContext;
  DeviceType           Device;

  // Find the gpio controller protocol.  ASSERT if not found.
  Status = gBS->LocateProtocol (&gTlmmGpioProtocolGuid, NULL, (VOID **)&gGpio);
  ASSERT_EFI_ERROR (Status);
#if DEVICETYPE == 1
  Device = LEO;
#endif
#if DEVICETYPE == 2
  Device = SCHUBERT;
#endif
#if DEVICETYPE == 3
  Device = BRAVO;
#endif
#if DEVICETYPE== 4
  Device = PASSION;
#endif
#if KP_LED_ENABLE_METHOD == 2
  // Find the MicroP protocol. ASSERT if not found.
  Status = gBS->LocateProtocol(&gHtcLeoMicropProtocolGuid, NULL, (VOID **)&gMicroP);
  ASSERT_EFI_ERROR(Status);
#endif

  // Reset all keys
  for (Index = 0; Index < (sizeof(KeyList) / sizeof(KeyList[0])); Index++) {
    KeypadInitializeKeyContextPrivate(KeyList[Index]);
  }

switch(Device){
  case SCHUBERT:
  // volume up button
    StaticContext                  = KeypadKeyCodeToKeyContext(115);
    StaticContext->DeviceType      = KEY_DEVICE_TYPE_LEGACY;
    StaticContext->Gpio            = 94;
    StaticContext->ActiveLow       = 0x1 & 0x1;
    StaticContext->EnableKeyPadLed = FALSE;
    StaticContext->IsValid         = TRUE;

  // volume down button
    StaticContext                  = KeypadKeyCodeToKeyContext(114);
    StaticContext->DeviceType      = KEY_DEVICE_TYPE_LEGACY;
    StaticContext->Gpio            = 41;
    StaticContext->ActiveLow       = 0x1 & 0x1;
    StaticContext->EnableKeyPadLed = FALSE;
    StaticContext->IsValid         = TRUE;

    // camera button
    StaticContext                  = KeypadKeyCodeToKeyContext(120);
    StaticContext->DeviceType      = KEY_DEVICE_TYPE_LEGACY;
    StaticContext->Gpio            = 90;
    StaticContext->ActiveLow       = 0x1 & 0x1;
    StaticContext->EnableKeyPadLed = FALSE;
    StaticContext->IsValid         = TRUE;

    // power button
    StaticContext                  = KeypadKeyCodeToKeyContext(118);
    StaticContext->DeviceType      = KEY_DEVICE_TYPE_LEGACY;
    StaticContext->Gpio            = 42;
    StaticContext->ActiveLow       = 0x1 & 0x1;
    StaticContext->EnableKeyPadLed = TRUE;
    StaticContext->IsValid         = TRUE;
    break;
  case LEO:
  // power button
    StaticContext                  = KeypadKeyCodeToKeyContext(116);
    StaticContext->DeviceType      = KEY_DEVICE_TYPE_LEGACY;
    StaticContext->Gpio            = 94;
    StaticContext->ActiveLow       = 0x1 & 0x1;
    StaticContext->EnableKeyPadLed = TRUE;
    StaticContext->IsValid         = TRUE;

    // back button
    StaticContext                  = KeypadKeyCodeToKeyContext(117);
    StaticContext->DeviceType      = KEY_DEVICE_TYPE_KEYMATRIX;
    StaticContext->GpioOut         = 31;
    StaticContext->GpioIn          = 42;
    StaticContext->ActiveLow       = 0x1 & 0x1;
    StaticContext->EnableKeyPadLed = TRUE;
    StaticContext->IsValid         = TRUE;

  // windows button
    StaticContext                  = KeypadKeyCodeToKeyContext(118);
    StaticContext->DeviceType      = KEY_DEVICE_TYPE_KEYMATRIX;
    StaticContext->GpioOut         = 32;
    StaticContext->GpioIn          = 42;
    StaticContext->ActiveLow       = 0x1 & 0x1;
    StaticContext->EnableKeyPadLed = TRUE;
    StaticContext->IsValid         = TRUE;

    // home button
    StaticContext                  = KeypadKeyCodeToKeyContext(119);
    StaticContext->DeviceType      = KEY_DEVICE_TYPE_KEYMATRIX;
    StaticContext->GpioOut         = 31;
    StaticContext->GpioIn          = 41;
    StaticContext->ActiveLow       = 0x1 & 0x1;
    StaticContext->EnableKeyPadLed = TRUE;
    StaticContext->IsValid         = TRUE;
  case BRAVO:
    //bravo has no support for its capative touch buttons yet
    break;
  case PASSION:
    //passion has no support for its capative touch buttons yet
    break;
}
if (Device == LEO || Device == BRAVO || Device == PASSION){
          // Common "Enter" Button (trackball/pad on passion/bravo and dial button on leo)
          StaticContext             = KeypadKeyCodeToKeyContext(120);
          StaticContext->DeviceType = KEY_DEVICE_TYPE_KEYMATRIX;
          StaticContext->GpioOut    = 32;
          StaticContext->GpioIn     = 41;
          StaticContext->ActiveLow  = 0x1 & 0x1;
          StaticContext->EnableKeyPadLed = TRUE;
          StaticContext->IsValid    = TRUE;

            // volume up button
          StaticContext              = KeypadKeyCodeToKeyContext(115);
          StaticContext->DeviceType  = KEY_DEVICE_TYPE_KEYMATRIX;
          StaticContext->GpioOut     = 33;
          StaticContext->GpioIn      = 42;
          StaticContext->ActiveLow   = 0x1 & 0x1;
          StaticContext->EnableKeyPadLed = FALSE;
          StaticContext->IsValid     = TRUE;

          // volume down button
          StaticContext              = KeypadKeyCodeToKeyContext(114);
          StaticContext->DeviceType  = KEY_DEVICE_TYPE_KEYMATRIX;
          StaticContext->GpioOut     = 33;
          StaticContext->GpioIn      = 41;
          StaticContext->ActiveLow   = 0x1 & 0x1;
          StaticContext->EnableKeyPadLed = FALSE;
          StaticContext->IsValid     = TRUE;
}

  // Register for ExitBootServicesEvent
  Status = gBS->CreateEvent (
             EVT_SIGNAL_EXIT_BOOT_SERVICES,
             TPL_NOTIFY,
             ExitBootServicesEvent,
             NULL,
             &EfiExitBootServicesEvent);

  return Status;
}

EFI_STATUS EFIAPI KeypadDeviceImplReset(KEYPAD_DEVICE_PROTOCOL *This)
{
  LibKeyInitializeKeyContext(&KeyContextPower.EfiKeyContext);
  KeyContextPower.EfiKeyContext.KeyData.Key.ScanCode = SCAN_ESC;

  LibKeyInitializeKeyContext(&KeyContextVolumeUp.EfiKeyContext);
  KeyContextVolumeUp.EfiKeyContext.KeyData.Key.ScanCode = SCAN_UP;

  LibKeyInitializeKeyContext(&KeyContextVolumeDown.EfiKeyContext);
  KeyContextVolumeDown.EfiKeyContext.KeyData.Key.ScanCode = SCAN_DOWN;

  LibKeyInitializeKeyContext(&KeyContextBack.EfiKeyContext);
  KeyContextBack.EfiKeyContext.KeyData.Key.UnicodeChar = CHAR_BACKSPACE;

  LibKeyInitializeKeyContext(&KeyContextWindows.EfiKeyContext);
  KeyContextWindows.EfiKeyContext.KeyData.Key.UnicodeChar = CHAR_TAB;

  LibKeyInitializeKeyContext(&KeyContextHome.EfiKeyContext);
  KeyContextHome.EfiKeyContext.KeyData.Key.ScanCode = SCAN_HOME;

  LibKeyInitializeKeyContext(&KeyContextDial.EfiKeyContext);
  KeyContextDial.EfiKeyContext.KeyData.Key.UnicodeChar = CHAR_CARRIAGE_RETURN;

  return EFI_SUCCESS;
}

// Callback function to turn off the LED after a certain time
VOID EFIAPI DisableKeyPadLed(IN EFI_EVENT Event, IN VOID *Context)
{
#if KP_LED_ENABLE_METHOD == 2
  // Disable keypad LED brightness
  gMicroP->KpLedSetBrightness(0);
#else
  // Disable the GPIO
  gGpio->Set(HTCLEO_GPIO_KP_LED, 0);
#endif
  timerRunning = FALSE;
}

// Function to enable the LED and schedule the callback
VOID EnableKeypadLedWithTimer(VOID)
{
    if (timerRunning) {
        // Cancel the running timer
        gBS->SetTimer(m_CallbackTimer, TimerCancel, 0);
        timerRunning = FALSE;
    }
#if KP_LED_ENABLE_METHOD == 2
    gMicroP->KpLedSetBrightness(255);
#else
    gGpio->Set(HTCLEO_GPIO_KP_LED, 1);
#endif
    EFI_STATUS Status;

    Status = gBS->CreateEvent(
        EVT_NOTIFY_SIGNAL | EVT_TIMER, TPL_CALLBACK, DisableKeyPadLed, NULL,
        &m_CallbackTimer);

    ASSERT_EFI_ERROR(Status);

    Status = gBS->SetTimer(
        m_CallbackTimer, TimerRelative, EFI_TIMER_PERIOD_MILLISECONDS(5000));

    ASSERT_EFI_ERROR(Status);

    timerRunning = TRUE;
}

EFI_STATUS KeypadDeviceImplGetKeys(
    KEYPAD_DEVICE_PROTOCOL *This, KEYPAD_RETURN_API *KeypadReturnApi,
    UINT64 Delta)
{
    UINT8 GpioStatus;
    BOOLEAN IsPressed;
    UINTN Index;

    for (Index = 0; Index < (sizeof(KeyList) / sizeof(KeyList[0])); Index++) {
        KEY_CONTEXT_PRIVATE *Context = KeyList[Index];

        // check if this is a valid key
        if (Context->IsValid == FALSE)
            continue;

        // get status
        if (Context->DeviceType == KEY_DEVICE_TYPE_LEGACY) {
            // implement hd2 gpio stuff here
            GpioStatus = gGpio->Get(Context->Gpio);
        } else if (Context->DeviceType == KEY_DEVICE_TYPE_KEYMATRIX) {
            gGpio->Set(Context->GpioOut, 0);
            GpioStatus = gGpio->Get(Context->GpioIn);
            gGpio->Set(Context->GpioOut, 1);
        } else {
            continue;
        }

        // update key status
        IsPressed = (GpioStatus ? 1 : 0) ^ Context->ActiveLow;

        if (IsPressed && Context->EnableKeyPadLed) {
            EnableKeypadLedWithTimer();
        }


        LibKeyUpdateKeyStatus(
            &Context->EfiKeyContext, KeypadReturnApi, IsPressed, Delta);
    }

    return EFI_SUCCESS;
}
