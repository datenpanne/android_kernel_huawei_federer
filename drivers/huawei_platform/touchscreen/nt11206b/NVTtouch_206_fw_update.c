/* drivers/input/touchscreen/nt11206/NVTtouch_206_fw_update.c
 *
 * Copyright (C) 2010 - 2016 Novatek, Inc.
 *
 * $Revision: 3307 $
 * $Date: 2016-02-01 19:16:12 +0800 (週一, 01 二月 2016) $
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

#include <linux/delay.h>
#include <linux/firmware.h>

#include "NVTtouch_206.h"
#ifdef CONFIG_APP_INFO
#include <misc/app_info.h>
#endif
#include <dsm/dsm_pub.h>

#define BOOT_UPDATE_FIRMWARE_FLAG_FILENAME	"/system/etc/tp_test_parameters/boot_update_firmware.flag"
#define BOOT_UPDATE_FIRMWARE_FLAG "boot_update_firmware_flag:"

#if BOOT_UPDATE_FIRMWARE

#define FW_BIN_SIZE_124KB 126976
#define FW_BIN_VER_OFFSET 0x1E000
#define FW_BIN_VER_BAR_OFFSET 0x1E001
#define FW_BIN_CHIP_ID_OFFSET 0x1E00E
#define FW_BIN_CHIP_ID 6
#define FLASH_SECTOR_SIZE 4096
#define SIZE_64KB 65536
#define BLOCK_64KB_NUM 2

#define NVT_SPEC_FW_VER 36

const struct firmware *fw_entry = NULL;
extern uint8_t fw_ver;

extern struct nvt_ts_data *ts;
extern int32_t CTP_I2C_READ(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len);
extern int32_t CTP_I2C_READ_DUMMY(struct i2c_client *client, uint16_t address);
extern int32_t CTP_I2C_WRITE(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len);
extern void nvt_hw_reset(void);
extern void nvt_bootloader_reset(void);
extern int32_t nvt_check_fw_reset_state(RST_COMPLETE_STATE check_reset_state);
extern int32_t nvt_sw_reset_idle(void);

/*******************************************************
Description:
	Novatek touchscreen request update firmware function.

return:
	Executive outcomes. 0---succeed. -1,-22---failed.
*******************************************************/
int32_t update_firmware_request(char *filename)
{
	int32_t ret = 0;

	if (NULL == filename) {
		return -1;
	}

	tp_log_info("%s: filename is %s\n", __func__, filename);

	ret = request_firmware(&fw_entry, filename, &ts->client->dev);
	if (ret) {
		tp_log_err("%s: firmware load failed, ret=%d\n", __func__, ret);
		return ret;
	}

	// check bin file size (124kb)
	if (fw_entry->size != FW_BIN_SIZE_124KB) {
		tp_log_err("%s: bin file size not match. (%zu)\n", __func__, fw_entry->size);
		return -EINVAL;
	}

	// check if FW version add FW version bar equals 0xFF
	if (*(fw_entry->data + FW_BIN_VER_OFFSET) + *(fw_entry->data + FW_BIN_VER_BAR_OFFSET) != 0xFF) {
		tp_log_err("%s: bin file FW_VER + FW_VER_BAR should be 0xFF!\n", __func__);
		tp_log_err("%s: FW_VER=0x%02X, FW_VER_BAR=0x%02X\n", __func__, *(fw_entry->data+FW_BIN_VER_OFFSET), *(fw_entry->data+FW_BIN_VER_BAR_OFFSET));
		return -EINVAL;
	}

	//check chipid == 6
	if (*(fw_entry->data+FW_BIN_CHIP_ID_OFFSET) != FW_BIN_CHIP_ID) {
		tp_log_err("%s : bin file check failed. (chipid=%d)\n", __func__, *(fw_entry->data+FW_BIN_CHIP_ID_OFFSET));
		return -EINVAL;
	}

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen release update firmware function.

return:
	n.a.
*******************************************************/
void update_firmware_release(void)
{
	if (fw_entry) {
		release_firmware(fw_entry);
	}
	fw_entry=NULL;
}

/*******************************************************
Description:
	Novatek touchscreen check firmware version function.

return:
	Executive outcomes. 0---need update. 1---need not
	update.
*******************************************************/
int32_t Check_FW_Ver(void)
{
	uint8_t buf[16] = {0};
	int32_t ret = 0;

	//---dummy read to resume TP before writing command---
	CTP_I2C_READ_DUMMY(ts->client, I2C_HW_Address);

	//write i2c index to 0x14700
	buf[0] = 0xFF;
	buf[1] = 0x01;
	buf[2] = 0x47;
	ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
	if (ret < 0) {
		tp_log_err("%s: i2c write error!(%d)\n", __func__, ret);
		return ret;
	}

	//read Firmware Version
	buf[0] = 0x78;
	buf[1] = 0x00;
	buf[2] = 0x00;
	ret = CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 3);
	if (ret < 0) {
		tp_log_err("%s: i2c read error!(%d)\n", __func__, ret);
		return ret;
	}

	tp_log_info("IC FW Ver = 0x%02X, FW Ver Bar = 0x%02X\n", buf[1], buf[2]);
	tp_log_info("Bin FW Ver = 0x%02X, FW ver Bar = 0x%02X\n",
			fw_entry->data[FW_BIN_VER_OFFSET], fw_entry->data[FW_BIN_VER_BAR_OFFSET]);

	// check IC FW_VER + FW_VER_BAR equals 0xFF or not, need to update if not
	if ((buf[1] + buf[2]) != 0xFF) {
		tp_log_err("%s: IC FW_VER + FW_VER_BAR not equals to 0xFF!\n", __func__);
		return 0;
	}

	// compare IC and binary FW version
	if (buf[1] == NVT_SPEC_FW_VER && buf[1] != fw_entry->data[FW_BIN_VER_OFFSET]) {
		tp_log_info("%s, IC fw ver is 36, will upgrade fw\n", __func__);
		return 0;
	} else if (fw_entry->data[FW_BIN_VER_OFFSET] >= buf[1] && fw_entry->data[FW_BIN_VER_OFFSET] != NVT_SPEC_FW_VER)
		return 0;
	else
		return 1;
}

