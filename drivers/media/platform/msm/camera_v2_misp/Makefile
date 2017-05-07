#/* < DTS2014122203451 yangyang 20141223 begin */
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/sensor
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/codecs
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/isps
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/pproc
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/msm_vb2
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/camera
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/jpeg_10
ccflags-y += -Idrivers/media/platform/msm/camera_v2_misp/fd
#/* DTS2014122203451 yangyang 20141223 end > */

obj-$(CONFIG_MSMB_CAMERA) += msm.o
#/* < DTS2014072601546 yanhuiwen/0083550 20140723 begin */
obj-$(CONFIG_MSMB_CAMERA) += hw_camera_common.o
#/* DTS2014072601546 yanhuiwen/00283550 20140723 end > */
#/* < DTS2014081209165 Zhangbo 00166742 20140812 begin */
obj-$(CONFIG_MSMB_CAMERA) += msm_camera_dsm.o
#/* DTS2014081209165 Zhangbo 00166742 20140812 end > */
obj-$(CONFIG_MSMB_CAMERA) += camera/
obj-$(CONFIG_MSMB_CAMERA) += msm_vb2/
obj-$(CONFIG_MSMB_CAMERA) += sensor/
obj-$(CONFIG_MSMB_CAMERA) += pproc/
obj-$(CONFIG_MSMB_CAMERA) += isp/
obj-$(CONFIG_MSMB_CAMERA) += ispif/
obj-$(CONFIG_MSMB_JPEG) += jpeg_10/
obj-$(CONFIG_MSMB_CAMERA) += msm_buf_mgr/
#/*<DTS2014060405107 luozhi/wx217218 20140604 begin*/
obj-$(CONFIG_MSMB_CAMERA) += hw_camera_log.o
#/*DTS2014060405107 luozhi/wx217218 20140604 end>*/

