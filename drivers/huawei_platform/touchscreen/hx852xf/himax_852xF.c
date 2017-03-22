/*< CPM2016032400222 shihuijun 20160324 begin */
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

#ifdef CONFIG_APP_INFO
#include <misc/app_info.h>
#endif

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <linux/hw_dev_dec.h>
#endif

static int	HX_TOUCH_INFO_POINT_CNT   = 0;

#define RETRY_TIMES 200

#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)

#ifdef HX_TP_PROC_DIAG
	int  touch_monitor_stop_flag 	= 0;
	uint8_t diag_coor[128];// = {0xFF};
#endif

#endif


#ifdef CONFIG_HUAWEI_DSM
struct dsm_dev dsm_hmx_tp = {
	.name = "dsm_i2c_bus",	// dsm client name
	.fops = NULL,
	.buff_size = TP_RADAR_BUF_MAX,
};
struct hmx_dsm_info hmx_tp_dsm_info;
struct dsm_client *hmx_tp_dclient = NULL;
#endif
uint8_t	IC_STATUS_CHECK	= 0xAA;
static uint8_t 	EN_NoiseFilter = 0x00;
static uint8_t	Last_EN_NoiseFilter = 0x00;

static int	hx_point_num	= 0;		//real point report number	// for himax_ts_work_func use

struct himax_ts_data *private_ts;
u8 	HW_RESET_ACTIVATE  = 1;

#ifdef HX_ESD_WORKAROUND
static u8 ESD_RESET_ACTIVATE = 1;
u8 ESD_R36_FAIL = 0;
#endif

#ifdef HX_CHIP_STATUS_MONITOR

static int HX_POLLING_TIMER  = 5;//unit:sec
int HX_ON_HAND_SHAKING       = 0;
#endif

//static int self_test_inter_flag = 0;
#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data);
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void himax_ts_early_suspend(struct early_suspend *h);
static void himax_ts_late_resume(struct early_suspend *h);
#endif

extern int get_boot_into_recovery_flag(void);

int himax_hand_shaking(void)    //0:Running, 1:Stop, 2:I2C Fail
{
	int ret, result;
	uint8_t hw_reset_check[1];
	uint8_t hw_reset_check_2[1];
	uint8_t buf0[2];

	memset(hw_reset_check, 0x00, sizeof(hw_reset_check));
	memset(hw_reset_check_2, 0x00, sizeof(hw_reset_check_2));

	buf0[0] = 0xF2;
	if (IC_STATUS_CHECK == 0xAA) {
		buf0[1] = 0xAA;
		IC_STATUS_CHECK = 0x55;
	} else {
		buf0[1] = 0x55;
		IC_STATUS_CHECK = 0xAA;
	}

	ret = i2c_himax_master_write(private_ts->client, buf0, 2, DEFAULT_RETRY_CNT);
	if (ret < 0) {
		tp_log_err("[Himax]:write 0xF2 failed line: %d \n",__LINE__);
		goto work_func_send_i2c_msg_fail;
	}
	msleep(50);

	buf0[0] = 0xF2;
	buf0[1] = 0x00;
	ret = i2c_himax_master_write(private_ts->client, buf0, 2, DEFAULT_RETRY_CNT);
	if (ret < 0) {
		tp_log_err("[Himax]:write 0xF2 failed line: %d \n",__LINE__);
		goto work_func_send_i2c_msg_fail;
	}
	msleep(2);

	ret = i2c_himax_read(private_ts->client, 0xD1, hw_reset_check, 1, DEFAULT_RETRY_CNT);
	if (ret < 0) {
		tp_log_err("[Himax]:i2c_himax_read 0xD1 failed line: %d \n",__LINE__);
		goto work_func_send_i2c_msg_fail;
	}

	if ((IC_STATUS_CHECK != hw_reset_check[0])) {
		msleep(2);
		ret = i2c_himax_read(private_ts->client, 0xD1, hw_reset_check_2, 1, DEFAULT_RETRY_CNT);
		if (ret < 0) {
			tp_log_err("[Himax]:i2c_himax_read 0xD1 failed line: %d \n",__LINE__);
			goto work_func_send_i2c_msg_fail;
		}

		if (hw_reset_check[0] == hw_reset_check_2[0]) {
			result = 1;
		} else {
			result = 0;
		}
	} else {
		result = 0;
	}

	return result;

work_func_send_i2c_msg_fail:
	return 2;
}


int himax_input_register(struct himax_ts_data *ts)
{
	int ret;
	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		tp_log_err("%s: Failed to allocate input device\n", __func__);
		return ret;
	}
	ts->input_dev->name = "huawei,touchscreen";

	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);

//	set_bit(KEY_BACK, ts->input_dev->keybit);
//	set_bit(KEY_HOME, ts->input_dev->keybit);
//	set_bit(KEY_MENU, ts->input_dev->keybit);
//	set_bit(KEY_SEARCH, ts->input_dev->keybit);
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	//set_bit(KEY_APP_SWITCH, ts->input_dev->keybit);

	set_bit(INPUT_PROP_DIRECT, ts->input_dev->propbit);

	if (ts->protocol_type == PROTOCOL_TYPE_A) {
		//ts->input_dev->mtsize = ts->nFinger_support;
		input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID,
		0, 3, 0, 0);
	} else {/* PROTOCOL_TYPE_B */
		set_bit(MT_TOOL_FINGER, ts->input_dev->keybit);
		input_mt_init_slots(ts->input_dev, ts->nFinger_support,INPUT_MT_DIRECT);
	}

	tp_log_info("input_set_abs_params: min_x %d, max_x %d, min_y %d, max_y %d\n",
		ts->pdata->abs_x_min, ts->pdata->abs_x_max, ts->pdata->abs_y_min, ts->pdata->abs_y_max);

	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X,ts->pdata->abs_x_min, ts->pdata->abs_x_max, ts->pdata->abs_x_fuzz, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y,ts->pdata->abs_y_min, ts->pdata->abs_y_max, ts->pdata->abs_y_fuzz, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR,ts->pdata->abs_pressure_min, ts->pdata->abs_pressure_max, ts->pdata->abs_pressure_fuzz, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE,ts->pdata->abs_pressure_min, ts->pdata->abs_pressure_max, ts->pdata->abs_pressure_fuzz, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR,ts->pdata->abs_width_min, ts->pdata->abs_width_max, ts->pdata->abs_pressure_fuzz, 0);

//	input_set_abs_params(ts->input_dev, ABS_MT_AMPLITUDE, 0, ((ts->pdata->abs_pressure_max << 16) | ts->pdata->abs_width_max), 0, 0);
//	input_set_abs_params(ts->input_dev, ABS_MT_POSITION, 0, (BIT(31) | (ts->pdata->abs_x_max << 16) | ts->pdata->abs_y_max), 0, 0);

	return input_register_device(ts->input_dev);
}

static void calcDataSize(uint8_t finger_num)
{
	struct himax_ts_data *ts_data = private_ts;
	ts_data->coord_data_size = 4 * finger_num;// 1 coord 4 bytes.
	ts_data->area_data_size = ((finger_num / 4) + (finger_num % 4 ? 1 : 0)) * 4;  // 1 area 4 finger ?
	ts_data->raw_data_frame_size = 128 - ts_data->coord_data_size - ts_data->area_data_size - 4 - 4 - 1;
	ts_data->raw_data_nframes  = ((uint32_t)ts_data->x_channel * ts_data->y_channel +
										ts_data->x_channel + ts_data->y_channel) / ts_data->raw_data_frame_size +
										(((uint32_t)ts_data->x_channel * ts_data->y_channel +
		  								ts_data->x_channel + ts_data->y_channel) % ts_data->raw_data_frame_size)? 1 : 0;
	tp_log_info("%s: coord_data_size: %d, area_data_size:%d, raw_data_frame_size:%d, raw_data_nframes:%d", __func__, ts_data->coord_data_size, ts_data->area_data_size, ts_data->raw_data_frame_size, ts_data->raw_data_nframes);
}

static void calculate_point_number(void)
{
	HX_TOUCH_INFO_POINT_CNT = HX_MAX_PT * 4 ;

	if ( (HX_MAX_PT % 4) == 0)
		HX_TOUCH_INFO_POINT_CNT += (HX_MAX_PT / 4) * 4 ;
	else
		HX_TOUCH_INFO_POINT_CNT += ((HX_MAX_PT / 4) +1) * 4 ;
}

