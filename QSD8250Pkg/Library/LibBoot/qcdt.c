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

#include <lib/boot.h>
#include <lib/boot/internal/boot_internal.h>
#include <lib/boot/qcdt.h>
#include <lib/boot/internal/qcdt.h>
#include <libfdt.h>

#ifndef __WEAK
#define __WEAK __attribute__((weak))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
#endif

typedef enum {
    FDT_PARSER_UNKNOWN = -1,
    FDT_PARSER_QCOM = 0,
    FDT_PARSER_QCOM_LGE,
    FDT_PARSER_QCOM_OPPO,
    FDT_PARSER_QCOM_MOTOROLA,
} fdt_parser_t;

static int devtree_entry_add_if_excact_match(dt_entry_local_t *cur_dt_entry, dt_entry_node_t *dt_list);
static dt_entry_local_t *devtree_get_best_entry(dt_entry_node_t *dt_list);
static int devtree_delete_incompatible_entries2(dt_entry_node_t *dt_list, boot_uint32_t dtb_info);

static const char *msm_id_names[] = {
    "htc,project-id",
    "qcom,msm-id",
};

__WEAK boot_uint32_t libboot_qcdt_get_lge_rev(void)
{
    return 0;
}

__WEAK boot_uint32_t libboot_qcdt_get_oppo_id0(void)
{
    return 0;
}

__WEAK boot_uint32_t libboot_qcdt_get_oppo_id1(void)
{
    return 0;
}

__WEAK const char *libboot_qcdt_get_motorola_model(void)
{
    return "";
}

__WEAK const char *libboot_qcdt_get_default_parser(void)
{
    return NULL;
}

/* Add function to allocate dt entry list, used for recording
*  the entry which conform to devtree_entry_add_if_excact_match()
*/
dt_entry_node_t *dt_entry_list_alloc_node(void)
{
    dt_entry_node_t *dt_node_member = NULL;

    dt_node_member = (dt_entry_node_t *) libboot_alloc(sizeof(dt_entry_node_t));
    LIBBOOT_ASSERT(dt_node_member);

    libboot_list_clear_node(&dt_node_member->node);

    dt_node_member->dt_entry_m = (dt_entry_local_t *) libboot_alloc(sizeof(dt_entry_local_t));
    LIBBOOT_ASSERT(dt_node_member->dt_entry_m);

    libboot_platform_memset(dt_node_member->dt_entry_m , 0, sizeof(dt_entry_local_t));
    return dt_node_member;
}

void dt_entry_list_insert(dt_entry_node_t *dt_list, dt_entry_node_t *dt_node_member)
{
    libboot_list_add_tail(&dt_list->node, &dt_node_member->node);
}

void dt_entry_list_delete(dt_entry_node_t *dt_node_member)
{
    if (libboot_list_in_list(&dt_node_member->node)) {
        libboot_list_delete(&dt_node_member->node);
        libboot_free(dt_node_member->dt_entry_m);
        libboot_free(dt_node_member);
    }
}

dt_entry_node_t *dt_entry_list_create(void)
{
    dt_entry_node_t *dt_list = (dt_entry_node_t *) libboot_alloc(sizeof(dt_entry_node_t));
    if (!dt_list)
        return NULL;

    libboot_list_initialize(&dt_list->node);
    dt_list->dt_entry_m = NULL;

    return dt_list;
}

void dt_entry_list_free(dt_entry_node_t *dt_list)
{
    while (!libboot_list_is_empty(&dt_list->node)) {
        dt_entry_node_t *dt_node = libboot_list_remove_tail_type(&dt_list->node, dt_entry_node_t, node);
        dt_entry_list_delete(dt_node);
    }
    libboot_free(dt_list);
}

static fdt_parser_t libboot_qcdt_get_parser(const char *parser)
{
    if (!parser)
        return FDT_PARSER_UNKNOWN;

    if (!libboot_platform_strcmp(parser, "qcom"))
        return FDT_PARSER_QCOM;
    if (!libboot_platform_strcmp(parser, "qcom_lge"))
        return FDT_PARSER_QCOM_LGE;
    if (!libboot_platform_strcmp(parser, "qcom_oppo"))
        return FDT_PARSER_QCOM_OPPO;
    if (!libboot_platform_strcmp(parser, "qcom_motorola"))
        return FDT_PARSER_QCOM_MOTOROLA;

    return FDT_PARSER_UNKNOWN;
}

