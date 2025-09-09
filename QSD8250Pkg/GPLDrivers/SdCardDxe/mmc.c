/*
 * Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
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

#include "SdCardDxe.h"

#ifdef USE_DM
  #define NUM_BLOCKS_MULT    256
#else
  #define NUM_BLOCKS_MULT    1
#endif
#define NUM_BLOCKS_STATUS  1024


static block_dev_desc_t mmc_dev;
struct sd_parms sdcn;

uint32_t scr[2];
int scr_valid = FALSE;
uint16_t rca;

// Structures for use with ADM
uint32_t sd_adm_cmd_ptr_list[8] __attribute__ ((aligned(8))); // Must aligned on 8 byte boundary
uint32_t sd_box_mode_entry[8]   __attribute__ ((aligned(8))); // Must aligned on 8 byte boundary

static uchar spec_ver;
static int mmc_ready = 0;
static int high_capacity = FALSE;

#ifdef USE_PROC_COMM
//the desired duty cycle is 50%,
//using proc_comm with 45Mhz possibly giving too low duty cycle,
//breaking it.

enum SD_MCLK_speed
{	MCLK_144KHz = 144000,
	MCLK_400KHz = 400000,
	MCLK_25MHz = 25000000,
	MCLK_48MHz = 49152000, //true 48Mhz not supported, use next highest
	MCLK_49MHz = 49152000,
	MCLK_50MHz = 50000000,
};
#else /*USE_PROC_COMM defined*/
enum SD_MCLK_speed
{
//	MCLK_144KHz, //not implemented w/o proc_comm
	MCLK_400KHz,
	MCLK_25MHz,
	MCLK_48MHz,
//	MCLK_49MHz, //not implemented w/o proc_comm
	MCLK_50MHz
};
#endif /*USE_PROC_COMM */

// Function prototypes
static void mmc_decode_cid(uint32_t * resp);
static void mmc_decode_csd(uint32_t * resp);
static int sdcc_send_cmd(uint16_t cmd, uint32_t arg, uint32_t response[]);
static int check_clear_read_status(void);
static int check_clear_write_status(void);
static int card_set_block_size(uint32_t size);
static int read_SCR_register(uint16_t rca);
static int read_SD_status(uint16_t rca);
int switch_mode(uint16_t rca);
int card_identification_selection(uint32_t cid[], uint16_t* rca, uint8_t* num_of_io_func);
static int card_transfer_init(uint16_t rca, uint32_t csd[], uint32_t cid[]);
static int read_a_block(uint32_t block_number, uint32_t read_buffer[]);
static int read_a_block_dm(uint32_t block_number, uint32_t num_blocks, uint32_t read_buffer[]);
static int write_a_block(uint32_t block_number, uint32_t write_buffer[], uint16_t rca);
static int write_a_block_dm(uint32_t block_number, uint32_t num_blocks, uint32_t write_buffer[], uint16_t rca);
static int SD_MCLK_set(enum SD_MCLK_speed speed);
static int SDCn_init(uint32_t instance);
static void sdcard_gpio_config(int instance);


block_dev_desc_t *mmc_get_dev()
{
	return ((block_dev_desc_t *) & mmc_dev);
}

ulong
/****************************************************/
mmc_bread(int dev_num, ulong blknr, lbaint_t blkcnt, void *dst)
/****************************************************/
{

    int i;
    lbaint_t run_blkcnt = 0;

    debug("bread blknr=0x%08lx blkcnt=0x%08lx dst=0x%08lx\n", blknr, blkcnt, dst);

    if (blkcnt == 0) {
        return 0;
    }

    /* Break up reads into multiples of NUM_BLOCKS_MULT */
    while (blkcnt != 0) {
        if (blkcnt >= NUM_BLOCKS_MULT)
           i = NUM_BLOCKS_MULT;
        else
           i = blkcnt;

        if (i==1)
        {
            // Single block read
            if(!read_a_block(blknr, dst))
            {
               debug("SD - read_a_block_dm error, blknr= 0x%08lx\n", blknr);
               return run_blkcnt;
            }
        }
        else
        {
            // Multiple block read using data mover
            if(!read_a_block_dm(blknr, i, dst))
            {
               debug("SD - read_a_block_dm error, blknr= 0x%08lx\n", blknr);
               return run_blkcnt;
            }
        }

        run_blkcnt += i;
        // Output status every NUM_BLOCKS_STATUS blocks
        if ((run_blkcnt % NUM_BLOCKS_STATUS) == 0)
        {
           debug(".");
        }

        blknr += i;
        blkcnt -= i;
        dst += (BLOCK_SIZE * i);
    }

    if (run_blkcnt >= NUM_BLOCKS_STATUS) {
        debug("\n");
    }

	return run_blkcnt;
}

// TODO: Actually implement
ulong
/****************************************************/
mmc_bwrite(int dev_num, ulong blknr, lbaint_t blkcnt, void *buffer)
/****************************************************/
{

    int i;
    lbaint_t run_blkcnt = 0;

    debug("bwrite blknr=0x%08lx blkcnt=0x%08lx buffer=0x%08lx\n", blknr, blkcnt, buffer);

    if (blkcnt == 0) {
        return 0;
    }

    /* Break up writes into multiples of NUM_BLOCKS_MULT */
    while (blkcnt != 0) {
        if (blkcnt >= NUM_BLOCKS_MULT)
           i = NUM_BLOCKS_MULT;
        else
           i = blkcnt;

        if (i==1)
        {
            // Single block read
            if(!write_a_block(blknr, buffer, rca))
            {
               debug("SD - write_a_block_dm error, blknr= 0x%08lx\n", blknr);
               return run_blkcnt;
            }
        }
        else
        {
            // Multiple block read using data mover
            if(!write_a_block_dm(blknr, i, buffer, rca))
            {
               debug("SD - write_a_block_dm error, blknr= 0x%08lx\n", blknr);
               return run_blkcnt;
            }
        }

        run_blkcnt += i;
        // Output status every NUM_BLOCKS_STATUS blocks
        if ((run_blkcnt % NUM_BLOCKS_STATUS) == 0)
        {
           debug(".");
        }

        blknr += i;
        blkcnt -= i;
        buffer += (BLOCK_SIZE * i);
    }

    if (run_blkcnt >= NUM_BLOCKS_STATUS) {
        debug("\n");
    }

	return run_blkcnt;
}


