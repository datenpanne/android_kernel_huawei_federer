/* drivers/input/touchscreen/nt11206/NVTtouch_206.c
 *
 * Copyright (C) 2010 - 2016 Novatek, Inc.
 *
 * $Revision: 3448 $
 * $Date: 2016-02-18 11:28:34 +0800 (週四, 18 二月 2016) $
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/proc_fs.h>
#include <linux/unistd.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#include <linux/input/mt.h>
#include <linux/wakelock.h>

#include <linux/of_gpio.h>
#include <linux/of_irq.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <linux/hw_dev_dec.h>
#endif


#include "NVTtouch_206.h"
#ifdef CONFIG_HUAWEI_DSM
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <dsm/dsm_pub.h>
#define DSMINFO_LEN_MAX        64
struct dsm_dev dsm_nvt_tp = {
	.name = "dsm_i2c_bus",	// dsm client name
	.fops = NULL,
	.buff_size = TP_RADAR_BUF_MAX,
};
struct tp_dsm_info nvt_tp_dsm_info;
struct dsm_client *nvt_tp_dclient = NULL;
#endif

#if NVT_TOUCH_EXT_PROC
extern int32_t nvt_extra_proc_init(void);
#endif

#if NVT_TOUCH_MP
extern int32_t nvt_mp_proc_init(void);
#endif

static void nvt_setup_fb_notifier(struct nvt_ts_data *cd);


struct nvt_ts_data *ts;
extern atomic_t nvt_mmi_test_status;

static struct workqueue_struct *nvt_wq;
extern int get_boot_into_recovery_flag(void);

#if BOOT_UPDATE_FIRMWARE
static struct workqueue_struct *nvt_fwu_wq;
extern void Boot_Update_Firmware(struct work_struct *work);
#else
extern void Boot_Update_Firmware(struct work_struct *work);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void nvt_ts_early_suspend(struct early_suspend *h);
static void nvt_ts_late_resume(struct early_suspend *h);
#endif

#if DRAGONBOARD_REGULATOR
#include <linux/regulator/consumer.h>
#endif

#if TOUCH_KEY_NUM > 0
const uint16_t touch_key_array[TOUCH_KEY_NUM] = {
    KEY_BACK,
    KEY_HOME,
    KEY_MENU
};
#endif

#if WAKEUP_GESTURE
const uint16_t gesture_key_array[] = {
    KEY_POWER,  //GESTURE_WORD_C
    KEY_POWER,  //GESTURE_WORD_W
    KEY_POWER,  //GESTURE_WORD_V
    KEY_POWER,  //GESTURE_DOUBLE_CLICK
    KEY_POWER,  //GESTURE_WORD_Z
    KEY_POWER,  //GESTURE_WORD_M
    KEY_POWER,  //GESTURE_WORD_O
    KEY_POWER,  //GESTURE_WORD_e
    KEY_POWER,  //GESTURE_WORD_S
    KEY_POWER,  //GESTURE_SLIDE_UP
    KEY_POWER,  //GESTURE_SLIDE_DOWN
    KEY_POWER,  //GESTURE_SLIDE_LEFT
    KEY_POWER,  //GESTURE_SLIDE_RIGHT
};
#endif

#define NVT_POWER_GPIO 1
#define NVT_POWER_LDO  2

/*******************************************************
Description:
	Novatek touchscreen i2c read function.

return:
	Executive outcomes. 2---succeed. -5---I/O error
*******************************************************/
int32_t CTP_I2C_READ(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len)
{
	struct i2c_msg msgs[2];
	int32_t ret = -1;
	int32_t retries = 0;
	int ii = 0;

	msgs[0].flags = !I2C_M_RD;
	msgs[0].addr  = address;
	msgs[0].len   = 1;
	msgs[0].buf   = &buf[0];

	while (retries < 5) {
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret == 1)	break;
		retries++;
	}

	if (unlikely(retries == 5)) {
#ifdef CONFIG_HUAWEI_DSM
		nvt_tp_report_dsm_err(&ts->client->dev, DSM_TP_I2C_RW_ERROR_NO , 0);
#endif
		tp_log_err("%s: error, ret=%d\n", __func__, ret);
		tp_log_debug("%s: write address:0x%x,data:\n", __func__, address);
		for(ii = 0; ii < len; ii++)
		{
			tp_log_debug("0x%x ,", buf[ii]);
		}
		tp_log_debug("\n	write end \n");

		return -EIO;
	}
	retries = 0;
	msgs[0].flags = I2C_M_RD;
	msgs[0].addr  = address;
	msgs[0].len   = len - 1;
	msgs[0].buf   = &buf[1];

	while (retries < 5) {
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret == 1)	break;
		retries++;
	}

	if (unlikely(retries == 5)) {
#ifdef CONFIG_HUAWEI_DSM
		nvt_tp_report_dsm_err(&ts->client->dev, DSM_TP_I2C_RW_ERROR_NO , 0);
#endif
		tp_log_err("%s: error, ret=%d\n", __func__, ret);
		tp_log_debug("%s: read address:0x%x,data:\n", __func__, address);
		for(ii = 0; ii < len; ii++)
		{
			tp_log_debug("0x%x ,", buf[ii]);
		}
		tp_log_debug("\n	read end \n");

		ret = -EIO;
	}

	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen i2c dummy read function.

return:
	Executive outcomes. 1---succeed. -5---I/O error
*******************************************************/
int32_t CTP_I2C_READ_DUMMY(struct i2c_client *client, uint16_t address)
{
	struct i2c_msg msg;
	int ii = 0;
	uint8_t buf[8] = {0};
	int32_t ret = -1;
	int32_t retries = 0;

	msg.flags = I2C_M_RD;
	msg.addr  = address;
	msg.len   = 1;
	msg.buf   = buf;

	while (retries < 5) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret == 1)	break;
		retries++;
	}

	if (unlikely(retries == 5)) {
#ifdef CONFIG_HUAWEI_DSM
		nvt_tp_report_dsm_err(&ts->client->dev, DSM_TP_I2C_RW_ERROR_NO , -1);
#endif
		tp_log_err("%s: error, ret=%d\n", __func__, ret);
		tp_log_debug("%s: read address:0x%x,data:\n", __func__, address);
		for(ii = 0; ii < 1; ii++)
		{
			tp_log_debug("0x%x ,", buf[ii]);
		}
		tp_log_debug("\n	read end \n");

		ret = -EIO;
	}

	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen i2c write function.

return:
	Executive outcomes. 1---succeed. -5---I/O error
*******************************************************/
int32_t CTP_I2C_WRITE(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len)
{
	struct i2c_msg msg;
	int ii = 0;
	int32_t ret = -1;
	int32_t retries = 0;

	msg.flags = !I2C_M_RD;
	msg.addr  = address;
	msg.len   = len;
	msg.buf   = buf;

	while (retries < 5) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if (ret == 1)	break;
		retries++;
	}

	if (unlikely(retries == 5)) {
#ifdef CONFIG_HUAWEI_DSM
		nvt_tp_report_dsm_err(&ts->client->dev, DSM_TP_I2C_RW_ERROR_NO , -2);
#endif
		tp_log_err("%s: error, ret=%d\n", __func__, ret);
		tp_log_debug("%s: write address:0x%x,data:\n", __func__, address);
		for(ii = 0; ii < len; ii++)
		{
			tp_log_debug("0x%x ,", buf[ii]);
		}
		tp_log_debug("\n  write end \n");
		ret = -EIO;
	}

	return ret;
}


/*******************************************************
Description:
	Novatek touchscreen IC hardware reset function.

return:
	n.a.
*******************************************************/
void nvt_hw_reset(void)
{
	//---trigger rst-pin to reset (pull low for 50ms)---
	tp_log_info("%s, enter\n", __func__);
	gpio_set_value(ts->reset_gpio, 1);
	mdelay(10);
	gpio_set_value(ts->reset_gpio, 0);
	mdelay(5);
	gpio_set_value(ts->reset_gpio, 1);
	mdelay(5);
}

/*******************************************************
Description:
	Novatek touchscreen set i2c debounce function.

return:
	n.a.
*******************************************************/

