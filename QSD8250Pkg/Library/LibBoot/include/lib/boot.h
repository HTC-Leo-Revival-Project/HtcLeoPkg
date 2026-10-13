/*
 * Copyright 2016, The EFIDroid Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#ifndef LIB_BOOT_H
#define LIB_BOOT_H

#include <lib/boot/boot_platform.h>
#include <lib/boot/list.h>

#define LIBBOOT_LOAD_TYPE_KERNEL 1
#define LIBBOOT_LOAD_TYPE_RAMDISK 2
#define LIBBOOT_LOAD_TYPE_TAGS 4
#define LIBBOOT_LOAD_TYPE_CMDLINE 8
#define LIBBOOT_LOAD_TYPE_ALL 0xffffffff

// IO
struct boot_io;

typedef boot_intn_t (*boot_io_fn_read_t)(struct boot_io *io, void *buf, boot_uintn_t blkoff, boot_uintn_t count);

struct boot_io {
    boot_io_fn_read_t read;
    boot_uintn_t blksz;
    boot_uintn_t numblocks;

    void *pdata;
    boot_uint8_t pdata_is_allocated;

    // memio
    boot_uint8_t is_memio;
};
typedef struct boot_io boot_io_t;

// TYPE
typedef enum {
    BOOTIMG_TYPE_UNKNOWN = -2,
    BOOTIMG_TYPE_RAW = -1,
    BOOTIMG_TYPE_ANDROID,
    BOOTIMG_TYPE_EFI,
    BOOTIMG_TYPE_ELF,
    BOOTIMG_TYPE_QCMBN,
    BOOTIMG_TYPE_ZIMAGE,
    BOOTIMG_TYPE_UIMAGE,
    BOOTIMG_TYPE_GZIP,
} bootimg_type_t;

typedef enum {
    LIBBOOT_TAGS_TYPE_UNKNOWN = -1,
    LIBBOOT_TAGS_TYPE_ATAGS,
    LIBBOOT_TAGS_TYPE_FDT,
    LIBBOOT_TAGS_TYPE_QCDT,
} libboot_tags_type_t;

static inline const char *bootimgtype2str(bootimg_type_t type)
{
    switch (type) {
        case BOOTIMG_TYPE_ANDROID:
            return "Android";
        case BOOTIMG_TYPE_EFI:
            return "EFI";
        case BOOTIMG_TYPE_ELF:
            return "ELF";
        case BOOTIMG_TYPE_QCMBN:
            return "QCMBN";
        case BOOTIMG_TYPE_ZIMAGE:
            return "zImage";
        case BOOTIMG_TYPE_UIMAGE:
            return "uImage";
        case BOOTIMG_TYPE_GZIP:
            return "gzip";
        case BOOTIMG_TYPE_RAW:
            return "RAW";

        default:
        case BOOTIMG_TYPE_UNKNOWN:
            return "unknown";
    }
}

static inline const char *tagtype2str(libboot_tags_type_t type)
{
    switch (type) {
        case LIBBOOT_TAGS_TYPE_ATAGS:
            return "atags";
        case LIBBOOT_TAGS_TYPE_FDT:
            return "fdt";
        case LIBBOOT_TAGS_TYPE_QCDT:
            return "qcdt";

        default:
        case LIBBOOT_TAGS_TYPE_UNKNOWN:
            return "unknown";
    }
}

// ERRORS
typedef enum {
    LIBBOOT_ERROR_GROUP_UNKNOWN = -1,
    LIBBOOT_ERROR_GROUP_COMMON,
    LIBBOOT_ERROR_GROUP_ANDROID,
    LIBBOOT_ERROR_GROUP_ELF,
    LIBBOOT_ERROR_GROUP_QCDT,
} libboot_error_group_t;

typedef enum {
    LIBBOOT_ERROR_TYPE_UNKNOWN = -1,
} libboot_error_type_t;

typedef enum {
    LIBBOOT_ERROR_COMMON_UNKNOWN = -1,
    LIBBOOT_ERROR_COMMON_OUT_OF_MEMORY,
    LIBBOOT_ERROR_COMMON_REFALLOC_NOT_FOUND,
    LIBBOOT_ERROR_COMMON_REFALLOC_INVALID,
    LIBBOOT_ERROR_COMMON_IO_READ,
    LIBBOOT_ERROR_COMMON_MEMIO_READ_ERROR,
    LIBBOOT_ERROR_COMMON_IDENTIFY_NO_MATCH,
    LIBBOOT_ERROR_COMMON_LOAD_NOT_IDENTIFIED,
    LIBBOOT_ERROR_COMMON_LOAD_NO_IO,
    LIBBOOT_ERROR_COMMON_LOAD_MODULE_ERROR,
    LIBBOOT_ERROR_COMMON_LOAD_NO_MATCH,
    LIBBOOT_ERROR_COMMON_LOAD_BUG,
    LIBBOOT_ERROR_COMMON_GENTAGS_MODULE_ERROR,
    LIBBOOT_ERROR_COMMON_GENTAGS_NO_MATCH,
    LIBBOOT_ERROR_COMMON_PREPARE_INVALID_TYPE,
    LIBBOOT_ERROR_COMMON_PREPARE_NO_KERNEL_MEMORY,
    LIBBOOT_ERROR_COMMON_PREPARE_NO_RAMDISK_MEMORY,
    LIBBOOT_ERROR_COMMON_PREPARE_NO_TAGS_MEMORY,
} libboot_error_common_t;

typedef enum {
    LIBBOOT_ERROR_ANDROID_UNKNOWN = -1,
    LIBBOOT_ERROR_ANDROID_SECOND_UNSUPPORTED,
    LIBBOOT_ERROR_ANDROID_ZERO_KERNEL,
    LIBBOOT_ERROR_ANDROID_READ_KERNEL,
    LIBBOOT_ERROR_ANDROID_READ_RAMDISK,
    LIBBOOT_ERROR_ANDROID_READ_TAGS,
    LIBBOOT_ERROR_ANDROID_ALLOC_CMDLINE,
} libboot_error_android_t;

typedef enum {
    LIBBOOT_ERROR_ELF_UNKNOWN = -1,
    LIBBOOT_ERROR_ELF_NO_CMDLINE,
    LIBBOOT_ERROR_ELF_UNKNOWN_IMAGE,
} libboot_error_elf_t;

typedef enum {
    LIBBOOT_ERROR_QCDT_UNKNOWN = -1,
    LIBBOOT_ERROR_QCDT_UNKNOWN_PARSER,
    LIBBOOT_ERROR_QCDT_PATH_NOT_FOUND,
    LIBBOOT_ERROR_QCDT_NOT_A_MULTIPLE,
    LIBBOOT_ERROR_QCDT_ID_ENTRY_NOT_FOUND,
    LIBBOOT_ERROR_QCDT_NO_MATCH,
    LIBBOOT_ERROR_QCDT_NO_MATCH2,
    LIBBOOT_ERROR_QCDT_INVALID_MAGIC,
    LIBBOOT_ERROR_QCDT_UNSUPPORTED_VERSION,
    LIBBOOT_ERROR_QCDT_INVALID_HEADER_SIZE,
} libboot_error_qcdt_t;

char **libboot_error_stack_get(void);
boot_uintn_t libboot_error_stack_count(void);
void libboot_error_stack_reset(void);

// MAIN

struct bootimg_context;

typedef void *(*libboot_context_fn_addatags_t)(void *tags);
typedef void  (*libboot_context_fn_patchfdt_t)(void *fdt);

struct bootimg_context {
    // identify
    bootimg_type_t type;
    bootimg_type_t outer_type;
    boot_io_t *rootio;
    boot_io_t *io;
    boot_uint32_t checksum;
    int magic_test_result;

    // load: kernel
    boot_uintn_t kernel_size;
    void *kernel_data;
    boot_uintn_t kernel_arguments[3];
    int kernel_is_linux;
    // load: ramdisk
    boot_uintn_t ramdisk_size;
    void *ramdisk_data;
    // load: tags
    libboot_tags_type_t tags_type;
    boot_uintn_t tags_size;
    void *tags_data;
    int tags_ready;
    // load: cmdline
    libboot_list_node_t cmdline;

    // prepare: final loading addresses
    boot_uintn_t kernel_addr;
    boot_uintn_t ramdisk_addr;
    boot_uintn_t tags_addr;

    // external configuration: optional
    void *default_fdt;
    void *default_qcdt;
    libboot_context_fn_addatags_t add_custom_atags;
    libboot_context_fn_patchfdt_t patch_fdt;
    const char *fdt_parser;
};
typedef struct bootimg_context bootimg_context_t;

// memory allocations
void *libboot_alloc(boot_uintn_t size);
void *libboot_refalloc(void *ptr, boot_uintn_t size);
boot_uintn_t libboot_get_refcount(void *ptr);
void  libboot_free(void *ptr);

// basic loading
int libboot_init(void);
void libboot_uninit(void);
void libboot_init_context(bootimg_context_t *context);
void libboot_free_context(bootimg_context_t *context);
int libboot_identify(boot_io_t *io, bootimg_context_t *context);
int libboot_identify_memory(void *mem, boot_uintn_t sz, bootimg_context_t *context);
int libboot_load(bootimg_context_t *context);
int libboot_load_partial(bootimg_context_t *context, boot_uintn_t type, boot_uint8_t recursive);
int libboot_unload(bootimg_context_t *context);
int libboot_prepare(bootimg_context_t *context);

// cmdline
void libboot_cmdline_init(libboot_list_node_t *list);
void libboot_cmdline_free(libboot_list_node_t *list);
void libboot_cmdline_addall(libboot_list_node_t *list, const char *cmdline, int overwrite);
void libboot_cmdline_add(libboot_list_node_t *list, const char *name, const char *value, int overwrite);
int libboot_cmdline_has(libboot_list_node_t *list, const char *name);
const char *libboot_cmdline_get(libboot_list_node_t *list, const char *name);
boot_uintn_t libboot_cmdline_generate(libboot_list_node_t *list, char *buf, boot_uintn_t bufsize);
void libboot_cmdline_remove(libboot_list_node_t *list, const char *name);
boot_uintn_t libboot_cmdline_length(libboot_list_node_t *list);

#endif // LIB_BOOT_H
