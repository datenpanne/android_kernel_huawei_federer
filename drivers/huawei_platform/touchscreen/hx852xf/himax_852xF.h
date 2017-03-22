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

#ifndef HIMAX852xF_H
#define HIMAX852xF_H

#include <asm/segment.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/async.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/input/mt.h>
#include <linux/firmware.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/wakelock.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include "himax_platform.h"

#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif

#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif

#define HIMAX_DRIVER_VER "0.0.10.1"

#define HIMAX852xf_NAME "Himax852xf"


#define INPUT_DEV_NAME	"himax-touchscreen"
#define FLASH_DUMP_FILE "/data/user/Flash_Dump.bin"


#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)

#define HX_TP_PROC_DIAG
#define HX_TP_PROC_RESET
#define HX_TP_PROC_REGISTER
#define HX_TP_PROC_DEBUG
#define HX_TP_PROC_FLASH_DUMP
#define HX_TP_PROC_SELF_TEST

#ifdef HX_TP_PROC_SELF_TEST
#define HIMAX_GOLDEN_SELF_TEST
#endif

#endif
//===========Himax Option function=============

//#define HX_LOADIN_CONFIG
#define HX_AUTO_UPDATE_FW

#ifdef HX_AUTO_UPDATE_FW
#define HX_UPDATE_WITH_BIN_BUILDIN
#endif
//#define HX_AUTO_UPDATE_CONFIG		//if enable HX_AUTO_UPDATE_CONFIG, need to disable HX_LOADIN_CONFIG

//#define HX_ESD_WORKAROUND
//#define HX_CHIP_STATUS_MONITOR		//for ESD 2nd solution,default off


#define HX_85XX_A_SERIES_PWON		1
#define HX_85XX_B_SERIES_PWON		2
#define HX_85XX_C_SERIES_PWON		3
#define HX_85XX_D_SERIES_PWON		4
#define HX_85XX_E_SERIES_PWON		5
#define HX_85XX_ES_SERIES_PWON		6
#define HX_85XX_F_SERIES_PWON		7

#define HX_TP_BIN_CHECKSUM_SW		1
#define HX_TP_BIN_CHECKSUM_HW		2
#define HX_TP_BIN_CHECKSUM_CRC		3

#define HX_KEY_MAX_COUNT             4
#define DEFAULT_RETRY_CNT            5

#define HX_VKEY_0   KEY_BACK
#define HX_VKEY_1   KEY_HOME
#define HX_VKEY_2   KEY_RESERVED
#define HX_VKEY_3   KEY_RESERVED
#define HX_KEY_ARRAY    {HX_VKEY_0, HX_VKEY_1, HX_VKEY_2, HX_VKEY_3}

#define SHIFTBITS 5
#define FLASH_SIZE 65536

#ifdef CONFIG_HUAWEI_DSM


#include <linux/regulator/consumer.h>
#include <dsm/dsm_pub.h>

enum hmx_tp_error_state {
	FWU_GET_SYSINFO_FAIL = 0,
	FWU_GENERATE_FW_NAME_FAIL,
	FWU_REQUEST_FW_FAIL,
	FWU_FW_CRC_ERROR,
	FWU_FW_WRITE_ERROR,
	TP_WOKR_READ_DATA_ERROR = 5,
	TP_WOKR_CHECKSUM_INFO_ERROR,
	TP_WOKR_CHECKSUM_ALL_ERROR,
	TP_WOKR_HEAD_ERROR,
	TS_UPDATE_STATE_UNDEFINE = 255,
};
struct hmx_dsm_info {
	int irq_gpio;
	int rst_gpio;
	int I2C_status;
	int UPDATE_status;
	int WORK_status;
	int ESD_status;
	int rawdata_status;
};
extern struct hmx_dsm_info hmx_tp_dsm_info;
extern struct dsm_client *hmx_tp_dclient;
#endif
struct himax_virtual_key {
	int index;
	int keycode;
	int x_range_min;
	int x_range_max;
	int y_range_min;
	int y_range_max;
};

