/* Himax Android Driver Sample Code for HMX852xF chipset
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

#include "himax_852xF.h"

//define for factory test
#define HIMAX_PROC_FACTORY_TEST_FILE	"touchscreen/tp_capacitance_data"

#define RESULT_LEN  100

#define IIR_DIAG_COMMAND   1
#define DC_DIAG_COMMAND        2
#define BANK_DIAG_COMMAND   3
#define BASEC_GOLDENBC_DIAG_COMMAND  5
#define CLOSE_DIAG_COMMAND  0

#define IIR_CMD   1
#define DC_CMD        2
#define BANK_CMD   3
#define BASEC_CMD  7
#define GOLDEN_BASEC_CMD  5

static char buf_test_result[RESULT_LEN] = { 0 };	/*store mmi test result*/

atomic_t hmx_mmi_test_status = ATOMIC_INIT(0);
#define RAW_DATA_SIZE   (PAGE_SIZE * 60)

uint8_t DC_golden_data[1024] = {
//DC_mutual data
150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,149,150,150,150,
150,150,150,150,150,150,150,150,150,150,150,150,150,149,150,150,149,150,150,149,150,150,150,150,150,149,150,150,150,150,150,150,150,150,150,150,150,149,
150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
149,150,150,150,150,150,150,150,150,150,150,150,150,150,150,149,150,150,150,150,150,150,150,150,150,150,150,150,150,150,149,150,150,150,150,150,150,150,
149,150,150,149,150,149,150,150,149,149,150,149,150,150,150,150,150,150,149,150,150,150,150,150,150,150,149,149,150,150,149,150,150,150,150,149,150,150,
150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
149,150,150,149,150,150,149,150,150,149,150,150,149,150,150,149,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
149,150,150,150,150,149,150,150,150,150,150,150,150,149,150,150,150,150,150,150,150,150,150,150,150,150,149,150,150,150,150,150,150,150,150,150,150,150,
150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
150,150,150,150,150,150,150,150,150,149,150,150,150,150,150,149,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
150,149,150,150,149,150,150,149,150,150,149,150,150,149,150,150,149,150,150,149,150,150,149,149,150,150,150,150,149,149,150,149,150,150,150,150,150,150,
150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
149,150,150,149,150,150,149,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,149,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
150,150,150,149,150,150,150,150,150,149,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,149,150,150,149,150,150,150,150,150,150,150,
150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,149,150,150,149,150,150,150,150,150,149,149,150,149,150,150,150,150,150,150,149,150,150,
150,150,150,150,150,150,150,150,150,150,150,149,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,149,150,150,150,150,150,150,150,150,150,150,150,150,150,150,150,
//DC RX data
81,86,79,91,89,92,85,86,82,74,77,73,80,79,87,89,89,89,89,92,79,87,83,79,92,85,85,81,78,76,78,87,86,92,90,92,81,90, 
//DC TX data
80,82,80,85,84,86,82,84,82,84,85,82,83,82,81,80,81,82,82,86,81,81,82,78,
};
uint8_t Bank_golden_data[1024] = {
//bank mutual data
89,78,78,78,77,76,77,76,75,76,75,74,75,74,73,75,73,73,74,73,73,75,73,73,74,73,72,74,73,73,75,73,74,76,75,75,79,82,
76,71,72,72,72,72,71,71,71,71,71,71,70,70,70,70,69,70,69,70,70,70,70,70,70,70,70,70,70,70,70,70,71,71,72,72,73,75,
74,71,71,71,71,71,70,70,70,70,70,70,70,69,69,69,68,69,69,69,69,69,69,69,69,69,69,69,69,69,69,69,70,70,71,71,72,72,
72,70,71,71,71,71,70,70,70,70,70,69,69,69,69,69,68,69,68,69,69,69,69,69,69,69,68,68,69,69,69,69,70,70,70,70,71,71,
72,70,71,70,71,71,70,70,70,70,69,69,69,69,69,68,68,68,68,68,69,69,69,69,68,69,68,68,69,69,69,69,69,70,70,70,71,70,
72,70,71,71,71,70,70,70,70,70,69,69,69,69,68,68,68,68,68,68,69,69,69,69,68,68,68,68,68,69,69,69,69,69,69,70,71,70,
71,70,71,70,71,70,70,69,70,70,69,69,69,69,68,68,68,68,68,68,68,69,69,69,68,68,68,68,68,68,68,69,69,69,70,70,70,70,
71,70,71,70,70,70,70,69,70,70,69,69,69,69,68,68,67,68,68,68,68,69,68,68,68,68,68,68,68,68,69,68,69,69,70,70,70,71,
71,70,71,70,70,70,69,69,70,70,69,69,69,69,68,68,67,68,68,68,68,68,68,68,68,68,68,68,68,68,68,68,69,69,69,69,70,71,
70,70,71,70,70,70,69,69,69,69,69,69,68,68,68,68,67,68,67,68,68,68,68,68,67,68,68,68,68,68,68,68,69,69,69,69,70,71,
70,70,70,70,70,70,69,69,69,70,69,68,68,68,68,68,67,68,67,68,68,68,68,68,67,68,67,67,68,68,68,68,69,69,69,69,70,71,
70,70,70,70,70,70,69,69,69,69,69,68,68,68,68,68,67,68,67,67,68,68,68,68,67,68,67,67,68,68,68,68,69,69,69,69,70,72,
69,70,70,70,70,70,69,69,69,69,68,68,68,68,68,68,67,67,67,67,68,68,68,68,67,68,68,67,68,68,68,68,69,69,69,69,70,72,
69,69,70,70,70,70,69,69,69,69,68,68,68,68,67,67,67,67,67,67,68,68,68,68,67,67,67,67,68,68,68,68,69,69,69,69,70,72,
69,69,70,70,70,70,69,69,69,69,68,68,68,68,68,68,67,67,67,67,68,68,68,68,67,68,67,67,68,68,68,68,69,69,69,69,70,72,
69,70,70,70,70,70,69,69,69,69,69,68,68,68,68,68,67,67,67,68,68,68,68,68,67,68,67,67,68,68,68,68,68,69,69,69,70,72,
68,69,70,70,70,70,69,69,69,69,68,68,68,68,68,67,67,67,67,67,68,68,68,68,67,68,67,67,68,68,68,68,69,69,69,69,70,73,
68,69,70,70,70,70,69,69,69,69,68,68,68,68,68,68,67,67,67,68,68,68,68,68,67,68,67,68,68,68,68,68,69,69,69,69,70,73,
68,70,70,70,70,70,69,69,69,69,69,68,68,68,68,68,67,68,67,68,68,68,68,68,67,68,67,67,68,68,68,68,69,69,69,69,70,73,
68,69,70,70,70,70,69,69,69,69,69,68,68,68,68,68,67,68,67,68,68,68,68,69,68,68,68,68,68,68,68,68,69,70,69,70,70,74,
68,70,70,70,70,70,69,69,69,70,69,69,68,69,68,68,67,68,68,68,68,68,68,69,68,68,68,68,68,68,69,69,69,70,70,70,71,74,
68,70,70,70,70,70,69,69,69,69,69,69,68,69,68,68,68,68,68,68,68,69,69,69,68,68,68,68,69,69,69,69,69,70,70,70,71,75,
67,70,71,70,71,70,70,70,70,70,69,69,69,69,69,69,68,69,68,69,69,69,69,69,69,69,69,69,69,69,69,69,70,71,70,70,72,76,
67,71,72,71,72,72,71,71,71,71,71,70,70,70,70,70,69,70,69,70,70,70,70,71,70,71,70,71,70,70,70,71,72,72,72,72,74,86,
//bank RX data
82,88,91,89,89,85,84,84,84,84,83,83,82,83,82,81,81,81,80,80,80,78,79,79,77,76,76,76,75,75,75,74,74,73,72,72,72,68,
//bank TX data
134,138,138,135,134,134,133,133,133,133,133,133,133,133,133,132,133,139,139,140,136,140,140,135,
};
uint8_t test_criteria_data[12] = {225,25,225,25,220,25,220,25,60,25, 60, 40};
//{self_bank_up_limit,self_bank_down_limit,mutual_bank_up_limit,mutual_bank_down_limit,self_DC_up_limit,self_DC_down_limit,mutual_DC_up_limit,mutual_DC_down_limit,self_baseC_dev,mutual_baseC_dev,self_IIR_up_limit,multual_IIR_up_limit}