int
/****************************************************/
mmc_legacy_init(int verbose)
/****************************************************/
{

    int rc = -ENODEV;
    uint32_t cid[4] = {0};
    uint32_t csd[4] = {0};
    uint8_t  dummy;
    uint32_t buffer[128];
#ifdef USE_4_BIT_BUS_MODE
    uint32_t temp32;
#endif
	/* Reset device interface type */
	mmc_dev.if_type = IF_TYPE_UNKNOWN;

    // Check to be sure ADM structures are 8 byte aligned.
    if ((((uint32_t)sd_adm_cmd_ptr_list & 0x7) != 0) ||
        (((uint32_t)sd_box_mode_entry & 0x7) != 0))
    {
        debug("SD - error ADM structures not 8 byte aligned\n");
        return rc;
    }
    debug("SD - ADM structures 8 byte aligned\n");

    // SD Init
    if (!SDCn_init(SDC_INSTANCE))
    {
       debug("SD - error initializing (SDCn_init)\n");
       return rc;
    }
    debug("SD - initialized (SDCn_init)\n");

    // Run card ID sequence
    if (!card_identification_selection(cid, &rca, &dummy))
    {
       debug("SD - error initializing (card_identification_selection)\n");
       return rc;
    }
    debug("SD - card_identification_selection\n");

    // Change SD clock configuration, set PWRSAVE and FLOW_ENA
    IO_WRITE32(sdcn.base + MCI_CLK, IO_READ32(sdcn.base + MCI_CLK) |
                         MCI_CLK__PWRSAVE___M |
                         MCI_CLK__FLOW_ENA___M );

    if (!card_transfer_init(rca, csd, cid))
    {
       debug("SD - error initializing (card_transfer_init)\n");
       return rc;
    }
    debug("SD - card_transfer_init\n");

#ifdef USE_4_BIT_BUS_MODE
    // Card is now in four bit mode, do the same with the clock
    temp32 = IO_READ32(sdcn.base + MCI_CLK);
    temp32 &= ~MCI_CLK__WIDEBUS___M;
    IO_WRITE32(sdcn.base + MCI_CLK, temp32 | (MCI_CLK__WIDEBUS__4_BIT_MODE << MCI_CLK__WIDEBUS___S));
#endif

    if (!read_SD_status(rca))
    {
       debug("SD - error reading SD status\n\r");
       return rc;
    }


    // The card is now in data transfer mode, standby state.


    // Increase MCLK to 25MHz
    SD_MCLK_set(MCLK_25MHz);

 // Put card in high-speed mode and increase the SD MCLK if supported.
 #ifdef USE_HIGH_SPEED_MODE
    if (switch_mode(rca) == TRUE)
    {
       udelay(1000);

 #if 0
       // Increase MCLK to 50MHz
       SD_MCLK_set(MCLK_50MHz);
 #else
       // Increase MCLK to 48MHz
       SD_MCLK_set(MCLK_48MHz);
 #endif

       /* Card is in high speed mode, use feedback clock. */
       temp32 = IO_READ32(sdcn.base + MCI_CLK);
       temp32 &= ~(MCI_CLK__SELECT_IN___M);
       temp32 |= (MCI_CLK__SELECT_IN__USING_FEEDBACK_CLOCK << MCI_CLK__SELECT_IN___S);
       IO_WRITE32(sdcn.base + MCI_CLK, temp32);

       udelay(1000);
    }

 #endif

    if (!card_set_block_size(BLOCK_SIZE))
    {
        debug("SD - Error setting block size\n\r");
        return rc;
    }

    // Read the first block of the SD card as a sanity check.
 #ifdef USE_DM
    if(!read_a_block_dm(0, 1, &buffer[0]))
 #else
    if(!read_a_block(0, &buffer[0]))
 #endif
    {
       debug("SD - error first block\n\r");
       return rc;
    }
    else
    {
       debug("SD - block read successful\n\r");
    }

    // Valid SD card found
    mmc_dev.if_type = IF_TYPE_SD;
    mmc_decode_csd(csd);
    mmc_decode_cid(cid);
    mmc_ready = 1;

	return 0;
}

int mmc_ident(block_dev_desc_t * dev)
{
	return 0;
}

int mmc2info(ulong addr)
{
#if 0
	if (addr >= CONFIG_SYS_MMC_BASE
	    && addr < CONFIG_SYS_MMC_BASE + (mmc_dev.lba * mmc_dev.blksz)) {
		return 1;
	}
#endif

    // ZZZZ not implemented yet.  Called by do_mem_cp().
	return 0;
}

#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)	(x)
#define unlikely(x)	(x)
#endif

#define UNSTUFF_BITS(resp,start,size)					\
	({								\
		const int __size = size;				\
		const uint32_t __mask = (__size < 32 ? 1 << __size : 0) - 1;	\
		const int32_t __off = 3 - ((start) / 32);			\
		const int32_t __shft = (start) & 31;			\
		uint32_t __res;						\
									\
		__res = resp[__off] >> __shft;				\
		if (__size + __shft > 32)				\
			__res |= resp[__off-1] << ((32 - __shft) % 32);	\
		__res & __mask;						\
	})

/*
 * Given the decoded CSD structure, decode the raw CID to our CID structure.
 */
static void mmc_decode_cid(uint32_t * resp)
{
	if (IF_TYPE_SD == mmc_dev.if_type) {
		/*
		 * SD doesn't currently have a version field so we will
		 * have to assume we can parse this.
		 */
		sprintf((char *)mmc_dev.vendor,
			"Man %02x OEM %c%c \"%c%c%c%c%c\" Date %02u/%04u",
			UNSTUFF_BITS(resp, 120, 8), UNSTUFF_BITS(resp, 112, 8),
			UNSTUFF_BITS(resp, 104, 8), UNSTUFF_BITS(resp, 96, 8),
			UNSTUFF_BITS(resp, 88, 8), UNSTUFF_BITS(resp, 80, 8),
			UNSTUFF_BITS(resp, 72, 8), UNSTUFF_BITS(resp, 64, 8),
			UNSTUFF_BITS(resp, 8, 4), UNSTUFF_BITS(resp, 12,
							       8) + 2000);
		sprintf((char *)mmc_dev.revision, "%d.%d",
			UNSTUFF_BITS(resp, 60, 4), UNSTUFF_BITS(resp, 56, 4));
		sprintf((char *)mmc_dev.product, "%u",
			UNSTUFF_BITS(resp, 24, 32));
	} else {
		/*
		 * The selection of the format here is based upon published
		 * specs from sandisk and from what people have reported.
		 */
		switch (spec_ver) {
		case 0:	/* MMC v1.0 - v1.2 */
		case 1:	/* MMC v1.4 */
			sprintf((char *)mmc_dev.vendor,
				"Man %02x%02x%02x \"%c%c%c%c%c%c%c\" Date %02u/%04u",
				UNSTUFF_BITS(resp, 120, 8), UNSTUFF_BITS(resp,
									 112,
									 8),
				UNSTUFF_BITS(resp, 104, 8), UNSTUFF_BITS(resp,
									 96, 8),
				UNSTUFF_BITS(resp, 88, 8), UNSTUFF_BITS(resp,
									80, 8),
				UNSTUFF_BITS(resp, 72, 8), UNSTUFF_BITS(resp,
									64, 8),
				UNSTUFF_BITS(resp, 56, 8), UNSTUFF_BITS(resp,
									48, 8),
				UNSTUFF_BITS(resp, 12, 4), UNSTUFF_BITS(resp, 8,
									4) +
				1997);
			sprintf((char *)mmc_dev.revision, "%d.%d",
				UNSTUFF_BITS(resp, 44, 4), UNSTUFF_BITS(resp,
									40, 4));
			sprintf((char *)mmc_dev.product, "%u",
				UNSTUFF_BITS(resp, 16, 24));
			break;

		case 2:	/* MMC v2.0 - v2.2 */
		case 3:	/* MMC v3.1 - v3.3 */
		case 4:	/* MMC v4 */
			sprintf((char *)mmc_dev.vendor,
				"Man %02x OEM %04x \"%c%c%c%c%c%c\" Date %02u/%04u",
				UNSTUFF_BITS(resp, 120, 8), UNSTUFF_BITS(resp,
									 104,
									 16),
				UNSTUFF_BITS(resp, 96, 8), UNSTUFF_BITS(resp,
									88, 8),
				UNSTUFF_BITS(resp, 80, 8), UNSTUFF_BITS(resp,
									72, 8),
				UNSTUFF_BITS(resp, 64, 8), UNSTUFF_BITS(resp,
									56, 8),
				UNSTUFF_BITS(resp, 12, 4), UNSTUFF_BITS(resp, 8,
									4) +
				1997);
			sprintf((char *)mmc_dev.product, "%u",
				UNSTUFF_BITS(resp, 16, 32));
			sprintf((char *)mmc_dev.revision, "N/A");
			break;

		default:
			printf("MMC card has unknown MMCA version %d\n",
			       spec_ver);
			break;
		}
	}
	printf("%s card.\nVendor: %s\nProduct: %s\nRevision: %s\n",
	       (IF_TYPE_SD == mmc_dev.if_type) ? "SD" : "MMC", mmc_dev.vendor,
	       mmc_dev.product, mmc_dev.revision);
}

/*
 * Given a 128-bit response, decode to our card CSD structure.
 */
