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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

boot_uint32_t libboot_qcdt_pmic_target(boot_uint8_t num_ent) {
    return 0;
}

boot_uint32_t libboot_qcdt_platform_id(void) {
    return 0;
}

boot_uint32_t libboot_qcdt_hardware_id(void) {
    return 0;
}

boot_uint32_t libboot_qcdt_hardware_subtype(void) {
    return 0;
}

boot_uint32_t libboot_qcdt_soc_version(void) {
    return 0;
}

boot_uint32_t libboot_qcdt_target_id(void) {
    return 0;
}

boot_uint32_t libboot_qcdt_foundry_id(void) {
    return 0;
}

boot_uint32_t libboot_qcdt_get_hlos_subtype(void) {
    return 0;
}

boot_uintn_t libboot_platform_machtype(void) {
    return 0;
}

void libboot_platform_memmove(void* dst, const void* src, boot_uintn_t num) {
    memmove(dst, src, num);
}

int libboot_platform_memcmp(const void *s1, const void *s2, boot_uintn_t n) {
    return memcmp(s1, s2, n);
}

void *libboot_platform_memset(void *s, int c, boot_uintn_t n) {
    return memset(s, c, n);
}

int libboot_platform_format_string(char* buf, boot_uintn_t sz, const char* fmt, ...) {
    int rc;

    va_list args;
    va_start(args, fmt);
    rc = vsnprintf(buf, sz, fmt, args);
    va_end (args);

    return rc;
}

char* libboot_platform_strdup(const char *s) {
    return strdup(s);
}

char* libboot_platform_strtok_r(char *str, const char *delim, char **saveptr) {
    return strtok_r(str, delim, saveptr);
}

char* libboot_platform_strchr(const char *s, int c) {
    return strchr(s, c);
}

int libboot_platform_strcmp(const char* str1, const char* str2) {
    return strcmp(str1, str2);
}

boot_uintn_t libboot_platform_strlen(const char* str) {
    return strlen(str);
}

char *libboot_platform_strncpy(char *dest, const char *src, boot_uintn_t n) {
    return strncpy(dest, src, n);
}

void* libboot_platform_getmemory(void *pdata, libboot_platform_getmemory_callback_t cb) {
    pdata = cb(pdata, 0x00000000, 0x20000000);
    pdata = cb(pdata, 0x20000000, 0x20000000);
    return pdata;
}

void* libboot_platform_alloc(boot_uintn_t size) {
    void* mem = malloc(size);
    if(!mem)
        libboot_format_error(LIBBOOT_ERROR_GROUP_COMMON, LIBBOOT_ERROR_COMMON_OUT_OF_MEMORY);

    return mem;
}

void libboot_platform_free(void *ptr) {
    free(ptr);
}

void* libboot_platform_bootalloc(boot_uintn_t addr, boot_uintn_t sz) {
    return NULL;
}

void libboot_platform_bootfree(boot_uintn_t addr, boot_uintn_t sz) {
}