static uint8_t *mutual_bank= NULL;
static uint8_t *self_bank= NULL;
static uint8_t *mutual_dc= NULL;
static uint8_t *self_dc= NULL;

static uint8_t *mutual_basec= NULL;
static uint8_t *self_basec= NULL;

static uint8_t *mutual_golden_basec= NULL;
static uint8_t *self_golden_basec= NULL;

static uint8_t *mutual_iir= NULL;
static uint8_t *self_iir= NULL;

void himax_cat_RawData(int mode, struct seq_file *s)
{
	uint32_t loop_i;
	uint16_t mutual_num, self_num, width;
	uint16_t x_channel = getXChannel();
	uint16_t y_channel = getYChannel();
	uint8_t *mutual_data = NULL;
	uint8_t *self_data = NULL;

	mutual_num	= x_channel * y_channel;
	self_num	= x_channel + y_channel; //don't add KEY_COUNT
	width	= x_channel;

	if(mode == 0x01){
		seq_printf(s, "IIRStart: %4d, %4d\n\n", x_channel, y_channel);
		tp_log_info("IIRStart: \n\n");
		mutual_data = mutual_iir;
		self_data      = self_iir;
	}
	else if(mode == 0x02){
		seq_printf(s, "DCStart: %4d, %4d\n\n", x_channel, y_channel);
		tp_log_info("DCStart: \n\n");
		mutual_data = mutual_dc;
		self_data      = self_dc;
	} else if( mode == 0x03 ){
		seq_printf(s, "BankStart: %4d, %4d\n\n", x_channel, y_channel);
		tp_log_info("BankStart: \n\n");
		mutual_data = mutual_bank;
		self_data      = self_bank;
	} else if( mode == 0x05 ){
		seq_printf(s, "GoldenBaseCStart: %4d, %4d\n\n", x_channel, y_channel);
		tp_log_info("GoldenBaseCStart: \n\n");
		mutual_data = mutual_golden_basec;
		self_data      = self_golden_basec;
	} else if( mode == 0x07 ){
		seq_printf(s, "BaseCStart: %4d, %4d\n\n", x_channel, y_channel);
		tp_log_info("BaseCStart: \n\n");
		mutual_data = mutual_basec;
		self_data      = self_basec;
	} else {
		return;
	}

	//print in consle
	for (loop_i = 0; loop_i < mutual_num; loop_i++) {
		seq_printf(s, "%4d", mutual_data[loop_i]);
		if ((loop_i % width) == (width - 1))
			seq_printf(s, " %3d\n", self_data[width + loop_i/width]);
	}
	seq_printf(s, "\n");
	for (loop_i = 0; loop_i < width; loop_i++) {
		seq_printf(s, "%4d", self_data[loop_i]);
		if (((loop_i) % width) == (width - 1))
			seq_printf(s, "\n");
	}

	//print in log
	for (loop_i = 0; loop_i < mutual_num; loop_i++) {
		printk("%5d", mutual_data[loop_i]);
		if ((loop_i % width) == (width - 1))
			printk("%6d\n", self_data[width + loop_i/width]);
	}
	printk("\n");
	for (loop_i = 0; loop_i < width; loop_i++) {
		printk("%5d", self_data[loop_i]);
		if (((loop_i) % width) == (width - 1))
			printk("\n");
	}

	if(mode == 0x01){
		seq_printf(s, "IIREnd\n");
		tp_log_info("IIREnd:\n");
	} else if(mode == 0x02){
		seq_printf(s, "DCEnd\n");
		tp_log_info("DCEnd:\n");
	} else if( mode == 0x03 ){
		seq_printf(s, "BankEnd\n");
		tp_log_info("BankEnd:\n");
	} else if( mode == 0x05 ){
		seq_printf(s, "GoldenBaseCEnd\n");
		tp_log_info("GoldenBaseCEnd:\n");
	} else if( mode == 0x07 ){
		seq_printf(s, "BaseCEnd\n");
		tp_log_info("BaseCEnd:\n");
	}
}

