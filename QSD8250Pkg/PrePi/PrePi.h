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
#define MDP_LCDC_EN (0xAA2E0000)

/* MDP 3.1 */
#define DMA_DSTC0G_8BITS (3<<0)
#define DMA_DSTC1B_8BITS (3<<2)
#define DMA_DSTC2R_8BITS (3<<4)

#define CLR_G 0x0
#define CLR_B 0x1
#define CLR_R 0x2
#define CLR_ALPHA 0x3

#define MDP_GET_PACK_PATTERN(a,x,y,z,bit) (((a)<<(bit*3))|((x)<<(bit*2))|((y)<<bit)|(z))
#define DMA_PACK_TIGHT                      (1 << 6)
#define DMA_PACK_LOOSE                      0
#define DMA_PACK_ALIGN_LSB                  0

#define DMA_PACK_PATTERN_RGB				\
        (MDP_GET_PACK_PATTERN(0,CLR_R,CLR_G,CLR_B, 2)<<8)
#define DMA_PACK_PATTERN_BGR \
        (MDP_GET_PACK_PATTERN(0, CLR_B, CLR_G, CLR_R, 2)<<8)

#define DMA_PACK_PATTERN_BGRA \
        (MDP_GET_PACK_PATTERN(CLR_ALPHA, CLR_B, CLR_G, CLR_R, 2)<<8)

#define DMA_DITHER_EN                         (1 << 24)
#define DMA_OUT_SEL_LCDC                      (1 << 20)
#define DMA_IBUF_FORMAT_RGB888			          (0 << 25)
#define DMA_IBUF_FORMAT_RGB565			          (1 << 25)
#define DMA_IBUF_FORMAT_XRGB8888		          (2 << 25)
#define DMA_IBUF_FORMAT_xRGB8888_OR_ARGB8888  (1 << 26)
#define DMA_IBUF_FORMAT_MASK			            (3 << 25)

#define DMA_DST_BITS_MASK 0x3F

/* MDP regs */
#define REG_MDP(offset)                       MSM_MDP_BASE1 + offset

#define MDP_DMA_P_CONFIG                      REG_MDP(0x90000)
#define MDP_DMA_P_OUT_XY                      REG_MDP(0x90010)
#define MDP_DMA_P_SIZE                        REG_MDP(0x90004)
#define MDP_DMA_P_BUF_ADDR                    REG_MDP(0x90008)
#define MDP_DMA_P_BUF_Y_STRIDE                REG_MDP(0x9000C)
#define MDP_DMA_P_OP_MODE                     REG_MDP(0x90070)

extern UINT64  mSystemMemoryEnd;

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

// Either implemented by PrePiLib or by MemoryInitPei
VOID
BuildMemoryTypeInformationHob (
  VOID
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
