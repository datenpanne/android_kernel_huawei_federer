/* < DTS2015020904827 yuguangcai 20150209 begin */

#ifndef _SENSOR_OTP_COMMON_H_
#define _SENSOR_OTP_COMMON_H_

#include "msm_sensor.h"
#include <media/msm_cam_sensor.h>

struct otp_data_t {
	char sensor_name[32];
	struct msm_sensor_otp_info *otp_info;
};

extern uint8_t get_otp_index(struct msm_sensor_ctrl_t *s_ctrl);
extern struct otp_data_t otp_data_lists[];
/* <DTS2015073106553 xurenjie xwx208548 20150731 begin */
extern struct msm_sensor_otp_info *get_otp_info_by_moduleid(int32_t module_id);
/* DTS2015073106553 xurenjie xwx208548 20150731 end > */
#endif

/* DTS2015020904827 yuguangcai 20150209 end > */