static void himax_RawDataCopy(int mode)
{
	uint16_t mutual_num;
	uint16_t self_num;
	uint16_t x_channel = getXChannel();
	uint16_t y_channel = getYChannel();

	uint8_t *mutual_data = getMutualBuffer();
	uint8_t *self_data = getSelfBuffer();
	
	mutual_num	= x_channel * y_channel;
	self_num	= x_channel + y_channel;
	if(mode == 0x01)//IIR
	{
		memcpy(mutual_iir, mutual_data, mutual_num);
		memcpy(self_iir, self_data, self_num);
	} else if(mode == 0x02)//DC
	{
		memcpy(mutual_dc, mutual_data, mutual_num);
		memcpy(self_dc, self_data, self_num);
	} else if(mode == 0x03)//Bank
	{
		memcpy(mutual_bank, mutual_data, mutual_num);
		memcpy(self_bank, self_data, self_num);
	}else if(mode == 0x05)//golden Basec
	{
		memcpy(mutual_golden_basec, mutual_data, mutual_num);
		memcpy(self_golden_basec, self_data, self_num);
	}else if(mode == 0x07)//Basec
	{
		memcpy(mutual_basec, mutual_data, mutual_num);
		memcpy(self_basec, self_data, self_num);
	}
}

static int himax_alloc_Rawmem(void)
{
	uint16_t mutual_num;
	uint16_t self_num;
	uint16_t x_channel = getXChannel();
	uint16_t y_channel = getYChannel();

	mutual_num	= x_channel * y_channel;
	self_num	= x_channel + y_channel;

	mutual_bank = kzalloc(mutual_num * sizeof(uint8_t), GFP_KERNEL);
	if (mutual_bank == NULL) {
		tp_log_err("%s:mutual_bank is NULL\n", __func__);
		goto exit_mutual_bank;
	}

	self_bank = kzalloc(self_num * sizeof(uint8_t), GFP_KERNEL);
	if (self_bank == NULL) {
		tp_log_err("%s:self_bank is NULL\n", __func__);
		goto exit_self_bank;
	}

	mutual_dc = kzalloc(mutual_num * sizeof(uint8_t), GFP_KERNEL);
	if (mutual_dc == NULL) {
		tp_log_err("%s:mutual_dc is NULL\n", __func__);
		goto exit_mutual_dc;
	}
	
	self_dc = kzalloc(self_num * sizeof(uint8_t), GFP_KERNEL);
	if (self_dc == NULL) {
		tp_log_err("%s:self_dc is NULL\n", __func__);
		goto exit_self_dc;
	}

	mutual_basec = kzalloc(mutual_num * sizeof(uint8_t), GFP_KERNEL);
	if (mutual_basec == NULL) {
		tp_log_err("%s:mutual_basec is NULL\n", __func__);
		goto exit_mutual_basec;
	}
	self_basec = kzalloc(self_num * sizeof(uint8_t), GFP_KERNEL);
	if (self_basec == NULL) {
		tp_log_err("%s:self_basec is NULL\n", __func__);
		goto exit_self_basec;
	}

	mutual_golden_basec = kzalloc(mutual_num * sizeof(uint8_t), GFP_KERNEL);
	if (mutual_golden_basec == NULL) {
		tp_log_err("%s:mutual_golden_basec is NULL\n", __func__);
		goto exit_mutual_golden_basec;
	}
	self_golden_basec = kzalloc(self_num * sizeof(uint8_t), GFP_KERNEL);
	if (self_golden_basec == NULL) {
		tp_log_err("%s:self_golden_basec is NULL\n", __func__);
		goto exit_self_golden_basec;
	}

	mutual_iir = kzalloc(mutual_num * sizeof(uint8_t), GFP_KERNEL);
	if (mutual_iir == NULL) {
		tp_log_err("%s:mutual_iir is NULL\n", __func__);
		goto exit_mutual_iir;
	}
	self_iir = kzalloc(self_num * sizeof(uint8_t), GFP_KERNEL);
	if (self_iir == NULL) {
		tp_log_err("%s:self_iir is NULL\n", __func__);
		goto exit_self_iir;
	}

	memset(mutual_bank, 0xFF, mutual_num * sizeof(uint8_t));
	memset(self_bank, 0xFF, self_num * sizeof(uint8_t));
	memset(mutual_dc, 0xFF, mutual_num * sizeof(uint8_t));
	memset(self_dc, 0xFF, self_num * sizeof(uint8_t));

	memset(mutual_basec, 0xFF, mutual_num * sizeof(uint8_t));
	memset(self_basec, 0xFF, self_num * sizeof(uint8_t));
	memset(mutual_golden_basec, 0xFF, mutual_num * sizeof(uint8_t));
	memset(self_golden_basec, 0xFF, self_num * sizeof(uint8_t));

	memset(mutual_iir, 0xFF, mutual_num * sizeof(uint8_t));
	memset(self_iir, 0xFF, self_num * sizeof(uint8_t));

	return 0;
exit_self_iir:
	kfree(mutual_iir);
	mutual_iir = NULL;
exit_mutual_iir:
	kfree(self_golden_basec);
	self_golden_basec = NULL;
exit_self_golden_basec:	
	kfree(mutual_golden_basec);
	mutual_golden_basec = NULL;
exit_mutual_golden_basec:
	kfree(self_basec);
	self_basec = NULL;
exit_self_basec:
	kfree(mutual_basec);
	mutual_basec = NULL;
exit_mutual_basec:
	kfree(self_dc);
	self_dc = NULL;
exit_self_dc:
	kfree(mutual_dc);
	mutual_dc = NULL;
exit_mutual_dc:
	kfree(self_bank);
	self_bank = NULL;
exit_self_bank:
	kfree(mutual_bank);
	mutual_bank = NULL;
exit_mutual_bank:
	return -1;
}

