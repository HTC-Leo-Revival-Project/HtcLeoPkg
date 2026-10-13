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

#include <zlib.h>

#define GZIP_HEADER_LEN 10
#define GZIP_FILENAME_LIMIT 256

static void zlib_free(voidpf qpaque, void *addr)
{
    (void)(qpaque);
    return libboot_free(addr);
}

static void *zlib_alloc(voidpf qpaque, uInt items, uInt size)
{
    (void)(qpaque);
    return libboot_alloc(items * size);
}

/* decompress gzip file "in_buf", return 0 if decompressed successful,
 * return -1 if decompressed failed.
 * in_buf - input gzip file
 * in_len - input the length file
 * out_buf - output the decompressed data
 * out_buf_len - the available length of out_buf
 * pos - position of the end of gzip file
 * out_len - the length of decompressed data
 */
static int decompress(unsigned char *in_buf, unsigned int in_len,
                      unsigned char *out_buf,
                      unsigned int out_buf_len,
                      unsigned int *pos,
                      unsigned int *out_len)
{
    struct z_stream_s *stream;
    int rc = -1;
    int i;

    if (in_len < GZIP_HEADER_LEN) {
        return rc;
    }
    if (out_buf_len < in_len) {
        return rc;
    }

    stream = libboot_alloc(sizeof(*stream));
    if (stream == NULL) {
        return rc;
    }

    stream->zalloc = zlib_alloc;
    stream->zfree = zlib_free;
    stream->next_out = out_buf;
    stream->avail_out = out_buf_len;

    /* skip over gzip header */
    stream->next_in = in_buf + GZIP_HEADER_LEN;
    stream->avail_in = out_buf_len - GZIP_HEADER_LEN;
    /* skip over asciz filename */
    if (in_buf[3] & 0x8) {
        for (i = 0; i < GZIP_FILENAME_LIMIT && *stream->next_in++; i++) {
            if (stream->avail_in == 0) {
                goto gunzip_end;
            }
            --stream->avail_in;
        }
    }

    rc = inflateInit2(stream, -MAX_WBITS);
    if (rc != Z_OK) {
        goto gunzip_end;
    }

    rc = inflate(stream, 0);
    /* Z_STREAM_END is "we unpacked it all" */
    if (rc == Z_STREAM_END) {
        rc = 0;
    } else if (rc != Z_OK) {
        rc = -1;
    }

    inflateEnd(stream);
    if (pos)
        /* alculation the length of the compressed package */
        *pos = stream->next_in - in_buf + 8;

    if (out_len)
        *out_len = stream->total_out;

gunzip_end:
    libboot_free(stream);
    return rc; /* returns 0 if decompressed successful */
}

static int ldrmodule_load(bootimg_context_t *context, boot_uintn_t type, boot_uint8_t recursive)
{
    int rc;
    unsigned int out_len = 0;
    unsigned int pos = 0;
    void *data = NULL;

    if (!(type&LIBBOOT_LOAD_TYPE_KERNEL))
        return 0;

    // we're first, just load the whole thing into memory
    // TODO: remove this and use chunk based decompression
    // we also have a problem with trailing garbage here if this is the root IO
    if (!context->kernel_data) {
        if (libboot_internal_load_rawdata_to_kernel(context))
            return -1;
    }

    // get size
    boot_uint32_t size = 50*1024*1024;

    // allocate data
    data = libboot_internal_io_alloc(context->io, size);
    if (!data) {
        rc = -1;
        goto out;
    }

    // extract
    rc = decompress(context->kernel_data, context->kernel_size, data, size, &pos, &out_len);
    if (rc) {
        goto out_free;
    }

    // find appended fdt
    if (pos<context->kernel_size) {
        void *fdt = libboot_refalloc(context->kernel_data + pos, context->kernel_size - pos);
        if (!fdt) return -1;

        libboot_free(context->tags_data);
        context->tags_data = fdt;
        context->tags_size = context->kernel_size - pos;
    }

    // re-identify with kernel as image
    libboot_identify_memory(data, out_len, context);

    // replace kernel data
    libboot_free(context->kernel_data);
    context->kernel_data = data;
    context->kernel_size = out_len;

    rc = 0;
    goto out;

out_free:
    libboot_free(data);

out:
    return rc;
}

static char magic[3] = {0x1f, 0x8b, 0x08};
static ldrmodule_t ldrmodule = {
    .type = BOOTIMG_TYPE_GZIP,
    .magic_custom_test = NULL,
    .magic_off = 0,
    .magic_sz = 3,
    .magic_val = magic,

    .load = ldrmodule_load,
};

int libboot_internal_ldrmodule_gzip_init(void)
{
    libboot_internal_ldrmodule_register(&ldrmodule);
    return 0;
}