static void mmc_decode_csd(uint32_t * resp)
{
	unsigned int mult, csd_struct;
    unsigned int high_capacity = FALSE;
    unsigned long int size_MB;

	if (IF_TYPE_SD == mmc_dev.if_type) {
		csd_struct = UNSTUFF_BITS(resp, 126, 2);
        switch (csd_struct)
        {
        case 0:
            break;
        case 1:
            high_capacity = TRUE;
            break;
        default:
			printf("SD: unrecognised CSD structure version %d\n",
			       csd_struct);
            return;

        }
	} else {
		/*
		 * We only understand CSD structure v1.1 and v1.2.
		 * v1.2 has extra information in bits 15, 11 and 10.
		 */
		csd_struct = UNSTUFF_BITS(resp, 126, 2);
		if (csd_struct != 1 && csd_struct != 2) {
			printf("MMC: unrecognised CSD structure version %d\n",
			       csd_struct);
			return;
		}

		spec_ver = UNSTUFF_BITS(resp, 122, 4);
		mmc_dev.if_type = IF_TYPE_MMC;
	}

    mmc_dev.blksz = 1 << UNSTUFF_BITS(resp, 80, 4);

    if (high_capacity == FALSE)
    {
        mult = 1 << (UNSTUFF_BITS(resp, 47, 3) + 2);
        mmc_dev.lba = (1 + UNSTUFF_BITS(resp, 62, 12)) * mult;
        size_MB = mmc_dev.lba * mmc_dev.blksz / (1024 * 1024);
    }
    else
    {
        // High Capacity SD CSD Version 2.0
        mmc_dev.lba = (1 + UNSTUFF_BITS(resp, 48, 16)) * 1024;
        size_MB = ((1 + UNSTUFF_BITS(resp, 48, 16)) * mmc_dev.blksz) / 1024;
    }

	/* FIXME: The following just makes assumes that's the partition type -- should really read it */
	mmc_dev.part_type = PART_TYPE_DOS;
	mmc_dev.dev = 0;
	mmc_dev.lun = 0;
	mmc_dev.type = DEV_TYPE_HARDDISK;
	mmc_dev.removable = 0;
	mmc_dev.block_read = mmc_bread;
   mmc_dev.block_write = mmc_bwrite;

	printf("Detected: %lu blocks of %lu bytes (%luMB) ",
		mmc_dev.lba,
		mmc_dev.blksz,
		size_MB);
}


static int sdcc_send_cmd(uint16_t cmd, uint32_t arg, uint32_t response[])
{
   uint8_t cmd_timeout = 0, cmd_crc_fail = 0, cmd_response_end = 0, n;
   uint8_t cmd_index = cmd & MCI_CMD__CMD_INDEX___M;
   uint32_t mci_status;

   // Program command argument before programming command register
   IO_WRITE32(sdcn.base + MCI_ARGUMENT, arg);

   // Program command index and command control bits
   IO_WRITE32(sdcn.base + MCI_CMD, cmd);

   // Check response if enabled
   if (cmd & MCI_CMD__RESPONSE___M)
   {
      // This condition has to be there because CmdCrcFail flag
      // sometimes gets set before CmdRespEnd gets set
      // Wait till one of the CMD flags is set
      while (!(cmd_crc_fail || cmd_timeout || cmd_response_end ||
              ((cmd_index == CMD5 || cmd_index == ACMD41 || cmd_index == CMD1)
                        && cmd_crc_fail)))
      {
         mci_status = IO_READ32(sdcn.base + MCI_STATUS);

         // command crc failed if flag is set
         cmd_crc_fail = (mci_status & MCI_STATUS__CMD_CRC_FAIL___M) >>
                                      MCI_STATUS__CMD_CRC_FAIL___S;

         // command response received w/o error
         cmd_response_end = (mci_status & MCI_STATUS__CMD_RESPONSE_END___M) >>
                                          MCI_STATUS__CMD_RESPONSE_END___S;

         // if CPSM intr disabled ==> timeout enabled
         if (!(cmd & MCI_CMD__INTERRUPT___M))
         {
            // command timed out flag is set
            cmd_timeout = (mci_status & MCI_STATUS__CMD_TIMEOUT___M) >>
                                        MCI_STATUS__CMD_TIMEOUT___S;
         }
      }

      // clear 'CmdRespEnd' status bit
      IO_WRITE32(sdcn.base + MCI_CLEAR, MCI_CLEAR__CMD_RESP_END_CLT___M);

      // Wait till CMD_RESP_END flag is cleared to handle slow 'mclks'
      while ((IO_READ32(sdcn.base + MCI_STATUS) & MCI_STATUS__CMD_RESPONSE_END___M));

      // Clear both just in case
      IO_WRITE32(sdcn.base + MCI_CLEAR, MCI_CLEAR__CMD_TIMOUT_CLR___M);
      IO_WRITE32(sdcn.base + MCI_CLEAR, MCI_CLEAR__CMD_CRC_FAIL_CLR___M);

      // Read the contents of 4 response registers (irrespective of
      // long/short response)
      for(n = 0; n < 4; n++)
         response[n] = IO_READ32(sdcn.base + MCI_RESPn(n));

      if ((cmd_response_end == 1) ||
          ((cmd_crc_fail == 1) &&
           (cmd_index == CMD5 || cmd_index == ACMD41 || cmd_index == CMD1)))
         return(TRUE);

      // Assuming argument (or RCA) value of 0x0 will be used for CMD5 and
      // CMD55 before card identification/initialization
      else if ((cmd_index == CMD5  && arg == 0 && cmd_timeout == 1) ||
               (cmd_index == CMD55 && arg == 0 && cmd_timeout == 1))
         return(TRUE);

      else
         return(FALSE);
   }

   // No response is required
   else
   {
      // Wait for 'CmdSent' flag to be set in status register
      while(!(IO_READ32(sdcn.base + MCI_STATUS) & MCI_STATUS__CMD_SENT___M));

      // clear 'CmdSent' status bit
      IO_WRITE32(sdcn.base + MCI_CLEAR, MCI_CLEAR__CMD_SENT_CLR___M);

      // Wait till CMD_SENT flag is cleared. To handle slow 'mclks'
      while(IO_READ32(sdcn.base + MCI_STATUS) & MCI_STATUS__CMD_SENT___M);

      return(TRUE);
   }
}


static int check_clear_read_status(void)
{
   uint32_t mci_status;
   uint8_t  data_block_end, data_crc_fail, data_timeout, rx_overrun, start_bit_err;

   // Check status
   do
   {
      mci_status = IO_READ32(sdcn.base + MCI_STATUS);

      data_block_end = (mci_status & MCI_STATUS__DATA_BLK_END___M) >>
                                     MCI_STATUS__DATA_BLK_END___S;
      data_crc_fail  = (mci_status & MCI_STATUS__DATA_CRC_FAIL___M) >>
                                     MCI_STATUS__DATA_CRC_FAIL___S;
      data_timeout   = (mci_status & MCI_STATUS__DATA_TIMEOUT___M) >>
                                     MCI_STATUS__DATA_TIMEOUT___S;
      rx_overrun     = (mci_status & MCI_STATUS__RX_OVERRUN___M) >>
                                     MCI_STATUS__RX_OVERRUN___S;
      start_bit_err  = (mci_status & MCI_STATUS__START_BIT_ERR___M) >>
                                     MCI_STATUS__START_BIT_ERR___S;

      if (data_crc_fail) return(FALSE);
      if (data_timeout ) return(FALSE);
      if (rx_overrun   ) return(FALSE);
      if (start_bit_err) return(FALSE);
   } while(!data_block_end);

   // Clear
   IO_WRITE32(sdcn.base + MCI_CLEAR, MCI_CLEAR__DATA_BLK_END_CLR___M);

   IO_WRITE32(sdcn.base + MCI_CLEAR, MCI_CLEAR__DATA_CRC_FAIL_CLR___M |
                          MCI_CLEAR__DATA_TIMEOUT_CLR___M  |
                          MCI_CLEAR__RX_OVERRUN_CLR___M    |
                          MCI_CLEAR__START_BIT_ERR_CLR___M);

   while(!(IO_READ32(sdcn.base + MCI_STATUS) & MCI_STATUS__DATAEND___M));
   IO_WRITE32(sdcn.base + MCI_CLEAR, MCI_CLEAR__DATA_END_CLR___M);

   while(IO_READ32(sdcn.base + MCI_STATUS) & MCI_STATUS__DATAEND___M);
   IO_WRITE32(sdcn.base + MCI_CLEAR, MCI_CLEAR__DATA_BLK_END_CLR___M);

   return(TRUE);

}