int libboot_qcdt_generate_entries(void *dtb, boot_uint32_t dtb_size, dt_entry_node_t *dtb_list, dt_entry_add_cb_t cb, const char *sparser)
{
    int root_offset;
    const void *prop = NULL;
    const char *plat_prop = NULL;
    const char *board_prop = NULL;
    const char *pmic_prop = NULL;
    char *model = NULL;
    dt_entry_local_t *cur_dt_entry;
    dt_entry_local_t *dt_entry_array = NULL;
    board_id_t *board_data = NULL;
    plat_id_t *platform_data = NULL;
    pmic_id_t *pmic_data = NULL;
    int len;
    int len_board_id;
    int len_board_id_item = 0;
    int len_plat_id;
    int len_plat_id_item = 0;
    int len_pmic_id;
    int len_pmic_id_item = 0;
    boot_uint32_t dtb_ver;
    boot_uint32_t num_entries = 0;
    boot_uint32_t i, j, k, n;
    boot_uint32_t msm_data_count = 0;
    boot_uint32_t board_data_count = 0;
    boot_uint32_t pmic_data_count = 0;
    fdt_parser_t parser;

    // use default parser
    if (!sparser)
        sparser = libboot_qcdt_get_default_parser();
    if (!sparser)
        sparser = "qcom";

    parser = libboot_qcdt_get_parser(sparser);
    if (parser==FDT_PARSER_UNKNOWN) {
        libboot_format_error(LIBBOOT_ERROR_GROUP_QCDT, LIBBOOT_ERROR_QCDT_UNKNOWN_PARSER, sparser);
        return 0;
    }

    root_offset = fdt_path_offset(dtb, "/");
    if (root_offset < 0) {
        libboot_format_error(LIBBOOT_ERROR_GROUP_QCDT, LIBBOOT_ERROR_QCDT_PATH_NOT_FOUND, "/", fdt_strerror(root_offset));
        return 0;
    }

    prop = fdt_getprop(dtb, root_offset, "model", &len);
    if (prop && len > 0) {
        model = (char *) libboot_alloc(sizeof(char) * len);
        LIBBOOT_ASSERT(model);
        libboot_internal_strlcpy(model, prop, len);
    } else {
        LOGI("model does not exist in device tree\n");
    }
    /* Find the pmic-id prop from DTB , if pmic-id is present then
    * the DTB is version 3, otherwise find the board-id prop from DTB ,
    * if board-id is present then the DTB is version 2 */
    pmic_prop = (const char *)fdt_getprop(dtb, root_offset, "qcom,pmic-id", &len_pmic_id);
    board_prop = (const char *)fdt_getprop(dtb, root_offset, "qcom,board-id", &len_board_id);
    if (pmic_prop && (len_pmic_id > 0) && board_prop && (len_board_id > 0)) {
        dtb_ver = DEV_TREE_VERSION_V3;
        len_plat_id_item = PLAT_ID_SIZE;
        len_board_id_item = BOARD_ID_SIZE;
        len_pmic_id_item = PMIC_ID_SIZE;
    } else if (board_prop && len_board_id > 0) {
        dtb_ver = DEV_TREE_VERSION_V2;
        len_plat_id_item = PLAT_ID_SIZE;
        len_board_id_item = BOARD_ID_SIZE;
    } else {
        dtb_ver = DEV_TREE_VERSION_V1;
        len_plat_id_item = DT_ENTRY_V1_SIZE;
        len_board_id_item = BOARD_ID_SIZE;

        if (parser==FDT_PARSER_QCOM_LGE) {
            len_plat_id_item = 4 * sizeof(boot_uint32_t);
        }
    }

    if (parser==FDT_PARSER_QCOM_OPPO) {
        len_board_id_item = 4 * sizeof(boot_uint32_t);
    }

    if (dtb_ver == DEV_TREE_VERSION_V2 || dtb_ver == DEV_TREE_VERSION_V3) {
        if (len_board_id % len_board_id_item) {
            libboot_format_error(LIBBOOT_ERROR_GROUP_QCDT, LIBBOOT_ERROR_QCDT_NOT_A_MULTIPLE, "qcom,board-id", len_board_id, len_board_id_item);
            return 0;
        }
    }

    if (dtb_ver == DEV_TREE_VERSION_V3) {
        if ((len_pmic_id % len_pmic_id_item)) {
            libboot_format_error(LIBBOOT_ERROR_GROUP_QCDT, LIBBOOT_ERROR_QCDT_NOT_A_MULTIPLE, "qcom,pmic-id", len_pmic_id, len_pmic_id_item);
            return 0;
        }
    }

    /* Get the msm-id prop from DTB */
    const char *msm_id_prop_name;
    for (i=0; i<ARRAY_SIZE(msm_id_names); i++) {
        plat_prop = (const char *)fdt_getprop(dtb, root_offset, msm_id_names[i], &len_plat_id);
        if (plat_prop) {
            msm_id_prop_name = msm_id_names[i];
            break;
        }
    }

    if (!plat_prop || len_plat_id <= 0) {
        libboot_format_error(LIBBOOT_ERROR_GROUP_QCDT, LIBBOOT_ERROR_QCDT_ID_ENTRY_NOT_FOUND);
        return 0;
    } else if (len_plat_id % len_plat_id_item) {
        libboot_format_error(LIBBOOT_ERROR_GROUP_QCDT, LIBBOOT_ERROR_QCDT_NOT_A_MULTIPLE, msm_id_prop_name, len_plat_id, len_plat_id_item);
        return 0;
    }

    /*
     * If DTB version is '1' look for <x y z> pair in the DTB
     * x: platform_id
     * y: variant_id
     * z: SOC rev
     */
    if (dtb_ver == DEV_TREE_VERSION_V1) {
        cur_dt_entry = (dt_entry_local_t *)
                       libboot_alloc(sizeof(dt_entry_local_t));

        if (!cur_dt_entry) {
            return 0;
        }
        libboot_platform_memset(cur_dt_entry, 0, sizeof(dt_entry_local_t));

        while (len_plat_id) {
            cur_dt_entry->data.version = dtb_ver;
            cur_dt_entry->data.platform_id = fdt32_to_cpu(((const dt_entry_v1_t *)plat_prop)->platform_id);
            cur_dt_entry->data.variant_id = fdt32_to_cpu(((const dt_entry_v1_t *)plat_prop)->variant_id);
            cur_dt_entry->data.soc_rev = fdt32_to_cpu(((const dt_entry_v1_t *)plat_prop)->soc_rev);
            cur_dt_entry->data.board_hw_subtype =
                fdt32_to_cpu(((const dt_entry_v1_t *)plat_prop)->variant_id) >> 0x18;
            cur_dt_entry->data.pmic_rev[0] = libboot_qcdt_pmic_target(0);
            cur_dt_entry->data.pmic_rev[1] = libboot_qcdt_pmic_target(1);
            cur_dt_entry->data.pmic_rev[2] = libboot_qcdt_pmic_target(2);
            cur_dt_entry->data.pmic_rev[3] = libboot_qcdt_pmic_target(3);
            cur_dt_entry->dtb_data = dtb;
            cur_dt_entry->dtb_size = dtb_size;
            cur_dt_entry->parser = sparser;

            if (parser==FDT_PARSER_QCOM_LGE) {
                cur_dt_entry->data.u.lge.lge_rev = fdt32_to_cpu(*((boot_uint32_t *)(plat_prop + 3*sizeof(boot_uint32_t))));
            }

            if (parser==FDT_PARSER_QCOM_MOTOROLA) {
                cur_dt_entry->data.u.motorola.version = 1;

                cur_dt_entry->data.u.motorola.model[0] = 0;
                if (model)
                    libboot_platform_strncpy(cur_dt_entry->data.u.motorola.model, model, sizeof(cur_dt_entry->data.u.motorola.model));
            }

            cb(cur_dt_entry, dtb_list, model);

            plat_prop += len_plat_id_item;
            len_plat_id -= len_plat_id_item;
        }
        libboot_free(cur_dt_entry);

    }
    /*
     * If DTB Version is '3' then we have split DTB with board & msm data & pmic
     * populated saperately in board-id & msm-id & pmic-id prop respectively.
     * Extract the data & prepare a look up table
     */
    else if (dtb_ver == DEV_TREE_VERSION_V2 || dtb_ver == DEV_TREE_VERSION_V3) {
        if (len_board_id_item>0)
            board_data_count = (len_board_id / len_board_id_item);
        if (len_plat_id_item>0)
            msm_data_count = (len_plat_id / len_plat_id_item);
        /* If dtb version is v2.0, the pmic_data_count will be <= 0 */
        if (len_pmic_id_item>0)
            pmic_data_count = (len_pmic_id / len_pmic_id_item);

        /* If we are using dtb v3.0, then we have split board, msm & pmic data in the DTB
        *  If we are using dtb v2.0, then we have split board & msmdata in the DTB
        */
        board_data = (board_id_t *) libboot_alloc(sizeof(board_id_t) * board_data_count);
        LIBBOOT_ASSERT(board_data);
        platform_data = (plat_id_t *) libboot_alloc(sizeof(plat_id_t) * msm_data_count);
        LIBBOOT_ASSERT(platform_data);
        if (dtb_ver == DEV_TREE_VERSION_V3) {
            pmic_data = (pmic_id_t *) libboot_alloc(sizeof(pmic_id_t) * pmic_data_count);
            LIBBOOT_ASSERT(pmic_data);
        }
        i = 0;

        /* Extract board data from DTB */
        for (i = 0 ; i < board_data_count; i++) {
            board_data[i].variant_id = fdt32_to_cpu(((board_id_t *)board_prop)->variant_id);
            board_data[i].platform_subtype = fdt32_to_cpu(((board_id_t *)board_prop)->platform_subtype);

            if (parser==FDT_PARSER_QCOM_OPPO) {
                board_data[i].u.oppo.id0 = fdt32_to_cpu(((board_id_t *)board_prop)->u.oppo.id0);
                board_data[i].u.oppo.id1 = fdt32_to_cpu(((board_id_t *)board_prop)->u.oppo.id1);
            }

            /* For V2/V3 version of DTBs we have platform version field as part
             * of variant ID, in such case the subtype will be mentioned as 0x0
             * As the qcom, board-id = <0xSSPMPmPH, 0x0>
             * SS -- Subtype
             * PM -- Platform major version
             * Pm -- Platform minor version
             * PH -- Platform hardware CDP/MTP
             * In such case to make it compatible with LK algorithm move the subtype
             * from variant_id to subtype field
             */
            if (board_data[i].platform_subtype == 0)
                board_data[i].platform_subtype =
                    fdt32_to_cpu(((board_id_t *)board_prop)->variant_id) >> 0x18;

            len_board_id -= len_board_id_item;
            board_prop += len_board_id_item;
        }

        /* Extract platform data from DTB */
        for (i = 0 ; i < msm_data_count; i++) {
            platform_data[i].platform_id = fdt32_to_cpu(((plat_id_t *)plat_prop)->platform_id);
            platform_data[i].soc_rev = fdt32_to_cpu(((plat_id_t *)plat_prop)->soc_rev);
            len_plat_id -= len_plat_id_item;
            plat_prop += len_plat_id_item;
        }

        if (dtb_ver == DEV_TREE_VERSION_V3 && pmic_prop) {
            /* Extract pmic data from DTB */
            for (i = 0 ; i < pmic_data_count; i++) {
                pmic_data[i].pmic_version[0]= fdt32_to_cpu(((pmic_id_t *)pmic_prop)->pmic_version[0]);
                pmic_data[i].pmic_version[1]= fdt32_to_cpu(((pmic_id_t *)pmic_prop)->pmic_version[1]);
                pmic_data[i].pmic_version[2]= fdt32_to_cpu(((pmic_id_t *)pmic_prop)->pmic_version[2]);
                pmic_data[i].pmic_version[3]= fdt32_to_cpu(((pmic_id_t *)pmic_prop)->pmic_version[3]);
                len_pmic_id -= len_pmic_id_item;
                pmic_prop += len_pmic_id_item;
            }

            /* We need to merge board & platform data into dt entry structure */
            num_entries = msm_data_count * board_data_count * pmic_data_count;
        } else {
            /* We need to merge board & platform data into dt entry structure */
            num_entries = msm_data_count * board_data_count;
        }

        if ((((boot_uint64_t)msm_data_count * (boot_uint64_t)board_data_count * (boot_uint64_t)pmic_data_count) !=
                msm_data_count * board_data_count * pmic_data_count) ||
                (((boot_uint64_t)msm_data_count * (boot_uint64_t)board_data_count) != msm_data_count * board_data_count)) {

            libboot_free(board_data);
            libboot_free(platform_data);
            if (pmic_data)
                libboot_free(pmic_data);
            if (model)
                libboot_free(model);
            return 0;
        }

        dt_entry_array = (dt_entry_local_t *) libboot_alloc(sizeof(dt_entry_local_t) * num_entries);
        LIBBOOT_ASSERT(dt_entry_array);

        /* If we have '<X>; <Y>; <Z>' as platform data & '<A>; <B>; <C>' as board data.
         * Then dt entry should look like
         * <X ,A >;<X, B>;<X, C>;
         * <Y ,A >;<Y, B>;<Y, C>;
         * <Z ,A >;<Z, B>;<Z, C>;
         */
        i = 0;
        k = 0;
        n = 0;
        for (i = 0; i < msm_data_count; i++) {
            for (j = 0; j < board_data_count; j++) {
                if (dtb_ver == DEV_TREE_VERSION_V3 && pmic_prop) {
                    for (n = 0; n < pmic_data_count; n++) {
                        dt_entry_array[k].data.version = dtb_ver;
                        dt_entry_array[k].data.platform_id = platform_data[i].platform_id;
                        dt_entry_array[k].data.soc_rev = platform_data[i].soc_rev;
                        dt_entry_array[k].data.variant_id = board_data[j].variant_id;
                        dt_entry_array[k].data.board_hw_subtype = board_data[j].platform_subtype;
                        dt_entry_array[k].data.pmic_rev[0]= pmic_data[n].pmic_version[0];
                        dt_entry_array[k].data.pmic_rev[1]= pmic_data[n].pmic_version[1];
                        dt_entry_array[k].data.pmic_rev[2]= pmic_data[n].pmic_version[2];
                        dt_entry_array[k].data.pmic_rev[3]= pmic_data[n].pmic_version[3];
                        dt_entry_array[k].dtb_data = dtb;
                        dt_entry_array[k].dtb_size = dtb_size;
                        dt_entry_array[k].parser = sparser;

                        if (parser==FDT_PARSER_QCOM_OPPO) {
                            dt_entry_array[k].data.u.oppo.id0 = board_data[j].u.oppo.id0;
                            dt_entry_array[k].data.u.oppo.id1 = board_data[j].u.oppo.id1;
                        }

                        if (parser==FDT_PARSER_QCOM_MOTOROLA) {
                            dt_entry_array[k].data.u.motorola.version = 1;

                            dt_entry_array[k].data.u.motorola.model[0] = 0;
                            if (model)
                                libboot_platform_strncpy(dt_entry_array[k].data.u.motorola.model, model, sizeof(dt_entry_array[k].data.u.motorola.model));
                        }

                        k++;
                    }

                } else {
                    dt_entry_array[k].data.version = dtb_ver;
                    dt_entry_array[k].data.platform_id = platform_data[i].platform_id;
                    dt_entry_array[k].data.soc_rev = platform_data[i].soc_rev;
                    dt_entry_array[k].data.variant_id = board_data[j].variant_id;
                    dt_entry_array[k].data.board_hw_subtype = board_data[j].platform_subtype;
                    dt_entry_array[k].data.pmic_rev[0]= libboot_qcdt_pmic_target(0);
                    dt_entry_array[k].data.pmic_rev[1]= libboot_qcdt_pmic_target(1);
                    dt_entry_array[k].data.pmic_rev[2]= libboot_qcdt_pmic_target(2);
                    dt_entry_array[k].data.pmic_rev[3]= libboot_qcdt_pmic_target(3);
                    dt_entry_array[k].dtb_data = dtb;
                    dt_entry_array[k].dtb_size = dtb_size;
                    dt_entry_array[k].parser = sparser;

                    if (parser==FDT_PARSER_QCOM_OPPO) {
                        dt_entry_array[k].data.u.oppo.id0 = board_data[j].u.oppo.id0;
                        dt_entry_array[k].data.u.oppo.id1 = board_data[j].u.oppo.id1;
                    }

                    if (parser==FDT_PARSER_QCOM_MOTOROLA) {
                        dt_entry_array[k].data.u.motorola.version = 1;

                        dt_entry_array[k].data.u.motorola.model[0] = 0;
                        if (model)
                            libboot_platform_strncpy(dt_entry_array[k].data.u.motorola.model, model, sizeof(dt_entry_array[k].data.u.motorola.model));
                    }

                    k++;
                }
            }
        }

        for (i=0 ; i < num_entries; i++) {
            cb(&(dt_entry_array[i]), dtb_list, model);
        }

        libboot_free(board_data);
        libboot_free(platform_data);
        if (pmic_data)
            libboot_free(pmic_data);
        libboot_free(dt_entry_array);
    }
    if (model)
        libboot_free(model);
    return 1;
}