static uint8_t himax_read_Sensor_ID(struct i2c_client *client)
{
	uint8_t val_high[1], val_low[1], ID0=0, ID1=0;
	uint8_t sensor_id;
	char data[3];

	data[0] = 0x56; data[1] = 0x02; data[2] = 0x02;/*ID pin PULL High*/
	i2c_himax_master_write(client, &data[0],3,DEFAULT_RETRY_CNT);
	msleep(1);

	//read id pin high
	i2c_himax_read(client, 0x57, val_high, 1, DEFAULT_RETRY_CNT);

	data[0] = 0x56; data[1] = 0x01; data[2] = 0x01;/*ID pin PULL Low*/
	i2c_himax_master_write(client, &data[0],3,DEFAULT_RETRY_CNT);
	msleep(1);

	//read id pin low
	i2c_himax_read(client, 0x57, val_low, 1, DEFAULT_RETRY_CNT);

	if((val_high[0] & 0x01) ==0)
		ID0=0x02;/*GND*/
	else if((val_low[0] & 0x01) ==0)
		ID0=0x01;/*Floating*/
	else
		ID0=0x04;/*VCC*/

	if((val_high[0] & 0x02) ==0)
		ID1=0x02;/*GND*/
	else if((val_low[0] & 0x02) ==0)
		ID1=0x01;/*Floating*/
	else
		ID1=0x04;/*VCC*/
	if((ID0==0x04)&&(ID1!=0x04))
		{
			data[0] = 0x56; data[1] = 0x02; data[2] = 0x01;/*ID pin PULL High,Low*/
			i2c_himax_master_write(client, &data[0],3,DEFAULT_RETRY_CNT);
			msleep(1);

		}
	else if((ID0!=0x04)&&(ID1==0x04))
		{
			data[0] = 0x56; data[1] = 0x01; data[2] = 0x02;/*ID pin PULL Low,High*/
			i2c_himax_master_write(client, &data[0],3,DEFAULT_RETRY_CNT);
			msleep(1);

		}
	else if((ID0==0x04)&&(ID1==0x04))
		{
			data[0] = 0x56; data[1] = 0x02; data[2] = 0x02;/*ID pin PULL High,High*/
			i2c_himax_master_write(client, &data[0],3,DEFAULT_RETRY_CNT);
			msleep(1);

		}
	sensor_id=(ID1<<4)|ID0;

	data[0] = 0xF3; data[1] = sensor_id;
	i2c_himax_master_write(client, &data[0],2,DEFAULT_RETRY_CNT);/*Write to MCU*/
	msleep(1);

	return sensor_id;

}

void himax_touch_information(void)
{
	char data[12] = {0};

	tp_log_info("%s:IC_TYPE =%d\n", __func__,IC_TYPE);

	if(IC_TYPE == HX_85XX_ES_SERIES_PWON)
		{
			data[0] = 0x8C;
			data[1] = 0x14;
			i2c_himax_master_write(private_ts->client, &data[0],2,DEFAULT_RETRY_CNT);
			msleep(10);
			data[0] = 0x8B;
			data[1] = 0x00;
			data[2] = 0x70;
			i2c_himax_master_write(private_ts->client, &data[0],3,DEFAULT_RETRY_CNT);
			msleep(10);
			i2c_himax_read(private_ts->client, 0x5A, data, 12, DEFAULT_RETRY_CNT);
			HX_RX_NUM = data[0];				 // FE(70)
			HX_TX_NUM = data[1];				 // FE(71)
			HX_MAX_PT = (data[2] & 0xF0) >> 4; // FE(72)
			if((data[4] & 0x04) == 0x04) {//FE(74)
				HX_XY_REVERSE = true;
			HX_Y_RES = data[6]*256 + data[7]; //FE(76),FE(77)
			HX_X_RES = data[8]*256 + data[9]; //FE(78),FE(79)
			} else {
			HX_XY_REVERSE = false;
			HX_X_RES = data[6]*256 + data[7]; //FE(76),FE(77)
			HX_Y_RES = data[8]*256 + data[9]; //FE(78),FE(79)
			}
			data[0] = 0x8C;
			data[1] = 0x00;
			i2c_himax_master_write(private_ts->client, &data[0],2,DEFAULT_RETRY_CNT);
			msleep(10);
			data[0] = 0x8C;
			data[1] = 0x14;
			i2c_himax_master_write(private_ts->client, &data[0],2,DEFAULT_RETRY_CNT);
			msleep(10);
			data[0] = 0x8B;
			data[1] = 0x00;
			data[2] = 0x02;
			i2c_himax_master_write(private_ts->client, &data[0],3,DEFAULT_RETRY_CNT);
			msleep(10);
			i2c_himax_read(private_ts->client, 0x5A, data, 10, DEFAULT_RETRY_CNT);
/*			if((data[1] & 0x01) == 1) {//FE(02)
				HX_INT_IS_EDGE = true;
			} else {
				HX_INT_IS_EDGE = false;
			}*/
			data[0] = 0x8C;
			data[1] = 0x00;
			i2c_himax_master_write(private_ts->client, &data[0],2,DEFAULT_RETRY_CNT);
			msleep(10);
			tp_log_info("%s:HX_RX_NUM =%d,HX_TX_NUM =%d,HX_MAX_PT=%d \n", __func__,HX_RX_NUM,HX_TX_NUM,HX_MAX_PT);
		}
		else if(IC_TYPE == HX_85XX_F_SERIES_PWON)
		{
			data[0] = 0x8C;
			data[1] = 0x11;
			i2c_himax_master_write(private_ts->client, &data[0],2,DEFAULT_RETRY_CNT);
			msleep(10);
			data[0] = 0x8B;
			data[1] = 0x00;
			data[2] = 0x70;
			i2c_himax_master_write(private_ts->client, &data[0],3,DEFAULT_RETRY_CNT);
			msleep(10);
			i2c_himax_read(private_ts->client, 0x5A, data, 12, DEFAULT_RETRY_CNT);
			HX_RX_NUM = data[0];				 // FE(70)
			HX_TX_NUM = data[1];				 // FE(71)
			HX_MAX_PT = 10;//(data[2] & 0xF0) >> 4; // FE(72)
			if((data[4] & 0x04) == 0x04) {//FE(74)
				HX_XY_REVERSE = true;
				HX_Y_RES = data[6]*256 + data[7]; //FE(76),FE(77)
				HX_X_RES = data[8]*256 + data[9]; //FE(78),FE(79)
			} else {
				HX_XY_REVERSE = false;
				HX_X_RES = data[6]*256 + data[7]; //FE(76),FE(77)
				HX_Y_RES = data[8]*256 + data[9]; //FE(78),FE(79)
			}
			data[0] = 0x8C;
			data[1] = 0x00;
			i2c_himax_master_write(private_ts->client, &data[0],2,DEFAULT_RETRY_CNT);
			msleep(10);
			data[0] = 0x8C;
			data[1] = 0x11;
			i2c_himax_master_write(private_ts->client, &data[0],2,DEFAULT_RETRY_CNT);
			msleep(10);
			data[0] = 0x8B;
			data[1] = 0x00;
			data[2] = 0x02;
			i2c_himax_master_write(private_ts->client, &data[0],3,DEFAULT_RETRY_CNT);
			msleep(10);
			i2c_himax_read(private_ts->client, 0x5A, data, 10, DEFAULT_RETRY_CNT);
/*			if((data[1] & 0x01) == 1) {//FE(02)
				HX_INT_IS_EDGE = true;
			} else {
				HX_INT_IS_EDGE = false;
			}*/
			data[0] = 0x8C;
			data[1] = 0x00;
			i2c_himax_master_write(private_ts->client, &data[0],2,DEFAULT_RETRY_CNT);
			msleep(10);
			tp_log_info("%s:HX_RX_NUM =%d,HX_TX_NUM =%d,HX_MAX_PT=%d \n", __func__,HX_RX_NUM,HX_TX_NUM,HX_MAX_PT);
			HX_RX_NUM				= 38;
			HX_TX_NUM				= 24;
			HX_BT_NUM				= 0;
			HX_X_RES				= 1200;
			HX_Y_RES				= 1920;
			HX_MAX_PT				= 10;
			HX_XY_REVERSE			= true;
		}
		else
		{
			HX_RX_NUM				= 0;
			HX_TX_NUM				= 0;
			HX_BT_NUM				= 0;
			HX_X_RES				= 0;
			HX_Y_RES				= 0;
			HX_MAX_PT				= 0;
			HX_XY_REVERSE			= false;
		}
}

void himax_power_on_initCMD(struct i2c_client *client)
{
	tp_log_info("%s:enter\n", __func__);

	/*i2c_himax_write_command(client, 0x83, DEFAULT_RETRY_CNT);
	msleep(30);

	i2c_himax_write_command(client, 0x81, DEFAULT_RETRY_CNT);
	msleep(50);

	i2c_himax_write_command(client, 0x82, DEFAULT_RETRY_CNT);
	msleep(50);

	i2c_himax_write_command(client, 0x80, DEFAULT_RETRY_CNT);
	msleep(50);*/

	i2c_himax_write_command(client, 0x83, DEFAULT_RETRY_CNT);
	msleep(30);

	i2c_himax_write_command(client, 0x81, DEFAULT_RETRY_CNT);
	msleep(50);

	tp_log_info("%s:exit\n", __func__);
}

static void himax_get_information(struct i2c_client *client)
{
	tp_log_info("%s:enter\n", __func__);
	//Sense on to update the information
	i2c_himax_write_command(client, 0x83, DEFAULT_RETRY_CNT);
	msleep(30);

	i2c_himax_write_command(client, 0x81, DEFAULT_RETRY_CNT);
	msleep(50);

	i2c_himax_write_command(client, 0x82, DEFAULT_RETRY_CNT);
	msleep(50);

	i2c_himax_write_command(client, 0x80, DEFAULT_RETRY_CNT);
	msleep(50);

	himax_touch_information();

	i2c_himax_write_command(client, 0x83, DEFAULT_RETRY_CNT);
	msleep(30);

	i2c_himax_write_command(client, 0x81, DEFAULT_RETRY_CNT);
	msleep(50);
}

