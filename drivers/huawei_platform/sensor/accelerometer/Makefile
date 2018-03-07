obj-$(CONFIG_HUAWEI_ACCEL_SENSORS_LIS3DH)		+= lis3dh_acc.o lis3dh_acc_monitor.o
obj-$(CONFIG_HUAWEI_ACCEL_SENSORS_KIONIX)		+= kionix_accel.o kionix_acc_monitor.o
obj-$(CONFIG_HUAWEI_ACCEL_SENSORS_BMA2x2)		+= bma2x2.o bstclass.o
ifeq ($(CONFIG_SENSORS_BMA2X2_ENABLE_INT1),y)
	EXTRA_CFLAGS += -DBMA2X2_ENABLE_INT1
endif
