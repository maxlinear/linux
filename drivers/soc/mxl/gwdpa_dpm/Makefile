ccflags-y += $(extra_ccflags)

obj-$(CONFIG_DPM_DATAPATH_EMULATE_BSP_TEST) += emulate_bsp/
obj-$(CONFIG_DPM_DATAPATH) += \
	datapath_api.o datapath_misc.o datapath_notifier.o datapath_instance.o \
	datapath_soc.o datapath_qos.o datapath_tx.o datapath_rx.o \
	datapath_cqm.o datapath_ndo_ops.o datapath_dflt_q_map.o \
	datapath_proc_api.o datapath_dts.o datapath_netdev.o
obj-$(CONFIG_DPM_DATAPATH_DISABLE_OPTIMIZE) += datapath_gdb_wa.o
obj-$(CONFIG_DPM_DATAPATH) += datapath_proc.o datapath_proc_qos.o
obj-$(CONFIG_DPM_DATAPATH) += hal/
