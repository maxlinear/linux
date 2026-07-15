# SPDX-License-Identifier: GPL-2.0

KCOV_INSTRUMENT := $(CONFIG_QOS_TC_KCOV)

# Build qos_tc.ko loadable module when QOS_TC is enabled
obj-$(CONFIG_QOS_TC) := qos_tc.o

# Common QoS TC objects (always included)
qos_tc-y := \
	qos_tc_main.o \
	qos_tc_qos.o \
	qos_tc_drr.o \
	qos_tc_debugfs.o \
	qos_tc_red.o \
	qos_tc_prio.o \
	qos_tc_mqprio.o \
	qos_tc_flower.o \
	qos_tc_tbf.o \
	qos_tc_codel.o \
	qos_tc_cpu_qos.o

# Switch offload objects — compiled only when enabled and linked into qos_tc.ko
qos_tc-$(CONFIG_QOS_TC_SWITCH_OFFLOAD) += \
	switch/qos_tc_qmap_ops.o \
	switch/qos_tc_qmap.o \
	switch/qos_tc_vlan_prepare.o \
	switch/qos_tc_vlan_storage.o \
	switch/qos_tc_ext_vlan.o \
	switch/qos_tc_vlan_filter.o \
	switch/qos_tc_pce.o \
	switch/qos_tc_parser.o \
	switch/qos_tc_mirred.o \
	switch/qos_tc_ds_tc_remap.o \
	switch/qos_tc_trap.o \
	switch/qos_tc_ip_drop.o \
	switch/qos_tc_skbedit.o \
	switch/qos_tc_police.o \
	switch/qos_tc_extract_cfm.o

# Optional tracing support
qos_tc-$(CONFIG_QOS_TC_TRACING) += qos_tc_trace.o
