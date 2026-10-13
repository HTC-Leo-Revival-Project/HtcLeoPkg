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

#ifndef LIB_BOOT_INTERNAL_PLATFORM_H
#define LIB_BOOT_INTERNAL_PLATFORM_H

typedef void *(*libboot_platform_getmemory_callback_t)(void *pdata, boot_uintn_t addr, boot_uintn_t size);

boot_uint32_t libboot_qcdt_pmic_target(boot_uint8_t num_ent);
boot_uint32_t libboot_qcdt_platform_id(void);
boot_uint32_t libboot_qcdt_hardware_id(void);
boot_uint32_t libboot_qcdt_hardware_subtype(void);
boot_uint32_t libboot_qcdt_soc_version(void);
boot_uint32_t libboot_qcdt_target_id(void);
boot_uint32_t libboot_qcdt_foundry_id(void);
boot_uint32_t libboot_qcdt_get_hlos_subtype(void);

void  libboot_platform_memmove(void *dst, const void *src, boot_uintn_t num);
int   libboot_platform_memcmp(const void *s1, const void *s2, boot_uintn_t n);
void *libboot_platform_memset(void *s, int c, boot_uintn_t n);
void *libboot_platform_alloc(boot_uintn_t size);
void  libboot_platform_free(void *ptr);
int   libboot_platform_format_string(char *buf, boot_uintn_t sz, const char *fmt, ...) __attribute__((__format__ (__printf__, 3, 4)));
char *libboot_platform_strdup(const char *s);
char *libboot_platform_strtok_r(char *str, const char *delim, char **saveptr);
char *libboot_platform_strchr(const char *s, int c);
int   libboot_platform_strcmp(const char *str1, const char *str2);
boot_uintn_t libboot_platform_strlen(const char *str);
char *libboot_platform_strncpy(char *dest, const char *src, boot_uintn_t n);

boot_uintn_t libboot_platform_machtype(void);
void *libboot_platform_bootalloc(boot_uintn_t addr, boot_uintn_t sz);
void  libboot_platform_bootfree(boot_uintn_t addr, boot_uintn_t sz);
void *libboot_platform_getmemory(void *pdata, libboot_platform_getmemory_callback_t cb);

#endif // LIB_BOOT_INTERNAL_PLATFORM_H