/*int_off:  false: before reset, need disable irq; true: before reset, don't need disable irq.*/
/*loadconfig:  after reset, load config or not.*/
void himax_HW_reset(uint8_t loadconfig,uint8_t int_off)
{
	struct himax_ts_data *ts = private_ts;
	int iCount = 0;

	HW_RESET_ACTIVATE=1;

	if(HX_UPDATE_FLAG==1)
	{
		HX_RESET_COUNT++;
		tp_log_info("HX still in updateing no reset ");
		return ;
	}

	if (ts->rst_gpio) {
		if(!int_off)
		{
			himax_int_enable(private_ts->client->irq,0);
			msleep(30);
			if((atomic_read(&ts->irq_complete) == 0) && (iCount <10) ){
				tp_log_info("%s irq function not complete. try time is %d\n",__func__, iCount);
				msleep(10);
				iCount ++;
			}
		}

		tp_log_info("%s: Now reset the Touch chip.\n", __func__);

		himax_rst_gpio_set(ts->rst_gpio, 0);
		msleep(20);
		himax_rst_gpio_set(ts->rst_gpio, 1);
		msleep(20);

		tp_log_info("%s: reset ok.\n", __func__);

		if(loadconfig)
			himax_loadSensorConfig(private_ts->client,private_ts->pdata);

		if(!int_off)
		{
			himax_int_enable(private_ts->client->irq,1);
		}
	}
}


static bool himax_ic_package_check(struct himax_ts_data *ts)
{
	uint8_t cmd[3];

	memset(cmd, 0x00, sizeof(cmd));

	if (i2c_himax_read(ts->client, 0xD1, cmd, 3, DEFAULT_RETRY_CNT) < 0)
		return false ;

	if(cmd[0] == 0x06 && cmd[1] == 0x85 &&
		(cmd[2] == 0x28 || cmd[2] == 0x29 || cmd[2] == 0x30 ))
		{
		IC_TYPE       			= HX_85XX_F_SERIES_PWON;
        	IC_CHECKSUM 		= HX_TP_BIN_CHECKSUM_CRC;
        	//Himax: Set FW and CFG Flash Address
        	FW_VER_MAJ_FLASH_ADDR    = 64901;  //0xFD85
        	FW_VER_MAJ_FLASH_LENG     = 1;
        	FW_VER_MIN_FLASH_ADDR    = 64902;  //0xFD86
        	FW_VER_MIN_FLASH_LENG     = 1;
        	CFG_VER_MAJ_FLASH_ADDR 	= 64928;   //0xFDA0
        	CFG_VER_MAJ_FLASH_LENG 	= 12;
        	CFG_VER_MIN_FLASH_ADDR 	= 64940;   //0xFDAC
        	CFG_VER_MIN_FLASH_LENG 	= 12;
		FW_CFG_VER_FLASH_ADDR	= 64900;  //0xFD84
#ifdef HX_AUTO_UPDATE_CONFIG
			CFB_START_ADDR 		= 0xFD80;
			CFB_LENGTH				= 638;
			CFB_INFO_LENGTH 		= 68;
#endif
		tp_log_info("Himax IC package 852x F\n");
	}
	else {
		tp_log_err("Himax IC package incorrect!!\n");
	}
	return true;
}