static int check_clear_write_status(void)
{
   uint32_t mci_status;
   uint8_t  data_block_end, data_crc_fail, data_timeout, tx_underrun, start_bit_err;

   // Check status
   do
   {
      mci_status = IO_READ32(sdcn.base + MCI_STATUS);

      data_block_end = (mci_status & MCI_STATUS__DATA_BLK_END___M) >>
                                     MCI_STATUS__DATA_BLK_END___S;
      data_crc_fail  = (mci_status & MCI_STATUS__DATA_CRC_FAIL___M) >>
                                     MCI_STATUS__DATA_CRC_FAIL___S;
      data_timeout   = (mci_status & MCI_STATUS__DATA_TIMEOUT___M) >>
                                     MCI_STATUS__DATA_TIMEOUT___S;
      tx_underrun    = (mci_status & MCI_STATUS__TX_UNDERRUN___M) >>
                                     MCI_STATUS__TX_UNDERRUN___S;
      start_bit_err  = (mci_status & MCI_STATUS__START_BIT_ERR___M) >>
                                     MCI_STATUS__START_BIT_ERR___S;

      if (data_crc_fail) return(FALSE);
      if (data_timeout ) return(FALSE);
      if (tx_underrun   ) return(FALSE);
      if (start_bit_err) return(FALSE);
   } while(!data_block_end);

   // Clear
   IO_WRITE32(sdcn.base + MCI_CLEAR, MCI_CLEAR__DATA_BLK_END_CLR___M);

   IO_WRITE32(sdcn.base + MCI_CLEAR, MCI_CLEAR__DATA_CRC_FAIL_CLR___M |
                          MCI_CLEAR__DATA_TIMEOUT_CLR___M  |
                          MCI_CLEAR__TX_UNDERRUN_CLR___M);

   while(!(IO_READ32(sdcn.base + MCI_STATUS) & MCI_STATUS__DATAEND___M));
   IO_WRITE32(sdcn.base + MCI_CLEAR, MCI_CLEAR__DATA_END_CLR___M);

   while(IO_READ32(sdcn.base + MCI_STATUS) & MCI_STATUS__DATAEND___M);
   IO_WRITE32(sdcn.base + MCI_CLEAR, MCI_CLEAR__DATA_BLK_END_CLR___M);

   return(TRUE);

}

static int card_set_block_size(uint32_t size)
{
   uint16_t cmd;
   uint32_t response[4];

   cmd = CMD16 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   if (!sdcc_send_cmd(cmd, size, response))
      return(FALSE);
   else
      return(TRUE);
}

static int read_SCR_register(uint16_t rca)
{
   uint16_t cmd;
   uint32_t response[4] = {0};
   uint32_t mci_status;

   IO_WRITE32(sdcn.base + MCI_DATA_TIMER,  RD_DATA_TIMEOUT);
   IO_WRITE32(sdcn.base + MCI_DATA_LENGTH, 8);  // size of SCR

   // ZZZZ is the following step necessary?

   // Set card block length to 8
   if (!card_set_block_size(8))
      return(FALSE);

   // Write data control register
   IO_WRITE32(sdcn.base + MCI_DATA_CTL, MCI_DATA_CTL__ENABLE___M |
                             MCI_DATA_CTL__DIRECTION___M |
                             (8 << MCI_DATA_CTL__BLOCKSIZE___S));

   // CMD55   APP_CMD follows
   cmd = CMD55 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   if (!sdcc_send_cmd(cmd, (rca << 16), response))
      return(FALSE);

   // ACMD51  SEND_SCR
   cmd = ACMD51 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   if (!sdcc_send_cmd(cmd, 0, response))
      return(FALSE);

   do
   {
      mci_status = IO_READ32(sdcn.base + MCI_STATUS);
   } while ((mci_status & MCI_STATUS__RXDATA_AVLBL___M) == 0);

   scr[0] = IO_READ32(sdcn.base + MCI_FIFO);
   scr[0] = Byte_swap32(scr[0]);
   scr[1] = IO_READ32(sdcn.base + MCI_FIFO);
   scr[1] = Byte_swap32(scr[1]);
   scr_valid = TRUE;

   return(TRUE);
}

static int read_SD_status(uint16_t rca)
{
   uint16_t cmd;
   uint32_t response[4] = {0};
   uint32_t mci_status;
   uint32_t data[16];
   uint32_t i;

   IO_WRITE32(sdcn.base + MCI_DATA_TIMER,  RD_DATA_TIMEOUT);
   IO_WRITE32(sdcn.base + MCI_DATA_LENGTH, 64);  // size of SD status

   // Set card block length to 64
   if (!card_set_block_size(64))
      return(FALSE);

   // Write data control register
   IO_WRITE32(sdcn.base + MCI_DATA_CTL, MCI_DATA_CTL__ENABLE___M |
                             MCI_DATA_CTL__DIRECTION___M |
                             (64 << MCI_DATA_CTL__BLOCKSIZE___S));

   // CMD55   APP_CMD follows
   cmd = CMD55 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   if (!sdcc_send_cmd(cmd, (rca << 16), response))
      return(FALSE);

   // ACMD13  SD_STATUS
   cmd = ACMD13 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   if (!sdcc_send_cmd(cmd, 0, response))
      return(FALSE);

   i = 0;
   while(i < 16)
   {
      mci_status = IO_READ32(sdcn.base + MCI_STATUS);

      if ((mci_status & MCI_STATUS__RXDATA_AVLBL___M) != 0)
      {
         data[i] =IO_READ32(sdcn.base + MCI_FIFO);
         i++;
      }
      else if ((mci_status & MCI_STATUS__RXACTIVE___M) == 0)
      {
         // Unexpected status on SD status read.
         return(FALSE);
      }
   }

   if (!check_clear_read_status())
   {
      return(FALSE);
   }

   // Byte swap the status dwords
   for (i=0; i<16; i++)
   {
      data[i] = Byte_swap32(data[i]);
   }

   return(TRUE);
}