static void generate_entries_add_cb(dt_entry_local_t *dt_entry, dt_entry_node_t *dt_list, const char *model)
{
    LOGV("Found an appended flattened device tree (%s - %u %u %u 0x%x)\n",
         *model ? model : "unknown",
         dt_entry->data.platform_id, dt_entry->data.variant_id, dt_entry->data.board_hw_subtype, dt_entry->data.soc_rev);

    if (devtree_entry_add_if_excact_match(dt_entry, dt_list)) {
        LOGV("Device tree exact match the board: <%u %u %u 0x%x> == <%u %u %u 0x%x>\n",
             dt_entry->data.platform_id,
             dt_entry->data.variant_id,
             dt_entry->data.soc_rev,
             dt_entry->data.board_hw_subtype,
             libboot_qcdt_platform_id(),
             libboot_qcdt_hardware_id(),
             libboot_qcdt_hardware_subtype(),
             libboot_qcdt_soc_version());

    } else {
        LOGV("Device tree's msm_id doesn't match the board: <%u %u %u 0x%x> != <%u %u %u 0x%x>\n",
             dt_entry->data.platform_id,
             dt_entry->data.variant_id,
             dt_entry->data.soc_rev,
             dt_entry->data.board_hw_subtype,
             libboot_qcdt_platform_id(),
             libboot_qcdt_hardware_id(),
             libboot_qcdt_hardware_subtype(),
             libboot_qcdt_soc_version());
    }
}