static void himax_free_Rawmem(void)
{
	kfree(mutual_bank);
	kfree(self_bank);
	kfree(mutual_dc);
	kfree(self_dc);
	kfree(mutual_basec);
	kfree(self_basec);
	kfree(mutual_golden_basec);
	kfree(self_golden_basec);
	kfree(mutual_iir);
	kfree(self_iir);

	mutual_bank  = NULL;
	self_bank        = NULL;
	mutual_dc       = NULL;
	self_dc             = NULL;
	mutual_basec = NULL;
	self_basec       = NULL;
	mutual_golden_basec = NULL;
	self_golden_basec       = NULL;
	mutual_iir = NULL;
	self_iir       = NULL;
}


/*data write format:
8C 11   ----open to write data
8B data_address
40 data
8C 00  ----close to write data
*/
int himax_wirte_golden_data(void)
{
	int ret = -1;
	uint8_t cmdbuf[4];
	int addr_start = 0x0160;//golden simple store start address
	int num_data = 0;
	int remain_data_num = 0;
	uint8_t write_times = 0;
	int i=0, j=0;
	uint16_t x_channel = getXChannel();
	uint16_t y_channel = getYChannel();

	num_data = x_channel*y_channel + x_channel + y_channel;
	tp_log_info("Number of data = %d\n",num_data);
	if (num_data%128)
		write_times = (num_data/128) + 1;
	else
		write_times = num_data/128;

	tp_log_info("Wirte Golden data - Start \n");

	//Wirte Golden data - Start
	cmdbuf[0] = 0x11;
	ret = i2c_himax_write(private_ts->client, 0x8C, &cmdbuf[0], 1, DEFAULT_RETRY_CNT);
	if( ret != 0 )
	{
		tp_log_err("himax_wirte_golden_data write 0x8C error!\n");
		return ret;
	}
	msleep(10);

	for (j = 0; j < 2; j++){
		remain_data_num = 0;
		for (i = 0; i < write_times; i++) {
			cmdbuf[0] = ((addr_start & 0xFF00) >> 8);
			cmdbuf[1] = addr_start & 0x00FF;
			remain_data_num = num_data - i*128;
		
			ret = i2c_himax_write(private_ts->client, 0x8B, &cmdbuf[0], 2, DEFAULT_RETRY_CNT);
			if( ret != 0 ){
				tp_log_err("himax_wirte_golden_data write 0x8B error!\n");
				return ret;
			}

			msleep(10);
			if(j == 0){
				if(remain_data_num >= 128){
					ret = i2c_himax_write(private_ts->client, 0x40, &DC_golden_data[i*128], 128, DEFAULT_RETRY_CNT);
					if( ret != 0 ){
						tp_log_err("himax_wirte_golden_data write 0x40 error!\n");
						return ret;
					}
					addr_start = addr_start + 128;
				} else {
					ret = i2c_himax_write(private_ts->client, 0x40, &DC_golden_data[i*128], remain_data_num, DEFAULT_RETRY_CNT);
					if( ret != 0 ){
						tp_log_err("himax_wirte_golden_data write 0x40 error!\n");
						return ret;
					}
					addr_start = addr_start + remain_data_num;
				}
				msleep(10);
			}else if(j == 1){
				if(remain_data_num >= 128){
					ret = i2c_himax_write(private_ts->client, 0x40, &Bank_golden_data[i*128], 128, DEFAULT_RETRY_CNT);
					if( ret != 0 ){
						tp_log_err("himax_wirte_golden_data write 0x40 > 128 error!\n");
						return ret;
					}
					addr_start = addr_start + 128;
				} else {
					ret = i2c_himax_write(private_ts->client, 0x40, &Bank_golden_data[i*128], remain_data_num, DEFAULT_RETRY_CNT);
					if( ret != 0 ){
						tp_log_err("himax_wirte_golden_data write 0x40 < 128 error!\n");
						return ret;
					}
					addr_start = addr_start + remain_data_num;
				}	
				msleep(10);
			}
		}
	}

	cmdbuf[0] = 0x00;
	ret = i2c_himax_write(private_ts->client, 0x8C, &cmdbuf[0], 1, DEFAULT_RETRY_CNT);
	if( ret != 0 ){
		tp_log_err("himax_wirte_golden_data write 0x8C error!\n");
		return ret;
	}
	//Write Golden data - End

	return ret;
}


