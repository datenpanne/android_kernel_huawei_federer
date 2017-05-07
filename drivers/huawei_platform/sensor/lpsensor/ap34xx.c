/* < DTS2015112600367  chendong 20151126 begin */
/*
 * This file is part of the AP3426, AP3212C and AP3216C sensor driver.
 * AP3426 is combined proximity and ambient light sensor.
 * AP3216C is combined proximity, ambient light sensor and IRLED.
 *
 * Contact: John Huang <john.huang@dyna-image.com>
 *	    Templeton Tsai <templeton.tsai@dyna-image.com>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 *
 * Filename: ap3426.c
 *
 * Summary:
 *	AP3426 device driver.
 *
 * Modification History:
 * Date     By       Summary
 * -------- -------- -------------------------------------------------------
 * 02/02/12 YC       1. Modify irq function to seperate two interrupt routine.
 *					 2. Fix the index of reg array error in em write.
 * 02/22/12 YC       3. Merge AP3426 and AP3216C into the same driver. (ver 1.8)
 * 03/01/12 YC       Add AP3212C into the driver. (ver 1.8)
 * 07/25/14 John	  Ver.2.1 , ported for Nexus 7
 * 08/21/14 Templeton AP3426 Ver 1.0, ported for Nexus 7
 * 09/24/14 kevin    Modify for Qualcomm8x10 to support device tree
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/of_gpio.h>
#include <linux/wakelock.h>
#include <huawei_platform/sensor/ap34xx.h>
#include <linux/regulator/consumer.h>
#include <linux/ioctl.h>
/* < DTS2016022205149 liuanqi 20160301 begin */
#include <linux/irq.h>
#define IRQ_PENDING 0x00000200
/* DTS2016022205149  liuanqi 20160301  end > */
/* < DTS2015120303965  chendong 20151126 begin */
#include <misc/app_info.h>
/* DTS2015120303965  chendong 20151126 end > */
#ifdef CONFIG_GET_HARDWARE_INFO
#include <soc/qcom/hardware_info.h>
#endif

#define AP3426_DRV_NAME		"ap3426"
#define DRIVER_VERSION		"1.0"

#define PL_TIMER_DELAY 200
/* < DTS2016011406267 chendong 20150118 begin */
#define	ALS_THRES_HIGH 	2000
/* < DTS2016022509409  chendong 20160225 begin */
#define	ALS_THRES_LOW 	500
#define	ALS_LIGHTBAR_VALUE 	1000
#define	DIST_DIFF_LIGHT 	120
#define	ALS_CW_VALUE	24906/10000
#define	ALS_AD_VALUE 8345/10000
/* < DTS2016032507153  chendong 20160325 begin */
#define	ALS_CW_ANGLE 225/200
/* DTS2016032507153  chendong 20160325 end > */
#define   ALS_RANGE_16 16
#define   ALS_LOW_VALUE 500
/* DTS2016022509409  chendong 20160225 end > */
/* misc define */
#define MIN_ALS_POLL_DELAY_MS	110
/* < DTS2016032805348 liuanqi 20160328 begin */
#define AP3426_LUX_MAX		65535
/* DTS2016032805348 liuanqi 20160328  end > */
#define AP3426_VDD_MIN_UV	2000000
#define AP3426_VDD_MAX_UV	3300000
#define AP3426_VIO_MIN_UV	1750000
#define AP3426_VIO_MAX_UV	1950000


/* DTS2016011406267 chendong 20150118 end > */

static int ap3426_power_ctl(struct ap3426_data *data, bool on);
static int ap3426_power_init(struct ap3426_data*data, bool on);

// AP3426 register
static u8 ap3426_reg_to_idx_array[AP3426_MAX_REG_NUM] = {
	0,	1,	2,	0xff,	0xff,	0xff,	3,	0xff,
	0xff,	0xff,	4,	5,	6,	7,	8,	9,
	10,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,	0xff,
	0xff,	0xff,	11,	12,	13,	14,	0xff,	0xff,
	15,	16,	17,	18,	19,	20,	21,	0xff,
	22,	23,	24,	25,	26,	27         //20-2f
};
static u8 ap3426_reg[AP3426_NUM_CACHABLE_REGS] = {
	0x00,0x01,0x02,0x06,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
	0x10,0x1A,0x1B,0x1C,0x1D,0x20,0x21,0x22,0x23,0x24,
	0x25,0x26,0x28,0x29,0x2A,0x2B,0x2C,0x2D
};
// AP3426 range
static int ap3426_range[4] = {32768,8192,2048,512};
static u8 *reg_array = ap3426_reg;
static int *range = ap3426_range;

static int cali = 100;
/* < DTS2016022509409  chendong 20160225 begin */
static int pre_als_value = 30;
/* DTS2016022509409  chendong 20160225 end > */
/* < DTS2016032805348 liuanqi 20160328 begin */
static int als_polling_count=0;
/* DTS2016032805348 liuanqi 20160328  end > */
struct regulator *vdd;
struct regulator *vio;
bool power_enabled;
static struct ap3426_data *pdev_data = NULL;
static int flag=0;

/* < DTS2016011101150 chendong 20150118 begin */
extern bool power_key_ps ;    //the value is true means powerkey is pressed, false means not pressed
/* DTS2016011101150 chendong 20150118 end > */


static int ap3426_debug_mask= 0;

module_param_named(ap3426_debug, ap3426_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

#define AP3426_ERR(x...) do {\
	if (ap3426_debug_mask >=0) \
        printk(KERN_ERR x);\
    } while (0)
#define AP3426_WARN(x...) do {\
	if (ap3426_debug_mask >=0) \
        printk(KERN_ERR x);\
    } while (0)
#define AP3426_INFO(x...) do {\
	if (ap3426_debug_mask >=1) \
        printk(KERN_ERR x);\
    } while (0)
#define AP3426_DEBUG(x...) do {\
    if (ap3426_debug_mask >=2) \
        printk(KERN_ERR x);\
    } while (0)

static struct sensors_classdev sensors_light_cdev = {
	.name = "ap3426-light",
	.vendor = "DI",
	.version = 1,
	.handle = SENSORS_LIGHT_HANDLE,
	.type = SENSOR_TYPE_LIGHT,
	.max_range = "65535",
	.resolution = "1.0",
	.sensor_power = "0.35",
	.min_delay = 0,	/* us */
	.fifo_reserved_event_count = 0,
	.fifo_max_event_count = 0,
	.flags = 0,
	.enabled = 0,
	.delay_msec = 200,
	.sensors_enable = NULL,
	.sensors_poll_delay = NULL,
	.sensors_calibrate = NULL,
	.sensors_write_cal_params = NULL,
	.params = NULL,
};


static struct sensors_classdev sensors_proximity_cdev = {
	.name = "ap3426-proximity",
	.vendor = "DI",
	.version = 1,
	.handle = SENSORS_PROXIMITY_HANDLE,
	.type = SENSOR_TYPE_PROXIMITY,
	.max_range = "5.0",
	.resolution = "5.0",
	.sensor_power = "0.35",
	.min_delay = 0,
	.fifo_reserved_event_count = 0,
	.fifo_max_event_count = 0,
	.flags = 1,
	.enabled = 0,
	.delay_msec = 200,
	.sensors_enable = NULL,
	.sensors_poll_delay = NULL,
	.sensors_calibrate = NULL,
	/* < DTS2016011406267 chendong 20150118 begin */
	.sensors_write_cal_params = NULL,
	/* DTS2016011406267 chendong 20150118 end > */
	.params = NULL,
};

static int __ap3426_read_reg(struct i2c_client *client,
	u32 reg, u8 mask, u8 shift)
{
	struct ap3426_data *data = i2c_get_clientdata(client);

	return (data->reg_cache[ap3426_reg_to_idx_array[reg]] & mask) >> shift;
}

static int __ap3426_write_reg(struct i2c_client *client,
	u32 reg, u8 mask, u8 shift, u8 val)
{
	struct ap3426_data *data = i2c_get_clientdata(client);
	int ret = 0;
	u8 tmp;

	tmp = data->reg_cache[ap3426_reg_to_idx_array[reg]];
	tmp &= ~mask;
	tmp |= val << shift;

	ret = i2c_smbus_write_byte_data(client, reg, tmp);
	if (!ret)
		data->reg_cache[ap3426_reg_to_idx_array[reg]] = tmp;


	return ret;
}

/*
 * internally used functions
 */

/* range */
static int ap3426_get_range(struct i2c_client *client)
{
	u8 idx = __ap3426_read_reg(client, AP3426_REG_ALS_CONF,
		AP3426_ALS_RANGE_MASK, AP3426_ALS_RANGE_SHIFT);
	return range[idx];
}

static int ap3426_set_range(struct i2c_client *client, int range)
{
	return __ap3426_write_reg(client, AP3426_REG_ALS_CONF,
		AP3426_ALS_RANGE_MASK, AP3426_ALS_RANGE_SHIFT, range);
}

/* mode */
static int ap3426_get_mode(struct i2c_client *client)
{
	int ret;

	ret = __ap3426_read_reg(client, AP3426_REG_SYS_CONF,
		AP3426_REG_SYS_CONF_MASK, AP3426_REG_SYS_CONF_SHIFT);
	return ret;
}

