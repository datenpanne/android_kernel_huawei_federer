/* < DTS2014041705903 zhangmin 20140418 begin */
#ifndef _HUAWEI_DEV_DCT_H_
#define _HUAWEI_DEV_DCT_H_

/* < DTS2014122400303 yuanzhen 20141224 begin */
/* hw device list */
enum hw_device_type {
    DEV_I2C_START,
    DEV_I2C_TOUCH_PANEL = DEV_I2C_START,
    DEV_I2C_COMPASS,
    DEV_I2C_G_SENSOR,
    DEV_I2C_CAMERA_MAIN,
    DEV_I2C_CAMERA_SLAVE,
    DEV_I2C_KEYPAD,
    DEV_I2C_APS,
    DEV_I2C_GYROSCOPE,
    DEV_I2C_NFC,
    DEV_I2C_DC_DC,
    DEV_I2C_SPEAKER,
    DEV_I2C_OFN,
    DEV_I2C_FLASH,
    DEV_I2C_L_SENSOR,
    DEV_I2C_CHARGER,
    DEV_I2C_BATTERY,
    DEV_I2C_NTC,
    DEV_I2C_MHL,
    DEV_I2C_AUDIENCE,
    DEV_I2C_USB_SWITCH,
	/* < DTS2015031002441  dingjingfeng 20150310 begin */
	DEV_I2C_SAR_SENSOR,
	/* DTS2015031002441  dingjingfeng 20150310 end > */
    DEV_I2C_MAX,
};
/* DTS2014122400303 yuanzhen 20141224 end > */

/* set a device flag as true */
void set_hw_dev_flag( int dev_id );

#endif
/* DTS2014041705903 zhangmin 20140418 end > */