static void himax_read_TP_info(struct i2c_client *client)
{
	char data[12] = {0};

	//read fw version
	if (i2c_himax_read(client, HX_VER_FW_MAJ, data, 1, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return;
	}
	private_ts->vendor_fw_ver_H = data[0];

	if (i2c_himax_read(client, HX_VER_FW_MIN, data, 1, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return;
	}
	private_ts->vendor_fw_ver_L = data[0];
	//read config version
	if (i2c_himax_read(client, HX_VER_FW_CFG, data, 1, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return;
	}
	private_ts->vendor_config_ver = data[0];

	//read sensor ID
	private_ts->vendor_sensor_id = himax_read_Sensor_ID(client);

	tp_log_info("sensor_id=%x.\n",private_ts->vendor_sensor_id);
	tp_log_info("fw_ver=%x,%x.\n",private_ts->vendor_fw_ver_H,private_ts->vendor_fw_ver_L);
	tp_log_info("config_ver=%x.\n",private_ts->vendor_config_ver);
}

#ifdef HX_ESD_WORKAROUND
void ESD_HW_REST(void)
{
		if (self_test_inter_flag == 1 )
		{
			tp_log_info("In self test ,not  TP: ESD - Reset\n");
			return;
		}

		if(HX_UPDATE_FLAG==1)
		{
			HX_ESD_RESET_COUNT++;
			tp_log_info("HX still in updateing , no ESD reset");
			return;
		}
		ESD_RESET_ACTIVATE = 1;

		ESD_R36_FAIL = 0;
#ifdef HX_CHIP_STATUS_MONITOR
		HX_CHIP_POLLING_COUNT=0;
#endif
		tp_log_info("START_Himax TP: ESD - Reset\n");

		while(ESD_R36_FAIL <=3 )
		{

			himax_rst_gpio_set(private_ts->rst_gpio, 0);
			msleep(20);
			himax_rst_gpio_set(private_ts->rst_gpio, 1);
			msleep(20);

			if(himax_loadSensorConfig(private_ts->client, private_ts->pdata)<0)
				ESD_R36_FAIL++;
			else
				break;

		}
		tp_log_info("END_Himax TP: ESD - Reset\n");
	}
#endif

#ifdef HX_CHIP_STATUS_MONITOR
static void himax_chip_monitor_function(struct work_struct *work) //for ESD solution
{
	int ret=0;

	tp_log_vdebug(" %s: POLLING_COUNT=%x, STATUS=%x \n", __func__,HX_CHIP_POLLING_COUNT,ret);
	if(HX_CHIP_POLLING_COUNT >= (HX_POLLING_TIMES-1))//POLLING TIME
	{
		HX_ON_HAND_SHAKING=1;
		ret = himax_hand_shaking(); //0:Running, 1:Stop, 2:I2C Fail
		HX_ON_HAND_SHAKING=0;
		if(ret == 2)
		{
			tp_log_info(" %s: I2C Fail \n", __func__);
			ESD_HW_REST();
		}
		else if(ret == 1)
		{
			tp_log_info(" %s: MCU Stop \n", __func__);
			ESD_HW_REST();
		}
		HX_CHIP_POLLING_COUNT=0;//clear polling counter
	}
	else
		HX_CHIP_POLLING_COUNT++;

	queue_delayed_work(private_ts->himax_chip_monitor_wq, &private_ts->himax_chip_monitor, HX_POLLING_TIMER*HZ);

	return;
}
#endif

inline void himax_ts_work(struct himax_ts_data *ts)
{
	int ret = 0;
	uint8_t buf[128], finger_num, hw_reset_check[2];
	uint16_t finger_pressed;
	int32_t loop_i;
	unsigned char check_sum_cal = 0;
	int RawDataLen = 0;
	int raw_cnt_max ;
	int raw_cnt_rmd ;
	int hx_touch_info_size;
	int base,x,y,w;
	uint8_t coordInfoSize = ts->coord_data_size + ts->area_data_size + 4;

	static int iCount = 0;
#ifdef HX_TP_PROC_DIAG
	uint8_t *mutual_data;
	uint8_t *self_data;
	uint8_t diag_cmd;
	int  	i;
	int 	mul_num;
	int 	self_num;
	int 	index = 0;
	int  	temp1, temp2;
#endif

#ifdef HX_CHIP_STATUS_MONITOR
	int j=0;
#endif

	memset(buf, 0x00, sizeof(buf));
	memset(hw_reset_check, 0x00, sizeof(hw_reset_check));

	atomic_set(&ts->irq_complete, 0);
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
			if(j==100)
			{
				tp_log_err("%s:HX_ON_HAND_SHAKING timeout reject interrupt\n",__func__);
				return;
			}
		}
#endif

	raw_cnt_max = HX_MAX_PT/4;//max point / 4 ?
	raw_cnt_rmd = HX_MAX_PT%4;

	if (raw_cnt_rmd != 0x00) //more than 4 fingers  //??
	{
		RawDataLen = 128 - ((HX_MAX_PT+raw_cnt_max+3)*4) - 1;
		hx_touch_info_size = (HX_MAX_PT+raw_cnt_max+2)*4;
	}
	else //less than 4 fingers
	{
		RawDataLen = 128 - ((HX_MAX_PT+raw_cnt_max+2)*4) - 1;
		hx_touch_info_size = (HX_MAX_PT+raw_cnt_max+1)*4;
	}

	//tp_log_info("%s: himax test: raw_cnt_max %d, raw_cnt_rmd: %d, RawDataLen %d, hx_touch_info_size %d \n",__func__, raw_cnt_max, raw_cnt_rmd, RawDataLen, hx_touch_info_size);//add for test  //2 2 67 56

#ifdef HX_TP_PROC_DIAG
	diag_cmd = getDiagCommand();

//	tp_log_info("%s: diag_cmd %d \n",__func__, diag_cmd);//add for test

#ifdef HX_ESD_WORKAROUND
	if((diag_cmd) || (ESD_RESET_ACTIVATE) || (HW_RESET_ACTIVATE))
#else
	if((diag_cmd) || (HW_RESET_ACTIVATE))
#endif
	{
		ret = i2c_himax_read(ts->client, 0x86, buf, 128,DEFAULT_RETRY_CNT);//diad cmd not 0, need to read 128.
	}
	else{
		if(touch_monitor_stop_flag != 0){
			ret = i2c_himax_read(ts->client, 0x86, buf, 128,DEFAULT_RETRY_CNT);
			touch_monitor_stop_flag-- ;
		}
		else{
			ret = i2c_himax_read(ts->client, 0x86, buf, hx_touch_info_size,DEFAULT_RETRY_CNT);
		}
	}
	if (ret < 0)
#else
	if (i2c_himax_read(ts->client, 0x86, buf, hx_touch_info_size,DEFAULT_RETRY_CNT))
#endif
	{
		tp_log_err("%s: can't read data from chip!\n", __func__);
#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_dsm_info.WORK_status = TP_WOKR_READ_DATA_ERROR;
			hmx_tp_report_dsm_err(DSM_TP_IC_ERROR_NO, 0);
#endif
		iCount++;
		tp_log_err("%s: error count is %d !\n", __func__, iCount);
		if(iCount >= RETRY_TIMES)
		{
			iCount = 0;
			goto err_workqueue_out;
		}
		goto err_no_reset_out;
	}
	else
	{
#ifdef HX_ESD_WORKAROUND
			for(i = 0; i < hx_touch_info_size; i++)
			{
				if (buf[i] == 0x00) //case 2 ESD recovery flow-Disable
				{
					check_sum_cal = 1;
				}
				else if(buf[i] == 0xED)/*case 1 ESD recovery flow*/
				{
					check_sum_cal = 2;
				}
				else
				{
					check_sum_cal = 0;
					i = hx_touch_info_size;
					break;
				}
			}

			//IC status is abnormal ,do hand shaking
#ifdef HX_TP_PROC_DIAG
			diag_cmd = getDiagCommand();
			if (check_sum_cal != 0 && ESD_RESET_ACTIVATE == 0 && HW_RESET_ACTIVATE == 0 
				&& diag_cmd == 0 && self_test_inter_flag == 0)  //ESD Check
#else
			if (check_sum_cal != 0 && ESD_RESET_ACTIVATE == 0 && HW_RESET_ACTIVATE == 0 
				&& self_test_inter_flag == 0)  //ESD Check
#endif
			{
				ret = himax_hand_shaking(); //check mcu status  ---  0:Running, 1:Stop, 2:I2C Fail
				if (ret == 2)
				{
					goto err_workqueue_out;
				}

				if ((ret == 1) && (check_sum_cal == 1))
				{
					tp_log_info("[HIMAX TP MSG]: ESD event checked - ALL Zero.\n");
					ESD_HW_REST();
				}
				else if (check_sum_cal == 2)
				{
					tp_log_info("[HIMAX TP MSG]: ESD event checked - ALL 0xED.\n");
					ESD_HW_REST();
				}

				//himax_int_enable(ts->client->irq,1);
				return;
			}
			else if (ESD_RESET_ACTIVATE)
			{
				ESD_RESET_ACTIVATE = 0;/*drop 1st interrupts after chip reset*/
				tp_log_info("[HIMAX TP MSG]:%s: Back from reset, ready to serve.\n", __func__);
				return;
			}
			else if (HW_RESET_ACTIVATE)
#else
			if (HW_RESET_ACTIVATE)
#endif
			{
				HW_RESET_ACTIVATE = 0;/*drop 1st interrupts after chip reset*/
				tp_log_info("[HIMAX TP MSG]:%s: HW_RST Back from reset, ready to serve.\n", __func__);
				atomic_set(&ts->irq_complete, 1);
				return;
			}

		for (loop_i = 0, check_sum_cal = 0; loop_i < hx_touch_info_size; loop_i++)
			check_sum_cal += buf[loop_i];

		if (check_sum_cal != 0x00 )  //self_test_inter_flag == 1
		{
			tp_log_info("[HIMAX TP MSG] checksum fail : check_sum_cal: 0x%02X\n", check_sum_cal);
#ifdef CONFIG_HUAWEI_DSM
				//hmx_tp_dsm_info.WORK_status = TP_WOKR_CHECKSUM_INFO_ERROR;
				//hmx_tp_report_dsm_err(DSM_TP_IC_ERROR_NO, 0);
#endif

		iCount++;
		tp_log_err("%s: error count is %d !\n", __func__, iCount);
		if(iCount >= RETRY_TIMES)
		{
			iCount = 0;
			goto err_workqueue_out;
		}
		goto err_no_reset_out;

		//	atomic_set(&ts->irq_complete, 1);
		//	return;
		}
	}

	if (ts->debug_log_level & BIT(0)) {
		tp_log_info("%s: raw data:\n", __func__);
		for (loop_i = 0; loop_i < hx_touch_info_size; loop_i++) {
			printk("0x%2.2X ", buf[loop_i]);
			if (loop_i % 8 == 7)
				printk("\n");
		}
	}

	//touch monitor raw data fetch
#ifdef HX_TP_PROC_DIAG
	diag_cmd = getDiagCommand();
	if (diag_cmd >= 1 && diag_cmd <= 6)
	{
		//tp_log_info("diag_cmd: %d\n", diag_cmd);

		//Check 128th byte CRC, after 56 bytes. 128 bytes
		for (i = hx_touch_info_size, check_sum_cal = 0; i < 128; i++)
		{
			check_sum_cal += buf[i];
		}

		if (check_sum_cal % 0x100 != 0)//designed to be devided by 0X100.
		{
			tp_log_err("fail,  check_sum_cal: %d\n", check_sum_cal);
#ifdef CONFIG_HUAWEI_DSM
				hmx_tp_dsm_info.WORK_status = TP_WOKR_CHECKSUM_ALL_ERROR;
				hmx_tp_report_dsm_err(DSM_TP_IC_ERROR_NO, 0);
#endif
			goto bypass_checksum_failed_packet;
		}

		mutual_data = getMutualBuffer();
		self_data 	= getSelfBuffer();

		// initiallize the block number of mutual and self
		mul_num = getXChannel() * getYChannel();
		self_num = getXChannel() + getYChannel();

		//Himax: Check Raw-Data Header  		//4byte header serial num, such as 1, 2..,can't be 0.   1 byte checksum.
		if (buf[hx_touch_info_size] == buf[hx_touch_info_size+1] && buf[hx_touch_info_size+1] == buf[hx_touch_info_size+2]
		&& buf[hx_touch_info_size+2] == buf[hx_touch_info_size+3] && buf[hx_touch_info_size] > 0)
		{
			index = (buf[hx_touch_info_size] - 1) * RawDataLen;
			tp_log_debug("Header[%d]: %x, %x, %x, %x, mutual: %d, self: %d\n", index, buf[56], buf[57], buf[58], buf[59], mul_num, self_num);

			temp2 = self_num + mul_num;
			for (i = 0; i < RawDataLen; i++)
			{
				temp1 = index + i;

				if (temp1 < mul_num)
				{ //mutual
					mutual_data[index + i] = buf[i + hx_touch_info_size+4];	//4: RawData Header
				}
				else
				{//self
					if (temp1 >= temp2)
					{
						break;
					}
					self_data[i+index-mul_num] = buf[i + hx_touch_info_size+4];	//4: RawData Header	
				}
			}
		}
		else
		{
			tp_log_info("[HIMAX TP MSG]%s: header format is wrong!\n", __func__);
#ifdef CONFIG_HUAWEI_DSM
				hmx_tp_dsm_info.WORK_status = TP_WOKR_HEAD_ERROR;
				hmx_tp_report_dsm_err(DSM_TP_IC_ERROR_NO, 0);
#endif
		}
	}
	else if (diag_cmd == 7)
	{
		memcpy(&(diag_coor[0]), &buf[0], 128);
	}

bypass_checksum_failed_packet:
#endif  //HX_TP_PROC_DIAG
		EN_NoiseFilter = (buf[HX_TOUCH_INFO_POINT_CNT+2]>>3);//HX_TOUCH_INFO_POINT_CNT: 52 ;  
		//tp_log_info("EN_NoiseFilter=%d\n",EN_NoiseFilter);//add for test   30
		EN_NoiseFilter = EN_NoiseFilter & 0x01;
		//tp_log_info("EN_NoiseFilter2=%d\n",EN_NoiseFilter);//add for test  0

		if (buf[HX_TOUCH_INFO_POINT_CNT] == 0xff)
			hx_point_num = 0;
		else
			hx_point_num = buf[HX_TOUCH_INFO_POINT_CNT] & 0x0f;//only use low 4 bits.

		// Touch Point information
		if (hx_point_num != 0 ) {
					uint16_t old_finger = ts->pre_finger_mask;
					finger_num = buf[coordInfoSize - 4] & 0x0F;
					finger_pressed = buf[coordInfoSize - 2] << 8 | buf[coordInfoSize - 3];
					for (loop_i = 0; loop_i < ts->nFinger_support; loop_i++) {
						if (((finger_pressed >> loop_i) & 1) == 1) {
							base = loop_i * 4;//every finger coordinate need 4 bytes.
							x = buf[base] << 8 | buf[base + 1];
							y = (buf[base + 2] << 8 | buf[base + 3]);
							//w = buf[(ts->nFinger_support * 4) + loop_i];//every finger pressure need 1 bytes.
							w = 10;
							finger_num--;

							if ((ts->debug_log_level & BIT(3)) > 0)//debug 3: print finger coordinate information 
							{
								if ((((old_finger >> loop_i) ^ (finger_pressed >> loop_i)) & 1) == 1)
								{
									if (ts->useScreenRes)
									{
										tp_log_info("status:%X, Screen:F:%02d Down, X:%d, Y:%d, W:%d, N:%d\n",
										finger_pressed, loop_i+1, x * ts->widthFactor >> SHIFTBITS,
										y * ts->heightFactor >> SHIFTBITS, w, EN_NoiseFilter);
									}
									else
									{
										tp_log_info("status:%X, Raw:F:%02d Down, X:%d, Y:%d, W:%d, N:%d\n",
										finger_pressed, loop_i+1, x, y, w, EN_NoiseFilter);
									}
								}
							}

							if (ts->protocol_type == PROTOCOL_TYPE_B)
							{
								input_mt_slot(ts->input_dev, loop_i);
							}

							input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, w);
							input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);
							input_report_abs(ts->input_dev, ABS_MT_PRESSURE, w);
							input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
							input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);

							if (ts->protocol_type == PROTOCOL_TYPE_A)
							{
								input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, loop_i);
								input_mt_sync(ts->input_dev);
							}
							else
							{
								ts->last_slot = loop_i;
								input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 1);
							}

							if (!ts->first_pressed)
							{
								ts->first_pressed = 1;//first report ??
								tp_log_info("S1@%d, %d\n", x, y);
							}

							ts->pre_finger_data[loop_i][0] = x;
							ts->pre_finger_data[loop_i][1] = y;


							if (ts->debug_log_level & BIT(1))
								tp_log_info("Finger %d=> X:%d, Y:%d W:%d, Z:%d, F:%d, N:%d\n",
									loop_i + 1, x, y, w, w, loop_i + 1, EN_NoiseFilter);
						} else {
							if (ts->protocol_type == PROTOCOL_TYPE_B)
							{
								input_mt_slot(ts->input_dev, loop_i);
								input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
								input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
								input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
								input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
								if (ts->debug_log_level & BIT(1))
									tp_log_info("All Finger leave_Clear_last_event\n");
							}

							if (loop_i == 0 && ts->first_pressed == 1)
							{
								ts->first_pressed = 2;
								tp_log_info("E1@%d, %d\n",
								ts->pre_finger_data[0][0] , ts->pre_finger_data[0][1]);
							}
							if ((ts->debug_log_level & BIT(3)) > 0)
							{
								if ((((old_finger >> loop_i) ^ (finger_pressed >> loop_i)) & 1) == 1)
								{
									if (ts->useScreenRes)
									{
										tp_log_info("status:%X, Screen:F:%02d Up, X:%d, Y:%d, N:%d\n",
										finger_pressed, loop_i+1, ts->pre_finger_data[loop_i][0] * ts->widthFactor >> SHIFTBITS,
										ts->pre_finger_data[loop_i][1] * ts->heightFactor >> SHIFTBITS, Last_EN_NoiseFilter);
									}
									else
									{
										tp_log_info("status:%X, Raw:F:%02d Up, X:%d, Y:%d, N:%d\n",
										finger_pressed, loop_i+1, ts->pre_finger_data[loop_i][0],
										ts->pre_finger_data[loop_i][1], Last_EN_NoiseFilter);
									}
								}
							}
						}
					}
					ts->pre_finger_mask = finger_pressed;

			input_report_key(ts->input_dev, BTN_TOUCH, 1);
			input_sync(ts->input_dev);
		} else if (hx_point_num == 0){
				// leave event
				if (ts->protocol_type == PROTOCOL_TYPE_A)
					input_mt_sync(ts->input_dev);

				for (loop_i = 0; loop_i < ts->nFinger_support; loop_i++) {
						if (((ts->pre_finger_mask >> loop_i) & 1) == 1) {
							if (ts->protocol_type == PROTOCOL_TYPE_B) {
								input_mt_slot(ts->input_dev, loop_i);
								input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
								input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
								input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
								input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
							}
						}
					}
				if (ts->pre_finger_mask > 0) {
					for (loop_i = 0; loop_i < ts->nFinger_support && (ts->debug_log_level & BIT(3)) > 0; loop_i++) {
						if (((ts->pre_finger_mask >> loop_i) & 1) == 1) {
							if (ts->useScreenRes) {
								tp_log_info("status:%X, Screen:F:%02d Up, X:%d, Y:%d, N:%d\n", 0, loop_i+1, ts->pre_finger_data[loop_i][0] * ts->widthFactor >> SHIFTBITS,
									ts->pre_finger_data[loop_i][1] * ts->heightFactor >> SHIFTBITS, Last_EN_NoiseFilter);
							} else {
								tp_log_info("status:%X, Raw:F:%02d Up, X:%d, Y:%d, N:%d\n",0, loop_i+1, ts->pre_finger_data[loop_i][0],ts->pre_finger_data[loop_i][1], Last_EN_NoiseFilter);
							}
						}
					}
					ts->pre_finger_mask = 0;
				}

				if (ts->first_pressed == 1) {
					ts->first_pressed = 2;
					tp_log_info("E1@%d, %d\n",ts->pre_finger_data[0][0] , ts->pre_finger_data[0][1]);
				}

				if (ts->debug_log_level & BIT(1))
					tp_log_info("All Finger leave\n");

			input_report_key(ts->input_dev, BTN_TOUCH, 0);
			input_sync(ts->input_dev);
		}

		Last_EN_NoiseFilter = EN_NoiseFilter;

	iCount = 0;//I2C communication ok, checksum ok;
	atomic_set(&ts->irq_complete, 1);
	return;

