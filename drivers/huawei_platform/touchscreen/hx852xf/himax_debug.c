/* Himax Android Driver Sample Code for HMX852xF chipset
*
* Copyright (C) 2014 Himax Corporation.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include "himax_852xF.h"

int self_test_inter_flag = 0;

#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
	#define HIMAX_PROC_TOUCH_FOLDER 	"touchscreen"
	#define HIMAX_PROC_DEBUG_LEVEL_FILE	"debug_level"
	#define HIMAX_PROC_VENDOR_FILE		"vendor"
	#define HIMAX_PROC_ATTN_FILE		"attn"
	#define HIMAX_PROC_INT_EN_FILE		"int_en"
	#define HIMAX_PROC_LAYOUT_FILE		"layout"

	static struct proc_dir_entry *himax_touch_proc_dir 			= NULL;

	uint8_t HX_PROC_SEND_FLAG = 0;

#ifdef HX_TP_PROC_RESET
#define HIMAX_PROC_RESET_FILE		"reset"
#endif

#ifdef HX_TP_PROC_DIAG
	#define HIMAX_PROC_DIAG_FILE	"diag"

	static int 	touch_monitor_stop_limit 		= 5;
	static uint8_t x_channel 		= 0;
	static uint8_t y_channel 		= 0;
	static uint8_t *diag_mutual = NULL;

	static int diag_command = 0;
	uint8_t diag_self[100] = {0};
#endif

#ifdef HX_TP_PROC_REGISTER
	#define HIMAX_PROC_REGISTER_FILE	"register"

	static uint8_t register_command 			= 0;
	static uint8_t multi_register_command = 0;
	static uint8_t multi_register[8] 			= {0x00};
	static uint8_t multi_cfg_bank[8] 			= {0x00};
	static uint8_t multi_value[1024] 			= {0x00};
	static bool 	config_bank_reg 				= false;
#endif

#ifdef HX_TP_PROC_DEBUG
	#define HIMAX_PROC_DEBUG_FILE	"debug"
	static bool	fw_update_complete = false;
	static int handshaking_result = 0;
	static unsigned char debug_level_cmd = 0;
	static unsigned char upgrade_fw[64*1024];
#endif

#ifdef HX_TP_PROC_FLASH_DUMP
	#define HIMAX_PROC_FLASH_DUMP_FILE	"flash_dump"
	static uint8_t *flash_buffer 				= NULL;
	static uint8_t flash_command 				= 0;//command 0 1 2 3 4
	static uint8_t flash_read_step 			= 0;
	static uint8_t flash_progress 			= 0;
	static uint8_t flash_dump_complete	= 0;
	static uint8_t flash_dump_fail 			= 0;
	static uint8_t sys_operation				= 0;
	static uint8_t flash_dump_sector	 	= 0;
	static uint8_t flash_dump_page 			= 0;
	static bool    flash_dump_going			= false;

	static uint8_t getFlashCommand(void);
	static uint8_t getFlashDumpComplete(void);
	static uint8_t getFlashDumpFail(void);
	static uint8_t getFlashDumpProgress(void);
	static uint8_t getFlashReadStep(void);
	static uint8_t getSysOperation(void);
	static uint8_t getFlashDumpSector(void);
	static uint8_t getFlashDumpPage(void);

	static void setFlashCommand(uint8_t command);
	static void setFlashReadStep(uint8_t step);
	static void setFlashDumpComplete(uint8_t complete);
	static void setFlashDumpFail(uint8_t fail);
	static void setFlashDumpProgress(uint8_t progress);
	
	static void setFlashDumpSector(uint8_t sector);
	static void setFlashDumpPage(uint8_t page);
	static void setFlashDumpGoing(bool going);
#endif

#ifdef HX_TP_PROC_SELF_TEST
	#define HIMAX_PROC_SELF_TEST_FILE	"self_test"
#endif

#endif


//=============================================================================================================
//
//	Segment : Himax SYS Debug Function
//
//=============================================================================================================

static ssize_t himax_debug_level_read(struct file *file, char *buf,
	size_t len, loff_t *pos)
{
	struct himax_ts_data *ts_data;
	ssize_t ret = 0;

	ts_data = private_ts;
	if(!HX_PROC_SEND_FLAG)
		{
			ret += sprintf(buf, "%d\n", ts_data->debug_log_level);
			HX_PROC_SEND_FLAG=1;
		}
	else
		HX_PROC_SEND_FLAG=0;

	return ret;
}

static ssize_t himax_debug_level_write(struct file *file, const char *buff,
	size_t len, loff_t *pos)
{
	struct himax_ts_data *ts;
	char buf_tmp[12]= {0};
	int i;

	if (len >= 12)
	{
		tp_log_info("%s: no command exceeds 12 chars.\n", __func__);
		return -EFAULT;
	}
	if (copy_from_user(buf_tmp, buff, len))
	{
		return -EFAULT;
	}
	ts = private_ts;

	ts->debug_log_level = 0;
	for(i=0; i<len-1; i++)
	{
		if( buf_tmp[i]>='0' && buf_tmp[i]<='9' )
			ts->debug_log_level |= (buf_tmp[i]-'0');
		else if( buf_tmp[i]>='A' && buf_tmp[i]<='F' )
			ts->debug_log_level |= (buf_tmp[i]-'A'+10);
		else if( buf_tmp[i]>='a' && buf_tmp[i]<='f' )
			ts->debug_log_level |= (buf_tmp[i]-'a'+10);

		if(i!=len-2)
			ts->debug_log_level <<= 4;
	}

	if (ts->debug_log_level & BIT(3)) {
		if (ts->pdata->screenWidth > 0 && ts->pdata->screenHeight > 0 &&
		 (ts->pdata->abs_x_max - ts->pdata->abs_x_min) > 0 &&
		 (ts->pdata->abs_y_max - ts->pdata->abs_y_min) > 0) {
			ts->widthFactor = (ts->pdata->screenWidth << SHIFTBITS)/(ts->pdata->abs_x_max - ts->pdata->abs_x_min);
			ts->heightFactor = (ts->pdata->screenHeight << SHIFTBITS)/(ts->pdata->abs_y_max - ts->pdata->abs_y_min);
			if (ts->widthFactor > 0 && ts->heightFactor > 0)
				ts->useScreenRes = 1;
			else {
				ts->heightFactor = 0;
				ts->widthFactor = 0;
				ts->useScreenRes = 0;
			}
		} else
			tp_log_info("Enable finger debug with raw position mode!\n");
	} else {
		ts->useScreenRes = 0;
		ts->widthFactor = 0;
		ts->heightFactor = 0;
	}

	return len;
}

static struct file_operations himax_proc_debug_level_ops =
{
	.owner = THIS_MODULE,
	.read = himax_debug_level_read,
	.write = himax_debug_level_write,
};
static ssize_t himax_vendor_read(struct file *file, char *buf,
	size_t len, loff_t *pos)
{
	ssize_t ret = 0;
	struct himax_ts_data *ts_data;
	ts_data = private_ts;
	if(!HX_PROC_SEND_FLAG)
		{
			ret += sprintf(buf, "%s_FW:%#x,%x_CFG:%#x_SensorId:%#x\n", HIMAX852xf_NAME,
				ts_data->vendor_fw_ver_H, ts_data->vendor_fw_ver_L, ts_data->vendor_config_ver, ts_data->vendor_sensor_id);
			HX_PROC_SEND_FLAG=1;
		}
	else
		HX_PROC_SEND_FLAG=0;

	return ret;
}

static struct file_operations himax_proc_vendor_ops =
{
	.owner = THIS_MODULE,
	.read = himax_vendor_read,
};
static ssize_t himax_attn_read(struct file *file, char *buf,
	size_t len, loff_t *pos)
{
	ssize_t ret = 0;
	struct himax_ts_data *ts_data;
	ts_data = private_ts;

	if(!HX_PROC_SEND_FLAG)
		{
			sprintf(buf, "attn = %x\n", himax_int_gpio_read(ts_data->pdata->gpio_irq));
			ret = strlen(buf) + 1;
			HX_PROC_SEND_FLAG=1;
		}
	else
		HX_PROC_SEND_FLAG=0;

	return ret;
}

static struct file_operations himax_proc_attn_ops =
{
	.owner = THIS_MODULE,
	.read = himax_attn_read,
};
static ssize_t himax_int_en_read(struct file *file, char *buf,
	size_t len, loff_t *pos)
{
	struct himax_ts_data *ts = private_ts;
	size_t ret = 0;

	if(!HX_PROC_SEND_FLAG)
		{
			ret += sprintf(buf + ret, "%d ", ts->irq_enabled);
			ret += sprintf(buf + ret, "\n");
			HX_PROC_SEND_FLAG=1;
		}
		else
			HX_PROC_SEND_FLAG=0;

	return ret;
}

static ssize_t himax_int_en_write(struct file *file, const char *buff,
	size_t len, loff_t *pos)
{
	struct himax_ts_data *ts = private_ts;
	char buf_tmp[12]= {0};
	int value, ret=0;

	if (len >= 12)
	{
		tp_log_info("%s: no command exceeds 12 chars.\n", __func__);
		return -EFAULT;
	}
	if (copy_from_user(buf_tmp, buff, len))
	{
		return -EFAULT;
	}

	if (buf_tmp[0] == '0')
		value = false;
	else if (buf_tmp[0] == '1')
		value = true;
	else
		return -EINVAL;
	if (value) {
		ret = request_threaded_irq(ts->client->irq, NULL, himax_ts_thread,
			IRQF_TRIGGER_LOW | IRQF_ONESHOT, ts->client->name, ts);
		if (ret == 0) {
			ts->irq_enabled = 1;
			irq_enable_count = 1;
		}
	} else {
		himax_int_enable(ts->client->irq,0);
		free_irq(ts->client->irq, ts);
		ts->irq_enabled = 0;
	}

	return len;
}

static struct file_operations himax_proc_int_en_ops =
{
	.owner = THIS_MODULE,
	.read = himax_int_en_read,
	.write = himax_int_en_write,
};
static ssize_t himax_layout_read(struct file *file, char *buf,
	size_t len, loff_t *pos)
{
	struct himax_ts_data *ts = private_ts;
	size_t ret = 0;

	if(!HX_PROC_SEND_FLAG)
		{
			ret += sprintf(buf + ret, "%d ", ts->pdata->abs_x_min);
			ret += sprintf(buf + ret, "%d ", ts->pdata->abs_x_max);
			ret += sprintf(buf + ret, "%d ", ts->pdata->abs_y_min);
			ret += sprintf(buf + ret, "%d ", ts->pdata->abs_y_max);
			ret += sprintf(buf + ret, "\n");
			HX_PROC_SEND_FLAG=1;
		}
	else
		HX_PROC_SEND_FLAG=0;

	return ret;
}

static ssize_t himax_layout_write(struct file *file, const char *buff,
	size_t len, loff_t *pos)
{
	struct himax_ts_data *ts = private_ts;
	char buf_tmp[5];
	int i = 0, j = 0, k = 0, ret;
	unsigned long value;
	int layout[4] = {0};
	char buf[80] = {0};

	if (len >= 80)
	{
		tp_log_info("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}
	if (copy_from_user(buf, buff, len))
	{
		return -EFAULT;
	}

	for (i = 0; i < 20; i++) {
		if (buf[i] == ',' || buf[i] == '\n') {
			memset(buf_tmp, 0x0, sizeof(buf_tmp));
			if (i - j <= 5)
				memcpy(buf_tmp, buf + j, i - j);
			else {
				tp_log_info("buffer size is over 5 char\n");
				return len;
			}
			j = i + 1;
			if (k < 4) {
				ret = strict_strtol(buf_tmp, 10, &value);
				layout[k++] = value;
			}
		}
	}
	if (k == 4) {
		ts->pdata->abs_x_min=layout[0];
		ts->pdata->abs_x_max=layout[1];
		ts->pdata->abs_y_min=layout[2];
		ts->pdata->abs_y_max=layout[3];
		tp_log_info("%d, %d, %d, %d\n",
			ts->pdata->abs_x_min, ts->pdata->abs_x_max, ts->pdata->abs_y_min, ts->pdata->abs_y_max);
		input_unregister_device(ts->input_dev);
		himax_input_register(ts);
	} else
		tp_log_info("ERR@%d, %d, %d, %d\n",
			ts->pdata->abs_x_min, ts->pdata->abs_x_max, ts->pdata->abs_y_min, ts->pdata->abs_y_max);
	return len;
}

static struct file_operations himax_proc_layout_ops =
{
	.owner = THIS_MODULE,
	.read = himax_layout_read,
	.write = himax_layout_write,
};
#ifdef HX_TP_PROC_RESET
static ssize_t himax_reset_write(struct file *file, const char *buff,
	size_t len, loff_t *pos)
{
	char buf_tmp[12];

	if (len >= 12)
	{
		tp_log_info("%s: no command exceeds 12 chars.\n", __func__);
		return -EFAULT;
	}
	if (copy_from_user(buf_tmp, buff, len))
	{
		return -EFAULT;
	}

	if (buf_tmp[0] == '1')
		himax_HW_reset(true,false);
	return len;
}

static struct file_operations himax_proc_reset_ops =
{
	.owner = THIS_MODULE,
	.write = himax_reset_write,
};
#endif
#ifdef HX_TP_PROC_DIAG
uint8_t *getMutualBuffer(void)
{
	return diag_mutual;
}
uint8_t *getSelfBuffer(void)
{
	return &diag_self[0];
}
uint8_t getXChannel(void)
{
	return x_channel;
}
uint8_t getYChannel(void)
{
	return y_channel;
}
uint8_t getDiagCommand(void)
{
	return diag_command;
}
void setDiagCommand(uint8_t cmd)
{
	diag_command = cmd;
}
void setXChannel(uint8_t x)
{
	x_channel = x;
}
void setYChannel(uint8_t y)
{
	y_channel = y;
}
void setMutualBuffer(void)
{
	diag_mutual = kzalloc(x_channel * y_channel * sizeof(uint8_t), GFP_KERNEL);
}

static void *himax_diag_seq_start(struct seq_file *s, loff_t *pos)
{
	if (*pos>=1) return NULL;
	return (void *)((unsigned long) *pos+1);
}

static void *himax_diag_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	return NULL;
}
static void himax_diag_seq_stop(struct seq_file *s, void *v)
{
}
static int himax_diag_seq_read(struct seq_file *s, void *v)
{
	size_t count = 0;
	uint32_t loop_i;
	uint16_t mutual_num, self_num, width;

	mutual_num	= x_channel * y_channel;
	self_num	= x_channel + y_channel; //don't add KEY_COUNT
	width		= x_channel;
	seq_printf(s, "ChannelStart: %4d, %4d\n\n", x_channel, y_channel);

	// start to show out the raw data in adb shell
	if (diag_command >= 1 && diag_command <= 6) {
		if (diag_command <= 3) {
			for (loop_i = 0; loop_i < mutual_num; loop_i++) {
				seq_printf(s, "%4d", diag_mutual[loop_i]);
				if ((loop_i % width) == (width - 1))
					seq_printf(s, " %3d\n", diag_self[width + loop_i/width]);
			}
			seq_printf(s, "\n");
			for (loop_i = 0; loop_i < width; loop_i++) {
				seq_printf(s, "%4d", diag_self[loop_i]);
				if (((loop_i) % width) == (width - 1))
					seq_printf(s, "\n");
			}
		} else if (diag_command > 4) {
			for (loop_i = 0; loop_i < self_num; loop_i++) {
				seq_printf(s, "%4d", diag_self[loop_i]);
				if (((loop_i - mutual_num) % width) == (width - 1))
					seq_printf(s, "\n");
			}
		} else {
			for (loop_i = 0; loop_i < mutual_num; loop_i++) {
				seq_printf(s, "%4d", diag_mutual[loop_i]);
				if ((loop_i % width) == (width - 1))
					seq_printf(s, "\n");
			}
		}
		seq_printf(s, "ChannelEnd");
		seq_printf(s, "\n");
	} else if (diag_command == 7) {
		for (loop_i = 0; loop_i < 128 ;loop_i++) {
			if ((loop_i % 16) == 0)
				seq_printf(s, "LineStart:");
				seq_printf(s, "%4d", diag_coor[loop_i]);
			if ((loop_i % 16) == 15)
				seq_printf(s, "\n");
		}
	}
	return count;
}

static struct seq_operations himax_diag_seq_ops =
{
	.start	= himax_diag_seq_start,
	.next	= himax_diag_seq_next,
	.stop	= himax_diag_seq_stop,
	.show	= himax_diag_seq_read,
};
static int himax_diag_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &himax_diag_seq_ops);
};
static ssize_t himax_diag_write(struct file *filp, const char __user *buff, size_t len, loff_t *data)
{
	const uint8_t command_ec_128_raw_flag 		= 0x02;
	const uint8_t command_ec_24_normal_flag 	= 0x00;
	uint8_t command_ec_128_raw_baseline_flag 	= 0x01;
	uint8_t command_ec_128_raw_bank_flag 		= 0x03;
	uint8_t command_F1h[2] = {0xF1, 0x00};
	char messages[80] = {0};

	if (len >= 80)
	{
		tp_log_info("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}
	if (copy_from_user(messages, buff, len))
	{
		return -EFAULT;
	}

	diag_command = messages[0] - '0';

	tp_log_info("[Himax]diag_command=0x%x\n",diag_command);
	if (diag_command == 0x01)	{//DC
		command_F1h[1] = command_ec_128_raw_baseline_flag;
		tp_log_err("[Himax]diag_command=0x%x, data = %d\n",command_F1h[0], command_F1h[1]);
		i2c_himax_write(private_ts->client, command_F1h[0] ,&command_F1h[1], 1, DEFAULT_RETRY_CNT);
	} else if (diag_command == 0x02) {//IIR
		command_F1h[1] = command_ec_128_raw_flag;
		i2c_himax_write(private_ts->client, command_F1h[0] ,&command_F1h[1], 1, DEFAULT_RETRY_CNT);
	} else if (diag_command == 0x03) {	//BANK
		command_F1h[1] = command_ec_128_raw_bank_flag;	//0x03
		i2c_himax_write(private_ts->client, command_F1h[0] ,&command_F1h[1], 1, DEFAULT_RETRY_CNT);
	} else if (diag_command == 0x04 ) { // 2T3R IIR
		command_F1h[1] = 0x04; //2T3R IIR
		i2c_himax_write(private_ts->client, command_F1h[0] ,&command_F1h[1], 1, DEFAULT_RETRY_CNT);
	} else if (diag_command == 0x00) {//Disable
		command_F1h[1] = command_ec_24_normal_flag;
		i2c_himax_write(private_ts->client, command_F1h[0] ,&command_F1h[1], 1, DEFAULT_RETRY_CNT);
		touch_monitor_stop_flag = touch_monitor_stop_limit;
	}
	else{
		tp_log_err("[Himax]Diag command error!diag_command=0x%x\n",diag_command);
	}
	return len;
}

static struct file_operations himax_proc_diag_ops =
{
	.owner = THIS_MODULE,
	.open = himax_diag_proc_open,
	.read = seq_read,
	.write = himax_diag_write,
};
#endif

#ifdef HX_TP_PROC_REGISTER
static ssize_t himax_register_read(struct file *file, char *buf,size_t len, loff_t *pos)
{
	int ret = 0;
	int base = 0;
	uint16_t loop_i,loop_j;
	uint8_t inData[128];
	uint8_t outData[5];

	memset(outData, 0x00, sizeof(outData));
	memset(inData, 0x00, sizeof(inData));

	tp_log_info("Himax multi_register_command = %d \n",multi_register_command);
	if(!HX_PROC_SEND_FLAG)
	{
		if (multi_register_command == 1) {
			base = 0;

			for(loop_i = 0; loop_i < 6; loop_i++) {
				if (multi_register[loop_i] != 0x00) {
					if (multi_cfg_bank[loop_i] == 1) {//config bank register
				              if(IC_TYPE == HX_85XX_F_SERIES_PWON) {
              					outData[0] = 0x11;
              				} else {
                					outData[0] = 0x14;
						}
						i2c_himax_write(private_ts->client, 0x8C ,&outData[0], 1, DEFAULT_RETRY_CNT);
						msleep(10);

						outData[0] = 0x00;
						outData[1] = multi_register[loop_i];
						i2c_himax_write(private_ts->client, 0x8B ,&outData[0], 2, DEFAULT_RETRY_CNT);
						msleep(10);

						i2c_himax_read(private_ts->client, 0x5A, inData, 128, DEFAULT_RETRY_CNT);

						outData[0] = 0x00;
						i2c_himax_write(private_ts->client, 0x8C ,&outData[0], 1, DEFAULT_RETRY_CNT);

						for(loop_j=0; loop_j<128; loop_j++)
							multi_value[base++] = inData[loop_j];
					} else {//normal register
						i2c_himax_read(private_ts->client, multi_register[loop_i], inData, 128, DEFAULT_RETRY_CNT);

						for(loop_j=0; loop_j<128; loop_j++)
							multi_value[base++] = inData[loop_j];
					}
				}
			}

			base = 0;
			for(loop_i = 0; loop_i < 6; loop_i++) {
				if (multi_register[loop_i] != 0x00) {
					if (multi_cfg_bank[loop_i] == 1)
						ret += sprintf(buf + ret, "Register: FE(%x)\n", multi_register[loop_i]);
					else
						ret += sprintf(buf + ret, "Register: %x\n", multi_register[loop_i]);

					for (loop_j = 0; loop_j < 128; loop_j++) {
						ret += sprintf(buf + ret, "0x%2.2X ", multi_value[base++]);
						if ((loop_j % 16) == 15)
							ret += sprintf(buf + ret, "\n");
					}
				}
			}
			return ret;
			}

			if (config_bank_reg) {
				tp_log_info("%s: register_command = FE(%x)\n", __func__, register_command);

				//Config bank register read flow.

		              if(IC_TYPE == HX_85XX_F_SERIES_PWON) {
					outData[0] = 0x11;
              		} else {
                			outData[0] = 0x14;
				}
				i2c_himax_write(private_ts->client, 0x8C,&outData[0], 1, DEFAULT_RETRY_CNT);

				msleep(10);

				outData[0] = 0x00;
				outData[1] = register_command;
				i2c_himax_write(private_ts->client, 0x8B,&outData[0], 2, DEFAULT_RETRY_CNT);
				msleep(10);

				i2c_himax_read(private_ts->client, 0x5A, inData, 128, DEFAULT_RETRY_CNT);
				msleep(10);

				outData[0] = 0x00;
				i2c_himax_write(private_ts->client, 0x8C,&outData[0], 1, DEFAULT_RETRY_CNT);
			} else {
				if (i2c_himax_read(private_ts->client, register_command, inData, 128, DEFAULT_RETRY_CNT) < 0)
					return ret;
			}

			if (config_bank_reg)
				ret += sprintf(buf, "command: FE(%x)\n", register_command);
			else
				ret += sprintf(buf, "command: %x\n", register_command);

			for (loop_i = 0; loop_i < 128; loop_i++) {
				ret += sprintf(buf + ret, "0x%2.2X ", inData[loop_i]);
				if ((loop_i % 16) == 15)
					ret += sprintf(buf + ret, "\n");
			}
			ret += sprintf(buf + ret, "\n");
			HX_PROC_SEND_FLAG=1;
		}
	else
		HX_PROC_SEND_FLAG=0;

	return ret;
}

static ssize_t himax_register_write(struct file *file, const char *buff,
	size_t len, loff_t *pos)
{
	char buf_tmp[6], length = 0;
	unsigned long result    = 0;
	uint8_t loop_i          = 0;
	uint16_t base           = 5;
	uint8_t write_da[128];
	uint8_t outData[5];
	char buf[80] = {0};

	if (len >= 80)
	{
		tp_log_info("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}
	if (copy_from_user(buf, buff, len))
	{
		return -EFAULT;
	}

	memset(buf_tmp, 0x0, sizeof(buf_tmp));
	memset(write_da, 0x0, sizeof(write_da));
	memset(outData, 0x0, sizeof(outData));

	tp_log_info("himax %s \n",buf);

	if (buf[0] == 'm' && buf[1] == 'r' && buf[2] == ':') {
		memset(multi_register, 0x00, sizeof(multi_register));
		memset(multi_cfg_bank, 0x00, sizeof(multi_cfg_bank));
		memset(multi_value, 0x00, sizeof(multi_value));

		tp_log_info("himax multi register enter\n");

		multi_register_command = 1;

		base 	= 2;
		loop_i 	= 0;

		while(true) {
			if (buf[base] == '\n')
				break;

			if (loop_i >= 6 )
				break;

			if (buf[base] == ':' && buf[base+1] == 'x' && buf[base+2] == 'F' &&
					buf[base+3] == 'E' && buf[base+4] != ':') {
				memcpy(buf_tmp, buf + base + 4, 2);
				if (!strict_strtoul(buf_tmp, 16, &result)) {
					multi_register[loop_i] = result;
					multi_cfg_bank[loop_i++] = 1;
				}
				base += 6;
			} else {
				memcpy(buf_tmp, buf + base + 2, 2);
				if (!strict_strtoul(buf_tmp, 16, &result)) {
					multi_register[loop_i] = result;
					multi_cfg_bank[loop_i++] = 0;
				}
				base += 4;
			}
		}

		tp_log_info("========================== \n");
		for(loop_i = 0; loop_i < 6; loop_i++)
			tp_log_info("%d,%d:",multi_register[loop_i],multi_cfg_bank[loop_i]);
		tp_log_info("\n");
	} else if ((buf[0] == 'r' || buf[0] == 'w') && buf[1] == ':') {
		multi_register_command = 0;

		if (buf[2] == 'x') {
			if (buf[3] == 'F' && buf[4] == 'E') {//Config bank register
				config_bank_reg = true;

				memcpy(buf_tmp, buf + 5, 2);
				if (!strict_strtoul(buf_tmp, 16, &result))
					register_command = result;
				base = 7;

				tp_log_info("CMD: FE(%x)\n", register_command);
			} else {
				config_bank_reg = false;

				memcpy(buf_tmp, buf + 3, 2);
				if (!strict_strtoul(buf_tmp, 16, &result))
					register_command = result;
				base = 5;
				tp_log_info("CMD: %x\n", register_command);
			}

			for (loop_i = 0; loop_i < 128; loop_i++) {
				if (buf[base] == '\n') {
					if (buf[0] == 'w') {
						if (config_bank_reg) {
							              if(IC_TYPE == HX_85XX_F_SERIES_PWON)
                {
              outData[0] = 0x11;
              }
              else
                {
                outData[0] = 0x14;

               }
							i2c_himax_write(private_ts->client, 0x8C, &outData[0], 1, DEFAULT_RETRY_CNT);
              msleep(10);

							outData[0] = 0x00;
							outData[1] = register_command;
							i2c_himax_write(private_ts->client, 0x8B, &outData[0], 2, DEFAULT_RETRY_CNT);

							msleep(10);
							i2c_himax_write(private_ts->client, 0x40, &write_da[0], length, DEFAULT_RETRY_CNT);

							msleep(10);
              outData[0] = 0x00;
							i2c_himax_write(private_ts->client, 0x8C, &outData[0], 1, DEFAULT_RETRY_CNT);

							tp_log_info("CMD: FE(%x), %x, %d\n", register_command,write_da[0], length);
						} else {
							i2c_himax_write(private_ts->client, register_command, &write_da[0], length, DEFAULT_RETRY_CNT);
							tp_log_info("CMD: %x, %x, %d\n", register_command,write_da[0], length);
						}
					}
					tp_log_info("\n");
					return len;
				}
				if (buf[base + 1] == 'x') {
					buf_tmp[4] = '\n';
					buf_tmp[5] = '\0';
					memcpy(buf_tmp, buf + base + 2, 2);
					if (!strict_strtoul(buf_tmp, 16, &result)) {
						write_da[loop_i] = result;
					}
					length++;
				}
				base += 4;
			}
		}
	}
	return len;
}

static struct file_operations himax_proc_register_ops =
{
	.owner = THIS_MODULE,
	.read = himax_register_read,
	.write = himax_register_write,
};
#endif
#ifdef HX_TP_PROC_DEBUG
static ssize_t himax_debug_read(struct file *file, char *buf,
	size_t len, loff_t *pos)
{
	size_t ret = 0;

	if(!HX_PROC_SEND_FLAG)
	{
		if (debug_level_cmd == 't')
		{
			if (fw_update_complete)
			{
				ret += sprintf(buf, "FW Update Complete ");
			}
			else
			{
				ret += sprintf(buf, "FW Update Fail ");
			}
		}
		else if (debug_level_cmd == 'h')
		{
			if (handshaking_result == 0)
			{
				ret += sprintf(buf, "Handshaking Result = %d (MCU Running)\n",handshaking_result);
			}
			else if (handshaking_result == 1)
			{
				ret += sprintf(buf, "Handshaking Result = %d (MCU Stop)\n",handshaking_result);
			}
			else if (handshaking_result == 2)
			{
				ret += sprintf(buf, "Handshaking Result = %d (I2C Error)\n",handshaking_result);
			}
			else
			{
				ret += sprintf(buf, "Handshaking Result = error \n");
			}
		}
		else if (debug_level_cmd == 'v')
		{
			ret += sprintf(buf + ret, "FW_VER = ");
	           ret += sprintf(buf + ret, "0x%2.2X, %2.2X \n",private_ts->vendor_fw_ver_H,private_ts->vendor_fw_ver_L);

			ret += sprintf(buf + ret, "CONFIG_VER = ");
	           ret += sprintf(buf + ret, "0x%2.2X \n",private_ts->vendor_config_ver);
			ret += sprintf(buf + ret, "\n");
		}
		else if (debug_level_cmd == 'd')
		{
			ret += sprintf(buf + ret, "Himax Touch IC Information :\n");
			if (IC_TYPE == HX_85XX_D_SERIES_PWON)
			{
				ret += sprintf(buf + ret, "IC Type : D\n");
			}
			else if (IC_TYPE == HX_85XX_E_SERIES_PWON)
			{
				ret += sprintf(buf + ret, "IC Type : E\n");
			}
			else if (IC_TYPE == HX_85XX_ES_SERIES_PWON)
			{
				ret += sprintf(buf + ret, "IC Type : ES\n");
			}
			else if (IC_TYPE == HX_85XX_F_SERIES_PWON)
			{
				ret += sprintf(buf + ret, "IC Type : F\n");
			}
			else
			{
				ret += sprintf(buf + ret, "IC Type error.\n");
			}

			if (IC_CHECKSUM == HX_TP_BIN_CHECKSUM_SW)
			{
				ret += sprintf(buf + ret, "IC Checksum : SW\n");
			}
			else if (IC_CHECKSUM == HX_TP_BIN_CHECKSUM_HW)
			{
				ret += sprintf(buf + ret, "IC Checksum : HW\n");
			}
			else if (IC_CHECKSUM == HX_TP_BIN_CHECKSUM_CRC)
			{
				ret += sprintf(buf + ret, "IC Checksum : CRC\n");
			}
			else
			{
				ret += sprintf(buf + ret, "IC Checksum error.\n");
			}

			ret += sprintf(buf + ret, "Interrupt : LEVEL TRIGGER\n");
			ret += sprintf(buf + ret, "RX Num : %d\n",HX_RX_NUM);
			ret += sprintf(buf + ret, "TX Num : %d\n",HX_TX_NUM);
			ret += sprintf(buf + ret, "BT Num : %d\n",HX_BT_NUM);
			ret += sprintf(buf + ret, "X Resolution : %d\n",HX_X_RES);
			ret += sprintf(buf + ret, "Y Resolution : %d\n",HX_Y_RES);
			ret += sprintf(buf + ret, "Max Point : %d\n",HX_MAX_PT);
		}
		else if (debug_level_cmd == 'i')
		{
			ret += sprintf(buf + ret, "Himax Touch Driver Version:\n");
			ret += sprintf(buf + ret, "%s \n", HIMAX_DRIVER_VER);
		}
		HX_PROC_SEND_FLAG=1;
	}
	else
		HX_PROC_SEND_FLAG=0;
	return ret;
}

static u8 himax_read_FW_ver(bool hw_reset)
{
	uint8_t cmd[3];

	himax_int_enable(private_ts->client->irq,0);

	if (hw_reset) {
		himax_HW_reset(false,true);
	}

	msleep(120);
	if (i2c_himax_read(private_ts->client, HX_VER_FW_MAJ, cmd, 1, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return 0;
	}
	private_ts->vendor_fw_ver_H = cmd[0];
	if (i2c_himax_read(private_ts->client, HX_VER_FW_MIN, cmd, 1, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return 0;
	}
	private_ts->vendor_fw_ver_L = cmd[0];
	tp_log_info("FW_VER : %d,%d \n",private_ts->vendor_fw_ver_H,private_ts->vendor_fw_ver_L);

	if (i2c_himax_read(private_ts->client, HX_VER_FW_CFG, cmd, 1, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return 0;
	}
	private_ts->vendor_config_ver = cmd[0];
	tp_log_info("CFG_VER : %d \n",private_ts->vendor_config_ver);

	himax_HW_reset(true,true);

	himax_int_enable(private_ts->client->irq,1);

	return 0;
}


static ssize_t himax_debug_write(struct file *file, const char *buff,
	size_t len, loff_t *pos)
{
	struct file* hx_filp = NULL;
	mm_segment_t oldfs;
	int result = 0;
	char fileName[128];
	char buf[80] = {0};

	if (len >= 80)
	{
		tp_log_info("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}
	if (copy_from_user(buf, buff, len))
	{
		return -EFAULT;
	}

	if ( buf[0] == 'h') //handshaking
	{
		debug_level_cmd = buf[0];

		himax_int_enable(private_ts->client->irq,0);

		handshaking_result = himax_hand_shaking(); //0:Running, 1:Stop, 2:I2C Fail

		himax_int_enable(private_ts->client->irq,1);

		return len;
	}

	else if ( buf[0] == 'v') //firmware version
	{
		debug_level_cmd = buf[0];
		himax_read_FW_ver(true);
		return len;
	}

	else if ( buf[0] == 'd') //test
	{
		debug_level_cmd = buf[0];
		return len;
	}

	else if ( buf[0] == 'i') //driver version
	{
		debug_level_cmd = buf[0];
		return len;
	}

	else if (buf[0] == 't')
	{
		himax_int_enable(private_ts->client->irq,0);
		wake_lock(&private_ts->ts_flash_wake_lock);
#ifdef HX_CHIP_STATUS_MONITOR
		HX_CHIP_POLLING_COUNT = 0;
		cancel_delayed_work_sync(&private_ts->himax_chip_monitor);
#endif

		debug_level_cmd 		= buf[0];
		fw_update_complete		= false;

		memset(fileName, 0, 128);
		// parse the file name
		snprintf(fileName, len-2, "%s", &buf[2]);
		tp_log_info("%s: upgrade from file(%s) start!\n", __func__, fileName);
		// open file
		hx_filp = filp_open(fileName, O_RDONLY, 0);
		if (IS_ERR(hx_filp))
		{
			tp_log_err("%s: open firmware file failed\n", __func__);
			goto firmware_upgrade_done;
			//return len;
		}
		oldfs = get_fs();
		set_fs(get_ds());

		// read the latest firmware binary file
		result=hx_filp->f_op->read(hx_filp,upgrade_fw,sizeof(upgrade_fw), &hx_filp->f_pos);
		if (result < 0)
		{
			tp_log_err("%s: read firmware file failed\n", __func__);
			set_fs(oldfs);
			goto firmware_upgrade_done;
			//return len;
		}

		set_fs(oldfs);
		filp_close(hx_filp, NULL);

		tp_log_info("%s: upgrade start,len %d: %02X, %02X, %02X, %02X\n", __func__, result, upgrade_fw[0], upgrade_fw[1], upgrade_fw[2], upgrade_fw[3]);

		if (result > 0)
		{
			// start to upgrade
			//himax_int_enable(private_ts->client->irq,0);

			himax_HW_reset(false,true);

			if (fts_ctpm_fw_upgrade_with_fs(upgrade_fw, result, true) == 0)
			{
				tp_log_err("%s: TP upgrade error, line: %d\n", __func__, __LINE__);
				fw_update_complete = false;
			}
			else
			{
				tp_log_info("%s: TP upgrade OK, line: %d\n", __func__, __LINE__);
				fw_update_complete = true;

				private_ts->vendor_fw_ver_H = upgrade_fw[FW_VER_MAJ_FLASH_ADDR];
				private_ts->vendor_fw_ver_L = upgrade_fw[FW_VER_MIN_FLASH_ADDR];
				private_ts->vendor_config_ver = upgrade_fw[FW_CFG_VER_FLASH_ADDR];
			}
			//himax_int_enable(private_ts->client->irq,1);
			himax_HW_reset(true,true);
			goto firmware_upgrade_done;
			//return len;
		}
	}

	firmware_upgrade_done:

	wake_unlock(&private_ts->ts_flash_wake_lock);
	himax_int_enable(private_ts->client->irq,1);
#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	queue_delayed_work(private_ts->himax_chip_monitor_wq, &private_ts->himax_chip_monitor, HX_POLLING_TIMES*HZ);
#endif
	//todo himax_chip->tp_firmware_upgrade_proceed = 0;
	//todo himax_chip->suspend_state = 0;
	//todo enable_irq(himax_chip->irq);
	return len;
}

static struct file_operations himax_proc_debug_ops =
{
	.owner = THIS_MODULE,
	.read = himax_debug_read,
	.write = himax_debug_write,
};
#endif

#ifdef HX_TP_PROC_FLASH_DUMP

static uint8_t getFlashCommand(void)
{
	return flash_command;
}

static uint8_t getFlashDumpProgress(void)
{
	return flash_progress;
}

static uint8_t getFlashDumpComplete(void)
{
	return flash_dump_complete;
}

static uint8_t getFlashDumpFail(void)
{
	return flash_dump_fail;
}

static uint8_t getSysOperation(void)
{
	return sys_operation;
}

static uint8_t getFlashReadStep(void)
{
	return flash_read_step;
}

static uint8_t getFlashDumpSector(void)
{
	return flash_dump_sector;
}

static uint8_t getFlashDumpPage(void)
{
	return flash_dump_page;
}

bool getFlashDumpGoing(void)
{
	return flash_dump_going;
}

void setFlashBuffer(void)
{
	//int i=0;
	flash_buffer = kzalloc(FLASH_SIZE * sizeof(uint8_t), GFP_KERNEL);
	memset(flash_buffer,0x00,FLASH_SIZE);
	//for(i=0; i<FLASH_SIZE; i++)
	//{
	//	flash_buffer[i] = 0x00;
	//}
}

void setSysOperation(uint8_t operation)
{
	sys_operation = operation;
}

static void setFlashDumpProgress(uint8_t progress)
{
	flash_progress = progress;
	//tp_log_info("setFlashDumpProgress : progress = %d ,flash_progress = %d \n",progress,flash_progress);
}

static void setFlashDumpComplete(uint8_t status)
{
	flash_dump_complete = status;
}

static void setFlashDumpFail(uint8_t fail)
{
	flash_dump_fail = fail;
}

static void setFlashCommand(uint8_t command)
{
	flash_command = command;
}

static void setFlashReadStep(uint8_t step)
{
	flash_read_step = step;
}

static void setFlashDumpSector(uint8_t sector)
{
	flash_dump_sector = sector;
}

static void setFlashDumpPage(uint8_t page)
{
	flash_dump_page = page;
}

static void setFlashDumpGoing(bool going)
{
	flash_dump_going = going;
}

void himax_ts_flash_work_func(struct work_struct *work)
{
	struct himax_ts_data *ts = container_of(work, struct himax_ts_data, flash_work);

	uint8_t page_tmp[128];
	uint8_t x59_tmp[4] = {0,0,0,0};
	int i=0, j=0, k=0, l=0,/* j_limit = 0,*/ buffer_ptr = 0/*, flash_end_count = 0*/;
	uint8_t local_flash_command = 0;
	uint8_t sector = 0;
	uint8_t page = 0;

	/*uint8_t xAA_command[2] = {0xAA,0x00};*/
	uint8_t x81_command[2] = {0x81,0x00};
	uint8_t x82_command[2] = {0x82,0x00};
	uint8_t x42_command[2] = {0x42,0x00};
	uint8_t x43_command[4] = {0x43,0x00,0x00,0x00};
	uint8_t x44_command[4] = {0x44,0x00,0x00,0x00};
	uint8_t x45_command[5] = {0x45,0x00,0x00,0x00,0x00};
	uint8_t x46_command[2] = {0x46,0x00};
	/*uint8_t x4A_command[2] = {0x4A,0x00};*/
	uint8_t x4D_command[2] = {0x4D,0x00};
	uint8_t x5B_command[2] = {0x5B,0x00};
	uint8_t x5C_command[2] = {0x5C,0x00};

	himax_int_enable(private_ts->client->irq,0);