static int libboot_qcdt_add_compatible_entries(void *dtb, boot_uint32_t dtb_size, dt_entry_node_t *dtb_list, const char *parser)
{
    return libboot_qcdt_generate_entries(dtb, dtb_size, dtb_list, generate_entries_add_cb, parser);
}

/*
 * Will relocate the DTB to the tags addr if the device tree is found and return
 * its address
 *
 * Arguments:    kernel - Start address of the kernel loaded in RAM
 *               tags - Start address of the tags loaded in RAM
 *               kernel_size - Size of the kernel in bytes
 *
 * Return Value: DTB address : If appended device tree is found
 *               'NULL'         : Otherwise
 */
void *libboot_qcdt_appended(void *fdt, boot_uintn_t fdt_size, const char *parser)
{
    void *fdt_end = fdt + fdt_size;
    void *dtb = NULL;
    void *bestmatch_tag = NULL;
    dt_entry_local_t *best_match_dt_entry = NULL;
    dt_entry_node_t *dt_entry_queue = NULL;


    /* Initialize the dtb entry node*/
    dt_entry_queue = dt_entry_list_create();

    if (!dt_entry_queue) {
        return NULL;
    }

    dtb = fdt;
    while (((boot_uintn_t)dtb + sizeof(struct fdt_header)) < (boot_uintn_t)fdt_end) {
        struct fdt_header dtb_hdr;
        boot_uint32_t dtb_size;

        /* the DTB could be unaligned, so extract the header,
         * and operate on it separately */
        libboot_platform_memmove(&dtb_hdr, dtb, sizeof(struct fdt_header));
        if (fdt_check_header((const void *)&dtb_hdr) != 0 ||
                ((boot_uintn_t)dtb + (boot_uintn_t)fdt_totalsize((const void *)&dtb_hdr) < (boot_uintn_t)dtb) ||
                ((boot_uintn_t)dtb + (boot_uintn_t)fdt_totalsize((const void *)&dtb_hdr) > (boot_uintn_t)fdt_end))
            break;
        dtb_size = fdt_totalsize(&dtb_hdr);

        libboot_qcdt_add_compatible_entries(dtb, dtb_size, dt_entry_queue, parser);

        /* goto the next device tree if any */
        dtb += dtb_size;
    }

    best_match_dt_entry = devtree_get_best_entry(dt_entry_queue);
    if (best_match_dt_entry) {
        bestmatch_tag = best_match_dt_entry->dtb_data;
        LOGI("Best match DTB tags %u/%08x/0x%08x/%x/%x/%x/%x/%x/%p/%x\n",
             best_match_dt_entry->data.platform_id, best_match_dt_entry->data.variant_id,
             best_match_dt_entry->data.board_hw_subtype, best_match_dt_entry->data.soc_rev,
             best_match_dt_entry->data.pmic_rev[0], best_match_dt_entry->data.pmic_rev[1],
             best_match_dt_entry->data.pmic_rev[2], best_match_dt_entry->data.pmic_rev[3],
             best_match_dt_entry->dtb_data, best_match_dt_entry->dtb_size);
        LOGI("Using pmic info 0x%0x/0x%x/0x%x/0x%0x for device 0x%0x/0x%x/0x%x/0x%0x\n",
             best_match_dt_entry->data.pmic_rev[0], best_match_dt_entry->data.pmic_rev[1],
             best_match_dt_entry->data.pmic_rev[2], best_match_dt_entry->data.pmic_rev[3],
             libboot_qcdt_pmic_target(0), libboot_qcdt_pmic_target(1),
             libboot_qcdt_pmic_target(2), libboot_qcdt_pmic_target(3));
    }

    /* libboot_free queue's memory */
    dt_entry_list_free(dt_entry_queue);

    if (bestmatch_tag) {
        return bestmatch_tag;
    }

    libboot_format_error(LIBBOOT_ERROR_GROUP_QCDT, LIBBOOT_ERROR_QCDT_NO_MATCH,
                         libboot_qcdt_platform_id(),
                         libboot_qcdt_hardware_id(),
                         libboot_qcdt_soc_version(),
                         libboot_qcdt_pmic_target(0), libboot_qcdt_pmic_target(1),
                         libboot_qcdt_pmic_target(2), libboot_qcdt_pmic_target(3));

    return NULL;
}

/* Returns 0 if the device tree is valid. */
int libboot_qcdt_validate(dt_table_t *table, boot_uint32_t *dt_hdr_size)
{
    int dt_entry_size;
    boot_uint64_t hdr_size;
    boot_uint32_t qcdt_version;

    /* Validate the device tree table header */
    if (table->magic != DEV_TREE_MAGIC) {
        libboot_format_error(LIBBOOT_ERROR_GROUP_QCDT, LIBBOOT_ERROR_QCDT_INVALID_MAGIC);
        return -1;
    }

    qcdt_version = table->version & 0xff;
    if (qcdt_version == DEV_TREE_VERSION_V1) {
        dt_entry_size = sizeof(dt_entry_v1_t);
    } else if (qcdt_version == DEV_TREE_VERSION_V2) {
        dt_entry_size = sizeof(dt_entry_v2_t);
    } else if (qcdt_version == DEV_TREE_VERSION_V3) {
        dt_entry_size = sizeof(dt_entry_t);
    } else {
        libboot_format_error(LIBBOOT_ERROR_GROUP_QCDT, LIBBOOT_ERROR_QCDT_UNSUPPORTED_VERSION, table->version);
        return -1;
    }

    hdr_size = (boot_uint64_t)table->num_entries * dt_entry_size + DEV_TREE_HEADER_SIZE;

    if (hdr_size > UINT_MAX) {
        libboot_format_error(LIBBOOT_ERROR_GROUP_QCDT, LIBBOOT_ERROR_QCDT_INVALID_HEADER_SIZE);
        return -1;
    } else
        *dt_hdr_size = hdr_size & UINT_MAX;

    return 0;
}

