/*< DTS2015012606635 tangying/205982 20150126 begin*/
/*add camera ois driver*/
/*< DTS2015012906386 tangying/205982 20150130 begin*/
/*adjust back camera resolution and optimize ois driver*/
#ifndef _MINIISP_OIS_INTERFACE_HEAD
#define _MINIISP_OIS_INTERFACE_HEAD

#define SUNNY_MODULE_ID   0x1
#define LITEON_MODULE_ID  0x3
#define LGIT_MODULE_ID    0x7

/*< DTS2015040101432 ganfan/206212 20150401 begin*/
typedef enum
{
    OIS_PREVIEW,
	OIS_CAPTURE,
}OIS_HALL_LIMIT_TYPE;

#define		X_CURRENT_LMT_PREVIEW	0x3F666666//0.9
#define		Y_CURRENT_LMT_PREVIEW	0x3F666666//0.9
#define		X_CURRENT_LMT_CAPTURE	0x3F666666// 0.9
#define		Y_CURRENT_LMT_CAPTURE	0x3F666666// 0.9
/*DTS2015040101432 ganfan/206212 20150401 end >*/


/* <DTS2015073106553 xurenjie xwx208548 20150731 begin */
//delete ois otp
/* DTS2015073106553 xurenjie xwx208548 20150731 end > */
int32_t mini_isp_ois_init(int32_t module_id);
/*< DTS2015042909943 ganfan/206212 20150505 begin*/
int32_t mini_isp_ois_turn_on(int32_t module_id, uint32_t onoff, int32_t turn_on_type);
/*DTS2015042909943 ganfan/206212 20150505 end >*/
int mini_isp_ois_start_running_test(int32_t module_id);
/*< DTS2015071100243 ganfan/206212 20150714 begin*/
/*delete mini_isp_ois_start_mmi_test function, no use*/
/*DTS2015071100243 ganfan/206212 20150714 end >*/
/*< DTS2015042007464 ganfan/206212 20150420 begin*/
int32_t mini_isp_ois_start_mag_test(int32_t module_id, void *userdata);
/*DTS2015042007464 ganfan/206212 20150420 end >*/
void mini_isp_init_exit_flag(int flag);
/* <DTS2015073106553 xurenjie xwx208548 20150731 begin */
int32_t mini_isp_ois_setGyroGain(int32_t module_id, int32_t new_xgain, int32_t new_ygain);
int32_t mini_isp_ois_getGyroGain(int32_t module_id, int32_t* xgain, int32_t* ygain);
/* DTS2015073106553 xurenjie xwx208548 20150731 end > */
    
int mini_isp_ois_RamWrite32A(uint32_t addr, uint32_t data);
int mini_isp_ois_RamRead32A(uint16_t addr, void* data);
int mini_isp_ois_RamWrite16A(uint16_t addr, uint16_t data);
int mini_isp_ois_RamRead16A(uint16_t addr, uint16_t *data);
int mini_isp_ois_RegWrite8A(uint16_t addr, uint16_t data);
int mini_isp_ois_RegRead8A(uint16_t addr, uint8_t *data);
void mini_isp_ois_WitTim(uint16_t delay);
int mini_isp_ois_RamWrite16Burst(uint8_t data[], uint16_t length);
int mini_isp_ois_RegWrite8Burst(uint8_t data[], uint16_t length);
int mini_isp_ois_RamWrite32Burst(uint8_t data[], uint16_t length);

#endif
/*DTS2015012906386 tangying/205982 20150130 end >*/
/*DTS2015012606635 tangying/205982 20150126 end >*/