static int ap3426_set_mode(struct i2c_client *client, int mode)
{
	int ret;
	ret = __ap3426_write_reg(client, AP3426_REG_SYS_CONF,
		AP3426_REG_SYS_CONF_MASK, AP3426_REG_SYS_CONF_SHIFT, mode);
	return ret;
}

/* ALS low threshold */
static int ap3426_get_althres(struct i2c_client *client)
{
	int lsb, msb;
	lsb = __ap3426_read_reg(client, AP3426_REG_ALS_THDL_L,
	    AP3426_REG_ALS_THDL_L_MASK, AP3426_REG_ALS_THDL_L_SHIFT);
	msb = __ap3426_read_reg(client, AP3426_REG_ALS_THDL_H,
	    AP3426_REG_ALS_THDL_H_MASK, AP3426_REG_ALS_THDL_H_SHIFT);
	return ((msb << 8) | lsb);
}

static int ap3426_set_althres(struct i2c_client *client, int val)
{

	int lsb, msb, err;
	msb = val >> 8;
	lsb = val & AP3426_REG_ALS_THDL_L_MASK;

	err = __ap3426_write_reg(client, AP3426_REG_ALS_THDL_L,
		AP3426_REG_ALS_THDL_L_MASK, AP3426_REG_ALS_THDL_L_SHIFT, lsb);
	if (err)
		return err;

	err = __ap3426_write_reg(client, AP3426_REG_ALS_THDL_H,
	AP3426_REG_ALS_THDL_H_MASK, AP3426_REG_ALS_THDL_H_SHIFT, msb);

	return err;
}

/* ALS high threshold */
static int ap3426_get_ahthres(struct i2c_client *client)
{
	int lsb, msb;
	lsb = __ap3426_read_reg(client, AP3426_REG_ALS_THDH_L,
		AP3426_REG_ALS_THDH_L_MASK, AP3426_REG_ALS_THDH_L_SHIFT);
	msb = __ap3426_read_reg(client, AP3426_REG_ALS_THDH_H,
		AP3426_REG_ALS_THDH_H_MASK, AP3426_REG_ALS_THDH_H_SHIFT);
	return ((msb << 8) | lsb);
}

static int ap3426_set_ahthres(struct i2c_client *client, int val)
{
	int lsb, msb, err;

	msb = val >> 8;
	lsb = val & AP3426_REG_ALS_THDH_L_MASK;

	err = __ap3426_write_reg(client, AP3426_REG_ALS_THDH_L,
		AP3426_REG_ALS_THDH_L_MASK, AP3426_REG_ALS_THDH_L_SHIFT, lsb);
	if (err)
		return err;

	err = __ap3426_write_reg(client, AP3426_REG_ALS_THDH_H,
		AP3426_REG_ALS_THDH_H_MASK, AP3426_REG_ALS_THDH_H_SHIFT, msb);

	return err;
}

static int ap3426_get_plthres(struct i2c_client *client)
{
	int lsb, msb;
	lsb = __ap3426_read_reg(client, AP3426_REG_PS_THDL_L,
		AP3426_REG_PS_THDL_L_MASK, AP3426_REG_PS_THDL_L_SHIFT);
	msb = __ap3426_read_reg(client, AP3426_REG_PS_THDL_H,
		AP3426_REG_PS_THDL_H_MASK, AP3426_REG_PS_THDL_H_SHIFT);
	return ((msb << 8) | lsb);
}

static int ap3426_set_plthres(struct i2c_client *client, int val)
{
	int lsb, msb, err;

	msb = val >> 8;
	lsb = val & AP3426_REG_PS_THDL_L_MASK;

	err = __ap3426_write_reg(client, AP3426_REG_PS_THDL_L,
		AP3426_REG_PS_THDL_L_MASK, AP3426_REG_PS_THDL_L_SHIFT, lsb);
	if (err)
		return err;

	err = __ap3426_write_reg(client, AP3426_REG_PS_THDL_H,
		AP3426_REG_PS_THDL_H_MASK, AP3426_REG_PS_THDL_H_SHIFT, msb);

	return err;
}

static int ap3426_get_phthres(struct i2c_client *client)
{
	int lsb, msb;
	lsb = __ap3426_read_reg(client, AP3426_REG_PS_THDH_L,
	    AP3426_REG_PS_THDH_L_MASK, AP3426_REG_PS_THDH_L_SHIFT);
	msb = __ap3426_read_reg(client, AP3426_REG_PS_THDH_H,
	    AP3426_REG_PS_THDH_H_MASK, AP3426_REG_PS_THDH_H_SHIFT);
	return ((msb << 8) | lsb);
}

static int ap3426_set_phthres(struct i2c_client *client, int val)
{
	int lsb, msb, err;

	msb = val >> 8;
	lsb = val & AP3426_REG_PS_THDH_L_MASK;
	err = __ap3426_write_reg(client, AP3426_REG_PS_THDH_L,
		AP3426_REG_PS_THDH_L_MASK, AP3426_REG_PS_THDH_L_SHIFT, lsb);
	if (err)
		return err;

	err = __ap3426_write_reg(client, AP3426_REG_PS_THDH_H,
		AP3426_REG_PS_THDH_H_MASK, AP3426_REG_PS_THDH_H_SHIFT, msb);

	return err;
}

static int ap3426_get_adc_value(struct i2c_client *client)
{
	unsigned int lsb, msb, val;
#ifdef LSC_DBG
	unsigned int tmp,range;
#endif
	lsb = i2c_smbus_read_byte_data(client, AP3426_REG_ALS_DATA_LOW);
	if (lsb < 0) {
		return lsb;
	}

	msb = i2c_smbus_read_byte_data(client, AP3426_REG_ALS_DATA_HIGH);
	if (msb < 0)
		return msb;

#ifdef LSC_DBG
	range = ap3426_get_range(client);
	tmp = (((msb << 8) | lsb) * range) >> 16;
	tmp = tmp * cali / 100;
#endif
	val = msb << 8 | lsb;

	return val;
}

/* < DTS2016011406267 chendong 20150118 begin */
static int ap3426_get_ir_value(struct i2c_client *client)
{
	unsigned int lsb, msb, val;
	/* < DTS2016022509409  chendong 20160225 begin */
	i2c_smbus_write_byte_data(client, AP3426_REG_PS_LEDD, 0x02);
	i2c_smbus_write_byte_data(client, AP3426_REG_PS_MEAN, 0x01);
	/* < DTS2016021900917  chendong 20160224 begin */
	i2c_smbus_write_byte_data(client, AP3426_REG_PS_INTEGR, 0x07);
	/* DTS2016021900917  chendong 20160224 end > */
	if(pre_als_value >= ALS_LOW_VALUE)
	{
		i2c_smbus_write_byte_data(client, AP3426_REG_ALS_CONF, 0x10);
	}
	else
	{
		i2c_smbus_write_byte_data(client, AP3426_REG_ALS_CONF, 0x30);
	}
	/* DTS2016022509409  chendong 20160225 end > */
	lsb = i2c_smbus_read_byte_data(client, AP3426_REG_IR_DATA_LOW);
	if (lsb < 0) {
		return lsb;
	}
	msb = i2c_smbus_read_byte_data(client, AP3426_REG_IR_DATA_HIGH);
	if (msb < 0){
		return msb;
	}
	val = (msb&0x03) << 8 | lsb;
	return val;
}
/* DTS2016011406267 chendong 20150118 end > */
static int ap3426_get_object(struct i2c_client *client)
{
	int val;

	val = i2c_smbus_read_byte_data(client, AP3426_OBJ_COMMAND);
	val &= AP3426_OBJ_MASK;

	return !(val >> AP3426_OBJ_SHIFT);
}

static int ap3426_get_intstat(struct i2c_client *client)
{
	int val;

	val = i2c_smbus_read_byte_data(client, AP3426_REG_SYS_INTSTATUS);
	val &= AP3426_REG_SYS_INT_MASK;

	return val >> AP3426_REG_SYS_INT_SHIFT;
}

static int ap3426_get_px_value(struct i2c_client *client)
{
	int lsb, msb;

	lsb = i2c_smbus_read_byte_data(client, AP3426_REG_PS_DATA_LOW);
	msb = i2c_smbus_read_byte_data(client, AP3426_REG_PS_DATA_HIGH);

	return (u32)(((msb & AL3426_REG_PS_DATA_HIGH_MASK) << 8) | (lsb & AL3426_REG_PS_DATA_LOW_MASK));
}