int32_t  nvt_set_i2c_debounce(void)
{
	int32_t i2cRet = 0;
	uint8_t buf[8] = {0};
	uint8_t reg1_val = 0;
	uint8_t reg2_val = 0;
	uint32_t retry = 0;

	do {
		msleep(10);

		// set xdata index to 0x1F000
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0xF0;
		i2cRet = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
		if(i2cRet < 0) {
			tp_log_err("%s,%d, i2c err\n", __func__, __LINE__);
			return i2cRet;
		}

		// set i2c debounce 34ns
		buf[0] = 0x15;
		buf[1] = 0x17;
		i2cRet = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);
		if(i2cRet < 0) {
			tp_log_err("%s,%d, i2c err\n", __func__, __LINE__);
			return i2cRet;
		}

		buf[0] = 0x15;
		buf[1] = 0x00;
		i2cRet = CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);
		if(i2cRet < 0) {
			tp_log_err("%s,%d, i2c err\n", __func__, __LINE__);
			return i2cRet;
		}
		reg1_val = buf[1];

		// set schmitt trigger enable
		buf[0] = 0x3E;
		buf[1] = 0x07;
		i2cRet = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);
		if(i2cRet < 0) {
			tp_log_err("%s,%d, i2c err\n", __func__, __LINE__);
			return i2cRet;
		}

		buf[0] = 0x3E;
		buf[1] = 0x00;
		i2cRet = CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);
		if(i2cRet < 0) {
			tp_log_err("%s,%d, i2c err\n", __func__, __LINE__);
			return i2cRet;
		}
		reg2_val = buf[1];
	} while (((reg1_val != 0x17) || (reg2_val != 0x07)) && (retry++ < 20));

	if(retry == 20) {
		tp_log_err("%s: set i2c debounce failed, reg1_val=0x%02X, reg2_val=0x%02X\n", __func__, reg1_val, reg2_val);
	} else {
		tp_log_info("%s: set i2c debounce success, reg1_val=0x%02X, reg2_val=0x%02X, retry = %d\n", __func__, reg1_val, reg2_val, retry);
	}
	return i2cRet;
}


/*******************************************************
Description:
	Novatek touchscreen reset MCU then into idle mode
    function.

return:
	n.a.
*******************************************************/
int32_t nvt_sw_reset_idle(void)
{
	int32_t i2cRet = 0;
	uint8_t buf[4]={0};
	int i,retry = 5;
	tp_log_info("%s, enter\n", __func__);

	//---write i2c cmds to reset idle---
	buf[0]=0x00;
	buf[1]=0xA5;
	i2cRet = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
	if(i2cRet < 0) {
		tp_log_err("%s,%d, i2c err\n", __func__, __LINE__);
		return i2cRet;
	}
	msleep(10);

	for(i = 0; i < retry; i++) {
		i2cRet = nvt_set_i2c_debounce();
		if(i2cRet < 0) {
			tp_log_err("%s,%d, i2c err\n", __func__, __LINE__);
			//return i2cRet;
			//goto fail;
			nvt_hw_reset();
			nvt_bootloader_reset();
			msleep(100);
			buf[0]=0x00;
			buf[1]=0xA5;
			i2cRet = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
			if(i2cRet < 0) {
				tp_log_err("%s,%d, i2c err\n", __func__, __LINE__);
				//return i2cRet;
			}
			msleep(10);
			tp_log_info("%s, retry :%d", __func__, i);
			//continue;
		} else {
			break;
		}
	}
	if(i == retry)
		return -1;
	return i2cRet;
}

/*******************************************************
Description:
	Novatek touchscreen reset MCU (boot) function.

return:
	n.a.
*******************************************************/
void nvt_bootloader_reset(void)
{
	uint8_t buf[8] = {0};
	tp_log_info("%s, enter\n", __func__);

	//---write i2c cmds to reset---
	buf[0] = 0x00;
	buf[1] = 0x69;
	CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);

	// need 5ms delay after bootloader reset
	msleep(5);
}

/*******************************************************
Description:
	Novatek touchscreen clear FW status function.

return:
	Executive outcomes. 0---succeed. -1---fail.
*******************************************************/
int32_t nvt_clear_fw_status(void)
{
	uint8_t buf[8] = {0};
	int32_t i = 0;
	const int32_t retry = 10;

	//---dummy read to resume TP before writing command---
	CTP_I2C_READ_DUMMY(ts->client, I2C_FW_Address);

	for (i = 0; i < retry; i++) {
		//---set xdata index to 0x14700---
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0x47;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);

		//---clear fw status---
		buf[0] = 0x51;
		buf[1] = 0x00;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);

		//---read fw status---
		buf[0] = 0x51;
		buf[1] = 0xFF;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);

		if (buf[1] == 0x00)
			break;

		msleep(10);
	}

	if (i >= retry)
		return -1;
	else
		return 0;
}

/*******************************************************
Description:
	Novatek touchscreen check FW status function.

return:
	Executive outcomes. 0---succeed. -1---failed.
*******************************************************/
int32_t nvt_check_fw_status(void)
{
	uint8_t buf[8] = {0};
	int32_t i = 0;
	const int32_t retry = 10;

	//---dummy read to resume TP before writing command---
	CTP_I2C_READ_DUMMY(ts->client, I2C_FW_Address);

	for (i = 0; i < retry; i++) {
		//---set xdata index to 0x14700---
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0x47;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);

		//---read fw status---
		buf[0] = 0x51;
		buf[1] = 0x00;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);

		if ((buf[1] & 0xF0) == 0xA0)
			break;

		msleep(10);
	}

	if (i >= retry)
		return -1;
	else
		return 0;
}

/*******************************************************
Description:
	Novatek touchscreen check FW reset state function.

return:
	Executive outcomes. 0---succeed. -1---failed.
*******************************************************/
int32_t nvt_check_fw_reset_state(RST_COMPLETE_STATE check_reset_state)
{
	uint8_t buf[8] = {0};
	int32_t ret = 0;
	int32_t retry = 0;

	while (1) {
		mdelay(10);

		//---read reset state---
		buf[0] = 0x60;
		buf[1] = 0x00;
		CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 2);

		if ((buf[1] >= check_reset_state) && (buf[1] < 0xFF)) {
			ret = 0;
			break;
		}

		retry++;
		if(unlikely(retry > 100)) {
			ret = -1;
			tp_log_err("%s: error, retry=%d, buf[1]=0x%02X\n", __func__, retry, buf[1]);
			break;
		}
	}

	return ret;
}

/*******************************************************
  Create Device Node (Proc Entry)
*******************************************************/
#if NVT_TOUCH_PROC
static struct proc_dir_entry *NVT_proc_entry;
#define DEVICE_NAME	"NVTflash"

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTflash read function.

return:
	Executive outcomes. 2---succeed. -5,-14---failed.
