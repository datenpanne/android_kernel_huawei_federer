/* < DTS2014042905347 zhaoyuxia 20140429 begin */
/* Copyright (c), Code HUAWEI. All rights reserved.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef HW_LCD_COMMON_H
#define HW_LCD_COMMON_H

/* < DTS2014111001776 zhaoyuxia 20141114 begin */
/* remove to hw_lcd_debug.h */
/* DTS2014111001776 zhaoyuxia 20141114 end > */
/* < DTS2015012707755 zhaoyuxia 20140128 begin */
#ifdef CONFIG_HUAWEI_DSM
#include <linux/dsm_pub.h>
#endif
/* DTS2015012707755 zhaoyuxia 20140128 end > */
/* DTS2014051603610 zhaoyuxia 20140516 end > */

/* Add dynamic_log interface */
#define LCD_ERR  1
#define LCD_INFO 2
#define LCD_DBG  3

#define OPER_READ  (1)
#define OPER_WRITE (2)
#define MIPI_DCS_COMMAND (1<<0)
#define MIPI_GEN_COMMAND 4
#define MIPI_PATH_OPEN		1
#define MIPI_PATH_CLOSE	0
#ifdef CONFIG_DEBUG_FS
extern atomic_t mipi_path_status;
#endif
/* DTS2014050808504 daiyuhong 20140508 end > */
/* <DTS2015010801501 sunlibin 20150108 begin */
/* Modify JDI tp/lcd power on/off to reduce power consumption */
enum
{
	LENS_JOINT  = 0,
	LENS_ONCELL = 1,
	LENS_INCELL = 2,
};
/* DTS2015010801501 sunlibin 20150108 end> */
/* DTS2015030903154 fengzhiqiang/00217916 20150309 begin */
int get_tp_gesture_enable_status(void);
void set_tp_gesture_enable_status(int type);
/* DTS2015030903154 fengzhiqiang/00217916 20140309 end > */
/* < DTS2015080704409 tangying 20150908 begin */
/*
*merge qcom patch from 02098626: 08_20_patches.zip
*add delay time before vddio-incell enable for ATH JDINT35695. if vddio-incell pull down time is smaller than 80ms.
*/
void set_tp_vddio_poweroff_time(unsigned long jz);
unsigned long get_tp_vddio_poweroff_time(void);
/* DTS2015080704409 tangying 20150908 end > */
extern int lcd_debug_mask ;
/* < DTS2014080106240 renxigang 20140801 begin */
/* < DTS2015032505215 tianye 20150325 begin */
/*add power status error judge,avoid red screen*/
struct lcd_pwr_status_t
{
	int panel_power_on;
	int lcd_dcm_pwr_status;
	struct timer_list lcd_dsm_t;
	struct tm tm_unblank;
	struct timeval tvl_unblank;
	struct tm tm_lcd_on;
	struct timeval tvl_lcd_on;
	struct tm tm_set_frame;
	struct timeval tvl_set_frame;
	struct tm tm_backlight;
	struct timeval tvl_backlight;
};
extern int lcd_dcm_pwr_status;
extern struct lcd_pwr_status_t lcd_pwr_status;

#define  LCD_PWR_STAT_GOOD 0x000f
#define  LCD_PWR_STAT_IS_GOOD 0x0000
/* DTS2015032505215 tianye 20150325 end > */
/* DTS2014080106240 renxigang 20140801 end > */
/* < DTS2015072108592 tianye 20150731 begin */
/* FPC unlock can't light lcd backlight */
int get_lcd_power_delay_time(void);
void set_lcd_power_delay_time(int delay_timeValue);
/* DTS2015072108592 tianye 20150731 end > */
/* < DTS2015102906691 tianye 20151109 begin */
/*open tp gesture can't wake up screen probability*/
bool get_tp_reset_enable(void);
void set_tp_reset_status(bool ath_tp_reset);
/* DTS2015102906691 tianye 20151109 end > */
#ifndef LCD_LOG_ERR
#define LCD_LOG_ERR( x...)					\
do{											\
	if( lcd_debug_mask >= LCD_ERR )			\
	{										\
		printk(KERN_ERR "[LCD_ERR] " x);	\
	}										\
											\
}while(0)
#endif

