/*
 * QSD8x50_reg.h
 * QSD8x50 Register and Bit definitions
 *
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 * 
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a 
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 * 
 * START
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * 
 * END
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __QC_8x50_REG_H
#define __QC_8x50_REG_H

#define NAND_FLASH_CMD (0xA0A00000)
#define NAND_FLASH_CMD__LAST_PAGE___M 0x00000020
#define NAND_FLASH_CMD__PAGE_ACC___M 0x00000010
#define NAND_FLASH_CMD__OP_CMD__PAGE_READ 0x2
#define NAND_FLASH_CMD__OP_CMD__PAGE_READ_WITH_ECC 0x3
#define NAND_FLASH_CMD__OP_CMD__FETCH_ID 0xB
#define NAND_FLASH_CMD__OP_CMD__RESET_NAND_FLASH_DEVICE_OR_ONENAND_REGISTER_WRI 0xD
#define NAND_FLASH_CHIP_SELECT (0xA0A0000C)
#define NAND_FLASH_CHIP_SELECT__DM_EN___M 0x00000004
#define NANDC_EXEC_CMD (0xA0A00010)
#define NANDC_EXEC_CMD__EXEC_CMD__EXECUTE_THE_COMMAND 0x1
#define NAND_FLASH_STATUS (0xA0A00014)
#define NAND_FLASH_STATUS__OP_ERR___M 0x00000010
#define NAND_DEVn_CFG0(n) (0xA0A00020+0x10*n)
#define NAND_DEV0_CFG0 (0xA0A00020)
#define NAND_DEVn_CFG1(n) (0xA0A00024+0x10*n)
#define NAND_FLASH_READ_ID (0xA0A00040)
#define FLASH_DEV_CMD_VLD (0xA0A000AC)
#define FLASH_DEV_CMD_VLD__SEQ_READ_START_VLD___M 0x00000010
#define FLASH_DEV_CMD_VLD__ERASE_START_VLD___M 0x00000008
#define FLASH_DEV_CMD_VLD__WRITE_START_VLD___M 0x00000004
#define FLASH_DEV_CMD_VLD__READ_START_VLD___M 0x00000001
#define SFLASHC_BURST_CFG (0xA0A000E0)
#define EBI2_ECC_BUF_CFG (0xA0A000F0)
#define FLASH_BUFF0_ACC (0xA0A00100)
#define USBH1_USB_OTG_HS_AHB_MODE (0xA0800098)
#define USBH1_USB_OTG_HS_CAPLENGTH (0xA0800100)
#define USBH1_USB_OTG_HS_ULPI_VIEWPORT (0xA0800170)
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIWU___M 0x80000000
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIRUN___M 0x40000000
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIRW___M 0x20000000
#define USB_OTG_HS_ULPI_VIEWPORT__ULPISS___M 0x08000000
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIPORT___S 24
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIADDR___S 16
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIDATRD___M 0x0000FF00
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIDATRD___S 8
#define USBH1_USB_OTG_HS_PORTSC (0xA0800184)
#define USB_OTG_HS_PORTSC__PP___M 0x00001000
#define USBH1_USB_OTG_HS_USBMODE (0xA08001A8)
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIWU___M 0x80000000
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIRUN___M 0x40000000
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIRW___M 0x20000000
#define USB_OTG_HS_ULPI_VIEWPORT__ULPISS___M 0x08000000
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIPORT___S 24
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIADDR___S 16
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIDATRD___M 0x0000FF00
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIDATRD___S 8
#define USB_OTG_HS_PORTSC__PP___M 0x00001000
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIWU___M 0x80000000
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIRUN___M 0x40000000
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIRW___M 0x20000000
#define USB_OTG_HS_ULPI_VIEWPORT__ULPISS___M 0x08000000
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIPORT___S 24
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIADDR___S 16
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIDATRD___M 0x0000FF00
#define USB_OTG_HS_ULPI_VIEWPORT__ULPIDATRD___S 8
#define USB_OTG_HS_PORTSC__PP___M 0x00001000

/* SD Card Controller (SDC) Registers */
#define SDC4_BASE (0xA0600000)
#define SDC3_BASE (0xA0500000)
#define SDC2_BASE (0xA0400000)
#define SDC1_BASE (0xA0300000)