*******************************************************/
static ssize_t nvt_flash_read(struct file *file, char __user *buff, size_t count, loff_t *offp)
{
	struct i2c_msg msgs[2];
	uint8_t str[64] = {0};
	int32_t ret = -1;
	int32_t retries = 0, retries1 = 0;
	int8_t i2c_wr = 0;

	if (count > sizeof(str))
		return -EFAULT;

	if (copy_from_user(str, buff, count))
		return -EFAULT;

	i2c_wr = str[0] >> 7;

	if (i2c_wr == 0) {	//I2C write
		msgs[0].flags = !I2C_M_RD;
		msgs[0].addr  = str[0] & 0x7F;
		msgs[0].len   = str[1];
		msgs[0].buf   = &str[2];

		while (retries < 20) {
			ret = i2c_transfer(ts->client->adapter, msgs, 1);
			if (ret == 1)
				break;
			else
				tp_log_err("%s: error, retries=%d, ret=%d\n", __func__, retries, ret);

			retries++;
		}

		if (unlikely(retries == 20)) {
			tp_log_err("%s: error, ret = %d\n", __func__, ret);
			return -EIO;
		}

		return ret;
	} else if (i2c_wr == 1) {	//I2C read
		msgs[0].flags = !I2C_M_RD;
		msgs[0].addr  = str[0] & 0x7F;
		msgs[0].len   = 1;
		msgs[0].buf   = &str[2];

		while (retries < 20) {
			ret = i2c_transfer(ts->client->adapter, msgs, 1);
			if (ret == 1)
				break;
			else
				tp_log_err("%s: error, retries=%d, ret=%d\n", __func__, retries, ret);

			retries++;
		}

		msgs[0].flags = I2C_M_RD;
		msgs[0].addr  = str[0] & 0x7F;
		msgs[0].len   = str[1]-1;
		msgs[0].buf   = &str[3];

		while (retries1 < 20) {
			ret = i2c_transfer(ts->client->adapter, msgs, 1);
			if (ret == 1)
				break;
			else
				tp_log_err("%s: error, retries1=%d, ret=%d\n", __func__, retries1, ret);

			retries1++;
		}

		// copy buff to user if i2c transfer
		if (retries < 20 && retries1 < 20) {
			if (copy_to_user(buff, str, count))
				return -EFAULT;
		}

		if (unlikely(retries == 20 || retries1 == 20)) {
			tp_log_err("%s: error, ret = %d\n", __func__, ret);
			return -EIO;
		}

		return ret;
	} else {
		tp_log_err("%s: Call error, str[0]=%d\n", __func__, str[0]);
		return -EFAULT;
	}
}

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTflash open function.

return:
	Executive outcomes. 0---succeed. -12---failed.
