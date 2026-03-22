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
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include <Device/DeviceType.h>


#include <Protocol/HtcDeviceDetection.h>

#include "Atags.h"
#include "Spl.h"

UINT32 mSystemMemoryBase = FixedPcdGet32 (PcdSystemMemoryBase);
HtcDevice mDevice;

BOOLEAN DetectCLK(CHAR8* cmdline) {
    if (cmdline == NULL) {
        return FALSE;
    }

if (AsciiStrStr(cmdline, "clk=") != NULL &&
    (AsciiStrStr(cmdline, "androidboot.hardware=leo") != NULL ||
     AsciiStrStr(cmdline, "androidboot.hardware=schubert") != NULL)) {
    return TRUE;
}

    return FALSE;
}

CHAR8* GetStringFromMemory(UINT8* MemoryAddr, UINTN BufferSize, UINTN MemReadSize)
{
    CHAR8* Buffer = AllocateZeroPool(BufferSize);
    if (Buffer == NULL) {
        DEBUG((DEBUG_ERROR, "Failed to allocate buffer\n"));
        return NULL;
    }

    CopyMem(Buffer, MemoryAddr, MemReadSize);
    Buffer[MemReadSize] = '\0';

    return Buffer;
}

VOID SPLDetectDevice(VOID) {
  CHAR8* SplVersion = GetStringFromMemory((UINT8*)SPL_ADRESS, 24, 9);
  DEBUG((EFI_D_INFO | EFI_D_LOAD, "SPL version string: %a\n", SplVersion));
  mDevice.type = ParseSPLVersion(SplVersion);
  mDevice.cLK = FALSE;
  mDevice.nandBoot = TRUE;
  mDevice.cmdline = "";
  FreePool(SplVersion);

}

DeviceType AtagsDetectDevice(CHAR8 *cmdline) {
    if (strstr(cmdline, "androidboot.hardware=leo"))
        return LEO;
    if (strstr(cmdline, "androidboot.hardware=schubert"))
        return SCHUBERT;
    if (strstr(cmdline, "board_bravo"))
        return BRAVO;
    if (strstr(cmdline, "board_mahimahi"))
        return PASSION;

    return UNKNOWN;
}

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
  if (mDevice.type != UNKNOWN){
    DEBUG((EFI_D_INFO | EFI_D_LOAD,"[ATAG] Finished parsing ATAGs\n"));
  }else {
    DEBUG((EFI_D_INFO | EFI_D_LOAD,"Could not find ATAGS falling back to smem\n"));
    SPLDetectDevice();
  }
  Status = gBS->InstallMultipleProtocolInterfaces(&Handle, &gHtcDeviceDetectionProtocolGuid, &gHtcDeviceDetectionProtocol, NULL);
	ASSERT_EFI_ERROR(Status);

  return Status;
}