/*
 *  linux/include/asm/setup.h
 *
 *  Copyright (C) 1997-1999 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Structure passed to kernel to tell it about the
 *  hardware it's running on.  See Documentation/arm/Setup
 *  for more info.
 */
#ifndef LIB_BOOT_INTERNAL_ATAGS_H
#define LIB_BOOT_INTERNAL_ATAGS_H

#define COMMAND_LINE_SIZE 1024

/* The list ends with an ATAG_NONE node. */
#define ATAG_NONE   0x00000000

struct tag_header {
    boot_uint32_t size;
    boot_uint32_t tag;
};
typedef struct tag_header tag_header_t;

/* The list must start with an ATAG_CORE node */
#define ATAG_CORE   0x54410001
#define FLAG_READONLY   1

struct tag_core {
    boot_uint32_t flags;        /* bit 0 = read-only */
    boot_uint32_t pagesize;
    boot_uint32_t rootdev;
};
typedef struct tag_core tag_core_t;

/* it is allowed to have multiple ATAG_MEM nodes */
#define ATAG_MEM    0x54410002

struct tag_mem32 {
    boot_uint32_t size;
    boot_uint32_t start;        /* physical start address */
};
typedef struct tag_mem32 tag_mem32_t;

/* VGA text type displays */
#define ATAG_VIDEOTEXT  0x54410003

struct tag_videotext {
    boot_uint8_t x;
    boot_uint8_t y;
    boot_uint16_t video_page;
    boot_uint8_t video_mode;
    boot_uint8_t video_cols;
    boot_uint16_t video_ega_bx;
    boot_uint8_t video_lines;
    boot_uint8_t video_isvga;
    boot_uint16_t video_points;
};
typedef struct tag_videotext tag_videotext_t;

/* describes how the ramdisk will be used in kernel */
#define ATAG_RAMDISK    0x54410004

struct tag_ramdisk {
    boot_uint32_t flags;        /* bit 0 = load, bit 1 = prompt */
    boot_uint32_t size;     /* decompressed ramdisk size in _kilo_ bytes */
    boot_uint32_t start;        /* starting block of floppy-based RAM disk image */
};
typedef struct tag_ramdisk tag_ramdisk_t;

/* describes where the compressed ramdisk image lives (virtual address) */
/*
 * this one accidentally used virtual addresses - as such,
 * it's deprecated.
 */
#define ATAG_INITRD 0x54410005

/* describes where the compressed ramdisk image lives (physical address) */
#define ATAG_INITRD2    0x54420005

struct tag_initrd {
    boot_uint32_t start;        /* physical start address */
    boot_uint32_t size;     /* size of compressed ramdisk image in bytes */
};
typedef struct tag_initrd tag_initrd_t;

/* board serial number. "64 bits should be enough for everybody" */
#define ATAG_SERIAL 0x54410006

struct tag_serialnr {
    boot_uint32_t low;
    boot_uint32_t high;
};
typedef struct tag_serialnr tag_serialnr_t;

/* board revision */
#define ATAG_REVISION   0x54410007

struct tag_revision {
    boot_uint32_t rev;
};
typedef struct tag_revision tag_revision_t;

/* initial values for vesafb-type framebuffers. see struct screen_info
 * in include/linux/tty.h
 */
#define ATAG_VIDEOLFB   0x54410008

struct tag_videolfb {
    boot_uint16_t lfb_width;
    boot_uint16_t lfb_height;
    boot_uint16_t lfb_depth;
    boot_uint16_t lfb_linelength;
    boot_uint32_t lfb_base;
    boot_uint32_t lfb_size;
    boot_uint8_t red_size;
    boot_uint8_t red_pos;
    boot_uint8_t green_size;
    boot_uint8_t green_pos;
    boot_uint8_t blue_size;
    boot_uint8_t blue_pos;
    boot_uint8_t rsvd_size;
    boot_uint8_t rsvd_pos;
};
typedef struct tag_videolfb tag_videolfb_t;

/* command line: \0 terminated string */
#define ATAG_CMDLINE    0x54410009

struct tag_cmdline {
    char cmdline[1];    /* this is the minimum size */
};
typedef struct tag_cmdline tag_cmdline_t;

/* acorn RiscPC specific information */
#define ATAG_ACORN  0x41000101

struct tag_acorn {
    boot_uint32_t memc_control_reg;
    boot_uint32_t vram_pages;
    boot_uint8_t sounddefault;
    boot_uint8_t adfsdrives;
};
typedef struct tag_acorn tag_acorn_t;

/* footbridge memory clock, see arch/arm/mach-footbridge/arch.c */
#define ATAG_MEMCLK 0x41000402

struct tag_memclk {
    boot_uint32_t fmemclk;
};
typedef struct tag_memclk tag_memclk_t;

struct tag {
    struct tag_header hdr;
    union {
        struct tag_core core;
        struct tag_mem32 mem;
        struct tag_videotext videotext;
        struct tag_ramdisk ramdisk;
        struct tag_initrd initrd;
        struct tag_serialnr serialnr;
        struct tag_revision revision;
        struct tag_videolfb videolfb;
        struct tag_cmdline cmdline;

        /*
         * Acorn specific
         */
        struct tag_acorn acorn;

        /*
         * DC21285 specific
         */
        struct tag_memclk memclk;
    } u;
};
typedef struct tag tag_t;

struct tagtable {
    boot_uint32_t tag;
    int (*parse) (const struct tag *);
};
typedef struct tagtable tagtable_t;

#define tag_member_present(tag,member)              \
    ((unsigned long)(&((struct tag *)0L)->member + 1)   \
        <= (tag)->hdr.size * 4)

#define tag_next(t) ((struct tag *)((boot_uint32_t *)(t) + (t)->hdr.size))
#define tag_size(type)  ((sizeof(struct tag_header) + sizeof(struct type)) >> 2)

#define for_each_tag(t,base)        \
    for (t = base; t->hdr.size; t = tag_next(t))

#endif // LIB_BOOT_INTERNAL_ATAGS_H
