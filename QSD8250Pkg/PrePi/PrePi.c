/** @file

  Copyright (c) 2011-2017, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/PrePiLib.h>
#include <Library/PrintLib.h>
#include <Library/PrePiHobListPointerLib.h>
#include <Library/TimerLib.h>
#include <Library/PerformanceLib.h>

#include <Ppi/GuidedSectionExtraction.h>
#include <Ppi/ArmMpCoreInfo.h>
#include <Ppi/SecPerformance.h>

#include "PrePi.h"

UINT64  mSystemMemoryEnd = FixedPcdGet64 (PcdSystemMemoryBase) +
                           FixedPcdGet64 (PcdSystemMemorySize) - 1;

UINTN Width = FixedPcdGet32(PcdMipiFrameBufferWidth);
UINTN Height = FixedPcdGet32(PcdMipiFrameBufferHeight);
UINTN Bpp = FixedPcdGet32(PcdMipiFrameBufferPixelBpp);
UINTN FbAddr = FixedPcdGet32(PcdMipiFrameBufferAddress);

VOID
PaintScreen(
  IN  UINTN   BgColor
)
{
  // Code from FramebufferSerialPortLib
	char* Pixels = (void*)FixedPcdGet32(PcdMipiFrameBufferAddress);

	// Set color.
	for (UINTN i = 0; i < Width; i++)
	{
		for (UINTN j = 0; j < Height; j++)
		{
			// Set pixel bit
			for (UINTN p = 0; p < (Bpp / 8); p++)
			{
				*Pixels = (unsigned char)BgColor;
				BgColor = BgColor >> 8;
				Pixels++;
			}
		}
	}
}

VOID
ReconfigFb()
{
  UINT32 dma_cfg = 0;

  PaintScreen(0);

  // Stop any previous transfers
  MmioWrite32(MDP_LCDC_EN, 0);

  ArmInstructionSynchronizationBarrier();
  ArmDataMemoryBarrier();

  // Format
  // https://github.com/marc1706/hd2_kernel/blob/f4951cda4525e4cba87a3de83fd00aee61bb2897/drivers/video/msm/mdp_lcdc.c#L152
  dma_cfg |= (DMA_PACK_ALIGN_LSB |
          DMA_PACK_PATTERN_RGB |
          DMA_DITHER_EN);
  
  dma_cfg |= DMA_OUT_SEL_LCDC; // Select the DMA channel for LCDC
  
  // Format
  if(Bpp == 16) {
      dma_cfg |= DMA_IBUF_FORMAT_RGB565;
  }
  else if(Bpp == 24) {
      dma_cfg |= DMA_IBUF_FORMAT_RGB888;
  }
  else if(Bpp == 32) {
      dma_cfg |= DMA_IBUF_FORMAT_XRGB8888;
  }

  dma_cfg &= ~DMA_DST_BITS_MASK;
  dma_cfg |= DMA_DSTC0G_8BITS|DMA_DSTC1B_8BITS|DMA_DSTC2R_8BITS;

  MmioWrite32(MDP_DMA_P_CONFIG, dma_cfg);

  // Stride
  MmioWrite32(MDP_DMA_P_BUF_Y_STRIDE, (Bpp / 8) * Width);

  // Write fb addr (relocates fb to 0x02A00000 on schubert)
  MmioWrite32(MDP_DMA_P_BUF_ADDR, FixedPcdGet32(PcdMipiFrameBufferAddress));

  // Ensure all transfers finished
  ArmInstructionSynchronizationBarrier();
  ArmDataMemoryBarrier();

  // Enable LCDC
  MmioWrite32(MDP_LCDC_EN, 0x1);
}

VOID
EnableCounter()
{
  /* enable cp10 and cp11 */
	UINT32 val;
	__asm__ volatile("mrc	p15, 0, %0, c1, c0, 2" : "=r" (val));
	val |= (3<<22)|(3<<20);
	__asm__ volatile("mcr	p15, 0, %0, c1, c0, 2" :: "r" (val));

  ArmInstructionSynchronizationBarrier();
  ArmDataMemoryBarrier();

	/* set enable bit in fpexc */
	__asm__ volatile("mrc  p10, 7, %0, c8, c0, 0" : "=r" (val));
	val |= (1<<30);
	__asm__ volatile("mcr  p10, 7, %0, c8, c0, 0" :: "r" (val));

  /* enable the cycle count register */
	UINT32 en;
	__asm__ volatile("mrc	p15, 0, %0, c9, c12, 0" : "=r" (en));
	en &= ~(1<<3); /* cycle count every cycle */
	en |= 1; /* enable all performance counters */
	__asm__ volatile("mcr	p15, 0, %0, c9, c12, 0" :: "r" (en));

	/* enable cycle counter */
	en = (1<<31);
	__asm__ volatile("mcr	p15, 0, %0, c9, c12, 1" :: "r" (en));
}