void ap3426_ps_enable_work(struct work_struct *work)
{
	struct ap3426_data *ps_data = container_of(work, struct ap3426_data, prox_enable_work);
	int32_t ret;
	int value;
	int i=0;
	int tmp;
	int pxvalue=0;
	int read_count = 0;
	int sum_value = 0;

	AP3426_ERR("%s %d:\n",__func__,__LINE__);
	mutex_lock(&ps_data->lock);
	ret = __ap3426_write_reg(ps_data->client,
		             AP3426_REG_SYS_CONF, AP3426_MODE_MASK, 0,2);
	msleep(10);
	for(i=0;i<4;i++)
	{
		tmp = ap3426_get_px_value(ps_data->client);
		 if(tmp < 0)
		 {
		 	AP3426_ERR("%s %d:i2c read ps data fail. \n",__func__,__LINE__);
			mutex_unlock(&ps_data->lock);
			return;
		 }
		 sum_value += tmp;
		 read_count++;
		 msleep(10);
	}
	ret = __ap3426_write_reg(ps_data->client,
		             AP3426_REG_SYS_CONF, AP3426_MODE_MASK, 0,0);
	if(read_count > 0)
	{
		value=sum_value/read_count;
		AP3426_INFO("%s %d:value=%d sum_value=%d read_count=%d. \n",__func__,__LINE__,
			value,sum_value,read_count);
	}
	else
	{
		value=200;
	}
	if(value == 0)
	{
		value = 200;
	}
	pxvalue=value-ps_data->crosstalk;
	AP3426_INFO("%s %d:px_value=%d ps_data->crosstalk=%d \n",__func__,__LINE__,
			pxvalue,ps_data->crosstalk);
	if((pxvalue < 300)&&(pxvalue > 0))
	{
		ps_data->ps_thres_high=value+ps_data->ps_high;
		ps_data->ps_thres_low =value+ps_data->ps_low;
		flag =1;
		ps_data->prevalue=value;
	}
	else if(pxvalue <= 0)
	{
		ps_data->ps_thres_high=ps_data->crosstalk+ps_data->ps_high;
		ps_data->ps_thres_low =ps_data->crosstalk+ps_data->ps_low;
		flag =1;
		ps_data->prevalue=ps_data->crosstalk;
	}
	else
	{
		if(flag == 1)
		{
			ps_data->ps_thres_high=ps_data->prevalue+ps_data->ps_high;
			ps_data->ps_thres_low =ps_data->prevalue+ps_data->ps_low;
		}
		else
		{
			ps_data->ps_thres_high=ps_data->crosstalk+ps_data->ps_high;
			ps_data->ps_thres_low =ps_data->crosstalk+ps_data->ps_low;
		}
	}
	msleep(10);
	if(ps_data->ps_enable==0)
	{
		ps_data->ps_enable = 1;
		ap3426_set_phthres(ps_data->client,ps_data->ps_thres_high);
		ap3426_set_plthres(ps_data->client, ps_data->ps_thres_low);
		if(ps_data->rels_enable==1)
		{
			ret = __ap3426_write_reg(ps_data->client,
			AP3426_REG_SYS_CONF, AP3426_MODE_MASK, 0,3);
			AP3426_INFO("%s %d:misc_ls_opened=%d \n",__func__,__LINE__,ps_data->rels_enable);
		}
		else
		{
			ret = __ap3426_write_reg(ps_data->client,
			AP3426_REG_SYS_CONF, AP3426_MODE_MASK, 0, 2);
			AP3426_INFO("%s %d:misc_ls_opened=%d \n",__func__,__LINE__,ps_data->rels_enable);
		}
		/* < DTS2016030906110 liuanqi 20160318 begin */
		//delete msleep(150);
		/* DTS2016030906110  liuanqi 20160318 end > */
		enable_irq(ps_data->client->irq);
		enable_irq_wake(ps_data->client->irq);
	}

	if(ret < 0){
		AP3426_ERR("%s %d:ps enable error!!!!!!\n",__func__,__LINE__);
	}
	mutex_unlock(&ps_data->lock);

	return;
}

/* < DTS2016011101150 chendong 20150118 begin */
void ap3426_powerkey_screen_handler(struct work_struct *work)
{
        struct ap3426_data *ps_data = container_of((struct delayed_work *)work, struct ap3426_data, powerkey_work);
        if(power_key_ps)
        {
                AP3426_ERR("%s : power_key_ps (%d) press\n",__func__, power_key_ps);
                power_key_ps=false;
                input_report_abs(ps_data->psensor_input_dev, ABS_DISTANCE, 1);
                input_sync(ps_data->psensor_input_dev);
        }
        schedule_delayed_work(&ps_data->powerkey_work, msecs_to_jiffies(500));
}
/* DTS2016011101150 chendong 20150118 end > */
void ap3426_ps_disable_work(struct work_struct *work)
{
	struct ap3426_data *ps_data = container_of(work, struct ap3426_data, prox_disable_work);
	int32_t ret;

	ktime_t timestamp;
	timestamp = ktime_get_boottime();

	AP3426_ERR("%s %d:\n",__func__,__LINE__);
	mutex_lock(&ps_data->lock);

	if(ps_data->ps_enable==1)
	{
		ps_data->ps_enable= 0;

		if(ps_data->rels_enable==1)
		{
			ap3426_set_phthres(ps_data->client,1023);
			ap3426_set_plthres(ps_data->client, 0);
			ret = __ap3426_write_reg(ps_data->client,
				AP3426_REG_SYS_CONF, AP3426_MODE_MASK, 0, 3);
		}
		else
		{
			ret = __ap3426_write_reg(ps_data->client,
				AP3426_REG_SYS_CONF, AP3426_MODE_MASK, 0, 0);
			AP3426_ERR("%s %d:misc_ls_opened=%d\n",__func__,__LINE__,ps_data->rels_enable);
		}
                /* < DTS2016031405308  chendong 20160315 begin */
		input_report_abs(ps_data->psensor_input_dev, ABS_DISTANCE, 1);
		input_sync(ps_data->psensor_input_dev);
                /* DTS2016031405308  chendong 20160315 end > */
		disable_irq_wake(ps_data->client->irq);
		disable_irq(ps_data->client->irq);
	}


	if(ret < 0){
		AP3426_ERR("%s %d:ps disable error!!!!!!\n",__func__,__LINE__);
	}
	mutex_unlock(&ps_data->lock);

	return;
}

void ap3426_als_enable_work(struct work_struct *work)
{
	struct ap3426_data *ps_data = container_of(work, struct ap3426_data, als_enable_work);
	int32_t ret;

	AP3426_ERR("%s %d:\n",__func__,__LINE__);
	mutex_lock(&ps_data->lock);

	if(ps_data->rels_enable==0)
	{
		ps_data->rels_enable=1;
		if(ps_data->ps_enable==1)
		{
			ret = __ap3426_write_reg(ps_data->client,
			  AP3426_REG_SYS_CONF, AP3426_MODE_MASK, 0, 3);
		}
		else
		{
			 ap3426_set_phthres(ps_data->client,1023);
			 ap3426_set_plthres(ps_data->client, 0);
			 ret = __ap3426_write_reg(ps_data->client,
				   AP3426_REG_SYS_CONF, AP3426_MODE_MASK, 0, 3);
		}
		AP3426_INFO("%s %d:misc_ps_opened = %d\n",__func__,__LINE__,ps_data->ps_enable);
		cancel_delayed_work_sync(&ps_data->light_report_work);
		/* < DTS2016032805348 liuanqi 20160328 begin */
		als_polling_count=0;
		/* DTS2016032805348 liuanqi 20160328  end > */
		/* < DTS2016031507744  chendong 20160324 begin */
		queue_delayed_work(ps_data->alsps_wq, &ps_data->light_report_work, msecs_to_jiffies(ps_data->light_poll_delay));
		/* DTS2016031507744  chendong 20160324 end > */
	}

	if(ret < 0){
		AP3426_ERR("%s %d:ls enable error!!!!!!\n",__func__,__LINE__);
	}
	mutex_unlock(&ps_data->lock);

	return;
}

void ap3426_als_disable_work(struct work_struct *work)
{
	struct ap3426_data *ps_data = container_of(work, struct ap3426_data, als_disable_work);
	int32_t ret;

	AP3426_ERR("%s %d:\n",__func__,__LINE__);
	mutex_lock(&ps_data->lock);

	if(ps_data->rels_enable==1)
	{
		ps_data->rels_enable=0;
		if(ps_data->ps_enable==1)
		{
			ret = __ap3426_write_reg(ps_data->client,
			AP3426_REG_SYS_CONF, AP3426_MODE_MASK, 0, 2);
		}
		else
		{
			ret = __ap3426_write_reg(ps_data->client,
				AP3426_REG_SYS_CONF, AP3426_MODE_MASK, 0, 0);
		}
		AP3426_ERR("%s %d:misc_ps_opened = %d\n",__func__,__LINE__,ps_data->ps_enable);

	}
	cancel_delayed_work_sync(&ps_data->light_report_work);

	if(ret < 0){
		AP3426_ERR("%s %d:ls disable error!!!!!!\n",__func__,__LINE__);
	}
	mutex_unlock(&ps_data->lock);

	return;
}

/*********************************************************************
light sensor register & unregister
********************************************************************/