/*******************************************************
Description:
	Novatek touchscreen check firmware checksum function.

return:
	Executive outcomes. 0---checksum not match.
	1---checksum match. -1--- checksum read failed.
*******************************************************/
int32_t Check_CheckSum(void)
{
	uint8_t buf[64] = {0};
	uint32_t XDATA_Addr = 0x14000;
	int32_t ret = 0;
	int32_t i = 0;
	int32_t k = 0;
	uint16_t WR_Filechksum[BLOCK_64KB_NUM] = {0};
	uint16_t RD_Filechksum[BLOCK_64KB_NUM] = {0};
	size_t fw_bin_size = 0;
	size_t len_in_blk = 0;
	int32_t retry = 0;

	fw_bin_size = fw_entry->size;

	for (i = 0; i < BLOCK_64KB_NUM; i++) {
		if (fw_bin_size > (i * SIZE_64KB)) {
			len_in_blk = min(fw_bin_size - i * SIZE_64KB, (size_t)SIZE_64KB);
			// Calculate WR_Filechksum of each 64KB block
			WR_Filechksum[i] = i + 0x00 + 0x00 + (((len_in_blk - 1) >> 8) & 0xFF) + ((len_in_blk - 1) & 0xFF);
			for (k = 0; k < len_in_blk; k++) {
				WR_Filechksum[i] += fw_entry->data[k + i * SIZE_64KB];
			}
			WR_Filechksum[i] = 65535 - WR_Filechksum[i] + 1;

			if (i == 0) {
				//---dummy read to resume TP before writing command---
				CTP_I2C_READ_DUMMY(ts->client, I2C_HW_Address);
			}

			// Fast Read Command
			buf[0] = 0x00;
			buf[1] = 0x07;
			buf[2] = i;
			buf[3] = 0x00;
			buf[4] = 0x00;
			buf[5] = ((len_in_blk - 1) >> 8) & 0xFF;
			buf[6] = (len_in_blk - 1) & 0xFF;
			ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 7);
			if (ret < 0) {
				tp_log_err("%s: Fast Read Command error!!(%d)\n", __func__, ret);
				return ret;
			}
			// Check 0xAA (Fast Read Command)
			retry = 0;
			while (1) {
				msleep(80);
				buf[0] = 0x00;
				buf[1] = 0x00;
				ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 2);
				if (ret < 0) {
					tp_log_err("%s: Check 0xAA (Fast Read Command) error!!(%d)\n", __func__, ret);
					return ret;
				}
				if (buf[1] == 0xAA) {
					break;
				}
				retry++;
				if (unlikely(retry > 5)) {
					tp_log_err("%s: Check 0xAA (Fast Read Command) failed, buf[1]=0x%02X, retry=%d\n", __func__, buf[1], retry);
					return -1;
				}
			}

			// Read Checksum (write addr high byte & middle byte)
			buf[0] = 0xFF;
			buf[1] = XDATA_Addr >> 16;
			buf[2] = (XDATA_Addr >> 8) & 0xFF;
			ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
			if (ret < 0) {
				tp_log_err("%s: Read Checksum (write addr high byte & middle byte) error!!(%d)\n", __func__, ret);
				return ret;
			}
			// Read Checksum
			buf[0] = (XDATA_Addr) & 0xFF;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 3);
			if (ret < 0) {
				tp_log_err("%s: Read Checksum error!!(%d)\n", __func__, ret);
				return ret;
			}

			RD_Filechksum[i] = (uint16_t)((buf[2] << 8) | buf[1]);
			if (WR_Filechksum[i] != RD_Filechksum[i]) {
				tp_log_err("RD_Filechksum[%d]=0x%04X, WR_Filechksum[%d]=0x%04X\n", i, RD_Filechksum[i], i, WR_Filechksum[i]);
				tp_log_err("%s: firmware checksum not match!!\n", __func__);
				return 0;
			}
		}
	}

	tp_log_info("%s: firmware checksum match\n", __func__);
	return 1;
}