/*data write format:
8C 11   ----open to write data
8B data_address
40 data
8C 00  ----close to write data
*/
int himax_wirte_criteria_data(void)
{
	int ret = -1;
	uint8_t cmdbuf[4];
	int addr_criteria = 0x98;//setup criteria address
	int i =0;

	for(i=0;i<12;i++) {
		tp_log_info("[Himax]: self test test_criteria_data is  data[%d] = %d\n", i, test_criteria_data[i]);
	}

	//Write Criteria
	cmdbuf[0] = 0x11;
	ret = i2c_himax_write(private_ts->client, 0x8C, &cmdbuf[0], 1, DEFAULT_RETRY_CNT);
	if( ret != 0 ){
		tp_log_err("himax_wirte_criteria_data write 0x8C error!\n");
		return ret;
	}
	msleep(10);

	cmdbuf[0] = 0x00;
	cmdbuf[1] = addr_criteria;
	ret = i2c_himax_write(private_ts->client, 0x8B, &cmdbuf[0], 2, DEFAULT_RETRY_CNT);
	if( ret != 0 ){
		tp_log_err("himax_wirte_criteria_data write 0x8B error!\n");
		return ret;
	}

	msleep(10);
	ret = i2c_himax_write(private_ts->client, 0x40, &test_criteria_data[0], 12, DEFAULT_RETRY_CNT);
	if( ret != 0 ){
		tp_log_err("himax_wirte_criteria_data write 0x40 error!\n");
		return ret;
	}

	cmdbuf[0] = 0x00;
	ret = i2c_himax_write(private_ts->client, 0x8C, &cmdbuf[0], 1, DEFAULT_RETRY_CNT);
	if( ret != 0 ){
		tp_log_err("himax_wirte_criteria_data write 0x8C error!\n");
		return ret;
	}
	return ret;
}


