/** @file
 * Device Detection DXE Driver
 *
 * Copyright (c) 2026, J0SH1X <aljoshua.hell@gmail.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

**/
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/IoLib.h>
#include <Library/Atags.h>
#include <Device/DeviceType.h>
#include <Library/PcdLib.h>
#include <Protocol/HtcDeviceDetection.h>
#include "Atags.h"

UINT32 mSystemMemoryBase = FixedPcdGet32 (PcdSystemMemoryBase);
HtcDevice mDevice;


HtcDevice GetDeviceInfo(VOID) {
    return mDevice;
}

HTC_DEVICE_DETECTION_PROTOCOL gHtcDeviceDetectionProtocol = {
  GetDeviceInfo
};

EFI_STATUS 
EFIAPI 
DeviceDetectionDxeInit(
  IN EFI_HANDLE ImageHandle, 
  IN EFI_SYSTEM_TABLE *SystemTable) 
{
  EFI_STATUS Status = EFI_SUCCESS;
  EFI_HANDLE Handle = NULL;

  ParseAtags((UINT32)mSystemMemoryBase+0x100,&mDevice); // ATAGs are typically located at 0x100 from the start of memory
  DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Finished parsing ATAGs\n"));

  Status = gBS->InstallMultipleProtocolInterfaces(&Handle, &gHtcDeviceDetectionProtocolGuid, &gHtcDeviceDetectionProtocol, NULL);
	ASSERT_EFI_ERROR(Status);

  return Status;
}