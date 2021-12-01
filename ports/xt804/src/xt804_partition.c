/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <string.h>

#include "py/runtime.h"
#include "py/mperrno.h"
#include "extmod/vfs.h"
//#include "modesp32.h"
//#include "esp_ota_ops.h"
#include "modxt804.h"
#include "hal_common.h"

#include <wm_internal_flash.h>

#define BLOCK_SIZE_BYTES (INSIDE_FLS_SECTOR_SIZE)
#define INTERNAL_FLASH_ADDR (INSIDE_FLS_BASE_ADDR)
#define INTERNAL_FLASH_SIZE (INSIDE_FLS_END_ADDR - INSIDE_FLS_BASE_ADDR)

/*
    0x8(x)FFFFF ------- partition data end ((x)M internal flash)
    |
    | partition data area (1534KB)
    |
    0x8080400 ------- partition data 
    | partition header area (1KB)
    0x8080000 ------- partition header
    0x807FFFF ------- running image end
    |
    | running image data area (446KB)
    |
    0x8010400 ------- running image data (len: ~0x4F6FC, 317KB)
    | running image header area (1KB)
    0x8010000 ------- running image header
    0x800FFFF ------- secboot data end
    |
    | secboot data area (54KB)
    |
    0x8002400 ------- secboot data (len: ~0x7B5C, 30KB)
    | secboot header area (1KB)
    0x8002000 ------- secboot header
*/


#define XT804_PARTITION_HEADER_ADDR (0x08080000)
#define XT804_PARTITION_DATA_ADDR   (0x08080400)
#define XT804_PARTITION_DATA_SIZE   (INTERNAL_FLASH_ADDR + INTERNAL_FLASH_SIZE - XT804_PARTITION_DATA_ADDR)

#define XT804_PARTITION_TYPE_APP (0xA)
#define XT804_PARTITION_TYPE_DATA (0xD)

#define XT804_IMAGE_MAGIC_NUMER (0xA0FFFF9F)
#define XT804_IMAGE_ATTR_TYPE_DATA (XT804_PARTITION_TYPE_DATA)

typedef struct {
    uint32_t magic_no;
    struct {
        uint32_t img_type:4;
        uint32_t code_encrypt: 1;
        uint32_t pricey_sel: 3;
        uint32_t signature: 1;
        uint32_t zip_type: 1;
        uint32_t reserved: 1;
        uint32_t erase_block_en: 1;
        uint32_t erase_always: 1;
    } img_attr;
    uint32_t img_addr;
    uint32_t img_len;
    uint32_t img_header_addr;
    uint32_t upgrade_img_addr;
    uint32_t org_checksum;
    uint32_t upd_no;
    uint32_t ver;
    uint32_t next;
    uint32_t hd_checksum;
    uint8_t  label[8];
    uint32_t reserved;
} xt804_partition_header_t;

typedef struct {
    uint32_t address;
    uint32_t size;
    uint8_t  label[8];
} xt804_partition_t;

static xt804_partition_t DEFAULT_DATA_PARTITION = {
    XT804_PARTITION_DATA_ADDR,
    XT804_PARTITION_DATA_SIZE,
    { 0, 0, 0, 0, 0, 0, 0, 0 },
};

typedef struct _xt804_partition_obj_t {
    mp_obj_base_t base;
    const xt804_partition_t *part;
} xt804_partition_obj_t;

STATIC xt804_partition_obj_t * xt804_partition_new(const xt804_partition_t *part) {
    if (part == NULL) {
        mp_raise_OSError(MP_ENOENT);
    }
    xt804_partition_obj_t *self = m_new_obj(xt804_partition_obj_t);
    self->base.type = &xt804_partition_type;
    self->part = part;
    return self;
}

STATIC void xt804_partition_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    xt804_partition_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "<Partition address=0x%X, size=%u, label=%s>",
        self->part->address, self->part->size, self->part->label
    );
}

STATIC mp_obj_t xt804_partition_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // Check args
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    // Get requested partition
    xt804_partition_t * part = &DEFAULT_DATA_PARTITION;

    // create partition
    if (part != NULL) {
        TDEBUG("CREATE DATA PARTITION: addr:0x%X, size:%d", part->address, part->size);
        xt804_partition_header_t image_info;
        image_info.magic_no = XT804_IMAGE_MAGIC_NUMER;
        image_info.img_attr.img_type = XT804_PARTITION_TYPE_DATA;
        image_info.img_attr.code_encrypt = 0;
        image_info.img_attr.pricey_sel = 0;
        image_info.img_attr.signature = 0;
        image_info.img_attr.zip_type = 0;
        image_info.img_attr.erase_block_en = 1;
        image_info.img_attr.erase_always = 0;
        image_info.img_addr = part->address;
        image_info.img_len = part->size;
        image_info.img_header_addr = XT804_PARTITION_HEADER_ADDR;
        image_info.upgrade_img_addr = 0;
        image_info.org_checksum = 0;
        image_info.upd_no = 0;
        image_info.ver = 0;
        image_info.next = 0;
        memset(image_info.label, 0, sizeof(image_info.label));
        image_info.reserved = 0;

        const char *label = mp_obj_str_get_str(all_args[0]);
        if (label != NULL) {
            strncpy((char*)(image_info.label), label, sizeof(image_info.label));
        }
        HAL_FLASH_Write(image_info.img_header_addr, (uint8_t*)(&image_info), sizeof(image_info));
    }

    // Return new object
    return MP_OBJ_FROM_PTR(xt804_partition_new(part));
}