VOID
PrePiMain (
  IN  UINTN   UefiMemoryBase,
  IN  UINTN   StacksBase,
  IN  UINT64  StartTimeStamp
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE  *HobList;
  EFI_STATUS                  Status;
  CHAR8                       Buffer[100];
  UINTN                       CharCount;
  UINTN                       StacksSize;
  FIRMWARE_SEC_PERFORMANCE    Performance;

  PaintScreen(0xFFFF); //RGB565_WHITE

  // Initialize the architecture specific bits
  ArchInitialize ();

  PaintScreen(0xF800); //RGB565_RED

  // Reconfigure the framebuffer based on PCD
  if(FixedPcdGetBool(PcdMipiFrameBufferReconfig)) {
    ReconfigFb();
  }
  else {
    // Just clear screen to black for edk2 logs to be visible
    PaintScreen(0x0000);
  }

  // Enable the counter (code from PrimeG2Pkg)
  EnableCounter();

  PaintScreen(0x07E0); //RGB565_GREEN


  // Initialize the Serial Port
  SerialPortInitialize ();
  CharCount = AsciiSPrint (
                Buffer,
                sizeof (Buffer),
                "UEFI firmware (version %s built at %a on %a)\n\r",
                (CHAR16 *)PcdGetPtr (PcdFirmwareVersionString),
                __TIME__,
                __DATE__
                );
  SerialPortWrite ((UINT8 *)Buffer, CharCount);

  DEBUG((
        EFI_D_INFO | EFI_D_LOAD,
        "UEFI Memory Base = 0x%p, Stack Base = 0x%p\n",
        UefiMemoryBase,
        StacksBase
    ));

  // Initialize the Debug Agent for Source Level Debugging
  InitializeDebugAgent (DEBUG_AGENT_INIT_POSTMEM_SEC, NULL, NULL);
  SaveAndSetDebugTimerInterrupt (TRUE);

  // Declare the PI/UEFI memory region
  HobList = HobConstructor (
              (VOID *)UefiMemoryBase,
              FixedPcdGet32 (PcdSystemMemoryUefiRegionSize),
              (VOID *)UefiMemoryBase,
              (VOID *)StacksBase // The top of the UEFI Memory is reserved for the stacks
              );
  PrePeiSetHobList (HobList);

  // Initialize MMU and Memory HOBs (Resource Descriptor HOBs)
  Status = MemoryPeim (UefiMemoryBase, FixedPcdGet32 (PcdSystemMemoryUefiRegionSize));
  ASSERT_EFI_ERROR (Status);

  // Create the Stacks HOB (reserve the memory for all stacks)
  if (ArmIsMpCore ()) {
    StacksSize = PcdGet32 (PcdCPUCorePrimaryStackSize) +
                 ((FixedPcdGet32 (PcdCoreCount) - 1) * FixedPcdGet32 (PcdCPUCoreSecondaryStackSize));
  } else {
    StacksSize = PcdGet32 (PcdCPUCorePrimaryStackSize);
  }

  BuildStackHob (StacksBase, StacksSize);

  // TODO: Call CpuPei as a library
  BuildCpuHob (ArmGetPhysicalAddressBits (), PcdGet8 (PcdPrePiCpuIoSize));

  // Store timer value logged at the beginning of firmware image execution
  Performance.ResetEnd = GetTimeInNanoSecond (StartTimeStamp);

  // Build SEC Performance Data Hob
  BuildGuidDataHob (&gEfiFirmwarePerformanceGuid, &Performance, sizeof (Performance));

  // Set the Boot Mode
  SetBootMode (ArmPlatformGetBootMode ());

  // Initialize Platform HOBs (CpuHob and FvHob)
  Status = PlatformPeim ();
  ASSERT_EFI_ERROR (Status);

  // Now, the HOB List has been initialized, we can register performance information
  PERF_START (NULL, "PEI", NULL, StartTimeStamp);

  // SEC phase needs to run library constructors by hand.
  ProcessLibraryConstructorList ();

  // Assume the FV that contains the SEC (our code) also contains a compressed FV.
  Status = DecompressFirstFv ();
  ASSERT_EFI_ERROR (Status);

  // Load the DXE Core and transfer control to it
  Status = LoadDxeCoreFromFv (NULL, 0);
  ASSERT_EFI_ERROR (Status);
}

VOID
CEntryPoint (
  IN  UINTN  MpId,
  IN  UINTN  UefiMemoryBase,
  IN  UINTN  StacksBase
  )
{
  UINT64  StartTimeStamp;

  // Initialize the platform specific controllers
  ArmPlatformInitialize (MpId);

  StartTimeStamp = 0;

  // Data Cache enabled on Primary core when MMU is enabled.
  ArmDisableDataCache ();
  // Invalidate instruction cache
  ArmInvalidateInstructionCache ();
  // Enable Instruction Caches on all cores.
  ArmEnableInstructionCache ();

  // Wait the Primary core has defined the address of the Global Variable region (event: ARM_CPU_EVENT_DEFAULT)
  ArmCallWFE ();

  // Goto primary Main.
  //PrimaryMain (UefiMemoryBase, StacksBase, StartTimeStamp);
  PrePiMain (UefiMemoryBase, StacksBase, StartTimeStamp);

  // DXE Core should always load and never return
  ASSERT (FALSE);
}
