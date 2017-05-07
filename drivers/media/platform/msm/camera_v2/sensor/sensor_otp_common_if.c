/* <DTS2014051302540 zhuchengming 20140513 begin */
/* < DTS2014072601546 yanhuiwen/0083550 20140723 begin */
/* <DTS2015051109283 jiweifeng/jwx206032 20150515 begin */
/* < DTS2015071306992 jiweifeng/WX206032 20150713 begin */
/*************************************************
  Copyright (C), 1988-1999, Huawei Tech. Co., Ltd.
  File name        : Sensor_otp_common_if.c
  Author            : z00278703
  Version           : Initial Draft
  Date               : 2014/05/14
  Description   : this file is used by driver to check if the otp
            is supported by the driver.
  Function List    :
            is_exist_otp_function
  History            :
  1.Date            : 2014/05/14
     Author   : z00278703
     Modification  : Created File

*************************************************/

//#define HW_CMR_LOGSWC 0   //file log switch set 0 off,default is 1 on
#define HW_CMR_LOG_TAG "sensor_otp_common_if"

#include <linux/hw_camera_common.h>
#include "msm_sensor.h"
#include "sensor_otp_common_if.h"

/***** 23060132(ov5648/s5k4e1), 2015/07/14 begin *****/
#define OV5648_FOXCONN_132_RG_RATIO_TYPICAL  0x278
#define OV5648_FOXCONN_132_BG_RATIO_TYPICAL  0x2d2

/*< DTS2014121706384 gwx229921 20141217 begin */
#define OV5648_OFILM_132_RG_RATIO_TYPICAL    0x02e8
#define OV5648_OFILM_132_BG_RATIO_TYPICAL    0x0312
/* DTS2014121706384 gwx229921 20141217 end >*/

#define S5K4E1_SUNNY_132_RG_RATIO_TYPICAL     0x30c
#define S5K4E1_SUNNY_132_BG_RATIO_TYPICAL     0x2b1
/***** 23060132(ov5648/s5k4e1), 2015/07/14 end *****/


/***** 23060167(0v13850/imx328), 2015/07/14 begin *****/
/*< DTS2015021200322  jwx206032 20150215 begin */
#define OV13850_OFILM_167_RG_RATIO_TYPICAL    0x248
#define OV13850_OFILM_167_BG_RATIO_TYPICAL    0x257
/* DTS2015021200322  jwx206032 20150215 end >*/

/* <DTS2014081404502 wangguoying 20140814 begin */
#define IMX328_SUNNY_167_RG_RATIO_TYPICAL      0x01ED
#define IMX328_SUNNY_167_BG_RATIO_TYPICAL      0x0172
/* DTS2014081404502 wangguoying 20140814 end> */

#define OV13850_LITEON_167_RG_RATIO_TYPICAL    0x248
#define OV13850_LITEON_167_BG_RATIO_TYPICAL    0x257
/***** 23060167(0v13850/imx328), 2015/07/14 end *****/


/***** 23060193(0v13850/ar1335), 2015/07/14 begin *****/
/* average of only two golden values(20150519) */
#define OV13850_OFILM_193_RG_RATIO_TYPICAL  0x264
#define OV13850_OFILM_193_BG_RATIO_TYPICAL  0x24c

/* average of 5 golden values OK(20150519) */
#define OV13850_LITEON_193_RG_RATIO_TYPICAL    0x242
#define OV13850_LITEON_193_BG_RATIO_TYPICAL    0x233

#define AR1335_SUNNY_193_RG_RATIO_TYPICAL    0x26D
#define AR1335_SUNNY_193_BG_RATIO_TYPICAL    0x267
/***** 23060193(0v13850/ar1335), 2015/07/14 end *****/

/********* 23060156(imx214), 2015/07/16 begin ************/
/* < DTS2015071606479   wangqiaoli/w00345499 20150716 begin */
#define IMX214_FOXCONN_RG_RATIO_TYPICAL 0x1E0
#define IMX214_FOXCONN_BG_RATIO_TYPICAL 0x179
#define IMX214_SUNNY_RG_RATIO_TYPICAL 0x1DD
#define IMX214_SUNNY_BG_RATIO_TYPICAL 0x192
/* DTS2015071606479   wangqiaoli/w00345499 20150716 end> */
/*<DTS2015073008870 wangqiaoli/w00345499 20150730 begin*/
#define IMX214_OFILM_RG_RATIO_TYPICAL 0x1E1
#define IMX214_OFILM_BG_RATIO_TYPICAL 0x1A3
/*DTS2015073008870 wangqiaoli/w00345499 20150730 end>*/
/********* 23060156(imx214), 2015/07/16 end ************/

/* <DTS2015073102718  swx234330 20150731 begin */
/* < DTS2015071601105  maliang wx289078 20150716 begin */
#define OV8858_FOXCONN_PAD_RG_RATIO_TYPICAL 0x24B
#define OV8858_FOXCONN_PAD_BG_RATIO_TYPICAL 0x274
/* DTS2015071601105  maliang wx289078 20150716 end >*/

#define IMX219_LITEON_PAD_RG_RATIO_TYPICAL 0x23A
#define IMX219_LITEON_PAD_BG_RATIO_TYPICAL 0x251

#define IMX219_OFILM_PAD_RG_RATIO_TYPICAL 0x271
#define IMX219_OFILM_PAD_BG_RATIO_TYPICAL 0x2ba
/* DTS2015073102718  swx234330 20150731 end >*/

/* <DTS2015101602691 z00285045 20151016 begin */
#define S5K3M2_SUNNY_RG_RATIO_TYPICAL 0x1FE
#define S5K3M2_SUNNY_BG_RATIO_TYPICAL 0x224
/* DTS2015101602691 z00285045 20151016 end> */

/* < DTS2015072100317 maliang wx289078 20150721 begin */
struct otp_function_t otp_function_lists []=
{
	#include "sensor_otp_kiw.h"
	#include "sensor_otp_chm.h"
	#include "sensor_otp_ale.h"
	#include "sensor_otp_t2.h"
};
/* DTS2015072100317  maliang wx289078 20150721  end >*/

/*************************************************
  Function    : is_exist_otp_function
  Description: Detect the otp we support
  Calls:
  Called By  : msm_sensor_config
  Input       : s_ctrl
  Output     : index
  Return      : true describe the otp we support
                false describe the otp we don't support

*************************************************/
bool is_exist_otp_function( struct msm_sensor_ctrl_t *s_ctrl, int32_t *index)
{
	int32_t i = 0;

	for (i=0; i<(sizeof(otp_function_lists)/sizeof(otp_function_lists[0])); ++i)
	{
		/* <DTS2014092901923 ganfan 20140929 begin */
        if(strlen(s_ctrl->sensordata->sensor_name) != strlen(otp_function_lists[i].sensor_name))
            continue;
		/* DTS2014092901923 ganfan 20140929 end> */
		if (0 == strncmp(s_ctrl->sensordata->sensor_name, otp_function_lists[i].sensor_name, strlen(s_ctrl->sensordata->sensor_name)))
		{
			*index = i;
			CMR_LOGI("is_exist_otp_function success i = %d\n", i);
			return true;
		}
	}
	return false;
}
/* DTS2015071306992 jiweifeng/WX206032 20150713 end > */
/* DTS2015051109283 jiweifeng/jwx206032 20150515 end> */
/* DTS2014072601546 yanhuiwen/00283550 20140723 end > */

/* DTS2014051302540 zhuchengming 20140513 end> */
