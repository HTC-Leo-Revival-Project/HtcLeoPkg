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
#include <lib/boot/internal/atags.h>

#define ATAG_MAX_SIZE   0x3000

static void *getmemory_callback(void *pdata, boot_uintn_t addr, boot_uintn_t size)
{
    tag_t *tag = pdata;

    tag = tag_next(tag);
    tag->hdr.tag = ATAG_MEM;
    tag->hdr.size = tag_size(tag_mem32);
    tag->u.mem.size  = size;
    tag->u.mem.start = addr;

    return tag;
}

static int tagmodule_patch(bootimg_context_t *context)
{
    // atags always get generated
    if (context->tags_data!=NULL)
        return -1;

    // allocate data
    void *data = libboot_alloc(ATAG_MAX_SIZE);
    if (!data) return -1;

    // generate atags
    tag_t *tag = data;

    // core
    tag->hdr.tag  = ATAG_CORE;
    tag->hdr.size = tag_size(tag_core);
    tag->u.core.flags = 0;
    tag->u.core.pagesize = 0;
    tag->u.core.rootdev = 0;

    // initrd
    if (context->ramdisk_size) {
        tag = tag_next(tag);
        tag->hdr.tag = ATAG_INITRD2;
        tag->hdr.size = tag_size(tag_initrd);
        tag->u.initrd.start = context->ramdisk_addr;
        tag->u.initrd.size  = context->ramdisk_size;
    }

    // mem
    tag = libboot_platform_getmemory(tag, getmemory_callback);

    // cmdline
    boot_uintn_t cmdline_len = libboot_cmdline_length(&context->cmdline);
    if (cmdline_len) {
        tag = tag_next(tag);
        tag->hdr.tag = ATAG_CMDLINE;
        tag->hdr.size = (sizeof(struct tag_header) + cmdline_len + 1 +  4) >> 2;
        libboot_cmdline_generate(&context->cmdline, tag->u.cmdline.cmdline, cmdline_len+1);
    }

    // platform specific tags
    if (context->add_custom_atags)
        tag  = context->add_custom_atags(tag);

    // end
    tag = tag_next(tag);
    tag->hdr.tag = ATAG_NONE;
    tag->hdr.size = 0;

    // set data
    libboot_free(context->tags_data);
    context->tags_data = data;
    context->tags_size = ATAG_MAX_SIZE;
    context->tags_type = LIBBOOT_TAGS_TYPE_ATAGS;
    context->tags_ready = 1;

    return 0;
}

static boot_uint32_t magic = ATAG_CORE;
static tagmodule_t tagmodule = {
    .type = LIBBOOT_TAGS_TYPE_ATAGS,
    .magic_custom_test = NULL,
    .magic_off = 0,
    .magic_sz = 4,
    .magic_val = &magic,

    .patch = tagmodule_patch,
};

int libboot_internal_tagmodule_atags_init(void)
{
    libboot_internal_tagmodule_register(&tagmodule);
    return 0;
}