struct himax_config {
	uint8_t  default_cfg;
	uint8_t  sensor_id;
	uint8_t  fw_ver;
	uint16_t length;
	uint32_t tw_x_min;
	uint32_t tw_x_max;
	uint32_t tw_y_min;
	uint32_t tw_y_max;
	uint32_t pl_x_min;
	uint32_t pl_x_max;
	uint32_t pl_y_min;
	uint32_t pl_y_max;
	uint8_t c1[11];
	uint8_t c2[11];
	uint8_t c3[11];
	uint8_t c4[11];
	uint8_t c5[11];
	uint8_t c6[11];
	uint8_t c7[11];
	uint8_t c8[11];
	uint8_t c9[11];
	uint8_t c10[11];
	uint8_t c11[11];
	uint8_t c12[11];
	uint8_t c13[11];
	uint8_t c14[11];
	uint8_t c15[11];
	uint8_t c16[11];
	uint8_t c17[11];
	uint8_t c18[17];
	uint8_t c19[15];
	uint8_t c20[5];
	uint8_t c21[11];
	uint8_t c22[4];
	uint8_t c23[3];
	uint8_t c24[3];
	uint8_t c25[4];
	uint8_t c26[2];
	uint8_t c27[2];
	uint8_t c28[2];
	uint8_t c29[2];
	uint8_t c30[2];
	uint8_t c31[2];
	uint8_t c32[2];
	uint8_t c33[2];
	uint8_t c34[2];
	uint8_t c35[3];
	uint8_t c36[5];
	uint8_t c37[5];
	uint8_t c38[9];
	uint8_t c39[14];
	uint8_t c40[159];
	uint8_t c41[99];
};

struct himax_ts_data {
	bool suspended;
	atomic_t suspend_mode;
	atomic_t irq_complete;
	uint8_t x_channel;
	uint8_t y_channel;
	uint8_t useScreenRes;
	uint8_t diag_command;
	uint8_t vendor_fw_ver_H;
	uint8_t vendor_fw_ver_L;
	uint8_t vendor_config_ver;
	uint8_t vendor_sensor_id;

	uint8_t protocol_type;
	uint8_t first_pressed;
	uint8_t coord_data_size;
	uint8_t area_data_size;
	uint8_t raw_data_frame_size;
	uint8_t raw_data_nframes;
	uint8_t nFinger_support;
	uint8_t irq_enabled;

	uint16_t finger_pressed;
	uint16_t last_slot;
	uint16_t pre_finger_mask;

	uint32_t debug_log_level;
	uint32_t widthFactor;
	uint32_t heightFactor;
	uint32_t tw_x_min;
	uint32_t tw_x_max;
	uint32_t tw_y_min;
	uint32_t tw_y_max;
	uint32_t pl_x_min;
	uint32_t pl_x_max;
	uint32_t pl_y_min;
	uint32_t pl_y_max;

	int (*power)(int on);
	int pre_finger_data[10][2];

	struct device *dev;
	struct input_dev *input_dev;
	struct i2c_client *client;
	struct himax_i2c_platform_data *pdata;
	struct himax_virtual_key *button;
	struct wake_lock ts_flash_wake_lock;

#if defined(CONFIG_FB)
	struct notifier_block fb_notif;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
#ifdef HX_CHIP_STATUS_MONITOR
	struct workqueue_struct *himax_chip_monitor_wq;
	struct delayed_work himax_chip_monitor;
#endif

#ifdef HX_UPDATE_WITH_BIN_BUILDIN
	struct workqueue_struct *himax_update_firmware;
	struct delayed_work himax_update_work;
#endif 
#ifdef HX_TP_PROC_FLASH_DUMP
	struct workqueue_struct 			*flash_wq;
	struct work_struct 					flash_work;
#endif
	struct pinctrl *pctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_idle;
	int rst_gpio;
};

extern struct himax_ts_data *private_ts;

#define HX_CMD_NOP					 0x00
#define HX_CMD_SETMICROOFF			 0x35
#define HX_CMD_SETROMRDY			 0x36
#define HX_CMD_TSSLPIN				 0x80
#define HX_CMD_TSSLPOUT 			 0x81
#define HX_CMD_TSSOFF				 0x82
#define HX_CMD_TSSON				 0x83
#define HX_CMD_ROE					 0x85
#define HX_CMD_RAE					 0x86
#define HX_CMD_RLE					 0x87
#define HX_CMD_CLRES				 0x88
#define HX_CMD_TSSWRESET			 0x9E
#define HX_CMD_SETDEEPSTB			 0xD7
#define HX_CMD_SET_CACHE_FUN		 0xDD
#define HX_CMD_SETIDLE				 0xF2
#define HX_CMD_SETIDLEDELAY 		 0xF3
#define HX_CMD_SELFTEST_BUFFER		 0x8D
#define HX_CMD_MANUALMODE			 0x42
#define HX_CMD_FLASH_ENABLE 		 0x43
#define HX_CMD_FLASH_SET_ADDRESS	 0x44
#define HX_CMD_FLASH_WRITE_REGISTER  0x45
#define HX_CMD_FLASH_SET_COMMAND	 0x47
#define HX_CMD_FLASH_WRITE_BUFFER	 0x48
#define HX_CMD_FLASH_PAGE_ERASE 	 0x4D
#define HX_CMD_FLASH_SECTOR_ERASE	 0x4E
#define HX_CMD_CB					 0xCB
#define HX_CMD_EA					 0xEA
#define HX_CMD_4A					 0x4A
#define HX_CMD_4F					 0x4F
#define HX_CMD_B9					 0xB9
#define HX_CMD_76					 0x76