*******************************************************/
static int32_t nvt_flash_open(struct inode *inode, struct file *file)
{
	struct nvt_flash_data *dev;

	dev = kmalloc(sizeof(struct nvt_flash_data), GFP_KERNEL);
	if (dev == NULL) {
		tp_log_err("%s: Failed to allocate memory for nvt flash data\n", __func__);
		return -ENOMEM;
	}

	rwlock_init(&dev->lock);
	file->private_data = dev;

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTflash close function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_flash_close(struct inode *inode, struct file *file)
{
	struct nvt_flash_data *dev = file->private_data;

	if (dev)
		kfree(dev);

	return 0;
}

static const struct file_operations nvt_flash_fops = {
	.owner = THIS_MODULE,
	.open = nvt_flash_open,
	.release = nvt_flash_close,
	.read = nvt_flash_read,
};

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTflash initial function.

return:
	Executive outcomes. 0---succeed. -12---failed.
*******************************************************/
static int32_t nvt_flash_proc_init(void)
{
	NVT_proc_entry = proc_create(DEVICE_NAME, 0444, NULL,&nvt_flash_fops);
	if (NVT_proc_entry == NULL) {
		tp_log_err("%s: Failed!\n", __func__);
		return -ENOMEM;
	} else {
		tp_log_info("%s: Succeeded!\n", __func__);
	}

	tp_log_info("============================================================\n");
	tp_log_info("Create /proc/NVTflash\n");
	tp_log_info("============================================================\n");

	return 0;
}
#endif

#if WAKEUP_GESTURE
#define GESTURE_WORD_C			12
#define GESTURE_WORD_W			13
#define GESTURE_WORD_V			14
#define GESTURE_DOUBLE_CLICK	15
#define GESTURE_WORD_Z			16
#define GESTURE_WORD_M			17
#define GESTURE_WORD_O			18
#define GESTURE_WORD_e			19
#define GESTURE_WORD_S			20
#define GESTURE_SLIDE_UP		21
#define GESTURE_SLIDE_DOWN		22
#define GESTURE_SLIDE_LEFT		23
#define GESTURE_SLIDE_RIGHT		24

static struct wake_lock gestrue_wakelock;
static uint8_t bTouchIsAwake = 1;
static uint8_t bWakeupByGesture = 0;

/*******************************************************
Description:
	Novatek touchscreen wake up gesture key report function.

return:
	n.a.
*******************************************************/
void nvt_ts_wakeup_gesture_report(uint8_t gesture_id)
{
	struct i2c_client *client = ts->client;
	uint32_t keycode = 0;

	tp_log_info("gesture_id = %d\n", gesture_id);

	switch (gesture_id) {
		case GESTURE_WORD_C:
			tp_log_info( "Gesture : Word-C.\n");
			keycode = gesture_key_array[0];
			break;
		case GESTURE_WORD_W:
			tp_log_info("Gesture : Word-W.\n");
			keycode = gesture_key_array[1];
			break;
		case GESTURE_WORD_V:
			tp_log_info("Gesture : Word-V.\n");
			keycode = gesture_key_array[2];
			break;
		case GESTURE_DOUBLE_CLICK:
			tp_log_info("Gesture : Double Click.\n");
			keycode = gesture_key_array[3];
			break;
		case GESTURE_WORD_Z:
			tp_log_info("Gesture : Word-Z.\n");
			keycode = gesture_key_array[4];
			break;
		case GESTURE_WORD_M:
			tp_log_info("Gesture : Word-M.\n");
			keycode = gesture_key_array[5];
			break;
		case GESTURE_WORD_O:
			tp_log_info("Gesture : Word-O.\n");
			keycode = gesture_key_array[6];
			break;
		case GESTURE_WORD_e:
			tp_log_info("Gesture : Word-e.\n");
			keycode = gesture_key_array[7];
			break;
		case GESTURE_WORD_S:
			tp_log_info("Gesture : Word-S.\n");
			keycode = gesture_key_array[8];
			break;
		case GESTURE_SLIDE_UP:
			tp_log_info("Gesture : Slide UP.\n");
			keycode = gesture_key_array[9];
			break;
		case GESTURE_SLIDE_DOWN:
			tp_log_info("Gesture : Slide DOWN.\n");
			keycode = gesture_key_array[10];
			break;
		case GESTURE_SLIDE_LEFT:
			tp_log_info("Gesture : Slide LEFT.\n");
			keycode = gesture_key_array[11];
			break;
		case GESTURE_SLIDE_RIGHT:
			tp_log_info("Gesture : Slide RIGHT.\n");
			keycode = gesture_key_array[12];
			break;
		default:
			break;
	}

	if (keycode > 0) {
		input_report_key(ts->input_dev, keycode, 1);
		input_sync(ts->input_dev);
		input_report_key(ts->input_dev, keycode, 0);
		input_sync(ts->input_dev);

		bWakeupByGesture = 1;
	}

	msleep(250);
}
#endif

#if DRAGONBOARD_REGULATOR
/*******************************************************
Description:
	Novatek touchscreen power on function.

return:
	Executive outcomes. 0---succeed. not 0---failed.
*******************************************************/
static int32_t nvt_power_on(struct nvt_ts_data *ts)
{
	int32_t ret = 0;

	if (ts->power_on) {
		tp_log_info("Device already power on\n");
		return 0;
	}

	if(NVT_POWER_GPIO == ts->vdd_type) {
		ret = gpio_direction_output(ts->vdd_gpio, 1);
		if(ret){
			tp_log_err("%s %d: Fail set vdd_gpio.\n", __func__,__LINE__);
			goto err_enable_vdd;  
		}
	} else if(NVT_POWER_LDO == ts->vdd_type) {
		if (!IS_ERR(ts->vdd)) {
			ret = regulator_enable(ts->vdd);
			if (ret) {
				tp_log_err("Regulator vdd enable failed ret=%d\n", ret);
				goto err_enable_vdd;
			}
		}
	} else {
		tp_log_err("%s,%d,ts->vdd_type is invalid\n", __func__, __LINE__);
	}

	//msleep(10);
	if(NVT_POWER_GPIO == ts->avdd_type) {
		ret = gpio_direction_output(ts->avdd_gpio, 1);
		if(ret){
			tp_log_err("%s %d: Fail set avdd_gpio.\n", __func__,__LINE__);
			goto err_enable_avdd;
		}
	} else if(NVT_POWER_LDO == ts->avdd_type) {
		if (!IS_ERR(ts->avdd)) {
			ret = regulator_enable(ts->avdd);
			if (ret) {
				tp_log_err("Regulator avdd enable failed ret=%d\n", ret);
				goto err_enable_avdd;
			}
		}
	} else {
		tp_log_err("%s,%d,ts->avdd_type is invalid\n", __func__, __LINE__);
	}

	ts->power_on = true;
	return 0;

err_enable_avdd:
	if(NVT_POWER_GPIO == ts->vdd_type) {
		gpio_direction_output(ts->vdd_gpio, 0);
	} else if(NVT_POWER_LDO == ts->vdd_type) {
		if (!IS_ERR(ts->vdd))
			regulator_disable(ts->vdd);
	}
	
err_enable_vdd:
	ts->power_on = false;
	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen power off function.

return:
	Executive outcomes. 0---succeed. not 0---failed.
*******************************************************/
static int32_t nvt_power_off(struct nvt_ts_data *ts)
{
	int32_t ret = 0;

	if (!ts->power_on) {
		tp_log_info("Device already power off\n");
		return 0;
	}

	if(NVT_POWER_GPIO == ts->avdd_type) {
		ret = gpio_direction_output(ts->avdd_gpio, 0);
		if(ret){
			tp_log_err("%s %d: Fail set avdd_gpio.\n", __func__,__LINE__);
			return -EINVAL;
		}
	} else if(NVT_POWER_LDO == ts->avdd_type) {
		if (!IS_ERR(ts->avdd)) {
			ret = regulator_disable(ts->avdd);
			if (ret)
				tp_log_err("Regulator avdd disable failed ret=%d\n", ret);
		}
	}

	if(NVT_POWER_GPIO == ts->vdd_type) {
		ret = gpio_direction_output(ts->vdd_gpio, 0);
		if(ret){
			tp_log_err("%s %d: Fail set vdd_gpio.\n", __func__,__LINE__);
			return -EINVAL;
		}
	} else if(NVT_POWER_LDO == ts->vdd_type) {
		if (!IS_ERR(ts->vdd)) {
			ret = regulator_disable(ts->vdd);
			if (ret)
				tp_log_err("Regulator vdd disable failed ret=%d\n", ret);
		}
	}
	ts->power_on = false;
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen power initial function.

return:
	Executive outcomes. 0---succeed. not 0---failed.
*******************************************************/
static int32_t nvt_power_init(struct nvt_ts_data *ts)
{
	int32_t ret = 0;
	if(NVT_POWER_GPIO == ts->avdd_type)  {// 1:gpio, 2:LDO ,other reserve
		if (-1 == ts->avdd_gpio) {
			tp_log_err("%s %d: Invalid gpio: %d.",__func__,__LINE__,ts->avdd_gpio);
			return -EINVAL;
		}

		tp_log_info("%s %d: request gpio of avdd_en: %d\n", __func__,__LINE__,ts->avdd_gpio);
		ret = gpio_request(ts->avdd_gpio, "nvt_avdd_gpio");
		if(ret){
			tp_log_err("%s: unable to reques nvt_avdd_gpio : %d.\n",__func__, ts->avdd_gpio);
			return -EINVAL;
		}
	} else if(NVT_POWER_LDO== ts->avdd_type){
		ts->avdd = regulator_get(&ts->client->dev, "avdd");
		if (IS_ERR(ts->avdd)) {
			ret = PTR_ERR(ts->avdd);
			tp_log_err("%s, %d, Regulator get failed avdd ret=%d\n", __func__, __LINE__,ret);
			return -EINVAL;
		}
	} else {
		tp_log_err("%s %d: avdd config invalid: %d\n", __func__,__LINE__, ts->avdd_type);
		return -EINVAL;
	}

	if(NVT_POWER_GPIO == ts->vdd_type)  {// 1:gpio, 2:LDO ,other reserve
		if (-1 == ts->vdd_gpio) {
			tp_log_err("%s %d: Invalid gpio: %d.",__func__,__LINE__,ts->vdd_gpio);
			goto err_vdd_free;
		}

		tp_log_info("%s %d: request gpio of vdd_en: %d\n", __func__,__LINE__,ts->vdd_gpio);
		ret = gpio_request(ts->vdd_gpio, "nvt_vdd_gpio");
		if(ret){
			tp_log_err("%s: unable to reques nvt_vdd_gpio : %d.\n",__func__, ts->vdd_gpio);
			goto err_vdd_free;
		}
	} else if(NVT_POWER_LDO == ts->vdd_type){
		ts->vdd = regulator_get(&ts->client->dev, "vdd");
		if (IS_ERR(ts->vdd)) {
			ret = PTR_ERR(ts->vdd);
			tp_log_err("%s, %d, Regulator get failed vdd ret=%d\n", __func__, __LINE__,ret);
			goto err_vdd_free;
		}
	} else {
		tp_log_err("%s %d: vdd config invalid: %d\n", __func__,__LINE__, ts->vdd_type);
		goto err_free;
	}

	return 0;

err_free:
	if(NVT_POWER_GPIO == ts->vdd_type) {
		gpio_free(ts->vdd_gpio);
	} else if(NVT_POWER_LDO == ts->vdd_type) {
		regulator_put(ts->vdd);
	}

err_vdd_free:
	if(NVT_POWER_GPIO == ts->avdd_type) {
		gpio_free(ts->avdd_gpio);
	} else if(NVT_POWER_LDO == ts->avdd_type) {
		regulator_put(ts->avdd);
	}
	return -EINVAL;
}

/*******************************************************
Description:
	Novatek touchscreen power remove function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_power_remove(struct nvt_ts_data *ts)
{
	if(NVT_POWER_GPIO == ts->avdd_type) {
		gpio_free(ts->avdd_gpio);
	} else if(NVT_POWER_LDO == ts->avdd_type) {
		regulator_put(ts->avdd);
	}

	if(NVT_POWER_GPIO == ts->vdd_type) {
		gpio_free(ts->vdd_gpio);
	} else if(NVT_POWER_LDO == ts->vdd_type) {
		regulator_put(ts->vdd);
	}

	return 0;
}
#endif


/*******************************************************
Description:
	Novatek touchscreen parse device tree function.

return:
	n.a.
*******************************************************/
#ifdef CONFIG_OF
static void nvt_parse_dt(struct device *dev)
{
	struct device_node *np = dev->of_node;
	u32 value = 0;
	int rc = 0;

	if (of_find_property(np, "novatek,irq-gpio", NULL)) {
		ts->irq_gpio = of_get_named_gpio_flags(np, "novatek,irq-gpio", 0, NULL);
		tp_log_info("%s %d:[novatek,irq-gpio] is %d.\n", __func__, __LINE__, ts->irq_gpio);
	} else {
		ts->irq_gpio = -1;
		tp_log_warning("%s %d:[novatek,irq-gpio] read fail.\n", __func__, __LINE__);
	}
	if (of_find_property(np, "novatek,rst-gpio", NULL)) {
		ts->reset_gpio = of_get_named_gpio_flags(np, "novatek,rst-gpio", 0, NULL);
		tp_log_info("%s %d:[novatek,rst-gpio] is %d.\n", __func__, __LINE__, ts->reset_gpio);
	} else {
		ts->reset_gpio = -1;
		tp_log_warning("%s %d:[novatek,rst-gpio] read fail.\n", __func__, __LINE__);
	}

	if (of_find_property(np, "novatek,vdd-gpio", NULL)) {
		ts->vdd_gpio = of_get_named_gpio_flags(np, "novatek,vdd-gpio", 0, NULL);
		tp_log_info("%s %d:[novatek,vdd-gpio] is %d.\n", __func__, __LINE__, ts->vdd_gpio);
	} else {
		ts->vdd_gpio = -1;
		tp_log_warning("%s %d:[novatek,vdd-gpio] read fail.\n", __func__, __LINE__);
	}
	if (of_find_property(np, "novatek,avdd-gpio", NULL)) {
		ts->avdd_gpio = of_get_named_gpio_flags(np, "novatek,avdd-gpio", 0, NULL);
		tp_log_info("%s %d:[novatek,avdd-gpio] is %d.\n", __func__, __LINE__, ts->avdd_gpio);
	} else {
		ts->avdd_gpio = -1;
		tp_log_warning("%s %d:[novatek,avdd-gpio] read fail.\n", __func__, __LINE__);
	}

	rc = of_property_read_u32(np, "novatek,vdd-type", &value);
	if (rc) {
		tp_log_err("%s %d: [novatek,vdd-type] read fail, rc = %d.\n", __func__, __LINE__, rc);
		goto fail_free;
	} else {
		ts->vdd_type = value;
		tp_log_info("%s %d: [novatek,vdd-type] is %d.\n", __func__, __LINE__, ts->vdd_type);
	}
	rc = of_property_read_u32(np, "novatek,avdd-type", &value);
	if (rc) {
		tp_log_err("%s %d: [novatek,avdd-type] read fail, rc = %d.\n", __func__, __LINE__, rc);
		goto fail_free;
	} else {
		ts->avdd_type = value;
		tp_log_info("%s %d: [novatek,vdd-type] is %d.\n", __func__, __LINE__, ts->avdd_type);
	}

fail_free:
	return ;
}
#else
static void nvt_parse_dt(struct device *dev)
{
	ts->reset_gpio = NVTTOUCH_RST_PIN;
	ts->irq_gpio = NVTTOUCH_INT_PIN;
}
#endif

/*******************************************************
Description:
	Novatek touchscreen work function.

return:
	n.a.
*******************************************************/
static void nvt_ts_work_func(struct work_struct *work)
{
	//struct nvt_ts_data *ts = container_of(work, struct nvt_ts_data, work);
//	struct i2c_client *client = ts->client;

	int32_t ret = -1;
	uint8_t point_data[64] = {0};
	uint32_t position = 0;
	uint32_t input_x = 0;
	uint32_t input_y = 0;
	uint32_t input_w = 0;
	uint8_t input_id = 0;
	uint8_t press_id[TOUCH_MAX_FINGER_NUM] = {0};

	int32_t i = 0;
	int32_t finger_cnt = 0;

	ret = CTP_I2C_READ(ts->client, I2C_FW_Address, point_data, 62 + 1);
	if (ret < 0) {
		tp_log_err("%s: CTP_I2C_READ failed.(%d)\n", __func__, ret);
		goto XFER_ERROR;
	}

	//--- dump I2C buf ---
	for (i = 0; i < 10; i++) {
		tp_log_debug("%02X %02X %02X %02X %02X %02X  ", point_data[1+i*6], point_data[2+i*6], point_data[3+i*6], point_data[4+i*6], point_data[5+i*6], point_data[6+i*6]);
	}

	finger_cnt = 0;
	input_id = (uint8_t)(point_data[1] >> 3);

#if WAKEUP_GESTURE
	if (bTouchIsAwake == 0) {
		nvt_ts_wakeup_gesture_report(input_id);
		enable_irq(ts->client->irq);
		return;
	}
#endif

#if MT_PROTOCOL_B
	for (i = 0; i < ts->max_touch_num; i++) {
		position = 1 + 6 * i;
		input_id = (uint8_t)(point_data[position + 0] >> 3);
		if (input_id > TOUCH_MAX_FINGER_NUM)
			continue;

		if (((point_data[position] & 0x07) == 0x01) || ((point_data[position] & 0x07) == 0x02)) {	//finger down (enter & moving)
			input_x = (uint32_t)(point_data[position + 1] << 4) + (uint32_t) (point_data[position + 3] >> 4);
			input_y = (uint32_t)(point_data[position + 2] << 4) + (uint32_t) (point_data[position + 3] & 0x0F);
			input_w = (uint32_t)(point_data[position + 4]) + 10;
			if (input_w > 255)
				input_w = 255;

			if ((input_x < 0) || (input_y < 0))
				continue;
			if ((input_x > ts->abs_x_max)||(input_y > ts->abs_y_max))
				continue;

			press_id[input_id - 1] = 1;
			input_mt_slot(ts->input_dev, input_id - 1);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);

			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, input_x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, input_y);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, input_w);
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, input_w);

			finger_cnt++;
		}
	}

	for (i = 0; i < ts->max_touch_num; i++) {
		if (press_id[i] != 1) {
			input_mt_slot(ts->input_dev, i);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
			input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
		}
	}

	input_report_key(ts->input_dev, BTN_TOUCH, (finger_cnt > 0));