/*******************************************************
Description:
	Novatek touchscreen initial bootloader and flash
	block function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t Init_BootLoader(void)
{
	uint8_t buf[64] = {0};
	int32_t ret = 0;

	// SW Reset & Idle
	nvt_sw_reset_idle();

	// Initiate Flash Block
	buf[0] = 0x00;
	buf[1] = 0x00;
	ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
	if (ret < 0) {
		tp_log_err("%s: Inittial Flash Block error!!(%d)\n", __func__, ret);
		return ret;
	}
	msleep(5);

	// Check 0xAA (Initiate Flash Block)
	buf[0] = 0x00;
	buf[1] = 0x00;
	ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 2);
	if (ret < 0) {
		tp_log_err("%s: Check 0xAA (Inittial Flash Block) error!!(%d)\n", __func__, ret);
		return ret;
	}
	if (buf[1] != 0xAA) {
		tp_log_info("%s: Check 0xAA (Inittial Flash Block) error!! status=0x%02X\n", __func__, buf[1]);
		return -1;
	}

	tp_log_info("Init OK \n");
	msleep(20);

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen erase flash sectors function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t Erase_Flash(uint8_t *data, int32_t data_len)
{
	uint8_t buf[64] = {0};
	int32_t ret = 0;
	int32_t count = 0;
	int32_t i = 0;
	int32_t Flash_Address = 0;
	int32_t retry = 0;

	if (data_len % FLASH_SECTOR_SIZE)
		count = data_len / FLASH_SECTOR_SIZE + 1;
	else
		count = data_len / FLASH_SECTOR_SIZE;

	for(i = 0; i < count; i++) {
		// Write Enable
		buf[0] = 0x00;
		buf[1] = 0x06;
		ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
		if (ret < 0) {
			tp_log_err("%s: Write Enable error!!(%d,%d)\n", __func__, ret, i);
			return ret;
		}
		msleep(10);

		// Check 0xAA (Write Enable)
		buf[0] = 0x00;
		buf[1] = 0x00;
		ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 2);
		if (ret < 0) {
			tp_log_err("%s: Check 0xAA (Write Enable) error!!(%d,%d)\n", __func__, ret, i);
			return ret;
		}
		if (buf[1] != 0xAA) {
			tp_log_info("%s: Check 0xAA (Write Enable) error!! status=0x%02X\n", __func__, buf[1]);
			return -1;
		}
		msleep(10);

		Flash_Address = i * FLASH_SECTOR_SIZE;

		// Sector Erase
		buf[0] = 0x00;
		buf[1] = 0x20;    // Command : Sector Erase
		buf[2] = ((Flash_Address >> 16) & 0xFF);
		buf[3] = ((Flash_Address >> 8) & 0xFF);
		buf[4] = (Flash_Address & 0xFF);
		ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 5);
		if (ret < 0) {
			tp_log_err("%s: Sector Erase error!!(%d,%d)\n", __func__, ret, i);
			return ret;
		}
		msleep(20);

		retry = 0;
		while (1) {
			// Check 0xAA (Sector Erase)
			buf[0] = 0x00;
			buf[1] = 0x00;
			ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 2);
			if (ret < 0) {
				tp_log_err("%s: Check 0xAA (Sector Erase) error!!(%d,%d)\n", __func__, ret, i);
				return ret;
			}
			if (buf[1] == 0xAA) {
				break;
			}
			retry++;
			if (unlikely(retry > 5)) {
				tp_log_err("%s: Check 0xAA (Sector Erase) failed, buf[1]=0x%02X, retry=%d\n", __func__, buf[1], retry);
				return -1;
			}
		}

		// Read Status
		retry = 0;
		while (1) {
			msleep(30);
			buf[0] = 0x00;
			buf[1] = 0x05;
			ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
			if (ret < 0) {
				tp_log_err("%s: Read Status error!!(%d,%d)\n", __func__, ret, i);
				return ret;
			}

			// Check 0xAA (Read Status)
			buf[0] = 0x00;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 3);
			if (ret < 0) {
				tp_log_err("%s: Check 0xAA (Read Status) error!!(%d,%d)\n", __func__, ret, i);
				return ret;
			}
			if ((buf[1] == 0xAA) && (buf[2] == 0x00)) {
				break;
			}
			retry++;
			if (unlikely(retry > 5)) {
				tp_log_err("%s:Check 0xAA (Read Status) failed, buf[1]=0x%02X, buf[2]=0x%02X, retry=%d\n", __func__, buf[1], buf[2], retry);
				return -1;
			}
		}
	}

	tp_log_info("Erase OK \n");
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen write flash sectors function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t Write_Flash(uint8_t *data, int32_t data_len)
{
	uint8_t buf[64] = {0};
	uint32_t XDATA_Addr = 0x14002;
	uint32_t Flash_Address = 0;
	int32_t i = 0, j = 0, k = 0;
	uint8_t tmpvalue = 0;
	int32_t count = 0;
	int32_t ret = 0;
	int32_t retry = 0;

	// change I2C buffer index
	buf[0] = 0xFF;
	buf[1] = XDATA_Addr >> 16;
	buf[2] = (XDATA_Addr >> 8) & 0xFF;
	ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
	if (ret < 0) {
		tp_log_err("%s: change I2C buffer index error!!(%d)\n", __func__, ret);
		return ret;
	}

	if (data_len % 256)
		count = data_len / 256 + 1;
	else
		count = data_len / 256;

	for (i = 0; i < count; i++) {
		Flash_Address = i * 256;

		// Write Enable
		buf[0] = 0x00;
		buf[1] = 0x06;
		ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
		if (ret < 0) {
			tp_log_err("%s: Write Enable error!!(%d)\n", __func__, ret);
			return ret;
		}
		udelay(100);

		// Write Page : 256 bytes
		for (j = 0; j < min((size_t)data_len - i * 256, (size_t)256); j += 32) {
			buf[0] = (XDATA_Addr + j) & 0xFF;
			for (k = 0; k < 32; k++) {
				buf[1 + k] = data[Flash_Address + j + k];
			}
			ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 33);
			if (ret < 0) {
				tp_log_err("%s: Write Page error!!(%d), j=%d\n", __func__, ret, j);
				return ret;
			}
		}
		if (data_len - Flash_Address >= 256)
			tmpvalue=(Flash_Address >> 16) + ((Flash_Address >> 8) & 0xFF) + (Flash_Address & 0xFF) + 0x00 + (255);
		else
			tmpvalue=(Flash_Address >> 16) + ((Flash_Address >> 8) & 0xFF) + (Flash_Address & 0xFF) + 0x00 + (data_len - Flash_Address - 1);

		for (k = 0;k < min((size_t)data_len - Flash_Address,(size_t)256); k++)
			tmpvalue += data[Flash_Address + k];

		tmpvalue = 255 - tmpvalue + 1;

		// Page Program
		buf[0] = 0x00;
		buf[1] = 0x02;
		buf[2] = ((Flash_Address >> 16) & 0xFF);
		buf[3] = ((Flash_Address >> 8) & 0xFF);
		buf[4] = (Flash_Address & 0xFF);
		buf[5] = 0x00;
		buf[6] = min((size_t)data_len - Flash_Address,(size_t)256) - 1;
		buf[7] = tmpvalue;
		ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 8);
		if (ret < 0) {
			tp_log_err("%s: Page Program error!!(%d), i=%d\n", __func__, ret, i);
			return ret;
		}

		// Check 0xAA (Page Program)
		retry = 0;
		while (1) {
			mdelay(3);
			buf[0] = 0x00;
			buf[1] = 0x00;
			ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 2);
			if (ret < 0) {
				tp_log_err("%s: Page Program error!!(%d)\n", __func__, ret);
				return ret;
			}
			if (buf[1] == 0xAA || buf[1] == 0xEA) {
				break;
			}
			retry++;
			if (unlikely(retry > 5)) {
				tp_log_err("%s: Check 0xAA (Page Program) failed, buf[1]=0x%02X, retry=%d\n", __func__, buf[1], retry);
				return -1;
			}
		}
		if (buf[1] == 0xEA) {
			tp_log_err("%s: Page Program error!! i=%d\n", __func__, i);
			return -3;
		}

		// Read Status
		retry = 0;
		while (1) {
			mdelay(2);
			buf[0] = 0x00;
			buf[1] = 0x05;
			ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 2);
			if (ret < 0) {
				tp_log_err("%s: Read Status error!!(%d)\n", __func__, ret);
				return ret;
			}

			// Check 0xAA (Read Status)
			buf[0] = 0x00;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 3);
			if (ret < 0) {
				tp_log_err("%s: Check 0xAA (Read Status) error!!(%d)\n", __func__, ret);
				return ret;
			}
			if (((buf[1] == 0xAA) && (buf[2] == 0x00)) || (buf[1] == 0xEA)) {
				break;
			}
			retry++;
			if (unlikely(retry > 5)) {
				tp_log_err("%s: Check 0xAA (Read Status) failed, buf[1]=0x%02X, buf[2]=0x%02X, retry=%d\n", __func__, buf[1], buf[2], retry);
				return -1;
			}
		}
		if (buf[1] == 0xEA) {
			tp_log_err("%s: Page Program error!! i=%d\n", __func__, i);
			return -4;
		}

		//tp_log_info("Programming...%2d%%\r", ((i * 100) / count));
	}

	//tp_log_info("Programming...%2d%%\r", 100);
	tp_log_info("Program OK         \n");
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen verify checksum of written
	flash function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t Verify_Flash(uint8_t *data, int32_t data_len)
{
	uint8_t buf[64] = {0};
	uint32_t XDATA_Addr = 0x14000;
	int32_t ret = 0;
	int32_t i = 0;
	int32_t k = 0;
	uint16_t WR_Filechksum[BLOCK_64KB_NUM] = {0};
	uint16_t RD_Filechksum[BLOCK_64KB_NUM] = {0};
	size_t fw_bin_size = 0;
	size_t len_in_blk = 0;
	int32_t retry = 0;

	fw_bin_size = data_len;

	for (i = 0; i < BLOCK_64KB_NUM; i++) {
		if (fw_bin_size > (i * SIZE_64KB)) {
			len_in_blk = min(fw_bin_size - i * SIZE_64KB, (size_t)SIZE_64KB);
			// Calculate WR_Filechksum of each 64KB block
			WR_Filechksum[i] = i + 0x00 + 0x00 + (((len_in_blk - 1) >> 8) & 0xFF) + ((len_in_blk - 1) & 0xFF);
			for (k = 0; k < len_in_blk; k++) {
				WR_Filechksum[i] +=  data[k + i * SIZE_64KB];
			}
			WR_Filechksum[i] = 65535 - WR_Filechksum[i] + 1;

			// Fast Read Command
			buf[0] = 0x00;
			buf[1] = 0x07;
			buf[2] = i;
			buf[3] = 0x00;
			buf[4] = 0x00;
			buf[5] = ((len_in_blk - 1) >> 8) & 0xFF;
			buf[6] = (len_in_blk - 1) & 0xFF;
			ret = CTP_I2C_WRITE(ts->client, I2C_HW_Address, buf, 7);
			if (ret < 0) {
				tp_log_err("%s: Fast Read Command error!!(%d)\n", __func__, ret);
				return ret;
			}
			// Check 0xAA (Fast Read Command)
			retry = 0;
			while (1) {
				msleep(80);
				buf[0] = 0x00;
				buf[1] = 0x00;
				ret = CTP_I2C_READ(ts->client, I2C_HW_Address, buf, 2);
				if (ret < 0) {
					tp_log_err("%s: Check 0xAA (Fast Read Command) error!!(%d)\n", __func__, ret);
					return ret;
				}
				if (buf[1] == 0xAA) {
					break;
				}
				retry++;
				if (unlikely(retry > 5)) {
					tp_log_err("%s: Check 0xAA (Fast Read Command) failed, buf[1]=0x%02X, retry=%d\n", __func__, buf[1], retry);
					return -1;
				}
			}

			// Read Checksum (write addr high byte & middle byte)
			buf[0] = 0xFF;
			buf[1] = XDATA_Addr >> 16;
			buf[2] = (XDATA_Addr >> 8) & 0xFF;
			ret = CTP_I2C_WRITE(ts->client, I2C_FW_Address, buf, 3);
			if (ret < 0) {
				tp_log_err("%s: Read Checksum (write addr high byte & middle byte) error!!(%d)\n", __func__, ret);
				return ret;
			}
			// Read Checksum
			buf[0] = (XDATA_Addr) & 0xFF;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = CTP_I2C_READ(ts->client, I2C_FW_Address, buf, 3);
			if (ret < 0) {
				tp_log_err("%s: Read Checksum error!!(%d)\n", __func__, ret);
				return ret;
			}

			RD_Filechksum[i] = (uint16_t)((buf[2] << 8) | buf[1]);
			if (WR_Filechksum[i] != RD_Filechksum[i]) {
				tp_log_err("Verify Fail%d!!\n", i);
				tp_log_err("RD_Filechksum[%d]=0x%04X, WR_Filechksum[%d]=0x%04X\n", i, RD_Filechksum[i], i, WR_Filechksum[i]);
				return -1;
			}
		}
	}

	tp_log_info("Verify OK \n");
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen update firmware function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t Update_Firmware(uint8_t *data, int32_t data_len)
{
	int32_t ret = 0;

	// Step 1 : initial bootloader
	ret = Init_BootLoader();
	if (ret) {
		return ret;
	}

	// Step 2 : Erase
	ret = Erase_Flash(data, data_len);
	if (ret) {
		return ret;
	}

	// Stap 3 : Program
	ret = Write_Flash(data, data_len);
	if (ret) {
		return ret;
	}

	// Stap 4 : Verify
	ret = Verify_Flash(data, data_len);
	if (ret) {
		return ret;
	}

	//Step 5 : Bootloader Reset
	nvt_bootloader_reset();
	nvt_check_fw_reset_state(RESET_STATE_INIT);

	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen update firmware when booting
	function.

return:
	n.a.
*******************************************************/
void Boot_Update_Firmware(struct work_struct *work)
{
//	struct i2c_client *client = ts->client;
	int32_t ret = 0;
	int value = 0;

	char firmware_name[256] = "";
	char chipinfo[256] = {0};
	sprintf(firmware_name, BOOT_UPDATE_FIRMWARE_NAME);

	// request bin file in "/etc/firmware"
	ret = update_firmware_request(firmware_name);
	if (ret) {
	#ifdef CONFIG_HUAWEI_DSM
		nvt_tp_report_dsm_err(&ts->client->dev, DSM_TP_FW_ERROR_NO , -1);
	#endif
		tp_log_err("%s: update_firmware_request failed. (%d)\n", __func__, ret);
		nvt_get_chip_info(chipinfo);
	#ifdef CONFIG_APP_INFO
		app_info_set("touch_panel", chipinfo);
	#endif
		return;
	}
	wake_lock(&ts->ts_flash_wake_lock);

	nvt_sw_reset_idle();

	ret = Check_CheckSum();
	if(ret == 0) {
		tp_log_info("start check version mode\n");
		value = nvt_check_update_firmware_flag();
		if(0 == value){
			tp_log_info("now is factory test mode, not update firmware\n");
			ret = 1;
		}
	}

	if (ret < 0) {	// read firmware checksum failed
	#ifdef CONFIG_HUAWEI_DSM
		nvt_tp_report_dsm_err(&ts->client->dev, DSM_TP_FW_ERROR_NO , -2);
	#endif
		tp_log_info("%s: read firmware checksum failed\n", __func__);
		Update_Firmware((uint8_t *)fw_entry->data, fw_entry->size);
	} else if ((ret == 0) && (Check_FW_Ver() == 0)) {	// (fw checksum not match) && (bin fw version >= ic fw version)
		tp_log_info("%s: firmware version is higher\n", __func__);
		Update_Firmware((uint8_t *)fw_entry->data, fw_entry->size);
	} else {
		// Bootloader Reset
		tp_log_info("%s: firmware not upgrade\n", __func__);
		//Update_Firmware();
		nvt_bootloader_reset();
		nvt_check_fw_reset_state(RESET_STATE_INIT);
	}

	nvt_get_chip_info(chipinfo);
	tp_log_info("tp cur firmware version:0x%x\n", fw_ver);
#ifdef CONFIG_APP_INFO
	app_info_set("touch_panel", chipinfo);
#endif
	wake_unlock(&ts->ts_flash_wake_lock);
	update_firmware_release();
}

