/* drivers/input/touchscreen/nt11206/NVTtouch_206.h
 *
 * Copyright (C) 2010 - 2016 Novatek, Inc.
 *
 * $Revision: 3459 $
 * $Date: 2016-02-18 17:38:19 +0800 (週四, 18 二月 2016) $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */
#ifndef 	_LINUX_NVT_TOUCH_H
#define		_LINUX_NVT_TOUCH_H

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/proc_fs.h>
#include <linux/wakelock.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#elif defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>

#endif
#include <huawei_platform/touchscreen/hw_tp_common.h>

#ifdef CONFIG_HUAWEI_DSM
enum FW_uptate_state {
	FWU_GET_SYSINFO_FAIL = 0,
	FWU_GENERATE_FW_NAME_FAIL,
	FWU_REQUEST_FW_FAIL,
	FWU_FW_CONT_BUILTIN_ERROR,
	FWU_FW_CONT_ERROR,
	FWU_REQUEST_EXCLUSIVE_FAIL,
	TS_UPDATE_STATE_UNDEFINE = 255,
};

struct tp_dsm_info {
	int irq_gpio;
	int rst_gpio;
	int constraints_I2C_status;
	int constraints_UPDATE_status;
	int constraints_ESD_status;
};

#endif

//---GPIO number---
#define NVTTOUCH_RST_PIN 980
#define NVTTOUCH_INT_PIN 943


//---INT trigger mode---
//#define IRQ_TYPE_EDGE_RISING 1
//#define IRQ_TYPE_EDGE_FALLING 2
//#define IRQ_TYPE_LEVEL_HIGH 4
//#define IRQ_TYPE_LEVEL_LOW 8
#define INT_TRIGGER_TYPE IRQ_TYPE_EDGE_RISING
//#define INT_TRIGGER_TYPE IRQ_TYPE_EDGE_FALLING


//---I2C driver info.---
#define NVT_I2C_NAME "NVT-ts"
#define I2C_FW_Address 0x01
#define I2C_HW_Address 0x62

#define NVT_FIRMWARE_NAME_FROM_SDCARD  "/sdcard/novatek_ts_fw.bin"

//---Input device info.---
#define NVT_TS_NAME "NVTCapacitiveTouchScreen"


//---Touch info.---
#define TOUCH_MAX_WIDTH 1200
#define TOUCH_MAX_HEIGHT 1920
#define TOUCH_MAX_FINGER_NUM 10
#define TOUCH_KEY_NUM 0
#if TOUCH_KEY_NUM > 0
extern const uint16_t touch_key_array[TOUCH_KEY_NUM];
#endif

//---Customerized func.---
#define NVT_TOUCH_PROC 1
#define NVT_TOUCH_EXT_PROC 1
#define NVT_TOUCH_MP 1
#define MT_PROTOCOL_B 1
#define WAKEUP_GESTURE 0
#define TOUCH_INFO_SIZE		128
#define RAW_DATA_SIZE (PAGE_SIZE * 32)

#if WAKEUP_GESTURE
extern const uint16_t gesture_key_array[];
#endif

#define BOOT_UPDATE_FIRMWARE 1
#define BOOT_UPDATE_FIRMWARE_NAME "jordan_novatek_ts_fw.bin"

#define DRAGONBOARD_REGULATOR 1
#define DRAGONBOARD_TP_SUSPEND 1

struct nvt_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct nvt_work;
	struct delayed_work nvt_fwu_work;
	struct wake_lock ts_flash_wake_lock;
	struct kobject *nvt_kobject;
	uint16_t addr;
	int8_t phys[32];
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#elif defined(CONFIG_FB)
	struct notifier_block fb_notifier;
#endif
	uint16_t abs_x_max;
	uint16_t abs_y_max;
	uint8_t x_num;
	uint8_t y_num;
	uint8_t max_touch_num;
	uint8_t max_button_num;
	uint32_t int_trigger_type;
#if DRAGONBOARD_REGULATOR
	struct regulator *avdd;
	struct regulator *vdd;
	bool power_on;
#endif
	int32_t vdd_type;
	int32_t avdd_type;
	int32_t vdd_gpio;
	int32_t avdd_gpio;
	int32_t irq_gpio;
	uint32_t irq_flags;
	int32_t reset_gpio;
	uint32_t reset_flags;
	uint8_t sleep_mode;
	uint8_t update_firmware_status;
};

#if NVT_TOUCH_PROC
struct nvt_flash_data{
	rwlock_t lock;
	struct i2c_client *client;
};
#endif

typedef enum {
	RESET_STATE_INIT = 0xA0,// IC reset
	RESET_STATE_REK,		// ReK baseline
	RESET_STATE_REK_FINISH,	// baseline is ready
	RESET_STATE_NORMAL_RUN	// normal run
} RST_COMPLETE_STATE;


int nvt_fw_update_from_sdcard(u8 *file_path, bool update_flag);

int nvt_get_chip_info(char *buf);

void nvt_sysfs_init(void);

void nvt_sysfs_remove(void);

int nvt_check_update_firmware_flag(void);

void nvt_bootloader_reset(void);

#ifdef CONFIG_HUAWEI_DSM
int nvt_tp_report_dsm_err(struct device *dev, int type, int err_numb);
#endif

#endif /* _LINUX_NVT_TOUCH_H */