#else

	for (i = 0; i < ts->max_touch_num; i++) {
		position = 1 + 6 * i;
		input_id = (uint8_t)(point_data[position + 0] >> 3);

		if ((point_data[position] & 0x07) == 0x03) {	// finger up (break)
			continue;//input_report_key(ts->input_dev, BTN_TOUCH, 0);
		} else if (((point_data[position] & 0x07) == 0x01) || ((point_data[position] & 0x07) == 0x02)) {	//finger down (enter & moving)
			input_x = (uint32_t)(point_data[position + 1] << 4) + (uint32_t) (point_data[position + 3] >> 4);
			input_y = (uint32_t)(point_data[position + 2] << 4) + (uint32_t) (point_data[position + 3] & 0x0F);
			input_w = (uint32_t)(point_data[position + 4]) + 10;
			if (input_w > 255)
				input_w = 255;

			if ((input_x < 0) || (input_y < 0))
				continue;
			if ((input_x > ts->abs_x_max)||(input_y > ts->abs_y_max))
				continue;

			press_id[input_id - 1] = 1;
			input_report_key(ts->input_dev, BTN_TOUCH, 1);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, input_x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, input_y);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, input_w);
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, input_w);
			input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, input_id - 1);

			input_mt_sync(ts->input_dev);

			finger_cnt++;
		}
	}
	if (finger_cnt == 0) {
		input_report_key(ts->input_dev, BTN_TOUCH, 0);

		input_mt_sync(ts->input_dev);
	}
#endif


#if TOUCH_KEY_NUM > 0
	if (point_data[61] == 0xF8) {
		for (i = 0; i < TOUCH_KEY_NUM; i++) {
			input_report_key(ts->input_dev, touch_key_array[i], ((point_data[62] >> i) & 0x01));
		}
	} else {
		for (i = 0; i < TOUCH_KEY_NUM; i++) {
			input_report_key(ts->input_dev, touch_key_array[i], 0);
		}
	}
#endif

	input_sync(ts->input_dev);

XFER_ERROR:
	enable_irq(ts->client->irq);
}

/*******************************************************
Description:
	External interrupt service routine.

return:
	irq execute status.
*******************************************************/
static irqreturn_t nvt_ts_irq_handler(int32_t irq, void *dev_id)
{
	//struct nvt_ts_data *ts = dev_id;
	disable_irq_nosync(ts->client->irq);

#if WAKEUP_GESTURE
	if (bTouchIsAwake == 0) {
		wake_lock_timeout(&gestrue_wakelock, msecs_to_jiffies(5000));
	}
#endif

	queue_work(nvt_wq, &ts->nvt_work);

	return IRQ_HANDLED;
}


/*******************************************************
Description:
	Novatek touchscreen print chip version function.

return:
	Executive outcomes. 2---succeed. -5---failed.
*******************************************************/
static int32_t nvt_ts_chip_version(void)
{
	uint8_t buf[8] = {0};
	int32_t ret = 0;
	uint8_t cut_number = 0;
	uint8_t cut_version = 0;

	//---dummy read to resume TP before writing command---
	CTP_I2C_READ_DUMMY(ts->client, I2C_FW_Address);

	//write i2c index to 0x1F000
	buf[0] = 0xFF;
	buf[1] = 0x01;
	buf[2] = 0xF0;
	ret = CTP_I2C_WRITE(ts->client, 0x01, buf, 3);
	if (ret < 0) {
		tp_log_err("%s: write i2c index error!!(%d)\n", __func__, ret);
		return ret;
	}

	//read chip version
	buf[0] = 0x01;
	buf[1] = 0x00;
	ret = CTP_I2C_READ(ts->client, 0x01, buf, 3);
	if (ret < 0) {
		tp_log_err("%s: read chip version error!!(%d)\n", __func__, ret);
		return ret;
	}

	// [4:0]: Cut Number
	cut_number = buf[1] & 0x1F;
	// [7:5]: Cut Version
	cut_version = (buf[1] & 0xE0) >> 5;

	tp_log_info("chip version: cut number=%d, cut version=%d\n", cut_number, cut_version);

	return ret;
}