static int nvt_file_open_firmware(u8 *file_path,u8 *databuf,
                int *file_size)
{
    struct file *filp = NULL;
    struct inode *inode = NULL;
    unsigned int file_len = 0;
    mm_segment_t oldfs;
    int retval = 0;

    if(file_path == NULL || databuf == NULL){
        tp_log_err("%s: path || buf is NULL.\n", __func__);
        return -EINVAL;
    }

    tp_log_info("%s: path = %s.\n",__func__, file_path);

    // open file
    oldfs = get_fs();
    set_fs(KERNEL_DS);
    filp = filp_open(file_path, O_RDONLY, S_IRUSR);
    if (IS_ERR(filp)){
        tp_log_err("%s: open %s error.\n", __func__, file_path);
        retval = -EIO;
        goto err;
    }

    if (filp->f_op == NULL) {
        tp_log_err("%s: File Operation Method Error\n", __func__);
        retval = -EINVAL;
        goto exit;
    }

    inode = filp->f_path.dentry->d_inode;
    if (inode == NULL) {
        tp_log_err("%s: Get inode from filp failed\n", __func__);
        retval = -EINVAL;
        goto exit;
    }

    //Get file size
    file_len = i_size_read(inode->i_mapping->host);
    if (file_len == 0){
        tp_log_err("%s: file size error,file_len = %d\n",  __func__, file_len);
        retval = -EINVAL;
        goto exit;
    }

    // read image data to kernel */
    if (filp->f_op->read(filp, databuf, file_len, &filp->f_pos) != file_len) {
        tp_log_err("%s: file read error.\n", __func__);
        retval = -EINVAL;
        goto exit;
    }

    *file_size = file_len;

exit:
    filp_close(filp, NULL);
err:
    set_fs(oldfs);
    return retval;
}

