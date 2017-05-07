#
# Makefile for MMC/SD card drivers
#

obj-$(CONFIG_MMC_BLOCK)		+= mmc_block.o
mmc_block-objs			:= block.o queue.o
obj-$(CONFIG_MMC_TEST)		+= mmc_test.o

obj-$(CONFIG_SDIO_UART)		+= sdio_uart.o

obj-$(CONFIG_MMC_BLOCK_TEST)		+= mmc_block_test.o
#/* < DTS2015042905057 duanhuan 20150429 begin */
#remove it
#/* DTS2015042905057 duanhuan 20150429 end > */
#/* < DTS2014112404668 wangwenlei 20141124 begin */
obj-$(CONFIG_HW_MMC_TEST_MODULE)	+= hw_mmc_test.o
#/* DTS2014112404668 wangwenlei 20141124 end > */
#/* < DTS2015012801593 zhaoyingchun 20150128 begin */ 
obj-$(CONFIG_MMC_FFU)		+= ffu.o
#/* DTS2015012801593 zhaoyingchun 20150128 end > */ 

# /*< DTS2015070911954 yuanxiaofeng 20150709 begin */
obj-$(CONFIG_HW_SD_HEALTH_DETECT)	+= mmc_health_diag.o
# /* DTS2015070911954 yuanxiaofeng 20150709 end>*/