/*******************************************************
Description:
	Novatek touchscreen read chip id function.

return:
	Executive outcomes. 0x26---succeed.
*******************************************************/
static uint8_t nvt_ts_read_chipid(void)
{
	uint8_t buf[8] = {0};
	int32_t retry = 0;
	int32_t ret = 0;

	//---dummy read to resume TP before writing command---
	ret = CTP_I2C_READ_DUMMY(ts->client, I2C_HW_Address);
	if(ret < 0) {
		tp_log_err("%s, CTP_I2C_READ_DUMMY failed\n", __func__);
		return ret;
	}

	// reset idle to keep default addr 0x01 to read chipid
	buf[0] = 0x00;
	buf[1] = 0xA5;
	ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
	if(ret < 0) {
		tp_log_err("%s, I2c write 0x00A5 failed\n", __func__);
		return ret;
	}

	msleep(10);

	//---Check NT11206 for 5 times---
	for (retry = 5; retry > 0; retry--) {
		//write i2c index to 0x1F000
		buf[0] = 0xFF;
		buf[1] = 0x01;
		buf[2] = 0xF0;
		CTP_I2C_WRITE(ts->client, 0x01, buf, 3);

		//read hw chip id
		buf[0] = 0x00;
		buf[1] = 0x00;
		CTP_I2C_READ(ts->client, 0x01, buf, 3);

		if (buf[1] == 0x26)
			break;
	}

	return buf[1];
}


/*******************************************************
Description:
	Novatek touchscreen driver probe function.

return:
	Executive outcomes. 0---succeed. negative---failed
*******************************************************/
static int32_t nvt_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int32_t ret = 0;
#if ((TOUCH_KEY_NUM > 0) || WAKEUP_GESTURE)
	int32_t retry = 0;
#endif
	int recovery_flag = 0;

	tp_log_info("%s: start.......\n", __func__);
	//---check i2c func.---
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		tp_log_err("i2c_check_functionality failed. (no I2C_FUNC_I2C)\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kmalloc(sizeof(struct nvt_ts_data), GFP_KERNEL);
	if (ts == NULL) {
		tp_log_err("%s: failed to allocated memory for nvt ts data\n", __func__);
		return -ENOMEM;
	}
	ts->client = client;
	i2c_set_clientdata(client, ts);

	//---parse dts---
	nvt_parse_dt(&client->dev);

#if DRAGONBOARD_REGULATOR
	ts->power_on = false;

	//---request regulator for DragonBoard---
	ret = nvt_power_init(ts);
	if (ret) {
		tp_log_err("nvt power init failed\n");
		goto err_power_init;
	}

	//---turn on regulator for DragonBoard---
	ret = nvt_power_on(ts);
	if (ret) {
		tp_log_err("nvt power on failed\n");
		goto err_nvt_power_on;
	}
#endif

	//msleep(50);
	//---request RST-pin & INT-pin---
	ret = gpio_request(ts->reset_gpio, "NVT-rst");
	if (ret) {
		tp_log_info("Failed to get NVT-rst GPIO\n");
		goto err_gpio_request_rst;
	}
	ret = gpio_direction_output(ts->reset_gpio, 1);
	if (ret < 0) {
		tp_log_err("%s: Fail set output gpio=%d\n",	__func__, ts->reset_gpio);
		goto err_gpio_request_irq;
	}
	ret = gpio_request(ts->irq_gpio, "NVT-int");
	if (ret) {
		tp_log_info("Failed to get NVT-int GPIO\n");
		goto err_gpio_request_irq;
	}
	gpio_direction_input(ts->irq_gpio);
	nvt_tp_dsm_info.irq_gpio = ts->irq_gpio;
	nvt_tp_dsm_info.rst_gpio = ts->reset_gpio;

	// need 10ms delay after POR(power on reset)
	msleep(80);

	//---check chip id---
	ret = nvt_ts_read_chipid();
	if (ret != 0x26) {
		tp_log_err("nvt_ts_read_chipid is not 0x26. ret=0x%02X\n", ret);
		ret = -EINVAL;
		goto err_chipid_failed;
	}
	tp_log_info("%s,%d nvt chipid=%d\n", __func__, __LINE__, ret);
#ifdef CONFIG_HUAWEI_DSM
		nvt_tp_dclient = dsm_register_client(&dsm_nvt_tp);
		if (!nvt_tp_dclient)
			tp_log_err("%s: dsm register client failed\n", __func__);
#endif/*CONFIG_HUAWEI_DSM*/

	//---check chip version---
	ret = nvt_ts_chip_version();

	//---create workqueue---
	nvt_wq = create_workqueue("nvt_wq");
	if (!nvt_wq) {
		tp_log_err("%s: nvt_wq create workqueue failed\n", __func__);
		ret = -ENOMEM;
		goto err_create_nvt_wq_failed;
	}
	INIT_WORK(&ts->nvt_work, nvt_ts_work_func);

	//---allocate input device---
	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		tp_log_err("%s: allocate input device failed\n", __func__);
		ret = -ENOMEM;
		goto err_input_dev_alloc_failed;
	}

	ts->abs_x_max = TOUCH_MAX_WIDTH;
	ts->abs_y_max = TOUCH_MAX_HEIGHT;
	ts->max_touch_num = TOUCH_MAX_FINGER_NUM;

#if TOUCH_KEY_NUM > 0
	ts->max_button_num = TOUCH_KEY_NUM;
#endif

	ts->int_trigger_type = INT_TRIGGER_TYPE;

	//---set input device info.---
	ts->input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) ;
	ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	ts->input_dev->propbit[0] = BIT(INPUT_PROP_DIRECT);

#if MT_PROTOCOL_B
	input_mt_init_slots(ts->input_dev, ts->max_touch_num, 0);
#endif

	input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE, 0, 255, 0, 0);    //pressure = 255

#if TOUCH_MAX_FINGER_NUM > 1
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);    //area = 255

	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, ts->abs_x_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, ts->abs_y_max, 0, 0);
#if MT_PROTOCOL_B
	// no need to set ABS_MT_TRACKING_ID, input_mt_init_slots() already set it
#else
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, ts->max_touch_num, 0, 0);
#endif
#endif

#if TOUCH_KEY_NUM > 0
	for (retry = 0; retry < TOUCH_KEY_NUM; retry++) {
		input_set_capability(ts->input_dev, EV_KEY, touch_key_array[retry]);
	}
#endif

#if WAKEUP_GESTURE
	for (retry = 0; retry < (sizeof(gesture_key_array) / sizeof(gesture_key_array[0])); retry++) {
		input_set_capability(ts->input_dev, EV_KEY, gesture_key_array[retry]);
	}
	wake_lock_init(&gestrue_wakelock, WAKE_LOCK_SUSPEND, "poll-wake-lock");
#endif

	sprintf(ts->phys, "input/ts");
	ts->input_dev->name = "huawei,touchscreen";
	ts->input_dev->phys = ts->phys;
	ts->input_dev->id.bustype = BUS_I2C;


	//---register input device---
	ret = input_register_device(ts->input_dev);
	if (ret) {
		tp_log_err("register input device (%s) failed. ret=%d\n", ts->input_dev->name, ret);
		goto err_input_register_device_failed;
	}


	//---set int-pin & request irq---
	client->irq = gpio_to_irq(ts->irq_gpio);
	if (client->irq) {
		tp_log_info("int_trigger_type=%d\n", ts->int_trigger_type);

#if WAKEUP_GESTURE
		ret = request_irq(client->irq, nvt_ts_irq_handler, ts->int_trigger_type | IRQF_NO_SUSPEND, client->name, ts);
#else
		ret = request_irq(client->irq, nvt_ts_irq_handler, ts->int_trigger_type, client->name, ts);
#endif
		if (ret != 0) {
			tp_log_err("request irq failed. ret=%d\n", ret);
			goto err_int_request_failed;
		} else {
			disable_irq(client->irq);
			tp_log_info("request irq %d succeed\n", client->irq);
		}
	}

	nvt_hw_reset();
	ret = nvt_check_fw_reset_state(RESET_STATE_INIT);
    if (ret != 0) {
		tp_log_err("nvt_check_fw_reset_state failed. ret=%d\n", ret);
		//goto err_init_NVT_ts;
    }

	//---set device node---
