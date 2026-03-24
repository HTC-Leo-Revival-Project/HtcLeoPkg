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
#include <lib/boot/qcdt.h>
#include <libfdt.h>

#define DTB_PAD_SIZE (1*1024*1024)

typedef struct {
    void *fdt;
    boot_uint32_t memoffset;
    boot_uint8_t do_append;

    boot_uint32_t addr_cell_size;
    boot_uint32_t size_cell_size;

    int rc;
} meminfo_pdata_t;

static int fdtloader_appendprop_string(void *fdt, int nodeoffset, const char *name, const char *str)
{
    int err, oldlen;
    struct fdt_property *prop;

    // get property
    prop = fdt_get_property_w(fdt, nodeoffset, name, &oldlen);

    // append string
    err = fdt_appendprop_string(fdt, nodeoffset, name, str);
    if (err)
        return err;

    if (prop) {
        // Add space to separate the appended strings
        prop->data[oldlen-1] = 0x20;
    }

    return 0;
}


static int fdtloader_get_cell_sizes(meminfo_pdata_t *pdata)
{
    int rc;
    int len;
    const boot_uint32_t *valp;
    boot_uint32_t offset;
    boot_uint32_t addr_cell_size = 0;
    boot_uint32_t size_cell_size = 0;

    // get root node offset
    rc = fdt_path_offset(pdata->fdt, "/");
    if (rc<0) return -1;
    offset = rc;

    // find the #address-cells size
    valp = fdt_getprop(pdata->fdt, offset, "#address-cells", &len);
    if (len<=0 || !valp) {
        if (len == -FDT_ERR_NOTFOUND)
            addr_cell_size = 2;
        else return -1;
    } else {
        addr_cell_size = fdt32_to_cpu(*valp);
    }

    // find the #size-cells size
    valp = fdt_getprop(pdata->fdt, offset, "#size-cells", &len);
    if (len<=0 || !valp) {
        if (len == -FDT_ERR_NOTFOUND)
            size_cell_size = 2;
        else return -1;
    } else {
        size_cell_size = fdt32_to_cpu(*valp);
    }

    pdata->addr_cell_size = addr_cell_size;
    pdata->size_cell_size = size_cell_size;

    return 0;
}

static int fdtloader_add_single_meminfo(meminfo_pdata_t *pdata, boot_uint64_t addr, boot_uint64_t size)
{
    int rc = 0;

    // set first addr
    if (!pdata->do_append) {
        if (pdata->addr_cell_size == 2) {
            rc = fdt_setprop_u32(pdata->fdt, pdata->memoffset, "reg", addr >> 32);
            if (rc) return -1;

            rc = fdt_appendprop_u32(pdata->fdt, pdata->memoffset, "reg", (boot_uint32_t)addr);
            if (rc) return -1;
        } else {
            rc = fdt_setprop_u32(pdata->fdt, pdata->memoffset, "reg", (boot_uint32_t)addr);
            if (rc) if (rc) return -1;
        }

        pdata->do_append = 1;
    }
    // append addr
    else {
        if (pdata->addr_cell_size == 2) {
            rc = fdt_appendprop_u32(pdata->fdt, pdata->memoffset, "reg", addr >> 32);
            if (rc) return -1;
        }

        rc = fdt_appendprop_u32(pdata->fdt, pdata->memoffset, "reg", (boot_uint32_t)addr);
        if (rc) return -1;
    }

    // append size
    if (pdata->addr_cell_size == 2) {
        rc = fdt_appendprop_u32(pdata->fdt, pdata->memoffset, "reg", size>>32);
        if (rc) return -1;
    }
    rc = fdt_appendprop_u32(pdata->fdt, pdata->memoffset, "reg", (boot_uint32_t)size);
    if (rc) return -1;

    return 0;
}

static void *getmemory_callback(void *_pdata, boot_uintn_t addr, boot_uintn_t size)
{
    meminfo_pdata_t *pdata = _pdata;

    // a previous call failed
    if (pdata->rc) return pdata;

    // add meminfo
    pdata->rc = fdtloader_add_single_meminfo(pdata, (boot_uint64_t)addr, (boot_uint64_t)size);

    return pdata;
}

static int fdtloader_add_meminfo(void *fdt)
{
    int rc;

    meminfo_pdata_t pdata = {
        .fdt = fdt,
        .memoffset = 0,
        .do_append = 0,
        .addr_cell_size = 1,
        .size_cell_size = 1,
        .rc = 0,
    };

    // get memory node offset
    rc = fdt_path_offset(fdt, "/memory");
    if (rc<0) return -1;
    pdata.memoffset = rc;

    // get cell sizes
    fdtloader_get_cell_sizes(&pdata);

    // add all memory ranges
    libboot_platform_getmemory(&pdata, getmemory_callback);
    if (pdata.rc) return -1;

    return 0;
}