static int devtree_entry_add_if_excact_match(dt_entry_local_t *cur_dt_entry, dt_entry_node_t *dt_list)
{
    boot_uint32_t cur_dt_hlos_ddr;
    boot_uint32_t cur_dt_hw_platform;
    boot_uint32_t cur_dt_hw_subtype;
    boot_uint32_t cur_dt_msm_id;
    dt_entry_node_t *dt_node_tmp = NULL;

    /* Platform-id
    * bit no |31     24|23  16|15   0|
    *        |reserved|foundry-id|msm-id|
    */
    cur_dt_msm_id = (cur_dt_entry->data.platform_id & 0x0000ffff);
    cur_dt_hw_platform = (cur_dt_entry->data.variant_id & 0x000000ff);
    cur_dt_hw_subtype = (cur_dt_entry->data.board_hw_subtype & 0xff);

    /* Determine the bits 10:8 to check the DT with the DDR Size */
    cur_dt_hlos_ddr = (cur_dt_entry->data.board_hw_subtype & 0x700);

    /* 1. must match the msm_id, platform_hw_id, platform_subtype and DDR size
    *  soc, board major/minor, pmic major/minor must less than board info
    *  2. find the matched DTB then return 1
    *  3. otherwise return 0
    */
    if ((cur_dt_msm_id == (libboot_qcdt_platform_id() & 0x0000ffff)) &&
            (cur_dt_hw_platform == libboot_qcdt_hardware_id()) &&
            (cur_dt_hw_subtype == libboot_qcdt_hardware_subtype()) &&
            (cur_dt_hlos_ddr == (libboot_qcdt_get_hlos_subtype() & 0x700)) &&
            (cur_dt_entry->data.soc_rev <= libboot_qcdt_soc_version()) &&
            ((cur_dt_entry->data.variant_id & 0x00ffff00) <= (libboot_qcdt_target_id() & 0x00ffff00)) &&
            ((cur_dt_entry->data.pmic_rev[0] & 0x00ffff00) <= (libboot_qcdt_pmic_target(0) & 0x00ffff00)) &&
            ((cur_dt_entry->data.pmic_rev[1] & 0x00ffff00) <= (libboot_qcdt_pmic_target(1) & 0x00ffff00)) &&
            ((cur_dt_entry->data.pmic_rev[2] & 0x00ffff00) <= (libboot_qcdt_pmic_target(2) & 0x00ffff00)) &&
            ((cur_dt_entry->data.pmic_rev[3] & 0x00ffff00) <= (libboot_qcdt_pmic_target(3) & 0x00ffff00))) {

        dt_node_tmp = dt_entry_list_alloc_node();
        libboot_platform_memmove((char *)dt_node_tmp->dt_entry_m,(char *)cur_dt_entry, sizeof(dt_entry_local_t));

        if (!libboot_platform_strcmp(cur_dt_entry->parser, "qcom_oppo")) {
            if (cur_dt_entry->data.u.oppo.id0!=libboot_qcdt_get_oppo_id0()) {
                goto incompatible_entry;
            }
            if (cur_dt_entry->data.u.oppo.id1!=libboot_qcdt_get_oppo_id1()) {
                goto incompatible_entry;
            }
        }

        if (!libboot_platform_strcmp(cur_dt_entry->parser, "qcom_lge")) {
            if (cur_dt_entry->data.u.lge.lge_rev!=libboot_qcdt_get_lge_rev()) {
                goto incompatible_entry;
            }
        }

        if (!libboot_platform_strcmp(cur_dt_entry->parser, "qcom_motorola") && cur_dt_entry->data.u.motorola.version) {
            if (cur_dt_entry->data.u.motorola.model[0]) {
                if (libboot_platform_strcmp(cur_dt_entry->data.u.motorola.model, libboot_qcdt_get_motorola_model()))
                    goto incompatible_entry;
            }
        }

        LOGV("Add DTB entry %u/%08x/0x%08x/%x/%x/%x/%x/%x/%p/%x\n",
             dt_node_tmp->dt_entry_m->data.platform_id, dt_node_tmp->dt_entry_m->data.variant_id,
             dt_node_tmp->dt_entry_m->data.board_hw_subtype, dt_node_tmp->dt_entry_m->data.soc_rev,
             dt_node_tmp->dt_entry_m->data.pmic_rev[0], dt_node_tmp->dt_entry_m->data.pmic_rev[1],
             dt_node_tmp->dt_entry_m->data.pmic_rev[2], dt_node_tmp->dt_entry_m->data.pmic_rev[3],
             dt_node_tmp->dt_entry_m->dtb_data, dt_node_tmp->dt_entry_m->dtb_size);

        dt_entry_list_insert(dt_list, dt_node_tmp);
        return 1;
    }

incompatible_entry:
    return 0;
}

static int devtree_delete_incompatible_entries(dt_entry_node_t *dt_list, boot_uint32_t dtb_info)
{
    dt_entry_node_t *dt_node_tmp1 = NULL;
    dt_entry_node_t *dt_node_tmp2 = NULL;
    boot_uint32_t current_info = 0;
    boot_uint32_t board_info = 0;
    boot_uint32_t best_info = 0;
    boot_uint32_t current_pmic_model[4] = {0, 0, 0, 0};
    boot_uint32_t board_pmic_model[4] = {0, 0, 0, 0};
    boot_uint32_t best_pmic_model[4] = {0, 0, 0, 0};
    boot_uint32_t delete_current_dt = 0;
    boot_uint32_t i;

    /* start to select the exact entry
    * default to exact match 0, if find current DTB entry info is the same as board info,
    * then exact match board info.
    */
    libboot_list_for_every_entry(&dt_list->node, dt_node_tmp1, dt_entry_node_t, node) {
        if (!dt_node_tmp1) {
            LOGV("Current node is the end\n");
            break;
        }
        switch (dtb_info) {
            case DTB_FOUNDRY:
                current_info = ((dt_node_tmp1->dt_entry_m->data.platform_id) & 0x00ff0000);
                board_info = libboot_qcdt_foundry_id() << 16;
                break;
            case DTB_PMIC_MODEL:
                for (i = 0; i < 4; i++) {
                    current_pmic_model[i] = (dt_node_tmp1->dt_entry_m->data.pmic_rev[i] & 0xff);
                    board_pmic_model[i] = (libboot_qcdt_pmic_target(i) & 0xff);
                }
                break;
            case DTB_PANEL_TYPE:
                current_info = ((dt_node_tmp1->dt_entry_m->data.board_hw_subtype) & 0x1800);
                board_info = (libboot_qcdt_get_hlos_subtype() & 0x1800);
                break;
            case DTB_BOOT_DEVICE:
                current_info = ((dt_node_tmp1->dt_entry_m->data.board_hw_subtype) & 0xf0000);
                board_info = (libboot_qcdt_get_hlos_subtype() & 0xf0000);
                break;
            default:
                LOGE("ERROR: Unsupported version (%d) in dt node check \n",
                     dtb_info);
                return 0;
        }

        if (dtb_info == DTB_PMIC_MODEL) {
            if ((current_pmic_model[0] == board_pmic_model[0]) &&
                    (current_pmic_model[1] == board_pmic_model[1]) &&
                    (current_pmic_model[2] == board_pmic_model[2]) &&
                    (current_pmic_model[3] == board_pmic_model[3])) {

                for (i = 0; i < 4; i++) {
                    best_pmic_model[i] = current_pmic_model[i];
                }
                break;
            }
        } else {
            if (current_info == board_info) {
                best_info = current_info;
                break;
            }
        }
    }

    libboot_list_for_every_entry(&dt_list->node, dt_node_tmp1, dt_entry_node_t, node) {
        if (!dt_node_tmp1) {
            LOGV("Current node is the end\n");
            break;
        }
        switch (dtb_info) {
            case DTB_FOUNDRY:
                current_info = ((dt_node_tmp1->dt_entry_m->data.platform_id) & 0x00ff0000);
                break;
            case DTB_PMIC_MODEL:
                for (i = 0; i < 4; i++) {
                    current_pmic_model[i] = (dt_node_tmp1->dt_entry_m->data.pmic_rev[i] & 0xff);
                }
                break;
            case DTB_PANEL_TYPE:
                current_info = ((dt_node_tmp1->dt_entry_m->data.board_hw_subtype) & 0x1800);
                break;
            case DTB_BOOT_DEVICE:
                current_info = ((dt_node_tmp1->dt_entry_m->data.board_hw_subtype) & 0xf0000);
                break;
            default:
                LOGE("ERROR: Unsupported version (%d) in dt node check \n",
                     dtb_info);
                return 0;
        }

        if (dtb_info == DTB_PMIC_MODEL) {
            if ((current_pmic_model[0] != best_pmic_model[0]) ||
                    (current_pmic_model[1] != best_pmic_model[1]) ||
                    (current_pmic_model[2] != best_pmic_model[2]) ||
                    (current_pmic_model[3] != best_pmic_model[3])) {

                delete_current_dt = 1;
            }
        } else {
            if (current_info != best_info) {
                delete_current_dt = 1;
            }
        }

        if (delete_current_dt) {
            LOGV("Delete don't fit DTB entry %u/%08x/0x%08x/%x/%x/%x/%x/%x/%p/%x\n",
                 dt_node_tmp1->dt_entry_m->data.platform_id, dt_node_tmp1->dt_entry_m->data.variant_id,
                 dt_node_tmp1->dt_entry_m->data.board_hw_subtype, dt_node_tmp1->dt_entry_m->data.soc_rev,
                 dt_node_tmp1->dt_entry_m->data.pmic_rev[0], dt_node_tmp1->dt_entry_m->data.pmic_rev[1],
                 dt_node_tmp1->dt_entry_m->data.pmic_rev[2], dt_node_tmp1->dt_entry_m->data.pmic_rev[3],
                 dt_node_tmp1->dt_entry_m->dtb_data, dt_node_tmp1->dt_entry_m->dtb_size);

            dt_node_tmp2 = (dt_entry_node_t *) dt_node_tmp1->node.prev;
            dt_entry_list_delete(dt_node_tmp1);
            dt_node_tmp1 = dt_node_tmp2;
            delete_current_dt = 0;
        }
    }

    return 1;
}

