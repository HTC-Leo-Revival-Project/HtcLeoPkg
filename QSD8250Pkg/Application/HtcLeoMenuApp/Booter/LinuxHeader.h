//from https://github.com/ARM-software/u-boot/blob/master/arch/arm/lib/zimage.c
#define	LINUX_ARM_ZIMAGE_MAGIC	0x016f2818

struct arm_z_header {
	UINT32	code[9];
	UINT32	zi_magic;
	UINT32	zi_start;
	UINT32	zi_end;
} __attribute__ ((__packed__));