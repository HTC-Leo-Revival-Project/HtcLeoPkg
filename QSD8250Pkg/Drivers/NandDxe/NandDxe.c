#include <PiDxe.h>
#include <Uefi.h>

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/LKEnvLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DMA.h>

#include <Device/Flash.h>
#include <Device/Nand.h>

STATIC struct flash_info flash_info;
INTN flash_bad_blocks;
STATIC UINTN *flash_ptrlist;
STATIC DMOV_S *flash_cmdlist;
STATIC VOID *flash_spare;
STATIC VOID *flash_data;
STATIC UINTN num_pages_per_blk = 0;
STATIC UINTN num_pages_per_blk_mask = 0;
STATIC UINTN flash_pagesize = 0;
UINTN blocks_per_mb = 1;
UINTN nand_num_blocks;
UINTN nand_cfg0;
UINTN nand_cfg1;
STATIC UINTN CFG0, CFG1;

static void flash_nand_read_id(DMOV_S *cmdlist, unsigned *ptrlist)
{
	DMOV_S *cmd = cmdlist;
	unsigned *ptr = ptrlist;
	unsigned *data = ptrlist + 4;

	data[0] = 0 | 4;
	data[1] = NAND_CMD_FETCH_ID;
	data[2] = 1;
	data[3] = 0;
	data[4] = 0;
	data[5] = 0;
	data[6] = 0;
	data[7] = 0xAAD40000;  /* Default value for CFG0 for reading device id */

	/* Read NAND device id */
	cmd[0].cmd = 0 | CMD_OCB;
	cmd[0].src = paddr(&data[7]);
	cmd[0].dst = NAND_DEV0_CFG0;
	cmd[0].len = 4;

	cmd[1].cmd = 0;
	cmd[1].src = NAND_SFLASHC_BURST_CFG;
	cmd[1].dst = paddr(&data[5]);
	cmd[1].len = 4;

	cmd[2].cmd = 0;
	cmd[2].src = paddr(&data[6]);
	cmd[2].dst = NAND_SFLASHC_BURST_CFG;
	cmd[2].len = 4;

	cmd[3].cmd = 0;
	cmd[3].src = paddr(&data[0]);
	cmd[3].dst = NAND_FLASH_CHIP_SELECT;
	cmd[3].len = 4;

	cmd[4].cmd = DST_CRCI_NAND_CMD;
	cmd[4].src = paddr(&data[1]);
	cmd[4].dst = NAND_FLASH_CMD;
	cmd[4].len = 4;

	cmd[5].cmd = 0;
	cmd[5].src = paddr(&data[2]);
	cmd[5].dst = NAND_EXEC_CMD;
	cmd[5].len = 4;

	cmd[6].cmd = SRC_CRCI_NAND_DATA;
	cmd[6].src = NAND_FLASH_STATUS;
	cmd[6].dst = paddr(&data[3]);
	cmd[6].len = 4;

	cmd[7].cmd = 0;
	cmd[7].src = NAND_READ_ID;
	cmd[7].dst = paddr(&data[4]);
	cmd[7].len = 4;

	cmd[8].cmd = CMD_OCU | CMD_LC;
	cmd[8].src = paddr(&data[5]);
	cmd[8].dst = NAND_SFLASHC_BURST_CFG;
	cmd[8].len = 4;

	ptr[0] = (paddr(cmd) >> 3) | CMD_PTR_LP;

	DmovExecCmdPtr(DMOV_NAND_CHAN, ptr);

	DEBUG((EFI_D_ERROR, "   Status: %x\n", data[3]));

	flash_info.id = data[4];
	flash_info.vendor = data[4] & 0xff;
	flash_info.device = (data[4] >> 8) & 0xff;
	return;
}