#define HX_VER_FW_MAJ				 0x33
#define HX_VER_FW_MIN				 0x32
#define HX_VER_FW_CFG				 0x39

enum input_protocol_type {
	PROTOCOL_TYPE_A	= 0x00,
	PROTOCOL_TYPE_B	= 0x01,
};


#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)

	extern uint8_t HX_PROC_SEND_FLAG;
	extern int  touch_monitor_stop_flag;
	extern uint8_t diag_coor[128];// = {0xFF};

#ifdef HX_TP_PROC_DIAG

	extern uint8_t *getMutualBuffer(void);
	extern uint8_t *getSelfBuffer(void);
	extern uint8_t 	getDiagCommand(void);
	extern void setDiagCommand(uint8_t cmd);
	extern uint8_t 	getXChannel(void);
	extern uint8_t 	getYChannel(void);

	extern void 	setMutualBuffer(void);
	extern void 	setXChannel(uint8_t x);
	extern void 	setYChannel(uint8_t y);
#endif


#ifdef HX_TP_PROC_FLASH_DUMP
	extern bool  getFlashDumpGoing(void);
	extern void setFlashBuffer(void);
	extern void setSysOperation(uint8_t operation);
	extern void himax_ts_flash_work_func(struct work_struct *work);
#endif


#endif

#ifdef HX_ESD_WORKAROUND
	extern u8		ESD_R36_FAIL;
#endif

	void himax_HW_reset(uint8_t loadconfig,uint8_t int_off);



#ifdef HX_AUTO_UPDATE_CONFIG
static int		CFB_START_ADDR					= 0;
static int		CFB_LENGTH						= 0;
static int		CFB_INFO_LENGTH 				= 0;
#endif

#ifdef HX_CHIP_STATUS_MONITOR
static int HX_CHIP_POLLING_COUNT;

static int HX_POLLING_TIMES		=2;//ex:5(timer)x2(times)=10sec(polling time)
extern int HX_ON_HAND_SHAKING;
#endif


extern int irq_enable_count;

extern int		HX_RX_NUM;
extern int		HX_TX_NUM;
extern int		HX_BT_NUM;
extern int		HX_X_RES;
extern int		HX_Y_RES;
extern int		HX_MAX_PT;
extern bool		HX_XY_REVERSE;


extern unsigned int	       FW_VER_MAJ_FLASH_ADDR;
extern unsigned int 	FW_VER_MAJ_FLASH_LENG;
extern unsigned int 	FW_VER_MIN_FLASH_ADDR;
extern unsigned int 	FW_VER_MIN_FLASH_LENG;

extern unsigned int 	FW_CFG_VER_FLASH_ADDR;

extern unsigned int 	CFG_VER_MAJ_FLASH_ADDR;
extern unsigned int 	CFG_VER_MAJ_FLASH_LENG;
extern unsigned int 	CFG_VER_MIN_FLASH_ADDR;
extern unsigned int 	CFG_VER_MIN_FLASH_LENG;

extern uint8_t	IC_STATUS_CHECK;

extern unsigned char	IC_CHECKSUM;
extern unsigned char	IC_TYPE;

extern int self_test_inter_flag;
extern atomic_t hmx_mmi_test_status;

//for update test
extern unsigned int   HX_UPDATE_FLAG;
//extern unsigned int   HX_FACTEST_FLAG;
extern unsigned int   HX_RESET_COUNT;
extern unsigned  int   HX_ESD_RESET_COUNT ;
extern u8 HW_RESET_ACTIVATE;
//for update test

//define in debug.c
extern int himax_touch_proc_init(void);
extern void himax_touch_proc_deinit(void);     //define here is ok


//define in fw_update.c
extern int himax_lock_flash(int enable);
extern int i_update_FW(void);
extern void himax_update_fw_func(struct work_struct *work) ;
extern void himax_power_on_initCMD(struct i2c_client *client);
extern int PowerOnSeq(struct himax_ts_data *ts);


//need to check in probe
extern uint8_t himax_calculateChecksum(bool change_iref);    //need use FW_VER_MAJ_FLASH_ADDR
extern int himax_loadSensorConfig(struct i2c_client *client, struct himax_i2c_platform_data *pdata); ////need after reset

extern irqreturn_t himax_ts_thread(int irq, void *ptr);
extern int himax_input_register(struct himax_ts_data *ts);

extern int fts_ctpm_fw_upgrade_with_fs(const unsigned char *fw, int len, bool change_iref);

extern int himax_hand_shaking(void);

//used in factory test
extern int himax_chip_self_test(void);
extern void himax_factory_test_init(void);
#ifdef CONFIG_HUAWEI_DSM
extern int hmx_tp_report_dsm_err(int type, int err_numb);
#endif
int himax_check_update_firmware_flag(void);
#endif
