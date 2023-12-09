/** @file

  Copyright (c) 2011 - 2020, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PREPI_H_
#define _PREPI_H_

#include <PiPei.h>

#include <Library/PcdLib.h>
#include <Library/ArmLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HobLib.h>
#include <Library/SerialPortLib.h>
#include <Library/ArmPlatformLib.h>

extern UINT64  mSystemMemoryEnd;

/* some BGRA8888 color definitions */
#define FB_BGRA8888_BLACK 0xff000000
#define FB_BGRA8888_WHITE 0xffffffff
#define FB_BGRA8888_CYAN 0xff00ffff
#define FB_BGRA8888_BLUE 0xff0000ff
#define FB_BGRA8888_SILVER 0xffc0c0c0
#define FB_BGRA8888_YELLOW 0xffffff00
#define FB_BGRA8888_ORANGE 0xffffa500
#define FB_BGRA8888_RED 0xffff0000
#define FB_BGRA8888_GREEN 0xff00ff00

/* MDP-related defines */
#define MSM_MDP_BASE1 	0xAA200000
#define LCDC_BASE     	0xE0000

//#define BIT(x)  (1<<(x))

// from https://gitlab.utc.fr/mi11/linux/linux-raspberrypi/-/blob/a26c4d1f1428082f6aba79207535527c91115c76/include/linux/msm_mdp.h
enum {
	MDP_RGB_565,		/* RGB 565 planar */
	MDP_XRGB_8888,		/* RGB 888 padded */
	MDP_Y_CBCR_H2V2,	/* Y and CbCr, pseudo planar w/ Cb is in MSB */
	MDP_ARGB_8888,		/* ARGB 888 */
	MDP_RGB_888,		/* RGB 888 planar */
	MDP_Y_CRCB_H2V2,	/* Y and CrCb, pseudo planar w/ Cr is in MSB */
	MDP_YCRYCB_H2V1,	/* YCrYCb interleave */
	MDP_Y_CRCB_H2V1,	/* Y and CrCb, pseduo planar w/ Cr is in MSB */
	MDP_Y_CBCR_H2V1,	/* Y and CrCb, pseduo planar w/ Cr is in MSB */
	MDP_RGBA_8888,		/* ARGB 888 */
	MDP_BGRA_8888,		/* ABGR 888 */
	MDP_RGBX_8888,		/* RGBX 888 */
	MDP_IMGTYPE_LIMIT	/* Non valid image type after this enum */
};


/* MDP 3.1 */

#define DMA_DSTC0G_8BITS (BIT(1)|BIT(0))
#define DMA_DSTC1B_8BITS (BIT(3)|BIT(2))
#define DMA_DSTC2R_8BITS (BIT(5)|BIT(4))
#define DMA_DSTC3A_8BITS (BIT(7)|BIT(6))
//#define DMA_DSTC0G_8BITS (3<<0)
//#define DMA_DSTC1B_8BITS (3<<2)
//#define DMA_DSTC2R_8BITS (3<<4)

#define CLR_G 0x0
#define CLR_B 0x1
#define CLR_R 0x2
#define CLR_ALPHA 0x3
#define MDP_GET_PACK_PATTERN(a,x,y,z,bit) (((a)<<(bit*3))|((x)<<(bit*2))|((y)<<bit)|(z))
#define DMA_PACK_TIGHT                      BIT(6)
#define DMA_PACK_LOOSE                      0
#define DMA_PACK_ALIGN_LSB                  0
#define DMA_PACK_PATTERN_RGB				\
        (MDP_GET_PACK_PATTERN(0,CLR_R,CLR_G,CLR_B, 2)<<8)
#define DMA_PACK_PATTERN_BGR \
        (MDP_GET_PACK_PATTERN(0, CLR_B, CLR_G, CLR_R, 2)<<8)