static void flash_read_id(DMOV_S *cmdlist, unsigned *ptrlist)
{
	int dev_found = 0;
	unsigned index;

	// Try to read id
	flash_nand_read_id(cmdlist, ptrlist);
	// Check if we support the device
	for (index=1;
		 index < (sizeof(supported_flash)/sizeof(struct flash_identification));
		 index++)
	{
		if ((flash_info.id & supported_flash[index].mask) ==
			(supported_flash[index].flash_id &
			(supported_flash[index].mask))) {
			dev_found = 1;
			break;
		}
	}

	if(dev_found) {
		if (supported_flash[index].widebus)
			flash_info.type = FLASH_16BIT_NAND_DEVICE;
		else
			flash_info.type = FLASH_8BIT_NAND_DEVICE;

		flash_info.page_size = supported_flash[index].pagesize;
		flash_pagesize = flash_info.page_size;
		flash_info.block_size = supported_flash[index].blksize;
		flash_info.spare_size = supported_flash[index].oobsize;
		AsciiStrCpyS(flash_info.manufactory,sizeof(flash_info.manufactory),supported_flash[index].man);
		if (flash_info.block_size && flash_info.page_size)
		{
			flash_info.num_blocks = supported_flash[index].density;
			flash_info.num_blocks /= (flash_info.block_size);
		}
		else
		{
			flash_info.num_blocks = 0;
		}
		ASSERT(flash_info.num_blocks);
		num_pages_per_blk = flash_info.block_size / flash_pagesize;
		num_pages_per_blk_mask = num_pages_per_blk - 1;
		return;
	}

	// Assume 8 bit nand device for backward compatability
	if (dev_found == 0) {
		DEBUG((EFI_D_ERROR, "   Device not supported.  Assuming 8 bit NAND device\n"));
		flash_info.type = FLASH_8BIT_NAND_DEVICE;
	}
	DEBUG((EFI_D_ERROR, "   nandid: 0x%x maker=0x%02x device=0x%02x page_size=%d\n",
		flash_info.id, flash_info.vendor, flash_info.device,
		flash_info.page_size));
	DEBUG((EFI_D_ERROR, "   spare_size=%d block_size=%d num_blocks=%d\n",
		flash_info.spare_size, flash_info.block_size,
		flash_info.num_blocks));
}

static int flash_nand_read_config(DMOV_S *cmdlist, unsigned *ptrlist)
{
	static unsigned CFG0_TMP, CFG1_TMP;
	cmdlist[0].cmd = CMD_OCB;
	cmdlist[0].src = NAND_DEV0_CFG0;
	cmdlist[0].dst = paddr(&CFG0_TMP);
	cmdlist[0].len = 4;

	cmdlist[1].cmd = CMD_OCU | CMD_LC;
	cmdlist[1].src = NAND_DEV0_CFG1;
	cmdlist[1].dst = paddr(&CFG1_TMP);
	cmdlist[1].len = 4;

	*ptrlist = (paddr(cmdlist) >> 3) | CMD_PTR_LP;

	DmovExecCmdPtr(DMOV_NAND_CHAN, ptrlist);

	if((CFG0_TMP == 0) || (CFG1_TMP == 0)) {
		return -1;
	}

	CFG0 = CFG0_TMP;
	CFG1 = CFG1_TMP;
	if (flash_info.type == FLASH_16BIT_NAND_DEVICE) {
		nand_cfg1 |= CFG1_WIDE_FLASH;
	}
	DEBUG((EFI_D_ERROR, "   nandcfg: %x %x (initial)\n", CFG0_TMP, CFG1_TMP));

	CFG0 = (((flash_pagesize >> 9) - 1) <<  6)  /* 4/8 cw/pg for 2/4k */
		|	(512 <<  9)  /* 516 user data bytes */
		|	(10 << 19)  /* 10 parity bytes */
		|	(4 << 23)  /* spare size */
		|	(5 << 27)  /* 5 address cycles */
		|	(1 << 30)  /* Do not read status before data */
		|	(1 << 31);  /* Send read cmd */

	CFG1 = CFG1
	  	|	((flash_pagesize - (528 * ((flash_pagesize >> 9) - 1)) + 1) <<  6)	/* Bad block marker location */
		|	(nand_cfg1 & CFG1_WIDE_FLASH); /* preserve wide flash flag */
	CFG1 = CFG1
		&   ~(1 <<  0)  /* Enable ecc */
		&   ~(1 << 16); /* Bad block in user data area */
	DEBUG((EFI_D_ERROR, "   nandcfg: %x %x (used)\n", CFG0, CFG1));

	return 0;
}


