/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Library/ArmPlatformLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS  16

// DDR attributes
#define DDR_ATTRIBUTES_CACHED               ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
#define DDR_ATTRIBUTES_UNCACHED             ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED

#define SOC_REGISTERS_ATTRIBUTES            ARM_MEMORY_REGION_ATTRIBUTE_DEVICE

/**
  Return the Virtual Memory Map of your platform

  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU on your platform.

  @param[out]   VirtualMemoryMap    Array of ARM_MEMORY_REGION_DESCRIPTOR describing a Physical-to-
                                    Virtual Memory mapping. This array must be ended by a zero-filled
                                    entry

**/
VOID
ArmPlatformGetVirtualMemoryMap (
  IN ARM_MEMORY_REGION_DESCRIPTOR** VirtualMemoryMap
  )
{
  ARM_MEMORY_REGION_ATTRIBUTES  CacheAttributes;
  UINTN                         Index = 0;
  ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;

  ASSERT (VirtualMemoryMap != NULL);
  DEBUG ((DEBUG_VERBOSE, "Enter: ArmPlatformGetVirtualMemoryMap\n"));
  VirtualMemoryTable = (ARM_MEMORY_REGION_DESCRIPTOR *) AllocatePages (
      EFI_SIZE_TO_PAGES (sizeof (ARM_MEMORY_REGION_DESCRIPTOR) *
      MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS));
  if (VirtualMemoryTable == NULL) {
    return;
  }

  CacheAttributes = DDR_ATTRIBUTES_CACHED;

  // SOC registers region
  VirtualMemoryTable[Index].PhysicalBase   = 0x00000000;
  VirtualMemoryTable[Index].VirtualBase    = 0x00000000;
  VirtualMemoryTable[Index].Length         = 0x11800000;
  VirtualMemoryTable[Index].Attributes     = SOC_REGISTERS_ATTRIBUTES;

  // Framebuffer
  /*VirtualMemoryTable[++Index].PhysicalBase = 0x02A00000;
  VirtualMemoryTable[Index].VirtualBase    = 0x02A00000;
  VirtualMemoryTable[Index].Length         = 0x000BBB00;
  VirtualMemoryTable[Index].Attributes     = DDR_ATTRIBUTES_UNCACHED;*/

  // SOC registers region 2
  /*VirtualMemoryTable[Index].PhysicalBase   = 0x02ABBB00;
  VirtualMemoryTable[Index].VirtualBase    = 0x02ABBB00;
  VirtualMemoryTable[Index].Length         = 0x0ED44500;
  VirtualMemoryTable[Index].Attributes     = SOC_REGISTERS_ATTRIBUTES;*/

  // Rearranged system memory regions
  VirtualMemoryTable[++Index].PhysicalBase = 0x11800000;
  VirtualMemoryTable[Index].VirtualBase    = 0x11800000;
  VirtualMemoryTable[Index].Length         = 0x00008000;
  VirtualMemoryTable[Index].Attributes     = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_THROUGH;

  // FD
  VirtualMemoryTable[++Index].PhysicalBase = 0x11808000;
  VirtualMemoryTable[Index].VirtualBase    = 0x11808000;
  VirtualMemoryTable[Index].Length         = 0x00200000;
  VirtualMemoryTable[Index].Attributes     = CacheAttributes;

  // Free memory
  VirtualMemoryTable[++Index].PhysicalBase = 0x28000000;
  VirtualMemoryTable[Index].VirtualBase    = 0x28000000;
  VirtualMemoryTable[Index].Length         = 0x08000000;
  VirtualMemoryTable[Index].Attributes     = CacheAttributes;

  // End of Table
  VirtualMemoryTable[++Index].PhysicalBase = 0;
  VirtualMemoryTable[Index].VirtualBase    = 0;
  VirtualMemoryTable[Index].Length         = 0;
  VirtualMemoryTable[Index].Attributes     = (ARM_MEMORY_REGION_ATTRIBUTES)0;

  ASSERT ((Index + 1) <= MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS);

  *VirtualMemoryMap = VirtualMemoryTable;
}

// DDR - 1.0GB section
    /*VirtualMemoryTable[Index].PhysicalBase    = PcdGet64 (PcdSystemMemoryBase);
    VirtualMemoryTable[Index].VirtualBase     = PcdGet64 (PcdSystemMemoryBase);
    VirtualMemoryTable[Index].Length          = PcdGet64 (PcdSystemMemorySize);
    VirtualMemoryTable[Index].Attributes      = CacheAttributes;
    // QSD8250 SOC peripherals
    VirtualMemoryTable[++Index].PhysicalBase  = QSD8250_PERIPH_BASE;
    VirtualMemoryTable[Index].VirtualBase     = QSD8250_PERIPH_BASE;
    VirtualMemoryTable[Index].Length          = QSD8250_PERIPH_SZ;
    VirtualMemoryTable[Index].Attributes      = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;*/