err_workqueue_out:
	tp_log_err("%s: Now reset the Touch chip.\n", __func__);

	himax_HW_reset(true,false);

err_no_reset_out:
	atomic_set(&ts->irq_complete, 1);
	return;
}


irqreturn_t himax_ts_thread(int irq, void *ptr)
{
	struct himax_ts_data *ts = ptr;
	struct timespec timeStart, timeEnd, timeDelta;

	if (ts->debug_log_level & BIT(2)) {
			getnstimeofday(&timeStart);
			/*tp_log_info(" Irq start time = %ld.%06ld s\n",
				timeStart.tv_sec, timeStart.tv_nsec/1000);*/
	}

	himax_ts_work(ts);
	if(ts->debug_log_level & BIT(2)) {
			getnstimeofday(&timeEnd);
				timeDelta.tv_nsec = (timeEnd.tv_sec*1000000000+timeEnd.tv_nsec)
				-(timeStart.tv_sec*1000000000+timeStart.tv_nsec);
			/*tp_log_info("Irq finish time = %ld.%06ld s\n",
				timeEnd.tv_sec, timeEnd.tv_nsec/1000);*/
			tp_log_info("Touch latency = %ld us\n", timeDelta.tv_nsec/1000);
	}
	return IRQ_HANDLED;
}

int himax_ts_register_interrupt(struct i2c_client *client)
{
	struct himax_ts_data *ts = i2c_get_clientdata(client);
	int ret = 0;

	ts->irq_enabled = 0;

	//Work functon
	if (client->irq) {/*INT mode*/
		tp_log_info("%s level trigger low\n ",__func__);
		ret = request_threaded_irq(client->irq, NULL, himax_ts_thread,IRQF_TRIGGER_LOW | IRQF_ONESHOT, client->name, ts);

		if (ret == 0) {
			ts->irq_enabled = 1;
			irq_enable_count = 1;
			himax_int_enable(ts->client->irq,0);
			tp_log_info("%s: irq register at qpio: %d\n", __func__, client->irq);
		} else {
			tp_log_err("%s: request_irq failed\n", __func__);
		}
	} else {
		tp_log_info("%s: client->irq is empty, use polling mode.\n", __func__);
	}

	return ret;
}

#ifdef CONFIG_FB
static void himax_fb_register(struct himax_ts_data *ts)
{
	int ret = 0;
	
	tp_log_info(" %s in", __func__);

	ts->fb_notif.notifier_call = fb_notifier_callback;
	ret = fb_register_client(&ts->fb_notif);
	if (ret)
		tp_log_err(" Unable to register fb_notifier: %d\n", ret);
}
#endif


