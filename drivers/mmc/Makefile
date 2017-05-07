#
# Makefile for the kernel mmc device drivers.
#

subdir-ccflags-$(CONFIG_MMC_DEBUG) := -DDEBUG

obj-$(CONFIG_MMC)		+= core/
obj-$(CONFIG_MMC)		+= card/
obj-$(subst m,y,$(CONFIG_MMC))	+= host/
#/* < DTS2015012801593 zhaoyingchun 20150128 begin */
obj-$(CONFIG_MMC_SWRM)          += swrm/
#/* DTS2015012801593 zhaoyingchun 20150128 end > */

