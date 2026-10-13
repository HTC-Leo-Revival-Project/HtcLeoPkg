#include <Uefi.h>
#include <PiDxe.h>

#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>

#include <Chipset/gpio.h>
#include <Chipset/interrupts.h>
#include <Chipset/irqs.h>
#include <Chipset/msm_i2c.h>
#include <Library/gpio.h>

#include <Protocol/AbsolutePointer.h>
#include <Protocol/GpioTlmm.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/HtcLeoI2C.h>
#include <Protocol/HtcLeoSirc.h>
#include <Protocol/HtcLeoTouch.h>
#include <Protocol/SimplePointer.h>

#define TS_GPIO_POWER   160
#define TS_GPIO_IRQ      92
#define TS_GPIO_SEL     108
#define TS_GPIO_MULT     82
#define H2W_GPIO_CLK     27

#define TS_IRQ_BANK_BIT   (1u << (TS_GPIO_IRQ - 68))

#define TS_REPORT_LEN     9
#define TS_WATERMARK    0x5A
#define TS_CONTACT_ID   0x55
#define TS_ENABLE_CMD    0xD0

#define TS_PANEL_X      480
#define TS_PANEL_Y      800

typedef struct {
  HTCLEO_TOUCH_PROTOCOL          Touch;
  EFI_ABSOLUTE_POINTER_PROTOCOL  AbsolutePointer;
  EFI_SIMPLE_POINTER_PROTOCOL    SimplePointer;
  EFI_ABSOLUTE_POINTER_MODE      AbsoluteMode;
  EFI_SIMPLE_POINTER_MODE        SimpleMode;

  HTCLEO_TOUCH_STATUS            Status;

  HTCLEO_I2C_PROTOCOL           *I2c;
  HTCLEO_SIRC_PROTOCOL          *Sirc;
  TLMM_GPIO                     *Gpio;

  EFI_EVENT                      WaitEvent;

  UINT32                        PanelX;
  UINT32                        PanelY;
  UINT32                        RawX;
  UINT32                        RawY;
  UINT32                        Contacts;
  BOOLEAN                       Down;
  BOOLEAN                       Ready;

  BOOLEAN                       Announced;
  BOOLEAN                       ByInterrupt;

  UINT32                        LastX;
  UINT32                        LastY;
} TOUCH_DEVICE;

static TOUCH_DEVICE  *mTouch;

static
VOID
TsGpioOutput (
  IN UINTN  Pin,
  IN UINTN  On
  )
{
  mTouch->Gpio->Set (Pin, On);
}

static
UINTN
TsGpioInput (
  IN UINTN  Pin
  )
{
  return mTouch->Gpio->Get (Pin);
}

static
VOID
TsResetController (
  VOID
  )
{
  UINTN  Waited;

  TsGpioOutput (TS_GPIO_SEL,   1);
  TsGpioOutput (TS_GPIO_POWER, 0);
  TsGpioOutput (TS_GPIO_MULT,  0);
  TsGpioOutput (H2W_GPIO_CLK,  0);
  (VOID) TsGpioInput (TS_GPIO_IRQ);
  MicroSecondDelay (100 * 1000);

  TsGpioOutput (TS_GPIO_MULT,  1);
  TsGpioOutput (H2W_GPIO_CLK,  1);
  TsGpioOutput (TS_GPIO_POWER, 1);

  for (Waited = 0; Waited < 100 && TsGpioInput (TS_GPIO_IRQ) != 0; Waited++) {
    MicroSecondDelay (10 * 1000);
  }

  TsGpioOutput (TS_GPIO_SEL, 0);
  MicroSecondDelay (300 * 1000);

  DEBUG ((
    EFI_D_INFO,
    "TouchDxe: power up done, interrupt line %s after %u ms\n",
    TsGpioInput (TS_GPIO_IRQ) == 0 ? L"low" : L"high",
    (UINT32) (Waited * 10)
    ));
}