static int devtree_delete_incompatible_entries2(dt_entry_node_t *dt_list, boot_uint32_t dtb_info)
{
    dt_entry_node_t *dt_node_tmp1 = NULL;
    dt_entry_node_t *dt_node_tmp2 = NULL;
    boot_uint32_t current_info = 0;
    boot_uint32_t board_info = 0;
    boot_uint32_t best_info = 0;

    /* start to select the best entry*/
    libboot_list_for_every_entry(&dt_list->node, dt_node_tmp1, dt_entry_node_t, node) {
        if (!dt_node_tmp1) {
            LOGV("Current node is the end\n");
            break;
        }
        switch (dtb_info) {
            case DTB_SOC:
                current_info = dt_node_tmp1->dt_entry_m->data.soc_rev;
                board_info = libboot_qcdt_soc_version();
                break;
            case DTB_MAJOR_MINOR:
                current_info = ((dt_node_tmp1->dt_entry_m->data.variant_id) & 0x00ffff00);
                board_info = (libboot_qcdt_target_id() & 0x00ffff00);
                break;
            case DTB_PMIC0:
                current_info = ((dt_node_tmp1->dt_entry_m->data.pmic_rev[0]) & 0x00ffff00);
                board_info = (libboot_qcdt_pmic_target(0) & 0x00ffff00);
                break;
            case DTB_PMIC1:
                current_info = ((dt_node_tmp1->dt_entry_m->data.pmic_rev[1]) & 0x00ffff00);
                board_info = (libboot_qcdt_pmic_target(1) & 0x00ffff00);
                break;
            case DTB_PMIC2:
                current_info = ((dt_node_tmp1->dt_entry_m->data.pmic_rev[2]) & 0x00ffff00);
                board_info = (libboot_qcdt_pmic_target(2) & 0x00ffff00);
                break;
            case DTB_PMIC3:
                current_info = ((dt_node_tmp1->dt_entry_m->data.pmic_rev[3]) & 0x00ffff00);
                board_info = (libboot_qcdt_pmic_target(3) & 0x00ffff00);
                break;
            default:
                LOGE("ERROR: Unsupported version (%d) in dt node check \n",
                     dtb_info);
                return 0;
        }

        if (current_info == board_info) {
            best_info = current_info;
            break;
        }
        if ((current_info < board_info) && (current_info > best_info)) {
            best_info = current_info;
        }
        if (current_info < best_info) {
            LOGV("Delete don't fit DTB entry %u/%08x/0x%08x/%x/%x/%x/%x/%x/%p/%x\n",
                 dt_node_tmp1->dt_entry_m->data.platform_id, dt_node_tmp1->dt_entry_m->data.variant_id,
                 dt_node_tmp1->dt_entry_m->data.board_hw_subtype, dt_node_tmp1->dt_entry_m->data.soc_rev,
                 dt_node_tmp1->dt_entry_m->data.pmic_rev[0], dt_node_tmp1->dt_entry_m->data.pmic_rev[1],
                 dt_node_tmp1->dt_entry_m->data.pmic_rev[2], dt_node_tmp1->dt_entry_m->data.pmic_rev[3],
                 dt_node_tmp1->dt_entry_m->dtb_data, dt_node_tmp1->dt_entry_m->dtb_size);

            dt_node_tmp2 = (dt_entry_node_t *) dt_node_tmp1->node.prev;
            dt_entry_list_delete(dt_node_tmp1);
            dt_node_tmp1 = dt_node_tmp2;
        }
    }

    libboot_list_for_every_entry(&dt_list->node, dt_node_tmp1, dt_entry_node_t, node) {
        if (!dt_node_tmp1) {
            LOGV("Current node is the end\n");
            break;
        }
        switch (dtb_info) {
            case DTB_SOC:
                current_info = dt_node_tmp1->dt_entry_m->data.soc_rev;
                break;
            case DTB_MAJOR_MINOR:
                current_info = ((dt_node_tmp1->dt_entry_m->data.variant_id) & 0x00ffff00);
                break;
            case DTB_PMIC0:
                current_info = ((dt_node_tmp1->dt_entry_m->data.pmic_rev[0]) & 0x00ffff00);
                break;
            case DTB_PMIC1:
                current_info = ((dt_node_tmp1->dt_entry_m->data.pmic_rev[1]) & 0x00ffff00);
                break;
            case DTB_PMIC2:
                current_info = ((dt_node_tmp1->dt_entry_m->data.pmic_rev[2]) & 0x00ffff00);
                break;
            case DTB_PMIC3:
                current_info = ((dt_node_tmp1->dt_entry_m->data.pmic_rev[3]) & 0x00ffff00);
                break;
            default:
                LOGE("ERROR: Unsupported version (%d) in dt node check \n",
                     dtb_info);
                return 0;
        }

        if (current_info != best_info) {
            LOGV("Delete don't fit DTB entry %u/%08x/0x%08x/%x/%x/%x/%x/%x/%p/%x\n",
                 dt_node_tmp1->dt_entry_m->data.platform_id, dt_node_tmp1->dt_entry_m->data.variant_id,
                 dt_node_tmp1->dt_entry_m->data.board_hw_subtype, dt_node_tmp1->dt_entry_m->data.soc_rev,
                 dt_node_tmp1->dt_entry_m->data.pmic_rev[0], dt_node_tmp1->dt_entry_m->data.pmic_rev[1],
                 dt_node_tmp1->dt_entry_m->data.pmic_rev[2], dt_node_tmp1->dt_entry_m->data.pmic_rev[3],
                 dt_node_tmp1->dt_entry_m->dtb_data, dt_node_tmp1->dt_entry_m->dtb_size);

            dt_node_tmp2 = (dt_entry_node_t *) dt_node_tmp1->node.prev;
            dt_entry_list_delete(dt_node_tmp1);
            dt_node_tmp1 = dt_node_tmp2;
        }
    }
    return 1;
}

