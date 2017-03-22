/* drivers/input/touchscreen/nt11206/NVTtouch_206_dsm.c
 *
 * Copyright (C) 2010 - 2016 Novatek, Inc.
 *
 * $Revision: 3448 $
 * $Date: 2016-02-18 11:28:34 +0800 () $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */
#include "NVTtouch_206.h"
#include <dsm/dsm_pub.h>
#include <linux/gpio.h>

#ifdef CONFIG_HUAWEI_DSM
extern struct tp_dsm_info nvt_tp_dsm_info;
extern struct dsm_client *nvt_tp_dclient;

ssize_t nvt_dsm_record_basic_err_info(struct device *dev)
{
	ssize_t size = 0;
	ssize_t total_size = 0;

	/* record irq and reset gpio status */
	tp_log_debug("%s: record irq and reset gpio!\n", __func__);
	size = dsm_client_record(nvt_tp_dclient,
				"[irq gpio]   num:%d, irq gpio status:%d, [reset gpio] num:%d, reset gpio status:%d\n",
				nvt_tp_dsm_info.irq_gpio, gpio_get_value(nvt_tp_dsm_info.irq_gpio),
				nvt_tp_dsm_info.rst_gpio, gpio_get_value(nvt_tp_dsm_info.rst_gpio));
	total_size += size;

	tp_log_err("[irq gpio]   num:%d, irq gpio status:%d, [reset gpio] num:%d, reset gpio status:%d\n",
				nvt_tp_dsm_info.irq_gpio, gpio_get_value(nvt_tp_dsm_info.irq_gpio),
				nvt_tp_dsm_info.rst_gpio, gpio_get_value(nvt_tp_dsm_info.rst_gpio));

	return total_size;
}

/* report error infomation */
ssize_t nvt_dsm_record_common_err_info(char * err_info)
{

	ssize_t size = 0;
	ssize_t total_size = 0;

	tp_log_err("%s: entry!\n", __func__);

	/* err number */
	size = dsm_client_record(nvt_tp_dclient, "%s\n", err_info);
	total_size += size;

	return total_size;
}

/* i2c error infomation: err number, register infomation */
ssize_t nvt_dsm_record_i2c_err_info( int err_numb )
{

	ssize_t size = 0;
	ssize_t total_size = 0;

	tp_log_err("%s: entry!\n", __func__);

	/* err number */
	size = dsm_client_record(nvt_tp_dclient, "i2c err number:%d\n", err_numb );
	total_size += size;

	return total_size;
}
/* fw err infomation: err number */
ssize_t nvt_dsm_record_fw_err_info( int err_numb )
{

	ssize_t size = 0;
	ssize_t total_size = 0;

	tp_log_err("%s: entry!\n", __func__);

	/* err number */
	size = dsm_client_record(nvt_tp_dclient, "fw update result:failed, retval is %d\n", err_numb);
	total_size += size;

	/* fw err status */
	size = dsm_client_record(nvt_tp_dclient, "updata status is %d\n", nvt_tp_dsm_info.constraints_UPDATE_status);
	total_size += size;

	return total_size;
}

ssize_t nvt_dsm_record_esd_err_info( int err_numb )
{

	ssize_t size = 0;
	ssize_t total_size = 0;

	tp_log_info("%s: entry!\n", __func__);

	/* err number */
	size = dsm_client_record(nvt_tp_dclient, "esd err number:%d\n", err_numb );
	total_size += size;

	return total_size;
}

/* tp report err according to err type */
int nvt_tp_report_dsm_err(struct device *dev, int type, int err_numb)
{
	tp_log_err("%s: entry! type:%d\n", __func__, type);

	if( NULL == nvt_tp_dclient )
	{
		tp_log_err("%s: there is not tp_dclient!\n", __func__);
		return -1;
	}

	/* try to get permission to use the buffer */
	if(dsm_client_ocuppy(nvt_tp_dclient))
	{
		/* buffer is busy */
		tp_log_err("%s: buffer is busy!\n", __func__);
		return -1;
	}

	/* tp report err according to err type */
	switch(type)
	{
		case DSM_TP_I2C_RW_ERROR_NO:
			/* report tp basic infomation */
			nvt_dsm_record_basic_err_info(dev);
			/* report i2c infomation */
			nvt_dsm_record_i2c_err_info(err_numb);
			break;
		case DSM_TP_FW_ERROR_NO:
			/* report tp basic infomation */
			nvt_dsm_record_basic_err_info(dev);
			/* report fw infomation */
			nvt_dsm_record_fw_err_info(err_numb);
			break;
		case DSM_TP_CYTTSP5_WAKEUP_ERROR_NO:
			/* report tp basic infomation */
			nvt_dsm_record_basic_err_info(dev);
			/* report error infomation */
			nvt_dsm_record_common_err_info("wakeup error");
			break;
		case DSM_TP_ESD_ERROR_NO:
			/* report tp basic infomation */
			nvt_dsm_record_basic_err_info(dev);
			/* report esd infomation */
			nvt_dsm_record_esd_err_info(err_numb);
			break;
		default:
			break;
	}
	dsm_client_notify(nvt_tp_dclient, type);

	return 0;
}
#endif/*CONFIG_HUAWEI_DSM*/

	
	
