/*
 * recycled from arch/arm/mach-msm/include/mach/msm_iomap-8x50.h
 *
 * Copyright (C) 2007 Google, Inc.
 * Author: Brian Swetland <swetland@google.com>
 *
 * The MSM peripherals are spread all over across 768MB of physical space.
 *
 */

#ifndef _PLATFORM_QSD8K_IOMAP_H_
#define _PLATFORM_QSD8K_IOMAP_H_

#define MSM_SHARED_BASE       0x00100000

#define MSM_ADM_BASE          0xA9700000 /* same as dmov, qsd8x50 has 1 adm as confirmed by qualcomm employee */
#define MSM_ADM_SD_OFFSET     0x00000400

#define MSM_SDC1_BASE         0xA0300000
#define MSM_SDC2_BASE         0xA0400000 /* sd slot */
#define MSM_SDC3_BASE         0xA0500000
#define MSM_SDC4_BASE         0xA0600000
#define MSM_HSUSB_BASE        0xA0800000
#define MSM_USB_BASE          0xA0800000
#define MSM_VFE_BASE          0xA0F00000
#define MSM_SSBI_BASE         0xA8100000
#define MSM_AXI_BASE	      0xA8200000
#define MSM_AXIGS_BASE        0xA8250000
#define MSM_IMEM_BASE	      0xA8500000
#define MSM_CLK_CTL_BASE      0xA8600000
#define MSM_SCPLL_BASE        0xA8800000
#define SCPLL_CTL             0xA8800004
#define SCPLL_CTLE            0xA8800010
#define SCPLL_STAT            0xA8800018
#define MSM_GPIOCFG1_BASE     0xA8E00000
#define MSM_GPIOCFG2_BASE     0xA8F00000
#define MSM_GPIO1_BASE        0xA9000000
#define MSM_GPIO2_BASE        0xA9100000
#define MSM_DMOV_BASE         0xA9700000
#define MSM_I2C_BASE          0xA9900000
#define MSM_UART1_BASE        0xA9A00000
#define MSM_UART2_BASE        0xA9B00000
#define MSM_UART3_BASE        0xA9C00000
#define MSM_MDP_BASE          0xAA200000
#define MSM_CLK_CTL_SH2_BASE  0xABA01000
#define MSM_VIC_BASE          0xAC000000
#define MSM_SIRC_BASE         0xAC200000
#define MSM_CSR_BASE          0xAC100000
#define MSM_GPT_BASE          0xAC100000
#define A11S_CLK_CNTL         0xAC100100
#define A11S_CLK_SEL          0xAC100104

#define GPT_REG(off)         (MSM_GPT_BASE + (off))
#define GPT_MATCH_VAL        GPT_REG(0x0000)
#define GPT_COUNT_VAL        GPT_REG(0x0004)
#define GPT_ENABLE           GPT_REG(0x0008)
#define GPT_CLEAR            GPT_REG(0x000C)
#define DGT_MATCH_VAL        GPT_REG(0x0010)
#define DGT_COUNT_VAL        GPT_REG(0x0014)
#define DGT_ENABLE           GPT_REG(0x0018)
#define DGT_CLEAR            GPT_REG(0x001C)
#define SPSS_TIMER_STATUS    GPT_REG(0x0034)

#endif