int nvt_fw_update_from_sdcard(u8 *file_path, bool update_flag)
{
    int retval;
    u8 *fw_data = NULL;
    int fw_size = 0;

    tp_log_info("%s: file_name is %s.\n", __func__, file_path);

    fw_data = kzalloc(FW_BIN_SIZE_124KB, GFP_KERNEL);
    if(fw_data == NULL){
       tp_log_err("%s: kzalloc error.\n", __func__);
       return -EINVAL;
    }

    wake_lock(&ts->ts_flash_wake_lock);
    ts->update_firmware_status = 1;

    retval = nvt_file_open_firmware(file_path, fw_data, &fw_size);
    if (retval != 0) {
        tp_log_err("file_open_firmware error, code = %d\n", retval);
        goto exit;
    }

	//nvt_hw_reset();
	//nvt_check_fw_reset_state(RESET_STATE_INIT);

	retval = Update_Firmware(fw_data, fw_size);
	if(retval != 0) {
		tp_log_err("%s, nvt_update_firmware failed\n", __func__);
	}
    tp_log_info("%s is done\n",__func__);

exit:
    ts->update_firmware_status = 0;
    wake_unlock(&ts->ts_flash_wake_lock);

    kfree(fw_data);
    return retval;
}

#else
int nvt_fw_update_from_sdcard(u8 *file_path, bool update_flag)
{
	return 0;
}