/* SD controller register offsets */
#define MCI_POWER               (0x000)
#define MCI_CLK                 (0x004)
#define MCI_ARGUMENT            (0x008)
#define MCI_CMD                 (0x00C)
#define MCI_RESP_CMD            (0x010)
#define MCI_RESPn(n)            (0x014+4*n)
#define MCI_RESP0               (0x014)
#define MCI_RESP1               (0x018)
#define MCI_RESP2               (0x01C)
#define MCI_RESP3               (0x020)
#define MCI_DATA_TIMER          (0x024)
#define MCI_DATA_LENGTH         (0x028)
#define MCI_DATA_CTL            (0x02C)
#define MCI_DATA_COUNT          (0x030)
#define MCI_STATUS              (0x034)
#define MCI_CLEAR               (0x038)
#define MCI_INT_MASKn(n)        (0x03C+4*n)
#define MCI_INT_MASK0           (0x03C)
#define MCI_INT_MASK1           (0x040)
#define MCI_FIFO_COUNT          (0x044)
#define MCI_CCS_TIMER           (0x058)
#define MCI_FIFO                (0x080)
#define MCI_TESTBUS_CONFIG      (0x0CC)
#define MCI_TEST_CTL            (0x0D0)
#define MCI_TEST_INPUT          (0x0D4)
#define MCI_TEST_OUT            (0x0D8)
#define MCI_PERPH_ID0           (0x0E0)
#define MCI_PERPH_ID1           (0x0E4)
#define MCI_PERPH_ID2           (0x0E8)
#define MCI_PERPH_ID3           (0x0EC)
#define MCI_PCELL_ID0           (0x0F0)
#define MCI_PCELL_ID1           (0x0F4)
#define MCI_PCELL_ID2           (0x0F8)
#define MCI_PCELL_ID3           (0x0FC)

/* SDCC Register masks, shifts */
#define MCI_POWER__CONTROL__POWERON 0x3
#define MCI_CLK__SELECT_IN___M 0x0000C000
#define MCI_CLK__SELECT_IN___S 14
#define MCI_CLK__SELECT_IN__ON_THE_FALLING_EDGE_OF_MCICLOCK 0x0
#define MCI_CLK__SELECT_IN__USING_FEEDBACK_CLOCK 0x2
#define MCI_CLK__FLOW_ENA___M 0x00001000
#define MCI_CLK__WIDEBUS___M 0x00000C00
#define MCI_CLK__WIDEBUS___S 10
#define MCI_CLK__WIDEBUS__4_BIT_MODE 0x2
#define MCI_CLK__PWRSAVE___M 0x00000200
#define MCI_CLK__ENABLE___M 0x00000100
#define MCI_CMD__DAT_CMD___M  0x00001000
#define MCI_CMD__PROG_ENA___M 0x00000800
#define MCI_CMD__ENABLE___M 0x00000400
#define MCI_CMD__INTERRUPT___M 0x00000100
#define MCI_CMD__LONGRSP___M 0x00000080
#define MCI_CMD__RESPONSE___M 0x00000040
#define MCI_CMD__CMD_INDEX___M 0x0000003F
#define MCI_DATA_CTL__BLOCKSIZE___S 4
#define MCI_DATA_CTL__DM_ENABLE___M 0x00000008
#define MCI_DATA_CTL__DIRECTION___M 0x00000002
#define MCI_DATA_CTL__ENABLE___M 0x00000001
#define MCI_STATUS__PROG_DONE___M 0x00800000
#define MCI_STATUS__SDIO_INTR___M 0x00400000
#define MCI_STATUS__RXDATA_AVLBL___M 0x00200000
#define MCI_STATUS__TXFIFO_FULL___M 0x00010000
#define MCI_STATUS__RXACTIVE___M 0x00002000
#define MCI_STATUS__DATA_BLK_END___M 0x00000400
#define MCI_STATUS__DATA_BLK_END___S 10
#define MCI_STATUS__START_BIT_ERR___M 0x00000200
#define MCI_STATUS__START_BIT_ERR___S 9
#define MCI_STATUS__DATAEND___M 0x00000100
#define MCI_STATUS__CMD_SENT___M 0x00000080
#define MCI_STATUS__CMD_RESPONSE_END___M 0x00000040
#define MCI_STATUS__CMD_RESPONSE_END___S 6
#define MCI_STATUS__RX_OVERRUN___M 0x00000020
#define MCI_STATUS__RX_OVERRUN___S 5
#define MCI_STATUS__TX_UNDERRUN___M 0x00000010
#define MCI_STATUS__TX_UNDERRUN___S 4
#define MCI_STATUS__DATA_TIMEOUT___M 0x00000008
#define MCI_STATUS__DATA_TIMEOUT___S 3
#define MCI_STATUS__CMD_TIMEOUT___M 0x00000004
#define MCI_STATUS__CMD_TIMEOUT___S 2
#define MCI_STATUS__DATA_CRC_FAIL___M 0x00000002
#define MCI_STATUS__DATA_CRC_FAIL___S 1
#define MCI_STATUS__CMD_CRC_FAIL___M 0x00000001
#define MCI_STATUS__CMD_CRC_FAIL___S 0
#define MCI_CLEAR__PROG_DONE_CLR___M 0x00800000
#define MCI_CLEAR__SDIO_INTR_CLR___M 0x00400000
#define MCI_CLEAR__DATA_BLK_END_CLR___M 0x00000400
#define MCI_CLEAR__START_BIT_ERR_CLR___M 0x00000200
#define MCI_CLEAR__DATA_END_CLR___M 0x00000100
#define MCI_CLEAR__CMD_SENT_CLR___M 0x00000080
#define MCI_CLEAR__CMD_RESP_END_CLT___M 0x00000040
#define MCI_CLEAR__RX_OVERRUN_CLR___M 0x00000020
#define MCI_CLEAR__TX_UNDERRUN_CLR___M 0x00000010
#define MCI_CLEAR__DATA_TIMEOUT_CLR___M 0x00000008
#define MCI_CLEAR__CMD_TIMOUT_CLR___M 0x00000004
#define MCI_CLEAR__DATA_CRC_FAIL_CLR___M 0x00000002
#define MCI_CLEAR__CMD_CRC_FAIL_CLR___M 0x00000001

