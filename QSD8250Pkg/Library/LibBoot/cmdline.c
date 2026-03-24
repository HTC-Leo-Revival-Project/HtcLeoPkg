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

typedef struct cmdline_item {
    libboot_list_node_t node;
    char *name;
    char *value;
} cmdline_item_t;

static cmdline_item_t *cmdline_get_internal(libboot_list_node_t *list, const char *name)
{
    cmdline_item_t *item;
    libboot_list_for_every_entry(list, item, cmdline_item_t, node) {
        if (!libboot_platform_strcmp(name, item->name))
            return item;
    }

    return NULL;
}

int libboot_cmdline_has(libboot_list_node_t *list, const char *name)
{
    return !!cmdline_get_internal(list, name);
}

const char *libboot_cmdline_get(libboot_list_node_t *list, const char *name)
{
    cmdline_item_t *item = cmdline_get_internal(list, name);

    if (!item)
        return NULL;

    return item->value;
}

void libboot_cmdline_add(libboot_list_node_t *list, const char *name, const char *value, int overwrite)
{
    cmdline_item_t *item = cmdline_get_internal(list, name);
    if (item) {
        if (!overwrite) return;

        libboot_list_delete(&item->node);
        libboot_free(item->name);
        libboot_free(item->value);
        libboot_free(item);
    }

    item = libboot_alloc(sizeof(cmdline_item_t));
    item->name = libboot_platform_strdup(name);
    item->value = value?libboot_platform_strdup(value):NULL;

    libboot_list_add_tail(list, &item->node);
}

void libboot_cmdline_remove(libboot_list_node_t *list, const char *name)
{
    cmdline_item_t *item = cmdline_get_internal(list, name);
    if (item) {
        libboot_list_delete(&item->node);
        libboot_free(item->name);
        libboot_free(item->value);
        libboot_free(item);
    }
}

boot_uintn_t libboot_cmdline_length(libboot_list_node_t *list)
{
    boot_uintn_t len = 0;

    cmdline_item_t *item;
    libboot_list_for_every_entry(list, item, cmdline_item_t, node) {
        // leading space
        if (len!=0) len++;
        // name
        len+=libboot_platform_strlen(item->name);
        // '=' and value
        if (item->value)
            len+= 1 + libboot_platform_strlen(item->value);
    }

    // 0 terminator
    if (len>0) len++;

    return len;
}

boot_uintn_t libboot_cmdline_generate(libboot_list_node_t *list, char *buf, boot_uintn_t bufsize)
{
    boot_uintn_t len = 0;

    if (bufsize>0)
        buf[0] = 0;

    cmdline_item_t *item;
    libboot_list_for_every_entry(list, item, cmdline_item_t, node) {
        if (len!=0) buf[len++] = ' ';
        len+=libboot_internal_strlcpy(buf+len, item->name, bufsize-len);

        if (item->value) {
            buf[len++] = '=';
            len+=libboot_internal_strlcpy(buf+len, item->value, bufsize-len);
        }
    }

    return len;
}

static int str2nameval(const char *str, char **name, char **value)
{
    char *c;
    int index;
    char *ret_name;
    char *ret_value;

    // get index of delimiter
    c = libboot_platform_strchr(str, '=');
    if (c==NULL) {
        *name = libboot_platform_strdup(str);
        *value = NULL;
        return -1;
    }
    index = (int)(c - str);

    // get name
    ret_name = libboot_alloc(index+1);
    libboot_platform_memmove(ret_name, str, index);
    ret_name[index] = 0;

    // get value
    ret_value = libboot_platform_strdup(str+index+1);

    *name = ret_name;
    *value = ret_value;

    return 0;
}

void libboot_cmdline_addall(libboot_list_node_t *list, const char *_cmdline, int overwrite)
{
    const char *sep = " ";
    char *saveptr = NULL;

    char *cmdline = libboot_platform_strdup(_cmdline);
    if (!cmdline) return;

    char *pch = libboot_platform_strtok_r(cmdline, sep, &saveptr);
    while (pch != NULL) {
        // parse
        char *name = NULL;
        char *value = NULL;
        str2nameval(pch, &name, &value);

        // add
        libboot_cmdline_add(list, name, value, overwrite);

        // free
        libboot_free(name);
        libboot_free(value);

        // next
        pch = libboot_platform_strtok_r(NULL, sep, &saveptr);
    }

    libboot_free(cmdline);
}

void libboot_cmdline_init(libboot_list_node_t *list)
{
    libboot_list_initialize(list);
}

void libboot_cmdline_free(libboot_list_node_t *list)
{
    while (!libboot_list_is_empty(list)) {
        cmdline_item_t *item = libboot_list_remove_tail_type(list, cmdline_item_t, node);

        libboot_free(item->name);
        libboot_free(item->value);
        libboot_free(item);
    }
}
