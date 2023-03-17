#ifndef _DEVICE_MEMORY_MAP_H_
#define _DEVICE_MEMORY_MAP_H_

#include <Library/ArmLib.h>

#define MAX_ARM_MEMORY_REGION_DESCRIPTOR_COUNT 32

/* Below flag is used for system memory */
#define SYSTEM_MEMORY_RESOURCE_ATTR_CAPABILITIES \
                EFI_RESOURCE_ATTRIBUTE_PRESENT |                 \
                EFI_RESOURCE_ATTRIBUTE_INITIALIZED |             \
                EFI_RESOURCE_ATTRIBUTE_TESTED |                  \
                EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |             \
                EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |       \
                EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE | \
                EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |    \
                EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTABLE

typedef enum {
	NoHob,
	AddMem,
	AddDev,
	MaxMem
} DeviceMemoryAddHob;

typedef struct {
	EFI_PHYSICAL_ADDRESS         Address;
	UINT64                       Length;
	EFI_RESOURCE_TYPE            ResourceType;
	EFI_RESOURCE_ATTRIBUTE_TYPE	 ResourceAttribute;
	ARM_MEMORY_REGION_ATTRIBUTES ArmAttributes;
	DeviceMemoryAddHob					 HobOption;
	EFI_MEMORY_TYPE              MemoryType;
} ARM_MEMORY_REGION_DESCRIPTOR_EX, *PARM_MEMORY_REGION_DESCRIPTOR_EX;

static ARM_MEMORY_REGION_DESCRIPTOR_EX gDeviceMemoryDescriptorEx[] =
{
	/* Address, Length, ResourceType, Resource Attribute, ARM MMU Attribute, HobOption, EFI Memory Type */

  /* Registers regions */
  { 0x00000000, 0x11800000, EFI_RESOURCE_MEMORY_MAPPED_IO, EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE, 
    ARM_MEMORY_REGION_ATTRIBUTE_DEVICE, AddDev, EfiMemoryMappedIO },
  /* HLOS 1 */
  { 0x11800000, 0x00008000, EFI_RESOURCE_SYSTEM_MEMORY, SYSTEM_MEMORY_RESOURCE_ATTR_CAPABILITIES,
    ARM_MEMORY_REGION_ATTRIBUTE_WRITE_THROUGH, AddMem, EfiConventionalMemory },
  /* UEFI FD */
	{ 0x11808000, 0x00200000, EFI_RESOURCE_SYSTEM_MEMORY, SYSTEM_MEMORY_RESOURCE_ATTR_CAPABILITIES,
	  ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK, AddMem, EfiBootServicesCode },
  /* HLOS 2 */
  { 0x11A08000, 0x1E5F8000, EFI_RESOURCE_SYSTEM_MEMORY, SYSTEM_MEMORY_RESOURCE_ATTR_CAPABILITIES,
    ARM_MEMORY_REGION_ATTRIBUTE_WRITE_BACK, AddMem, EfiConventionalMemory },

  /* Peripheral regions */
  /* SDC1 */
  { 0xA0300000, 0x00100000, EFI_RESOURCE_MEMORY_MAPPED_IO, EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE, ARM_MEMORY_REGION_ATTRIBUTE_DEVICE, AddDev, EfiMemoryMappedIO },
  /* SDC2 */
  { 0xA0400000, 0x00100000, EFI_RESOURCE_MEMORY_MAPPED_IO, EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE, ARM_MEMORY_REGION_ATTRIBUTE_DEVICE, AddDev, EfiMemoryMappedIO },
  /* SDC3 */
  { 0xA0500000, 0x00100000, EFI_RESOURCE_MEMORY_MAPPED_IO, EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE, ARM_MEMORY_REGION_ATTRIBUTE_DEVICE, AddDev, EfiMemoryMappedIO },
  /* SDC4 */
  { 0xA0600000, 0x00100000, EFI_RESOURCE_MEMORY_MAPPED_IO, EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE, ARM_MEMORY_REGION_ATTRIBUTE_DEVICE, AddDev, EfiMemoryMappedIO },
  /* Others */
  { 0xA8000000, 0x04200000, EFI_RESOURCE_MEMORY_MAPPED_IO, EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE, ARM_MEMORY_REGION_ATTRIBUTE_DEVICE, AddDev, EfiMemoryMappedIO },

  /* Terminator */
  { 0 }
};

#endif