#ifdef USE_HIGH_SPEED_MODE
static int switch_mode(uint16_t rca)
{

#define SWITCH_CHECK  (0 << 31)
#define SWITCH_SET    (1 << 31)

   uint16_t cmd;
   uint32_t response[4] = {0};
   uint32_t arg;
   uint32_t data[16];     // 512 bits
   uint32_t i;
   uint32_t mci_status;


   // Only cards that comply with SD Physical Layer Spec Version 1.1 or greater support
   // CMD6. Need to check the spec level in the SCR register before sending CMD6.

   if (scr_valid != TRUE)
   {
      return(FALSE);
   }

   if (((scr[0] & 0x0F000000) >> 24) == 0)
   {
      return(FALSE);
   }

   IO_WRITE32(sdcn.base + MCI_DATA_TIMER,  RD_DATA_TIMEOUT);
   IO_WRITE32(sdcn.base + MCI_DATA_LENGTH, 64);  // size of switch data

   // Set card block length to 64 bytes
   if (!card_set_block_size(64))
      return(FALSE);

   // Write data control register
   IO_WRITE32(sdcn.base + MCI_DATA_CTL, MCI_DATA_CTL__ENABLE___M |
                             MCI_DATA_CTL__DIRECTION___M |
                             (64 << MCI_DATA_CTL__BLOCKSIZE___S));

   // CMD6 Check Function
   cmd = CMD6 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   arg = SWITCH_CHECK | 0x00FFFFF1;  // check if High-Speed is supported
   if (!sdcc_send_cmd(cmd, arg, response))
      return(FALSE);

   i = 0;
   while(i < 16)
   {
      mci_status = IO_READ32(sdcn.base + MCI_STATUS);

      if ((mci_status & MCI_STATUS__RXDATA_AVLBL___M) != 0)
      {
         data[i] = IO_READ32(sdcn.base + MCI_FIFO);
         i++;
      }
      else if ((mci_status & MCI_STATUS__RXACTIVE___M) == 0)
      {
         // Unexpected status on SD status read.
         return(FALSE);
      }
   }

   if (!check_clear_read_status())
   {
      return(FALSE);
   }

   // Byte swap the status.
   for (i=0; i<16; i++)
   {
      data[i] = Byte_swap32(data[i]);
   }

   // Check to see if High-Speed mode is supported.
   // Look at bit 1 of the Function Group 1 information field.
   // This is bit 401 of the 512 byte switch status.
   if ((data[3] & 0x00020000) == 0)
   {
      return(FALSE);
   }

   // Check to see if we can switch to function 1 in group 1.
   // This is in bits 379:376 of the 512 byte switch status.
   if ((data[4] & 0x0F000000) != 0x01000000)
   {
      return(FALSE);
   }


   // At this point it is safe to change to high-speed mode.


   // Write data control register
   IO_WRITE32(sdcn.base + MCI_DATA_CTL, MCI_DATA_CTL__ENABLE___M |
                             MCI_DATA_CTL__DIRECTION___M |
                             (64 << MCI_DATA_CTL__BLOCKSIZE___S));


   // CMD6 Set Function
   cmd = CMD6 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   arg = SWITCH_SET | 0x00FFFFF1;  // Set function 1 to High-Speed
   if (!sdcc_send_cmd(cmd, arg, response))
      return(FALSE);

   i = 0;
   while(i < 16)
   {
      mci_status = IO_READ32(sdcn.base + MCI_STATUS);

      if ((mci_status & MCI_STATUS__RXDATA_AVLBL___M) != 0)
      {
         data[i] = IO_READ32(sdcn.base + MCI_FIFO);
         i++;
      }
      else if ((mci_status & MCI_STATUS__RXACTIVE___M) == 0)
      {
         // Unexpected status on SD status read.
         return(FALSE);
      }
   }

   if (!check_clear_read_status())
   {
      return(FALSE);
   }

   // Byte swap the status.
   for (i=0; i<16; i++)
   {
      data[i] = Byte_swap32(data[i]);
   }

   // Check to see if there was a successful switch to high speed mode.
   // This is in bits 379:376 of the 512 byte switch status.
   if ((data[4] & 0x0F000000) != 0x01000000)
   {
      return(FALSE);
   }

   return(TRUE);
}
#endif

int card_identification_selection(uint32_t cid[],
                                         uint16_t* rca,
                                         uint8_t* num_of_io_func)
{
   uint32_t i;
   uint16_t cmd;
   uint32_t response[4] = {0};
   uint32_t arg;
   uint32_t hc_support = 0;

   // CMD0 - put the card in the idle state
   cmd = CMD0 | MCI_CMD__ENABLE___M;
   if (!sdcc_send_cmd(cmd, 0x00000000, response))
      return(FALSE);

   // CMD8 - send interface condition
   // Attempt to init SD v2.0 features
   // voltage range 2.7-3.6V, check pattern AA
   arg = 0x000001AA;
   cmd = CMD8 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   if (!sdcc_send_cmd(cmd, arg, response))
      hc_support = 0;          // no response, not high capacity
   else
      hc_support = (1 << 30);  // HCS bit for ACMD41

   udelay(1000);
   // CMD55
   // Send before any application specific command
   // Use RCA address = 0 during initialization
   cmd = CMD55 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   if (!sdcc_send_cmd(cmd, 0x00000000, response))
      return(FALSE);

   // ACMD41
   // Reads OCR register contents
   arg = 0x00FF8000 | hc_support;
   cmd = ACMD41 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   if (!sdcc_send_cmd(cmd, arg, response))
      return(FALSE);

   // If stuck in an infinite loop after CMD55 or ACMD41 -
   // the card might have gone into inactive state w/o accepting vdd range
   // sent with ACMD41
   // Loop till power up status (busy) bit is set
   while (!(response[0] & 0x80000000))
   {

      // A short delay here after the ACMD41 will prevent the next
      // command from failing with a CRC error when I-cache is enabled.
      udelay(1000);

      cmd = CMD55 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
      if (!sdcc_send_cmd(cmd, 0x00000000, response))
         return(FALSE);

      cmd = ACMD41 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
      if (!sdcc_send_cmd(cmd, arg, response))
         return(FALSE);
   }

   // Check to see if this is a high capacity SD (SDHC) card.
   if ((response[0] & hc_support) != 0)
   {
      high_capacity = TRUE;
   }

   // A short delay here after the ACMD41 will prevent the next
   // command from failing with a CRC error when I-cache is enabled.
   udelay(1000);

   // CMD2
   // Reads CID register contents - long response R2
   cmd = CMD2 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M | MCI_CMD__LONGRSP___M;
   if (!sdcc_send_cmd(cmd, 0x00000000, response))
      return(FALSE);
   for (i = 0; i < 4; i++)
      cid[i] = response[i];

   // CMD3
   cmd = CMD3 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   if (!sdcc_send_cmd(cmd, 0x00000000, response))
      return(FALSE);
   *rca = response[0] >> 16;

   return(TRUE);
}

static int card_transfer_init(uint16_t rca, uint32_t csd[], uint32_t cid[])
{
   uint16_t cmd, i;
   uint32_t response[4] = {0};

   // CMD9    SEND_CSD
   cmd = CMD9 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M | MCI_CMD__LONGRSP___M;
   if (!sdcc_send_cmd(cmd, (rca << 16), response))
      return(FALSE);
   for (i=0; i<4; i++)
      csd[i] = response[i];

   // CMD10   SEND_CID
   cmd = CMD10 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M | MCI_CMD__LONGRSP___M;
   if (!sdcc_send_cmd(cmd, (rca << 16), response))
      return(FALSE);
   for (i=0; i<4; i++)
      cid[i] = response[i];

   // CMD7    SELECT (RCA!=0)
   cmd = CMD7 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   if (!sdcc_send_cmd(cmd, (rca << 16), response))
      return(FALSE);

   // Card is now in Transfer State

   if (!read_SCR_register(rca))
      return(FALSE);

#ifdef USE_4_BIT_BUS_MODE
   // CMD55   APP_CMD follows
   cmd = CMD55 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   if (!sdcc_send_cmd(cmd, (rca << 16), response))
      return(FALSE);

   // ACMD6   SET_BUS_WIDTH
   cmd = ACMD6 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   if (!sdcc_send_cmd(cmd, 2, response))   // 4 bit bus
      return(FALSE);
#endif

   // CMD13   SEND_STATUS
   cmd = CMD13 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   if (!sdcc_send_cmd(cmd, (rca << 16), response))
      return(FALSE);

   return(TRUE);
}