/*data read format:
8C 11   ----open to write data
8B data_address
5a data
8C 00  ----close to write data
*/
int himax_read_result_data(void)
{
	int ret = -1;
	uint8_t cmdbuf[4];
	uint8_t databuf[10];
	int addr_result = 0x96;//get result address 
	int i=0;

	cmdbuf[0] = 0x11;
	ret = i2c_himax_write(private_ts->client, 0x8C, &cmdbuf[0], 1, DEFAULT_RETRY_CNT);//read result
	if( ret != 0 ){
		tp_log_err("himax_read_result_data write 0x8C error!\n");
		return ret;
	}
	msleep(10);

	cmdbuf[0] = 0x00;
	cmdbuf[1] = addr_result;
	
	ret = i2c_himax_write(private_ts->client, 0x8B ,&cmdbuf[0], 2, DEFAULT_RETRY_CNT);
	if( ret != 0 ){
		tp_log_err("himax_read_result_data write 0x8B error!\n");
		return ret;
	}
	msleep(10);

	ret = i2c_himax_read(private_ts->client, 0x5A, databuf, 9, DEFAULT_RETRY_CNT);
	if( ret != 0 ){
		tp_log_err("himax_read_result_data write 0x5A error!\n");
		return ret;
	}

	cmdbuf[0] = 0x00;
	ret = i2c_himax_write(private_ts->client, 0x8C ,&cmdbuf[0], 1, DEFAULT_RETRY_CNT);
	if( ret != 0 ){
		tp_log_err("himax_read_result_data write 0x8C error!\n");
		return ret;
	}

	for(i=0;i<9;i++) {
		tp_log_info("[Himax]: After self test %X databuf[%d] = 0x%x\n", addr_result, i, databuf[i]);
	}

	if (databuf[0]==0xAA) {
		tp_log_info("[Himax]: self-test pass\n");
		strncat(buf_test_result, "1P-2P-3P-4P", strlen("1P-2P-3P-4P")+1 );
		return 0;
	} else if ((databuf[0]&0xF)==0) {
		tp_log_info("[Himax]: self-test error\n");
		strncat(buf_test_result, "1F-2F-3F-4F", strlen("1F-2F-3F-4F")+1 );
		#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_report_dsm_err(DSM_TP_BROKEN_ERROR_NO , databuf[0]);
		#endif
		return 1;
	} else {
		#ifdef CONFIG_HUAWEI_DSM
			hmx_tp_report_dsm_err(DSM_TP_BROKEN_ERROR_NO , databuf[0]);
		#endif
		if( (databuf[0] & 1) == 1 )//bit 0: bank
		{
			tp_log_err("[Himax]: bank fail\n");
			strncat(buf_test_result, "1F-", strlen("1F-")+1 );
		} else {
			strncat(buf_test_result, "1P-", strlen("1P-")+1 );
		}

		if( (databuf[0] & 2) == 2 )//bit 1: dc
		{
			tp_log_err("[Himax]: dc fail\n");
			strncat(buf_test_result, "2F-", strlen("2F-")+1 );
		} else {
			strncat(buf_test_result, "2P-", strlen("2P-")+1 );
		}

		if( (databuf[0] & 4) == 4 )//bit 2: Basec
		{
			tp_log_err("[Himax]: Basec fail\n");
			strncat(buf_test_result, "3F-", strlen("3F-")+1 );
		} else {
			strncat(buf_test_result, "3P-", strlen("3P-")+1 );
		}

		if( (databuf[0] & 8) == 8 )//bit 3: IIR
		{
			tp_log_err("[Himax]: IIR fail\n");
			strncat(buf_test_result, "4F", strlen("4F")+1 );
		} else {
			strncat(buf_test_result, "4P", strlen("4P")+1 );
		}

		tp_log_err("[Himax]: self-test fail, %s\n", buf_test_result);
		return 1;
	}

}


