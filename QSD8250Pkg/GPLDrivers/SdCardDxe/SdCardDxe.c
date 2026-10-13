/*
 * Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 * Copytight (c) 2023, Dominik Kobinski <dominikkobinski314@gmail.com>
 *
 * (C) Copyright 2003
 * Kyle Harris, Nexus Technologies, Inc. kharris@nexus-tech.net
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
 
#include <Library/ArmLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/GpioTlmm.h>
#include <Protocol/EmbeddedClock.h>

#include <Chipset/clock.h>

#include "SdCardDxe.h"

static block_dev_desc_t *sdc_dev;

// Cached copy of the Hardware Gpio protocol instance
TLMM_GPIO *gGpio = NULL;

// Cached copy of the Embedded Clock protocol instance
EMBEDDED_CLOCK_PROTOCOL  *gClock = NULL;

EFI_BLOCK_IO_MEDIA gMMCHSMedia = 
{
	SIGNATURE_32('s', 'd', 'c', 'c'),         // MediaId
	TRUE,                                    // RemovableMedia
	TRUE,                                     // MediaPresent
	FALSE,                                    // LogicalPartition
	FALSE,                                    // ReadOnly
	FALSE,                                    // WriteCaching
	512,                                      // BlockSize
	4,                                        // IoAlign
	0,                                        // Pad
	0                                         // LastBlock
};

typedef struct
{
	VENDOR_DEVICE_PATH  Mmc;
	EFI_DEVICE_PATH     End;
} MMCHS_DEVICE_PATH;

MMCHS_DEVICE_PATH gMmcHsDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      { (UINT8)(sizeof(VENDOR_DEVICE_PATH)), (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8) },
    },
    { 0xb615f1f5, 0x5088, 0x43cd, { 0x80, 0x9c, 0xa1, 0x6e, 0x52, 0x48, 0x7d, 0x00 } }
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
  }
};


/**

  Reset the Block Device.

  @param  This                 Indicates a pointer to the calling context.
  @param  ExtendedVerification Driver may perform diagnostics on reset.
  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
  not be reset.
  **/
EFI_STATUS
EFIAPI
MMCHSReset(
	IN EFI_BLOCK_IO_PROTOCOL          *This,
	IN BOOLEAN                        ExtendedVerification
)
{
	return EFI_SUCCESS;
}


/**

  Read BufferSize bytes from Lba into Buffer.
  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the destination buffer for the data. The caller is
  responsible for either having implicit or explicit ownership of the buffer.


  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,

  or the buffer is not on proper alignment.

  EFI_STATUS

  **/
EFI_STATUS
EFIAPI
MMCHSReadBlocks(
	IN EFI_BLOCK_IO_PROTOCOL          *This,
	IN UINT32                         MediaId,
	IN EFI_LBA                        Lba,
	IN UINTN                          BufferSize,
	OUT VOID                          *Buffer
)
{
	EFI_STATUS Status = EFI_SUCCESS;
	UINTN      ret = 0;
    UINTN      ReadSize = 0;

	if (BufferSize % gMMCHSMedia.BlockSize != 0) 
    {
		DEBUG((EFI_D_ERROR, "MMCHSReadBlocks: BAD buffer!!!\n"));
    	MicroSecondDelay(5000);
        return EFI_BAD_BUFFER_SIZE;
    }

	if (Buffer == NULL) 
    {
		DEBUG((EFI_D_ERROR, "MMCHSReadBlocks: Invalid parameter!!!\n"));
    	MicroSecondDelay(5000);
        return EFI_INVALID_PARAMETER;
    }

	if (BufferSize == 0) 
    {
		DEBUG((EFI_D_ERROR, "MMCHSReadBlocks: BufferSize = 0\n"));
    	MicroSecondDelay(5000);
        return EFI_SUCCESS;
    }

    ReadSize = BufferSize / gMMCHSMedia.BlockSize;

	ret = sdc_dev->block_read(SDC_INSTANCE, (ulong)Lba, (lbaint_t)ReadSize, Buffer);

	if (ret)
    {
        return EFI_SUCCESS;
    }
    else
    {
        DEBUG((EFI_D_ERROR, "MMCHSReadBlocks: Read error!\n"));
        MicroSecondDelay(5000);
        return EFI_DEVICE_ERROR;
    }
    
	return Status;
}

/**

  Write BufferSize bytes from Lba into Buffer.
  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    The media ID that the write request is for.
  @param  Lba        The starting logical block address to be written. The caller is
  responsible for writing to only legitimate locations.
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
  or the buffer is not on proper alignment.

  **/
