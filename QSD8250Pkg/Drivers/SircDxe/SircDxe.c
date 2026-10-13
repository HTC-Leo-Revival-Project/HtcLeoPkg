/**
  Secondary interrupt controller (SIRC) driver.

  Copyright (c) 2026.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>

#include <Chipset/interrupts.h>
#include <Chipset/irqs.h>

#include <Protocol/HardwareInterrupt.h>
#include <Protocol/HtcLeoSirc.h>

typedef VOID (EFIAPI *SIRC_LINE_HANDLER) (VOID *Context);

typedef struct {
  SIRC_LINE_HANDLER  Handler;
  VOID              *Context;
} SIRC_LINE;

static
EFI_STATUS
EFIAPI
SircRegisterLine (
  IN HTCLEO_SIRC_PROTOCOL  *This,
  IN UINT32                 Line,
  IN UINT32                 Flags,
  IN VOID                  *Handler,
  IN VOID                  *Context
  );

static EFI_HARDWARE_INTERRUPT_PROTOCOL  *mInterrupt;
static SIRC_LINE                        mLine[HTCLEO_SIRC_NUM_LINES];

static HTCLEO_SIRC_PROTOCOL  mSircProtocol = {
  SircRegisterLine,
  { 0 }
};

static
VOID
EFIAPI
SircInterruptHandler (
  IN HARDWARE_INTERRUPT_SOURCE  Source,
  IN EFI_SYSTEM_CONTEXT         SystemContext
  )
{
  UINT32  Status;
  UINT32  Line;
  UINT32  Ack;

  Status = MmioRead32 (SIRC_IRQ0_STATUS);
  if (Status == 0) {
    DEBUG ((EFI_D_WARN, "SircDxe: interrupt but nothing pending\n"));
    return;
  }

  Ack = 0;
  for (Line = 0; Line < HTCLEO_SIRC_NUM_LINES; Line++) {
    if ((Status & (1u << Line)) == 0) {
      continue;
    }

    if (mLine[Line].Handler != NULL) {
      mSircProtocol.IrqCount[Line]++;
      mLine[Line].Handler (mLine[Line].Context);
      Ack |= 1u << Line;
    }
  }

  if (Ack != 0) {
    MmioWrite32 (SIRC_INT_ACK, Ack);
  }
}

static
EFI_STATUS
EFIAPI
SircRegisterLine (
  IN HTCLEO_SIRC_PROTOCOL  *This,
  IN UINT32                 Line,
  IN UINT32                 Flags,
  IN VOID                  *Handler,
  IN VOID                  *Context
  )
{
  UINT32  Bit;

  if (Line >= HTCLEO_SIRC_NUM_LINES) {
    return EFI_OUT_OF_RESOURCES;
  }

  Bit = 1u << Line;

  if (Handler == NULL) {
    mLine[Line].Handler = NULL;
    mLine[Line].Context = NULL;
    MmioWrite32 (SIRC_INT_ENCLEAR, Bit);
    return EFI_SUCCESS;
  }

  mLine[Line].Handler = (SIRC_LINE_HANDLER) Handler;
  mLine[Line].Context = Context;

  MmioWrite32 (SIRC_INT_ENCLEAR, Bit);

  if ((Flags & HTCLEO_SIRC_EDGE) != 0) {
    MmioOr32 (SIRC_INT_TYPE, Bit);
  } else {
    MmioAnd32 (SIRC_INT_TYPE, ~Bit);
  }

  if ((Flags & HTCLEO_SIRC_NEGATIVE) != 0) {
    MmioOr32 (SIRC_INT_POLARITY, Bit);
  } else {
    MmioAnd32 (SIRC_INT_POLARITY, ~Bit);
  }

  MmioWrite32 (SIRC_INT_ENSET, Bit);

  DEBUG ((
    EFI_D_INFO,
    "SircDxe: line %u claimed (%a, %a)\n",
    Line,
    (Flags & HTCLEO_SIRC_EDGE) ? "edge" : "level",
    (Flags & HTCLEO_SIRC_NEGATIVE) ? "active low" : "active high"
    ));

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SircDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  Status = gBS->LocateProtocol (&gHardwareInterruptProtocolGuid, NULL, (VOID **) &mInterrupt);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SircDxe: no hardware interrupt protocol\n"));
    return Status;
  }

  MmioWrite32 (SIRC_INT_SELECT, 0);
  MmioWrite32 (SIRC_INT_TYPE, 0);
  MmioWrite32 (SIRC_INT_POLARITY, 0);
  MmioWrite32 (SIRC_INT_ENABLE, 0);

  Status = mInterrupt->RegisterInterruptSource (mInterrupt, INT_SIRC_0, SircInterruptHandler);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SircDxe: cannot claim INT_SIRC_0 (%r)\n", Status));
    return Status;
  }

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                            &Handle,
                            &gHtcLeoSircProtocolGuid, &mSircProtocol,
                            NULL
                            );
  if (EFI_ERROR (Status)) {
    mInterrupt->RegisterInterruptSource (mInterrupt, INT_SIRC_0, NULL);
    return Status;
  }

  DEBUG ((EFI_D_ERROR, "SircDxe: ready\n"));

  return EFI_SUCCESS;
}
