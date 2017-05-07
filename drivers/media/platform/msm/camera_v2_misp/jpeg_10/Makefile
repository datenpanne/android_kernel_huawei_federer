GCC_VERSION      := $(shell $(CONFIG_SHELL) $(srctree)/scripts/gcc-version.sh $(CROSS_COMPILE)gcc)

#/* < DTS2014122203451 yangyang 20141223 begin */
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/jpeg_10
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/sensor/io
#/* DTS2014122203451 yangyang 20141223 end > */

obj-$(CONFIG_MSMB_JPEG) += msm_jpeg_dev.o msm_jpeg_sync.o msm_jpeg_core.o msm_jpeg_hw.o msm_jpeg_platform.o
