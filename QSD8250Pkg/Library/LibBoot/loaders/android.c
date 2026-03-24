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

#include <lib/boot.h>
#include <lib/boot/internal/boot_internal.h>

#include <lib/boot/internal/bootimg.h>

static int ldrmodule_load(bootimg_context_t *context, boot_uintn_t type, boot_uint8_t recursive)
{
    int rc;
    int ret = -1;
    void *kernel_data = NULL;
    void *ramdisk_data = NULL;
    void *tags_data = NULL;
    boot_uintn_t kernel_size = 0;
    boot_uintn_t ramdisk_size = 0;
    boot_uintn_t tags_size = 0;

    boot_img_hdr *hdr = libboot_internal_io_alloc(context->io, sizeof(boot_img_hdr));
    if (!hdr) goto out;

    rc = libboot_internal_io_read(context->io, hdr, 0, sizeof(*hdr), (void **)&hdr);
    if (rc<0) goto out;

    if (hdr->second_size!=0) {
        libboot_format_error(LIBBOOT_ERROR_GROUP_ANDROID, LIBBOOT_ERROR_ANDROID_SECOND_UNSUPPORTED, hdr->second_size);
        goto out;
    }

    // calculate offsets
    boot_uintn_t off_kernel  = hdr->page_size;
    boot_uintn_t off_ramdisk = off_kernel  + ALIGN(hdr->kernel_size,  hdr->page_size);
    boot_uintn_t off_second  = off_ramdisk + ALIGN(hdr->ramdisk_size, hdr->page_size);
    boot_uintn_t off_tags    = off_second  + ALIGN(hdr->second_size,  hdr->page_size);

    // load kernel
    if (type&LIBBOOT_LOAD_TYPE_KERNEL) {
        kernel_size = hdr->kernel_size;
        if (kernel_size>0) {
            // refalloc
            if (context->io->is_memio && context->io->pdata_is_allocated) {
                kernel_data = libboot_refalloc(context->io->pdata + off_kernel, kernel_size);
                if (!kernel_data) goto err_free;
            }

            // read from IO
            else {
                kernel_data = libboot_internal_io_alloc(context->io, kernel_size);
                if (!kernel_data) goto err_free;

                rc = libboot_internal_io_read(context->io, kernel_data, off_kernel, kernel_size, &kernel_data);
                if (rc<0) {
                    libboot_format_error(LIBBOOT_ERROR_GROUP_ANDROID, LIBBOOT_ERROR_ANDROID_READ_KERNEL, rc);
                    goto err_free;
                }
            }
        }
    }

    // load ramdisk
    if (type&LIBBOOT_LOAD_TYPE_RAMDISK) {
        ramdisk_size = hdr->ramdisk_size;
        if (ramdisk_size>0) {
            // refalloc
            if (context->io->is_memio && context->io->pdata_is_allocated) {
                ramdisk_data = libboot_refalloc(context->io->pdata + off_ramdisk, ramdisk_size);
                if (!ramdisk_data) goto err_free;
            }

            // read from IO
            else {
                ramdisk_data = libboot_internal_io_alloc(context->io, ramdisk_size);
                if (!ramdisk_data) goto err_free;

                rc = libboot_internal_io_read(context->io, ramdisk_data, off_ramdisk, ramdisk_size, &ramdisk_data);
                if (rc<0) {
                    libboot_format_error(LIBBOOT_ERROR_GROUP_ANDROID, LIBBOOT_ERROR_ANDROID_READ_RAMDISK, rc);
                    goto err_free;
                }
            }
        }
    }

    // load tags
    if (type&LIBBOOT_LOAD_TYPE_TAGS) {
        tags_size = hdr->dt_size;
        if (tags_size>0) {
            // refalloc
            if (context->io->is_memio && context->io->pdata_is_allocated) {
                tags_data = libboot_refalloc(context->io->pdata + off_tags, tags_size);
                if (!tags_data) goto err_free;
            }

            // read from IO
            else {
                tags_data = libboot_internal_io_alloc(context->io, tags_size);
                if (!tags_data) goto err_free;

                rc = libboot_internal_io_read(context->io, tags_data, off_tags, tags_size, &tags_data);
                if (rc<0) {
                    libboot_format_error(LIBBOOT_ERROR_GROUP_ANDROID, LIBBOOT_ERROR_ANDROID_READ_TAGS, rc);
                    goto err_free;
                }
            }
        }
    }

    // load cmdline
    hdr->cmdline[BOOT_ARGS_SIZE-1] = 0;
    hdr->extra_cmdline[BOOT_EXTRA_ARGS_SIZE-1] = 0;
    if (type&LIBBOOT_LOAD_TYPE_CMDLINE) {
        libboot_cmdline_addall(&context->cmdline, (char *)hdr->cmdline, 1);
        libboot_cmdline_addall(&context->cmdline, (char *)hdr->extra_cmdline, 1);
    }

    // set data
    if (type&LIBBOOT_LOAD_TYPE_KERNEL) {
        if (kernel_size > 0) {
            // re-identify with kernel as image
            rc = libboot_identify_memory(kernel_data, kernel_size, context);
            if (rc) {
                goto err_free;
            }

            // the data is used by context->io now, so refalloc it
            if (!libboot_refalloc(kernel_data, kernel_size)) {
                goto err_free;
            }
            context->io->pdata_is_allocated = 1;

            // we assume that this always holds a linux image, if not it doesn't hurt to generate (unused) tags
            context->kernel_is_linux = 1;
        }

        libboot_free(context->kernel_data);
        context->kernel_data = kernel_data;
        context->kernel_size = kernel_size;
        context->kernel_addr = hdr->kernel_addr;
    }
    if (type&LIBBOOT_LOAD_TYPE_RAMDISK) {
        libboot_free(context->ramdisk_data);
        context->ramdisk_data = ramdisk_data;
        context->ramdisk_size = ramdisk_size;
        context->ramdisk_addr = hdr->ramdisk_addr;
    }
    if (type&LIBBOOT_LOAD_TYPE_TAGS) {
        libboot_free(context->tags_data);
        context->tags_data = tags_data;
        context->tags_size = tags_size;
        context->tags_addr = hdr->tags_addr;
    }

    // success
    ret = 0;
    goto out;

err_free:
    libboot_free(kernel_data);
    libboot_free(ramdisk_data);
    libboot_free(tags_data);

out:
    libboot_free(hdr);

    return ret;
}

static int ldrmodule_magictest(boot_io_t *io, boot_uint32_t *checksum)
{
    int rc;
    int ret = -1;

    // allocate header
    boot_img_hdr *hdr = libboot_internal_io_alloc(io, sizeof(boot_img_hdr));
    if (!hdr) goto out;

    // read header
    rc = libboot_internal_io_read(io, hdr, 0, sizeof(*hdr), (void **)&hdr);
    if (rc<0) goto out;

    // check magic
    if (libboot_platform_memcmp(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE))
        goto out;

    // calculate checksum
    if (checksum) {
        *checksum = libboot_crc32(0, (void *)hdr, sizeof(boot_img_hdr));
    }

    ret = 0;

out:
    libboot_free(hdr);

    return ret;
}

static ldrmodule_t ldrmodule = {
    .type = BOOTIMG_TYPE_ANDROID,
    .magic_custom_test = ldrmodule_magictest,

    .load = ldrmodule_load,
};

int libboot_internal_ldrmodule_android_init(void)
{
    libboot_internal_ldrmodule_register(&ldrmodule);
    return 0;
}
