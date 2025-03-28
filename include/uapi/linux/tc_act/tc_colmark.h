/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/******************************************************************************
 *
 * Copyright (c) 2022 MaxLinear, Inc.
 *
 *****************************************************************************/
#ifndef __LINUX_TC_COLMARK_H
#define __LINUX_TC_COLMARK_H

#include <linux/pkt_cls.h>

#define COLMARK_F_MODE			0x1
#define COLMARK_F_DROP_PRECEDENCE	0x2
#define COLMARK_F_METER_TYPE		0x4

enum tc_drop_precedence {
	TC_COLMARK_NO_MARKING,
	TC_COLMARK_INTERNAL = 1,
	TC_COLMARK_DEI = 2,
	TC_COLMARK_PCP_8P0D = 3,
	TC_COLMARK_PCP_7P1D = 4,
	TC_COLMARK_PCP_6P2D = 5,
	TC_COLMARK_PCP_5P3D = 6,
	TC_COLMARK_DSCP_AF = 7,
};

enum tc_meter_type {
	TC_COLMARK_SRTCM,
	TC_COLMARK_TRTCM,
};

struct tc_colmark {
	tc_gen;
};

enum {
	TCA_COLMARK_UNSPEC,
	TCA_COLMARK_TM,
	TCA_COLMARK_PARMS,
	TCA_COLMARK_MODE,
	TCA_COLMARK_DROP_PRECEDENCE,
	TCA_COLMARK_METER_TYPE,
	TCA_COLMARK_PAD,
	__TCA_COLMARK_MAX
};
#define TCA_COLMARK_MAX (__TCA_COLMARK_MAX - 1)

#endif