static int read_a_block(uint32_t block_number, uint32_t read_buffer[])
{
   uint16_t cmd, byte_count;
   uint32_t mci_status, response[4];
   uint32_t address;

   // TODO? Verify buffer address is mapped in the MMU.

   if (high_capacity == FALSE)
   {
       address = block_number * BLOCK_SIZE;
   }
   else
   {
       address = block_number;
   }

   // Set timeout and data length
   IO_WRITE32(sdcn.base + MCI_DATA_TIMER,  RD_DATA_TIMEOUT);
   IO_WRITE32(sdcn.base + MCI_DATA_LENGTH, BLOCK_SIZE);

   // Write data control register
   IO_WRITE32(sdcn.base + MCI_DATA_CTL, MCI_DATA_CTL__ENABLE___M |
                             MCI_DATA_CTL__DIRECTION___M |
                             (BLOCK_SIZE << MCI_DATA_CTL__BLOCKSIZE___S));

   // Send READ_SINGLE_BLOCK command
   cmd = CMD17 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   if (!sdcc_send_cmd(cmd, address, response))
      return(FALSE);

   // Read the block
   byte_count = 0;
   while (byte_count < BLOCK_SIZE)
   {
      mci_status = IO_READ32(sdcn.base + MCI_STATUS);
      if ((mci_status & MCI_STATUS__RXDATA_AVLBL___M) != 0)
      {
         *read_buffer = IO_READ32(sdcn.base + MCI_FIFO);
         read_buffer++;
         byte_count += 4;
      }

      else if ((mci_status & MCI_STATUS__RXACTIVE___M) == 0)
      {
         //Unexpected status on data read.
         return(FALSE);
      }
   }

   if (!check_clear_read_status())
   {
      return(FALSE);
   }

   return(TRUE);
}

static int read_a_block_dm(uint32_t block_number, uint32_t num_blocks, uint32_t read_buffer[])
{
   uint16_t cmd;
   uint32_t response[4];
   uint32_t address;
   uint32_t num_rows;
   uint32_t addr_shft;

   // TODO? Verify buffer address is mapped in the MMU.

   if (high_capacity == FALSE)
   {
       address = block_number * BLOCK_SIZE;
   }
   else
   {
       address = block_number;
   }

   // Set timeout and data length
   IO_WRITE32(sdcn.base + MCI_DATA_TIMER,  RD_DATA_TIMEOUT);
   IO_WRITE32(sdcn.base + MCI_DATA_LENGTH, BLOCK_SIZE * num_blocks);

   // Write data control register enabling DMA
   IO_WRITE32(sdcn.base + MCI_DATA_CTL, MCI_DATA_CTL__ENABLE___M |
                             MCI_DATA_CTL__DIRECTION___M |
                             MCI_DATA_CTL__DM_ENABLE___M |
                             (BLOCK_SIZE << MCI_DATA_CTL__BLOCKSIZE___S));

   // Send READ command, READ_MULT if more than one block requested.
   if (num_blocks == 1)
      cmd = CMD17 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   else
      cmd = CMD18 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   if (!sdcc_send_cmd(cmd, address, response))
      return(FALSE);

   // Initialize the DM Box mode command entry (single entry)
   // CRCI number is inserted for the source
   num_rows = ROWS_PER_BLOCK * num_blocks;
   sd_box_mode_entry[0] = (ADM_CMD_LIST_LC | (sdcn.adm_crci_num << 3) | ADM_ADDR_MODE_BOX);
   sd_box_mode_entry[1] = sdcn.base + MCI_FIFO;                 // SRC addr
   sd_box_mode_entry[2] = (uint32_t)read_buffer;                  // DST addr
   sd_box_mode_entry[3] = ((SDCC_FIFO_SIZE << 16) | (SDCC_FIFO_SIZE << 0)); // SRC/DST row len
   sd_box_mode_entry[4] = ((num_rows << 16) | (num_rows << 0));             // SRC/DST num rows
   sd_box_mode_entry[5] = ((0 << 16) | (SDCC_FIFO_SIZE << 0));              // SRC/DST offset

   // Initialize the DM Command Pointer List (single entry)
   addr_shft = ((uint32_t)(&sd_box_mode_entry[0])) >> 3;
   sd_adm_cmd_ptr_list[0] = (ADM_CMD_PTR_LP | ADM_CMD_PTR_CMD_LIST | addr_shft);

   // Start ADM transfer
   if (adm_start_transfer(ADM_AARM_SD_CHN, sd_adm_cmd_ptr_list) != 0)
   {
      return(FALSE);
   }

   if (num_blocks > 1)
   {
      // Send STOP_TRANSMISSION
      cmd = CMD12 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
      if (!sdcc_send_cmd(cmd, address, response))
         return(FALSE);
   }

   if (!check_clear_read_status())
   {
      return(FALSE);
   }

#ifdef CONFIG_DCACHE
   // Invalidate cache so buffer ADM updated can be seen.
   invalidate_dcache_range((uint32_t)read_buffer,
                           (uint32_t)read_buffer + (num_blocks *BLOCK_SIZE));
#endif

   return(TRUE);
}

static int write_a_block(uint32_t block_number, uint32_t write_buffer[], uint16_t rca)
{
   uint16_t cmd, byte_count;
   uint32_t mci_status, response[4];
   uint32_t address;

   // TODO? Verify buffer address is mapped in the MMU.

   if (high_capacity == FALSE)
   {
       address = block_number * BLOCK_SIZE;
   }
   else
   {
       address = block_number;
   }

   // Set timeout and data length
   IO_WRITE32(sdcn.base + MCI_DATA_TIMER,  WR_DATA_TIMEOUT);
   IO_WRITE32(sdcn.base + MCI_DATA_LENGTH, BLOCK_SIZE);

   // Send WRITE_BLOCK command
   cmd = CMD24 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   if (!sdcc_send_cmd(cmd, address, response))
      return(FALSE);

   // Write data control register
   IO_WRITE32(sdcn.base + MCI_DATA_CTL, MCI_DATA_CTL__ENABLE___M |
                             (BLOCK_SIZE << MCI_DATA_CTL__BLOCKSIZE___S));

   // Write the block
   byte_count = 0;
   while (byte_count < BLOCK_SIZE)
   {
      mci_status = IO_READ32(sdcn.base + MCI_STATUS);
      if ((mci_status & MCI_STATUS__TXFIFO_FULL___M) == 0)
      {
         IO_WRITE32(sdcn.base + MCI_FIFO, *write_buffer);
         write_buffer++;
         byte_count += 4;
      }

      if (mci_status & (MCI_STATUS__CMD_CRC_FAIL___M |
                        MCI_STATUS__DATA_TIMEOUT___M |
                        MCI_STATUS__TX_UNDERRUN___M ))
      {
         return(FALSE);
      }
   }

   if (!check_clear_write_status())
   {
      return(FALSE);
   }

   // Send SEND_STATUS command (with PROG_ENA, can poll on PROG_DONE below)
   cmd = CMD13 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M | MCI_CMD__PROG_ENA___M;
   if (!sdcc_send_cmd(cmd, (rca << 16), response))
      return(FALSE);

   // Wait for PROG_DONE
   while(!(IO_READ32(sdcn.base + MCI_STATUS) & MCI_STATUS__PROG_DONE___M));

   // Clear PROG_DONE and wait until cleared
   IO_WRITE32(sdcn.base + MCI_CLEAR, MCI_CLEAR__PROG_DONE_CLR___M);
   while(IO_READ32(sdcn.base + MCI_STATUS) & MCI_STATUS__PROG_DONE___M);

   return(TRUE);
}

