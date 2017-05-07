#
# Makefile for the Linux proc filesystem routines.
#

obj-y   += proc.o

proc-y			:= nommu.o task_nommu.o
proc-$(CONFIG_MMU)	:= task_mmu.o

proc-y       += inode.o root.o base.o generic.o array.o \
		fd.o
proc-$(CONFIG_TTY)      += proc_tty.o
proc-y	+= cmdline.o
proc-y	+= consoles.o
proc-y	+= cpuinfo.o
proc-y	+= devices.o
proc-y	+= interrupts.o
proc-y	+= loadavg.o
proc-y	+= meminfo.o
proc-y	+= stat.o
proc-y	+= uptime.o
proc-y	+= version.o
proc-y	+= softirqs.o
proc-y	+= namespaces.o
proc-y	+= self.o
proc-$(CONFIG_PROC_SYSCTL)	+= proc_sysctl.o
proc-$(CONFIG_NET)		+= proc_net.o
proc-$(CONFIG_PROC_KCORE)	+= kcore.o
proc-$(CONFIG_PROC_VMCORE)	+= vmcore.o
proc-$(CONFIG_PROC_DEVICETREE)	+= proc_devtree.o
proc-$(CONFIG_PRINTK)	+= kmsg.o
proc-$(CONFIG_PROC_PAGE_MONITOR)	+= page.o
#/* < DTS2014040104155 xufeng 20140401 begin */
proc-$(CONFIG_APP_INFO)         += app_info.o
#/* DTS2014040104155 xufeng 20140401 end > */
#/*< DTS2014120404357 yedapeng 20141201 begin*/
proc-y  += check_root.o
#/*DTS2014120404357 yedapeng 20141201 end >*/
#/* < DTS2014112904825 duanhuan 20141129 begin */
proc-$(CONFIG_FEATURE_HUAWEI_EMERGENCY_DATA) += emerg_data.o
#/* DTS2014112904825 duanhuan 20141129 end > */