/* from https://nv-tegra-prod.hwinf-scm-aws.nvidia.com/r/plugins/gitiles/linux-3.10/+/a724eada8c2a7b62463b73ccf73fd0bb6e928aeb/drivers/video/msm/mdp_hw.h */
#define PPP_PACK_PATTERN_MDP_RGBA_8888 \
	      MDP_GET_PACK_PATTERN(CLR_ALPHA, CLR_B, CLR_G, CLR_R, 8)

#define PPP_PACK_PATTERN_MDP_BGRA_8888 \
	MDP_GET_PACK_PATTERN(CLR_ALPHA, CLR_R, CLR_G, CLR_B, 8)
/* end */

#define DMA_PACK_PATTERN_BGRA \
        (MDP_GET_PACK_PATTERN(CLR_ALPHA, CLR_B, CLR_G, CLR_R, 2)<<8)

#define DMA_DITHER_EN                         BIT(24)
#define DMA_OUT_SEL_LCDC                      BIT(20)
#define DMA_IBUF_FORMAT_RGB888			          (0 << 25)
#define DMA_IBUF_FORMAT_RGB565			          (1 << 25)
#define DMA_IBUF_FORMAT_XRGB8888		          (2 << 25)
#define DMA_IBUF_FORMAT_xRGB8888_OR_ARGB8888  BIT(26)
#define DMA_IBUF_FORMAT_MASK			            (3 << 25)

/* MDP 3.1 end*/

/* SURF */
#define LCDC_FB_WIDTH     800
#define LCDC_FB_HEIGHT    480
#define LCDC_HSYNC_PULSE_WIDTH_DCLK 60
#define LCDC_HSYNC_BACK_PORCH_DCLK  81
#define LCDC_HSYNC_FRONT_PORCH_DCLK 81
#define LCDC_HSYNC_SKEW_DCLK        0
#define LCDC_VSYNC_PULSE_WIDTH_LINES 2
#define LCDC_VSYNC_BACK_PORCH_LINES  20
#define LCDC_VSYNC_FRONT_PORCH_LINES 27
/* SURF end */

/* MDP regs */
#define REG_MDP(offset)                       MSM_MDP_BASE1 + offset

#define MDP_DMA_P_CONFIG                      REG_MDP(0x90000)
#define MDP_DMA_P_OUT_XY                      REG_MDP(0x90010)
#define MDP_DMA_P_SIZE                        REG_MDP(0x90004)
#define MDP_DMA_P_BUF_ADDR                    REG_MDP(0x90008)
#define MDP_DMA_P_BUF_Y_STRIDE                REG_MDP(0x9000C)
#define MDP_DMA_P_OP_MODE                     REG_MDP(0x90070)

RETURN_STATUS
EFIAPI
TimerConstructor (
  VOID
  );

VOID
PrePiMain (
  IN  UINTN   UefiMemoryBase,
  IN  UINTN   StacksBase,
  IN  UINT64  StartTimeStamp
  );

EFI_STATUS
EFIAPI
MemoryPeim (
  IN EFI_PHYSICAL_ADDRESS  UefiMemoryBase,
  IN UINT64                UefiMemorySize
  );

EFI_STATUS
EFIAPI
PlatformPeim (
  VOID
  );

VOID
PrimaryMain (
  IN  UINTN   UefiMemoryBase,
  IN  UINTN   StacksBase,
  IN  UINT64  StartTimeStamp
  );

VOID
SecondaryMain (
  IN  UINTN  MpId
  );

// Either implemented by PrePiLib or by MemoryInitPei
VOID
BuildMemoryTypeInformationHob (
  VOID
  );

EFI_STATUS
GetPlatformPpi (
  IN  EFI_GUID  *PpiGuid,
  OUT VOID      **Ppi
  );

// Initialize the Architecture specific controllers
VOID
ArchInitialize (
  VOID
  );

VOID
EFIAPI
ProcessLibraryConstructorList (
  VOID
  );

#endif /* _PREPI_H_ */