int himax_factory_self_test(struct seq_file *m, void *v)
{
	int ret = -1;

	uint8_t cmdbuf[4];

	uint8_t command_F1h[2] = {0xF1, 0x00};
	


	tp_log_info("%s, proc buffer size:%u\n", __func__, (unsigned int)m->size);
	if(m->size <= RAW_DATA_SIZE/4) {
		m->count = m->size;
		return 0;
	}

	if(private_ts->suspended)
	{
		tp_log_err("%s: Already suspended. Can't do factory test. \n", __func__);
		return 0;
	}

	//don't suspend
//	HX_FACTEST_FLAG = 1;
	//self_test_inter_flag= 1;
	if(atomic_read(&hmx_mmi_test_status)){
		tp_log_err("%s factory test already has been called.\n",__func__);
		return -1;
	}
	atomic_set(&hmx_mmi_test_status, 1);
	memset(buf_test_result, 0, RESULT_LEN);
	memcpy(buf_test_result, "result: ", strlen("result: ")+1);

	//add wakelock
	wake_lock(&private_ts->ts_flash_wake_lock);

	tp_log_info("himax_gold_self_test enter \n");

	ret = himax_alloc_Rawmem();
	if( ret != 0 )
	{
		strncat(buf_test_result, "0F-\n", strlen("0F-\n")+1);
		seq_printf(m, "%s\n", buf_test_result);
		goto err_alloc;
	}

	himax_int_enable(private_ts->client->irq,0);
	himax_HW_reset(true,true);
	ret = i2c_himax_write(private_ts->client, 0x82,&cmdbuf[0], 0, DEFAULT_RETRY_CNT);//sense off--analog- close
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		strncat(buf_test_result, "0F-\n", strlen("0F-\n")+1 );
		seq_printf(m, "%s\n", buf_test_result);
		goto err_i2c;
	} else {
		strncat(buf_test_result, "0P-", strlen("0P-") );
	}
	msleep(120);

	ret = i2c_himax_write(private_ts->client, 0x80,&cmdbuf[0], 0, DEFAULT_RETRY_CNT);//sleep in-- digital - close
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(120);

	//Write golden data
	ret = himax_wirte_golden_data();
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	
	//Disable flash reload
	cmdbuf[0] = 0x00;
	ret = i2c_himax_write(private_ts->client, 0x9B, &cmdbuf[0], 1, DEFAULT_RETRY_CNT);
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}

	//Write Criteria
	ret = himax_wirte_criteria_data();
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}

	//Selftest - Start
	tp_log_info("Get basec - Start \n");

	//BashC
	cmdbuf[0] = 0xC0;
	ret = i2c_himax_write(private_ts->client, 0xF4,&cmdbuf[0], 1, DEFAULT_RETRY_CNT);//change to sorting mode
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(120);

	setDiagCommand(BASEC_GOLDENBC_DIAG_COMMAND);
	cmdbuf[0] = BASEC_CMD;
	ret = i2c_himax_write(private_ts->client, 0xF1,&cmdbuf[0], 1, DEFAULT_RETRY_CNT);//written command: will be test base c
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(120);
	ret = i2c_himax_write(private_ts->client, 0x83,&cmdbuf[0], 0, DEFAULT_RETRY_CNT);//sense on -analog- open
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(120);
	ret = i2c_himax_write(private_ts->client, 0x81,&cmdbuf[0], 0, DEFAULT_RETRY_CNT);//sleep out -digital- open
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	HW_RESET_ACTIVATE = 1;
	himax_int_enable(private_ts->client->irq,1);
	msleep(3000); //begin test base c 

	himax_RawDataCopy(BASEC_CMD);
	tp_log_info("Get basec - End \n");

	himax_int_enable(private_ts->client->irq,0);

	setDiagCommand(CLOSE_DIAG_COMMAND);
	cmdbuf[0] = 0x00;
	ret = i2c_himax_write(private_ts->client, 0xF1,&cmdbuf[0], 1, DEFAULT_RETRY_CNT);//close test
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(120);

	ret = i2c_himax_write(private_ts->client, 0x82,&cmdbuf[0], 0, DEFAULT_RETRY_CNT);//sense off
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(120);	
	ret = i2c_himax_write(private_ts->client, 0x80,&cmdbuf[0], 0, DEFAULT_RETRY_CNT);//sleep in
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(120);
	//get result.
	ret = himax_read_result_data();
	if( ret < 0 ) 
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	seq_printf(m, "%s\n", buf_test_result);

	//get dc and bank data
	himax_HW_reset(true,true);

	//Switch mode
	ret = i2c_himax_write(private_ts->client, 0x82,&cmdbuf[0], 0, DEFAULT_RETRY_CNT);//sleep in --close micro-digital
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(120);	
	ret = i2c_himax_write(private_ts->client, 0x80,&cmdbuf[0], 0, DEFAULT_RETRY_CNT);//sense off -- analog
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(120);
	
	cmdbuf[0] = 0x80;//switch to Sorting mode  //self test command
	ret = i2c_himax_write(private_ts->client, 0xF4,&cmdbuf[0], 1, DEFAULT_RETRY_CNT);
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(120);
	
	ret = i2c_himax_write(private_ts->client, 0x83,&cmdbuf[0], 0, DEFAULT_RETRY_CNT);//sense on -analog- open
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(120);	
	ret = i2c_himax_write(private_ts->client, 0x81,&cmdbuf[0], 0, DEFAULT_RETRY_CNT);//sleep out -digital- open
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	HW_RESET_ACTIVATE = 1;
	himax_int_enable(private_ts->client->irq,1);
	msleep(120); 

	tp_log_info("Get IIR - Start \n");
	setDiagCommand(IIR_DIAG_COMMAND);//IIR command
	command_F1h[1]=IIR_CMD;
	ret = i2c_himax_write(private_ts->client, command_F1h[0] ,&command_F1h[1], 1, DEFAULT_RETRY_CNT);
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(2000);
	himax_RawDataCopy(IIR_CMD);
	tp_log_info("Get IIR - End \n");

	tp_log_info("Get DC - Start \n");
	setDiagCommand(DC_DIAG_COMMAND);//DC command
	command_F1h[1]=DC_CMD;
	ret = i2c_himax_write(private_ts->client, command_F1h[0] ,&command_F1h[1], 1, DEFAULT_RETRY_CNT);
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(1500);
	himax_RawDataCopy(DC_CMD);
	tp_log_info("Get DC - End \n");

	tp_log_info("Get Bank - Start \n");
	setDiagCommand(BANK_DIAG_COMMAND);//Bank command
	command_F1h[1]=BANK_CMD;
	ret = i2c_himax_write(private_ts->client, command_F1h[0] ,&command_F1h[1], 1, DEFAULT_RETRY_CNT);
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(1500);
	himax_RawDataCopy(BANK_CMD);
	tp_log_info("Get Bank - End \n");

	tp_log_info("Get Golden Basec - Start \n");
	setDiagCommand(BASEC_GOLDENBC_DIAG_COMMAND);//Bank command
	command_F1h[1]=GOLDEN_BASEC_CMD;
	ret = i2c_himax_write(private_ts->client, command_F1h[0] ,&command_F1h[1], 1, DEFAULT_RETRY_CNT);
	if( ret != 0 )
	{
		tp_log_err("himax_factory_self_test error!\n");
		goto err_i2c;
	}
	msleep(1500);
	himax_RawDataCopy(GOLDEN_BASEC_CMD);
	tp_log_info("Get Golden Basec  - End \n");

	setDiagCommand(CLOSE_DIAG_COMMAND);//close test

	//print data
	//IIR
	himax_int_enable(private_ts->client->irq,0);
	himax_cat_RawData(IIR_CMD, m);
	//dc
	himax_cat_RawData(DC_CMD, m);
	//bank
	himax_cat_RawData(BANK_CMD, m);
	//golden basec
	himax_cat_RawData(BASEC_CMD, m);
	//basec
	himax_cat_RawData(GOLDEN_BASEC_CMD, m);	



	himax_HW_reset(true,true);
	himax_int_enable(private_ts->client->irq,1);

err_i2c:
	himax_free_Rawmem();
err_alloc:
	wake_unlock(&private_ts->ts_flash_wake_lock);

	atomic_set(&hmx_mmi_test_status, 0);
//	HX_FACTEST_FLAG = 0;
	return 0;
}

static int himax_factory_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, himax_factory_self_test, NULL);
};

static struct file_operations himax_proc_factory_test_ops =
{
	.open = himax_factory_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

void himax_factory_test_init(void)
{
	static struct proc_dir_entry *touch_proc_dir=NULL;
		
	tp_log_info(" %s: enter!\n", __func__);
	
	//add factory test.
	touch_proc_dir = proc_create(HIMAX_PROC_FACTORY_TEST_FILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
		NULL, &himax_proc_factory_test_ops);
	if(touch_proc_dir == NULL)
	{
		tp_log_err(" %s: proc self_test file create failed!\n", __func__);
	}
}

