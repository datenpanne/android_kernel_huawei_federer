/* Himax Android Driver Sample Code Ver for Himax chipset
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

#include "himax_platform.h"
#include "himax_852xF.h"



int irq_enable_count = 0;

DEFINE_MUTEX(tp_wr_access);


int i2c_himax_read(struct i2c_client *client, uint8_t command, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	int retry;
	int ret = -1;
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &command,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
			.buf = data,
			.buf = data,
			.buf = data,
		}
	};

	for (retry = 0; retry < toRetry; retry++) {
		ret = i2c_transfer(client->adapter, msg, 2);
		if (ret == 2)
			break;
		mdelay(5);
	}
	if (retry == toRetry) {
		tp_log_err("%s: i2c_read_block error %d, retry over %d\n",
			__func__, ret, toRetry);
		#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_report_dsm_err(DSM_TP_I2C_RW_ERROR_NO , ret);
		#endif
		return -EIO;
	}
	return 0;

}

int i2c_himax_write(struct i2c_client *client, uint8_t command, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	int retry/*, loop_i*/;
	uint8_t buf[length + 1];
	int ret = -1;

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = length + 1,
			.buf = buf,
		}
	};

	buf[0] = command;
	memcpy(buf+1, data, length);

	for (retry = 0; retry < toRetry; retry++) {
		ret = i2c_transfer(client->adapter, msg, 1);
		if (ret == 1)
			break;
		mdelay(5);
	}

	if (retry == toRetry) {
		tp_log_err("%s: i2c_read_block error %d, retry over %d\n",
			__func__, ret, toRetry);
		#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_report_dsm_err(DSM_TP_I2C_RW_ERROR_NO , ret);
		#endif
		return -EIO;
	}
	return 0;

}

int i2c_himax_read_command(struct i2c_client *client, uint8_t length, uint8_t *data, uint8_t *readlength, uint8_t toRetry)
{
	int retry;
	int ret = -1;
	struct i2c_msg msg[] = {
		{
		.addr = client->addr,
		.flags = I2C_M_RD,
		.len = length,
		.buf = data,
		}
	};

	for (retry = 0; retry < toRetry; retry++) {
		ret = i2c_transfer(client->adapter, msg, 1);
		if ( ret == 1)
			break;
		mdelay(5);
	}
	if (retry == toRetry) {
		tp_log_err("%s: i2c_read_block error %d, retry over %d\n",
			__func__, ret, toRetry);
		#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_report_dsm_err(DSM_TP_I2C_RW_ERROR_NO , ret);
		#endif
		return -EIO;
	}
	return 0;
}

int i2c_himax_write_command(struct i2c_client *client, uint8_t command, uint8_t toRetry)
{
	return i2c_himax_write(client, command, NULL, 0, toRetry);
}

int i2c_himax_master_write(struct i2c_client *client, uint8_t *data, uint8_t length, uint8_t toRetry)
{
	int retry/*, loop_i*/;
	uint8_t buf[length];
	int ret = -1;

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = length,
			.buf = buf,
		}
	};

	memcpy(buf, data, length);

	for (retry = 0; retry < toRetry; retry++) {
		ret = i2c_transfer(client->adapter, msg, 1);
		if ( ret == 1 )
			break;
		mdelay(5);
	}

	if (retry == toRetry) {
		tp_log_err("%s: i2c_read_block error %d, retry over %d\n",
			__func__, ret, toRetry);
		#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_report_dsm_err(DSM_TP_I2C_RW_ERROR_NO , ret);
		#endif
		return -EIO;
	}
	return 0;
}

void himax_int_enable(int irqnum, int enable)
{
	tp_log_info("irq_enable_count = %d, enable =%d\n", irq_enable_count, enable);

	if (enable == 1 && irq_enable_count == 0) {
		enable_irq(irqnum);
		irq_enable_count=1;
	} else if (enable == 0 && irq_enable_count == 1) {
		disable_irq_nosync(irqnum);
		irq_enable_count=0;
	}
	tp_log_info("irq_enable_count = %d, enable =%d\n", irq_enable_count, enable);
}

void himax_rst_gpio_set(int pinnum, uint8_t value)
{
	gpio_direction_output(pinnum, value);
}

uint8_t himax_int_gpio_read(int pinnum)
{
	return gpio_get_value(pinnum);
}


