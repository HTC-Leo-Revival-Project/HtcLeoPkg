/** @file
  HTC HD2 (QSD8250) touchscreen support.

  The panel is an I2C controller on the MSM I2C bus; the Linux driver for it is
  arch/arm/mach-msm/board-htcleo-ts.c.  This header describes the small vendor
  protocol TouchDxe adds on top of the standard absolute/simple pointer ones, so
  that a test application can look at what the controller actually reported
  (raw controller coordinates, the number of contacts, the state of the IRQ
  line) rather than only the scaled screen position.

  Copyright (c) 2024.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __HTCLEO_TOUCH_H__
#define __HTCLEO_TOUCH_H__

#include <Protocol/AbsolutePointer.h>

///
/// The controller types board-htcleo-ts.c probes for.  Only 2A is supported
/// there too; B8 and 68 are detected and rejected.
///
#define HTCLEO_TOUCH_TYPE_UNKNOWN  0
#define HTCLEO_TOUCH_TYPE_B8       1   // I2C 0xB8 >> 1
#define HTCLEO_TOUCH_TYPE_68       2   // I2C 0x68 >> 1
#define HTCLEO_TOUCH_TYPE_2A       3   // I2C 0x2A >> 1

///
/// Coordinate range the controller reports, as declared by input_set_abs_params()
/// in board-htcleo-ts.c.  Note that this is larger than the 480x800 panel, so
/// the driver scales it.
///
#define HTCLEO_TOUCH_X_MIN    6
#define HTCLEO_TOUCH_X_MAX    576
#define HTCLEO_TOUCH_Y_MIN    6
#define HTCLEO_TOUCH_Y_MAX    952

///
/// I2C addresses (already shifted down to the 7-bit form) of the probes.
///
#define HTCLEO_TOUCH_I2C_B8   0x5C
#define HTCLEO_TOUCH_I2C_68   0x34
#define HTCLEO_TOUCH_I2C_2A   0x15

typedef struct {
  UINT32 Version;
  ///
  /// HTCLEO_TOUCH_TYPE_* as detected at driver start up.
  ///
  UINT32 ControllerType;
  ///
  /// The active coordinate range exposed through the pointer protocols; this is
  /// the panel resolution the driver scaled the controller range to.
  ///
  UINT32 ResolutionX;
  UINT32 ResolutionY;
  ///
  /// Number of contacts (1 or 2) in the most recent report.
  ///
  UINT32 Contacts;
  ///
  /// Controller space coordinates of the most recent report, before scaling.
  ///
  UINT32 RawX;
  UINT32 RawY;
  ///
  /// TRUE while the controller holds its (active low) interrupt line low, i.e.
  /// while a report is waiting to be read.
  ///
  BOOLEAN IrqActive;
  ///
  /// How many interrupts the driver has taken since start up.  This is the
  /// counter that says whether the TLMM -> SIRC -> VIC path really delivered
  /// the controller's interrupt.
  ///
  UINT64 IrqCount;
  ///
  /// How many reports have been read since start up.
  ///
  UINT64 ReportCount;
  ///
  /// TRUE when the report that was read last was picked up because an
  /// interrupt had announced it, FALSE when it was found by looking at the
  /// interrupt line without one having been delivered.
  ///
  BOOLEAN LastReportByInterrupt;
} HTCLEO_TOUCH_STATUS;

#define HTCLEO_TOUCH_PROTOCOL_GUID                                             \
  {                                                                            \
    0x2c898318, 0x41c1, 0x4309,                                                \
    {                                                                          \
      0x89, 0x8a, 0x2f, 0x55, 0xc8, 0xcf, 0x0b, 0x89                           \
    }                                                                          \
  }

typedef struct _HTCLEO_TOUCH_PROTOCOL HTCLEO_TOUCH_PROTOCOL;

///
/// Reads the controller if it says it has something to say.  Cheap to call in a
/// loop: it does no I2C traffic at all while the interrupt line is idle, and at
/// most one report per call.
///
typedef
EFI_STATUS
(EFIAPI *HTCLEO_TOUCH_POLL)(
  VOID
  );

///
/// Copies the cached status out.
///
typedef
VOID
(EFIAPI *HTCLEO_TOUCH_GET_STATUS)(
  OUT HTCLEO_TOUCH_STATUS  *Status
  );

struct _HTCLEO_TOUCH_PROTOCOL {
  HTCLEO_TOUCH_POLL        Poll;
  HTCLEO_TOUCH_GET_STATUS  GetStatus;
};

extern EFI_GUID  gHtcLeoTouchProtocolGuid;

#endif