#ifndef LCD_LOG_INFO
#define LCD_LOG_INFO( x...)					\
do{											\
	if( lcd_debug_mask >= LCD_INFO )		\
	{										\
		printk(KERN_ERR "[LCD_INFO] " x);	\
	}										\
											\
}while(0)
#endif

/* < DTS2014071607596 renxigang 20140717 begin */
/*KERNEL_HWFLOW is for controlling all the log of devices*/
#ifndef LCD_LOG_DBG
#define LCD_LOG_DBG( x...)					\
do{											\
	if( (KERNEL_HWFLOW) && (lcd_debug_mask >= LCD_DBG) )			\
	{										\
		printk(KERN_ERR "[LCD_DBG] " x);	\
	}										\
											\
}while(0)
#endif
/* DTS2014071607596 renxigang 20140717 end >*/

/* LCD_MDELAY will select mdelay or msleep according value */
#define LCD_MDELAY(time_ms)   	\
	do							\
	{ 							\
		if (time_ms>10)			\
			msleep(time_ms);	\
		else					\
			mdelay(time_ms);	\
	}while(0)	

#endif
/* DTS2014042905347 zhaoyuxia 20140429 end >*/
/* < DTS2014050808504 daiyuhong 20140508 begin */
/* < DTS2014111001776 zhaoyuxia 20141114 begin */
/* remove to hw_lcd_debug.h */
/* < DTS2014051603610 zhaoyuxia 20140516 begin */
/* < DTS2015012707755 zhaoyuxia 20140128 begin */
#ifdef CONFIG_HUAWEI_DSM
extern struct dsm_client *lcd_dclient;
#else
#define DSM_LCD_MIPI_ERROR_NO				(20100)
#define DSM_LCD_TE_TIME_OUT_ERROR_NO		(20101)
#define DSM_LCD_STATUS_ERROR_NO				(20102)
#define DSM_LCD_PWM_ERROR_NO				(20104)
#define DSM_LCD_BRIGHTNESS_ERROR_NO			(20105)
#define DSM_LCD_MDSS_DSI_ISR_ERROR_NO		(20106)
#define DSM_LCD_MDSS_MDP_ISR_ERROR_NO		(20107)
#define DSM_LCD_ESD_STATUS_ERROR_NO			(20108)
#define DSM_LCD_ESD_REBOOT_ERROR_NO			(20109)
#define DSM_LCD_POWER_STATUS_ERROR_NO		(20110)
#define DSM_LCD_MDSS_UNDERRUN_ERROR_NO		(20111)
#define DSM_LCD_MDSS_IOMMU_ERROR_NO			(20112)
#define DSM_LCD_MDSS_PIPE_ERROR_NO			(20113)
#define DSM_LCD_MDSS_PINGPONG_ERROR_NO		(20114)
#define DSM_LCD_MDSS_VSP_ERROR_NO			(20115)
#define DSM_LCD_MDSS_ROTATOR_ERROR_NO		(20116)
#define DSM_LCD_MDSS_FENCE_ERROR_NO			(20117)
#define DSM_LCD_MDSS_CMD_STOP_ERROR_NO		(20118)
#define DSM_LCD_MDSS_VIDEO_DISPLAY_ERROR_NO	(20119)
#define DSM_LCD_MDSS_MDP_CLK_ERROR_NO		(20120)
#define DSM_LCD_MDSS_MDP_BUSY_ERROR_NO		(20121)
#endif
/* DTS2015012707755 zhaoyuxia 20140128 end > */
int lcd_report_dsm_err(int type,  int err_value,int add_value);
int mdss_record_dsm_err(u32 *dsi_status);
/* DTS2014111001776 zhaoyuxia 20141114 end > */
/* DTS2014051603610 zhaoyuxia 20140516 end > */
/* < DTS2014080106240 renxigang 20140801 begin */
void lcd_dcm_pwr_status_handler(unsigned long data);
/* DTS2014080106240 renxigang 20140801 end > */
/* < DTS2014101301850 zhoujian 20141013 begin */
/*DTS2015032801385 renxigang 20150328 begin */
/*delete mdss_mdp_get_clk_rate() to avoid panic*/
void mdp_underrun_dsm_report(unsigned long num,unsigned long underrun_cnt);
/*DTS2015032801385 renxigang 20150328 end > */
/* DTS2014101301850 zhoujian 20141013 end > */
/* DTS2014050808504 daiyuhong 20140508 end > */