#if NVT_TOUCH_PROC
	ret = nvt_flash_proc_init();
	if (ret != 0) {
		tp_log_err("nvt flash proc init failed. ret=%d\n", ret);
		goto err_init_NVT_ts;
	}
#endif

#if NVT_TOUCH_EXT_PROC
	ret = nvt_extra_proc_init();
	if (ret != 0) {
		tp_log_err("nvt extra proc init failed. ret=%d\n", ret);
		goto err_init_NVT_ts;
	}
#endif

#if NVT_TOUCH_MP
	ret = nvt_mp_proc_init();
	if (ret != 0) {
		tp_log_err("nvt mp proc init failed. ret=%d\n", ret);
		goto err_init_NVT_ts;
	}
#endif
	//add sysfs
	nvt_sysfs_init();

#if BOOT_UPDATE_FIRMWARE
	wake_lock_init(&ts->ts_flash_wake_lock, WAKE_LOCK_SUSPEND, NVT_TS_NAME);
	nvt_fwu_wq = create_singlethread_workqueue("nvt_fwu_wq");
	if (!nvt_fwu_wq) {
		tp_log_err("%s: nvt_fwu_wq create workqueue failed\n", __func__);
		ret = -ENOMEM;
		goto err_create_nvt_fwu_wq_failed;
	}
	INIT_DELAYED_WORK(&ts->nvt_fwu_work, Boot_Update_Firmware);
	recovery_flag = get_boot_into_recovery_flag();
	if(1 == recovery_flag) {
		tp_log_info("%s, recovery mode not update firmware\n", __func__);
	} else {
		tp_log_info("recovery_flag:%d\n", recovery_flag);
		queue_delayed_work(nvt_fwu_wq, &ts->nvt_fwu_work, msecs_to_jiffies(2000));
	}
#else
	INIT_DELAYED_WORK(&ts->nvt_fwu_work, Boot_Update_Firmware);
#endif

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	/* detect current device successful, set the flag as present */
	set_hw_dev_flag(DEV_I2C_TOUCH_PANEL);
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = nvt_ts_early_suspend;
	ts->early_suspend.resume = nvt_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#elif defined(CONFIG_FB)
	nvt_setup_fb_notifier(ts);
#endif
	tp_log_info("sleep_mode = %d\n", ts->sleep_mode);
	ts->sleep_mode = 0;
	tp_log_info("%s: finished\n", __func__);
	enable_irq(client->irq);
	return 0;

err_init_NVT_ts:
	free_irq(client->irq,ts);
err_int_request_failed:
err_input_register_device_failed:
#if BOOT_UPDATE_FIRMWARE
err_create_nvt_fwu_wq_failed:
	wake_lock_destroy(&ts->ts_flash_wake_lock);
#endif
	nvt_sysfs_remove();
	input_free_device(ts->input_dev);
err_input_dev_alloc_failed:
err_create_nvt_wq_failed:
#ifdef CONFIG_HUAWEI_DSM
	if (nvt_tp_dclient) {
		dsm_unregister_client(nvt_tp_dclient, &dsm_nvt_tp);
		nvt_tp_dclient = NULL;
	}
#endif/*CONFIG_HUAWEI_DSM*/
err_chipid_failed:
	gpio_free(ts->irq_gpio);
err_gpio_request_irq:
	gpio_free(ts->reset_gpio);
err_gpio_request_rst:
#if DRAGONBOARD_REGULATOR
	nvt_power_off(ts);
err_nvt_power_on:
	nvt_power_remove(ts);
err_power_init:
#endif
	i2c_set_clientdata(client, NULL);
	kfree(ts);
err_check_functionality_failed:
	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen driver release function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_ts_remove(struct i2c_client *client)
{
	//struct nvt_ts_data *ts = i2c_get_clientdata(client);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#elif defined(CONFIG_FB)
	fb_unregister_client(&ts->fb_notifier);
#endif

	tp_log_info("%s: removing driver...\n", __func__);

	free_irq(client->irq, ts);
#if BOOT_UPDATE_FIRMWARE
	wake_lock_destroy(&ts->ts_flash_wake_lock);
#endif

	gpio_free(ts->irq_gpio);
	gpio_free(ts->reset_gpio);

	nvt_sysfs_remove();
	input_unregister_device(ts->input_dev);
	i2c_set_clientdata(client, NULL);
	kfree(ts);
	tp_log_info("%s\n", __func__);
	return 0;
}

static ssize_t nvt_manual_upgrade_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int rc = 0;
	int update_flag = 0;
	tp_log_info("%s, start manual_upgrade_firmware\n", __func__);

	rc = sscanf(buf, "%d", &update_flag);
	if (rc < 0){
		tp_log_err("%s: kstrtoul error.\n",__func__);
		return rc;
	}

	if (update_flag != 0x01 && update_flag != 0x00){
		tp_log_err("%s: input value is error.\n",__func__);
		return -EINVAL;
	}

	tp_log_info("%s, firmware file name :%s\n", __func__, NVT_FIRMWARE_NAME_FROM_SDCARD);
	rc = nvt_fw_update_from_sdcard(NVT_FIRMWARE_NAME_FROM_SDCARD, update_flag);
	if(rc != 0){
		tp_log_err("%s, nvt_fw_update_from_sdcard failed\n" ,__func__);
		return rc;
	}
	return size;
}
/* set write-only permission for sysfs node*/

static ssize_t nvt_chip_info_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
	int ret = 0;
	tp_log_info("%s, called nvt_chip_info_show \n", __func__ );
	ret = nvt_get_chip_info(buf);
	if(ret < 0) {
		tp_log_err("%s, nvt_get_chip_info failed\n" ,__func__);
		return ret;
	}
	return ret;
}

static ssize_t nvt_hw_reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int rc = 0;
	int reset_flag = 0;
	tp_log_info("%s, start nvt_hw_reset_store\n", __func__);

	rc = sscanf(buf, "%d", &reset_flag);
	if (rc < 0){
		tp_log_err("%s: kstrtoul error.\n",__func__);
		return rc;
	}

	if (reset_flag != 0x01){
		tp_log_err("%s: input value is error.input value:%d\n",__func__, reset_flag);
		return -EINVAL;
	}
	nvt_hw_reset();
	tp_log_info("%s, end nvt_hw_reset_store\n", __func__);

	return size;
}


static DEVICE_ATTR(manual_upgrade, S_IWUSR,
	NULL, nvt_manual_upgrade_store);

static DEVICE_ATTR(hw_reset, S_IWUSR,
	NULL, nvt_hw_reset_store);

static DEVICE_ATTR(touch_chip_info, 0444, nvt_chip_info_show, NULL);

static struct attribute *nvt_ts_attributes[] = {
	&dev_attr_manual_upgrade.attr,
	&dev_attr_touch_chip_info.attr,
	&dev_attr_hw_reset.attr,
	NULL
};

static struct attribute_group nvt_ts_attribute_group = {
	.attrs = nvt_ts_attributes
};

void nvt_sysfs_init(void)
{
	int ret=0;
	ts->nvt_kobject= NULL;
	ts->nvt_kobject = kobject_create_and_add("touchscreen", NULL);
	if (!ts->nvt_kobject) {
		tp_log_err("create kobjetct error!\n");
		return ;
	}
	ret = sysfs_create_group(ts->nvt_kobject, &nvt_ts_attribute_group);
	if (ret) {
		tp_log_err("%s() - ERROR: sysfs_create_group() failed: %d\n", __func__, ret);
	} else {
		tp_log_err("%s() - sysfs_create_group() succeeded.\n", __func__);
	}
}

void nvt_sysfs_remove(void)
{
	if (!ts->nvt_kobject) {
		tp_log_err("ts->nvt_kobject no exist!\n");
		return ;
	}
	sysfs_remove_group(ts->nvt_kobject, &nvt_ts_attribute_group);
}

