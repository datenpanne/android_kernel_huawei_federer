#/* < DTS2014122203451 yangyang 20141223 begin */
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/sensor/io
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/sensor/cci
#/*< DTS2015031803765 ganfan/206212 20150318 begin*/
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/sensor/mini_isp
#/*DTS2015031803765 ganfan/206212 20150318 end >*/
#/* DTS2014122203451 yangyang 20141223 end > */
# /*< DTS2015012606635 tangying/205982 20150126 begin*/
# /*add camera ois driver*/
obj-$(CONFIG_MSMB_CAMERA) += imx278_ois/
obj-$(CONFIG_MSMB_CAMERA) += msm_ois.o

obj-$(CONFIG_MSMB_CAMERA) += mini_isp_ois_interface.o
# /*DTS2015012606635 tangying/205982 20150126 end >*/
