/* <DTS2014120200352 weiqiangqiang 20140212 begin */
/*move hw_tp_common.h to here*/
/* < DTS2014111407354  weiqiangqiang 20141115 begin */
/*Update from android kk to L version*/
/* < DTS2014042402686 sunlibin 20140424 begin */
/*Add for huawei TP*/
/*
 * Copyright (c) 2014 Huawei Device Company
 *
 * This file provide common requeirment for different touch IC.
 * 
 * 2014-01-04:Add "tp_get_touch_screen_obj" by sunlibin
 *
 */
#ifndef __HW_TP_COMMON__
#define __HW_TP_COMMON__

/* < DTS2014052402264 shenjinming 20140526 begin */
/* delete 2 lines */
/* DTS2014052402264 shenjinming 20140526 end > */
/*IC type*/
#define IC_TYPE_3207 3207

#define FW_OFILM_STR "000"
#define FW_EELY_STR "001"
#define FW_TRULY_STR "002"
#define FW_MUTTO_STR "003"
#define FW_JUNDA_STR "005"
#define FW_LENSONE_STR "006"

#define MODULE_STR_LEN 3

/* < DTS2014052402264 shenjinming 20140526 begin */
/* buffer size for dsm tp client */
#define TP_RADAR_BUF_MAX	4096
/* DTS2014052402264 shenjinming 20140526 end > */

enum f54_product_module_name {
	FW_OFILM = 0,
	FW_EELY = 1,
	FW_TRULY = 2,
	FW_MUTTO = 3,
	/* < DTS2014070819376 sunlibin 20140709 begin */
	/*Modify G760L tp_cap threshold get from V3*/
	FW_GIS = 4,

	FW_JUNDA = 5,
	FW_LENSONE = 6,
	/*< DTS2015033003517 zhangyonggang 20150402 begin */
	FW_YASSY = 7,
	/* DTS2015033003517 zhangyonggang 20150402 end > */
	/* DTS2014070819376 sunlibin 20140709 end> */
	
	UNKNOW_PRODUCT_MODULE = 0xff,
};
struct holster_mode{
	unsigned long holster_enable;
	int top_left_x0;
	int top_left_y0;
	int bottom_right_x1;
	int bottom_right_y1;
};

struct kobject* tp_get_touch_screen_obj(void);
/* <DTS2014120803003 songrongyuan 20141208 begin */
struct kobject* tp_get_virtual_key_obj(char *name);
/* DTS2014120803003 songrongyuan 20141208 end> */
struct kobject* tp_get_glove_func_obj(void);

/* < DTS2014052402264 shenjinming 20140526 begin */
/* delete the func declare */
/* DTS2014052402264 shenjinming 20140526 end > */

/* < DTS2014071601734 yanghaizhou 20140715 begin */
/* add phone name so that a tp-driver can behave differentlly
accroding to different products*/
/* < DTS2015042202611 yuanbo 20150422 begin */
#define PHONE_NAME_Y550      "Y550"
#define PHONE_NAME_ULC02    "ULC02"
/* < DTS2014122210216 songrongyuan 20141222 begin */
#define PHONE_NAME_RIO	"Rio"
/* DTS2014122210216 songrongyuan 20141222 end > */
/* DTS2014071601734 yanghaizhou 20140715 end > */
/* < DTS2015062406532 yuanbo 20150624 begin */
#define PHONE_NAME_Y538     "Y538"
#define PHONE_NAME_SCALE    "SCALE"
#define PHONE_NAME_KIWI     "Kiwi"
#define PHONE_NAME_ALICE    "Alice"
/* DTS2015062406532 yuanbo 20150624 end > */
/* DTS2015042202611 yuanbo 20150422 end > */

/* < DTS2014082603541 yanghaizhou 20140826 begin */
unsigned char already_has_tp_driver_running(void);
void set_tp_driver_running(void);
/* DTS2014082603541 yanghaizhou 20140826 end > */
/* < DTS2015033003517 zhangyonggang 20150402 begin */
int get_tp_type(void);
void set_tp_type(int type);
/* DTS2015033003517 zhangyonggang 20150402 end > */
#define MMITEST