#ifdef CONFIG_OF
static int himax_parse_dt(struct himax_ts_data *ts,
				struct himax_i2c_platform_data *pdata)
{
	int rc, coords_size = 0;
	uint32_t coords[4] = {0};
	struct property *prop;
	struct device_node *dt = ts->client->dev.of_node;
	u32 data = 0;

	prop = of_find_property(dt, "himax,panel-coords", NULL);
	if (prop) {
		coords_size = prop->length / sizeof(u32);
		if (coords_size != 4)
			tp_log_debug(" %s:Invalid panel coords size %d", __func__, coords_size);
	}

	if (of_property_read_u32_array(dt, "himax,panel-coords", coords, coords_size) == 0) {
		pdata->abs_x_min = coords[0], pdata->abs_x_max = coords[1];
		pdata->abs_y_min = coords[2], pdata->abs_y_max = coords[3];
		tp_log_info(" DT-%s:panel-coords = %d, %d, %d, %d\n", __func__, pdata->abs_x_min,
				pdata->abs_x_max, pdata->abs_y_min, pdata->abs_y_max);
	}
	else
	{
		//if can't find dts, use default value.
		pdata->abs_x_max =1200;
		pdata->abs_y_max =1920;
	}


	prop = of_find_property(dt, "himax,display-coords", NULL);
	if (prop) {
		coords_size = prop->length / sizeof(u32);
		if (coords_size != 4)
			tp_log_debug(" %s:Invalid display coords size %d", __func__, coords_size);
	}
	rc = of_property_read_u32_array(dt, "himax,display-coords", coords, coords_size);
	if (rc && (rc != -EINVAL)) {
		tp_log_debug(" %s:Fail to read display-coords %d\n", __func__, rc);
		return rc;
	}
	pdata->screenWidth  = coords[1];//test
	pdata->screenHeight = coords[3];//test

	tp_log_info(" DT-%s:display-coords = (%d, %d)", __func__, pdata->screenWidth,
		pdata->screenHeight);

	pdata->gpio_irq = of_get_named_gpio(dt, "himax,irq-gpio", 0);
	if (!gpio_is_valid(pdata->gpio_irq)) {
		tp_log_info(" DT:gpio_irq value is not valid\n");
	}

	pdata->gpio_reset = of_get_named_gpio(dt, "himax,rst-gpio", 0);
	if (!gpio_is_valid(pdata->gpio_reset)) {
		tp_log_info(" DT:gpio_rst value is not valid\n");
	}

	pdata->gpio_3v3_en = of_get_named_gpio(dt, "himax,vdd_ana-supply", 0);
	if (!gpio_is_valid(pdata->gpio_3v3_en)) {
		tp_log_info(" DT:gpio_3v3_en value is not valid\n");
	}

	pdata->gpio_1v8_en = of_get_named_gpio(dt, "himax,vcc_i2c-supply", 0);
	if (!gpio_is_valid(pdata->gpio_1v8_en)) {
		tp_log_info(" DT:pdata->gpio_1v8_en is not valid\n");
	}

	tp_log_info(" DT:gpio_irq=%d, gpio_rst=%d, gpio_3v3_en=%d,gpio_1v8_en=%d\n", pdata->gpio_irq, pdata->gpio_reset, pdata->gpio_3v3_en,pdata->gpio_1v8_en);

	if (of_property_read_u32(dt, "report_type", &data) == 0) {
		pdata->protocol_type = data;
		tp_log_info(" DT:protocol_type=%d", pdata->protocol_type);
	}

	return 0;
}
#endif


static int himax852xf_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = -ENOMEM;
	int recovery_flag = 0;
	struct himax_ts_data *ts;
	struct himax_i2c_platform_data *pdata;
	tp_log_info("%s:Enter \n", __func__);
	//Check I2C functionality
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		tp_log_err("%s: i2c check functionality error\n", __func__);
		err = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(struct himax_ts_data), GFP_KERNEL);
	if (ts == NULL) {
		tp_log_err("%s: allocate himax_ts_data failed\n", __func__);
		err = -ENOMEM;
		goto err_alloc_data_failed;
	}

	i2c_set_clientdata(client, ts);
	ts->client = client;
	ts->dev = &client->dev;

#if 0
	/* Remove this since this will cause request_firmware fail.
	   Thie line has no use to touch function. */
	dev_set_name(ts->dev, HIMAX852xf_NAME);
#endif

	pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
	if (pdata == NULL) {
		err = -ENOMEM;
		goto err_dt_platform_data_fail;
	}
#ifdef CONFIG_OF
	if (client->dev.of_node) { /*DeviceTree Init Platform_data*/
		err = himax_parse_dt(ts, pdata);
		if (err < 0) {
			tp_log_info(" pdata is NULL for DT\n");
			goto err_alloc_dt_pdata_failed;
		}
	}
#else
		pdata = client->dev.platform_data;
		if (pdata == NULL) {
			tp_log_info(" pdata is NULL(dev.platform_data)\n");
			goto err_get_platform_data_fail;
		}
#endif

	ts->rst_gpio = pdata->gpio_reset;

	err = himax_gpio_power_config(ts->client, pdata);
	if (err) {
		tp_log_err("%s: himax_gpio_power_config fail\n", __func__);
		goto err_himax_gpio_power_config;
	}

	err = himax_gpio_power_on(ts->client, pdata);
	if (err) {
		tp_log_err("%s: himax_gpio_power_on fail\n", __func__);
		goto err_himax_gpio_power_on;
	}

#ifndef CONFIG_OF
	if (pdata->power) {
		tp_log_info("%s:enter pdata->power\n", __func__);
		err = pdata->power(1);
		if (err < 0) {
			tp_log_err("%s: power on failed\n", __func__);
			goto err_power_failed;
		}
	}
#endif
	private_ts = ts;

#ifdef CONFIG_HUAWEI_DSM
	hmx_tp_dclient = dsm_register_client(&dsm_hmx_tp);
	if (!hmx_tp_dclient)
	{
		tp_log_err("%s: dsm register client failed\n", __func__);
		goto err_dsm_register_failed;
	}
	hmx_tp_dsm_info.irq_gpio = pdata->gpio_irq;
	hmx_tp_dsm_info.rst_gpio = pdata->gpio_reset;
#endif/*CONFIG_HUAWEI_DSM*/
	//Get Himax IC Type / FW information / Calculate the point number/HX8529  F series
	if (himax_ic_package_check(ts) == false) {
		tp_log_err("Himax chip does NOT EXIST");
		goto err_ic_package_failed;
	}

	if (pdata->virtual_key)
		ts->button = pdata->virtual_key;
	
#ifdef  HX_TP_PROC_FLASH_DUMP
		ts->flash_wq = create_singlethread_workqueue("himax_flash_wq");
		if (!ts->flash_wq)
		{
			tp_log_err("%s: create flash workqueue failed\n", __func__);
			err = -ENOMEM;
			goto err_create_flash_wq_failed;
		}

		INIT_WORK(&ts->flash_work, himax_ts_flash_work_func);

		setSysOperation(0);
		setFlashBuffer();
#endif

	himax_read_TP_info(client);

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	/* detect current device successful, set the flag as present */	
	set_hw_dev_flag(DEV_I2C_TOUCH_PANEL);
#endif

#ifdef HX_AUTO_UPDATE_FW

#ifdef HX_UPDATE_WITH_BIN_BUILDIN
	ts->himax_update_firmware = create_singlethread_workqueue("himax_update_firmware");
	if (!ts->himax_update_firmware) {
		tp_log_err("%s: himax  create workqueue failed\n", __func__);
		err = -ENOMEM;
		goto err_create_fwu_wq_failed;
	}
	INIT_DELAYED_WORK(&ts->himax_update_work, himax_update_fw_func);
	recovery_flag = get_boot_into_recovery_flag();
	if(1 == recovery_flag) {
		tp_log_info("%s, recovery mode not update firmware\n", __func__);
	} else {
		queue_delayed_work(ts->himax_update_firmware, &ts->himax_update_work, msecs_to_jiffies(2000));
		tp_log_info("himax update workqueque delay time start ");
	}
#endif

#else
	if(himax_calculateChecksum(false)==0)
		goto err_FW_CRC_failed;
#endif

	//Himax Power On and Load Config
	if (himax_loadSensorConfig(client, pdata) < 0) {
		tp_log_err("%s: Load Sesnsor configuration failed, unload driver.\n", __func__);
		goto err_detect_failed;
	}

	himax_get_information(client);

	calculate_point_number();
#ifdef HX_TP_PROC_DIAG
	setXChannel(HX_RX_NUM); // X channel
	setYChannel(HX_TX_NUM); // Y channel

	setMutualBuffer();
	if (getMutualBuffer() == NULL) {
		tp_log_err("%s: mutual buffer allocate fail failed\n", __func__);
		goto err_setchannel_failed;
	}
#endif
#ifdef CONFIG_OF
	ts->power = pdata->power;
