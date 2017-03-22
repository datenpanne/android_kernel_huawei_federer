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
#if defined(HX_AUTO_UPDATE_FW)||defined(HX_AUTO_UPDATE_CONFIG)

#if 0
const struct firmware *fw_entry = NULL;
unsigned char *filename = NULL;
#endif 
#endif

unsigned int	FW_VER_MAJ_FLASH_ADDR;
unsigned int 	FW_VER_MAJ_FLASH_LENG;
unsigned int 	FW_VER_MIN_FLASH_ADDR;
unsigned int 	FW_VER_MIN_FLASH_LENG;

unsigned int 	FW_CFG_VER_FLASH_ADDR;

unsigned int 	CFG_VER_MAJ_FLASH_ADDR;
unsigned int 	CFG_VER_MAJ_FLASH_LENG;
unsigned int 	CFG_VER_MIN_FLASH_ADDR;
unsigned int 	CFG_VER_MIN_FLASH_LENG;

#define HX_FW_NAME "jordan_himax_auo_fw.bin"
#define BOOT_UPDATE_FIRMWARE_FLAG_FILENAME	"/system/etc/tp_test_parameters/boot_update_firmware.flag"
#define BOOT_UPDATE_FIRMWARE_FLAG "boot_update_firmware_flag:"

//for update test
unsigned int   HX_UPDATE_FLAG=0;
//unsigned int   HX_FACTEST_FLAG=0;
unsigned int   HX_RESET_COUNT=0;
unsigned  int   HX_ESD_RESET_COUNT =0;
//for update test 
static bool	config_load		= false;
static struct himax_config *config_selected = NULL;

static int iref_number = 11;//??
static bool iref_found = false;//??

//used in firmware upgrade
//1uA
static unsigned char E_IrefTable_1[16][2] = { {0x20,0x0F},{0x20,0x1F},{0x20,0x2F},{0x20,0x3F},
											{0x20,0x4F},{0x20,0x5F},{0x20,0x6F},{0x20,0x7F},
											{0x20,0x8F},{0x20,0x9F},{0x20,0xAF},{0x20,0xBF},
											{0x20,0xCF},{0x20,0xDF},{0x20,0xEF},{0x20,0xFF}};

//2uA
static unsigned char E_IrefTable_2[16][2] = { {0xA0,0x0E},{0xA0,0x1E},{0xA0,0x2E},{0xA0,0x3E},
											{0xA0,0x4E},{0xA0,0x5E},{0xA0,0x6E},{0xA0,0x7E},
											{0xA0,0x8E},{0xA0,0x9E},{0xA0,0xAE},{0xA0,0xBE},
											{0xA0,0xCE},{0xA0,0xDE},{0xA0,0xEE},{0xA0,0xFE}};

//3uA
static unsigned char E_IrefTable_3[16][2] = { {0x20,0x0E},{0x20,0x1E},{0x20,0x2E},{0x20,0x3E},
											{0x20,0x4E},{0x20,0x5E},{0x20,0x6E},{0x20,0x7E},
											{0x20,0x8E},{0x20,0x9E},{0x20,0xAE},{0x20,0xBE},
											{0x20,0xCE},{0x20,0xDE},{0x20,0xEE},{0x20,0xFE}};

//4uA
static unsigned char E_IrefTable_4[16][2] = { {0xA0,0x0D},{0xA0,0x1D},{0xA0,0x2D},{0xA0,0x3D},
											{0xA0,0x4D},{0xA0,0x5D},{0xA0,0x6D},{0xA0,0x7D},
											{0xA0,0x8D},{0xA0,0x9D},{0xA0,0xAD},{0xA0,0xBD},
											{0xA0,0xCD},{0xA0,0xDD},{0xA0,0xED},{0xA0,0xFD}};

//5uA
static unsigned char E_IrefTable_5[16][2] = { {0x20,0x0D},{0x20,0x1D},{0x20,0x2D},{0x20,0x3D},
											{0x20,0x4D},{0x20,0x5D},{0x20,0x6D},{0x20,0x7D},
											{0x20,0x8D},{0x20,0x9D},{0x20,0xAD},{0x20,0xBD},
											{0x20,0xCD},{0x20,0xDD},{0x20,0xED},{0x20,0xFD}};

//6uA
static unsigned char E_IrefTable_6[16][2] = { {0xA0,0x0C},{0xA0,0x1C},{0xA0,0x2C},{0xA0,0x3C},
											{0xA0,0x4C},{0xA0,0x5C},{0xA0,0x6C},{0xA0,0x7C},
											{0xA0,0x8C},{0xA0,0x9C},{0xA0,0xAC},{0xA0,0xBC},
											{0xA0,0xCC},{0xA0,0xDC},{0xA0,0xEC},{0xA0,0xFC}};

//7uA
static unsigned char E_IrefTable_7[16][2] = { {0x20,0x0C},{0x20,0x1C},{0x20,0x2C},{0x20,0x3C},
											{0x20,0x4C},{0x20,0x5C},{0x20,0x6C},{0x20,0x7C},
											{0x20,0x8C},{0x20,0x9C},{0x20,0xAC},{0x20,0xBC},
											{0x20,0xCC},{0x20,0xDC},{0x20,0xEC},{0x20,0xFC}}; 

unsigned char	IC_CHECKSUM               = 0;
unsigned char	IC_TYPE                   = 0;

int		HX_RX_NUM                 = 0;
int		HX_TX_NUM                 = 0;
int		HX_BT_NUM                 = 0;
int		HX_X_RES                  = 0;
int		HX_Y_RES                  = 0;
int		HX_MAX_PT                 = 0;
bool		HX_XY_REVERSE             = false;