static int ap3426_register_lsensor_device(struct i2c_client *client, struct ap3426_data *data)
{
	struct input_dev *input_dev;
	int rc;
	AP3426_ERR("%s %d:allocating input device lsensor\n",__func__,__LINE__);
	input_dev = input_allocate_device();
	if (!input_dev) {
		AP3426_ERR("%s %d: could not allocate input device for lsensor\n",__func__,__LINE__);
		rc = -ENOMEM;
		goto done;
	}
	data->lsensor_input_dev = input_dev;
	input_set_drvdata(input_dev, data);
	input_dev->name = "light";
	input_dev->dev.parent = &client->dev;
	set_bit(EV_ABS, input_dev->evbit);
	input_set_abs_params(input_dev, ABS_MISC, 0, 65535, 0, 0);

	rc = input_register_device(input_dev);
	if (rc < 0) {
		AP3426_ERR("%s %d: could not register input device for lsensor \n",__func__,__LINE__);
		goto done;
	}
done:
	return rc;
}

static void ap3426_unregister_lsensor_device(struct i2c_client *client, struct ap3426_data *data)
{
	input_unregister_device(data->lsensor_input_dev);
}


static void ap3426_unregister_heartbeat_device(struct i2c_client *client, struct ap3426_data *data)
{
	input_unregister_device(data->hsensor_input_dev);
}

/*********************************************************************
proximity sensor register & unregister
********************************************************************/

static int ap3426_register_psensor_device(struct i2c_client *client, struct ap3426_data *data)
{
	struct input_dev *input_dev;
	int rc;
	AP3426_ERR("%s %d: allocating input device psensor \n",__func__,__LINE__);
	input_dev = input_allocate_device();
	if (!input_dev) {
	AP3426_ERR("%s %d: could not allocate input device for psensor \n",__func__,__LINE__);
	rc = -ENOMEM;
	goto done;
	}
	data->psensor_input_dev = input_dev;
	input_set_drvdata(input_dev, data);
	input_dev->name = "proximity";
	input_dev->dev.parent = &client->dev;
	set_bit(EV_ABS, input_dev->evbit);
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, 1, 0, 0);

	rc = input_register_device(input_dev);
	if (rc < 0) {
		AP3426_ERR("%s %d: could not register input device for psensor \n",__func__,__LINE__);
		goto done;
	}


done:
	return rc;
}

static void ap3426_unregister_psensor_device(struct i2c_client *client, struct ap3426_data *data)
{
	input_unregister_device(data->psensor_input_dev);
}


/* range */
static ssize_t ap3426_show_range(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);
	return sprintf(buf, "%i\n", ap3426_get_range(data->client));
}

static ssize_t ap3426_store_range(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);
	unsigned long val;
	int ret;

	if ((strict_strtoul(buf, 10, &val) < 0) || (val > 3))
		return -EINVAL;

	ret = ap3426_set_range(data->client, val);

	return (ret < 0)? ret:count;
}


static int ap3426_als_enable_set(struct sensors_classdev *sensors_cdev,
						unsigned int enabled)
{
	struct ap3426_data *als_data = container_of(sensors_cdev,
						struct ap3426_data, als_cdev);
	if (enabled)
		queue_work(als_data->alsps_wq, &als_data->als_enable_work);
	else
		queue_work(als_data->alsps_wq, &als_data->als_disable_work);
	return 0;
}

static ssize_t ap3426_show_ps_calibration(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);
	struct i2c_client *client=data->client;
	int tmp;
	int i;
	int read_count = 0;
	int sum_value = 0;
	int ps_th_limit = 700;
	msleep(100);
	for(i=0;i<20;i++){
		tmp = ap3426_get_px_value(client);
		if(tmp < 0)
		{
			AP3426_ERR("%s %d:  i2c read ps data fail. \n",__func__,__LINE__);
			return -1;
		}else if(tmp < ps_th_limit){
			sum_value += tmp;
			read_count ++;
		}
		AP3426_ERR("%s %d:  tmp=%d  read_count=%d \n",__func__,__LINE__,tmp,read_count);
		msleep(50);
	}

	if(read_count == 0){
		data->crosstalk =-1;
		return sprintf(buf, "%d\n",  data->crosstalk);
	}else{
		 data->crosstalk = sum_value/read_count;
		 if(data->crosstalk == 0)
		 {
		 	data->crosstalk = 5;
		 }
		AP3426_ERR("%s %d:  data->crosstalk=%d,sum_value=%d,read_count=%d \n",__func__,__LINE__,
			data->crosstalk,sum_value,read_count);
		 tmp=ap3426_set_phthres(client, data->ps_high+data->crosstalk);
		 if(tmp<0)
		 {
			data->ps_thres_high = data->ps_high+data->crosstalk;
			data->ps_thres_low = data->ps_low+data->crosstalk;
			AP3426_ERR("%s %d: ap3426_set_phthres \n",__func__,__LINE__);
			return sprintf(buf, "%d\n", data->crosstalk);
		 }
		 data->ps_thres_high=data->ps_high+data->crosstalk;
		 tmp=ap3426_set_plthres(client, data->ps_low+data->crosstalk);
		 if(tmp<0)
		 {
		 		data->ps_thres_high = data->ps_high+data->crosstalk;
				data->ps_thres_low = data->ps_low+data->crosstalk;
				AP3426_ERR("%s %d: ap3p3426_set_plthres \n",__func__,__LINE__);
				return sprintf(buf, "%d\n", data->crosstalk);

		 }
		 data->ps_thres_low=data->ps_low+data->crosstalk;
	}
	AP3426_ERR("%s %d:data->ps_thres_high=%d data->ps_thres_low=%d \n",__func__,__LINE__,
		data->ps_thres_high,data->ps_thres_low);
	return sprintf(buf, "%d\n", data->crosstalk);

}
/* < DTS2016011406267 chendong 20150118 begin */
static ssize_t ap3426_show_als_cal(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);
	int  lux;
	cancel_delayed_work_sync(&data->light_report_work);
	lux = ap3426_get_adc_value(data->client);
	if((lux>ALS_THRES_LOW )&&(lux< ALS_THRES_HIGH))
	{
		cali = ALS_LIGHTBAR_VALUE * 100 / (lux*ALS_CW_VALUE);
	}
	else
	    cali=100;
	queue_delayed_work(data->alsps_wq, &data->light_report_work, msecs_to_jiffies(data->light_poll_delay));

	AP3426_ERR("%s %d:lux=%d cali=%d \n",__func__,__LINE__,lux,cali);
	return sprintf(buf, "%d\n", cali);
}
static DEVICE_ATTR(ps_cal, 0664, ap3426_show_ps_calibration, NULL);

static DEVICE_ATTR(als_cal, S_IWUSR |S_IRUGO,ap3426_show_als_cal,NULL);
static ssize_t ap3426_store_als_cali(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	unsigned long als_data;

	if (strict_strtoul(buf, 10, &als_data) < 0)
		return -EINVAL;
	cali= als_data;
	AP3426_ERR("%s %d:cali=%d \n",__func__,__LINE__,cali);
	return count;
}

static DEVICE_ATTR(als_calibration, 0664,NULL,ap3426_store_als_cali);
static ssize_t ap3426_store_ps_crosstalk(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);
	struct i2c_client *client=data->client;
	int ret;
	unsigned long prox_data;

	if (strict_strtoul(buf, 10, &prox_data) < 0)
		return -EINVAL;
	data->crosstalk= prox_data;
	AP3426_ERR("%s %d prox_data=%d \n",__func__,__LINE__,data->crosstalk);
	mutex_lock(&data->lock);
	if(data->crosstalk==0)
	{
		data->ps_thres_high = data->ps_high+data->crosstalk;
		data->ps_thres_low = data->ps_low+data->crosstalk;
		mutex_unlock(&data->lock);
		AP3426_ERR("%s %d: ps_thres_high=%d ps_thres_low=%d  ps_high=%d ps_low=%d\n",__func__,__LINE__,
			data->ps_thres_high,data->ps_thres_low,data->ps_high,data->ps_low);
		return count;
	}

	ret=ap3426_set_phthres(client, data->ps_high+data->crosstalk);
	if(ret<0)
	{
		data->ps_thres_high = data->ps_high+data->crosstalk;
		data->ps_thres_low = data->ps_low+data->crosstalk;
		mutex_unlock(&data->lock);
		AP3426_ERR("%s %d: ap3426_set_phthres \n",__func__,__LINE__);
		return count;
	}
	else
	{
		data->ps_thres_high=data->ps_high+data->crosstalk;
	}
	ret=ap3426_set_plthres(client, data->ps_low+data->crosstalk);
	if(ret<0)
	{
		data->ps_thres_high = data->ps_high+data->crosstalk;
		data->ps_thres_low = data->ps_low+data->crosstalk;
		mutex_unlock(&data->lock);
		AP3426_ERR("%s %d: ap3426_set_plthres \n",__func__,__LINE__);
		return count;
	 }
	else
	{
		data->ps_thres_low=data->ps_low+data->crosstalk;
	}
	mutex_unlock(&data->lock);
	AP3426_ERR("%s %d: data->ps_thres_high=%d data->ps_thres_low=%d  data->ps_high= %d data->ps_low=%d \n",
		__func__,__LINE__,data->ps_thres_high,data->ps_thres_low,data->ps_high,data->ps_low);
	return count;
}

