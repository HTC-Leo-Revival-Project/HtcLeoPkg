/** @file
*
*  Copyright (c) 2018, Linaro Limited. All rights reserved.
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
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>

// The total number of descriptors, including the final "end-of-table" descriptor.
#define MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS 12

// DDR attributes
#define DDR_ATTRIBUTES_CACHED           ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK
#define DDR_ATTRIBUTES_UNCACHED         ARM_MEMORY_REGION_ATTRIBUTE_UNCACHED_UNBUFFERED
#define WRITE_BACK_XN                   ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK

#define QSD8250_PERIPH_BASE              0xA0000000
#define QSD8250_PERIPH_SIZE              0x0C300000

#define FB_ADDR FixedPcdGet32(PcdMipiFrameBufferAddress)
#define FB_SIZE FixedPcdGet32(PcdMipiFrameBufferWidth)*FixedPcdGet32(PcdMipiFrameBufferHeight)*(FixedPcdGet32(PcdMipiFrameBufferPixelBpp) / 8)

STATIC struct ReservedMemory {
    EFI_PHYSICAL_ADDRESS         Offset;
    EFI_PHYSICAL_ADDRESS         Size;
} ReservedMemoryBuffer [] = {
    { 0x00000000, 0x00100000 },    // APPSBL
    { 0x00100000, 0x00100000 },    // SMEM
    { 0x00200000, 0x00200000 },    // OEMSBL
    { 0x00400000, 0x02100000 },    // AMSS
    { 0x10000000, 0x01800000 },    // QDSP6
    { 0x2FFC0000, 0x00040000 },    // PSTORE
#if ENABLEUART == 1
    { 0xA9A00000, 0x00001000 },    //UART
#endif
    
    { FB_ADDR,    FB_SIZE    },    // Display Reserved
};

/**
  Return the Virtual Memory Map of your platform
  This Virtual Memory Map is used by MemoryInitPei Module to initialize the MMU on your platform.
  @param[out]   VirtualMemoryMap    Array of ARM_MEMORY_REGION_DESCRIPTOR describing a Physical-to-
                                    Virtual Memory mapping. This array must be ended by a zero-filled
                                    entry
**/
VOID
ArmPlatformGetVirtualMemoryMap (
    IN ARM_MEMORY_REGION_DESCRIPTOR **VirtualMemoryMap
) {
    ARM_MEMORY_REGION_ATTRIBUTES  CacheAttributes;
    ARM_MEMORY_REGION_DESCRIPTOR  *VirtualMemoryTable;
    EFI_RESOURCE_ATTRIBUTE_TYPE   ResourceAttributes;
    UINTN                         Index = 0, Count, ReservedTop;
    UINTN                         IndexDram = -1;
    EFI_PEI_HOB_POINTERS          NextHob;
    UINT64                        ResourceLength;
    EFI_PHYSICAL_ADDRESS          ResourceTop;
    ResourceAttributes = (
                             EFI_RESOURCE_ATTRIBUTE_PRESENT |
                             EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
                             EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
                             EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
                             EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
                             EFI_RESOURCE_ATTRIBUTE_TESTED
                         );
    // Create initial Base Hob for system memory.
    BuildResourceDescriptorHob (
        EFI_RESOURCE_SYSTEM_MEMORY,
        ResourceAttributes,
        PcdGet64 (PcdSystemMemoryBase),
        PcdGet64 (PcdSystemMemorySize)
    );
    NextHob.Raw = GetHobList ();
    Count = sizeof (ReservedMemoryBuffer) / sizeof (struct ReservedMemory);
    while ((NextHob.Raw = GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, NextHob.Raw)) != NULL) {
        if (Index >= Count)
            break;
        if ((NextHob.ResourceDescriptor->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) &&
                (ReservedMemoryBuffer[Index].Offset >= NextHob.ResourceDescriptor->PhysicalStart) &&
                ((ReservedMemoryBuffer[Index].Offset + ReservedMemoryBuffer[Index].Size) <=
                 NextHob.ResourceDescriptor->PhysicalStart + NextHob.ResourceDescriptor->ResourceLength)) {
            ResourceAttributes = NextHob.ResourceDescriptor->ResourceAttribute;
            ResourceLength = NextHob.ResourceDescriptor->ResourceLength;
            ResourceTop = NextHob.ResourceDescriptor->PhysicalStart + ResourceLength;
            ReservedTop = ReservedMemoryBuffer[Index].Offset + ReservedMemoryBuffer[Index].Size;
            // Create the System Memory HOB for the reserved buffer
            BuildResourceDescriptorHob (
                EFI_RESOURCE_MEMORY_RESERVED,
                EFI_RESOURCE_ATTRIBUTE_PRESENT,
                ReservedMemoryBuffer[Index].Offset,
                ReservedMemoryBuffer[Index].Size
            );
            // Update the HOB
            NextHob.ResourceDescriptor->ResourceLength = ReservedMemoryBuffer[Index].Offset -
                    NextHob.ResourceDescriptor->PhysicalStart;
            // If there is some memory available on the top of the reserved memory then create a HOB
            if (ReservedTop < ResourceTop) {
                BuildResourceDescriptorHob (EFI_RESOURCE_SYSTEM_MEMORY,
                                            ResourceAttributes,
                                            ReservedTop,
                                            ResourceTop - ReservedTop);
            }
            Index++;
        }
        NextHob.Raw = GET_NEXT_HOB (NextHob);
    }
    ASSERT (VirtualMemoryMap != NULL);
    VirtualMemoryTable = (ARM_MEMORY_REGION_DESCRIPTOR *)AllocatePages (
                             EFI_SIZE_TO_PAGES (sizeof(ARM_MEMORY_REGION_DESCRIPTOR) * MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS)
                         );
    if (VirtualMemoryTable == NULL)
        return;
    CacheAttributes = DDR_ATTRIBUTES_CACHED;
    Index = 0;

    // System DRAM
    VirtualMemoryTable[Index].PhysicalBase = PcdGet64 (PcdSystemMemoryBase);
    VirtualMemoryTable[Index].VirtualBase  = VirtualMemoryTable[Index].PhysicalBase;
    VirtualMemoryTable[Index].Length       = PcdGet64 (PcdSystemMemorySize);
    VirtualMemoryTable[Index].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;
    IndexDram = Index++;

    // Peripheral space before DRAM
    if (VirtualMemoryTable[IndexDram].PhysicalBase != 0) {
        VirtualMemoryTable[Index].PhysicalBase = 0x0;
        VirtualMemoryTable[Index].VirtualBase  = 0x0;
        VirtualMemoryTable[Index].Length       = VirtualMemoryTable[IndexDram].PhysicalBase;
        VirtualMemoryTable[Index++].Attributes   = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;
    }

    // Remap the framebuffer region as uncached memory
    VirtualMemoryTable[Index].PhysicalBase  = FB_ADDR;
    VirtualMemoryTable[Index].VirtualBase     = VirtualMemoryTable[Index].PhysicalBase;
    VirtualMemoryTable[Index].Length          = FB_SIZE;
    VirtualMemoryTable[Index++].Attributes      = DDR_ATTRIBUTES_UNCACHED;

    // Remap the FD region as normal executable memory
    VirtualMemoryTable[Index].PhysicalBase  = PcdGet64 (PcdFdBaseAddress);
    VirtualMemoryTable[Index].VirtualBase     = VirtualMemoryTable[Index].PhysicalBase;
    VirtualMemoryTable[Index].Length          = FixedPcdGet32 (PcdFdSize);
    VirtualMemoryTable[Index++].Attributes      = ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK;

    // SOC peripherals after DDR
    VirtualMemoryTable[Index].PhysicalBase  = QSD8250_PERIPH_BASE;
    VirtualMemoryTable[Index].VirtualBase     = VirtualMemoryTable[Index].PhysicalBase;
    VirtualMemoryTable[Index].Length          = QSD8250_PERIPH_SIZE;
    VirtualMemoryTable[Index++].Attributes      = ARM_MEMORY_REGION_ATTRIBUTE_DEVICE;

    // End of Table
    VirtualMemoryTable[Index].PhysicalBase  = 0;
    VirtualMemoryTable[Index].VirtualBase     = 0;
    VirtualMemoryTable[Index].Length          = 0;
    VirtualMemoryTable[Index++].Attributes      = (ARM_MEMORY_REGION_ATTRIBUTES)0;

    ASSERT((Index + 1) <= MAX_VIRTUAL_MEMORY_MAP_DESCRIPTORS);
    *VirtualMemoryMap = VirtualMemoryTable;
}