void flash_init(void)
{
	//ASSERT(flash_ptable == NULL);

flash_ptrlist = (UINTN*)AllocateAlignedPages(EFI_SIZE_TO_PAGES(1024), 32);
flash_cmdlist = (DMOV_S*)AllocateAlignedPages(EFI_SIZE_TO_PAGES(1024), 32);
flash_data    = (UINTN*)AllocateAlignedPages(EFI_SIZE_TO_PAGES(4096 + 128), 32);
flash_spare   = (UINTN*)AllocateAlignedPages(EFI_SIZE_TO_PAGES(128), 32);

	flash_read_id(flash_cmdlist, flash_ptrlist);
	if((FLASH_8BIT_NAND_DEVICE == flash_info.type)
		||(FLASH_16BIT_NAND_DEVICE == flash_info.type)) {
		if(flash_nand_read_config(flash_cmdlist, flash_ptrlist)) {
			DEBUG((EFI_D_ERROR, "\n   ERROR: could not read CFG0/CFG1 state"));
			ASSERT(0);
		}
	}
	flash_bad_blocks = -1;//block_tbl not present
}

struct flash_info *flash_get_info(void)
{
	return &flash_info;
}

VOID DumpFlashInfo(struct flash_info *Info)
{
    ASSERT(Info != NULL);

    DEBUG((EFI_D_ERROR, "==== FLASH INFO DUMP ====\n"));

    DEBUG((EFI_D_ERROR, "ID            : 0x%08x\n", Info->id));
    DEBUG((EFI_D_ERROR, "Type          : %u\n", Info->type));
    DEBUG((EFI_D_ERROR, "Vendor        : %u\n", Info->vendor));
    DEBUG((EFI_D_ERROR, "Device        : %u\n", Info->device));

    DEBUG((EFI_D_ERROR, "Page Size     : %u bytes\n", Info->page_size));
    DEBUG((EFI_D_ERROR, "Block Size    : %u bytes\n", Info->block_size));
    DEBUG((EFI_D_ERROR, "Spare Size    : %u bytes\n", Info->spare_size));
    DEBUG((EFI_D_ERROR, "Num Blocks    : %u\n", Info->num_blocks));

    DEBUG((EFI_D_ERROR, "Manufacturer  : %a\n", Info->manufactory));

    // Derived values (useful for debugging)
    if (Info->block_size != 0) {
        UINTN blocks_per_mb = (1024 * 1024) / Info->block_size;
        DEBUG((EFI_D_ERROR, "Blocks per MB : %u\n", blocks_per_mb));
    }

    DEBUG((EFI_D_ERROR, "Total Size    : %u MB\n",
        (Info->num_blocks * Info->block_size) / (1024 * 1024)));

    DEBUG((EFI_D_ERROR, "=========================\n"));
}


EFI_STATUS
EFIAPI
NandDxeInitialize(
	IN EFI_HANDLE         ImageHandle,
	IN EFI_SYSTEM_TABLE   *SystemTable
)
{
	EFI_STATUS  Status = EFI_SUCCESS;
	struct flash_info *flash_info;
	
	flash_init();
	DEBUG((EFI_D_ERROR, "flash_init() done \n"));
	flash_info = flash_get_info();

	ASSERT( flash_info );
	ASSERT( flash_info->num_blocks );

	for(int i = 0; i < 25; i++){
		DEBUG((EFI_D_ERROR,"%d \n", i));
	}
	DumpFlashInfo(flash_info);

	nand_num_blocks = flash_info->num_blocks;
	blocks_per_mb	= ( 1024 * 1024 ) / flash_info->block_size;

	return Status;
}