#endif
	ts->pdata = pdata;

	ts->x_channel = HX_RX_NUM;
	ts->y_channel = HX_TX_NUM;
	ts->nFinger_support = HX_MAX_PT;
	//calculate the i2c data size
	calcDataSize(ts->nFinger_support);
	tp_log_info("%s: calcDataSize complete\n", __func__);

	ts->pdata->abs_pressure_min        = 0;
	ts->pdata->abs_pressure_max        = 200;
	ts->pdata->abs_width_min           = 0;
	ts->pdata->abs_width_max           = 200;
	pdata->cable_config[0]             = 0x90;
	pdata->cable_config[1]             = 0x00;

	ts->suspended                      = false;
	ts->protocol_type = pdata->protocol_type;
	tp_log_info("%s: Use Protocol Type %c\n", __func__,
	ts->protocol_type == PROTOCOL_TYPE_A ? 'A' : 'B');

	err = himax_input_register(ts);
	if (err) {
		tp_log_err("%s: Unable to register %s input device\n",
			__func__, ts->input_dev->name);
		goto err_input_register_device_failed;
	}

#ifdef CONFIG_FB
	himax_fb_register(ts);

#elif defined(CONFIG_HAS_EARLYSUSPEND)
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_STOP_DRAWING + 1;
	ts->early_suspend.suspend = himax_ts_early_suspend;
	ts->early_suspend.resume = himax_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

#ifdef HX_CHIP_STATUS_MONITOR//for ESD solution
	ts->himax_chip_monitor_wq = create_singlethread_workqueue("himax_chip_monitor_wq");
	if (!ts->himax_chip_monitor_wq)
	{
		tp_log_err(" %s: create workqueue failed\n", __func__);
		err = -ENOMEM;
		goto err_create_chip_monitor_wq_failed;
	}

	INIT_DELAYED_WORK(&ts->himax_chip_monitor, himax_chip_monitor_function);
	queue_delayed_work(ts->himax_chip_monitor_wq, &ts->himax_chip_monitor, HX_POLLING_TIMER*HZ);
#endif

wake_lock_init(&ts->ts_flash_wake_lock, WAKE_LOCK_SUSPEND, HIMAX852xf_NAME);

#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
	himax_touch_proc_init();
#endif

	atomic_set(&ts->suspend_mode, 0);
	atomic_set(&ts->irq_complete, 1);
	himax_factory_test_init();

#ifdef HX_ESD_WORKAROUND
	ESD_RESET_ACTIVATE = 0;
#endif
	HW_RESET_ACTIVATE = 0;


	err = himax_ts_register_interrupt(ts->client);
	if (err)
		goto err_register_interrupt_failed;

	//himax_set_app_info(ts);

	tp_log_info("%s:exit\n", __func__);

	return 0;

err_register_interrupt_failed:


#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
	himax_touch_proc_deinit();
#endif
wake_lock_destroy(&ts->ts_flash_wake_lock);
#ifdef  HX_CHIP_STATUS_MONITOR
	cancel_delayed_work_sync(&ts->himax_chip_monitor);
	destroy_workqueue(ts->himax_chip_monitor_wq);
err_create_chip_monitor_wq_failed:
#endif
#ifdef CONFIG_FB
	if (fb_unregister_client(&ts->fb_notif))
			tp_log_info("Error occurred while unregistering fb_notifier.\n");
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&ts->early_suspend);
#endif
	input_free_device(ts->input_dev);
err_input_register_device_failed:
#ifdef HX_TP_PROC_DIAG
err_setchannel_failed:
#endif
err_detect_failed:
	destroy_workqueue(ts->flash_wq);

#ifdef HX_AUTO_UPDATE_FW

#ifdef HX_UPDATE_WITH_BIN_BUILDIN
err_create_fwu_wq_failed:
#endif

#else
err_FW_CRC_failed:
#endif

#ifdef  HX_TP_PROC_FLASH_DUMP
	destroy_workqueue(ts->flash_wq);
err_create_flash_wq_failed:
#endif


err_ic_package_failed:
#ifdef CONFIG_HUAWEI_DSM
	if (hmx_tp_dclient) {
		dsm_unregister_client(hmx_tp_dclient, &dsm_hmx_tp);
		hmx_tp_dclient = NULL;
	}
err_dsm_register_failed:
#endif/*CONFIG_HUAWEI_DSM*/
#ifndef CONFIG_OF
err_power_failed:
#endif
	himax_gpio_power_off(pdata);
err_himax_gpio_power_on:
	himax_gpio_power_deconfig(pdata);
err_himax_gpio_power_config:
#ifdef CONFIG_OF
err_alloc_dt_pdata_failed:
#else
err_get_platform_data_fail:
#endif
	kfree(pdata);
err_dt_platform_data_fail:
	i2c_set_clientdata(client, NULL);
	kfree(ts);

err_alloc_data_failed:
err_check_functionality_failed:

	return err;

}
int PowerOnSeq(struct himax_ts_data *ts)
{
	if (himax_loadSensorConfig(ts->client, ts->pdata) < 0) {
		tp_log_err("%s: Load Sesnsor configuration failed, unload driver.\n", __func__);
		return 1;
	}
	himax_get_information(ts->client);  
	calculate_point_number();
#ifdef HX_TP_PROC_DIAG
	setXChannel(HX_RX_NUM); // X channel
	setYChannel(HX_TX_NUM); // Y channel
	setMutualBuffer();
	if (getMutualBuffer() == NULL) {
		tp_log_err("%s: mutual buffer allocate fail failed\n", __func__);
		return 1;
	}
#endif
	ts->x_channel = HX_RX_NUM;
	ts->y_channel = HX_TX_NUM;
	ts->nFinger_support = HX_MAX_PT;
	calcDataSize(ts->nFinger_support);
	himax_ts_register_interrupt(ts->client);
	return 0;
}

static int himax852xf_remove(struct i2c_client *client)
{
	struct himax_ts_data *ts = i2c_get_clientdata(client);

	struct himax_i2c_platform_data *pdata = ts->pdata;

	tp_log_info("%s:Enter \n", __func__);

	if(ts->irq_enabled){
		himax_int_enable(ts->client->irq,0);
			free_irq(ts->client->irq, ts);
	}

#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
	himax_touch_proc_deinit();
#endif

	wake_lock_destroy(&ts->ts_flash_wake_lock);

#ifdef HX_CHIP_STATUS_MONITOR
		cancel_delayed_work_sync(&ts->himax_chip_monitor);
	destroy_workqueue(ts->himax_chip_monitor_wq);
#endif
#ifdef CONFIG_FB
		if (fb_unregister_client(&ts->fb_notif))
			tp_log_info("Error occurred while unregistering fb_notifier.\n");
#elif defined(CONFIG_HAS_EARLYSUSPEND)
		unregister_early_suspend(&ts->early_suspend);
#endif
		input_free_device(ts->input_dev);
#ifdef  HX_TP_PROC_FLASH_DUMP
		destroy_workqueue(ts->flash_wq);
#endif

	himax_gpio_power_off(pdata);
	himax_gpio_power_deconfig(pdata);

	kfree(pdata);

	i2c_set_clientdata(client, NULL);
	kfree(ts);

	return 0;
}

void report_up(struct device *dev)
{
	int i=0; 
	struct himax_ts_data *ts;
	ts = dev_get_drvdata(dev);

	// leave event
	for (i = 0; i < ts->nFinger_support; i++) {
		input_mt_slot(ts->input_dev, i);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, 0);
		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
		input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0);
		input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
	}
	input_report_key(ts->input_dev, BTN_TOUCH, 0);
	input_sync(ts->input_dev);
}

static int himax852xf_suspend(struct device *dev)
{
//	int ret;
//	uint8_t buf[2] = {0};
#ifdef HX_CHIP_STATUS_MONITOR
	int t=0;
#endif
	struct himax_ts_data *ts;
	int iCount = 0;
	tp_log_info("%s: Enter suspended. \n", __func__);

	ts = dev_get_drvdata(dev);
	if(ts->suspended)
	{
		tp_log_info("%s: Already suspended. Skipped. \n", __func__);
		return 0;
	}
	else
	{
		ts->suspended = true;
		tp_log_info("%s: enter \n", __func__);
	}

#ifdef HX_TP_PROC_FLASH_DUMP
	if (getFlashDumpGoing())
	{
		tp_log_info("[himax] %s: Flash dump is going, reject suspend\n",__func__);
		return 0;
	}
#endif

#ifdef HX_CHIP_STATUS_MONITOR
	if(HX_ON_HAND_SHAKING)//chip on hand shaking,wait hand shaking
	{
		for(t=0; t<100; t++)
			{
				if(HX_ON_HAND_SHAKING==0)//chip on hand shaking end
					{
						tp_log_info("%s:HX_ON_HAND_SHAKING OK check %d times\n",__func__,t);
						break;
					}
				else
					msleep(1);
			}
		if(t==100)
			{
				tp_log_err("%s:HX_ON_HAND_SHAKING timeout reject suspend\n",__func__);
				return 0;
			}
	}
#endif


	himax_int_enable(ts->client->irq,0);

#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	cancel_delayed_work_sync(&ts->himax_chip_monitor);
#endif

	//Himax 852xf IC enter sleep mode
	/*buf[0] = HX_CMD_TSSOFF;
	ret = i2c_himax_master_write(ts->client, buf, 1, DEFAULT_RETRY_CNT);
	if (ret < 0)
	{
		tp_log_err("[himax] %s: I2C access failed addr = 0x%x\n", __func__, ts->client->addr);
	}
	msleep(40);

	buf[0] = HX_CMD_TSSLPIN;
	ret = i2c_himax_master_write(ts->client, buf, 1, DEFAULT_RETRY_CNT);
	if (ret < 0)
	{
		tp_log_err("[himax] %s: I2C access failed addr = 0x%x\n", __func__, ts->client->addr);
	}*/


	ts->first_pressed = 0;
	atomic_set(&ts->suspend_mode, 1);
	ts->pre_finger_mask = 0;
	if (ts->pdata->powerOff3V3 && ts->pdata->power)
		ts->pdata->power(0);

	msleep(30);
	if((atomic_read(&ts->irq_complete) == 0) && (iCount <10) ){
		tp_log_info("%s irq function not complete. try time is %d\n",__func__, iCount);
		msleep(10);
		iCount ++;
	}
	/*power down*/
	gpio_direction_output(ts->pdata->gpio_3v3_en, 0);
	gpio_direction_output(ts->pdata->gpio_1v8_en, 0);

	gpio_direction_output(ts->rst_gpio, 0);

	report_up(dev);

	tp_log_info("%s: exit \n", __func__);

	return 0;
}

