/** @file
  MSM UART Serial Port Library interface.

  Provides basic polling-based serial I/O using MSM UART registers.
**/

#ifndef MSM_SERIAL_PORT_LIB_H_
#define MSM_SERIAL_PORT_LIB_H_

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/SerialPortLib.h>
#include <Library/IoLib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  Initialize the serial device hardware.

  @retval RETURN_SUCCESS        The serial device was initialized.
  @retval RETURN_DEVICE_ERROR   The serial device could not be initialized.
**/
RETURN_STATUS
EFIAPI
SerialPortInitialize (
  VOID
  );

/**
  Write data to serial device.
**/
UINTN
EFIAPI
SerialPortWrite (
  IN UINT8  *Buffer,
  IN UINTN   NumberOfBytes
  );

/**
  Read data from serial device.
**/
UINTN
EFIAPI
SerialPortRead (
  OUT UINT8  *Buffer,
  IN UINTN    NumberOfBytes
  );

/**
  Poll serial device for RX data.
**/
BOOLEAN
EFIAPI
SerialPortPoll (
  VOID
  );

/**
  Set serial control (unsupported).
**/
RETURN_STATUS
EFIAPI
SerialPortSetControl (
  IN UINT32 Control
  );

/**
  Get serial control (unsupported).
**/
RETURN_STATUS
EFIAPI
SerialPortGetControl (
  OUT UINT32 *Control
  );

/**
  Set attributes (unsupported).
**/
RETURN_STATUS
EFIAPI
SerialPortSetAttributes (
  IN OUT UINT64             *BaudRate,
  IN OUT UINT32             *ReceiveFifoDepth,
  IN OUT UINT32             *Timeout,
  IN OUT EFI_PARITY_TYPE    *Parity,
  IN OUT UINT8              *DataBits,
  IN OUT EFI_STOP_BITS_TYPE *StopBits
  );

#ifdef __cplusplus
}
#endif

#endif // MSM_SERIAL_PORT_LIB_H_