#define EBI2CS7_BASE                 (0x70000000)
#define EBI2CS6_BASE                 (0x60000000)
#define EBI2CS5_BASE                 (0x94000000)
#define EBI2CS4_BASE                 (0x90000000)
#define EBI2CS3_BASE                 (0x8C000000)
#define EBI2CS2_BASE                 (0x88000000)
#define EBI2CS1_BASE                 (0x84000000)
#define EBI2CS0_BASE                 (0x80000000)

#define MDP_LCDC_EN (0xAA2E0000)
#define MDP_LCDC_HSYNC_CTL (0xAA2E0004)
#define MDP_LCDC_VSYNC_PERIOD (0xAA2E0008)
#define MDP_LCDC_VSYNC_PULSE_WIDTH (0xAA2E000C)
#define MDP_LCDC_DISPLAY_HCTL (0xAA2E0010)
#define MDP_LCDC_DISPLAY_V_START (0xAA2E0014)
#define MDP_LCDC_DISPLAY_V_END (0xAA2E0018)
#define MDP_LCDC_ACTIVE_HCTL (0xAA2E001C)
#define MDP_LCDC_ACTIVE_V_START (0xAA2E0020)
#define MDP_LCDC_ACTIVE_V_END (0xAA2E0024)
#define MDP_LCDC_BORDER_CLR (0xAA2E0028)
#define MDP_LCDC_UNDERFLOW_CTL (0xAA2E002C)
#define MDP_LCDC_HSYNC_SKEW (0xAA2E0030)
#define MDP_LCDC_CTL_POLARITY (0xAA2E0038)
#define MDP_DMA_P_CONFIG (0xAA290000)
#define MDP_DMA_P_SIZE (0xAA290004)
#define MDP_DMA_P_IBUF_ADDR (0xAA290008)
#define MDP_DMA_P_IBUF_Y_STRIDE (0xAA29000C)
#define MDP_DMA_P_OUT_XY (0xAA290010)
#define MDP_DMA_P_OP_MODE (0xAA290070)

#define HI0_CHn_CMD_PTR_SD3(n) (0xA9700C00+4*n)
#define HI0_CHn_RSLT_SD3(n) (0xA9700C40+4*n)
#define HI0_CHn_RSLT_SD3__V___M 0x80000000
#define HI0_CHn_RSLT_SD3__ERR___M 0x00000008
#define HI0_CHn_RSLT_SD3__TPD___M 0x00000002
#define HI0_CH8_RSLT_CONF_SD3 (0xA9700F20)
#define HI0_CHn_STATUS_SD3(n) (0xA9700E00+4*n)
#define HI0_CHn_STATUS_SD3__RSLT_VLD___M 0x00000002

