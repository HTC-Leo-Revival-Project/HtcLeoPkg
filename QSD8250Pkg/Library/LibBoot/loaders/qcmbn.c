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

typedef struct {
    boot_uint32_t image_id;
    boot_uint32_t header_vsn_num;
    boot_uint32_t image_src;
    boot_uint32_t image_dest_ptr;
    boot_uint32_t image_size;
    boot_uint32_t code_size;
    boot_uint32_t signature_ptr;
    boot_uint32_t signature_size;
    boot_uint32_t cert_chain_ptr;
    boot_uint32_t cert_chain_size;
} qcom_bootimg_t;

static int ldrmodule_magictest(boot_io_t *io, boot_uint32_t *checksum)
{
    int rc;
    int ret = -1;

    // allocate header
    qcom_bootimg_t *hdr = libboot_internal_io_alloc(io, sizeof(qcom_bootimg_t));
    if (!hdr) return ret;

    // read header
    rc = libboot_internal_io_read(io, hdr, 0, sizeof(*hdr), (void **)&hdr);
    if (rc<0) goto out;

    // verify identity
    if (hdr->image_size>0 && hdr->image_size==hdr->code_size+hdr->signature_size+hdr->cert_chain_size) {
        ret = 0;

        // calculate checksum
        if (checksum) {
            *checksum = libboot_crc32(0, (void *)hdr, sizeof(*hdr));
        }
    }

out:
    libboot_free(hdr);

    return ret;
}

static int ldrmodule_load(bootimg_context_t *context, boot_uintn_t type, boot_uint8_t recursive)
{
    int rc;
    int ret = -1;

    void *kernel_data = NULL;

    if (!(type&LIBBOOT_LOAD_TYPE_KERNEL))
        return 0;

    // allocate header
    qcom_bootimg_t *hdr = libboot_internal_io_alloc(context->io, sizeof(qcom_bootimg_t));
    if (!hdr) return ret;

    // read header
    rc = libboot_internal_io_read(context->io, hdr, 0, sizeof(*hdr), (void **)&hdr);
    if (rc<0) goto out;

    // load kernel
    boot_uintn_t kernel_size = hdr->code_size;
    boot_uintn_t kernel_offset = sizeof(*hdr) + hdr->image_src;
    // refalloc
    if (context->io->is_memio && context->io->pdata_is_allocated) {
        kernel_data = libboot_refalloc(context->io->pdata + kernel_offset, kernel_size);
        if (!kernel_data) goto out;
    }
    // read from IO
    else {
        kernel_data = libboot_internal_io_alloc(context->io, kernel_size);
        if (!kernel_data) goto out;
        rc = libboot_internal_io_read(context->io, kernel_data, kernel_offset, kernel_size, &kernel_data);
        if (rc<0) {
            //libboot_format_error(LIBBOOT_ERROR_GROUP_ANDROID, LIBBOOT_ERROR_ANDROID_READ_KERNEL, rc);
            goto err_free;
        }
    }

    // replace kernel data
    libboot_free(context->kernel_data);
    context->kernel_data = kernel_data;
    context->kernel_size = kernel_size;

    // set kernel address
    context->kernel_addr = hdr->image_dest_ptr;
    context->kernel_is_linux = 0;
    context->type = BOOTIMG_TYPE_RAW;

    ret = 0;
    goto out;

err_free:
    libboot_free(kernel_data);

out:
    libboot_free(hdr);

    return ret;
}

static ldrmodule_t ldrmodule = {
    .type = BOOTIMG_TYPE_QCMBN,
    .magic_custom_test = ldrmodule_magictest,

    .load = ldrmodule_load,
};

int libboot_internal_ldrmodule_qcmbn_init(void)
{
    libboot_internal_ldrmodule_register(&ldrmodule);
    return 0;
}