static dt_entry_local_t *devtree_get_best_entry(dt_entry_node_t *dt_list)
{
    dt_entry_node_t *dt_node_tmp1 = NULL;

    /* check Foundry id
    * the foundry id must exact match board founddry id, this is compatibility check,
    * if couldn't find the exact match from DTB, will exact match 0x0.
    */
    if (!devtree_delete_incompatible_entries(dt_list, DTB_FOUNDRY))
        return NULL;

    /* check PMIC model
    * the PMIC model must exact match board PMIC model, this is compatibility check,
    * if couldn't find the exact match from DTB, will exact match 0x0.
    */
    if (!devtree_delete_incompatible_entries(dt_list, DTB_PMIC_MODEL))
        return NULL;

    /* check panel type
    * the panel  type must exact match board panel type, this is compatibility check,
    * if couldn't find the exact match from DTB, will exact match 0x0.
    */
    if (!devtree_delete_incompatible_entries(dt_list, DTB_PANEL_TYPE))
        return NULL;

    /* check boot device subtype
    * the boot device subtype must exact match board boot device subtype, this is compatibility check,
    * if couldn't find the exact match from DTB, will exact match 0x0.
    */
    if (!devtree_delete_incompatible_entries(dt_list, DTB_BOOT_DEVICE))
        return NULL;

    /* check soc version
    * the suitable soc version must less than or equal to board soc version
    */
    if (!devtree_delete_incompatible_entries2(dt_list, DTB_SOC))
        return NULL;

    /*check major and minor version
    * the suitable major&minor version must less than or equal to board major&minor version
    */
    if (!devtree_delete_incompatible_entries2(dt_list, DTB_MAJOR_MINOR))
        return NULL;

    /*check pmic info
    * the suitable pmic major&minor info must less than or equal to board pmic major&minor version
    */
    if (!devtree_delete_incompatible_entries2(dt_list, DTB_PMIC0))
        return NULL;
    if (!devtree_delete_incompatible_entries2(dt_list, DTB_PMIC1))
        return NULL;
    if (!devtree_delete_incompatible_entries2(dt_list, DTB_PMIC2))
        return NULL;
    if (!devtree_delete_incompatible_entries2(dt_list, DTB_PMIC3))
        return NULL;

    libboot_list_for_every_entry(&dt_list->node, dt_node_tmp1, dt_entry_node_t, node) {
        if (!dt_node_tmp1) {
            LOGE("ERROR: Couldn't find the suitable DTB!\n");
            return NULL;
        }
        if (dt_node_tmp1->dt_entry_m)
            return dt_node_tmp1->dt_entry_m;
    }

    return NULL;
}

/* Function to obtain the index information for the correct device tree
 *  based on the platform data.
 *  If a matching device tree is found, the information is returned in the
 *  "dt_entry_info" out parameter and a function value of 0 is returned, otherwise
 *  a non-zero function value is returned.
 */
