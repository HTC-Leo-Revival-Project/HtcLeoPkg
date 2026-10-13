/* Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 *  with the distribution.
 *   * Neither the name of The Linux Foundation nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef LIB_BOOT_INTERNAL_QCDT_H
#define LIB_BOOT_INTERNAL_QCDT_H

/*
 * For DTB V1: The DTB entries would be of the format
 * qcom,msm-id = <msm8974, CDP, rev_1>; (3 * sizeof(boot_uint32_t))
 * For DTB V2: The DTB entries would be of the format
 * qcom,msm-id   = <msm8974, rev_1>;  (2 * sizeof(boot_uint32_t))
 * qcom,board-id = <CDP, subtype_ID>; (2 * sizeof(boot_uint32_t))
 * The macros below are defined based on these.
 */
#define DT_ENTRY_V1_SIZE        (3*sizeof(boot_uint32_t))
#define PLAT_ID_SIZE            (2*sizeof(boot_uint32_t))
#define BOARD_ID_SIZE           (2*sizeof(boot_uint32_t))
#define PMIC_ID_SIZE            (4*sizeof(boot_uint32_t))

typedef struct {
    boot_uint32_t platform_id;
    boot_uint32_t variant_id;
    boot_uint32_t soc_rev;
    boot_uint32_t offset;
    boot_uint32_t size;
} dt_entry_v1_t;

typedef struct {
    boot_uint32_t platform_id;
    boot_uint32_t variant_id;
    boot_uint32_t board_hw_subtype;
    boot_uint32_t soc_rev;
    boot_uint32_t offset;
    boot_uint32_t size;
} dt_entry_v2_t;

typedef struct {
    boot_uint32_t platform_id;
    boot_uint32_t variant_id;
    boot_uint32_t board_hw_subtype;
    boot_uint32_t soc_rev;
    boot_uint32_t pmic_rev[4];
    boot_uint32_t offset;
    boot_uint32_t size;
} dt_entry_t;

typedef struct  {
    boot_uint32_t platform_id;
    boot_uint32_t soc_rev;
} plat_id_t;

typedef struct {
    boot_uint32_t variant_id;
    boot_uint32_t platform_subtype;

    union {
        struct {
            boot_uint32_t id0;
            boot_uint32_t id1;
        } oppo;
    } u;
} board_id_t;

typedef struct {
    boot_uint32_t pmic_version[4];
} pmic_id_t;

typedef enum {
    DTB_FOUNDRY = 0,
    DTB_SOC,
    DTB_MAJOR_MINOR,
    DTB_PMIC0,
    DTB_PMIC1,
    DTB_PMIC2,
    DTB_PMIC3,
    DTB_PMIC_MODEL,
    DTB_PANEL_TYPE,
    DTB_BOOT_DEVICE,
} dt_entry_info_t;

#endif // LIB_BOOT_INTERNAL_QCDT_H