static DEVICE_ATTR(ps_crosstalk, 0664,NULL,ap3426_store_ps_crosstalk);
/* DTS2016011406267 chendong 20150118 end > */
static int ap3426_als_poll_delay_set(struct sensors_classdev *sensors_cdev,
					   unsigned int delay_msec)
{
	struct ap3426_data *als_data = container_of(sensors_cdev,
						struct ap3426_data, als_cdev);

	if (delay_msec < ALS_SET_MIN_DELAY_TIME * 1000000)
		delay_msec = ALS_SET_MIN_DELAY_TIME * 1000000;

	als_data->light_poll_delay = delay_msec /1000000;


	/* we need this polling timer routine for sunlight canellation */
	if(als_data->rels_enable == 1){
		/*
		 * If work is already scheduled then subsequent schedules will not
		 * change the scheduled time that's why we have to cancel it first.
		 */
		cancel_delayed_work_sync(&als_data->light_report_work);
		queue_delayed_work(als_data->alsps_wq, &als_data->light_report_work, msecs_to_jiffies(als_data->light_poll_delay));

	}
	return 0;
}

/* < DTS2016011101150 chendong 20150118 begin */
static int ap3426_ps_enable_set(struct sensors_classdev *sensors_cdev,
					   unsigned int enabled)
{
   struct ap3426_data *ps_data = container_of(sensors_cdev,
					   struct ap3426_data, ps_cdev);

	if (enabled)
	{
		queue_work(ps_data->alsps_wq, &ps_data->prox_enable_work);
		power_key_ps = false;
		/* < DTS2016031507744  chendong 20160324 begin */
                schedule_delayed_work(&ps_data->powerkey_work, msecs_to_jiffies(100));
		/* DTS2016031507744  chendong 20160324 end > */
	}
	else
	{
		queue_work(ps_data->alsps_wq, &ps_data->prox_disable_work);
		cancel_delayed_work(&ps_data->powerkey_work);
		
	}

   return 0;
}
/* DTS2016011101150 chendong 20150118 end > */

static int ap3426_ps_flush(struct sensors_classdev *sensors_cdev)
{
	struct ap3426_data *data = container_of(sensors_cdev,
			struct ap3426_data, ps_cdev);

	input_event(data->psensor_input_dev, EV_SYN, SYN_CONFIG,
			data->flush_count++);
	input_sync(data->psensor_input_dev);

	return 0;
}

static int ap3426_als_flush(struct sensors_classdev *sensors_cdev)
{
	struct ap3426_data *data = container_of(sensors_cdev,
			struct ap3426_data, als_cdev);

	input_event(data->lsensor_input_dev, EV_SYN, SYN_CONFIG, data->flush_count++);
	input_sync(data->lsensor_input_dev);

	return 0;
}

//end
static int ap3426_power_ctl(struct ap3426_data *data, bool on)
{
	int ret = 0;

	if (!on && data->power_enabled)
	{
		ret = regulator_disable(data->vdd);
		if (ret)
		{
			AP3426_ERR("%s %d: Regulator vdd disable failed ret=%d \n",__func__,__LINE__,ret);
			return ret;
		}

		ret = regulator_disable(data->vio);
		if (ret)
		{
			AP3426_ERR("%s %d: Regulator vio disable failed ret=%d \n",__func__,__LINE__,ret);
			ret = regulator_enable(data->vdd);
			if (ret)
			{
				AP3426_ERR("%s %d: Regulator vdd enable failed ret=%d \n",__func__,__LINE__,ret);
			}
			return ret;
		}

		data->power_enabled = on;
		AP3426_ERR("%s %d: ap3426_power_ctl on=%d \n",__func__,__LINE__,on);
	}
	else if (on && !data->power_enabled)
	{
		ret = regulator_enable(data->vdd);
		if (ret)
		{
			AP3426_ERR("%s %d: Regulator vdd enable failed ret=%d \n",__func__,__LINE__,ret);
			return ret;
		}

		ret = regulator_enable(data->vio);
		if (ret)
		{
			AP3426_ERR("%s %d: Regulator vio enable failed ret=%d \n",__func__,__LINE__,ret);
			regulator_disable(data->vdd);
			return ret;
		}

		data->power_enabled = on;
		AP3426_ERR("%s %d: enable ap3426 power\n",__func__,__LINE__);
	}
	else
	{
		AP3426_ERR("%s %d: Power on=%d. enabled=%d\n",__func__,__LINE__,on, data->power_enabled);
	}

	return ret;
}

static int ap3426_power_init(struct ap3426_data*data, bool on)
{
	int ret =0;

	if (!on)
	{
		if (regulator_count_voltages(data->vdd) > 0)
			regulator_set_voltage(data->vdd,
					0, AP3426_VDD_MAX_UV);

		regulator_put(data->vdd);

		if (regulator_count_voltages(data->vio) > 0)
			regulator_set_voltage(data->vio,
					0, AP3426_VIO_MAX_UV);

		regulator_put(data->vio);
	}
	else
	{
		data->vdd = regulator_get(&data->client->dev, "vdd");
		if (IS_ERR(data->vdd))
		{
			ret = PTR_ERR(data->vdd);
			AP3426_ERR("%s %d: Regulator get failed vdd ret=%d \n",__func__,__LINE__,ret);
			return ret;
		}

		if (regulator_count_voltages(data->vdd) > 0)
		{
			ret = regulator_set_voltage(data->vdd,
					AP3426_VDD_MIN_UV,
					AP3426_VDD_MAX_UV);
			if (ret)
			{
				AP3426_ERR("%s %d: Regulator set failed vdd ret=%d \n",__func__,__LINE__,ret);
				goto reg_vdd_put;
			}
		}

		data->vio = regulator_get(&data->client->dev, "vio");
		if (IS_ERR(data->vio))
		{
			ret = PTR_ERR(data->vio);
			AP3426_ERR("%s %d: Regulator get failed vio ret=%d \n",__func__,__LINE__,ret);
			goto reg_vdd_set;
		}

		if (regulator_count_voltages(data->vio) > 0)
		{
			ret = regulator_set_voltage(data->vio,
					AP3426_VIO_MIN_UV,
					AP3426_VIO_MAX_UV);
			if (ret)
			{
				AP3426_ERR("%s %d: Regulator set failed vio ret=%d \n",__func__,__LINE__,ret);
				goto reg_vio_put;
			}
		}
	}

	return 0;

reg_vio_put:
	regulator_put(data->vio);
reg_vdd_set:
	if (regulator_count_voltages(data->vdd) > 0)
		regulator_set_voltage(data->vdd, 0, AP3426_VDD_MAX_UV);
reg_vdd_put:
	regulator_put(data->vdd);
	return ret;
}


static DEVICE_ATTR(range, S_IWUSR | S_IRUGO,
	ap3426_show_range, ap3426_store_range);


/* mode */
static ssize_t ap3426_show_mode(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);
	return sprintf(buf, "%d\n", ap3426_get_mode(data->client));
}

static ssize_t ap3426_store_mode(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);
	unsigned long val;
	int ret;

	if ((strict_strtoul(buf, 10, &val) < 0) || (val > 7))
	return -EINVAL;

	ret = ap3426_set_mode(data->client, val);

	if (ret < 0)
	return ret;
	AP3426_DEBUG("%s %d: Starting timer to fire in 200ms (%ld)\n",__func__,__LINE__,jiffies);
	ret = mod_timer(&data->pl_timer, jiffies + msecs_to_jiffies(PL_TIMER_DELAY));

	if(ret)
	AP3426_DEBUG("%s %d: Timer Error\n",__func__,__LINE__);
	return count;
}
static DEVICE_ATTR(mode,0664,
	ap3426_show_mode, ap3426_store_mode);
/* lux */
static ssize_t ap3426_show_lux(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);

	 /* No LUX data if power down */
	if (ap3426_get_mode(data->client) == AP3426_SYS_DEV_DOWN)
		return sprintf((char*) buf, "%s\n", "Please power up first!");

	return sprintf(buf, "%d\n", ap3426_get_adc_value(data->client));
}

static DEVICE_ATTR(lux, S_IRUGO, ap3426_show_lux, NULL);


/* Px data */
static ssize_t ap3426_show_pxvalue(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);

	/* No Px data if power down */
	if (ap3426_get_mode(data->client) == AP3426_SYS_DEV_DOWN)
	return -EBUSY;

	return sprintf(buf, "%d\n", ap3426_get_px_value(data->client));
}

static DEVICE_ATTR(pxvalue, S_IRUGO, ap3426_show_pxvalue, NULL);


/* proximity object detect */
static ssize_t ap3426_show_object(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);
	return sprintf(buf, "%d\n", ap3426_get_object(data->client));
}

static DEVICE_ATTR(object, S_IRUGO, ap3426_show_object, NULL);


/* ALS low threshold */
static ssize_t ap3426_show_althres(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);
	return sprintf(buf, "%d\n", ap3426_get_althres(data->client));
}

static ssize_t ap3426_store_althres(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);
	unsigned long val;
	int ret;

	if (strict_strtoul(buf, 10, &val) < 0)
	return -EINVAL;

	ret = ap3426_set_althres(data->client, val);
	if (ret < 0)
	return ret;

	return count;
}