static
EFI_STATUS
TsEnableController (
  VOID
  )
{
  UINT8  Cmd[3];

  Cmd[0] = TS_ENABLE_CMD;
  Cmd[1] = 0x00;
  Cmd[2] = 0x01;

  if (mTouch->I2c->Write (HTCLEO_TOUCH_I2C_2A, Cmd, sizeof (Cmd)) != 0) {
    DEBUG ((EFI_D_ERROR, "TouchDxe: enable command failed\n"));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

static
VOID
TsArmInterrupt (
  VOID
  )
{
  MmioOr32 (GPIO_INT_EDGE_3, TS_IRQ_BANK_BIT);
  MmioAnd32 (GPIO_INT_POS_3, ~TS_IRQ_BANK_BIT);
  MmioWrite32 (GPIO_INT_CLEAR_3, TS_IRQ_BANK_BIT);
  MmioOr32 (GPIO_INT_EN_3, TS_IRQ_BANK_BIT);

  DEBUG ((
    EFI_D_INFO,
    "TouchDxe: TLMM armed, bank status 0x%x en 0x%x\n",
    MmioRead32 (GPIO_INT_STATUS_3),
    MmioRead32 (GPIO_INT_EN_3)
    ));
}

static
VOID
TsAckInterrupt (
  IN BOOLEAN  AckSirc
  )
{
  MmioWrite32 (GPIO_INT_CLEAR_3, TS_IRQ_BANK_BIT);

  if (AckSirc) {
    MmioWrite32 (SIRC_INT_ACK, 1u << HTCLEO_SIRC_GPIO_GROUP1);
  }
}

static
VOID
EFIAPI
TsWaitNotify (
  IN EFI_EVENT  Event,
  IN VOID      *Context
  )
{
}

static
VOID
EFIAPI
TsInterruptHandler (
  IN VOID  *Context
  )
{
  if ((MmioRead32 (GPIO_INT_STATUS_3) & TS_IRQ_BANK_BIT) != 0) {
    mTouch->Announced = TRUE;
    TsAckInterrupt (FALSE);

    if (mTouch->WaitEvent != NULL) {
      gBS->SignalEvent (mTouch->WaitEvent);
    }
  }

  MmioWrite32 (SIRC_INT_ACK, 1u << HTCLEO_SIRC_GPIO_GROUP1);
}

static
EFI_STATUS
TsReadReport (
  OUT UINT8  *Buf,
  IN  UINTN   Len
  )
{
  struct i2c_msg  Msg;
  INTN           Ret;

  ZeroMem (&Msg, sizeof (Msg));
  Msg.addr  = HTCLEO_TOUCH_I2C_2A;
  Msg.flags = I2C_M_RD;
  Msg.len   = (UINT16) Len;
  Msg.buf   = Buf;

  Ret = mTouch->I2c->Xfer (&Msg, 1);
  if (Ret != 1) {
    DEBUG ((EFI_D_WARN, "TouchDxe: I2C read of %u bytes failed (%d)\n", Len, Ret));
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

static
EFI_STATUS
TsReadRegister (
  IN  UINT8  Address,
  IN  UINT8  Reg,
  OUT UINT8  *Buf,
  IN  UINTN  Len
  )
{
  if (mTouch->I2c->Read (Address, Reg, Buf, Len) != 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

static
UINT32
TsDetectController (
  VOID
  )
{
  UINT8  Buf[4];

  if (!EFI_ERROR (TsReadRegister (HTCLEO_TOUCH_I2C_B8, 0x00, Buf, 1))) {
    return HTCLEO_TOUCH_TYPE_B8;
  }

  if (!EFI_ERROR (TsReadRegister (HTCLEO_TOUCH_I2C_68, 0x00, Buf, 1))) {
    return HTCLEO_TOUCH_TYPE_68;
  }

  if (!EFI_ERROR (TsReadReport (Buf, sizeof (Buf))) && Buf[0] == TS_CONTACT_ID) {
    return HTCLEO_TOUCH_TYPE_2A;
  }

  return HTCLEO_TOUCH_TYPE_UNKNOWN;
}

static
UINT32
TsScale (
  IN UINT32  Value,
  IN UINT32  Min,
  IN UINT32  Max,
  IN UINT32  Resolution
  )
{
  if (Value <= Min) {
    return 0;
  }

  if (Value >= Max) {
    return Resolution - 1;
  }

  return (Value - Min) * (Resolution - 1) / (Max - Min);
}

static
EFI_STATUS
TsPollReport (
  VOID
  )
{
  UINT8   Buf[TS_REPORT_LEN];
  UINT32  Points;
  UINT16  X;
  UINT16  Y;

  if (!mTouch->Ready) {
    return EFI_NOT_READY;
  }

  if (TsGpioInput (TS_GPIO_IRQ) != 0) {
    return EFI_NOT_READY;
  }

  if (EFI_ERROR (TsReadReport (Buf, sizeof (Buf)))) {
    return EFI_DEVICE_ERROR;
  }

  if (Buf[0] != TS_WATERMARK) {
    DEBUG ((EFI_D_WARN, "TouchDxe: no watermark, got 0x%02x\n", Buf[0]));
    return EFI_DEVICE_ERROR;
  }

  Points = (Buf[8] >> 1) & 3;
  if (Points > 2) {
    Points = 2;
  }

  X = (UINT16) (Buf[2] | (((Buf[1] >> 4) & 0x0F) << 8));
  Y = (UINT16) (Buf[3] | ((Buf[1] & 0x0F) << 8));

  mTouch->RawX         = X;
  mTouch->RawY         = Y;
  mTouch->Contacts     = Points;
  mTouch->Down         = (BOOLEAN) (Points != 0);
  mTouch->ByInterrupt  = mTouch->Announced;
  mTouch->Announced    = FALSE;
  mTouch->PanelX       = TsScale (X, HTCLEO_TOUCH_X_MIN, HTCLEO_TOUCH_X_MAX,
                                  (UINT32) mTouch->AbsoluteMode.AbsoluteMaxX);
  mTouch->PanelY       = TsScale (Y, HTCLEO_TOUCH_Y_MIN, HTCLEO_TOUCH_Y_MAX,
                                  (UINT32) mTouch->AbsoluteMode.AbsoluteMaxY);

  if (TsGpioInput (TS_GPIO_IRQ) == 0) {
    TsAckInterrupt (TRUE);
  }

  return EFI_SUCCESS;
}

static
VOID
TsRefreshStatus (
  VOID
  )
{
  mTouch->Status.ControllerType      = HTCLEO_TOUCH_TYPE_2A;
  mTouch->Status.ResolutionX          = (UINT32) mTouch->AbsoluteMode.AbsoluteMaxX + 1;
  mTouch->Status.ResolutionY          = (UINT32) mTouch->AbsoluteMode.AbsoluteMaxY + 1;
  mTouch->Status.Contacts             = mTouch->Contacts;
  mTouch->Status.RawX                 = mTouch->RawX;
  mTouch->Status.RawY                 = mTouch->RawY;
  mTouch->Status.IrqActive            = (BOOLEAN) (TsGpioInput (TS_GPIO_IRQ) == 0);
  mTouch->Status.IrqCount             = (mTouch->Sirc != NULL) ?
                                        mTouch->Sirc->IrqCount[HTCLEO_SIRC_GPIO_GROUP1] : 0;
  mTouch->Status.LastReportByInterrupt = mTouch->ByInterrupt;
}

static
EFI_STATUS
EFIAPI
TsPoll (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = TsPollReport ();

  if (Status == EFI_SUCCESS) {
    mTouch->Status.ReportCount++;
  }

  TsRefreshStatus ();

  return Status;
}

static
VOID
EFIAPI
TsGetStatus (
  OUT HTCLEO_TOUCH_STATUS  *Status
  )
{
  if (Status == NULL) {
    return;
  }

  CopyMem (Status, &mTouch->Status, sizeof (*Status));
}

static
EFI_STATUS
EFIAPI
TsAbsoluteReset (
  IN EFI_ABSOLUTE_POINTER_PROTOCOL  *This,
  IN BOOLEAN                       ExtendedVerification
  )
{
  mTouch->Down        = FALSE;
  mTouch->Announced   = FALSE;
  mTouch->ByInterrupt = FALSE;

  TsResetController ();

  return TsEnableController ();
}

static
EFI_STATUS
EFIAPI
TsAbsoluteGetState (
  IN  EFI_ABSOLUTE_POINTER_PROTOCOL  *This,
  OUT EFI_ABSOLUTE_POINTER_STATE     *State
  )
{
  EFI_STATUS  Status;

  Status = TsPollReport ();

  if (EFI_ERROR (Status) && Status != EFI_NOT_READY) {
    if (State == NULL) {
      return Status;
    }
  }

  if (State == NULL) {
    return EFI_SUCCESS;
  }

  ZeroMem (State, sizeof (*State));

  State->CurrentX      = mTouch->PanelX;
  State->CurrentY      = mTouch->PanelY;
  State->CurrentZ      = 0;
  State->ActiveButtons = mTouch->Down ? EFI_ABSP_TouchActive : 0;

  return EFI_SUCCESS;
}

static
EFI_STATUS
EFIAPI
TsSimpleReset (
  IN EFI_SIMPLE_POINTER_PROTOCOL  *This,
  IN BOOLEAN                       ExtendedVerification
  )
{
  mTouch->LastX = mTouch->PanelX;
  mTouch->LastY = mTouch->PanelY;

  mTouch->Down        = FALSE;
  mTouch->Announced   = FALSE;
  mTouch->ByInterrupt = FALSE;

  TsResetController ();

  return TsEnableController ();
}

static
EFI_STATUS
EFIAPI
TsSimpleGetState (
  IN  EFI_SIMPLE_POINTER_PROTOCOL  *This,
  OUT EFI_SIMPLE_POINTER_STATE     *State
  )
{
  EFI_STATUS  Status;
  UINT32      X;
  UINT32      Y;

  Status = TsPollReport ();
  if (EFI_ERROR (Status) && Status == EFI_DEVICE_ERROR && State == NULL) {
    return Status;
  }

  X = mTouch->PanelX;
  Y = mTouch->PanelY;

  if (State != NULL) {
    State->RelativeMovementX = (INT32) X - (INT32) mTouch->LastX;
    State->RelativeMovementY = (INT32) Y - (INT32) mTouch->LastY;
    State->LeftButton        = mTouch->Down ? TRUE : FALSE;
    State->RightButton       = FALSE;
  }

  mTouch->LastX = X;
  mTouch->LastY = Y;

  if (mTouch->SimplePointer.WaitForInput != NULL) {
    gBS->SignalEvent (mTouch->SimplePointer.WaitForInput);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
TouchDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                     Status;
  EFI_HANDLE                     Handle;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop;
  UINT32                         PanelX;
  UINT32                         PanelY;
  UINT32                         Type;

  mTouch = (TOUCH_DEVICE *) AllocateZeroPool (sizeof (TOUCH_DEVICE));
  if (mTouch == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->LocateProtocol (&gHtcLeoI2CProtocolGuid, NULL, (VOID **) &mTouch->I2c);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "TouchDxe: no I2C protocol\n"));
    goto Failed;
  }

  Status = gBS->LocateProtocol (&gTlmmGpioProtocolGuid, NULL, (VOID **) &mTouch->Gpio);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "TouchDxe: no GPIO protocol\n"));
    goto Failed;
  }

  Status = gBS->LocateProtocol (&gHtcLeoSircProtocolGuid, NULL, (VOID **) &mTouch->Sirc);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "TouchDxe: no SIRC protocol: %r\n", Status));
    mTouch->Sirc = NULL;
  }

  PanelX = TS_PANEL_X;
  PanelY = TS_PANEL_Y;

  Gop = NULL;
  Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **) &Gop);
  if (!EFI_ERROR (Status) && Gop != NULL && Gop->Mode != NULL) {
    PanelX = (UINT32) Gop->Mode->Info->HorizontalResolution;
    PanelY = (UINT32) Gop->Mode->Info->VerticalResolution;
  }

  mTouch->AbsoluteMode.AbsoluteMinX = 0;
  mTouch->AbsoluteMode.AbsoluteMinY = 0;
  mTouch->AbsoluteMode.AbsoluteMinZ = 0;
  mTouch->AbsoluteMode.AbsoluteMaxX = PanelX - 1;
  mTouch->AbsoluteMode.AbsoluteMaxY = PanelY - 1;
  mTouch->AbsoluteMode.AbsoluteMaxZ = 0;
  mTouch->AbsoluteMode.Attributes   = 0;

  mTouch->SimpleMode.ResolutionX = 1;
  mTouch->SimpleMode.ResolutionY = 1;
  mTouch->SimpleMode.ResolutionZ = 0;
  mTouch->SimpleMode.LeftButton  = TRUE;
  mTouch->SimpleMode.RightButton = FALSE;

  DEBUG ((
    EFI_D_INFO,
    "TouchDxe: controller range %u..%u x %u..%u, panel %ux%u\n",
    HTCLEO_TOUCH_X_MIN, HTCLEO_TOUCH_X_MAX, HTCLEO_TOUCH_Y_MIN,
    HTCLEO_TOUCH_Y_MAX, PanelX, PanelY
    ));

  TsResetController ();

  Type = TsDetectController ();
  DEBUG ((EFI_D_INFO, "TouchDxe: controller type %d\n", Type));
  if (Type != HTCLEO_TOUCH_TYPE_2A) {
    DEBUG ((EFI_D_ERROR, "TouchDxe: only type 2A is implemented\n"));
    Status = EFI_NOT_FOUND;
    goto Failed;
  }

  if (EFI_ERROR (TsEnableController ())) {
    Status = EFI_DEVICE_ERROR;
    goto Failed;
  }

  Status = gBS->CreateEvent (EVT_NOTIFY_SIGNAL, TPL_CALLBACK, TsWaitNotify,
                             mTouch, &mTouch->WaitEvent);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  mTouch->Status.Version = 1;
  mTouch->Touch.Poll      = TsPoll;
  mTouch->Touch.GetStatus = TsGetStatus;

  mTouch->AbsolutePointer.Reset         = TsAbsoluteReset;
  mTouch->AbsolutePointer.GetState      = TsAbsoluteGetState;
  mTouch->AbsolutePointer.WaitForInput  = mTouch->WaitEvent;
  mTouch->AbsolutePointer.Mode          = &mTouch->AbsoluteMode;

  mTouch->SimplePointer.Reset           = TsSimpleReset;
  mTouch->SimplePointer.GetState        = TsSimpleGetState;
  mTouch->SimplePointer.WaitForInput    = mTouch->WaitEvent;
  mTouch->SimplePointer.Mode            = &mTouch->SimpleMode;

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                            &Handle,
                            &gHtcLeoTouchProtocolGuid, &mTouch->Touch,
                            &gEfiAbsolutePointerProtocolGuid, &mTouch->AbsolutePointer,
                            &gEfiSimplePointerProtocolGuid, &mTouch->SimplePointer,
                            NULL
                            );
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  mTouch->Ready = TRUE;
  TsRefreshStatus ();
  TsArmInterrupt ();

  if (mTouch->Sirc != NULL) {
    Status = mTouch->Sirc->RegisterLine (
                             mTouch->Sirc,
                             HTCLEO_SIRC_GPIO_GROUP1,
                             HTCLEO_SIRC_LEVEL | HTCLEO_SIRC_POSITIVE,
                             TsInterruptHandler,
                             mTouch
                             );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        EFI_D_ERROR,
        "TouchDxe: no GPIO group interrupt (%r), reading the pin instead\n",
        Status
        ));
    }
  }

  DEBUG ((EFI_D_ERROR, "TouchDxe: ready\n"));

  return EFI_SUCCESS;

Failed:
  FreePool (mTouch);
  mTouch = NULL;

  return Status;
}