static int write_a_block_dm(uint32_t block_number, uint32_t num_blocks,
                            uint32_t write_buffer[], uint16_t rca)
{
   uint16_t cmd;
   uint32_t response[4];
   uint32_t address;
   uint32_t addr_shft;

   // TODO? Verify buffer address is mapped in the MMU.

   if (high_capacity == FALSE)
   {
       address = block_number * BLOCK_SIZE;
   }
   else
   {
       address = block_number;
   }

   if (num_blocks != 1)
   {
      return(FALSE);    // ZZZZ need to add support for multiple block DM write
   }

   // Set timeout and data length
   IO_WRITE32(sdcn.base + MCI_DATA_TIMER,  WR_DATA_TIMEOUT);
   IO_WRITE32(sdcn.base + MCI_DATA_LENGTH, BLOCK_SIZE);

   // Send WRITE_BLOCK command
   cmd = CMD24 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M;
   if (!sdcc_send_cmd(cmd, address, response))
      return(FALSE);

   // Write data control register
   IO_WRITE32(sdcn.base + MCI_DATA_CTL, MCI_DATA_CTL__ENABLE___M |
                             MCI_DATA_CTL__DM_ENABLE___M |
                             (BLOCK_SIZE << MCI_DATA_CTL__BLOCKSIZE___S));

   // Initialize the DM Box mode command entry (single entry)
   // CRCI number is inserted for the destination
   sd_box_mode_entry[0] = (ADM_CMD_LIST_LC | (sdcn.adm_crci_num << 7) | ADM_ADDR_MODE_BOX);
   sd_box_mode_entry[1] = (uint32_t)write_buffer;        // SRC addr
   sd_box_mode_entry[2] = sdcn.base + MCI_FIFO;          // DST addr
   sd_box_mode_entry[3] = ((SDCC_FIFO_SIZE << 16) | (SDCC_FIFO_SIZE << 0)); // SRC/DST row len
   sd_box_mode_entry[4] = ((ROWS_PER_BLOCK << 16) | (ROWS_PER_BLOCK << 0)); // SRC/DST num rows
   sd_box_mode_entry[5] = ((SDCC_FIFO_SIZE << 16) | (0 << 0));              // SRC/DST offset

   // Initialize the DM Command Pointer List (single entry)
   addr_shft = ((uint32_t)(&sd_box_mode_entry[0])) >> 3;
   sd_adm_cmd_ptr_list[0] = (ADM_CMD_PTR_LP | ADM_CMD_PTR_CMD_LIST | addr_shft);

   // Start ADM transfer
   if (adm_start_transfer(ADM_AARM_SD_CHN, sd_adm_cmd_ptr_list) != 0)
   {
      return(FALSE);
   }

   if (!check_clear_write_status())
   {
      return(FALSE);
   }

   // Send SEND_STATUS command (with PROG_ENA, can poll on PROG_DONE below)
   cmd = CMD13 | MCI_CMD__ENABLE___M | MCI_CMD__RESPONSE___M | MCI_CMD__PROG_ENA___M;
   if (!sdcc_send_cmd(cmd, (rca << 16), response))
      return(FALSE);

   // Wait for PROG_DONE
   while(!(IO_READ32(sdcn.base + MCI_STATUS) & MCI_STATUS__PROG_DONE___M));

   // Clear PROG_DONE and wait until cleared
   IO_WRITE32(sdcn.base + MCI_CLEAR, MCI_CLEAR__PROG_DONE_CLR___M);
   while(IO_READ32(sdcn.base + MCI_STATUS) & MCI_STATUS__PROG_DONE___M);

   return(TRUE);
}


/*
 *  Set SD MCLK speed
 */
static int SD_MCLK_set(enum SD_MCLK_speed speed)
{
#ifndef USE_PROC_COMM
   uint32_t md;
   uint32_t ns;
   static int init_value_saved = FALSE;

   if (init_value_saved == FALSE)
   {
       // Save initial value of MD and NS regs for restoring in deinit
       sdcn.md_initial = IO_READ32(sdcn.md_addr);
       sdcn.ns_initial = IO_READ32(sdcn.ns_addr);
       init_value_saved = TRUE;
   }
#endif

    debug("BEFORE::SDC[%d]_NS_REG=0x%08x\n", sdcn.instance, IO_READ32(sdcn.ns_addr)) ;
    debug("BEFORE::SDC[%d]_MD_REG=0x%08x\n", sdcn.instance, IO_READ32(sdcn.md_addr)) ;

#ifdef USE_PROC_COMM
    //SDCn_NS_REG clk enable bits are turned on automatically as part of
    //setting clk speed. No need to enable sdcard clk explicitely
    pcom_set_sdcard_clk(sdcn.instance, speed);
    debug("clkrate_hz=%lu\n",pcom_get_sdcard_clk(sdcn.instance));
#else /*USE_PROC_COMM not defined */
   switch (speed)
   {
   case MCLK_400KHz :
       md = MCLK_MD_400KHZ;
       ns = MCLK_NS_400KHZ;
       break;
   case MCLK_25MHz :
       md = MCLK_MD_25MHZ;
       ns = MCLK_NS_25MHZ;
       break;
   case MCLK_48MHz :
       md = MCLK_MD_48MHZ;
       ns = MCLK_NS_48MHZ;
       break;
   case MCLK_50MHz :
       md = MCLK_MD_50MHZ;
       ns = MCLK_NS_50MHZ;
       break;
   default:
       printf("Unsupported Speed\n");
       return FALSE;

   }

   // Write to MCLK registers
   IO_WRITE32(sdcn.md_addr, md);
   IO_WRITE32(sdcn.ns_addr, ns);
#endif /*USE_PROC_COMM*/

   debug("AFTER::SDC[%d]_NS_REG=0x%08x\n", sdcn.instance, IO_READ32(sdcn.ns_addr)) ;
   debug("AFTER::SDC[%d]_MD_REG=0x%08x\n", sdcn.instance, IO_READ32(sdcn.md_addr)) ;

   return(TRUE);
}


/*
 * Initialize the specified SD card controller.
 */
static int SDCn_init(uint32_t instance)
{

   // Initialize SD structure based on the controller instance.
   sdcn.instance = instance;
   switch(instance)
   {
   case 1:
      sdcn.base = SDC1_BASE;
      sdcn.ns_addr = SDC1_NS_REG;
      sdcn.md_addr = SDC1_MD_REG;
      sdcn.row_reset_mask = ROW_RESET__SDC1___M;
      sdcn.glbl_clk_ena_mask = GLBL_CLK_ENA__SDC1_H_CLK_ENA___M;
      sdcn.adm_crci_num = ADM_CRCI_SDC1;
      break;
   case 2:
      sdcn.base = SDC2_BASE;
      sdcn.ns_addr = SDC2_NS_REG;
      sdcn.md_addr = SDC2_MD_REG;
      sdcn.row_reset_mask = ROW_RESET__SDC2___M;
      sdcn.glbl_clk_ena_mask = GLBL_CLK_ENA__SDC2_H_CLK_ENA___M;
      sdcn.adm_crci_num = ADM_CRCI_SDC2;
      break;
   case 3:
      sdcn.base = SDC3_BASE;
      sdcn.ns_addr = SDC3_NS_REG;
      sdcn.md_addr = SDC3_MD_REG;
      sdcn.row_reset_mask = ROW_RESET__SDC3___M;
      sdcn.glbl_clk_ena_mask = GLBL_CLK_ENA__SDC3_H_CLK_ENA___M;
      sdcn.adm_crci_num = ADM_CRCI_SDC3;
      break;
   case 4:
      sdcn.base = SDC4_BASE;
      sdcn.ns_addr = SDC4_NS_REG;
      sdcn.md_addr = SDC4_MD_REG;
      sdcn.row_reset_mask = ROW_RESET__SDC4___M;
      sdcn.glbl_clk_ena_mask = GLBL_CLK_ENA__SDC4_H_CLK_ENA___M;
      sdcn.adm_crci_num = ADM_CRCI_SDC4;
      break;
   default:
      return(FALSE);        // Error: incorrect instance number
   }

#ifdef USE_PROC_COMM
   //switch on sd card power. The voltage regulator used is board specific
   pcom_sdcard_power(1); //enable
#endif
   // Set the appropriate bit in GLBL_CLK_ENA to start the HCLK
   // Save the initial value of the bit for restoring later
#ifndef USE_PROC_COMM
   sdcn.glbl_clk_ena_initial = (sdcn.glbl_clk_ena_mask & IO_READ32(GLBL_CLK_ENA));
   debug("BEFORE:: GLBL_CLK_ENA=0x%08x\n",IO_READ32(GLBL_CLK_ENA));
   debug("sdcn.glbl_clk_ena_initial = %d\n", sdcn.glbl_clk_ena_initial);
   if (sdcn.glbl_clk_ena_initial == 0)
   {
       IO_WRITE32(GLBL_CLK_ENA, IO_READ32(GLBL_CLK_ENA) | sdcn.glbl_clk_ena_mask);
   }
   debug("AFTER_ENABLE:: GLBL_CLK_ENA=0x%08x\n",IO_READ32(GLBL_CLK_ENA));

#else /* USE_PROC_COMM defined */
   sdcn.glbl_clk_ena_initial =  pcom_is_sdcard_pclk_enabled(sdcn.instance);
   debug("sdcn.glbl_clk_ena_initial = %d\n", sdcn.glbl_clk_ena_initial);
   pcom_enable_sdcard_pclk(sdcn.instance);
   debug("AFTER_ENABLE:: sdc_clk_enable=%d\n",
          pcom_is_sdcard_pclk_enabled(sdcn.instance));
#endif /*USE_PROC_COMM*/

   // Set SD MCLK to 400KHz for card detection
   SD_MCLK_set(MCLK_400KHz);

#ifdef USE_DM
   // Remember the initial value for restore
   sdcn.adm_ch8_rslt_conf_initial = IO_READ32(HI0_CH8_RSLT_CONF_SD3);
#endif

   // Configure GPIOs using proc_comm
   sdcard_gpio_config(sdcn.instance);
   // Clear all status bits
   IO_WRITE32(sdcn.base + MCI_CLEAR, 0x07FFFFFF);

   // Disable all interrupts sources
   IO_WRITE32(sdcn.base + MCI_INT_MASK0, 0x0);
   IO_WRITE32(sdcn.base + MCI_INT_MASK1, 0x0);

   // Power control to the card, enable MCICLK with power save mode
   // disabled, otherwise the initialization clock cycles will be
   // shut off and the card will not initialize.
   IO_WRITE32(sdcn.base + MCI_POWER, MCI_POWER__CONTROL__POWERON);
   IO_WRITE32(sdcn.base + MCI_CLK, (MCI_CLK__SELECT_IN__ON_THE_FALLING_EDGE_OF_MCICLOCK << MCI_CLK__SELECT_IN___S) |
                        MCI_CLK__ENABLE___M );

   // Delay
   udelay(1000);

   return(TRUE);
}