static DEVICE_ATTR(althres, S_IWUSR | S_IRUGO,
	ap3426_show_althres, ap3426_store_althres);


/* ALS high threshold */
static ssize_t ap3426_show_ahthres(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);
	return sprintf(buf, "%d\n", ap3426_get_ahthres(data->client));
}

static ssize_t ap3426_store_ahthres(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);
	unsigned long val;
	int ret;

	if (strict_strtoul(buf, 10, &val) < 0)
	return -EINVAL;

	ret = ap3426_set_ahthres(data->client, val);
	if (ret < 0)
	return ret;

	return count;
}

static DEVICE_ATTR(ahthres, S_IWUSR | S_IRUGO,
	ap3426_show_ahthres, ap3426_store_ahthres);

/* Px low threshold */
static ssize_t ap3426_show_plthres(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);
	return sprintf(buf, "%d\n", ap3426_get_plthres(data->client));
}

static ssize_t ap3426_store_plthres(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);
	unsigned long val;
	int ret;

	if (strict_strtoul(buf, 10, &val) < 0)
	return -EINVAL;

	ret = ap3426_set_plthres(data->client, val);
	if (ret < 0)
	return ret;

	return count;
}

static DEVICE_ATTR(plthres, S_IWUSR | S_IRUGO,
	ap3426_show_plthres, ap3426_store_plthres);

/* Px high threshold */
static ssize_t ap3426_show_phthres(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);
	return sprintf(buf, "%d\n", ap3426_get_phthres(data->client));
}

static ssize_t ap3426_store_phthres(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);
	unsigned long val;
	int ret;

	if (strict_strtoul(buf, 10, &val) < 0)
	return -EINVAL;

	ret = ap3426_set_phthres(data->client, val);
	if (ret < 0)
	return ret;

	return count;
}

static DEVICE_ATTR(phthres, S_IWUSR | S_IRUGO,
	ap3426_show_phthres, ap3426_store_phthres);


/* calibration */
static ssize_t ap3426_show_calibration_state(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	return sprintf(buf, "%d\n", cali);
}

static ssize_t ap3426_store_calibration_state(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct ap3426_data *data = input_get_drvdata(input);
	int stdls, lux;
	char tmp[10];
	AP3426_DEBUG("%s %d: \n",__func__,__LINE__);

	/* No LUX data if not operational */
	if (ap3426_get_mode(data->client) == AP3426_SYS_DEV_DOWN)
	{
		printk("Please power up first!");
		AP3426_ERR("%s %d: Please power up first!\n",__func__,__LINE__);

		return -EINVAL;
	}

	cali = 100;
	sscanf(buf, "%d %s", &stdls, tmp);

	if (!strncmp(tmp, "-setcv", 6))
	{
		cali = stdls;
		return -EBUSY;
	}

	if (stdls < 0)
	{
		AP3426_ERR("%s %d: Std light source: [%d] < 0 !!!\nCheck again, please.\n\
			Set calibration factor to 100.\n",__func__,__LINE__,stdls);

		return -EBUSY;
	}

	lux = ap3426_get_adc_value(data->client);
	cali = stdls * 100 / lux;

	return -EBUSY;
}

static DEVICE_ATTR(calibration, S_IWUSR | S_IRUGO,
	ap3426_show_calibration_state, ap3426_store_calibration_state);

#ifdef LSC_DBG
/* engineer mode */
static ssize_t ap3426_em_read(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ap3426_data *data = i2c_get_clientdata(client);
	int i;
	u8 tmp;

	AP3426_DEBUG("%s %d: \n",__func__,__LINE__);

	for (i = 0; i < AP3426_NUM_CACHABLE_REGS; i++)
	{
		tmp = i2c_smbus_read_byte_data(data->client, reg_array[i]);
		AP3426_ERR("%s %d: Reg[0x%x] Val[0x%x]\n",__func__,__LINE__, reg_array[i], tmp);

	}

	return 0;
}

static ssize_t ap3426_em_write(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ap3426_data *data = i2c_get_clientdata(client);
	u32 addr,val;
	int ret = 0;

	AP3426_ERR("%s %d: \n",__func__,__LINE__);

	sscanf(buf, "%x%x", &addr, &val);

	AP3426_ERR("%s %d:Write [%x] to Reg[%x]... \n",__func__,__LINE__,val,addr);

	ret = i2c_smbus_write_byte_data(data->client, addr, val);


	if (!ret)
	    data->reg_cache[ap3426_reg_to_idx_array[addr]] = val;

	return count;
}
static DEVICE_ATTR(em, S_IWUSR |S_IRUGO,
	ap3426_em_read, ap3426_em_write);
#endif

static struct attribute *ap3426_attributes[] = {
	&dev_attr_range.attr,
	&dev_attr_mode.attr,
	&dev_attr_lux.attr,
	&dev_attr_object.attr,
	&dev_attr_pxvalue.attr,
	&dev_attr_althres.attr,
	&dev_attr_ahthres.attr,
	&dev_attr_plthres.attr,
	&dev_attr_phthres.attr,
	&dev_attr_calibration.attr,
	&dev_attr_ps_cal.attr,
	/* < DTS2016011406267 chendong 20150118 begin */
	&dev_attr_als_cal.attr,
	&dev_attr_als_calibration.attr,
	&dev_attr_ps_crosstalk.attr,
	/* DTS2016011406267 chendong 20150118 end > */
#ifdef LSC_DBG
	&dev_attr_em.attr,
#endif
	NULL
};

static const struct attribute_group ap3426_attr_group = {
	.attrs = ap3426_attributes,
};

static int ap3426_init_client(struct i2c_client *client)
{
	struct ap3426_data *data = i2c_get_clientdata(client);
	int i;
	int ret;
 	AP3426_ERR("%s %d:\n",__func__,__LINE__);

	/*lsensor high low thread*/
	i2c_smbus_write_byte_data(client, AP3426_REG_ALS_THDL_L, 0);
	i2c_smbus_write_byte_data(client, AP3426_REG_ALS_THDL_H, 0);
	i2c_smbus_write_byte_data(client, AP3426_REG_ALS_THDH_L, 0xFF);
	i2c_smbus_write_byte_data(client, AP3426_REG_ALS_THDH_H, 0xFF);
	/*psensor high low thread*/
	i2c_smbus_write_byte_data(client, AP3426_REG_PS_THDL_L, 0x20);
	i2c_smbus_write_byte_data(client, AP3426_REG_PS_THDL_H, 0x03);
	i2c_smbus_write_byte_data(client, AP3426_REG_PS_THDH_L, 0x60);
	i2c_smbus_write_byte_data(client, AP3426_REG_PS_THDH_H, 0x03);

	i2c_smbus_write_byte_data(client, AP3426_REG_SYS_INTCTRL, 0x80);
	i2c_smbus_write_byte_data(client, AP3426_REG_ALS_CONF, 0x10);
	i2c_smbus_write_byte_data(client, AP3426_REG_PS_CONF, 0x0);
	i2c_smbus_write_byte_data(client, AP3426_REG_PS_LEDD, 0x02);
	i2c_smbus_write_byte_data(client, AP3426_REG_PS_MEAN, 0x01);
	/* < DTS2016011406267 chendong 20150118 begin */
	/* < DTS2016021900917  chendong 20160224 begin */
	i2c_smbus_write_byte_data(client, AP3426_REG_PS_INTEGR, 0x07);
	/* DTS2016021900917  chendong 20160224 end > */
	/* DTS2016011406267 chendong 20150118 end > */
	i2c_smbus_write_byte_data(client, AP3426_REG_PS_PERSIS, 0x02);

	data->ps_thres_high = 1023;
	data->ps_thres_low = 0;
	ret=ap3426_set_phthres(client, data->ps_thres_high);
	if(ret<0)
	{
		AP3426_ERR("%s %d:err ap3426_set_phthres\n",__func__,__LINE__);
	}

	 ret=ap3426_set_plthres(client, data->ps_thres_low);
	 if(ret<0)
	 {
		AP3426_ERR("%s %d:err ap3426_set_plthres\n",__func__,__LINE__);

	 }
	i2c_smbus_write_byte_data(client, AP3426_REG_PS_IFORM, 0);
	/* read all the registers once to fill the cache.
	 * if one of the reads fails, we consider the init failed */
	for (i = 0; i < AP3426_NUM_CACHABLE_REGS; i++) {
		int v = i2c_smbus_read_byte_data(client, reg_array[i]);
		if (v < 0)
		    return -ENODEV;
		data->reg_cache[i] = v;
	}

	ap3426_set_mode(client, AP3426_SYS_DEV_DOWN);

	return 0;
}

static int ap3426_check_id(struct ap3426_data *data)
{
	return 0;
}