#ifdef CONFIG_OF
#if defined(HX_LOADIN_CONFIG)||defined(HX_AUTO_UPDATE_CONFIG)
static int himax_parse_config(struct himax_ts_data *ts, struct himax_config *pdata);
{
	struct himax_config *cfg_table;
	struct device_node *node, *pp = NULL;
	struct property *prop;
	uint8_t cnt = 0, i = 0;
	u32 data = 0;
	uint32_t coords[4] = {0};
	int len = 0;
	char str[6]={0};

	node = ts->client->dev.of_node;
	if (node == NULL) {
		tp_log_err(" %s, can't find device_node", __func__);
		return -ENODEV;
	}

	while ((pp = of_get_next_child(node, pp)))
		cnt++;

	if (!cnt)
		return -ENODEV;

	cfg_table = kzalloc(cnt * (sizeof *cfg_table), GFP_KERNEL);
	if (!cfg_table)
		return -ENOMEM;

	pp = NULL;
	while ((pp = of_get_next_child(node, pp))) {
		if (of_property_read_u32(pp, "default_cfg", &data) == 0)
			cfg_table[i].default_cfg = data;

		if (of_property_read_u32(pp, "sensor_id", &data) == 0)
			cfg_table[i].sensor_id = (data);

		if (of_property_read_u32(pp, "fw_ver", &data) == 0)
			cfg_table[i].fw_ver = data;

		if (of_property_read_u32_array(pp, "himax,tw-coords", coords, 4) == 0) {
			cfg_table[i].tw_x_min = coords[0], cfg_table[i].tw_x_max = coords[1];	//x
			cfg_table[i].tw_y_min = coords[2], cfg_table[i].tw_y_max = coords[3];	//y
		}

		if (of_property_read_u32_array(pp, "himax,pl-coords", coords, 4) == 0) {
			cfg_table[i].pl_x_min = coords[0], cfg_table[i].pl_x_max = coords[1];	//x
			cfg_table[i].pl_y_min = coords[2], cfg_table[i].pl_y_max = coords[3];	//y
		}

		prop = of_find_property(pp, "c1", &len);
		if ((!prop)||(!len)) {
			strcpy(str,"c1");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c1, prop->value, len);
		prop = of_find_property(pp, "c2", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c2");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c2, prop->value, len);
		prop = of_find_property(pp, "c3", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c3");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c3, prop->value, len);
		prop = of_find_property(pp, "c4", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c4");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c4, prop->value, len);
		prop = of_find_property(pp, "c5", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c5");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c5, prop->value, len);
		prop = of_find_property(pp, "c6", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c6");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c6, prop->value, len);
		prop = of_find_property(pp, "c7", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c7");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c7, prop->value, len);
		prop = of_find_property(pp, "c8", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c8");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c8, prop->value, len);
		prop = of_find_property(pp, "c9", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c9");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c9, prop->value, len);
		prop = of_find_property(pp, "c10", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c10");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c10, prop->value, len);
		prop = of_find_property(pp, "c11", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c11");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c11, prop->value, len);
		prop = of_find_property(pp, "c12", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c12");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c12, prop->value, len);
		prop = of_find_property(pp, "c13", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c13");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c13, prop->value, len);
		prop = of_find_property(pp, "c14", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c14");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c14, prop->value, len);
		prop = of_find_property(pp, "c15", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c15");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c15, prop->value, len);
		prop = of_find_property(pp, "c16", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c16");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c16, prop->value, len);
		prop = of_find_property(pp, "c17", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c17");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c17, prop->value, len);
		prop = of_find_property(pp, "c18", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c18");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c18, prop->value, len);
		prop = of_find_property(pp, "c19", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c19");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c19, prop->value, len);
		prop = of_find_property(pp, "c20", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c20");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c20, prop->value, len);
		prop = of_find_property(pp, "c21", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c21");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c21, prop->value, len);
		prop = of_find_property(pp, "c22", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c22");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c22, prop->value, len);
		prop = of_find_property(pp, "c23", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c23");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c23, prop->value, len);
		prop = of_find_property(pp, "c24", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c24");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c24, prop->value, len);
		prop = of_find_property(pp, "c25", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c25");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c25, prop->value, len);
		prop = of_find_property(pp, "c26", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c26");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c26, prop->value, len);
		prop = of_find_property(pp, "c27", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c27");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c27, prop->value, len);
		prop = of_find_property(pp, "c28", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c28");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c28, prop->value, len);
		prop = of_find_property(pp, "c29", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c29");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c29, prop->value, len);
		prop = of_find_property(pp, "c30", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c30");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c30, prop->value, len);
		prop = of_find_property(pp, "c31", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c31");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c31, prop->value, len);
		prop = of_find_property(pp, "c32", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c32");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c32, prop->value, len);
		prop = of_find_property(pp, "c33", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c33");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c33, prop->value, len);
		prop = of_find_property(pp, "c34", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c34");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c34, prop->value, len);
		prop = of_find_property(pp, "c35", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c35");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c35, prop->value, len);
		prop = of_find_property(pp, "c36", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c36");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c36, prop->value, len);
		#if 1
		tp_log_info(" config version=[%02x]", cfg_table[i].c36[1]);
		#endif
		prop = of_find_property(pp, "c37", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c37");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c37, prop->value, len);
		prop = of_find_property(pp, "c38", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c38");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c38, prop->value, len);
		prop = of_find_property(pp, "c39", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c39");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c39, prop->value, len);
		prop = of_find_property(pp, "c40", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c40");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c40, prop->value, len);
		prop = of_find_property(pp, "c41", &len);
		if ((!prop)||(!len)) {
			strcpy(str, "c41");
			goto of_find_property_error;
			}
		memcpy(cfg_table[i].c41, prop->value, len);

	#if 1
		tp_log_info(" DT#%d-def_cfg:%d,id:%05x, FW:%d, len:%d,", i,
			cfg_table[i].default_cfg, cfg_table[i].sensor_id,
			cfg_table[i].fw_ver, cfg_table[i].length);
		//tp_log_info(" pl=[x_m%02d y_M%04d]\n", cfg_table[i].pl_x_min, cfg_table[i].pl_y_max);
	#endif
		i++;
	of_find_property_error:
	if (!prop) {
		tp_log_debug(" %s:Looking up %s property in node %s failed",
			__func__, str, pp->full_name);
		return -ENODEV;
	} else if (!len) {
		tp_log_debug(" %s:Invalid length of configuration data in %s\n",
			__func__, str);
		return -EINVAL;
		}
	}

	i = 0;	//confirm which config we should load
	while (cfg_table[i].fw_ver> ts->vendor_fw_ver_H) {
		i++;
	}
	if(cfg_table[i].default_cfg!=0)
		goto startloadconf;
	while (cfg_table[i].sensor_id > 0 && (cfg_table[i].sensor_id !=  ts->vendor_sensor_id)) {
		tp_log_info(" id:%#x!=%#x, (i++)",cfg_table[i].sensor_id, ts->vendor_sensor_id);
		i++;
	}
	startloadconf:
	if (i <= cnt) {
		tp_log_info(" DT-%s cfg idx(%d) in cnt(%d)", __func__, i, cnt);
		pdata->fw_ver 		= cfg_table[i].fw_ver;
		pdata->sensor_id      	= cfg_table[i].sensor_id;

		memcpy(pdata->c1, cfg_table[i].c1,sizeof(pdata->c1));
		memcpy(pdata->c2, cfg_table[i].c2,sizeof(pdata->c2));
		memcpy(pdata->c3, cfg_table[i].c3,sizeof(pdata->c3));
		memcpy(pdata->c4, cfg_table[i].c4,sizeof(pdata->c4));
		memcpy(pdata->c5, cfg_table[i].c5,sizeof(pdata->c5));
		memcpy(pdata->c6, cfg_table[i].c6,sizeof(pdata->c6));
		memcpy(pdata->c7, cfg_table[i].c7,sizeof(pdata->c7));
		memcpy(pdata->c8, cfg_table[i].c8,sizeof(pdata->c8));
		memcpy(pdata->c9, cfg_table[i].c9,sizeof(pdata->c9));
		memcpy(pdata->c10, cfg_table[i].c10,sizeof(pdata->c10));
		memcpy(pdata->c11, cfg_table[i].c11,sizeof(pdata->c11));
		memcpy(pdata->c12, cfg_table[i].c12,sizeof(pdata->c12));
		memcpy(pdata->c13, cfg_table[i].c13,sizeof(pdata->c13));
		memcpy(pdata->c14, cfg_table[i].c14,sizeof(pdata->c14));
		memcpy(pdata->c15, cfg_table[i].c15,sizeof(pdata->c15));
		memcpy(pdata->c16, cfg_table[i].c16,sizeof(pdata->c16));
		memcpy(pdata->c17, cfg_table[i].c17,sizeof(pdata->c17));
		memcpy(pdata->c18, cfg_table[i].c18,sizeof(pdata->c18));
		memcpy(pdata->c19, cfg_table[i].c19,sizeof(pdata->c19));
		memcpy(pdata->c20, cfg_table[i].c20,sizeof(pdata->c20));
		memcpy(pdata->c21, cfg_table[i].c21,sizeof(pdata->c21));
		memcpy(pdata->c22, cfg_table[i].c22,sizeof(pdata->c22));
		memcpy(pdata->c23, cfg_table[i].c23,sizeof(pdata->c23));
		memcpy(pdata->c24, cfg_table[i].c24,sizeof(pdata->c24));
		memcpy(pdata->c25, cfg_table[i].c25,sizeof(pdata->c25));
		memcpy(pdata->c26, cfg_table[i].c26,sizeof(pdata->c26));
		memcpy(pdata->c27, cfg_table[i].c27,sizeof(pdata->c27));
		memcpy(pdata->c28, cfg_table[i].c28,sizeof(pdata->c28));
		memcpy(pdata->c29, cfg_table[i].c29,sizeof(pdata->c29));
		memcpy(pdata->c30, cfg_table[i].c30,sizeof(pdata->c30));
		memcpy(pdata->c31, cfg_table[i].c31,sizeof(pdata->c31));
		memcpy(pdata->c32, cfg_table[i].c32,sizeof(pdata->c32));
		memcpy(pdata->c33, cfg_table[i].c33,sizeof(pdata->c33));
		memcpy(pdata->c34, cfg_table[i].c34,sizeof(pdata->c34));
		memcpy(pdata->c35, cfg_table[i].c35,sizeof(pdata->c35));
		memcpy(pdata->c36, cfg_table[i].c36,sizeof(pdata->c36));
		memcpy(pdata->c37, cfg_table[i].c37,sizeof(pdata->c37));
		memcpy(pdata->c38, cfg_table[i].c38,sizeof(pdata->c38));
		memcpy(pdata->c39, cfg_table[i].c39,sizeof(pdata->c39));
		memcpy(pdata->c40, cfg_table[i].c40,sizeof(pdata->c40));
		memcpy(pdata->c41, cfg_table[i].c41,sizeof(pdata->c41));

		ts->tw_x_min = cfg_table[i].tw_x_min, ts->tw_x_max = cfg_table[i].tw_x_max;	//x
		ts->tw_y_min = cfg_table[i].tw_y_min, ts->tw_y_max = cfg_table[i].tw_y_max;	//y

		ts->pl_x_min = cfg_table[i].pl_x_min, ts->pl_x_max = cfg_table[i].pl_x_max;	//x
		ts->pl_y_min = cfg_table[i].pl_y_min, ts->pl_y_max = cfg_table[i].pl_y_max;	//y

		tp_log_info(" DT#%d-def_cfg:%d,id:%05x, FW:%x, len:%d,", i,
			cfg_table[i].default_cfg, cfg_table[i].sensor_id,
			cfg_table[i].fw_ver, cfg_table[i].length);
		tp_log_info(" DT-%s:tw-coords = %d, %d, %d, %d\n", __func__, ts->tw_x_min,
				ts->tw_x_max, ts->tw_y_min, ts->tw_y_max);
		tp_log_info(" DT-%s:pl-coords = %d, %d, %d, %d\n", __func__, ts->pl_x_min,
				ts->pl_x_max, ts->pl_y_min, ts->pl_y_max);
		tp_log_info(" config version=[%02x]", pdata->c36[1]);
	} else {
		tp_log_err(" DT-%s cfg idx(%d) > cnt(%d)", __func__, i, cnt);
		return -EINVAL;
	}
	return 0;
}
#endif
#endif


static int himax_ManualMode(int enter)
{
	uint8_t cmd[2];
	cmd[0] = enter;
	if ( i2c_himax_write(private_ts->client, 0x42 ,&cmd[0], 1, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return 0;
	}
	return 0;
}

static int himax_FlashMode(int enter)
{
	uint8_t cmd[2];
	cmd[0] = enter;
	if ( i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 1, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return 0;
	}
	return 0;
}

int himax_lock_flash(int enable)
{
	uint8_t cmd[5];

	if(enable == 0){
		cmd[0] = 0x01;cmd[1] = 0x3C;cmd[2] = 0xA5;
		if (i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 3, 3) < 0) {
			tp_log_err("%s: i2c access fail!\n", __func__);
			return 0;
		}
	}

	/* lock sequence start */

	cmd[0] = 0x00;
	if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return 0;
	}
	if (enable != 0){
		cmd[0] = 0x02;
	}else{
		cmd[0] = 0x06;
	}
	if (i2c_himax_write(private_ts->client, 0x5C ,&cmd[0], 1, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return 0;
	}
	cmd[0] = 0x03;cmd[1] = 0x00;cmd[2] = 0x00;
	if (i2c_himax_write(private_ts->client, 0x44 ,&cmd[0], 3, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return 0;
	}
		if (i2c_himax_write_command(private_ts->client, 0x4A, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return 0;
	}
		cmd[0] = 0x03;cmd[1] = 0x00;cmd[2] = 0x00;
	if (i2c_himax_write(private_ts->client, 0x44 ,&cmd[0], 3, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return 0;
	}
	if(enable!=0){
			cmd[0] = 0x00;cmd[1] = 0x00;cmd[2] = 0x40;cmd[3] = 0x03;
		}
	else{
			cmd[0] = 0x00;cmd[1] = 0x00;cmd[2] = 0x00;cmd[3] = 0x00;
		}

	if (i2c_himax_write(private_ts->client, 0x45 ,&cmd[0], 4, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return 0;
	}

	if (i2c_himax_write_command(private_ts->client, 0x4B, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return 0;
	}
	if (i2c_himax_write_command(private_ts->client, 0x4C, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return 0;
	}
	msleep(50);
	if(enable != 0){
		cmd[0] = 0x00;cmd[1] = 0x00;cmd[2] = 0x00;
		if (i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 3, 3) < 0) {
			tp_log_err("%s: i2c access fail!\n", __func__);
			return 0;
		}
	}

	return 0;
	/* lock sequence stop */
}

/*change 1~7 MA */
static void himax_changeIref(int selected_iref){

	unsigned char temp_iref[16][2] = {	{0x00,0x00},{0x00,0x00},{0x00,0x00},{0x00,0x00},
										{0x00,0x00},{0x00,0x00},{0x00,0x00},{0x00,0x00},
										{0x00,0x00},{0x00,0x00},{0x00,0x00},{0x00,0x00},
										{0x00,0x00},{0x00,0x00},{0x00,0x00},{0x00,0x00}};
	uint8_t cmd[10];
	int i = 0;
	int j = 0;

	tp_log_info("%s: start to check iref,iref number = %d\n",__func__,selected_iref);

	for(i=0; i<16; i++){
		for(j=0; j<2; j++){
			if(selected_iref == 1){
				temp_iref[i][j] = E_IrefTable_1[i][j];
			}
			else if(selected_iref == 2){
				temp_iref[i][j] = E_IrefTable_2[i][j];
			}
			else if(selected_iref == 3){
				temp_iref[i][j] = E_IrefTable_3[i][j];
			}
			else if(selected_iref == 4){
				temp_iref[i][j] = E_IrefTable_4[i][j];
			}
			else if(selected_iref == 5){
				temp_iref[i][j] = E_IrefTable_5[i][j];
			}
			else if(selected_iref == 6){
				temp_iref[i][j] = E_IrefTable_6[i][j];
			}
			else if(selected_iref == 7){
				temp_iref[i][j] = E_IrefTable_7[i][j];
			}
		}
	}

	if(!iref_found){
		//Read Iref
		//Register 0x43
		//cmd[0] = 0x01;
		//cmd[1] = 0x00;
		//cmd[2] = 0x0A;
		//if( i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 3, 3) < 0){
		//	tp_log_err("%s: i2c access fail!\n", __func__);
		//	return;
		//}

		cmd[0] = 0x01;
		if (i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 1, 3) < 0) {
			tp_log_err("%s: i2c access fail!\n", __func__);
			return ;
		}

		cmd[0] = 0x00;
		if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
			tp_log_err("%s: i2c access fail!\n", __func__);
			return ;
		}

		cmd[0] = 0x0A;
		if (i2c_himax_write(private_ts->client, 0x5C ,&cmd[0], 1, 3) < 0) {
			tp_log_err("%s: i2c access fail!\n", __func__);
			return ;
		}

		//Register 0x44
		cmd[0] = 0x00;
		cmd[1] = 0x00;
		cmd[2] = 0x00;
		if( i2c_himax_write(private_ts->client, 0x44 ,&cmd[0], 3, 3) < 0){
			tp_log_err("%s: i2c access fail!\n", __func__);
			return ;
		}

		//Register 0x46
		if( i2c_himax_write(private_ts->client, 0x46 ,&cmd[0], 0, 3) < 0){
			tp_log_err("%s: i2c access fail!\n", __func__);
			return ;
		}

		//Register 0x59
		if( i2c_himax_read(private_ts->client, 0x59, cmd, 4, 3) < 0){
			tp_log_err("%s: i2c access fail!\n", __func__);
			return ;
		}

		//find iref group , default is iref 3
		for (i = 0; i < 16; i++){
			if ((cmd[0] == temp_iref[i][0]) &&
					(cmd[1] == temp_iref[i][1])){
				iref_number = i;
				iref_found = true;
				break;
			}
		}

		if(!iref_found ){
			tp_log_err("%s: Can't find iref number!\n", __func__);
			return ;
		}
		else{
			tp_log_info("%s: iref_number=%d, cmd[0]=0x%x, cmd[1]=0x%x\n", __func__, iref_number, cmd[0], cmd[1]);
		}
	}

	msleep(5);

	//iref write
	//Register 0x43
	//cmd[0] = 0x01;
	//cmd[1] = 0x00;
	//cmd[2] = 0x06;
	//if( i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 3, 3) < 0){
	//	tp_log_err("%s: i2c access fail!\n", __func__);
	//	return ;
	//}

	cmd[0] = 0x01;
	if (i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 1, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return ;
	}

	cmd[0] = 0x00;
	if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return ;
	}

	cmd[0] = 0x06;
	if (i2c_himax_write(private_ts->client, 0x5C ,&cmd[0], 1, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return ;
	}

	//Register 0x44
	cmd[0] = 0x00;
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	if( i2c_himax_write(private_ts->client, 0x44 ,&cmd[0], 3, 3) < 0){
		tp_log_err("%s: i2c access fail!\n", __func__);
		return ;
	}

	//Register 0x45
	cmd[0] = temp_iref[iref_number][0];
	cmd[1] = temp_iref[iref_number][1];
	cmd[2] = 0x17;
	cmd[3] = 0x28;

	if( i2c_himax_write(private_ts->client, 0x45 ,&cmd[0], 4, 3) < 0){
		tp_log_err("%s: i2c access fail!\n", __func__);
		return ;
	}

	//Register 0x4A
	if( i2c_himax_write(private_ts->client, 0x4A ,&cmd[0], 0, 3) < 0){
		tp_log_err("%s: i2c access fail!\n", __func__);
		return ;
	}

	//Read SFR to check the result
	//Register 0x43
	//cmd[0] = 0x01;
	//cmd[1] = 0x00;
	//cmd[2] = 0x0A;
	//if( i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 3, 3) < 0){
	//	tp_log_err("%s: i2c access fail!\n", __func__);
	//	return ;
	//}

	cmd[0] = 0x01;
	if (i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 1, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return ;
	}

	cmd[0] = 0x00;
	if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return ;
	}

	cmd[0] = 0x0A;
	if (i2c_himax_write(private_ts->client, 0x5C ,&cmd[0], 1, 3) < 0) {
		tp_log_err("%s: i2c access fail!\n", __func__);
		return ;
	}
	//Register 0x44
	cmd[0] = 0x00;
	cmd[1] = 0x00;
	cmd[2] = 0x00;
	if( i2c_himax_write(private_ts->client, 0x44 ,&cmd[0], 3, 3) < 0){
		tp_log_err("%s: i2c access fail!\n", __func__);
		return ;
	}

	//Register 0x46
	if( i2c_himax_write(private_ts->client, 0x46 ,&cmd[0], 0, 3) < 0){
		tp_log_err("%s: i2c access fail!\n", __func__);
		return ;
	}

	//Register 0x59
	if( i2c_himax_read(private_ts->client, 0x59, cmd, 4, 3) < 0){
		tp_log_err("%s: i2c access fail!\n", __func__);
		return ;
	}

	tp_log_info("%s:cmd[0]=%d,cmd[1]=%d,temp_iref_1=%d,temp_iref_2=%d\n",__func__, cmd[0], cmd[1], temp_iref[iref_number][0], temp_iref[iref_number][1]);

	if(cmd[0] != temp_iref[iref_number][0] || cmd[1] != temp_iref[iref_number][1]){
		tp_log_err("%s: IREF Read Back is not match.\n", __func__);
		tp_log_err("%s: Iref [0]=%d,[1]=%d\n", __func__,cmd[0],cmd[1]);
	}
	else{
		tp_log_info("%s: IREF Pass",__func__);
	}

}

/*
change_iref: ture, false
return value:
success: 1
failure: 0
*/
uint8_t himax_calculateChecksum(bool change_iref)
{

	int iref_flag = 0;
	uint8_t cmd[10];

	memset(cmd, 0x00, sizeof(cmd));

	//Sleep out
	if( i2c_himax_write(private_ts->client, 0x81 ,&cmd[0], 0, DEFAULT_RETRY_CNT) < 0)
	{
      		tp_log_err("%s: i2c access fail!\n", __func__);
      		return 0;
  	}
 	msleep(120);

	while(true){

		if(change_iref)
		{
			if(iref_flag == 0){
				himax_changeIref(2); //iref 2
			}
			else if(iref_flag == 1){
				himax_changeIref(5); //iref 5
			}
			else if(iref_flag == 2){
				himax_changeIref(1); //iref 1
			}
			else{
				goto CHECK_FAIL;
			}
			iref_flag ++;
		}


		if (i2c_himax_read(private_ts->client, 0xED, cmd, 4, DEFAULT_RETRY_CNT) < 0) {
			tp_log_err("%s: i2c access fail!\n", __func__);
			return -1;
		}

		cmd[3] = 0x04;

		if (i2c_himax_write(private_ts->client, 0xED ,&cmd[0], 4, DEFAULT_RETRY_CNT) < 0) {
			tp_log_err("%s: i2c access fail!\n", __func__);
			return 0;
		}

		//Enable Flash
		//cmd[0] = 0x01;
		//cmd[1] = 0x00;
		//cmd[2] = 0x02;

		//if (i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 3, DEFAULT_RETRY_CNT) < 0) {
		//	tp_log_err("%s: i2c access fail!\n", __func__);
		//	return 0;
		//}

		cmd[0] = 0x01;
		if (i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 1, 3) < 0) {
			tp_log_err("%s: i2c access fail!\n", __func__);
			return 0;
		}

		cmd[0] = 0x00;
		if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
			tp_log_err("%s: i2c access fail!\n", __func__);
			return 0;
		}

		cmd[0] = 0x02;
		if (i2c_himax_write(private_ts->client, 0x5C ,&cmd[0], 1, 3) < 0) {
			tp_log_err("%s: i2c access fail!\n", __func__);
			return 0;
		}

		cmd[0] = 0x01;
		cmd[1] = 0x00;
		if ( i2c_himax_write(private_ts->client, 0x94 ,&cmd[0], 2, DEFAULT_RETRY_CNT) < 0)
		{
			tp_log_err("%s: i2c access fail!\n", __func__);
			return 0;
		}

		cmd[0] = 0x01;
		if (i2c_himax_write(private_ts->client, 0x53 ,&cmd[0], 1, DEFAULT_RETRY_CNT) < 0) {
			tp_log_err("%s: i2c access fail!\n", __func__);
			return 0;
		}

		msleep(200);

		if (i2c_himax_read(private_ts->client, 0xAD, cmd, 4, DEFAULT_RETRY_CNT) < 0) {
			tp_log_err("%s: i2c access fail!\n", __func__);
			return -1;
		}

		tp_log_info("%s 0xAD[0,1,2,3] = %d,%d,%d,%d \n",__func__,cmd[0],cmd[1],cmd[2],cmd[3]);

		if (cmd[0] == 0 && cmd[1] == 0 && cmd[2] == 0 && cmd[3] == 0 ) {
			himax_FlashMode(0);
			goto CHECK_PASS;
		} else {
			himax_FlashMode(0);
			goto CHECK_FAIL;
		}


		CHECK_PASS:
			if(change_iref)
			{
				if(iref_flag < 3){
					continue;
				}
				else {
					return 1;
				}
			}
			else
			{
				return 1;
			}

		CHECK_FAIL:
			tp_log_err("%s: checksumResult fail!\n", __func__);
			himax_HW_reset(false,true);
			return 0;
	}

	return 0;
}


#define FLASH_56K_column 	14336 //(1024 x 56) /4
#define FLASH_63K_column 	16128 //(1024 x 63) /4
#define FLASH_56K_addr 		57344 //(1024 x 56) 
#define FLASH_63K_addr 		64512 //(1024 x 63)

/*fw upgrade flow*/
int fts_ctpm_fw_upgrade_with_fs(const unsigned char *fw, int len, bool change_iref)
{
	const unsigned char* ImageBuffer = fw;
	int fullFileLength = len;
	int i, j;
	int clm_cnt,ff_cnt,f_addr;
	uint8_t cmd[5], last_byte, prePage;
	int FileLength;
	uint8_t checksumResult = 0;
	uint8_t Flag_FW_skip = 0;

	tp_log_info("%s:himax Enter \n", __func__);
	HX_UPDATE_FLAG =1;

	//Try 3 Times
	for (j = 0; j < 3; j++)
	{
		FileLength = fullFileLength;

		if ( i2c_himax_write(private_ts->client, 0x81 ,&cmd[0], 0, DEFAULT_RETRY_CNT) < 0)
		{
			tp_log_err("%s: i2c access fail!\n", __func__);
			return 0;
		}

		msleep(120);

		himax_lock_flash(0);

		cmd[0] = 0x05;
		if (i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 1, 3) < 0) {
			tp_log_err("%s: i2c access fail!\n", __func__);
			return 0;
		}

		cmd[0] = 0x00;
		if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
			tp_log_err("%s: i2c access fail!\n", __func__);
			return 0;
		}

		cmd[0] = 0x02;
		if (i2c_himax_write(private_ts->client, 0x5C ,&cmd[0], 1, 3) < 0) {
			tp_log_err("%s: i2c access fail!\n", __func__);
			return 0;
		}

		if ( i2c_himax_write(private_ts->client, 0x4F ,&cmd[0], 0, DEFAULT_RETRY_CNT) < 0)
		{
			tp_log_err("%s: i2c access fail!\n", __func__);
			return 0;
		}
		msleep(50);

		himax_ManualMode(1);
		himax_FlashMode(1);

		if((ImageBuffer[FLASH_56K_addr]==0xFF) && (ImageBuffer[FLASH_63K_addr+1]==0xFF) && (ImageBuffer[FLASH_63K_addr-2]==0xFF) && (ImageBuffer[FLASH_63K_addr-1]==0xFF))
		{
			Flag_FW_skip = 1;
			tp_log_info("%s: Flag_FW_skip = 1\n", __func__);
		} else {
			Flag_FW_skip = 0;
			tp_log_info("%s: Flag_FW_skip = 0\n", __func__);
			tp_log_info("%s: ImageBuffer[%d]=%x\n", __func__,FLASH_56K_addr,ImageBuffer[FLASH_56K_addr]);
			tp_log_info("%s: ImageBuffer[%d]=%x\n", __func__,FLASH_56K_addr+1,ImageBuffer[FLASH_56K_addr+1]);
			tp_log_info("%s: ImageBuffer[%d]=%x\n", __func__,FLASH_63K_addr-2,ImageBuffer[FLASH_63K_addr-2]);
			tp_log_info("%s: ImageBuffer[%d]=%x\n", __func__,FLASH_63K_addr-1,ImageBuffer[FLASH_63K_addr-1]);
		}

		//Set Flash address (0x44H)
		//cmd[0] =column  1~5
		//cmd[1] =page      6~11
		//cmd[2] =sector    12~16
		FileLength = (FileLength + 3) / 4;
		for (i = 0, prePage = 0; i < FileLength; i++)
		{
			if ((Flag_FW_skip)&& (i > FLASH_56K_column) && (i < FLASH_63K_column)){//reserved area addr 56k~63k
				continue;
			}

			last_byte = 0;
			cmd[0] = i & 0x1F;//max 31 colume
			if (cmd[0] == 0x1F || i == FileLength - 1)
			{
				last_byte = 1;
			}
			cmd[1] = (i >> 5) & 0x3F;
			cmd[2] = (i >> 11) & 0x1F;

			if ( i2c_himax_write(private_ts->client, 0x44 ,&cmd[0], 3, DEFAULT_RETRY_CNT) < 0)
			{
				tp_log_err("%s: i2c access fail!\n", __func__);
				return 0;
			}

			if(((i%32) == 0)&&(i))
			{
				ff_cnt=0;
				for (clm_cnt = 0; clm_cnt < 32; clm_cnt++)
				{
					f_addr=(i*4)+(clm_cnt*4);
					if((ImageBuffer[f_addr]==0xFF) && (ImageBuffer[f_addr+1]==0xFF) 
							&&(ImageBuffer[f_addr+2]==0xFF)&&(ImageBuffer[f_addr+3]==0xFF))
								ff_cnt++;
						else
							break;
						if(ff_cnt==32) {
							//tp_log_info("%s: FW_skip f_addr=%x\n", __func__,f_addr);
							continue;
						}
				}
			}

			if (prePage != cmd[1] || i == 0)
			{
				prePage = cmd[1];

				cmd[0] = 0x09;
				if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				cmd[0] = 0x0D;
				if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				cmd[0] = 0x09;
				if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}
			}

			memcpy(&cmd[0], &ImageBuffer[4*i], 4);//write 4 bytes in every circle.

			if ( i2c_himax_write(private_ts->client, 0x45 ,&cmd[0], 4, DEFAULT_RETRY_CNT) < 0)
			{
				tp_log_err("%s: i2c access fail!\n", __func__);
				return 0;
			}

			cmd[0] = 0x0D;
			if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
				tp_log_err("%s: i2c access fail!\n", __func__);
				return 0;
			}

			cmd[0] = 0x09;
			if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
				tp_log_err("%s: i2c access fail!\n", __func__);
				return 0;
			}

			if (last_byte == 1)
			{
				cmd[0] = 0x01;
				if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				cmd[0] = 0x05;
				if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				cmd[0] = 0x01;
				if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				cmd[0] = 0x00;
				if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				mdelay(5);
				if (i == (FileLength - 1))
				{
					himax_FlashMode(0);
					himax_ManualMode(0);
					checksumResult = himax_calculateChecksum(change_iref);
					//himax_ManualMode(0);
					himax_lock_flash(1);
					if (checksumResult) //Success
					{
						HX_UPDATE_FLAG =0;
						return 1;
					}
					else //Fail
					{
						HX_UPDATE_FLAG =0;
						tp_log_err("%s: checksumResult fail!\n", __func__);
						//return 0;
					}
				}
			}
		}
	}
	HX_UPDATE_FLAG =0;
	tp_log_info("%s:exit \n", __func__);
	return 0;
}




#ifdef HX_LOADIN_CONFIG
static int himax_config_reg_write(struct i2c_client *client, uint8_t StartAddr, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	char cmd[12] = {0};

	cmd[0] = 0x8C;cmd[1] = 0x11;
	if ((i2c_himax_master_write(client, &cmd[0],2,toRetry))<0)
		return -1;

	cmd[0] = 0x8B;cmd[1] = 0x00;cmd[2] = StartAddr;	//Start addr
	if ((i2c_himax_master_write(client, &cmd[0],3,toRetry))<0)
		return -1;

	if ((i2c_himax_master_write(client, data,length,toRetry))<0)
		return -1;

	cmd[0] = 0x8C;cmd[1] = 0x00;
	if ((i2c_himax_master_write(client, &cmd[0],2,toRetry))<0)
		return -1;

	return 0;
}
#endif



#ifdef HX_AUTO_UPDATE_FW
static int check_firmware_version(const struct firmware *fw)
{
	if(!fw->data)
	{
		tp_log_info(" himax ======== fw requst fail =====");
		return 0;
	}
	tp_log_info("himax IMAGE FW_VER=%x,%x.\n", fw->data[FW_VER_MAJ_FLASH_ADDR], fw->data[FW_VER_MIN_FLASH_ADDR]);
	tp_log_info("himax IMAGE CFG_VER=%x.\n", fw->data[FW_CFG_VER_FLASH_ADDR]);

	tp_log_info("himax curr FW_VER=%x,%x.\n", private_ts->vendor_fw_ver_H, private_ts->vendor_fw_ver_L);
	tp_log_info("himax curr CFG_VER=%x.\n", private_ts->vendor_config_ver);

	if (( private_ts->vendor_fw_ver_H < fw->data[FW_VER_MAJ_FLASH_ADDR] )
		|| ( private_ts->vendor_fw_ver_L < fw->data[FW_VER_MIN_FLASH_ADDR] )
		|| ( private_ts->vendor_config_ver < fw->data[FW_CFG_VER_FLASH_ADDR] ))
	{
		tp_log_info("firmware is lower, must upgrade.\n");
		return 1;
	}
	else {
		tp_log_info("firmware not lower.\n");
		return 0;
	}
}

static int check_need_firmware_upgrade(const struct firmware *fw)
{
	//first, check firmware version.
	int ret = 0;
	if ( himax_calculateChecksum(false) == 0 )
	{
		tp_log_info("checksum failed, must upgrade.\n");
		#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_dsm_info.UPDATE_status = FWU_FW_CRC_ERROR;
			hmx_tp_report_dsm_err(DSM_TP_FW_ERROR_NO, ret);
		#endif
		return 1;
	}
	else {
		tp_log_info("checksum success.\n");
	}

	if (check_firmware_version(fw) )
	{
		ret = himax_check_update_firmware_flag();
		if(0 == ret){
			tp_log_info("now is factory test mode, not update firmware\n");
			return 0;
		}
		return 1;
	}

	return 0;
}

static void  firmware_update(const struct firmware *fw)
{
	int fullFileLength = 0;
	int ret = -1;

	if(!fw->data||!fw->size)
	{
		tp_log_info(" himax ======== fw requst fail =====");
		return;
	}

	//himax_int_enable(private_ts->client->irq,0);

	himax_HW_reset(false,true);

	tp_log_info("himax fw size =%u \n",(unsigned int )fw->size);

	fullFileLength=(unsigned int) fw->size;
	if (check_need_firmware_upgrade(fw))
	{
			//do upgrade
			//himax_int_enable(private_ts->client->irq,0);
			wake_lock(&private_ts->ts_flash_wake_lock);

			#ifdef HX_CHIP_STATUS_MONITOR
				HX_CHIP_POLLING_COUNT = 0;
				cancel_delayed_work_sync(&private_ts->himax_chip_monitor);
			#endif

			//himax_HW_reset(false,true);
			tp_log_info("himax flash write start");
			ret = fts_ctpm_fw_upgrade_with_fs(fw->data,fullFileLength,true);
			if( ret == 0 )
			{
				tp_log_err("%s:himax  TP upgrade error\n", __func__);
				tp_log_info("himax  update function end====");
				#ifdef CONFIG_HUAWEI_DSM
					hmx_tp_dsm_info.UPDATE_status = FWU_FW_WRITE_ERROR;
					hmx_tp_report_dsm_err(DSM_TP_FW_ERROR_NO, ret);
				#endif
			}
			else
			{

				tp_log_info("%s: himax TP upgrade OK\n", __func__);
				#ifdef HX_UPDATE_WITH_BIN_BUILDIN
				private_ts->vendor_fw_ver_H = fw->data[FW_VER_MAJ_FLASH_ADDR];
				private_ts->vendor_fw_ver_L = fw->data[FW_VER_MIN_FLASH_ADDR];
				private_ts->vendor_config_ver = fw->data[FW_CFG_VER_FLASH_ADDR];
				
				tp_log_info("himax upgraded IMAGE FW_VER=%x,%x.\n",fw->data[FW_VER_MAJ_FLASH_ADDR],fw->data[FW_VER_MIN_FLASH_ADDR]);
				tp_log_info("himax upgraded IMAGE CFG_VER=%x.\n",fw->data[FW_CFG_VER_FLASH_ADDR]);
				#endif
			}
			tp_log_info("himax flash write end");
			//himax_HW_reset(false,true);
			//himax_HW_reset(true,true);

			wake_unlock(&private_ts->ts_flash_wake_lock);
			//himax_int_enable(private_ts->client->irq,1);
			#ifdef HX_CHIP_STATUS_MONITOR
				HX_CHIP_POLLING_COUNT = 0;
				queue_delayed_work(private_ts->himax_chip_monitor_wq, &private_ts->himax_chip_monitor, HX_POLLING_TIMES*HZ);
			#endif
			tp_log_info("himax i_update function end ");
	}
	else {
		tp_log_info("himax don't need upgrade firmware");
	}

	himax_HW_reset(true,true);
	himax_int_enable(private_ts->client->irq,1);
	return ;
}

static void himax_set_app_info(struct himax_ts_data *ts) 
{
	char touch_info[31] = {0};
	snprintf(touch_info, 30,
				"himax_852xF_AUO_%d.%d.%d",
				ts->vendor_fw_ver_H,
				ts->vendor_fw_ver_L,
				ts->vendor_config_ver);
#ifdef CONFIG_APP_INFO
	app_info_set("touch_panel", touch_info);
#endif
}
void himax_update_fw_func(struct work_struct *work) 
{
	int retval = 0;
	const  struct firmware *fw_entry = NULL;
	char firmware_name[100] = "";

	tp_log_info("%s: enter!\n", __func__);
	sprintf(firmware_name, HX_FW_NAME);

	tp_log_info("himax start to request firmware  %s", firmware_name);
	retval = request_firmware(&fw_entry, firmware_name, &private_ts->client->dev);
	if (retval < 0) {	
		tp_log_err("himax %s %d: Fail request firmware %s, ret = %d\n", 
					__func__, __LINE__, firmware_name, retval);
		#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_dsm_info.UPDATE_status = FWU_REQUEST_FW_FAIL;
			hmx_tp_report_dsm_err(DSM_TP_FW_ERROR_NO, retval);
		#endif
		return ;
	}

	firmware_update(fw_entry);

	release_firmware(fw_entry);

	himax_set_app_info(private_ts);
	//PowerOnSeq(private_ts);

	fw_entry = NULL;
	tp_log_info("%s: end!\n", __func__);
}

#endif


#ifdef HX_AUTO_UPDATE_CONFIG
int fts_ctpm_fw_upgrade_with_i_file_flash_cfg(struct himax_config *cfg)
{
	unsigned char* ImageBuffer= i_CTPM_FW;//NULL ;//
	int fullFileLength = FLASH_SIZE;

	int i, j, k;
	uint8_t cmd[5], last_byte, prePage;
	uint8_t tmp[5];
	int FileLength;
	int Polynomial = 0x82F63B78;
	int CRC32 = 0xFFFFFFFF;
	int BinDataWord = 0;
	int current_index = 0;
	uint8_t checksumResult = 0;

	tp_log_info(" %s: flash CONFIG only START!\n", __func__);

#if 0
	//=========Dump FW image from Flash========
	setSysOperation(1);
	setFlashCommand(0x0F);
	setFlashDumpProgress(0);
	setFlashDumpComplete(0);
	setFlashDumpFail(0);
	queue_work(private_ts->flash_wq, &private_ts->flash_work);
	for(i=0 ; i<1024 ; i++)
		{
			if(getFlashDumpComplete())
				{
					ImageBuffer = flash_buffer;
					fullFileLength = FLASH_SIZE;
					tp_log_info(" %s: Load FW from flash OK! cycle=%d fullFileLength=%x\n", __func__,i,fullFileLength);
					break;
				}
			msleep(5);
		}
	if(i==400)
	{
		tp_log_err(" %s: Load FW from flash time out fail!\n", __func__);
		return 0;
	}
	//===================================
#endif
	current_index = CFB_START_ADDR + CFB_INFO_LENGTH;

	//Update the Config Part from config array
	for(i=1 ; i<sizeof(cfg->c1)/sizeof(cfg->c1[0]) ; i++) ImageBuffer[current_index++] = cfg->c1[i];
	for(i=1 ; i<sizeof(cfg->c2)/sizeof(cfg->c2[0]) ; i++) ImageBuffer[current_index++] = cfg->c2[i];
	for(i=1 ; i<sizeof(cfg->c3)/sizeof(cfg->c3[0]) ; i++) ImageBuffer[current_index++] = cfg->c3[i];
	for(i=1 ; i<sizeof(cfg->c4)/sizeof(cfg->c4[0]) ; i++) ImageBuffer[current_index++] = cfg->c4[i];
	for(i=1 ; i<sizeof(cfg->c5)/sizeof(cfg->c5[0]) ; i++) ImageBuffer[current_index++] = cfg->c5[i];
	for(i=1 ; i<sizeof(cfg->c6)/sizeof(cfg->c6[0]) ; i++) ImageBuffer[current_index++] = cfg->c6[i];
	for(i=1 ; i<sizeof(cfg->c7)/sizeof(cfg->c7[0]) ; i++) ImageBuffer[current_index++] = cfg->c7[i];
	for(i=1 ; i<sizeof(cfg->c8)/sizeof(cfg->c8[0]) ; i++) ImageBuffer[current_index++] = cfg->c8[i];
	for(i=1 ; i<sizeof(cfg->c9)/sizeof(cfg->c9[0]) ; i++) ImageBuffer[current_index++] = cfg->c9[i];
	for(i=1 ; i<sizeof(cfg->c10)/sizeof(cfg->c10[0]) ; i++) ImageBuffer[current_index++] = cfg->c10[i];
	for(i=1 ; i<sizeof(cfg->c11)/sizeof(cfg->c11[0]) ; i++) ImageBuffer[current_index++] = cfg->c11[i];
	for(i=1 ; i<sizeof(cfg->c12)/sizeof(cfg->c12[0]) ; i++) ImageBuffer[current_index++] = cfg->c12[i];
	for(i=1 ; i<sizeof(cfg->c13)/sizeof(cfg->c13[0]) ; i++) ImageBuffer[current_index++] = cfg->c13[i];
	for(i=1 ; i<sizeof(cfg->c14)/sizeof(cfg->c14[0]) ; i++) ImageBuffer[current_index++] = cfg->c14[i];
	for(i=1 ; i<sizeof(cfg->c15)/sizeof(cfg->c15[0]) ; i++) ImageBuffer[current_index++] = cfg->c15[i];
	for(i=1 ; i<sizeof(cfg->c16)/sizeof(cfg->c16[0]) ; i++) ImageBuffer[current_index++] = cfg->c16[i];
	for(i=1 ; i<sizeof(cfg->c17)/sizeof(cfg->c17[0]) ; i++) ImageBuffer[current_index++] = cfg->c17[i];
	for(i=1 ; i<sizeof(cfg->c18)/sizeof(cfg->c18[0]) ; i++) ImageBuffer[current_index++] = cfg->c18[i];
	for(i=1 ; i<sizeof(cfg->c19)/sizeof(cfg->c19[0]) ; i++) ImageBuffer[current_index++] = cfg->c19[i];
	for(i=1 ; i<sizeof(cfg->c20)/sizeof(cfg->c20[0]) ; i++) ImageBuffer[current_index++] = cfg->c20[i];
	for(i=1 ; i<sizeof(cfg->c21)/sizeof(cfg->c21[0]) ; i++) ImageBuffer[current_index++] = cfg->c21[i];
	for(i=1 ; i<sizeof(cfg->c22)/sizeof(cfg->c22[0]) ; i++) ImageBuffer[current_index++] = cfg->c22[i];
	for(i=1 ; i<sizeof(cfg->c23)/sizeof(cfg->c23[0]) ; i++) ImageBuffer[current_index++] = cfg->c23[i];
	for(i=1 ; i<sizeof(cfg->c24)/sizeof(cfg->c24[0]) ; i++) ImageBuffer[current_index++] = cfg->c24[i];
	for(i=1 ; i<sizeof(cfg->c25)/sizeof(cfg->c25[0]) ; i++) ImageBuffer[current_index++] = cfg->c25[i];
	for(i=1 ; i<sizeof(cfg->c26)/sizeof(cfg->c26[0]) ; i++) ImageBuffer[current_index++] = cfg->c26[i];
	for(i=1 ; i<sizeof(cfg->c27)/sizeof(cfg->c27[0]) ; i++) ImageBuffer[current_index++] = cfg->c27[i];
	for(i=1 ; i<sizeof(cfg->c28)/sizeof(cfg->c28[0]) ; i++) ImageBuffer[current_index++] = cfg->c28[i];
	for(i=1 ; i<sizeof(cfg->c29)/sizeof(cfg->c29[0]) ; i++) ImageBuffer[current_index++] = cfg->c29[i];
	for(i=1 ; i<sizeof(cfg->c30)/sizeof(cfg->c30[0]) ; i++) ImageBuffer[current_index++] = cfg->c30[i];
	for(i=1 ; i<sizeof(cfg->c31)/sizeof(cfg->c31[0]) ; i++) ImageBuffer[current_index++] = cfg->c31[i];
	for(i=1 ; i<sizeof(cfg->c32)/sizeof(cfg->c32[0]) ; i++) ImageBuffer[current_index++] = cfg->c32[i];
	for(i=1 ; i<sizeof(cfg->c33)/sizeof(cfg->c33[0]) ; i++) ImageBuffer[current_index++] = cfg->c33[i];
	for(i=1 ; i<sizeof(cfg->c34)/sizeof(cfg->c34[0]) ; i++) ImageBuffer[current_index++] = cfg->c34[i];
	for(i=1 ; i<sizeof(cfg->c35)/sizeof(cfg->c35[0]) ; i++) ImageBuffer[current_index++] = cfg->c35[i];
	for(i=1 ; i<sizeof(cfg->c36)/sizeof(cfg->c36[0]) ; i++) ImageBuffer[current_index++] = cfg->c36[i];
	for(i=1 ; i<sizeof(cfg->c37)/sizeof(cfg->c37[0]) ; i++) ImageBuffer[current_index++] = cfg->c37[i];
	for(i=1 ; i<sizeof(cfg->c38)/sizeof(cfg->c38[0]) ; i++) ImageBuffer[current_index++] = cfg->c38[i];
	for(i=1 ; i<sizeof(cfg->c39)/sizeof(cfg->c39[0]) ; i++) ImageBuffer[current_index++] = cfg->c39[i];
	current_index=current_index+50;//dummy data
	//tp_log_info(" %s: current_index=%d\n", __func__,current_index);
	for(i=1 ; i<sizeof(cfg->c40)/sizeof(cfg->c40[0]) ; i++) ImageBuffer[current_index++] = cfg->c40[i];
	for(i=1 ; i<sizeof(cfg->c41)/sizeof(cfg->c41[0]) ; i++) ImageBuffer[current_index++] = cfg->c41[i];

	//cal_checksum start//
	FileLength = fullFileLength;
	FileLength = (FileLength + 3) / 4;
	for (i = 0; i < FileLength; i++)
	{
		memcpy(&cmd[0], &ImageBuffer[4*i], 4);
		if (i < (FileLength - 1))/*cal_checksum*/
		{
			for(k = 0; k < 4; k++)
			{
				BinDataWord |= cmd[k] << (k * 8);
			}

			CRC32 = BinDataWord ^ CRC32;
			for(k = 0; k < 32; k++)
			{
				if((CRC32 % 2) != 0)
				{
					CRC32 = ((CRC32 >> 1) & 0x7FFFFFFF) ^ Polynomial;
				}
				else
				{
					CRC32 = ((CRC32 >> 1) & 0x7FFFFFFF);
				}
			}
			BinDataWord = 0;
		}
	}
	//cal_checksum end//

	//Try 3 Times
	for (j = 0; j < 3; j++)
	{
		FileLength = fullFileLength;

		himax_HW_reset(false,false);


		if( i2c_himax_write(private_ts->client, 0x81 ,&cmd[0], 0, DEFAULT_RETRY_CNT) < 0)
		{
			tp_log_err(" %s: i2c access fail!\n", __func__);
			return 0;
		}

		msleep(120);

		himax_lock_flash(0);

		himax_ManualMode(1);
		himax_FlashMode(1);

		FileLength = (FileLength + 3) / 4;
		for (i = 0, prePage = 0; i < FileLength; i++)
		{
			last_byte = 0;

			if(i<0x20)
				continue;
			else if((i>0xBF)&&(i<(FileLength-0x20)))
				continue;

			cmd[0] = i & 0x1F;
			if (cmd[0] == 0x1F || i == FileLength - 1)
			{
				last_byte = 1;
			}
			cmd[1] = (i >> 3) & 0x1F;
			cmd[2] = (i >> 11) & 0x1F;

			if( i2c_himax_write(private_ts->client, 0x44 ,&cmd[0], 3, DEFAULT_RETRY_CNT) < 0)
			{
				tp_log_err(" %s: i2c access fail!\n", __func__);
				return 0;
			}

			//tp_log_info(" %s: CMD 44 [0,1,2]=[%x,%x,%x]\n", __func__,cmd[0],cmd[1],cmd[2]);
			if (prePage != cmd[1] || i == 0x20)
			{
				prePage = cmd[1];
				//tp_log_info(" %s: %d page erase\n",__func__,prePage);
				//tmp[0] = 0x01;tmp[1] = 0x09;//tmp[2] = 0x02;
				//if( i2c_himax_write(private_ts->client, 0x43 ,&tmp[0], 2, DEFAULT_RETRY_CNT) < 0)
				//{
				//	tp_log_err(" %s: i2c access fail!\n", __func__);
				//	return 0;
				//}
				tmp[0] = 0x01;
				if (i2c_himax_write(private_ts->client, 0x43 ,&tmp[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				tmp[0] = 0x09;
				if (i2c_himax_write(private_ts->client, 0x5B ,&tmp[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				//tmp[0] = 0x05;tmp[1] = 0x2D;//tmp[2] = 0x02;
				//if( i2c_himax_write(private_ts->client, 0x43 ,&tmp[0], 2, DEFAULT_RETRY_CNT) < 0)
				//{
				//	tp_log_err(" %s: i2c access fail!\n", __func__);
				//	return 0;
				//}
				tmp[0] = 0x05;
				if (i2c_himax_write(private_ts->client, 0x43 ,&tmp[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				tmp[0] = 0x2D;
				if (i2c_himax_write(private_ts->client, 0x5B ,&tmp[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				msleep(30);
				//tmp[0] = 0x01;tmp[1] = 0x09;//tmp[2] = 0x02;
				//if( i2c_himax_write(private_ts->client, 0x43 ,&tmp[0], 2, DEFAULT_RETRY_CNT) < 0)
				//{
				//	tp_log_err(" %s: i2c access fail!\n", __func__);
				//	return 0;
				//}
				tmp[0] = 0x01;
				if (i2c_himax_write(private_ts->client, 0x43 ,&tmp[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				tmp[0] = 0x09;
				if (i2c_himax_write(private_ts->client, 0x5B ,&tmp[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				if( i2c_himax_write(private_ts->client, 0x44 ,&cmd[0], 3, DEFAULT_RETRY_CNT) < 0)
				{
					tp_log_err(" %s: i2c access fail!\n", __func__);
					return 0;
				}

				//tp_log_info(" %s: CMD 44-47 [0,1,2]=[%x,%x,%x]\n", __func__,cmd[0],cmd[1],cmd[2]);

				//tmp[0] = 0x01;tmp[1] = 0x09;//tmp[2] = 0x02;
				//if( i2c_himax_write(private_ts->client, 0x43 ,&tmp[0], 2, DEFAULT_RETRY_CNT) < 0)
				//{
				//	tp_log_err(" %s: i2c access fail!\n", __func__);
				//	return 0;
				//}

				tmp[0] = 0x01;
				if (i2c_himax_write(private_ts->client, 0x43 ,&tmp[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				tmp[0] = 0x09;
				if (i2c_himax_write(private_ts->client, 0x5B ,&tmp[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				//tmp[0] = 0x01;tmp[1] = 0x0D;//tmp[2] = 0x02;
				//if( i2c_himax_write(private_ts->client, 0x43 ,&tmp[0], 2, DEFAULT_RETRY_CNT) < 0)
				//{
				//	tp_log_err(" %s: i2c access fail!\n", __func__);
				//	return 0;
				//}

				tmp[0] = 0x01;
				if (i2c_himax_write(private_ts->client, 0x43 ,&tmp[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				tmp[0] = 0x0D;
				if (i2c_himax_write(private_ts->client, 0x5B ,&tmp[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				//tmp[0] = 0x01;tmp[1] = 0x09;//tmp[2] = 0x02;
				//if( i2c_himax_write(private_ts->client, 0x43 ,&tmp[0], 2, DEFAULT_RETRY_CNT) < 0)
				//{
				//	tp_log_err(" %s: i2c access fail!\n", __func__);
				//	return 0;
				//}

				tmp[0] = 0x01;
				if (i2c_himax_write(private_ts->client, 0x43 ,&tmp[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				tmp[0] = 0x09;
				if (i2c_himax_write(private_ts->client, 0x5B ,&tmp[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				if( i2c_himax_write(private_ts->client, 0x44 ,&cmd[0], 3, DEFAULT_RETRY_CNT) < 0)
				{
					tp_log_err(" %s: i2c access fail!\n", __func__);
					return 0;
				}

				//tp_log_info(" %s: CMD 44-45 [0,1,2]=[%x,%x,%x]\n", __func__,cmd[0],cmd[1],cmd[2]);
				//tp_log_info(" %s: Erase Page num=%x\n", __func__,prePage);
			}

			memcpy(&cmd[0], &ImageBuffer[4*i], 4);

			if (i == (FileLength - 1))
			{
				tmp[0]= (CRC32 & 0xFF);
				tmp[1]= ((CRC32 >>8) & 0xFF);
				tmp[2]= ((CRC32 >>16) & 0xFF);
				tmp[3]= ((CRC32 >>24) & 0xFF);

				memcpy(&cmd[0], &tmp[0], 4);
				tp_log_info("%s last_byte = 1, CRC32= %x, =[0,1,2,3] = %x,%x,%x,%x \n",__func__, CRC32,tmp[0],tmp[1],tmp[2],tmp[3]);
			}

			if( i2c_himax_write(private_ts->client, 0x45 ,&cmd[0], 4, DEFAULT_RETRY_CNT) < 0)
			{
				tp_log_err(" %s: i2c access fail!\n", __func__);
				return 0;
			}
			//tp_log_info(" %s: CMD 48 \n", __func__);
			//cmd[0] = 0x01;cmd[1] = 0x0D;//cmd[2] = 0x02;
			//if( i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 2, DEFAULT_RETRY_CNT) < 0)
			//{
			//	tp_log_err(" %s: i2c access fail!\n", __func__);
			//	return 0;
			//}
			cmd[0] = 0x01;
			if (i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 1, 3) < 0) {
				tp_log_err("%s: i2c access fail!\n", __func__);
				return 0;
			}

			cmd[0] = 0x0D;
			if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
				tp_log_err("%s: i2c access fail!\n", __func__);
				return 0;
			}


			//cmd[0] = 0x01;cmd[1] = 0x09;//cmd[2] = 0x02;
			//if( i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 2, DEFAULT_RETRY_CNT) < 0)
			//{
			//	tp_log_err(" %s: i2c access fail!\n", __func__);
			//	return 0;
			//}
			cmd[0] = 0x01;
			if (i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 1, 3) < 0) {
				tp_log_err("%s: i2c access fail!\n", __func__);
				return 0;
			}

			cmd[0] = 0x09;
			if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
				tp_log_err("%s: i2c access fail!\n", __func__);
				return 0;
			}

			if (last_byte == 1)
			{
				//cmd[0] = 0x01;cmd[1] = 0x01;//cmd[2] = 0x02;
				//if( i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 2, DEFAULT_RETRY_CNT) < 0)
				//{
				//	tp_log_err(" %s: i2c access fail!\n", __func__);
				//	return 0;
				//}
				cmd[0] = 0x01;
				if (i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				cmd[0] = 0x01;
				if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				//cmd[0] = 0x01;cmd[1] = 0x05;//cmd[2] = 0x02;
				//if( i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 2, DEFAULT_RETRY_CNT) < 0)
				//{
				//	tp_log_err(" %s: i2c access fail!\n", __func__);
				//	return 0;
				//}
				cmd[0] = 0x01;
				if (i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				cmd[0] = 0x05;
				if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				//cmd[0] = 0x01;cmd[1] = 0x01;//cmd[2] = 0x02;
				//if( i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 2, DEFAULT_RETRY_CNT) < 0)
				//{
				//	tp_log_err(" %s: i2c access fail!\n", __func__);
				//	return 0;
				//}
				cmd[0] = 0x01;
				if (i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				cmd[0] = 0x01;
				if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				//cmd[0] = 0x01;cmd[1] = 0x00;//cmd[2] = 0x02;
				//if( i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 2, DEFAULT_RETRY_CNT) < 0)
				//{
				//	tp_log_err(" %s: i2c access fail!\n", __func__);
				//	return 0;
				//}
				cmd[0] = 0x01;
				if (i2c_himax_write(private_ts->client, 0x43 ,&cmd[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				cmd[0] = 0x00;
				if (i2c_himax_write(private_ts->client, 0x5B ,&cmd[0], 1, 3) < 0) {
					tp_log_err("%s: i2c access fail!\n", __func__);
					return 0;
				}

				msleep(10);
				if (i == (FileLength - 1))
				{
					himax_FlashMode(0);
					himax_ManualMode(0);
					checksumResult = himax_calculateChecksum(true);//
					//himax_ManualMode(0);
					himax_lock_flash(1);

					tp_log_info(" %s: flash CONFIG only END!\n", __func__);
					if (checksumResult) //Success
					{
						return 1;
					}
					else //Fail
					{
						tp_log_err("%s: checksumResult fail!\n", __func__);
						return 0;
					}
				}
			}
		}
	}
	return 0;
}

static int i_update_FWCFG(struct himax_config *cfg)
{
	tp_log_info("%s: CHIP CONFG version=%x\n", __func__,private_ts->vendor_config_ver);
	tp_log_info("%s: LOAD CONFG version=%x\n", __func__,cfg->c36[1]);

	if ( private_ts->vendor_config_ver != cfg->c36[1] )
	{
		if(fts_ctpm_fw_upgrade_with_i_file_flash_cfg(cfg) == 0)
			tp_log_err("%s: TP upgrade error\n", __func__);
		else
			tp_log_info("%s: TP upgrade OK\n", __func__);
			himax_HW_reset(false,false);
			return 1;
	}
	else
	return 0;
}

#endif




int himax_loadSensorConfig(struct i2c_client *client, struct himax_i2c_platform_data *pdata)
{
#if defined(HX_LOADIN_CONFIG)||defined(HX_AUTO_UPDATE_CONFIG)
	int rc= 0;
#endif

#ifndef CONFIG_OF
#if defined(HX_LOADIN_CONFIG)||defined(HX_AUTO_UPDATE_CONFIG)
	int i = 0;
#endif
#endif
	char data[12] = {0};

	if (!client) {
		tp_log_err("%s: Necessary parameters client are null!\n", __func__);
		return -1;
	}

	if(config_load == false)
	{
		config_selected = kzalloc(sizeof(*config_selected), GFP_KERNEL);
		if (config_selected == NULL) {
			tp_log_err("%s: alloc config_selected fail!\n", __func__);
			return -1;
		}
	}
#ifndef CONFIG_OF
	pdata = client->dev.platform_data;
		if (!pdata) {
			tp_log_err("%s: Necessary parameters pdata are null!\n", __func__);
			return -1;
		}
#endif

#if defined(HX_LOADIN_CONFIG)||defined(HX_AUTO_UPDATE_CONFIG)
#ifdef CONFIG_OF
		tp_log_info("%s, config_selected, %X\n", __func__ ,(uint32_t)config_selected);
		if(config_load == false)
		{
			rc = himax_parse_config(private_ts, config_selected);
			if (rc < 0) {
				tp_log_err(" DT:cfg table parser FAIL. ret=%d\n", rc);
				goto HimaxErr;
			} else if (rc == 0){
				if ((private_ts->tw_x_max)&&(private_ts->tw_y_max))
				{
					pdata->abs_x_min = private_ts->tw_x_min;
					pdata->abs_x_max = private_ts->tw_x_max;
					pdata->abs_y_min = private_ts->tw_y_min;
					pdata->abs_y_max = private_ts->tw_y_max;
					tp_log_info(" DT-%s:config-panel-coords = %d, %d, %d, %d\n", __func__, pdata->abs_x_min,
					pdata->abs_x_max, pdata->abs_y_min, pdata->abs_y_max);
				}
				if ((private_ts->pl_x_max)&&(private_ts->pl_y_max))
				{
					pdata->screenWidth = private_ts->pl_x_max;
					pdata->screenHeight= private_ts->pl_y_max;
					tp_log_info(" DT-%s:config-display-coords = (%d, %d)", __func__, pdata->screenWidth,
					pdata->screenHeight);
				}
				config_load = true;
				tp_log_info(" DT parser Done\n");
			}
		}
#else
		tp_log_info("pdata->hx_config_size=%x.\n",(pdata->hx_config_size+1));
		tp_log_info("config_type_size=%x.\n",sizeof(struct himax_config));

		if (pdata->hx_config)
		{
			for (i = 0; i < pdata->hx_config_size/sizeof(struct himax_config); ++i) {
				tp_log_info("(pdata->hx_config)[%x].version=%x.\n",i,(pdata->hx_config)[i].fw_ver);
				tp_log_info("(pdata->hx_config)[%x].sensor_id=%x.\n",i,(pdata->hx_config)[i].sensor_id);

				if (private_ts->vendor_fw_ver_H < (pdata->hx_config)[i].fw_ver) {
					continue;
				}else{
					if ((private_ts->vendor_sensor_id == (pdata->hx_config)[i].sensor_id)) {
						config_selected = &((pdata->hx_config)[i]);
						tp_log_info("hx_config selected, %X\n", (uint32_t)config_selected);
						config_load = true;
						break;
					}
					else if ((pdata->hx_config)[i].default_cfg) {
						tp_log_info("default_cfg detected.\n");
						config_selected = &((pdata->hx_config)[i]);
						tp_log_info("hx_config selected, %X\n", (uint32_t)config_selected);
						config_load = true;
						break;
					}
				}
			}
		}
		else
		{
			tp_log_err("[HimaxError] %s pdata->hx_config is not exist \n",__func__);
			goto HimaxErr;
		}
#endif
#endif
//initial command
//data[0] = 0x42; data[1] = 0x02;
//i2c_himax_master_write(client, &data[0],2,DEFAULT_RETRY_CNT);
//msleep(1);
//
//data[0] = 0x36; data[1] = 0x0F; data[2] = 0x53;
//i2c_himax_master_write(client, &data[0],3,DEFAULT_RETRY_CNT);
//msleep(1);

data[0] = 0x97; data[1] = 0x06; data[2] = 0x03;
i2c_himax_master_write(client, &data[0],3,DEFAULT_RETRY_CNT);
mdelay(1);

data[0] = 0xA6; data[1] = 0x11; data[2] = 0x00;
i2c_himax_master_write(client, &data[0],3,DEFAULT_RETRY_CNT);
mdelay(1);

#ifdef HX_LOADIN_CONFIG
		if (config_selected) {
			//Read config id
			private_ts->vendor_config_ver = config_selected->c36[1];

			//Normal Register c1~c35
			i2c_himax_master_write(client, config_selected->c1,sizeof(config_selected->c1),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c2,sizeof(config_selected->c2),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c3,sizeof(config_selected->c3),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c4,sizeof(config_selected->c4),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c5,sizeof(config_selected->c5),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c6,sizeof(config_selected->c6),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c7,sizeof(config_selected->c7),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c8,sizeof(config_selected->c8),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c9,sizeof(config_selected->c9),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c10,sizeof(config_selected->c10),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c11,sizeof(config_selected->c11),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c12,sizeof(config_selected->c12),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c13,sizeof(config_selected->c13),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c14,sizeof(config_selected->c14),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c15,sizeof(config_selected->c15),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c16,sizeof(config_selected->c16),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c17,sizeof(config_selected->c17),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c18,sizeof(config_selected->c18),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c19,sizeof(config_selected->c19),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c20,sizeof(config_selected->c20),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c21,sizeof(config_selected->c21),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c22,sizeof(config_selected->c22),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c23,sizeof(config_selected->c23),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c24,sizeof(config_selected->c24),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c25,sizeof(config_selected->c25),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c26,sizeof(config_selected->c26),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c27,sizeof(config_selected->c27),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c28,sizeof(config_selected->c28),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c29,sizeof(config_selected->c29),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c30,sizeof(config_selected->c30),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c31,sizeof(config_selected->c31),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c32,sizeof(config_selected->c32),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c33,sizeof(config_selected->c33),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c34,sizeof(config_selected->c34),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c35,sizeof(config_selected->c35),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c36,sizeof(config_selected->c36),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c37,sizeof(config_selected->c37),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c38,sizeof(config_selected->c38),DEFAULT_RETRY_CNT);
			i2c_himax_master_write(client, config_selected->c39,sizeof(config_selected->c39),DEFAULT_RETRY_CNT);

			//Config Bank register
			himax_config_reg_write(client, 0x00,config_selected->c40,sizeof(config_selected->c40),DEFAULT_RETRY_CNT);
			himax_config_reg_write(client, 0x9E,config_selected->c41,sizeof(config_selected->c41),DEFAULT_RETRY_CNT);

			msleep(1);
		} else {
			tp_log_err("[HimaxError] %s config_selected is null.\n",__func__);
			goto HimaxErr;
		}
#endif
#ifdef HX_ESD_WORKAROUND
	//Check R36 to check IC Status
	i2c_himax_read(client, 0x36, data, 2, 10);
	if(data[0] != 0x0F || data[1] != 0x53)
	{
		//IC is abnormal
		tp_log_err("[HimaxError] %s R36 Fail : R36[0]=%d,R36[1]=%d,R36 Counter=%d \n",__func__,data[0],data[1],ESD_R36_FAIL);
		return -1;
	}
#endif

#ifdef HX_AUTO_UPDATE_CONFIG

		if(i_update_FWCFG(config_selected)==false)
			tp_log_info("NOT Have new FWCFG=NOT UPDATE=\n");
		else
			tp_log_info("Have new FWCFG=UPDATE=\n");
#endif

	himax_power_on_initCMD(client);

	tp_log_info("%s: initialization complete\n", __func__);

	return 1;
#if defined(HX_LOADIN_CONFIG)||defined(HX_AUTO_UPDATE_CONFIG)
HimaxErr:
	return -1;
#endif
}

static int himax_file_open_firmware(u8 *file_path, u8 *databuf,
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


int himax_check_update_firmware_flag(void)
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
	ret = himax_file_open_firmware(file_path, file_data, &file_size);
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