int himax_gpio_power_config(struct i2c_client *client,struct himax_i2c_platform_data *pdata)
{
	int error=0;
	tp_log_info("%s:enter\n", __func__);

	if (pdata->gpio_reset > 0) {
		error = gpio_request(pdata->gpio_reset, "himax-reset");
		if (error < 0){
			tp_log_err("%s: request reset pin failed\n", __func__);
			return error;
		}
	}
	if (pdata->gpio_3v3_en > 0) {
		error = gpio_request(pdata->gpio_3v3_en, "himax-3v3_en");
		if (error < 0) {
			tp_log_err("%s: request 3v3_en pin failed\n", __func__);
			goto err_req_3v3;
		}
	}
	if (pdata->gpio_1v8_en > 0) {
		error = gpio_request(pdata->gpio_1v8_en, "himax-1V8_en");
		if (error < 0) {
			tp_log_err("%s: request gpio_1v8_en pin failed\n", __func__);
			goto err_req_1v8;
		}
	}
	if (gpio_is_valid(pdata->gpio_irq)) {
		/* configure touchscreen irq gpio */
		error = gpio_request(pdata->gpio_irq, "himax_gpio_irq");
		if (error) {
			tp_log_err("unable to request gpio [%d]\n",pdata->gpio_irq);
			goto err_req_irq;
		}
		error = gpio_direction_input(pdata->gpio_irq);
		if (error) {
			tp_log_err("unable to set direction for gpio [%d]\n",pdata->gpio_irq);
			goto err_req_irq;
		}
		client->irq = gpio_to_irq(pdata->gpio_irq);
		goto exit;
	} else {
		tp_log_err("irq gpio not provided\n");
	}
err_req_irq:
	gpio_free(pdata->gpio_1v8_en);		
err_req_1v8:
	gpio_free(pdata->gpio_3v3_en);	
err_req_3v3:
	gpio_free(pdata->gpio_reset);	
exit:
	tp_log_info("%s:exit\n", __func__);
	return error;
}

int himax_gpio_power_on(struct i2c_client *client,struct himax_i2c_platform_data *pdata)
{
	int error=0;
	tp_log_info("%s:enter\n", __func__);

	if (pdata->gpio_reset >= 0) {
		error = gpio_direction_output(pdata->gpio_reset, 0);
		if (error) {
			tp_log_err("unable to set direction for gpio [%d]\n",
				pdata->gpio_reset);
			return error;
		}
	}
	if (pdata->gpio_3v3_en >= 0) {
		error = gpio_direction_output(pdata->gpio_3v3_en, 1);
		if (error) {
			tp_log_err("unable to set direction for gpio [%d]\n",
				pdata->gpio_reset);
			return error;
		}
		tp_log_info("3v3_en pin =%d\n", gpio_get_value(pdata->gpio_3v3_en));
	}
	usleep(80);
	if (pdata->gpio_1v8_en >= 0) {
		error = gpio_direction_output(pdata->gpio_1v8_en, 1);
		if (error) {
			tp_log_err("unable to set direction for gpio [%d]\n",
				pdata->gpio_reset);
			return error;
		}
		tp_log_info("gpio_1v8_en pin =%d\n", gpio_get_value(pdata->gpio_1v8_en));
	}

	msleep(25);
	if (pdata->gpio_reset >= 0) {
		error = gpio_direction_output(pdata->gpio_reset, 1);
		if (error) {
			tp_log_err("unable to set direction for gpio [%d]\n",
				pdata->gpio_reset);
			return error;
		}
	}
	msleep(20);
	tp_log_info("%s:exit\n", __func__);
	return error;
}

void himax_gpio_power_deconfig(struct himax_i2c_platform_data *pdata)
{
 	tp_log_info("%s:enter\n", __func__);

	if (pdata->gpio_reset >= 0) {
		gpio_free(pdata->gpio_reset);
	}
	if (pdata->gpio_3v3_en >= 0) {
		gpio_free(pdata->gpio_3v3_en);
	}
	if (pdata->gpio_1v8_en >= 0) {
		gpio_free(pdata->gpio_1v8_en);
	}
	if (gpio_is_valid(pdata->gpio_irq)) {
		gpio_free(pdata->gpio_irq);
	}
	tp_log_info("%s:exit\n", __func__);
}

void himax_gpio_power_off(struct himax_i2c_platform_data *pdata)
{
	int error=0;
	tp_log_info("%s:enter\n", __func__);

	if (pdata->gpio_3v3_en >= 0) {
		error = gpio_direction_output(pdata->gpio_3v3_en, 0);
		if (error) {
			tp_log_err("unable to set direction for gpio [%d]\n",
				pdata->gpio_3v3_en);
			return;
		}
	}
	if (pdata->gpio_1v8_en >= 0) {
		error = gpio_direction_output(pdata->gpio_1v8_en, 0);
		if (error) {
			tp_log_err("unable to set direction for gpio [%d]\n",
				pdata->gpio_1v8_en);
			return;
		}
	}

	tp_log_info("%s:exit\n", __func__);
}