#define UART3_BASE (0xA9C00000)
#define UART_SR__TXRDY___M 0x00000004
#define UART_SR__RXRDY___M 0x00000001
#define UART2_BASE (0xA9B00000)
#define UART_SR__TXRDY___M 0x00000004
#define UART_SR__RXRDY___M 0x00000001
#define UART1_BASE (0xA9A00000)
#define UART_SR__TXRDY___M 0x00000004
#define UART_SR__RXRDY___M 0x00000001
#define UART_NS_REG (0xA86000C0)
#define UART_NS_REG__UART3_SRC_SEL___M 0x00007000
#define UART_NS_REG__UART3_SRC_SEL___S 12
#define UART_NS_REG__UART2_SRC_SEL___M 0x000001C0
#define UART_NS_REG__UART2_SRC_SEL___S 6
#define UART_NS_REG__UART1_SRC_SEL___M 0x00000007
#define UART_NS_REG__UART1_SRC_SEL___S 0

#define PRPH_WEB_NS_REG (0xA8600080)
#define MSS_RESET (0xA8600204)
#define APPS_RESET (0xA8600214)
#define APPS_RESET__USB_PHY___M 0x00040000
#define APPS_RESET__USBH___M 0x00008000
#define GPIO1_PAGE (0xA8E00040)
#define GPIO1_CFG (0xA8E00044)
#define GLBL_CLK_ENA (0xA8600000)
#define GLBL_CLK_ENA__SDC4_H_CLK_ENA___M 0x10000000
#define GLBL_CLK_ENA__SDC3_H_CLK_ENA___M 0x08000000
#define GLBL_CLK_ENA__SDC2_H_CLK_ENA___M 0x00000100
#define GLBL_CLK_ENA__SDC1_H_CLK_ENA___M 0x00000080
#define ROW_RESET (0xA8600218)
#define ROW_RESET__SDC4___M 0x10000000
#define ROW_RESET__SDC3___M 0x08000000
#define ROW_RESET__SDC1___M 0x00000200
#define ROW_RESET__SDC2___M 0x00000100
#define SDC1_MD_REG (0xA86000A0)
#define SDC1_NS_REG (0xA86000A4)
#define SDC2_MD_REG (0xA86000A8)
#define SDC2_NS_REG (0xA86000AC)
#define SDC3_MD_REG (0xA86003D4)
#define SDC3_NS_REG (0xA86003D8)
#define SDC4_MD_REG (0xA86003DC)
#define SDC4_NS_REG (0xA86003E0)
#define USBH_NS_REG (0xA86003E8)
#define LCD_MD_REG (0xA86003EC)
#define LCD_NS_REG (0xA86003F0)
#define PLL_DEBUG (0xA8800000)
#define PLL_CTL (0xA8800004)
#define PLL_CTL__BYPASSNL___M 0x00400000
#define PLL_CTL__RESET_N___M 0x00200000
#define PLL_CTL__PLL_MODE__POWER_DOWN 0x0
#define PLL_CTL__PLL_MODE__STAND_BY 0x2
#define PLL_CTL__PLL_MODE__FULL_CALIBRATION 0x4
#define PLL_CTL__PLL_MODE__NORMAL_OPERATION 0x7
#define PLL_FSM_CTL_EXT (0xA8800010)
#define PLL_FSM_CTL_EXT__TARG_L_VAL___S 3
#define PLL_FSM_CTL_EXT__FRESWI_MODE__SHOT 0x4
#define PLL_FSM_CTL_EXT__FRESWI_MODE__HOP 0x5
#define PLL_STATUS (0xA8800018)
#define PLL_STATUS__CAL_ALL_DONE_N___M 0x00000002
#define PLL_TEST_CTL (0xA880001C)
#define PLL_INTERNAL (0xA8800020)
#define AGPT_MATCH_VAL (0xAC100000)
#define AGPT_COUNT_VAL (0xAC100004)
#define AGPT_ENABLE (0xAC100008)
#define AGPT_ENABLE__CLR_ON_MATCH_EN___M 0x00000002
#define AGPT_ENABLE__EN___M 0x00000001
#define AGPT_CLEAR (0xAC10000C)
#define AST_ENABLE (0xAC10002C)
#define SPSS_CLK_SEL (0xAC100104)
#define TCSR_SPARE2 (0xA8700060)
#define A2M_INT0 (0xAC100400)
#define A2M_INT1 (0xAC100404)
#define A2M_INT2 (0xAC100408)
#define A2M_INT3 (0xAC10040C)
#define A2M_INT4 (0xAC100410)
#define A2M_INT5 (0xAC100414)
#define A2M_INT6 (0xAC100418)

#endif   /* __QC_8x50_REG_H */