static void nvt_report_up(void)
{
	int i = 0;
	for (i = 0; i < ts->max_touch_num; i++) {
		input_mt_slot(ts->input_dev, i);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, false);
	}

	input_report_key(ts->input_dev, BTN_TOUCH, 0);
	input_sync(ts->input_dev);
}
/*******************************************************
Description:
	Novatek touchscreen driver suspend function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_ts_suspend(struct device *dev)
{
	uint8_t buf[4] = {0};

	tp_log_info("%s: begin...\n", __func__);
	if(ts->sleep_mode == 1) {
		tp_log_info("%s: tp already sleep...\n", __func__);
		goto exit;
	}

	if(ts->update_firmware_status == 1) {
		tp_log_info("%s: tp update_firmware now...\n", __func__);
		goto exit;
	}

	if(atomic_read(&nvt_mmi_test_status)) {
		tp_log_info("%s: tp factory test now...\n", __func__);
		goto exit;
	}
#if WAKEUP_GESTURE

	if (!work_busy(&ts->nvt_fwu_work.work)) {	// make sure fw update is not on-going
		bTouchIsAwake = 0;

		tp_log_info("%s: Enable touch wakeup gesture\n", __func__);

		//---dummy read to resume TP before writing command---
		CTP_I2C_READ_DUMMY(ts->client, I2C_FW_Address);

		//---write i2c command to enter "wakeup gesture mode"---
		buf[0] = 0x50;
		buf[1] = 0x13;
		buf[2] = 0xFF;
		buf[3] = 0xFF;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 4);

		enable_irq_wake(ts->client->irq);
	}
#else
	disable_irq(ts->client->irq);

	if (!work_busy(&ts->nvt_fwu_work.work)) {	// make sure fw update is not on-going
		//---dummy read to resume TP before writing command---
		CTP_I2C_READ_DUMMY(ts->client, I2C_FW_Address);

		//---write i2c command to enter "deep sleep mode"---
		buf[0] = 0x50;
		buf[1] = 0x12;
		CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);

		nvt_power_off(ts);
		gpio_direction_output(ts->reset_gpio, 0);
		msleep(30);
	}
#endif
	nvt_report_up();
	ts->sleep_mode = 1;
exit:
	tp_log_info("%s: end\n", __func__);

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen driver resume function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_ts_resume(struct device *dev)
{
#if WAKEUP_GESTURE
	uint8_t buf[4] = {0};
#endif
	tp_log_info("%s: begin...\n", __func__);
	if(ts->sleep_mode == 0) {
		tp_log_info("%s: tp already wake...\n", __func__);
		goto exit;
	}
#if WAKEUP_GESTURE
	if (bWakeupByGesture == 1) {
		bWakeupByGesture = 0;

		if (!work_busy(&ts->nvt_fwu_work.work)) {	// make sure fw update is not on-going
			//---write i2c command to leave "wakeup gesture mode"---
			buf[0] = 0x50;
			buf[1] = 0x14;
			CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 2);
		}
	} else {
		if (!work_busy(&ts->nvt_fwu_work.work)) {	// make sure fw update is not on-going
			nvt_hw_reset();
			nvt_check_fw_reset_state(RESET_STATE_INIT);
		}
	}

	bTouchIsAwake = 1;
#else

	if (!work_busy(&ts->nvt_fwu_work.work)) {	// make sure fw update is not on-going
		nvt_power_on(ts);
		msleep(70);

		nvt_hw_reset();
		//nvt_bootloader_reset();
		nvt_check_fw_reset_state(RESET_STATE_INIT);
	}
#endif
	ts->sleep_mode = 0;
	enable_irq(ts->client->irq);
exit:
	tp_log_info("%s: end\n", __func__);

	return 0;
}


#ifdef CONFIG_HAS_EARLYSUSPEND
/*******************************************************
Description:
	Novatek touchscreen driver early suspend function.

return:
	n.a.
*******************************************************/
static void nvt_ts_early_suspend(struct early_suspend *h)
{
	nvt_ts_suspend(ts->client, PMSG_SUSPEND);
}

/*******************************************************
Description:
	Novatek touchscreen driver late resume function.

return:
	n.a.
*******************************************************/
static void nvt_ts_late_resume(struct early_suspend *h)
{
	nvt_ts_resume(ts->client);
}

#elif defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *self,
		unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;

	if (event == FB_EVENT_BLANK && evdata) {
		blank = evdata->data;
		if (*blank == FB_BLANK_UNBLANK) {
			tp_log_info("%s: UNBLANK!\n", __func__);
			nvt_ts_resume(&ts->client->dev);
		} else if (*blank == FB_BLANK_POWERDOWN) {
			tp_log_info("%s: POWERDOWN!\n", __func__);
			nvt_ts_suspend(&ts->client->dev);
		}
	}

	return 0;
}

static void nvt_setup_fb_notifier(struct nvt_ts_data *cd)
{
	int rc = 0;

	cd->fb_notifier.notifier_call = fb_notifier_callback;

	rc = fb_register_client(&cd->fb_notifier);
	if (rc)
		tp_log_err( "Unable to register fb_notifier: %d\n", rc);
}

#endif

#if DRAGONBOARD_TP_SUSPEND
/*******************************************************
Description:
	Novatek touchscreen suspend from mdss function.

return:
	n.a.
*******************************************************/
void nvt_ts_suspend_mdss(void)
{
	nvt_ts_suspend(&ts->client->dev);
}

/*******************************************************
Description:
	Novatek touchscreen resume from mdss function.

return:
	n.a.
*******************************************************/
void nvt_ts_resume_mdss(void)
{
	nvt_ts_resume(&ts->client->dev);
}
#endif

/*
static const struct dev_pm_ops nvt_ts_dev_pm_ops = {
	.suspend = nvt_ts_suspend,
	.resume  = nvt_ts_resume,
};

*/
static const struct i2c_device_id nvt_ts_id[] = {
	{ NVT_I2C_NAME, 0 },
	{ }
};

#ifdef CONFIG_OF
static struct of_device_id nvt_match_table[] = {
	{ .compatible = "novatek,NVT-ts",},
	{ },
};
#endif
/*
static struct i2c_board_info __initdata nvt_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO(NVT_I2C_NAME, I2C_FW_Address),
	},
};
*/

static struct i2c_driver nvt_i2c_driver = {
	.probe		= nvt_ts_probe,
	.remove		= nvt_ts_remove,
//	.suspend	= nvt_ts_suspend,
//	.resume		= nvt_ts_resume,
	.id_table	= nvt_ts_id,
	.driver = {
		.name	= NVT_I2C_NAME,
		.owner	= THIS_MODULE,
/*
#ifdef CONFIG_PM
		.pm = &nvt_ts_dev_pm_ops,
#endif
*/
#ifdef CONFIG_OF
		.of_match_table = nvt_match_table,
#endif
	},
};

/*******************************************************
Description:
	Driver Install function.

return:
	Executive Outcomes. 0---succeed. not 0---failed.
********************************************************/
static int32_t __init nvt_driver_init(void)
{
	int32_t ret = 0;

	//---add i2c driver---
	ret = i2c_add_driver(&nvt_i2c_driver);
	if (ret) {
		tp_log_err("%s: failed to add i2c driver", __func__);
		goto err_driver;
	}

	tp_log_info("%s: finished\n", __func__);

err_driver:
	return ret;
}

/*******************************************************
Description:
	Driver uninstall function.

return:
	n.a.
********************************************************/
static void __exit nvt_driver_exit(void)
{
	i2c_del_driver(&nvt_i2c_driver);

	if (nvt_wq)
		destroy_workqueue(nvt_wq);

#if BOOT_UPDATE_FIRMWARE
	if (nvt_fwu_wq)
		destroy_workqueue(nvt_fwu_wq);
#endif

#if DRAGONBOARD_REGULATOR
	nvt_power_off(ts);
	nvt_power_remove(ts);
#endif
	tp_log_info("%s\n", __func__);
}

//late_initcall(nvt_driver_init);
module_init(nvt_driver_init);
module_exit(nvt_driver_exit);

MODULE_DESCRIPTION("Novatek Touchscreen Driver");
MODULE_LICENSE("GPL");