void SDCn_deinit(uint32_t instance)
{
    uint16_t cmd;
    uint32_t response[4] = {0};

    if (mmc_ready != 0)
    {

        // CMD0 - put the card in the idle state
        cmd = CMD0 | MCI_CMD__ENABLE___M;
        if (!sdcc_send_cmd(cmd, 0x00000000, response))
           debug("MMC error resetting card\n");

// no guarantee that the initial clk rate saved would
// be settable by proc_comm. Hence, not restoring
#ifndef USE_PROC_COMM
        // Restore the SD MCLK clock control registers to initial value
        IO_WRITE32(sdcn.md_addr, sdcn.md_initial);
        IO_WRITE32(sdcn.ns_addr, sdcn.ns_initial);
#endif

        // Assert ROW_RESET on the SD controller??
        // ZZZZ ??


        // Clear the SDCn global clock enable bit it if was 0 to start
        if (sdcn.glbl_clk_ena_initial == 0)
        {
#ifdef USE_PROC_COMM
	    pcom_disable_sdcard_pclk(sdcn.instance);
	    //verify
            debug("AFTER_ENABLE:: sdc_clk_enable=%d\n",
                   pcom_is_sdcard_pclk_enabled(sdcn.instance));
#else
            IO_WRITE32(GLBL_CLK_ENA, IO_READ32(GLBL_CLK_ENA) & ~sdcn.glbl_clk_ena_mask);
#endif /*USE_PROC_COMM*/
        }


        // ZZZZ TLMM

#ifdef USE_DM
        // Restore initial value
        IO_WRITE32(HI0_CH8_RSLT_CONF_SD3, sdcn.adm_ch8_rslt_conf_initial);
#endif

    }
#ifdef USE_PROC_COMM
    //sd power was unconditionally switched on ..
    //so switch off unconditionally
    pcom_sdcard_power(0); //disable
#endif
}

static void sdcard_gpio_config(int instance)
{

#ifndef USE_PROC_COMM
   uint32_t io_drive = 0x3;   // 8mA
   switch(instance)
   {
   case 1:
      // Configure the general purpose I/O for SDC1
      IO_WRITE32(GPIO1_PAGE, 55);
      IO_WRITE32(GPIO1_CFG, (0x1 << 2) | 0x3 | (io_drive << 6));

      IO_WRITE32(GPIO1_PAGE, 56);
      IO_WRITE32(GPIO1_CFG, (0x1 << 2) | 0x3 | (io_drive << 6));

      IO_WRITE32(GPIO1_PAGE, 54);
      IO_WRITE32(GPIO1_CFG, (0x1 << 2) | 0x3 | (io_drive << 6));

      IO_WRITE32(GPIO1_PAGE, 53);
      IO_WRITE32(GPIO1_CFG, (0x1 << 2) | 0x3 | (io_drive << 6));

      IO_WRITE32(GPIO1_PAGE, 52);
      IO_WRITE32(GPIO1_CFG, (0x1 << 2) | 0x3 | (io_drive << 6));

      IO_WRITE32(GPIO1_PAGE, 51);
      IO_WRITE32(GPIO1_CFG, (0x1 << 2) | 0x3 | (io_drive << 6));
      break;

   case 2:
      // Configure the general purpose I/O for SDC2
      IO_WRITE32(GPIO1_PAGE, 62);
      IO_WRITE32(GPIO1_CFG, (0x1 << 2) | 0x3 | (io_drive << 6));

      IO_WRITE32(GPIO1_PAGE, 63);
      IO_WRITE32(GPIO1_CFG, (0x1 << 2) | 0x3 | (io_drive << 6));

      IO_WRITE32(GPIO1_PAGE, 64);
      IO_WRITE32(GPIO1_CFG, (0x1 << 2) | 0x3 | (io_drive << 6));

      IO_WRITE32(GPIO1_PAGE, 65);
      IO_WRITE32(GPIO1_CFG, (0x1 << 2) | 0x3 | (io_drive << 6));

      IO_WRITE32(GPIO1_PAGE, 66);
      IO_WRITE32(GPIO1_CFG, (0x1 << 2) | 0x3 | (io_drive << 6));

      IO_WRITE32(GPIO1_PAGE, 67);
      IO_WRITE32(GPIO1_CFG, (0x1 << 2) | 0x3 | (io_drive << 6));
      break;

   case 3:
      // not working yet, this is an MMC socket on the SURF.
      break;

   case 4:
      // Configure the general purpose I/O for SDC4
      IO_WRITE32(GPIO1_PAGE, 142);
      IO_WRITE32(GPIO1_CFG, (0x3 << 2) | 0x3 | (io_drive << 6));

      IO_WRITE32(GPIO1_PAGE, 143);
      IO_WRITE32(GPIO1_CFG, (0x3 << 2) | 0x3 | (io_drive << 6));

      IO_WRITE32(GPIO1_PAGE, 144);
      IO_WRITE32(GPIO1_CFG, (0x2 << 2) | 0x3 | (io_drive << 6));

      IO_WRITE32(GPIO1_PAGE, 145);
      IO_WRITE32(GPIO1_CFG, (0x2 << 2) | 0x3 | (io_drive << 6));

      IO_WRITE32(GPIO1_PAGE, 146);
      IO_WRITE32(GPIO1_CFG, (0x3 << 2) | 0x3 | (io_drive << 6));

      IO_WRITE32(GPIO1_PAGE, 147);
      IO_WRITE32(GPIO1_CFG, (0x3 << 2) | 0x3 | (io_drive << 6));
      break;
}
#else /*USE_PROC_COMM defined */
	pcom_sdcard_gpio_config(instance);
#endif /*USE_PROC_COMM*/
} /*sdcard_gpio_config()*/