#endif
#define TP_ERR  1
#define TP_WARNING 2
#define TP_INFO 3
#define TP_DBG  4
#define TP_VDBG  5

#if CONFIG_DEBUG_HUAWEI_FLOW_LOGLEVEL
extern int KERNEL_HWFLOW;
#else
#define KERNEL_HWFLOW 	1
#endif 
/* < DTS2015010501865 songrongyuan 20140105 begin */
extern int hw_tp_common_debug_mask;
#ifndef TP_LOG_NAME
#define TP_LOG_NAME "[COMMON]"
#endif
#ifndef tp_log_err
#define tp_log_err(x...)                \
do{                                     \
    if( hw_tp_common_debug_mask >= TP_ERR )   \
    {                                   \
 		printk(KERN_ERR TP_LOG_NAME "[ERR]" x); 	\
    }                                   \
                                        \
}while(0)
#endif

#ifndef tp_log_warning
#define tp_log_warning(x...)               \
do{                                     \
    if( hw_tp_common_debug_mask >= TP_WARNING )  \
    {                                   \
 		printk(KERN_ERR TP_LOG_NAME "[WARNING]" x); 	\
    }                                   \
                                        \
}while(0)
#endif

#ifndef tp_log_info
#define tp_log_info(x...)               \
do{                                     \
    if( (KERNEL_HWFLOW)  &&  (hw_tp_common_debug_mask >= TP_INFO))  \
    {                                   \
 		printk(KERN_ERR TP_LOG_NAME "[INFO]" x); 	\
    }                                   \
                                        \
}while(0)
#endif

#ifndef tp_log_debug
#define tp_log_debug(x...)              \
do{                                     \
    if( (KERNEL_HWFLOW)  &&  (hw_tp_common_debug_mask >= TP_DBG) )   \
    {                                   \
 		printk(KERN_ERR TP_LOG_NAME "[DBG]" x); 	\
    }                                   \
                                        \
}while(0)
#endif

#ifndef tp_log_vdebug
#define tp_log_vdebug(x...)              \
do{                                     \
    if( (KERNEL_HWFLOW)  && (hw_tp_common_debug_mask >= TP_VDBG) )   \
    {                                   \
 		printk(KERN_ERR TP_LOG_NAME "[VDBG]" x); 	\
    }                                   \
                                        \
}while(0)
#endif
/* DTS2015010501865 songrongyuan 20140105 end >*/
/* DTS2014042402686 sunlibin 20140424 end> */
/* DTS2014111407354  weiqiangqiang 20141115 end >*/ 
/*< DTS2015062403881 zhoumin/wx222300 20150624 begin */
extern bool pt_test_enable_tp;
/* DTS2015062403881 zhoumin/wx222300 20150624 end >*/
/* < DTS2014062006092  songrongyuan 20140620 begin */

extern struct touch_hw_platform_data touch_hw_data;

struct touch_hw_platform_data
{
        int (*touch_power)(int on);     /* Only valid in first array entry */
        int (*set_touch_interrupt_gpio)(void);/*it will config the gpio*/
        void (*set_touch_probe_flag)(int detected);/*we use this to detect the probe is detected*/
        int (*read_touch_probe_flag)(void);/*when the touch is find we return a value*/
        int (*touch_reset)(void);
        int (*get_touch_reset_gpio)(void);
       // int (*get_touch_resolution)(struct tp_resolution_conversion *tp_resolution_type);/*add this function for judge the tp type*/
        int (*read_button_flag)(void);
       // int (*get_touch_button_map)(struct tp_button_map *tp_button_map);
};

/* DTS2014062006092  songrongyuan 20140620 end >*/
/* DTS2014120200352 weiqiangqiang 20140212 end> */