STATIC mp_obj_t xt804_partition_find(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // Parse args
    // enum { ARG_type, ARG_label };
    // static const mp_arg_t allowed_args[] = {
    //     { MP_QSTR_type, MP_ARG_INT, {.u_int = XT804_PARTITION_TYPE_APP} },
    //     { MP_QSTR_label, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE} },
    // };
    // mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    // mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get optional label string
    // const char * label = NULL;
    // if (args[ARG_label].u_obj != mp_const_none) {
    //     label = mp_obj_str_get_str(args[ARG_label].u_obj);
    // }

    //TDEBUG("xt804_partition_find: label: %s", label ? label : "");
    //TDEBUG("xt804_partition_find: type: %d sub_type: %d", args[ARG_type].u_int, args[ARG_subtype].u_int);

    // Build list of matching partitions
    // mp_obj_t list = mp_obj_new_list(0, NULL);
    // machine_partition_iterator_t iter = esp_partition_find(args[ARG_type].u_int, args[ARG_subtype].u_int, label);
    // while (iter != NULL) {
    //     mp_obj_list_append(list, MP_OBJ_FROM_PTR(xt804_partition_new(machine_partition_get(iter))));
    //     iter = machine_partition_next(iter);
    // }
    // machine_partition_iterator_release(iter);   

    // return list;

    mp_obj_t list = mp_obj_new_list(0, NULL);
    mp_obj_list_append(list, MP_OBJ_FROM_PTR(xt804_partition_new(&DEFAULT_DATA_PARTITION)));
    return list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(xt804_partition_find_fun_obj, 0, xt804_partition_find);
STATIC MP_DEFINE_CONST_STATICMETHOD_OBJ(xt804_partition_find_obj, MP_ROM_PTR(&xt804_partition_find_fun_obj));

STATIC mp_obj_t xt804_partition_info(mp_obj_t self_in) {
    xt804_partition_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_obj_t tuple[] = {
        MP_OBJ_NEW_SMALL_INT(XT804_PARTITION_TYPE_DATA),
        MP_OBJ_NEW_SMALL_INT(0),
        mp_obj_new_int_from_uint(self->part->address),
        mp_obj_new_int_from_uint(self->part->size),
        mp_obj_new_str((const char *)(self->part->label), strlen((const char *)(self->part->label))),
        mp_obj_new_bool(false),
    };
    return mp_obj_new_tuple(2, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(xt804_partition_info_obj, xt804_partition_info);

STATIC mp_obj_t xt804_partition_readblocks(size_t n_args, const mp_obj_t *args) {
    //TDEBUG("xt804_partition_readblocks");
    xt804_partition_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    uint32_t offset = mp_obj_get_int(args[1]) * BLOCK_SIZE_BYTES;
    //TDEBUG("xt804_partition_readblocks: offset:%u", offset);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_WRITE);
    //TDEBUG("xt804_partition_readblocks: mp_get_buffer_raise: bufinfo:0x%X, len: %d", bufinfo.buf, bufinfo.len);
    if (n_args > 3) {
        offset += mp_obj_get_int(args[3]);
    }
    //TDEBUG("xt804_partition_readblocks: HAL_FLASH_Read:(offset:%u, len:%d)", offset, bufinfo.len);
    HAL_FLASH_Read(self->part->address + offset, bufinfo.buf, bufinfo.len);
    //TDEBUG("xt804_partition_readblocks: done");
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(xt804_partition_readblocks_obj, 3, 4, xt804_partition_readblocks);

STATIC mp_obj_t xt804_partition_writeblocks(size_t n_args, const mp_obj_t *args) {
    //TDEBUG("xt804_partition_writeblocks");
    xt804_partition_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    uint32_t offset = mp_obj_get_int(args[1]) * BLOCK_SIZE_BYTES;
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_READ);
    if (n_args > 3) {
        offset += mp_obj_get_int(args[3]);
    }
    HAL_FLASH_Write(self->part->address + offset, bufinfo.buf, bufinfo.len);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(xt804_partition_writeblocks_obj, 3, 4, xt804_partition_writeblocks);

STATIC mp_obj_t xt804_partition_ioctl(mp_obj_t self_in, mp_obj_t cmd_in, mp_obj_t arg_in) {
    //TDEBUG("xt804_partition_ioctl");
    xt804_partition_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t cmd = mp_obj_get_int(cmd_in);
    switch (cmd) {
        case MP_BLOCKDEV_IOCTL_INIT:
            //TDEBUG("MP_BLOCKDEV_IOCTL_INIT");
            return MP_OBJ_NEW_SMALL_INT(0);
        case MP_BLOCKDEV_IOCTL_DEINIT:
            //TDEBUG("MP_BLOCKDEV_IOCTL_DEINIT");
            return MP_OBJ_NEW_SMALL_INT(0);
        case MP_BLOCKDEV_IOCTL_SYNC:
            //TDEBUG("MP_BLOCKDEV_IOCTL_SYNC");
            return MP_OBJ_NEW_SMALL_INT(0);
        case MP_BLOCKDEV_IOCTL_BLOCK_COUNT:
            //TDEBUG("MP_BLOCKDEV_IOCTL_BLOCK_COUNT: result: %d", self->part->size / BLOCK_SIZE_BYTES);
            return MP_OBJ_NEW_SMALL_INT(self->part->size / BLOCK_SIZE_BYTES);
        case MP_BLOCKDEV_IOCTL_BLOCK_SIZE:
            //TDEBUG("MP_BLOCKDEV_IOCTL_BLOCK_SIZE: result: %d", BLOCK_SIZE_BYTES);
            return MP_OBJ_NEW_SMALL_INT(BLOCK_SIZE_BYTES);
        case MP_BLOCKDEV_IOCTL_BLOCK_ERASE: {
            //TDEBUG("MP_BLOCKDEV_IOCTL_BLOCK_ERASE");
            uint32_t sector = mp_obj_int_get_uint_checked(arg_in);
            if (sector < self->part->size / BLOCK_SIZE_BYTES) {
                HAL_FLASH_erase(sector);
            } else {
                mp_raise_ValueError(MP_ERROR_TEXT("sector number out of range."));
            }
            return MP_OBJ_NEW_SMALL_INT(0);
        }
        default:
            return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(xt804_partition_ioctl_obj, xt804_partition_ioctl);

#if 0
STATIC mp_obj_t xt804_partition_set_boot(mp_obj_t self_in) {
    //xt804_partition_obj_t *self = MP_OBJ_TO_PTR(self_in);
    //check_esp_err(esp_ota_set_boot_partition(self->part));
    TERROR("not implement yet. <- xt804_partition_set_boot");
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(xt804_partition_set_boot_obj, xt804_partition_set_boot);

STATIC mp_obj_t xt804_partition_get_next_update(mp_obj_t self_in) {
    //xt804_partition_obj_t *self = MP_OBJ_TO_PTR(self_in);
    //return MP_OBJ_FROM_PTR(xt804_partition_new(esp_ota_get_next_update_partition(self->part)));
    TERROR("not implement yet. <- xt804_partition_get_next_update");
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(xt804_partition_get_next_update_obj,xt804_partition_get_next_update);

STATIC mp_obj_t xt804_partition_mark_app_valid_cancel_rollback(mp_obj_t cls_in) {
    //check_esp_err(esp_ota_mark_app_valid_cancel_rollback());
    TERROR("not implement yet. <- esp_ota_mark_app_valid_cancel_rollback");
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(xt804_partition_mark_app_valid_cancel_rollback_fun_obj,
    xt804_partition_mark_app_valid_cancel_rollback);
STATIC MP_DEFINE_CONST_CLASSMETHOD_OBJ(xt804_partition_mark_app_valid_cancel_rollback_obj,
    MP_ROM_PTR(&xt804_partition_mark_app_valid_cancel_rollback_fun_obj));
#endif

STATIC const mp_rom_map_elem_t xt804_partition_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_find), MP_ROM_PTR(&xt804_partition_find_obj) },

    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&xt804_partition_info_obj) },
    { MP_ROM_QSTR(MP_QSTR_readblocks), MP_ROM_PTR(&xt804_partition_readblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeblocks), MP_ROM_PTR(&xt804_partition_writeblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_ioctl), MP_ROM_PTR(&xt804_partition_ioctl_obj) },

#if 0
    { MP_ROM_QSTR(MP_QSTR_set_boot), MP_ROM_PTR(&xt804_partition_set_boot_obj) },
    { MP_ROM_QSTR(MP_QSTR_mark_app_valid_cancel_rollback), MP_ROM_PTR(&xt804_partition_mark_app_valid_cancel_rollback_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_next_update), MP_ROM_PTR(&xt804_partition_get_next_update_obj) }
#endif
    { MP_ROM_QSTR(MP_QSTR_TYPE_APP), MP_ROM_INT(XT804_PARTITION_TYPE_APP) },
    { MP_ROM_QSTR(MP_QSTR_TYPE_DATA), MP_ROM_INT(XT804_PARTITION_TYPE_DATA) },
};
STATIC MP_DEFINE_CONST_DICT(xt804_partition_locals_dict, xt804_partition_locals_dict_table);

const mp_obj_type_t xt804_partition_type = {
    { &mp_type_type },
    .name = MP_QSTR_Partition,
    .print = xt804_partition_print,
    .make_new = xt804_partition_make_new,
    .locals_dict = (mp_obj_dict_t *)&xt804_partition_locals_dict,
};
