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

static ldrmodule_t ldrmodule = {
    .type = BOOTIMG_TYPE_EFI,
    .magic_custom_test = NULL,
    .magic_off = 0,
    .magic_sz = 2,
    .magic_val = "MZ",
};

int libboot_internal_ldrmodule_efi_init(void)
{
    libboot_internal_ldrmodule_register(&ldrmodule);
    return 0;
}