/* < DTS2016011406267 chendong 20150118 begin */
static void lsensor_work_handler(struct work_struct *w)
{

	struct ap3426_data *data =
		container_of(w, struct ap3426_data, light_report_work.work);
       int value,value_ir;
	ktime_t timestamp;
	timestamp = ktime_get_boottime();
	queue_delayed_work(data->alsps_wq, &data->light_report_work, msecs_to_jiffies(data->light_poll_delay));
	mutex_lock(&data->lock);
	/* < DTS2016022509409  chendong 20160225 begin */
	value_ir =ap3426_get_ir_value(data->client);
	msleep(215);
	value = ap3426_get_adc_value(data->client);
	mutex_unlock(&data->lock);
	if(pre_als_value >= ALS_LOW_VALUE)
	{
		if(value_ir <DIST_DIFF_LIGHT)
		{
			value = value * ALS_CW_VALUE*cali/100*ALS_CW_ANGLE;
		}
		else
		{
			value = value * ALS_AD_VALUE;
		}

	}
	else
	{
		if(value_ir <DIST_DIFF_LIGHT)
		{
			value = value * ALS_CW_VALUE*cali/100*ALS_CW_ANGLE/ALS_RANGE_16;
		}
		else
		{
			value = value * ALS_AD_VALUE/ALS_RANGE_16;
		}
	}
	/* < DTS2016032805348 liuanqi 20160328 begin */
	if( als_polling_count < 5 )
	{
		if(value == AP3426_LUX_MAX)
		{
			value = value - als_polling_count%2;
		}
		else
		{
			value = value + als_polling_count%2;
		}
		als_polling_count++;
	}
	/* DTS2016032805348 liuanqi 20160328  end > */
	pre_als_value = value ;
	/* DTS2016022509409  chendong 20160225 end > */
        /* < DTS2016031405308  chendong 20160315 begin */
        input_report_abs(data->lsensor_input_dev, ABS_MISC, value);
        input_sync(data->lsensor_input_dev);
        /* DTS2016031405308  chendong 20160315 end > */

}
/* DTS2016011406267 chendong 20150118 end > */

static void ap3426_work_handler(struct work_struct *w)
{

	struct ap3426_data *data = container_of(w, struct ap3426_data, prox_report_work);
	u8 int_stat;
	int pxvalue;
	int distance;
	ktime_t timestamp;
	timestamp = ktime_get_boottime();
	msleep(10);
	mutex_lock(&data->lock);
	int_stat = ap3426_get_intstat(data->client);

	if(int_stat & AP3426_REG_SYS_INT_PMASK)

	{
		pxvalue = ap3426_get_px_value(data->client);
		if(pxvalue < 0)
		{
			AP3426_ERR("%s %d:i2c read ps data fail. \n",__func__,__LINE__);
			mutex_unlock(&data->lock);
			return;
		}

		if(pxvalue==1023)
		{
			pxvalue = 800;
		}

		if(pxvalue > data->ps_thres_high)
		{
			ap3426_set_phthres(data->client,1023);
			ap3426_set_plthres(data->client,data->ps_thres_low);

			distance = 0;
                        /* < DTS2016031405308  chendong 20160315 begin */
			input_report_abs(data->psensor_input_dev, ABS_DISTANCE, distance);
			input_sync(data->psensor_input_dev);
                        /* DTS2016031405308  chendong 20160315 end > */
			AP3426_ERR("%s %d:distance=%d pxvalue=%d  data->ps_thres_low=%d \n",
				__func__,__LINE__,distance,pxvalue,data->ps_thres_low);
		}
		else if(pxvalue < data->ps_thres_low)
		{
			ap3426_set_phthres(data->client,data->ps_thres_high);
			ap3426_set_plthres(data->client,0);

			distance = 1;
                        /* < DTS2016031405308  chendong 20160315 begin */
			input_report_abs(data->psensor_input_dev, ABS_DISTANCE, distance);
			input_sync(data->psensor_input_dev);
                        /* DTS2016031405308  chendong 20160315 end > */
			AP3426_ERR("%s %d:distance=%d pxvalue=%d  data->ps_thres_high=%d \n",
				__func__,__LINE__,distance,pxvalue,data->ps_thres_high);
		}
		else
		{
			distance = 1;
                        /* < DTS2016031405308  chendong 20160315 begin */
			input_report_abs(data->psensor_input_dev, ABS_DISTANCE, distance);
			input_sync(data->psensor_input_dev);
                        /* DTS2016031405308  chendong 20160315 end > */
			AP3426_ERR("%s %d:distance=%d pxvalue=%d \n",__func__,__LINE__,distance,pxvalue);
		}
		/* < DTS2016011406267 chendong 20150118 begin */
		//delete line
		/* DTS2016011406267 chendong 20150118 end > */
	}

	enable_irq(data->client->irq);
	enable_irq_wake(data->client->irq);
	mutex_unlock(&data->lock);

}


static irqreturn_t ap3426_irq(int irq, void *data_)
{
	struct ap3426_data *data = data_;
	AP3426_DEBUG("%s %d:  irq=%d ,  pin=%d \n",__func__,__LINE__,
		data->client->irq,gpio_to_irq(data->int_pin));
	disable_irq_nosync(data->client->irq);
	disable_irq_wake(data->client->irq);
	wake_lock_timeout(&data->prx_wake_lock, 5*HZ);
	queue_work(data->alsps_wq, &data->prox_report_work);
	return IRQ_HANDLED;
}
static int sensor_platform_hw_power_on(bool on)
{
	int err;

	if (pdev_data == NULL)
		return -ENODEV;

	if (!IS_ERR_OR_NULL(pdev_data->pinctrl)) {
		if (on)
			err = pinctrl_select_state(pdev_data->pinctrl,
				pdev_data->pin_default);
		else
			err = pinctrl_select_state(pdev_data->pinctrl,
				pdev_data->pin_sleep);
		if (err)
			AP3426_ERR("%s %d:  Can't select pinctrl state \n",__func__,__LINE__);
	}

	ap3426_power_ctl(pdev_data, on);


	return 0;
}
#ifdef CONFIG_OF
static int ap3426_parse_dt(struct device *dev, struct ap3426_data *pdata)
{
	struct device_node *dt = dev->of_node;
	unsigned int tmp;
	int rc = 0;

	if (pdata == NULL)
	{
		AP3426_ERR("%s %d:   pdata is NULL \n",__func__,__LINE__);

		return -EINVAL;
	}
	pdata->power_on = sensor_platform_hw_power_on;
	pdata->int_pin = of_get_named_gpio_flags(dt, "ap3426,irq-gpio",
	                            0, &pdata->irq_flags);

	if (pdata->int_pin < 0)
	{
		AP3426_ERR("%s %d: Unable to read irq-gpio \n",__func__,__LINE__);

		return pdata->int_pin;
	}
	rc = of_property_read_u32(dt, "ap3426,ps_high", &tmp);
	if (rc) {
		dev_err(dev, "Unable to read ps high threshold\n");
		return rc;
	}
	pdata->ps_high = tmp;

	rc = of_property_read_u32(dt, "ap3426,ps_low", &tmp);
	if (rc) {
		dev_err(dev, "Unable to read ps high threshold\n");
		return rc;
	}
	pdata->ps_low = tmp;

	return 0;
}
#endif

static int ap3426_pinctrl_init(struct ap3426_data *data)
{
	struct i2c_client *client = data->client;

	data->pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR_OR_NULL(data->pinctrl)) {
		AP3426_ERR("%s %d: Failed to get pinctrl \n",__func__,__LINE__);
		return PTR_ERR(data->pinctrl);
	}

	data->pin_default =
		pinctrl_lookup_state(data->pinctrl, "default");
	if (IS_ERR_OR_NULL(data->pin_default)) {
		AP3426_ERR("%s %d: Failed to look up default state\n",__func__,__LINE__);
		return PTR_ERR(data->pin_default);
	}
	data->pin_sleep =
		pinctrl_lookup_state(data->pinctrl, "sleep");
	if (IS_ERR_OR_NULL(data->pin_sleep)) {
		AP3426_ERR("%s %d: Failed to look up sleep state\n",__func__,__LINE__);
		return PTR_ERR(data->pin_sleep);
	}
	return 0;
}

static struct of_device_id ap3426_match_table[];
static int ap3426_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct ap3426_data *data;
	int err = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
	{
		err = -EIO;
		goto exit_free_gpio;
	}

	data = kzalloc(sizeof(struct ap3426_data), GFP_KERNEL);
	if (!data)
	{
		err = -ENOMEM;
		goto exit_free_gpio;
	}
	pdev_data = data;

#ifdef CONFIG_OF
	if (client->dev.of_node)
	{
		AP3426_ERR("%s %d:Device Tree parsing.\n",__func__,__LINE__);
		err = ap3426_parse_dt(&client->dev, data);
		if (err)
		{
			AP3426_ERR("%s %d:ap3426_parse_dt "
			    "for pdata failed. err = %d\n",__func__,__LINE__,err);

			goto exit_parse_dt_fail;
		}
	}

#else
	data->irq = client->irq;