static int himax852xf_resume(struct device *dev)
{
#ifdef HX_CHIP_STATUS_MONITOR
	int t=0;
#endif
	struct himax_ts_data *ts;
#if 1
	char data[12] = {0};
#endif

	tp_log_info("%s: enter \n", __func__);
	ts = dev_get_drvdata(dev);

	if(ts->suspended)
	{
		tp_log_info("%s: will be resume \n", __func__);
	}
	else
	{
		tp_log_info("%s: Already resumed. Skipped. \n", __func__);
		return 0;
	}

	if (ts->pdata->powerOff3V3 && ts->pdata->power){
		tp_log_info("enter resume ts->pdata->powerOff3V3 && ts->pdata->power \n");
		ts->pdata->power(1);
	}

	//himax_rst_gpio_set(ts->rst_gpio, 0);
	//mdelay(5);
	/*power on*/
	gpio_direction_output(ts->pdata->gpio_3v3_en, 1);
	udelay(80);
	gpio_direction_output(ts->pdata->gpio_1v8_en, 1);

	tp_log_info("%s: power on. \n", __func__);

	HW_RESET_ACTIVATE = 1;

	//mdelay(5);
	//himax_HW_reset(true, true);
#if 1
	himax_rst_gpio_set(ts->rst_gpio, 0);
	mdelay(5);
	himax_rst_gpio_set(ts->rst_gpio, 1);
	msleep(20);
	tp_log_info("%s: pull reset gpio on. \n", __func__);

	data[0] = 0x97; data[1] = 0x06; data[2] = 0x03;//adjust flash clock
	i2c_himax_master_write(ts->client, &data[0],3,DEFAULT_RETRY_CNT);
	mdelay(1);

	data[0] = 0xA6; data[1] = 0x11; data[2] = 0x00;//switch to mcu
	i2c_himax_master_write(ts->client, &data[0],3,DEFAULT_RETRY_CNT);
	mdelay(1);
	tp_log_info("%s: flash clock ok. \n", __func__);

	//Sense On
	i2c_himax_write_command(ts->client, HX_CMD_TSSON, DEFAULT_RETRY_CNT);
	mdelay(1);
	i2c_himax_write_command(ts->client, HX_CMD_TSSLPOUT, DEFAULT_RETRY_CNT);
	msleep(30);

	himax_rst_gpio_set(ts->rst_gpio, 0);
	mdelay(5);
	himax_rst_gpio_set(ts->rst_gpio, 1);
	msleep(20);
	tp_log_info("%s: 2: pull reset gpio on. \n", __func__);
	data[0] = 0x97; data[1] = 0x06; data[2] = 0x03;//adjust flash clock
	i2c_himax_master_write(ts->client, &data[0],3,DEFAULT_RETRY_CNT);
	mdelay(1);
	data[0] = 0xA6; data[1] = 0x11; data[2] = 0x00;//switch to mcu
	i2c_himax_master_write(ts->client, &data[0],3,DEFAULT_RETRY_CNT);
	mdelay(1);
	tp_log_info("%s: 2: flash clock ok. \n", __func__);
	i2c_himax_write_command(ts->client, HX_CMD_TSSON, DEFAULT_RETRY_CNT);
	mdelay(1);
	i2c_himax_write_command(ts->client, HX_CMD_TSSLPOUT, DEFAULT_RETRY_CNT);
#endif

#ifdef HX_CHIP_STATUS_MONITOR
	if(HX_ON_HAND_SHAKING)//chip on hand shaking,wait hand shaking
	{
		for(t=0; t<100; t++)
			{
				if(HX_ON_HAND_SHAKING==0)//chip on hand shaking end
					{
						tp_log_info("%s:HX_ON_HAND_SHAKING OK check %d times\n",__func__,t);
						break;
					}
				else
					msleep(1);
			}
		if(t==100)
			{
				tp_log_err("%s:HX_ON_HAND_SHAKING timeout reject resume\n",__func__);
				return 0;
			}
	}
#endif

#if 0
	//Sense On
	i2c_himax_write_command(ts->client, HX_CMD_TSSON, DEFAULT_RETRY_CNT);
	msleep(30);
	i2c_himax_write_command(ts->client, HX_CMD_TSSLPOUT, DEFAULT_RETRY_CNT);
#endif
	atomic_set(&ts->suspend_mode, 0);
	report_up(dev);

	himax_int_enable(ts->client->irq,1);

#ifdef HX_CHIP_STATUS_MONITOR
	HX_CHIP_POLLING_COUNT = 0;
	queue_delayed_work(ts->himax_chip_monitor_wq, &ts->himax_chip_monitor, HX_POLLING_TIMER*HZ); //for ESD solution
#endif

	ts->suspended = false;

	tp_log_info("%s: exit \n", __func__);

	return 0;
}


#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *self,
				 unsigned long event, void *data)
{
	struct fb_event *evdata = data;
	int *blank;
	struct himax_ts_data *ts=
		container_of(self, struct himax_ts_data, fb_notif);

	//tp_log_info(" %s enter\n", __func__);

	if(HX_UPDATE_FLAG==1)
	{
		HX_RESET_COUNT++;
		tp_log_info("HX still in updateing no reset ");
		return 0;
	}

	if(atomic_read(&hmx_mmi_test_status)) {
		tp_log_info("HX still in factory test, no suspend ");
		return 0;
	}

	if (evdata && evdata->data && event == FB_EVENT_BLANK && ts &&
			ts->client) {
		blank = evdata->data;

		tp_log_info("%s: blank is %d!\n", __func__, *blank);

		switch (*blank) {
		case FB_BLANK_UNBLANK:
			himax852xf_resume(&ts->client->dev);
		break;

		case FB_BLANK_POWERDOWN:
		case FB_BLANK_HSYNC_SUSPEND:
		case FB_BLANK_VSYNC_SUSPEND:
		case FB_BLANK_NORMAL:
			himax852xf_suspend(&ts->client->dev);
		break;
		}
	}

	return 0;
}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void himax_ts_early_suspend(struct early_suspend *h)
{
	struct himax_ts_data *ts;
	ts = container_of(h, struct himax_ts_data, early_suspend);

	himax852xf_suspend(ts->client, PMSG_SUSPEND);
}

static void himax_ts_late_resume(struct early_suspend *h)
{
	struct himax_ts_data *ts;
	ts = container_of(h, struct himax_ts_data, early_suspend);

	himax852xf_resume(ts->client);
}
#endif

static const struct i2c_device_id himax852xf_ts_id[] = {
	{HIMAX852xf_NAME, 0 },
	{}
};


#ifdef CONFIG_OF
static struct of_device_id himax_match_table[] = {
	{.compatible = "himax,852xf" },
	{},
};
#else
#define himax_match_table NULL
#endif



static struct i2c_driver himax852xf_driver = {
	.id_table	= himax852xf_ts_id,
	.probe		= himax852xf_probe,
	.remove		= himax852xf_remove,
	.driver		= {
		.name = HIMAX852xf_NAME,
		.owner = THIS_MODULE,
		.of_match_table = himax_match_table,
	},
};

static void __init himax852xf_init_async(void *unused, async_cookie_t cookie)
{
	tp_log_info("%s:Enter \n", __func__);
	i2c_add_driver(&himax852xf_driver);
}
static int __init himax852xf_init(void)
{
	tp_log_info("Himax 852xf touch panel driver init\n");
	async_schedule(himax852xf_init_async, NULL);
	return 0;
}

static void __exit himax852xf_exit(void)
{
	tp_log_info("%s:Enter \n", __func__);
	i2c_del_driver(&himax852xf_driver);
}

module_init(himax852xf_init);
module_exit(himax852xf_exit);

MODULE_DESCRIPTION("Himax852xf driver");
MODULE_LICENSE("GPL");
/* CPM2016032400222 shihuijun 20160324 end > */