static boot_uintn_t fdtloader_fdt_count(void *fdt, boot_uintn_t size)
{
    boot_uintn_t i = 0;
    while (fdt+sizeof(struct fdt_header) < fdt+size) {
        if (fdt_check_header(fdt)) break;
        boot_uintn_t fdtsize = fdt_totalsize(fdt);

        // next
        fdt += fdtsize;
        i++;
    }

    return i;
}

int fdtloader_process_multifdt(bootimg_context_t *context)
{
    int rc;

    // get pointer to compatible fdt
    void *fdt = libboot_qcdt_appended(context->tags_data, context->tags_size, context->fdt_parser);
    if (!fdt) return -1;

    // check fdt header
    rc = fdt_check_header(fdt);
    if (rc) return -1;

    // get size
    boot_intn_t fdt_size = fdt_totalsize(fdt);
    if (fdt_size<=0) return -1;

    void *tags_data = NULL;
    if (IS_ALIGNED_M1CHA(fdt, sizeof(boot_uintn_t))) {
        // refalloc fdt
        tags_data = libboot_refalloc(fdt, fdt_size);
        if (!tags_data) return -1;
    } else {
        // allocate new (aligned) memory
        tags_data = libboot_alloc(fdt_size);
        if (!tags_data) return -1;

        // copy data to new memory
        libboot_platform_memmove(tags_data, fdt, fdt_size);
    }

    // replace tags
    libboot_free(context->tags_data);
    context->tags_data = tags_data;
    context->tags_size = fdt_size;

    return 0;
}

static int tagmodule_patch(bootimg_context_t *context)
{
    int rc;
    int ret = -1;
    void *fdt = NULL;
    boot_uint32_t offset;

    // use default fdt
    if (context->tags_data==NULL) {
        if (!context->default_fdt) return -1;

        context->tags_data = context->default_fdt;
        context->tags_size = fdt_totalsize(context->tags_data);
        context->tags_type = LIBBOOT_TAGS_TYPE_FDT;
    }

    // process multi fdt in case this is one
    if (fdtloader_fdt_count(context->tags_data, context->tags_size)>1) {
        rc = fdtloader_process_multifdt(context);
        if (rc) goto out;
    }

    // check fdt header
    rc = fdt_check_header(context->tags_data);
    if (rc) goto out;

    // allocate fdt copy with padding
    boot_uintn_t newsize = fdt_totalsize(context->tags_data) + DTB_PAD_SIZE;
    fdt = libboot_alloc(newsize);
    if (!fdt) goto out;

    // copy fdt to new memory
    rc = fdt_open_into(context->tags_data, fdt, newsize);
    if (rc) goto out_free;

    // add memory info
    rc = fdtloader_add_meminfo(fdt);
    if (rc<0) goto out_free;

    // get chosen node offset
    rc = fdt_path_offset(fdt, "/chosen");
    if (rc<0) goto out_free;
    offset = rc;

    // cmdline
    boot_uintn_t cmdline_len = libboot_cmdline_length(&context->cmdline);
    if (cmdline_len) {
        char *cmdline = libboot_alloc(cmdline_len);
        libboot_cmdline_generate(&context->cmdline, cmdline, cmdline_len);

        rc = fdtloader_appendprop_string(fdt, offset, "bootargs", cmdline);
        libboot_free(cmdline);
        if (rc) goto out_free;
    }

    // ramdisk
    if (context->ramdisk_size) {
        rc = fdt_setprop_u32(fdt, offset, "linux,initrd-start", (boot_uint32_t)context->ramdisk_addr);
        if (rc) goto out_free;

        rc = fdt_setprop_u32(fdt, offset, "linux,initrd-end", ((boot_uint32_t)context->ramdisk_addr + context->ramdisk_size));
        if (rc) goto out_free;
    }

    // platform specific patches
    if (context->patch_fdt)
        context->patch_fdt(fdt);

    // pack fdt
    fdt_pack(fdt);

    // set new fdt
    libboot_free(context->tags_data);
    context->tags_data = fdt;
    context->tags_size = fdt_totalsize(fdt);
    context->tags_ready = 1;

    ret = 0;
    goto out;

out_free:
    libboot_free(fdt);

out:
    return ret;
}

static boot_uint32_t magic;
static tagmodule_t tagmodule = {
    .type = LIBBOOT_TAGS_TYPE_FDT,
    .magic_custom_test = NULL,
    .magic_off = 0,
    .magic_sz = 4,
    .magic_val = &magic,

    .patch = tagmodule_patch,
};

int libboot_internal_tagmodule_fdt_init(void)
{
    magic = fdt32_to_cpu(FDT_MAGIC);
    libboot_internal_tagmodule_register(&tagmodule);
    return 0;
}
