/* <DTS2014051302540 zhuchengming 20140513 begin */
/* <DTS2015051109283 jiweifeng/jwx206032 20150515 begin */
/*************************************************
  Copyright (C), 1988-1999, Huawei Tech. Co., Ltd.
  File name        : Sensor_otp_common_if.h 
  Author            : z00278703
  Version           : Initial Draft  
  Date               : 2014/05/14
  Description   : this file contions otp function we used. We 
               define a struct that contion otp we support
               and the otp function.
  Function List    :  
           is_exist_otp_function
           s5k4e1_liteon_13p1_otp_func
           ov5648_sunny_p5v18g_otp_func
  History            : 
  1.Date            : 2014/05/14
     Author   : z00278703
     Modification  : Created File

*************************************************/



#ifndef _SENSOR_OTP_COMMON_IF_H
#define _SENSOR_OTP_COMMON_IF_H


struct otp_function_t {
	char sensor_name[32];
	int (*sensor_otp_function) (struct msm_sensor_ctrl_t *s_ctrl, int index);
	uint32_t rg_ratio_typical;//r/g ratio
	uint32_t bg_ratio_typical;//b/g ratio
/* < DTS2014073004411 yangzhenxi 20140731 begin */
	bool is_boot_load; //whether load otp info when booting.
/* DTS2014073004411 yangzhenxi 20140731 end >*/
};


extern bool is_exist_otp_function(struct msm_sensor_ctrl_t *sctl, int32_t *index);
extern int s5k4e1_liteon_13p1_otp_func(struct msm_sensor_ctrl_t *s_ctrl, int index);
extern int ov5648_sunny_p5v18g_otp_func(struct msm_sensor_ctrl_t * s_ctrl, int index);
/* <DTS2014061204421 yangzhenxi/WX221546 20140612 begin */
extern int ov8858_foxconn_otp_func(struct msm_sensor_ctrl_t *s_ctrl, int index);
/* <DTS2014061204421 yangzhenxi/WX221546 20140612 end */

extern int ov5648_otp_func(struct msm_sensor_ctrl_t *s_ctrl, int index);
extern int s5k4e1_otp_func(struct msm_sensor_ctrl_t *s_ctrl, int index);

extern int ov13850_otp_func(struct msm_sensor_ctrl_t * s_ctrl, int index);

/* <DTS2014081404502 wangguoying 20140814 begin */
extern int imx328_sunny_p13n10a_otp_func(struct msm_sensor_ctrl_t *s_ctrl, int index);
/* DTS2014081404502 wangguoying 20140814 end> */
/* <DTS2015062302165 w00182304  20150623 begin */
extern int ar1335_sunny_f13m01f_otp_func(struct msm_sensor_ctrl_t * s_ctrl, int index);
/* DTS2015062302165 w00182304  20150623 end> */
/* < DTS2015071606479   wangqiaoli/w00345499 20150716 begin */
extern int imx214_otp_func(struct msm_sensor_ctrl_t * s_ctrl, int index);
/* DTS2015071606479   wangqiaoli/w00345499 20150716 end> */
/* <DTS2015073102718  swx234330 20150731 begin */
extern int imx219_otp_func(struct msm_sensor_ctrl_t * s_ctrl, int index);
/* DTS2015073102718  swx234330 20150731 end >*/
/* <DTS2015101602691 z00285045 20151016 begin */
extern int s5k3m2_otp_func(struct msm_sensor_ctrl_t * s_ctrl, int index);
/* DTS2015101602691 z00285045 20151016 end> */

extern struct otp_function_t otp_function_lists [];


#endif
/* DTS2015051109283 jiweifeng/jwx206032 20150515 end> */
/* DTS2014051302540 zhuchengming 20140513 end> */