int libboot_qcdt_get_entry_info(dt_table_t *table, dt_entry_local_t *dt_entry_info)
{
    boot_uint32_t i;
    unsigned char *table_ptr = NULL;
    dt_entry_local_t dt_entry_buf_1;
    dt_entry_local_t *cur_dt_entry = NULL;
    dt_entry_local_t *best_match_dt_entry = NULL;
    dt_entry_v1_t *dt_entry_v1 = NULL;
    dt_entry_v2_t *dt_entry_v2 = NULL;
    dt_entry_t *dt_entry_v3 = NULL;
    dt_entry_node_t *dt_entry_queue = NULL;
    boot_uint32_t found = 0;
    boot_uint32_t qcdt_version;
    boot_uint32_t motorola_version;
    boot_uint32_t entry_size;
    const char *parser = "qcom";

    if (!dt_entry_info) {
        LOGE("ERROR: Bad parameter passed to %s \n", __func__);
        return -1;
    }

    table_ptr = (unsigned char *)table + DEV_TREE_HEADER_SIZE;
    cur_dt_entry = &dt_entry_buf_1;
    best_match_dt_entry = NULL;
    dt_entry_queue = dt_entry_list_create();

    if (!dt_entry_queue) {
        return -1;
    }

    qcdt_version = table->version & 0xff;
    motorola_version = table->version >> 8;
    switch (qcdt_version) {
        case DEV_TREE_VERSION_V1:
            entry_size =  sizeof(dt_entry_v1_t);
            break;
        case DEV_TREE_VERSION_V2:
            entry_size = sizeof(dt_entry_v2_t);
            break;
        case DEV_TREE_VERSION_V3:
            entry_size = sizeof(dt_entry_t);
            break;
        default:
            libboot_format_error(LIBBOOT_ERROR_GROUP_QCDT, LIBBOOT_ERROR_QCDT_UNSUPPORTED_VERSION, qcdt_version);
            dt_entry_list_free(dt_entry_queue);
            return -1;
    }

    if (motorola_version) {
        parser = "qcom_motorola";
        entry_size += sizeof(cur_dt_entry->data.u.motorola.model);
    }

    LOGI("DTB Total entry: %d, DTB version: %d\n", table->num_entries, qcdt_version);
    for (i = 0; found == 0 && i < table->num_entries; i++) {
        libboot_platform_memset(cur_dt_entry, 0, sizeof(dt_entry_local_t));
        switch (qcdt_version) {
            case DEV_TREE_VERSION_V1:
                dt_entry_v1 = (dt_entry_v1_t *)table_ptr;
                cur_dt_entry->data.version = qcdt_version;
                cur_dt_entry->data.u.motorola.version = motorola_version;
                cur_dt_entry->data.platform_id = dt_entry_v1->platform_id;
                cur_dt_entry->data.variant_id = dt_entry_v1->variant_id;
                cur_dt_entry->data.soc_rev = dt_entry_v1->soc_rev;
                cur_dt_entry->data.board_hw_subtype = (dt_entry_v1->variant_id >> 0x18);
                cur_dt_entry->data.pmic_rev[0] = libboot_qcdt_pmic_target(0);
                cur_dt_entry->data.pmic_rev[1] = libboot_qcdt_pmic_target(1);
                cur_dt_entry->data.pmic_rev[2] = libboot_qcdt_pmic_target(2);
                cur_dt_entry->data.pmic_rev[3] = libboot_qcdt_pmic_target(3);
                cur_dt_entry->dtb_data = ((boot_uint8_t *)table) + dt_entry_v1->offset;
                cur_dt_entry->dtb_size = dt_entry_v1->size;
                cur_dt_entry->parser = parser;

                if (motorola_version) {
                    memcpy(cur_dt_entry->data.u.motorola.model, table_ptr+sizeof(*dt_entry_v1), sizeof(cur_dt_entry->data.u.motorola.model));
                }
                break;
            case DEV_TREE_VERSION_V2:
                dt_entry_v2 = (dt_entry_v2_t *)table_ptr;
                cur_dt_entry->data.version = qcdt_version;
                cur_dt_entry->data.u.motorola.version = motorola_version;
                cur_dt_entry->data.platform_id = dt_entry_v2->platform_id;
                cur_dt_entry->data.variant_id = dt_entry_v2->variant_id;
                cur_dt_entry->data.soc_rev = dt_entry_v2->soc_rev;
                /* For V2 version of DTBs we have platform version field as part
                 * of variant ID, in such case the subtype will be mentioned as 0x0
                 * As the qcom, board-id = <0xSSPMPmPH, 0x0>
                 * SS -- Subtype
                 * PM -- Platform major version
                 * Pm -- Platform minor version
                 * PH -- Platform hardware CDP/MTP
                 * In such case to make it compatible with LK algorithm move the subtype
                 * from variant_id to subtype field
                 */
                if (dt_entry_v2->board_hw_subtype == 0)
                    cur_dt_entry->data.board_hw_subtype = (cur_dt_entry->data.variant_id >> 0x18);
                else
                    cur_dt_entry->data.board_hw_subtype = dt_entry_v2->board_hw_subtype;
                cur_dt_entry->data.pmic_rev[0] = libboot_qcdt_pmic_target(0);
                cur_dt_entry->data.pmic_rev[1] = libboot_qcdt_pmic_target(1);
                cur_dt_entry->data.pmic_rev[2] = libboot_qcdt_pmic_target(2);
                cur_dt_entry->data.pmic_rev[3] = libboot_qcdt_pmic_target(3);
                cur_dt_entry->dtb_data = ((boot_uint8_t *)table) + dt_entry_v2->offset;;
                cur_dt_entry->dtb_size = dt_entry_v2->size;
                cur_dt_entry->parser = parser;

                if (motorola_version) {
                    memcpy(cur_dt_entry->data.u.motorola.model, table_ptr+sizeof(*dt_entry_v2), sizeof(cur_dt_entry->data.u.motorola.model));
                }
                break;
            case DEV_TREE_VERSION_V3:
                dt_entry_v3 = (dt_entry_t *)table_ptr;
                cur_dt_entry->data.version = qcdt_version;
                cur_dt_entry->data.u.motorola.version = motorola_version;
                cur_dt_entry->data.platform_id = dt_entry_v3->platform_id;
                cur_dt_entry->data.variant_id = dt_entry_v3->variant_id;
                cur_dt_entry->data.board_hw_subtype = dt_entry_v3->board_hw_subtype;
                cur_dt_entry->data.soc_rev = dt_entry_v3->soc_rev;

                cur_dt_entry->data.pmic_rev[0] = dt_entry_v3->pmic_rev[0];
                cur_dt_entry->data.pmic_rev[1] = dt_entry_v3->pmic_rev[1];
                cur_dt_entry->data.pmic_rev[2] = dt_entry_v3->pmic_rev[2];
                cur_dt_entry->data.pmic_rev[3] = dt_entry_v3->pmic_rev[3];

                cur_dt_entry->dtb_data = ((boot_uint8_t *)table) + dt_entry_v3->offset;
                cur_dt_entry->dtb_size = dt_entry_v3->size;
                cur_dt_entry->parser = parser;

                /* For V3 version of DTBs we have platform version field as part
                 * of variant ID, in such case the subtype will be mentioned as 0x0
                 * As the qcom, board-id = <0xSSPMPmPH, 0x0>
                 * SS -- Subtype
                 * PM -- Platform major version
                 * Pm -- Platform minor version
                 * PH -- Platform hardware CDP/MTP
                 * In such case to make it compatible with LK algorithm move the subtype
                 * from variant_id to subtype field
                 */
                if (cur_dt_entry->data.board_hw_subtype == 0)
                    cur_dt_entry->data.board_hw_subtype = (cur_dt_entry->data.variant_id >> 0x18);

                if (motorola_version) {
                    memcpy(cur_dt_entry->data.u.motorola.model, table_ptr+sizeof(*dt_entry_v3), sizeof(cur_dt_entry->data.u.motorola.model));
                }
                break;
            default:
                libboot_format_error(LIBBOOT_ERROR_GROUP_QCDT, LIBBOOT_ERROR_QCDT_UNSUPPORTED_VERSION, qcdt_version);
                dt_entry_list_free(dt_entry_queue);
                return -1;
        }

        /* DTBs must match the platform_id, platform_hw_id, platform_subtype and DDR size.
        * The satisfactory DTBs are stored in dt_entry_queue
        */
        devtree_entry_add_if_excact_match(cur_dt_entry, dt_entry_queue);

        table_ptr += entry_size;
    }
    best_match_dt_entry = devtree_get_best_entry(dt_entry_queue);
    if (best_match_dt_entry) {
        *dt_entry_info = *best_match_dt_entry;
        found = 1;
    }

    if (found != 0) {
        LOGI("Using DTB entry 0x%08x/%08x/0x%08x/%u for device 0x%08x/%08x/0x%08x/%u\n",
             dt_entry_info->data.platform_id, dt_entry_info->data.soc_rev,
             dt_entry_info->data.variant_id, dt_entry_info->data.board_hw_subtype,
             libboot_qcdt_platform_id(), libboot_qcdt_soc_version(),
             libboot_qcdt_target_id(), libboot_qcdt_hardware_subtype());
        if (dt_entry_info->data.pmic_rev[0] == 0 && dt_entry_info->data.pmic_rev[0] == 0 &&
                dt_entry_info->data.pmic_rev[0] == 0 && dt_entry_info->data.pmic_rev[0] == 0) {
            LOGV("No maintain pmic info in DTB, device pmic info is 0x%0x/0x%x/0x%x/0x%0x\n",
                 libboot_qcdt_pmic_target(0), libboot_qcdt_pmic_target(1),
                 libboot_qcdt_pmic_target(2), libboot_qcdt_pmic_target(3));
        } else {
            LOGI("Using pmic info 0x%0x/0x%x/0x%x/0x%0x for device 0x%0x/0x%x/0x%x/0x%0x\n",
                 dt_entry_info->data.pmic_rev[0], dt_entry_info->data.pmic_rev[1],
                 dt_entry_info->data.pmic_rev[2], dt_entry_info->data.pmic_rev[3],
                 libboot_qcdt_pmic_target(0), libboot_qcdt_pmic_target(1),
                 libboot_qcdt_pmic_target(2), libboot_qcdt_pmic_target(3));
        }
        dt_entry_list_free(dt_entry_queue);
        return 0;
    }

    libboot_format_error(LIBBOOT_ERROR_GROUP_QCDT, LIBBOOT_ERROR_QCDT_NO_MATCH2,
                         libboot_qcdt_platform_id(), libboot_qcdt_soc_version(),
                         libboot_qcdt_target_id(), libboot_qcdt_hardware_subtype());

    dt_entry_list_free(dt_entry_queue);
    return -1;
}