void Boot_Update_Firmware(struct work_struct *work)
{
	return;
}
#endif /* BOOT_UPDATE_FIRMWARE */

int nvt_check_update_firmware_flag(void)
{
	int ret = 0;
	u8 *file_data = NULL;
	int file_size = 0;
	u8 *tmp = NULL;
	int update_flag = 0;
	u8 *file_path = BOOT_UPDATE_FIRMWARE_FLAG_FILENAME;
	file_data = kzalloc(128, GFP_KERNEL);
	if(file_data == NULL){
       tp_log_err("%s: kzalloc error.\n", __func__);
       return -EINVAL;
	}
	ret = nvt_file_open_firmware(file_path, file_data, &file_size);
	if (ret != 0) {
        tp_log_err("%s, file_open error, code = %d\n", __func__, ret);
        goto exit;
	}
	tmp = strstr(file_data, BOOT_UPDATE_FIRMWARE_FLAG);
	if (tmp == NULL) {
		tp_log_err( "%s not found\n", BOOT_UPDATE_FIRMWARE_FLAG);
		ret = -1;
		goto exit;
	} else {
		tmp = tmp + strlen(BOOT_UPDATE_FIRMWARE_FLAG);
		sscanf(tmp, "%d", &update_flag);
	}
	if (update_flag == 1) {
		ret = 0;
		tp_log_info("%s ,check success, flag = %d\n", __func__, update_flag);
		goto exit;
	} else {
		ret = -1;
		tp_log_info("%s ,check failed, flag = %d\n", __func__, update_flag);
		goto exit;
	}
exit:
	kfree(file_data);
	return ret;
}

