/*
 * Copyright (c) 2008, Google Inc.
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
 *  * Neither the name of Google, Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/IoLib.h>
#include <Library/InterruptsLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/HardwareInterrupt.h>

#include <Chipset/irqs.h>
#include <Chipset/iomap.h>
#include <Chipset/interrupts.h>

typedef struct {
  UINTN                 Vector;
  int_handler           Handler;
  VOID                  *Arg;
  LIST_ENTRY            Link;
} HANDLER_ENTRY;

STATIC EFI_HARDWARE_INTERRUPT_PROTOCOL *mInterrupt = NULL;
STATIC LIST_ENTRY mHandlers = INITIALIZE_LIST_HEAD_VARIABLE(mHandlers);

STATIC HANDLER_ENTRY* GetInterruptHandlerEntry (UINTN Vector)
{
  LIST_ENTRY        *Link;
  HANDLER_ENTRY     *Entry;
  EFI_TPL        OriginalTPL;

  OriginalTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  for (Link = mHandlers.ForwardLink; Link != &mHandlers; Link = Link->ForwardLink) {
    Entry = BASE_CR (Link, HANDLER_ENTRY, Link);
    if (Entry->Vector == Vector) {
      gBS->RestoreTPL (OriginalTPL);
      return Entry;
    }
  }

  gBS->RestoreTPL (OriginalTPL);

  return NULL;
}

/* Calls into the handler */
VOID
EFIAPI
InterruptsLibIrqHandler (
  IN  HARDWARE_INTERRUPT_SOURCE   Source,
  IN  EFI_SYSTEM_CONTEXT          SystemContext
  )
{
  EFI_TPL     OriginalTPL;

  Source = MmioRead32(VIC_IRQ_VEC_PEND_RD);
  MmioWrite32((Source > 31) ? VIC_INT_CLEAR1 : VIC_INT_CLEAR0, 1 << (Source & 31));

  // get handler entry
  HANDLER_ENTRY* Entry = GetInterruptHandlerEntry ((UINTN)Source);
  ASSERT (Entry);

  OriginalTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  // call handler, ignore the return value because we don't support threads
  Entry->Handler (Entry->Arg);

  gBS->RestoreTPL (OriginalTPL);

  MmioWrite32(VIC_IRQ_VEC_WR, 0);
}

/* disables the interrupt */
INTN mask_interrupt(UINTN Vector)
{
    EFI_STATUS Status = mInterrupt->DisableInterruptSource (mInterrupt, Vector);
    ASSERT_EFI_ERROR (Status);
    return Status==EFI_SUCCESS?0:-1;
}

/* enables the interrupt */
INTN unmask_interrupt(UINTN Vector)
{
    EFI_STATUS Status = mInterrupt->EnableInterruptSource (mInterrupt, Vector);
    ASSERT_EFI_ERROR (Status);
    return Status==EFI_SUCCESS?0:-1;
}

/* Registers an interrupt handler for an IRQ */
VOID register_int_handler(UINTN Vector, int_handler Handler, VOID *Arg)
{
    EFI_STATUS     Status;
    EFI_TPL        OriginalTPL;
    HANDLER_ENTRY  *Entry;


    // make sure, we didn't register a handler already
    Entry = GetInterruptHandlerEntry (Vector);
    ASSERT (Entry == NULL);

    // allocate entry structure
    Entry = AllocateZeroPool (sizeof (HANDLER_ENTRY));
    if (Entry == NULL) {
        ASSERT (FALSE);
        return;
    }

    // fill struct and insert
    Entry->Vector     = Vector;
    Entry->Handler    = Handler;
    Entry->Arg        = Arg;
    InsertTailList (&mHandlers, &Entry->Link);

    OriginalTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);

    // register interrupt source
    Status = mInterrupt->RegisterInterruptSource (mInterrupt, Vector, InterruptsLibIrqHandler);
    ASSERT_EFI_ERROR (Status);

    gBS->RestoreTPL (OriginalTPL);
}


RETURN_STATUS
EFIAPI
InterruptsLibConstructor (
  VOID
  )
{
  EFI_STATUS Status;

  Status = gBS->LocateProtocol (&gHardwareInterruptProtocolGuid, NULL, (VOID **)&mInterrupt);
  ASSERT_EFI_ERROR(Status);

  return Status;
}