#endif
	data->client = client;
	i2c_set_clientdata(client, data);
	mutex_init(&data->lock);
	wake_lock_init(&data->prx_wake_lock, WAKE_LOCK_SUSPEND, "prx_wake_lock");
	err = ap3426_power_init(data, true);
	if (err)
		goto err_power_on;

	err = ap3426_power_ctl(data, true);
	if (err)
	{
		err = ap3426_power_init(data, false);
		goto exit_kfree;
	}

	/* initialize the AP3426 chip */
	err = ap3426_init_client(client);

	if (err)
	goto exit_kfree;
	if(ap3426_check_id(data) !=0 )
	{
		AP3426_ERR("%s %d:failed to check ap3426 id \n",__func__,__LINE__);

		goto err_power_on;
	}
	err = ap3426_pinctrl_init(data);
	if (err) {
		AP3426_ERR("%s %d:Can't initialize pinctrl \n",__func__,__LINE__);
		goto exit_kfree;
	}

	err = pinctrl_select_state(data->pinctrl, data->pin_default);
	if (err) {
		AP3426_ERR("%s %d:Can't select pinctrl default state\n",__func__,__LINE__);
		goto exit_kfree;
	}
	err = ap3426_register_lsensor_device(client,data);
	if (err)
	{
		AP3426_ERR("%s %d:failed to register_lsensor_device\n",__func__,__LINE__);
		goto exit_kfree;
	}

	err = ap3426_register_psensor_device(client, data);
	if (err)
	{
		AP3426_ERR("%s %d:failed to register_psensor_device\n",__func__,__LINE__);
		goto exit_free_ls_device;
	}

	/* register sysfs hooks */
	if (err)
	goto exit_free_ps_device;

	err = gpio_request(data->int_pin,"ap3426-int");
	if(err < 0)
	{
		AP3426_ERR("%s %d:gpio_request, err=%d\n",__func__,__LINE__,err);
		return err;
	}
	err = gpio_direction_input(data->int_pin);
	if(err < 0)
	{
		AP3426_ERR("%s %d:gpio_direction_input, err=%d\n",__func__,__LINE__,err);
		return err;
	}
	data->client->irq = gpio_to_irq(data->int_pin);

	if (data->power_on)
		err = data->power_on(true);
	err = request_irq(data->client->irq,ap3426_irq, IRQF_TRIGGER_FALLING| IRQF_ONESHOT,
			 "ap3426",(void *)data );
	if (err)
	{
		AP3426_ERR("%s %d:ret: %d, could not get IRQ %d\n",__func__,__LINE__,err,gpio_to_irq(data->int_pin));
		goto exit_free_ps_device;
	}

	disable_irq(data->client->irq);

	data->light_poll_delay=ALS_SET_DEFAULT_DELAY_TIME;
	INIT_WORK(&data->prox_report_work, ap3426_work_handler);
	INIT_DELAYED_WORK(&data->light_report_work, lsensor_work_handler);
	INIT_WORK(&data->prox_enable_work, ap3426_ps_enable_work);
	/* < DTS2016011101150 chendong 20150118 begin */
	INIT_DELAYED_WORK(&data->powerkey_work, ap3426_powerkey_screen_handler);
	/* DTS2016011101150 chendong 20150118 end > */
	INIT_WORK(&data->prox_disable_work, ap3426_ps_disable_work);
	INIT_WORK(&data->als_enable_work, ap3426_als_enable_work);
	INIT_WORK(&data->als_disable_work, ap3426_als_disable_work);

	data->alsps_wq = create_singlethread_workqueue("alsps_wq");

	err = sysfs_create_group(&data->client->dev.kobj, &ap3426_attr_group);

	if (err)
		goto exit_kfree;

	data->als_cdev = sensors_light_cdev;
	data->als_cdev.sensors_enable = ap3426_als_enable_set;
	data->als_cdev.sensors_poll_delay = ap3426_als_poll_delay_set;
	data->als_cdev.sensors_flush = ap3426_als_flush;
	err = sensors_classdev_register(&data->lsensor_input_dev->dev, &data->als_cdev);
	if (err)
		goto exit_pwoer_ctl;

	data->ps_cdev = sensors_proximity_cdev;
	data->ps_cdev.sensors_enable = ap3426_ps_enable_set;
	data->ps_cdev.sensors_flush = ap3426_ps_flush;
	err = sensors_classdev_register(&data->psensor_input_dev->dev, &data->ps_cdev);
	if(err)
		goto err_power_on;

	/* < DTS2015120303965  chendong 20151126 begin */
	err = app_info_set("P-Sensor", "AP3426");
        err += app_info_set("L-Sensor", "AP3426");
        if (err < 0)/*failed to add app_info*/
        {
        	AP3426_ERR("%s %d:failed to add app_info\n", __func__, __LINE__);
        }
	/* DTS2015120303965  chendong 20151126 end > */

#ifdef CONFIG_GET_HARDWARE_INFO
	register_hardware_info(ALS_PS, ap3426_match_table[0].compatible);
#endif
	err = ap3426_power_ctl(data, true);
	if (err)
		goto err_power_on;
	AP3426_ERR("%s %d:Driver version %s enabled\n",__func__,__LINE__,DRIVER_VERSION);
 	return 0;

exit_free_ps_device:
	ap3426_unregister_psensor_device(client,data);

exit_free_ls_device:
	ap3426_unregister_lsensor_device(client,data);
exit_pwoer_ctl:
ap3426_power_ctl(data, false);
	sensors_classdev_unregister(&data->ps_cdev);

err_power_on:
	ap3426_power_init(data, false);
	sensors_classdev_unregister(&data->als_cdev);

exit_kfree:
	wake_lock_destroy(&data->prx_wake_lock);
	mutex_destroy(&data->lock);
	if (data->power_on)
	data->power_on(false);
	pdev_data = NULL;
	kfree(data);
#ifdef CONFIG_OF
exit_parse_dt_fail:
	AP3426_ERR("%s %d:dts initialize failed.\n",__func__,__LINE__);
 	return err;

#endif
exit_free_gpio:

	return err;
}

static int ap3426_remove(struct i2c_client *client)
{
	struct ap3426_data *data = i2c_get_clientdata(client);
	free_irq(gpio_to_irq(data->int_pin), data);

	ap3426_power_ctl(data, false);
	wake_lock_destroy(&data->prx_wake_lock);

	sysfs_remove_group(&data->client->dev.kobj, &ap3426_attr_group);
	ap3426_unregister_psensor_device(client,data);
	ap3426_unregister_lsensor_device(client,data);
	ap3426_unregister_heartbeat_device(client,data);

	ap3426_power_init(data, false);


	ap3426_set_mode(client, 0);
	kfree(i2c_get_clientdata(client));


	if(&data->pl_timer)
	del_timer(&data->pl_timer);
	pdev_data = NULL;
	if (data->power_on)
		data->power_on(false);
	return 0;
}

static const struct i2c_device_id ap3426_id[] =
{
	{ AP3426_DRV_NAME, 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, ap3426_id);

#ifdef CONFIG_OF
/* < CQ_hw00003915 yanfei 20151121 begin */
static struct of_device_id ap3426_match_table[] =
{
	{.compatible = "di_ap34xx" },
	{},
};
/* CQ_hw00003915 yanfei 20151121 end > */
#else
#define ap3426_match_table NULL
#endif

static int ap3426_suspend(struct device *dev)
{
	struct ap3426_data *ps_data = dev_get_drvdata(dev);
/* < DTS2016022205149 liuanqi 20160301 begin */
	if(ps_data->client->irq){
                struct irq_desc *desc=irq_to_desc(ps_data->client->irq);
                if(desc->core_internal_state__do_not_mess_with_it & IRQ_PENDING){
                enable_irq(ps_data->client->irq);	/* enable irq if irq is pending */
                AP3426_ERR("%s %d:irq is pending,enable irq!\n",__func__,__LINE__);
                }
        }
/* DTS2016022205149 liuanqi 20160301 end > */
	if (ps_data->rels_enable==1)
	{
		cancel_delayed_work_sync(&ps_data->light_report_work);
	}
	return 0;
}

static int ap3426_resume(struct device *dev)
{
	struct ap3426_data *ps_data = dev_get_drvdata(dev);
	if(ps_data ->rels_enable == 1)
	{
		/* < DTS2016031507744  chendong 20160324 begin */
		queue_delayed_work(ps_data->alsps_wq, &ps_data->light_report_work, msecs_to_jiffies(ps_data->light_poll_delay));
		/* DTS2016031507744  chendong 20160324 end > */
	}
	return 0;
}

static const struct dev_pm_ops ap3426_pm_ops = {
	.suspend	= ap3426_suspend,
	.resume 	= ap3426_resume,
};
static struct i2c_driver ap3426_driver = {
    .driver = {
	.name	= AP3426_DRV_NAME,
	.owner	= THIS_MODULE,
	.of_match_table = ap3426_match_table,
	.pm     = &ap3426_pm_ops,
    },
    .probe	= ap3426_probe,
    .remove	= ap3426_remove,
    .id_table = ap3426_id,
};

static int __init ap3426_init(void)
{
    int ret;

    ret = i2c_add_driver(&ap3426_driver);
    return ret;

}

static void __exit ap3426_exit(void)
{
    i2c_del_driver(&ap3426_driver);
}

module_init(ap3426_init);
module_exit(ap3426_exit);
MODULE_AUTHOR("Kevin.dang, <kevin.dang@dyna-image.com>");
MODULE_DESCRIPTION("AP3426 driver.");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);
/* DTS2015112600367  chendong 20151126 end > */
