/* tools/mkbootimg/bootimg.h
**
** Copyright 2007, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
#ifndef LIB_BOOT_INTERNAL_BOOTIMG_H
#define LIB_BOOT_INTERNAL_BOOTIMG_H
typedef struct boot_img_hdr boot_img_hdr;
#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512
#define BOOT_EXTRA_ARGS_SIZE 1024
struct boot_img_hdr {
    boot_uint8_t magic[BOOT_MAGIC_SIZE];
    boot_uint32_t kernel_size;  /* size in bytes */
    boot_uint32_t kernel_addr;  /* physical load addr */
    boot_uint32_t ramdisk_size; /* size in bytes */
    boot_uint32_t ramdisk_addr; /* physical load addr */
    boot_uint32_t second_size;  /* size in bytes */
    boot_uint32_t second_addr;  /* physical load addr */
    boot_uint32_t tags_addr;    /* physical addr for kernel tags */
    boot_uint32_t page_size;    /* flash page size we assume */
    boot_uint32_t dt_size;      /* reserved for future expansion: MUST be 0 */
    /* operating system version and security patch level; for
     * version "A.B.C" and patch level "Y-M-D":
     * ver = A << 14 | B << 7 | C         (7 bits for each of A, B, C)
     * lvl = ((Y - 2000) & 127) << 4 | M  (7 bits for Y, 4 bits for M)
     * os_version = ver << 11 | lvl */
    boot_uint32_t os_version;
    boot_uint8_t name[BOOT_NAME_SIZE]; /* asciiz product name */
    boot_uint8_t cmdline[BOOT_ARGS_SIZE];
    boot_uint32_t id[8]; /* timestamp / checksum / sha1 / etc */
    /* Supplemental command line data; kept here to maintain
     * binary compatibility with older versions of mkbootimg */
    boot_uint8_t extra_cmdline[BOOT_EXTRA_ARGS_SIZE];
} __attribute__((packed));
/*
** +-----------------+
** | boot header     | 1 page
** +-----------------+
** | kernel          | n pages
** +-----------------+
** | ramdisk         | m pages
** +-----------------+
** | second stage    | o pages
** +-----------------+
**
** n = (kernel_size + page_size - 1) / page_size
** m = (ramdisk_size + page_size - 1) / page_size
** o = (second_size + page_size - 1) / page_size
**
** 0. all entities are page_size aligned in flash
** 1. kernel and ramdisk are required (size != 0)
** 2. second is optional (second_size == 0 -> no second)
** 3. load each element (kernel, ramdisk, second) at
**    the specified physical address (kernel_addr, etc)
** 4. prepare tags at tag_addr.  kernel_args[] is
**    appended to the kernel commandline in the tags.
** 5. r0 = 0, r1 = MACHINE_TYPE, r2 = tags_addr
** 6. if second_size != 0: jump to second_addr
**    else: jump to kernel_addr
*/
#if 0
typedef struct ptentry ptentry;
struct ptentry {
    char name[16];      /* asciiz partition name    */
    unsigned start;     /* starting block number    */
    unsigned length;    /* length in blocks         */
    unsigned flags;     /* set to zero              */
};
/* MSM Partition Table ATAG
**
** length: 2 + 7 * n
** atag:   0x4d534d70
**         <ptentry> x n
*/
#endif
#endif // LIB_BOOT_INTERNAL_BOOTIMG_H