EFI_STATUS
EFIAPI
MMCHSWriteBlocks(
	IN EFI_BLOCK_IO_PROTOCOL          *This,
	IN UINT32                         MediaId,
	IN EFI_LBA                        Lba,
	IN UINTN                          BufferSize,
	IN VOID                           *Buffer
)
{
	EFI_STATUS Status = EFI_SUCCESS;
	UINTN      ret = 0;
    UINTN      WriteSize = 0;

	if (BufferSize % gMMCHSMedia.BlockSize != 0) 
    {
		DEBUG((EFI_D_ERROR, "MMCHSWriteBlocks: BAD buffer!!!\n"));
    	MicroSecondDelay(5000);
        return EFI_BAD_BUFFER_SIZE;
    }

	if (Buffer == NULL) 
    {
		DEBUG((EFI_D_ERROR, "MMCHSWriteBlocks: Invalid parameter!!!\n"));
    	MicroSecondDelay(5000);
        return EFI_INVALID_PARAMETER;
    }

	if (BufferSize == 0) 
    {
		DEBUG((EFI_D_ERROR, "MMCHSWriteBlocks: BufferSize = 0\n"));
    	MicroSecondDelay(5000);
        return EFI_SUCCESS;
    }

	WriteSize = BufferSize / gMMCHSMedia.BlockSize;

	ret = sdc_dev->block_write(SDC_INSTANCE, (ulong)Lba, (lbaint_t)WriteSize, Buffer);
	
	if (ret)
    {
        return EFI_SUCCESS;
    }
    else
    {
        DEBUG((EFI_D_ERROR, "MMCHSWriteBlocks: Write error!\n"));
        MicroSecondDelay(5000);
        return EFI_DEVICE_ERROR;
    }
	
	return Status;
}


/**

  Flush the Block Device.
  @param  This              Indicates a pointer to the calling context.
  @retval EFI_SUCCESS       All outstanding data was written to the device
  @retval EFI_DEVICE_ERROR  The device reported an error while writting back the data
  @retval EFI_NO_MEDIA      There is no media in the device.

  **/
EFI_STATUS
EFIAPI
MMCHSFlushBlocks(
	IN EFI_BLOCK_IO_PROTOCOL  *This
)
{
	return EFI_SUCCESS;
}


EFI_BLOCK_IO_PROTOCOL gBlockIo = {
	EFI_BLOCK_IO_INTERFACE_REVISION,   // Revision
	&gMMCHSMedia,                      // *Media
	MMCHSReset,                        // Reset
	MMCHSReadBlocks,                   // ReadBlocks
	MMCHSWriteBlocks,                  // WriteBlocks
	MMCHSFlushBlocks                   // FlushBlocks
};

EFI_STATUS
EFIAPI
SdCardInitialize(
	IN EFI_HANDLE         ImageHandle,
	IN EFI_SYSTEM_TABLE   *SystemTable
)
{
	EFI_STATUS  Status = EFI_SUCCESS;

    // Find the gpio controller protocol.  ASSERT if not found.
    Status = gBS->LocateProtocol (&gTlmmGpioProtocolGuid, NULL, (VOID **)&gGpio);
    ASSERT_EFI_ERROR (Status);

    // Find the clock controller protocol.  ASSERT if not found.
  	Status = gBS->LocateProtocol (&gEmbeddedClockProtocolGuid, NULL, (VOID **)&gClock);
  	ASSERT_EFI_ERROR (Status);

#if DEVICETYPE == 1
    /* Read the gpio on leo (we assume sd is always in on schubert )*/
    if (!gGpio->Get(HTCLEO_GPIO_SD_STATUS))
    {
#endif
        // Enable the SDC2 clock
        gClock->ClkEnable(SDC2_CLK);

        // Enable SD
        mmc_legacy_init(0);

        sdc_dev = mmc_get_dev();

        gMMCHSMedia.LastBlock = sdc_dev->lba;
        gMMCHSMedia.BlockSize = sdc_dev->blksz;

        UINT8 BlkDump[gMMCHSMedia.BlockSize];
		ZeroMem(BlkDump, gMMCHSMedia.BlockSize);
		BOOLEAN FoundMbr = FALSE;

		for (UINTN i = 0; i <= MIN(gMMCHSMedia.LastBlock, 50); i++)
		{
            int blk = sdc_dev->block_read(SDC_INSTANCE, i, 1, &BlkDump);
            if (blk)
            {
                if (BlkDump[510] == 0x55 && BlkDump[511] == 0xAA)
                {
                    DEBUG((EFI_D_ERROR, "MBR found at %d \n", i));
                    FoundMbr = TRUE;
                    break;
                }
                DEBUG((EFI_D_ERROR, "MBR not found at %d \n", i));
            }
		}    
        if (!FoundMbr)
        {
            DEBUG((EFI_D_ERROR, "(Protective) MBR not found \n"));
            CpuDeadLoop();
        }

		//Publish BlockIO.
		Status = gBS->InstallMultipleProtocolInterfaces(
			&ImageHandle,
			&gEfiBlockIoProtocolGuid, &gBlockIo,
			&gEfiDevicePathProtocolGuid, &gMmcHsDevicePath,
			NULL
			);
#if DEVICETYPE == 1
    }
    else {
        // TODO: Defer installing protocol until card is found
        DEBUG((EFI_D_ERROR, "SD Card NOT inserted!\n"));
        return EFI_DEVICE_ERROR;
    }
#endif
	return Status;
}