#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	cancel_delayed_work_sync(&private_ts->himax_chip_monitor);
#endif

	setFlashDumpGoing(true);

	sector = getFlashDumpSector();
	page = getFlashDumpPage();

	local_flash_command = getFlashCommand();

	if(local_flash_command<0x0F)
		himax_HW_reset(false,false);


	//if( i2c_himax_master_write(ts->client, xAA_command, 1, 3) < 0 )//sleep out
	//{
	//	tp_log_err("%s i2c write AA fail.\n",__func__);
	//	goto Flash_Dump_i2c_transfer_error;
	//}
	//msleep(120);

	if ( i2c_himax_master_write(ts->client, x81_command, 1, 3) < 0 )//sleep out
	{
		tp_log_err("%s i2c write 81 fail.\n",__func__);
		goto Flash_Dump_i2c_transfer_error;
	}
	msleep(120);

	if ( i2c_himax_master_write(ts->client, x82_command, 1, 3) < 0 )
	{
		tp_log_err("%s i2c write 82 fail.\n",__func__);
		goto Flash_Dump_i2c_transfer_error;
	}
	msleep(100);

	tp_log_info("%s: local_flash_command = %d enter.\n", __func__,local_flash_command);

	if ((local_flash_command == 1 || local_flash_command == 2)|| (local_flash_command==0x0F))
	{
		x43_command[1] = 0x01;
		if ( i2c_himax_write(ts->client, x43_command[0],&x43_command[1], 1, DEFAULT_RETRY_CNT) < 0)
		{
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(100);

		for( i=0 ; i<8 ;i++)
		{
			for(j=0 ; j<64 ; j++)
			{
				//tp_log_info(" Step 2 i=%d , j=%d %s\n",i,j,__func__);
				//read page start
				for(k=0; k<128; k++)
				{
					page_tmp[k] = 0x00;
				}
				for(k=0; k<32; k++)
				{
					x44_command[1] = k;
					x44_command[2] = j;
					x44_command[3] = i;
					if ( i2c_himax_write(ts->client, x44_command[0],&x44_command[1], 3, DEFAULT_RETRY_CNT) < 0 )
					{
						tp_log_err("%s i2c write 44 fail.\n",__func__);
						goto Flash_Dump_i2c_transfer_error;
					}

					if ( i2c_himax_write_command(ts->client, x46_command[0], DEFAULT_RETRY_CNT) < 0)
					{
						tp_log_err("%s i2c write 46 fail.\n",__func__);
						goto Flash_Dump_i2c_transfer_error;
					}
					//msleep(2);
					if ( i2c_himax_read(ts->client, 0x59, x59_tmp, 4, DEFAULT_RETRY_CNT) < 0)
					{
						tp_log_err("%s i2c write 59 fail.\n",__func__);
						goto Flash_Dump_i2c_transfer_error;
					}
					//msleep(2);
					for(l=0; l<4; l++)
					{
						page_tmp[k*4+l] = x59_tmp[l];
					}
					//msleep(10);
				}
				//read page end

				for(k=0; k<128; k++)
				{
					flash_buffer[buffer_ptr++] = page_tmp[k];

				}
				setFlashDumpProgress(i*32 + j);
			}
		}
	}
	else if (local_flash_command == 3)
	{
		x43_command[1] = 0x01;
		if ( i2c_himax_write(ts->client, x43_command[0],&x43_command[1], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(100);

		for(i=0; i<128; i++)
		{
			page_tmp[i] = 0x00;
		}

		for(i=0; i<64; i++)
		{
			x44_command[1] = i;
			x44_command[2] = page;
			x44_command[3] = sector;

			if ( i2c_himax_write(ts->client, x44_command[0],&x44_command[1], 3, DEFAULT_RETRY_CNT) < 0 )
			{
				tp_log_err("%s i2c write 44 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}

			if ( i2c_himax_write_command(ts->client, x46_command[0], DEFAULT_RETRY_CNT) < 0 )
			{
				tp_log_err("%s i2c write 46 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			//msleep(2);
			if ( i2c_himax_read(ts->client, 0x59, x59_tmp, 4, DEFAULT_RETRY_CNT) < 0 )
			{
				tp_log_err("%s i2c write 59 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			//msleep(2);
			for(j=0; j<4; j++)
			{
				page_tmp[i*4+j] = x59_tmp[j];
			}
			//msleep(10);
		}
		//read page end
		for(i=0; i<128; i++)
		{
			flash_buffer[buffer_ptr++] = page_tmp[i];
		}
	}
	else if (local_flash_command == 4)
	{
		//page write flow.
		//tp_log_info("%s: local_flash_command = 4, enter.\n", __func__);

		// unlock flash
		himax_lock_flash(0);

		msleep(50);

		// page erase
		x43_command[1] = 0x01;
		x43_command[2] = 0x00;
		x43_command[3] = 0x02;
		if ( i2c_himax_write(ts->client, x43_command[0],&x43_command[1], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write(ts->client, x5B_command[0],&x43_command[2], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write(ts->client, x5C_command[0],&x43_command[3], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 5C fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x44_command[1] = 0x00;
		x44_command[2] = page;
		x44_command[3] = sector;
		if ( i2c_himax_write(ts->client, x44_command[0],&x44_command[1], 3, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 44 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		if ( i2c_himax_write_command(ts->client, x4D_command[0], DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 4D fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(100);

		// enter manual mode

		x42_command[1] = 0x01;
		if( i2c_himax_write(ts->client, x42_command[0],&x42_command[1], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 35 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		msleep(100);

		// flash enable
		x43_command[1] = 0x01;
		x43_command[2] = 0x00;
		if ( i2c_himax_write(ts->client, x43_command[0],&x43_command[1], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write(ts->client, x5B_command[0],&x43_command[2], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		// set flash address
		x44_command[1] = 0x00;
		x44_command[2] = page;
		x44_command[3] = sector;
		if ( i2c_himax_write(ts->client, x44_command[0],&x44_command[1], 3, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 44 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		// manual mode command : 47 to latch the flash address when page address change.
		x43_command[1] = 0x01;
		x43_command[2] = 0x09;
		if ( i2c_himax_write(ts->client, x43_command[0],&x43_command[1], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write(ts->client, x5B_command[0],&x43_command[2], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x43_command[1] = 0x01;
		x43_command[2] = 0x0D;
		if ( i2c_himax_write(ts->client, x43_command[0],&x43_command[1], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write(ts->client, x5B_command[0],&x43_command[2], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x43_command[1] = 0x01;
		x43_command[2] = 0x09;
		if ( i2c_himax_write(ts->client, x43_command[0],&x43_command[1], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write(ts->client, x5B_command[0],&x43_command[2], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		for(i=0; i<64; i++)
		{
			tp_log_info("himax :i=%d \n",i);
			x44_command[1] = i;
			x44_command[2] = page;
			x44_command[3] = sector;
			if ( i2c_himax_write(ts->client, x44_command[0],&x44_command[1], 3, DEFAULT_RETRY_CNT) < 0 )
			{
				tp_log_err("%s i2c write 44 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(10);

			x45_command[1] = flash_buffer[i*4 + 0];
			x45_command[2] = flash_buffer[i*4 + 1];
			x45_command[3] = flash_buffer[i*4 + 2];
			x45_command[4] = flash_buffer[i*4 + 3];
			if ( i2c_himax_write(ts->client, x45_command[0],&x45_command[1], 4, DEFAULT_RETRY_CNT) < 0 )
			{
				tp_log_err("%s i2c write 45 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(10);

			// manual mode command : 48 ,data will be written into flash buffer
			x43_command[1] = 0x01;
			x43_command[2] = 0x0D;
			if ( i2c_himax_write(ts->client, x43_command[0],&x43_command[1], 1, DEFAULT_RETRY_CNT) < 0 )
			{
				tp_log_err("%s i2c write 43 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}

			if ( i2c_himax_write(ts->client, x5B_command[0],&x43_command[2], 1, DEFAULT_RETRY_CNT) < 0 )
			{
				tp_log_err("%s i2c write 5B fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(10);

			x43_command[1] = 0x01;
			x43_command[2] = 0x09;
			if ( i2c_himax_write(ts->client, x43_command[0],&x43_command[1], 1, DEFAULT_RETRY_CNT) < 0 )
			{
				tp_log_err("%s i2c write 43 fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}

			if ( i2c_himax_write(ts->client, x5B_command[0],&x43_command[2], 1, DEFAULT_RETRY_CNT) < 0 )
			{
				tp_log_err("%s i2c write 5B fail.\n",__func__);
				goto Flash_Dump_i2c_transfer_error;
			}
			msleep(10);
		}

		// manual mode command : 49 ,program data from flash buffer to this page
		x43_command[1] = 0x01;
		x43_command[2] = 0x01;
		if ( i2c_himax_write(ts->client, x43_command[0],&x43_command[1], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write(ts->client, x5B_command[0],&x43_command[2], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x43_command[1] = 0x01;
		x43_command[2] = 0x05;
		if ( i2c_himax_write(ts->client, x43_command[0],&x43_command[1], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write(ts->client, x5B_command[0],&x43_command[2], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x43_command[1] = 0x01;
		x43_command[2] = 0x01;
		if ( i2c_himax_write(ts->client, x43_command[0],&x43_command[1], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write(ts->client, x5B_command[0],&x43_command[2], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		x43_command[1] = 0x01;
		x43_command[2] = 0x00;
		if ( i2c_himax_write(ts->client, x43_command[0],&x43_command[1], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		if ( i2c_himax_write(ts->client, x5B_command[0],&x43_command[2], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 5B fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		// flash disable
		x43_command[1] = 0x00;
		if ( i2c_himax_write(ts->client, x43_command[0],&x43_command[1], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 43 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}
		msleep(10);

		// leave manual mode
		x42_command[1] = 0x01;
		if( i2c_himax_write(ts->client, x42_command[0],&x42_command[1], 1, DEFAULT_RETRY_CNT) < 0 )
		{
			tp_log_err("%s i2c write 35 fail.\n",__func__);
			goto Flash_Dump_i2c_transfer_error;
		}

		msleep(10);

		// lock flash
		himax_lock_flash(1);
		msleep(50);

		buffer_ptr = 128;
		tp_log_info("Himax: Flash page write Complete~~~~~~~~~~~~~~~~~~~~~~~\n");
	}

	tp_log_info("Complete~~~~~~~~~~~~~~~~~~~~~~~\n");
	if(local_flash_command==0x01)
		{
			tp_log_info(" buffer_ptr = %d \n",buffer_ptr);

			for (i = 0; i < buffer_ptr; i++)
			{
				tp_log_info("%2.2X ", flash_buffer[i]);

				if ((i % 16) == 15)
				{
					tp_log_info("\n");
				}
			}
			tp_log_info("End~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		}

	i2c_himax_master_write(ts->client, x43_command, 1, 3);
	msleep(50);

	if (local_flash_command == 2)
	{
		struct file *fn;

		fn = filp_open(FLASH_DUMP_FILE,O_CREAT | O_WRONLY ,0);
		if (!IS_ERR(fn))
		{
			fn->f_op->write(fn,flash_buffer,buffer_ptr*sizeof(uint8_t),&fn->f_pos);
			filp_close(fn,NULL);
		}
	}

	if(local_flash_command<0x0F)
		himax_HW_reset(true,false);

	himax_int_enable(private_ts->client->irq,1);
#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	queue_delayed_work(private_ts->himax_chip_monitor_wq, &private_ts->himax_chip_monitor, HX_POLLING_TIMES*HZ);
#endif
	setFlashDumpGoing(false);

	setFlashDumpComplete(1);
	setSysOperation(0);
	return;

	Flash_Dump_i2c_transfer_error:

	himax_HW_reset(true,false);

	himax_int_enable(private_ts->client->irq,1);
#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	queue_delayed_work(private_ts->himax_chip_monitor_wq, &private_ts->himax_chip_monitor, HX_POLLING_TIMES*HZ);
#endif
	setFlashDumpGoing(false);
	setFlashDumpComplete(0);
	setFlashDumpFail(1);
	setSysOperation(0);
	return;
}

static ssize_t himax_flash_read(struct file *file, char *buf,
	size_t len, loff_t *pos)
{
	int ret = 0;
	int loop_i;
	uint8_t local_flash_read_step=0;
	uint8_t local_flash_complete = 0;
	uint8_t local_flash_progress = 0;
	uint8_t local_flash_command = 0;
	uint8_t local_flash_fail = 0;

	local_flash_complete = getFlashDumpComplete();
	local_flash_progress = getFlashDumpProgress();
	local_flash_command = getFlashCommand();
	local_flash_fail = getFlashDumpFail();

	tp_log_info("flash_progress = %d \n",local_flash_progress);
	if(!HX_PROC_SEND_FLAG)
	{

		if (local_flash_fail)
		{
			ret += sprintf(buf + ret, "FlashStart:Fail \n");
			ret += sprintf(buf + ret, "FlashEnd");
			ret += sprintf(buf + ret, "\n");
			HX_PROC_SEND_FLAG=1;
			return ret;
		}

		if (!local_flash_complete)
		{
			ret += sprintf(buf + ret, "FlashStart:Ongoing:0x%2.2x \n",flash_progress);
			ret += sprintf(buf + ret, "FlashEnd");
			ret += sprintf(buf + ret, "\n");
			HX_PROC_SEND_FLAG=1;
			return ret;
		}

		if (local_flash_command == 1 && local_flash_complete)
		{
			ret += sprintf(buf + ret, "FlashStart:Complete \n");
			ret += sprintf(buf + ret, "FlashEnd");
			ret += sprintf(buf + ret, "\n");
			HX_PROC_SEND_FLAG=1;
			return ret;
		}

		if (local_flash_command == 3 && local_flash_complete)
		{
			ret += sprintf(buf + ret, "FlashStart: \n");
			for(loop_i = 0; loop_i < 128; loop_i++)
			{
				ret += sprintf(buf + ret, "x%2.2x", flash_buffer[loop_i]);
				if ((loop_i % 16) == 15)
				{
					ret += sprintf(buf + ret, "\n");
				}
			}
			ret += sprintf(buf + ret, "FlashEnd");
			ret += sprintf(buf + ret, "\n");
			HX_PROC_SEND_FLAG=1;
			return ret;
		}

		//flash command == 0 , report the data
		local_flash_read_step = getFlashReadStep();

		ret += sprintf(buf + ret, "FlashStart:%2.2x \n",local_flash_read_step);

		for (loop_i = 0; loop_i < 1024; loop_i++)
		{
			ret += sprintf(buf + ret, "x%2.2X", flash_buffer[local_flash_read_step*1024 + loop_i]);

			if ((loop_i % 16) == 15)
			{
				ret += sprintf(buf + ret, "\n");
			}
		}

		ret += sprintf(buf + ret, "FlashEnd");
		ret += sprintf(buf + ret, "\n");
		HX_PROC_SEND_FLAG=1;
	}
	else
		HX_PROC_SEND_FLAG=0;
	return ret;
}

static ssize_t himax_flash_write(struct file *file, const char *buff,
	size_t len, loff_t *pos)
{
	char buf_tmp[6];
	unsigned long result = 0;
	uint8_t loop_i = 0;
	int base = 0;
	char buf[80] = {0};

	if (len >= 80)
	{
		tp_log_info("%s: no command exceeds 80 chars.\n", __func__);
		return -EFAULT;
	}
	if (copy_from_user(buf, buff, len))
	{
		return -EFAULT;
	}
	memset(buf_tmp, 0x0, sizeof(buf_tmp));

	tp_log_info("%s: buf[0] = %s\n", __func__, buf);

	if (getSysOperation() == 1)
	{
		tp_log_err("%s: SYS is busy , return!\n", __func__);
		return len;
	}

	if (buf[0] == '0')
	{
		setFlashCommand(0);
		if (buf[1] == ':' && buf[2] == 'x')
		{
			memcpy(buf_tmp, buf + 3, 2);
			tp_log_info("%s: read_Step = %s\n", __func__, buf_tmp);
			if (!strict_strtoul(buf_tmp, 16, &result))
			{
				tp_log_info("%s: read_Step = %lu \n", __func__, result);
				setFlashReadStep(result);
			}
		}
	}
	else if (buf[0] == '1')
	{
		setSysOperation(1);
		setFlashCommand(1);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);
		queue_work(private_ts->flash_wq, &private_ts->flash_work);
	}
	else if (buf[0] == '2')
	{
		setSysOperation(1);
		setFlashCommand(2);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);

		queue_work(private_ts->flash_wq, &private_ts->flash_work);
	}
	else if (buf[0] == '3')
	{
		setSysOperation(1);
		setFlashCommand(3);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);

		memcpy(buf_tmp, buf + 3, 2);
		if (!strict_strtoul(buf_tmp, 16, &result))
		{
			setFlashDumpSector(result);
		}

		memcpy(buf_tmp, buf + 7, 2);
		if (!strict_strtoul(buf_tmp, 16, &result))
		{
			setFlashDumpPage(result);
		}

		queue_work(private_ts->flash_wq, &private_ts->flash_work);
	}
	else if (buf[0] == '4')
	{
		tp_log_info("%s: command 4 enter.\n", __func__);
		setSysOperation(1);
		setFlashCommand(4);
		setFlashDumpProgress(0);
		setFlashDumpComplete(0);
		setFlashDumpFail(0);

		memcpy(buf_tmp, buf + 3, 2);
		if (!strict_strtoul(buf_tmp, 16, &result))
		{
			setFlashDumpSector(result);
		}
		else
		{
			tp_log_err("%s: command 4 , sector error.\n", __func__);
			return len;
		}

		memcpy(buf_tmp, buf + 7, 2);
		if (!strict_strtoul(buf_tmp, 16, &result))
		{
			setFlashDumpPage(result);
		}
		else
		{
			tp_log_err("%s: command 4 , page error.\n", __func__);
			return len;
		}

		base = 11;

		tp_log_info("=========Himax flash page buffer start=========\n");
		for(loop_i=0;loop_i<128;loop_i++)
		{
			memcpy(buf_tmp, buf + base, 2);
			if (!strict_strtoul(buf_tmp, 16, &result))
			{
				flash_buffer[loop_i] = result;
				tp_log_info("%d ",flash_buffer[loop_i]);
				if (loop_i % 16 == 15)
				{
					tp_log_info("\n");
				}
			}
			base += 3;
		}
		tp_log_info("=========Himax flash page buffer end=========\n");

		queue_work(private_ts->flash_wq, &private_ts->flash_work);
	}
	return len;
}

static struct file_operations himax_proc_flash_ops =
{
	.owner = THIS_MODULE,
	.read = himax_flash_read,
	.write = himax_flash_write,
};
#endif
#ifdef HX_TP_PROC_SELF_TEST
int himax_chip_self_test(void)
{
	uint8_t cmdbuf[11];
	uint8_t valuebuf[16];
	int i=0, pf_value=0x03;

	memset(cmdbuf, 0x00, sizeof(cmdbuf));
	memset(valuebuf, 0x00, sizeof(valuebuf));

	i2c_himax_write(private_ts->client, 0x82,&cmdbuf[0], 0, DEFAULT_RETRY_CNT);//paco add 0321
	msleep(50);
	i2c_himax_write(private_ts->client, 0x80,&cmdbuf[0], 0, DEFAULT_RETRY_CNT);
	msleep(50);

	cmdbuf[0] = 0x00;
	i2c_himax_write(private_ts->client, 0x9B, &cmdbuf[0], 1, DEFAULT_RETRY_CNT);
	msleep(10);

	cmdbuf[0] = 0x11;
	i2c_himax_write(private_ts->client, 0x8C, &cmdbuf[0], 1, DEFAULT_RETRY_CNT);
	msleep(10);

	cmdbuf[0] = 0x00;
	cmdbuf[1] = 0x96;
	i2c_himax_write(private_ts->client, 0x8B, &cmdbuf[0], 2, DEFAULT_RETRY_CNT);
	msleep(10);

	valuebuf[0] = 0x00;
	valuebuf[1] = 0x00;
	valuebuf[2] = 0xfb;//bank mul
	valuebuf[3] = 0x01;
	valuebuf[4] = 0xfe;//bank average
	valuebuf[5] = 0x01;
	valuebuf[6] = 0xfb;//bank self
	valuebuf[7] = 0x05;
	i2c_himax_write(private_ts->client, 0x40, &valuebuf[0], 8, DEFAULT_RETRY_CNT);
	msleep(10);

	cmdbuf[0] = 0x00;
	i2c_himax_write(private_ts->client, 0x8C, &cmdbuf[0], 1, DEFAULT_RETRY_CNT);
	//tp_log_info("CMD: FE(%x), %x, %d\n", register_command,valuebuf[0], length);


	cmdbuf[0] = 0x06;
	i2c_himax_write(private_ts->client, 0xF1,&cmdbuf[0], 1, DEFAULT_RETRY_CNT);
	msleep(50);

	i2c_himax_write(private_ts->client, 0x83,&cmdbuf[0], 0, DEFAULT_RETRY_CNT);
	msleep(30);

	i2c_himax_write(private_ts->client, 0x81,&cmdbuf[0], 0, DEFAULT_RETRY_CNT);
	msleep(5000);

	i2c_himax_write(private_ts->client, 0x82,&cmdbuf[0], 0, DEFAULT_RETRY_CNT);
	msleep(50);

	i2c_himax_write(private_ts->client, 0x80,&cmdbuf[0], 0, DEFAULT_RETRY_CNT);
	msleep(30);

	cmdbuf[0] = 0x11;
	i2c_himax_write(private_ts->client, 0x8C ,&cmdbuf[0], 1, DEFAULT_RETRY_CNT);
	msleep(10);

	cmdbuf[0] = 0x00;
	cmdbuf[1] = 0x96;
	i2c_himax_write(private_ts->client, 0x8B ,&cmdbuf[0], 2, DEFAULT_RETRY_CNT);
	msleep(10);

	i2c_himax_read(private_ts->client, 0x5A, valuebuf, 8, DEFAULT_RETRY_CNT);
	msleep(10);

	cmdbuf[0] = 0x00;
	i2c_himax_write(private_ts->client, 0x8C ,&cmdbuf[0], 1, DEFAULT_RETRY_CNT);

	msleep(30);

	if (valuebuf[0]==0xAA) {
		tp_log_info("[Himax]: self-test pass\n");
		pf_value = 0x0;
	} else if((valuebuf[0] == 0xf1) || (valuebuf[0] == 0xf2) || (valuebuf[0] == 0xf3)){
		tp_log_err("[Himax]: self-test fail\n");
		pf_value = 0x1;
	}else{
		tp_log_err("[Himax]: self-test not completed\n");
	}

	for(i=0;i<8;i++) {
		tp_log_info("[Himax]: After self test buff_back FE[9%x]  = 0x%x\n",i+6,valuebuf[i]);
	}

	cmdbuf[0] = 0x02;
	i2c_himax_write(private_ts->client, 0x9B, &cmdbuf[0], 1, DEFAULT_RETRY_CNT);
	msleep(10);

	cmdbuf[0] = 0x00;
	i2c_himax_write(private_ts->client, 0xF1,&cmdbuf[0], 1, DEFAULT_RETRY_CNT);
	msleep(50);
	return pf_value;
}

static ssize_t himax_self_test_read(struct file *file, char *buf,
	size_t len, loff_t *pos)
{
	int val=0x00;
	int ret = 0;

	#ifdef HX_CHIP_STATUS_MONITOR
	int j=0;
       #endif
  
       himax_int_enable(private_ts->client->irq,0);

     #ifdef HX_CHIP_STATUS_MONITOR
		HX_CHIP_POLLING_COUNT=0;
		if(HX_ON_HAND_SHAKING)//chip on hand shaking,wait hand shaking
		{
			for(j=0; j<100; j++)
				{
					if(HX_ON_HAND_SHAKING==0)//chip on hand shaking end
						{
							tp_log_info("%s:HX_ON_HAND_SHAKING OK check %d times\n",__func__,j);
							break;
						}
					else
						msleep(1);
				}

		}

		cancel_delayed_work_sync(&private_ts->himax_chip_monitor);

	#endif

	self_test_inter_flag= 1;
//HX_PROC_SEND_FLAG = 0;

	msleep(10);

	val = himax_chip_self_test();

	himax_HW_reset(true,false);

	himax_int_enable(private_ts->client->irq,1);
	#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	queue_delayed_work(private_ts->himax_chip_monitor_wq, &private_ts->himax_chip_monitor, HX_POLLING_TIMES*HZ);
	#endif
	//msleep(100);

	if(!HX_PROC_SEND_FLAG)
	{
		if (val == 0x00) {
			ret += sprintf(buf + ret, "Self_Test Pass\n");
		} else if (val == 01){
			ret += sprintf(buf + ret, "Self_Test Fail\n");
		} else {
			ret += sprintf(buf + ret, "Self_Test not complete\n");
		}
		HX_PROC_SEND_FLAG=1;
	}
	else
		HX_PROC_SEND_FLAG = 0;

	self_test_inter_flag= 0;
	return ret;
}

static struct file_operations himax_proc_self_test_ops =
{
	.owner = THIS_MODULE,
	.read = himax_self_test_read,
};

#endif


int himax_touch_proc_init(void)
{
	static struct proc_dir_entry *touch_proc_dir=NULL;

	tp_log_info(" %s: proc file create enter!\n", __func__);

	himax_touch_proc_dir = proc_mkdir( HIMAX_PROC_TOUCH_FOLDER, NULL);
	if (himax_touch_proc_dir == NULL)
	{

		tp_log_err(" %s: himax_touch_proc_dir file create failed!\n", __func__);
		return -ENOMEM;
	}

	touch_proc_dir = proc_create(HIMAX_PROC_DEBUG_LEVEL_FILE, (S_IWUSR|S_IRUGO),
		himax_touch_proc_dir, &himax_proc_debug_level_ops);
	if (touch_proc_dir == NULL)
	{
		tp_log_err(" %s: proc debug_level file create failed!\n", __func__);
		goto fail_1;
	}

	touch_proc_dir = proc_create(HIMAX_PROC_VENDOR_FILE, (S_IRUGO),
		himax_touch_proc_dir, &himax_proc_vendor_ops);
	if(touch_proc_dir == NULL)
	{
		tp_log_err(" %s: proc vendor file create failed!\n", __func__);
		goto fail_2;
	}

	touch_proc_dir = proc_create(HIMAX_PROC_ATTN_FILE, (S_IRUGO),
		himax_touch_proc_dir, &himax_proc_attn_ops);
	if(touch_proc_dir == NULL)
	{
		tp_log_err(" %s: proc attn file create failed!\n", __func__);
		goto fail_3;
	}

	touch_proc_dir = proc_create(HIMAX_PROC_INT_EN_FILE, (S_IWUSR|S_IRUGO),
		himax_touch_proc_dir, &himax_proc_int_en_ops);
	if(touch_proc_dir == NULL)
	{
		tp_log_err(" %s: proc int en file create failed!\n", __func__);
		goto fail_4;
	}

	touch_proc_dir = proc_create(HIMAX_PROC_LAYOUT_FILE, (S_IWUSR|S_IRUGO),
		himax_touch_proc_dir, &himax_proc_layout_ops);
	if(touch_proc_dir == NULL)
	{
		tp_log_err(" %s: proc layout file create failed!\n", __func__);
		goto fail_5;
	}
#ifdef HX_TP_PROC_RESET
	touch_proc_dir = proc_create(HIMAX_PROC_RESET_FILE, (S_IWUSR),
		himax_touch_proc_dir, &himax_proc_reset_ops);
	if(touch_proc_dir == NULL)
	{
		tp_log_err(" %s: proc reset file create failed!\n", __func__);
		goto fail_6;
	}
#endif

#ifdef HX_TP_PROC_DIAG
	touch_proc_dir = proc_create(HIMAX_PROC_DIAG_FILE, (S_IWUSR|S_IRUGO),
		himax_touch_proc_dir, &himax_proc_diag_ops);
	if(touch_proc_dir == NULL)
	{
		tp_log_err(" %s: proc diag file create failed!\n", __func__);
		goto fail_7;
	}
#endif

#ifdef HX_TP_PROC_REGISTER
	touch_proc_dir = proc_create(HIMAX_PROC_REGISTER_FILE, (S_IWUSR|S_IRUGO),
		himax_touch_proc_dir, &himax_proc_register_ops);
	if(touch_proc_dir == NULL)
	{
		tp_log_err(" %s: proc register file create failed!\n", __func__);
		goto fail_8;
	}
#endif

#ifdef HX_TP_PROC_DEBUG
	touch_proc_dir = proc_create(HIMAX_PROC_DEBUG_FILE, (S_IWUSR|S_IRUGO),
		himax_touch_proc_dir, &himax_proc_debug_ops);
	if(touch_proc_dir == NULL)
	{
		tp_log_err(" %s: proc debug file create failed!\n", __func__);
		goto fail_9;
	}
#endif

#ifdef HX_TP_PROC_FLASH_DUMP
	touch_proc_dir = proc_create(HIMAX_PROC_FLASH_DUMP_FILE, (S_IWUSR|S_IRUGO),
		himax_touch_proc_dir, &himax_proc_flash_ops);
	if(touch_proc_dir == NULL)
	{
		tp_log_err(" %s: proc flash dump file create failed!\n", __func__);
		goto fail_10;
	}
#endif
#ifdef HX_TP_PROC_SELF_TEST
	touch_proc_dir = proc_create(HIMAX_PROC_SELF_TEST_FILE, (S_IRUGO),
		himax_touch_proc_dir, &himax_proc_self_test_ops);
	if(touch_proc_dir == NULL)
	{
		tp_log_err(" %s: proc self_test file create failed!\n", __func__);
		goto fail_11;
	}
#endif

	return 0 ;

#ifdef HX_TP_PROC_SELF_TEST
	fail_11: remove_proc_entry( HIMAX_PROC_SELF_TEST_FILE, himax_touch_proc_dir );
#endif
#ifdef HX_TP_PROC_FLASH_DUMP
	fail_10: remove_proc_entry( HIMAX_PROC_FLASH_DUMP_FILE, himax_touch_proc_dir );
#endif
#ifdef HX_TP_PROC_DEBUG
	fail_9: remove_proc_entry( HIMAX_PROC_DEBUG_FILE, himax_touch_proc_dir );
#endif
#ifdef HX_TP_PROC_REGISTER
	fail_8: remove_proc_entry( HIMAX_PROC_REGISTER_FILE, himax_touch_proc_dir );
#endif
#ifdef HX_TP_PROC_DIAG
	fail_7: remove_proc_entry( HIMAX_PROC_DIAG_FILE, himax_touch_proc_dir );
#endif
#ifdef HX_TP_PROC_RESET
	fail_6: remove_proc_entry( HIMAX_PROC_RESET_FILE, himax_touch_proc_dir );
#endif
	fail_5: remove_proc_entry( HIMAX_PROC_LAYOUT_FILE, himax_touch_proc_dir );
	fail_4: remove_proc_entry( HIMAX_PROC_INT_EN_FILE, himax_touch_proc_dir );
	fail_3: remove_proc_entry( HIMAX_PROC_ATTN_FILE, himax_touch_proc_dir );
	fail_2: remove_proc_entry( HIMAX_PROC_VENDOR_FILE, himax_touch_proc_dir );
	fail_1: remove_proc_entry( HIMAX_PROC_DEBUG_LEVEL_FILE, NULL );
		return -ENOMEM;
}

void himax_touch_proc_deinit(void)
{

#ifdef HX_TP_PROC_SELF_TEST
		remove_proc_entry(HIMAX_PROC_SELF_TEST_FILE, himax_touch_proc_dir);
#endif
#ifdef HX_TP_PROC_FLASH_DUMP
		remove_proc_entry(HIMAX_PROC_FLASH_DUMP_FILE, himax_touch_proc_dir);
#endif
#ifdef HX_TP_PROC_DEBUG
		remove_proc_entry( HIMAX_PROC_DEBUG_FILE, himax_touch_proc_dir );
#endif
#ifdef HX_TP_PROC_REGISTER
		remove_proc_entry(HIMAX_PROC_REGISTER_FILE, himax_touch_proc_dir);
#endif
#ifdef HX_TP_PROC_DIAG
		remove_proc_entry(HIMAX_PROC_DIAG_FILE, himax_touch_proc_dir);
#endif
#ifdef HX_TP_PROC_RESET
		remove_proc_entry( HIMAX_PROC_RESET_FILE, himax_touch_proc_dir );
#endif
		remove_proc_entry( HIMAX_PROC_LAYOUT_FILE, himax_touch_proc_dir );
		remove_proc_entry( HIMAX_PROC_INT_EN_FILE, himax_touch_proc_dir );
		remove_proc_entry( HIMAX_PROC_ATTN_FILE, himax_touch_proc_dir );
		remove_proc_entry( HIMAX_PROC_VENDOR_FILE, himax_touch_proc_dir );
		remove_proc_entry( HIMAX_PROC_DEBUG_LEVEL_FILE, himax_touch_proc_dir );
		remove_proc_entry( HIMAX_PROC_TOUCH_FOLDER, NULL );
}


