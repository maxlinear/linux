// SPDX-License-Identifier: GPL-2.0
/*
 * Maxlinear XPCS SysFs
 *
 * Copyright (C) 2020-2022 MaxLinear, Inc.
 * Copyright (C) 2018-2020, Intel Corporation.
 * Xu Liang <lxu@maxlinear.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/device.h>
#include "xpcs.h"
#include "xpcs_regs.h"

static const char * const mode_name[XPCS_MODE_MAX] = {
	[XPCS_MODE_SGMII] = "1G_SGMII",
	[XPCS_MODE_1000BASEX] = "1000BaseX",
	[XPCS_MODE_2500BASEX] = "2500BaseX",
	[XPCS_MODE_5GBASER] = "5G_R",
	[XPCS_MODE_10GBASER] = "10G_R",
	[XPCS_MODE_SUSXGMII] = "SUSXGMII",
	[XPCS_MODE_10GKR] = "10G_KR",
	[XPCS_MODE_QUSXGMII] = "QUSXGMII",
	[XPCS_MODE_3000BASEX] = "3000BaseX",
};

static ssize_t
linksts_show(struct device *dev,
		  struct device_attribute *attr, char *buf)
{
	struct xpcs_prv_data *pdata;
	int off = 0;
	u32 val;

	pdata = dev_get_drvdata(dev);

	if (!pdata->state)
		return sprintf(buf, "\n%s: power off\n", pdata->name);

	off = sprintf(buf + off, "\n%s: LINK Status\n", pdata->name);
	off += sprintf(buf + off, "\n");

	off += sprintf(buf + off, "\nPMA Tx status\n");
	off += sprintf(buf + off, "\n");

	val = XPCS_RGRD_VAL(pdata, PMA_TX_STS, TX_ACK_0);
	off += sprintf(buf + off, "\tTX_ACK_0:                            %s\n",
		       val ? "Tx Acknowledge on Lane 0" :
		       "Tx NOT Acknowledge on Lane 0");

	val = XPCS_RGRD_VAL(pdata, PMA_TX_STS, DETRX_RSLT_0);
	off += sprintf(buf + off, "\tRx Detection Result on lane 0:       %s\n",
		       val ? "Rx Detected on Lane 0" :
		       "Rx NOT Detected on Lane 0");

	val = XPCS_RGRD_VAL(pdata, PMA_TX_PS_CTRL, TX0_PSTATE);
	off += sprintf(buf + off,
		       "\tTX0_PSTATE:                          %08x\n",
		       val);

	off += sprintf(buf + off, "\nPMA Rx status\n");
	off += sprintf(buf + off, "\n");

	val = XPCS_RGRD_VAL(pdata, PMA_STS1, RLU);
	off += sprintf(buf + off, "\tPMA_STS:                             %s\n",
		       val ? "LINK UP" : "LINK DOWN");

	val = XPCS_RGRD_VAL(pdata, PMA_STS2, RF);
	off += sprintf(buf + off, "\tPMA_STATUS2 RF:                      %s\n",
		       val ? "Rx Fault" : "Rx No Fault");

	val = XPCS_RGRD_VAL(pdata, PMA_KR_PMD_STS, RCV_STS);
	off += sprintf(buf + off, "\tPMA_KR_PMD_STS RCV_STS:              %s\n",
		       val ? "Received trained and ready to receive data" :
		       "Rx training");

	val = XPCS_RGRD_VAL(pdata, PMA_KR_PMD_STS, FRM_LCK);
	off += sprintf(buf + off, "\tPMA_KR_PMD_STS FRM_LCK:              %s\n",
		       val ? "Training frame delineation detected" :
		       "Training frame delineation not detected");

	val = XPCS_RGRD_VAL(pdata, PMA_KR_PMD_STS, SU_PR_DTD);
	off += sprintf(buf + off, "\tPMA_KR_PMD_STS SU_PR_DTD:            %s\n",
		       val ? "Start-up protocol in progress" :
		       "Start-up protocol complete");

	val = XPCS_RGRD_VAL(pdata, PMA_KR_PMD_STS, TR_FAIL);
	off += sprintf(buf + off, "\tPMA_KR_PMD_STS TR_FAIL:              %s\n",
		       val ? "Training is completed with failure" :
		       "Training is not yet complete");

	off += sprintf(buf + off, "\n10GBASE-KR LP coeff Update\n");
	off += sprintf(buf + off, "\n");

	val = XPCS_RGRD_VAL(pdata, PMA_KR_LP_CEU, LP_INIT);
	off += sprintf(buf + off, "\tPMA_KR_PMD_STS LP_INIT:              %s\n",
		       val ? "Init the local device tx filter coeff" :
		       "Normal operation");

	val = XPCS_RGRD_VAL(pdata, PMA_KR_LP_CEU, LP_PRST);
	off += sprintf(buf + off, "\tPMA_KR_PMD_STS LP_PRST:              %s\n",
		       val ? "LD Pre-initialize coeffs" :
		       "Normal operation");

	off += sprintf(buf + off, "\n10GBASE-KR LP coeff Status\n");
	off += sprintf(buf + off, "\n");

	val = XPCS_RGRD_VAL(pdata, PMA_KR_LP_CESTS, LP_RR);
	off += sprintf(buf + off, "\tPMA_KR_LP_CESTS LP_RR:               %s\n",
		       val ? "The LP rx has determined that the training is "
		       "complete, and it is ready to receive data." :
		       "The LP rx is requesting that the training "
		       "should be continued.");

	off += sprintf(buf + off, "\n10GBASE-KR LD coeff Update Local Device "
		       "requests to Link Partner.\n");
	off += sprintf(buf + off, "\n");

	val = XPCS_RGRD_VAL(pdata, PMA_KR_LD_CEU, LD_INIT);
	off += sprintf(buf + off, "\tPMA_KR_LD_CEU LD_INIT:               %s\n",
		       val ? "Init the link partner tx filter coeff" :
		       "Normal operation");

	val = XPCS_RGRD_VAL(pdata, PMA_KR_LD_CEU, LD_PRST);
	off += sprintf(buf + off, "\tPMA_KR_LD_CEU PRST:                  %s\n",
		       val ? "LP Pre-initialize coeffs" :
		       "Normal operation");

	off += sprintf(buf + off, "\n10GBASE-KR LD coeff Status\n");
	off += sprintf(buf + off, "\n");

	val = XPCS_RGRD_VAL(pdata, PMA_KR_LD_CESTS, LD_RR);
	off += sprintf(buf + off, "\tPMA_KR_LD_CESTS RR:                  %s\n",
		       val ? "The LD rx has determined that the training is "
		       "complete, and it is ready to receive data." :
		       "The LD rx is requesting that the training "
		       "should be continued.");

	off += sprintf(buf + off, "\nAutonegotiation Status Register\n");
	off += sprintf(buf + off, "\n");

	val = XPCS_RGRD_VAL(pdata, AN_STS, LP_AN_ABL);
	off += sprintf(buf + off, "\tLP AN Ability:                       %s\n",
		       val ? "LP able to participate in AN" :
		       "LP unable to participate in AN");

	val = XPCS_RGRD_VAL(pdata, AN_STS, AN_LS);
	off += sprintf(buf + off, "\tAN Link Status:                      %s\n",
		       val ? "CL73 AN complete and determined a valid link" :
		       "CL73 not complete and no valid link");

	val = XPCS_RGRD_VAL(pdata, AN_STS, AN_ABL);
	off += sprintf(buf + off, "\tLD AN Ability:                       %s\n",
		       val ? "LD supports CL73 AN" :
		       "LD does not support CL73 AN");

	val = XPCS_RGRD_VAL(pdata, AN_STS, AN_RF);
	off += sprintf(buf + off, "\tAN Remote Fault:                     %s\n",
		       val ? "AN process detected a remote fault" :
		       "No fault detected");

	val = XPCS_RGRD_VAL(pdata, AN_STS, ANC);
	off += sprintf(buf + off, "\tAN Complete:                         %s\n",
		       val ? "AN completes" :
		       "AN not complete");

	val = XPCS_RGRD_VAL(pdata, AN_STS, PR);
	off += sprintf(buf + off, "\tPage Received:                       %s\n",
		       val ? "Page is received and the corresponding "
		       "Link Code Word is stored " : "No page received");

	val = XPCS_RGRD_VAL(pdata, AN_STS, PDF);
	off += sprintf(buf + off, "\tParallel Detection Fault in CL73 :   %s\n",
		       val ? "Parallel detection fault detected " :
		       "No fault detected");

	off += sprintf(buf + off, "\nPCS Rx Status\n");
	off += sprintf(buf + off, "\n");

	val = XPCS_RGRD_VAL(pdata, PCS_STS1, RLU);
	off += sprintf(buf + off, "\tPCS_STS:                             %s\n",
		       val ? "LINK UP" : "LINK DOWN");

	val = XPCS_RGRD_VAL(pdata, PCS_STS2, RF);
	off += sprintf(buf + off, "\tPCS_STATUS2 RF:                      %s\n",
		       val ? "Rx Fault" : "Rx No Fault");

	val = XPCS_RGRD_VAL(pdata, PCS_STS2, TF);
	off += sprintf(buf + off, "\tPCS_STATUS2 TF:                      %s\n",
		       val ? "Tx Fault" : "Tx No Fault");

	val = XPCS_RGRD_VAL(pdata, PCS_STS2, CAP_EN);
	off += sprintf(buf + off, "\tCAP_EN:                              %s\n",
		       val ? "10GBASE-R Capable" : "10GBASE-R Not Capable");

	val = XPCS_RGRD_VAL(pdata, PCS_STS2, CAP_10_1GC);
	off += sprintf(buf + off, "\tCAP_10_1GC:                          %s\n",
		       val ? "10GBASE-X Capable" : "10GBASE-X Not Capable");

	val = XPCS_RGRD_VAL(pdata, PCS_STS2, CAP_10GBW);
	off += sprintf(buf + off, "\tCAP_10GBW:                           %s\n",
		       val ? "10GBASE-W Capable" : "10GBASE-W Not Capable");

	val = XPCS_RGRD_VAL(pdata, PCS_STS2, CAP_10GBT);
	off += sprintf(buf + off, "\tCAP_10GBT:                           %s\n",
		       val ? "10GBASE-T Capable" : "10GBASE-T Not Capable");

	val = XPCS_RGRD_VAL(pdata, PCS_STS2, DS);
	off += sprintf(buf + off, "\tDS:                                  %s\n",
		       (val == 2) ?
		       "MMD is present and responding to this reg addr" :
		       "MMD is not present or not functioning properly");

	val = XPCS_RGRD(pdata, PCS_KR_STS2);
	off += sprintf(buf + off,
		       "\tPCS_KR_STS2 :                        %08X\n", val);

	val = XPCS_RGRD_VAL(pdata, PCS_CTRL2, PCS_TYPE_SEL);

	if (val == 0) {
		off += sprintf(buf + off,
			       "\tPCS_TYPE_SEL:                        %s\n",
			       "10GBASE-R PCS Type");
	} else if (val == 1) {
		off += sprintf(buf + off,
			       "\tPCS_TYPE_SEL:                        %s\n",
			       "10GBASE-X PCS Type");
	} else if (val == 2) {
		off += sprintf(buf + off,
			       "\tPCS_TYPE_SEL:                        %s\n",
			       "10GBASE-W PCS Type");
	} else {
		off += sprintf(buf + off,
			       "\tPCS_TYPE_SEL:                        %s\n",
			       "Reserved");
	}

	off += sprintf(buf + off, "\nMII Status\n");
	off += sprintf(buf + off, "\n");

	val = XPCS_RGRD_VAL(pdata, SR_MII_STS, LINK_STS);
	off += sprintf(buf + off, "\t1000BASE-X mode Rx:                  %s\n",
		       val ? "LINK UP" : "LINK DOWN");

	val = XPCS_RGRD_VAL(pdata, VR_MII_AN_CTRL, SGMII_LINK_STS);
	off += sprintf(buf + off, "\tSGMII/QSGMII/USXGMII mode Port0 Rx:  %s\n",
		       val ? "LINK UP" : "LINK DOWN");

	off += sprintf(buf + off, "\n10GBASE-R Status\n");
	off += sprintf(buf + off, "\n");
	val = XPCS_RGRD_VAL(pdata, PCS_KR_STS1, RPCS_BKLK);
	off += sprintf(buf + off, "\t10GBASE-R:  %s\n",
		       val ? "LINK UP" : "LINK DOWN");

	return off;
}

static DEVICE_ATTR_RO(linksts);

static ssize_t
cl73_info_show(struct device *dev,
		    struct device_attribute *attr, char *buf)
{
	struct xpcs_prv_data *pdata;
	int off = 0;
	u32 val;

	pdata = dev_get_drvdata(dev);

	if (!pdata->state)
		return sprintf(buf, "\n%s: power off\n", pdata->name);

	off = sprintf(buf + off, "\n%s: CL 73 Status\n", pdata->name);
	off += sprintf(buf + off, "\n");

	val = XPCS_RGRD_VAL(pdata, AN_INTR, AN_INT_CMPLT);
	off += sprintf(buf + off, "AN_INT_COMPL:                          %s\n",
		       val ? "SET" : "NOT_SET");

	val = XPCS_RGRD_VAL(pdata, AN_INTR, AN_INC_LINK);
	off += sprintf(buf + off, "AN_INC_LINK:                           %s\n",
		       val ? "SET" : "NOT_SET");

	val = XPCS_RGRD_VAL(pdata, AN_INTR, AN_PG_RCV);
	off += sprintf(buf + off, "AN_PG_RCV:                             %s\n",
		       val ? "SET" : "NOT_SET");

	val = XPCS_RGRD_VAL(pdata, AN_STS, LP_AN_ABL);
	off += sprintf(buf + off, "AN_STS:                                %s\n",
		       val ? "LP able to participate in AN" :
		       "LP unable to participate in AN");

	val = XPCS_RGRD_VAL(pdata, AN_STS, AN_LS);
	off += sprintf(buf + off, "AN_STS:                                %s\n",
		       val ? "CL73 AN complete and determined a valid link" :
		       "CL73 not complete and no valid link");

	val = XPCS_RGRD_VAL(pdata, AN_STS, AN_ABL);
	off += sprintf(buf + off, "AN_STS:                                %s\n",
		       val ? "LD supports CL73 AN" :
		       "LD does not support CL73 AN");

	val = XPCS_RGRD_VAL(pdata, AN_STS, AN_RF);
	off += sprintf(buf + off, "AN_STS:                                %s\n",
		       val ? "AN process detected a remote fault" :
		       "No fault detected");

	val = XPCS_RGRD_VAL(pdata, AN_STS, ANC);
	off += sprintf(buf + off, "AN_STS:                                %s\n",
		       val ? "AN completes" :
		       "AN not complete");

	val = XPCS_RGRD_VAL(pdata, AN_STS, PR);
	off += sprintf(buf + off, "AN_STS:                                %s\n",
		       val ? "Parallel detection fault detected " :
		       "No fault detected");

	val = XPCS_RGRD_VAL(pdata, AN_STS, PDF);
	off += sprintf(buf + off, "AN_STS:                                %s\n",
		       val ? "Parallel detection fault detected " :
		       "No fault detected");

	return off;
}

static DEVICE_ATTR_RO(cl73_info);

static ssize_t table_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	struct xpcs_prv_data *pdata;
	int off = 0;
	u32 val;
	u32 mplla_ctrl2 = 0, rx_eq_ctrl5;
	u32 tx_eq0, tx_eq1;

	pdata = dev_get_drvdata(dev);

	if (!pdata->state)
		return sprintf(buf, "\n%s: power off\n", pdata->name);

	off = sprintf(buf + off, "\n%s: Table Show\n", pdata->name);
	off += sprintf(buf + off, "\n");

	off += sprintf(buf + off, "Mode:                          %s\n",
		       mode_name[pdata->mode]);
	off += sprintf(buf + off, "ID:                            %d\n",
		       pdata->mode);
	off += sprintf(buf + off, "\n");

	val = XPCS_RGRD_VAL(pdata, PMA_MPLLA_C0, MPLLA_MULTIPLIER);
	off += sprintf(buf + off, "MPLLA_MULTIPLIER:              %x\n", val);

	val = XPCS_RGRD_VAL(pdata, PMA_MPLLA_C3, MPPLA_BANDWIDTH);
	off += sprintf(buf + off, "MPPLA_BANDWIDTH:               %x\n", val);

	switch (pdata->mode_cfg->lane) {
	case 4:
		val = XPCS_RGRD_VAL(pdata, PMA_VCO_CAL_LD3, VCO_LD_VAL_3);
		off += sprintf(buf + off, "VCO_LD_VAL_3:                  %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_VCO_CAL_REF1, VCO_REF_LD_3);
		off += sprintf(buf + off, "VCO_REF_LD_3:                  %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_TX_RATE_CTRL, TX_RATE_3);
		off += sprintf(buf + off, "TX_RATE_3:                     %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_RX_RATE_CTRL, RX_RATE_3);
		off += sprintf(buf + off, "RX_RATE_3:                     %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_TX_GENCTRL2, TX_WIDTH_3);
		off += sprintf(buf + off, "TX_WIDTH_3:                    %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_RX_GENCTRL2, RX_WIDTH_3);
		off += sprintf(buf + off, "RX_WIDTH_3:                    %x\n",
			       val);

		fallthrough;

	case 3:
		val = XPCS_RGRD_VAL(pdata, PMA_VCO_CAL_LD2, VCO_LD_VAL_2);
		off += sprintf(buf + off, "VCO_LD_VAL_2:                  %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_VCO_CAL_REF1, VCO_REF_LD_2);
		off += sprintf(buf + off, "VCO_REF_LD_2:                  %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_TX_RATE_CTRL, TX_RATE_2);
		off += sprintf(buf + off, "TX_RATE_2:                     %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_RX_RATE_CTRL, RX_RATE_2);
		off += sprintf(buf + off, "RX_RATE_2:                     %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_TX_GENCTRL2, TX_WIDTH_2);
		off += sprintf(buf + off, "TX_WIDTH_2:                    %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_RX_GENCTRL2, RX_WIDTH_2);
		off += sprintf(buf + off, "RX_WIDTH_2:                    %x\n",
			       val);

		fallthrough;

	case 2:
		val = XPCS_RGRD_VAL(pdata, PMA_VCO_CAL_LD1, VCO_LD_VAL_1);
		off += sprintf(buf + off, "VCO_LD_VAL_1:                  %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_VCO_CAL_REF0, VCO_REF_LD_1);
		off += sprintf(buf + off, "VCO_REF_LD_1:                  %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_TX_RATE_CTRL, TX_RATE_1);
		off += sprintf(buf + off, "TX_RATE_1:                     %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_RX_RATE_CTRL, RX_RATE_1);
		off += sprintf(buf + off, "RX_RATE_1:                     %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_TX_GENCTRL2, TX_WIDTH_1);
		off += sprintf(buf + off, "TX_WIDTH_1:                    %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_RX_GENCTRL2, RX_WIDTH_1);
		off += sprintf(buf + off, "RX_WIDTH_1:                    %x\n",
			       val);

		fallthrough;

	case 1:
		val = XPCS_RGRD_VAL(pdata, PMA_VCO_CAL_LD0, VCO_LD_VAL_0);
		off += sprintf(buf + off, "VCO_LD_VAL_0:                  %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_VCO_CAL_REF0, VCO_REF_LD_0);
		off += sprintf(buf + off, "VCO_REF_LD_0:                  %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_TX_RATE_CTRL, TX_RATE_0);
		off += sprintf(buf + off, "TX_RATE_0:                     %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_RX_RATE_CTRL, RX_RATE_0);
		off += sprintf(buf + off, "RX_RATE_0:                     %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_TX_GENCTRL2, TX_WIDTH_0);
		off += sprintf(buf + off, "TX_WIDTH_0:                    %x\n",
			       val);

		val = XPCS_RGRD_VAL(pdata, PMA_RX_GENCTRL2, RX_WIDTH_0);
		off += sprintf(buf + off, "RX_WIDTH_0:                    %x\n",
			       val);

		break;

	default:
		break;
	}

	rx_eq_ctrl5 = XPCS_RGRD(pdata, PMA_RX_EQ_CTRL5);

	val = XPCS_GET_VAL(rx_eq_ctrl5, PMA_RX_EQ_CTRL5, ADPT_SEL);
	off += sprintf(buf + off, "ADPT_SEL:                      %x\n", val);

	val = XPCS_GET_VAL(rx_eq_ctrl5, PMA_RX_EQ_CTRL5, ADPT_MODE);
	off += sprintf(buf + off, "ADPT_MODE:                     %x\n", val);

	val = XPCS_GET_VAL(rx_eq_ctrl5, PMA_RX_EQ_CTRL5, ADPT_PROG);
	off += sprintf(buf + off, "ADPT_PROG:                     %x\n", val);

	val = XPCS_RGRD_VAL(pdata, PMA_RX_EQ_CTRL4, CONT_ADAPT_0);
	off += sprintf(buf + off, "CONT_ADAPT_0:                  %x\n", val);

	val = XPCS_RGRD_VAL(pdata, PMA_RX_EQ_CTRL4, CONT_ADAPT_3_1);
	off += sprintf(buf + off, "CONT_ADAPT_3_1:                %x\n", val);

	mplla_ctrl2 = XPCS_RGRD(pdata, PMA_MPLLA_C2);

	val = XPCS_GET_VAL(mplla_ctrl2, PMA_MPLLA_C2, MPLLA_DIV16P5_CLK_EN);
	off += sprintf(buf + off, "MPLLA_DIV16P5_CLK_EN:          %x\n", val);

	val = XPCS_GET_VAL(mplla_ctrl2, PMA_MPLLA_C2, MPLLA_DIV10_CLK_EN);
	off += sprintf(buf + off, "MPLLA_DIV10_CLK_EN:            %x\n", val);

	val = XPCS_GET_VAL(mplla_ctrl2, PMA_MPLLA_C2, MPLLA_DIV8_CLK_EN);
	off += sprintf(buf + off, "MPLLA_DIV8_CLK_EN:             %x\n", val);

	tx_eq0 = XPCS_RGRD(pdata, PMA_TX_EQ_C0);

	val = XPCS_GET_VAL(tx_eq0, PMA_TX_EQ_C0, TX_EQ_MAIN);
	off += sprintf(buf + off, "TX_EQ_MAIN:                    %x\n", val);

	val = XPCS_GET_VAL(tx_eq0, PMA_TX_EQ_C0, TX_EQ_PRE);
	off += sprintf(buf + off, "TX_EQ_PRE:                     %x\n", val);

	tx_eq1 = XPCS_RGRD(pdata, PMA_TX_EQ_C1);

	val = XPCS_GET_VAL(tx_eq1, PMA_TX_EQ_C1, TX_EQ_POST);
	off += sprintf(buf + off, "TX_EQ_POST:                    %x\n", val);

	val = XPCS_GET_VAL(tx_eq1, PMA_TX_EQ_C1, TX_EQ_OVR_RIDE);
	off += sprintf(buf + off, "TX_EQ_OVR_RIDE:                %x\n", val);

	return off;
}

static DEVICE_ATTR_RO(table);

static const char * const xpcs_status_strings[] = {
	"Wait for Ack High 0",
	"Wait for Ack Low 0",
	"Wait for Ack High 1",
	"Wait for Ack Low 1",
	"Tx/Rx Stable (Power_Good state)",
	"Power Save state",
	"Power Down state",
};

static ssize_t status_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct xpcs_prv_data *pdata;
	int off = 0;
	u32 val = 0;

	pdata = dev_get_drvdata(dev);

	if (!pdata->state)
		return sprintf(buf, "\n%s: power off\n", pdata->name);

	off = sprintf(buf + off, "\n%s: RX/TX Stable Status\n", pdata->name);
	off += sprintf(buf + off, "\n");

	val = XPCS_RGRD_VAL(pdata, PCS_DIG_STS, PSEQ_STATE);

	if (val < ARRAY_SIZE(xpcs_status_strings))
		off += sprintf(buf + off, "\tStatus:		      %s\n",
			       xpcs_status_strings[val]);
	else
		off += sprintf(buf + off, "\tStatus:		      %s\n",
			       "Unknown/Invalid Status");

	return off;
}
static DEVICE_ATTR_RO(status);

static ssize_t reg_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct xpcs_prv_data *pdata = dev_get_drvdata(dev);
	int off = 0;

	pdata = dev_get_drvdata(dev);

	if (!pdata->state)
		return sprintf(buf, "\n%s: power off\n", pdata->name);

	off += sprintf(buf + off, "Usage:\n");
	off += sprintf(buf + off, "    read:  echo <addr> > reg\n");
	off += sprintf(buf + off, "    write: echo <addr> <value> > reg\n");
	off += sprintf(buf + off, "  notes: all parameters are HEX format\n");

	return off;
}

static ssize_t reg_store(struct device * dev, struct device_attribute * attr, const char * buf, size_t count)
{
	struct xpcs_prv_data *pdata = dev_get_drvdata(dev);
	u32 val = 0;
	unsigned long reg_val, reg_off;
	int err;
	char *token;
	char *b = (char *)&buf[0];

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (count >= 4 && strncasecmp(buf, "help", 4) == 0) {
		pr_info("Usage:\n");
		pr_info("    help:  echo help > reg\n");
		pr_info("    read:  echo <addr> > reg\n");
		pr_info("    write: echo <addr> <value> > reg\n");
		return  count;
	}

	token = strsep(&b, " ");
	if (!token)
		return count;
	err = kstrtoul(token, 16, &reg_off);
	if (err) {
		pr_err("incorrect XPCS register offset: %s\n", token);
		return count;
	}
	token = strsep(&b, " ");
	if (token) {
		err = kstrtoul(token, 16, &reg_val);
		if (err) {
			pr_err("incorrect value: %s\n", token);
			return count;
		}
		pr_info("%s: write off %08x val %08x\n", pdata->name,
			(u32)reg_off, (u32)reg_val);
		XPCS_RGWR(pdata, reg_off, reg_val);
	}
	val = XPCS_RGRD(pdata, reg_off);
	pr_info("%s: read off %08x val %08x\n",
		pdata->name, (u32)reg_off, val);
	return count;
}
static DEVICE_ATTR_RW(reg);

static ssize_t rxeq_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct xpcs_prv_data *pdata = dev_get_drvdata(dev);
	struct xpcs_serdes_cfg *pserdes = &pdata->lane_cfg[pdata->mode];
	int off = 0;
	u32 val;

	pdata = dev_get_drvdata(dev);

	if (!pdata->state)
		return sprintf(buf, "\n%s: power off\n", pdata->name);

	off += sprintf(buf + off, "Register:\n");
	val = XPCS_RGRD(pdata, PMA_RX_EQ_CTRL0);
	off += sprintf(buf + off, "  vga1_gain:  %u\n", (val >> 12) & 0x0F);
	off += sprintf(buf + off, "  vga2_gain:  %u\n", (val >> 8) & 0x0F);
	off += sprintf(buf + off, "  ctle_pole:  %u\n", (val >> 5) & 0x07);
	off += sprintf(buf + off, "  ctle_boost: %u\n", val & 0x1F);
	val = XPCS_RGRD(pdata, PMA_RX_ATTN_CTRL);
	off += sprintf(buf + off, "  attn_lvl:   %u\n", val & 0x0F);

	off += sprintf(buf + off, "Configure:\n");
	off += sprintf(buf + off, "  vga1_gain:  %u\n", pserdes->rx_vga1_gain);
	off += sprintf(buf + off, "  vga2_gain:  %u\n", pserdes->rx_vga2_gain);
	off += sprintf(buf + off, "  ctle_pole:  %u\n", pserdes->rx_ctle_pole);
	off += sprintf(buf + off, "  ctle_boost: %u\n", pserdes->rx_ctle_boost);
	off += sprintf(buf + off, "  attn_lvl:   %u\n", pserdes->rx_attn_lvl);

	off += sprintf(buf + off, "Calibration: %u\n", pserdes->rx_cali);

	return off;
}

static ssize_t rxeq_store(struct device * dev, struct device_attribute * attr,
			 const char * buf, size_t count)
{
	struct xpcs_prv_data *pdata = dev_get_drvdata(dev);
	struct xpcs_serdes_cfg *pserdes = &pdata->lane_cfg[pdata->mode];
	char lbuf[256];
	size_t n, len;
	u32 val;

	if (!pdata->state) {
		pr_err("\n%s: power off\n", pdata->name);
		return count;
	}

	len = min(count, sizeof(lbuf)-1);
	memcpy(lbuf, buf, len);
	lbuf[len] = 0;

	for (n = 0; n < len && lbuf[n] > ' '; n++);
	lbuf[n] = 0;
	for (; n < len && lbuf[n] <= ' '; n++);

	if (n >= len) {
		pr_err("echo <param> <val> > rxeq\n");
		pr_err("RX EQ parameter does not take effect until reinit!\n");
		return count;
	}

	val = simple_strtoull(&lbuf[n], NULL, 0);
	if (strncasecmp(lbuf, "vga1_gain", n) == 0)
		pserdes->rx_vga1_gain = val;
	else if (strncasecmp(lbuf, "vga2_gain", n) == 0)
		pserdes->rx_vga2_gain = val;
	else if (strncasecmp(lbuf, "ctle_pole", n) == 0)
		pserdes->rx_ctle_pole = val;
	else if (strncasecmp(lbuf, "ctle_boost", n) == 0)
		pserdes->rx_ctle_boost = val;
	else if (strncasecmp(lbuf, "attn_lvl", n) == 0)
		pserdes->rx_attn_lvl = val;
	else if (strncasecmp(lbuf, "rx_cali", n) == 0
		|| strncasecmp(lbuf, "Calibration", n) == 0)
		pserdes->rx_cali = val ? 1 : 0;
	else {
		pr_err("valid param:\n");
		pr_err("  vga1_gain\n");
		pr_err("  vga2_gain\n");
		pr_err("  ctle_pole\n");
		pr_err("  ctle_boost\n");
		pr_err("  att_lvl\n");
		pr_err("  rx_cali\n");
		pr_err("input:\n");
		pr_err("  \"%s\"\n", lbuf);
		return count;
	}

	pr_notice("RX EQ parameter does not take effect until reinit!\n");
	return count;
}
static DEVICE_ATTR_RW(rxeq);

static ssize_t txeq_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct xpcs_prv_data *pdata = dev_get_drvdata(dev);
	struct xpcs_serdes_cfg *pserdes = &pdata->lane_cfg[pdata->mode];
	int off = 0;
	u32 val, val1;

	pdata = dev_get_drvdata(dev);

	if (!pdata->state)
		return sprintf(buf, "\n%s: power off\n", pdata->name);

	off += sprintf(buf + off, "Register:\n");
	off += sprintf(buf + off, "  iboost:  %lu\n",
		       XPCS_RGRD_VAL(pdata, PMA_TX_BOOST_CTRL, TX0_IBOOST));
	val1 = XPCS_RGRD(pdata, PMA_TX_EQ_C1);
	off += sprintf(buf + off, "  ovrride: %lu\n",
		       XPCS_GET_VAL(val1, PMA_TX_EQ_C1, TX_EQ_OVR_RIDE));
	val = XPCS_RGRD(pdata, PMA_TX_EQ_C0);
	off += sprintf(buf + off, "  main:    %lu\n",
		       XPCS_GET_VAL(val, PMA_TX_EQ_C0, TX_EQ_MAIN));
	off += sprintf(buf + off, "  pre:     %lu\n",
		       XPCS_GET_VAL(val, PMA_TX_EQ_C0, TX_EQ_PRE));
	off += sprintf(buf + off, "  post:    %lu\n",
		       XPCS_GET_VAL(val1, PMA_TX_EQ_C1, TX_EQ_POST));

	off += sprintf(buf + off, "Configure:\n");
	off += sprintf(buf + off, "  iboost:  %u\n", pserdes->tx_iboost_lvl);
	off += sprintf(buf + off, "  ovrride: %u\n", pserdes->tx_eq_ovrride);
	off += sprintf(buf + off, "  main:    %u\n", pserdes->tx_eq_main);
	off += sprintf(buf + off, "  pre:     %u\n", pserdes->tx_eq_pre);
	off += sprintf(buf + off, "  post:    %u\n", pserdes->tx_eq_post);

	return off;
}

static ssize_t txeq_store(struct device * dev, struct device_attribute * attr,
			 const char * buf, size_t count)
{
	struct xpcs_prv_data *pdata = dev_get_drvdata(dev);
	struct xpcs_serdes_cfg *pserdes = &pdata->lane_cfg[pdata->mode];
	char lbuf[256];
	size_t n, len;
	u32 val;

	if (!pdata->state) {
		pr_err("\n%s: power off\n", pdata->name);
		return count;
	}

	len = min(count, sizeof(lbuf)-1);
	memcpy(lbuf, buf, len);
	lbuf[len] = 0;

	for (n = 0; n < len && lbuf[n] > ' '; n++);
	lbuf[n] = 0;
	for (; n < len && lbuf[n] <= ' '; n++);

	if (n >= len) {
		pr_err("echo <param> <val> > rxeq\n");
		return count;
	}

	val = simple_strtoull(&lbuf[n], NULL, 0);
	if (strncasecmp(lbuf, "iboost", n) == 0) {
		pserdes->tx_iboost_lvl = val;
		XPCS_RGWR_VAL(pdata, PMA_TX_BOOST_CTRL, TX0_IBOOST, val);
	} else if (strncasecmp(lbuf, "ovrride", n) == 0) {
		pserdes->tx_eq_ovrride = val;
		XPCS_RGWR_VAL(pdata, PMA_TX_EQ_C1, TX_EQ_OVR_RIDE, val);
	} else if (strncasecmp(lbuf, "main", n) == 0) {
		pserdes->tx_eq_main = val;
		XPCS_RGWR_VAL(pdata, PMA_TX_EQ_C0, TX_EQ_MAIN, val);
	} else if (strncasecmp(lbuf, "pre", n) == 0) {
		pserdes->tx_eq_pre = val;
		XPCS_RGWR_VAL(pdata, PMA_TX_EQ_C0, TX_EQ_PRE, val);
	} else if (strncasecmp(lbuf, "post", n) == 0) {
		pserdes->tx_eq_post = val;
		XPCS_RGWR_VAL(pdata, PMA_TX_EQ_C1, TX_EQ_POST, val);
	} else {
		pr_err("valid param:\n");
		pr_err("  iboost\n");
		pr_err("  ovrride\n");
		pr_err("  main\n");
		pr_err("  pre\n");
		pr_err("  post\n");
		pr_err("input:\n");
		pr_err("  \"%s\"\n", lbuf);
	}

	return count;
}
static DEVICE_ATTR_RW(txeq);

static ssize_t mode_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct xpcs_prv_data *pdata = dev_get_drvdata(dev);
	int off = 0;

	pdata = dev_get_drvdata(dev);

	if (!pdata->state)
		return sprintf(buf, "\n%s: power off\n", pdata->name);

	off += sprintf(buf, "%s\n", mode_name[pdata->mode]);

	return off;
}

static ssize_t mode_store(struct device * dev, struct device_attribute * attr,
			 const char * buf, size_t count)
{
	char lbuf[256];
	size_t n, len;

	len = min(count, sizeof(lbuf)-1);
	for (n = 0; n < len && buf[n] > ' '; n++)
		lbuf[n] = buf[n];
	lbuf[n] = 0;
	len = n;

	for (n = 0; n < ARRAY_SIZE(mode_name); n++) {
		if (strncasecmp(lbuf, mode_name[n], len))
			continue;
		xpcs_reinit(dev, n);
		return count;
	}
	if (n >= ARRAY_SIZE(mode_name)) {
		pr_err("valid modes:\n");
		for (n = 0; n < ARRAY_SIZE(mode_name); n++)
			pr_err("  %s\n", mode_name[n]);
		pr_err("input:\n");
		pr_err("  \"%s\"\n", lbuf);
	}

	return count;
}
static DEVICE_ATTR_RW(mode);

static ssize_t cl37_show(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct xpcs_prv_data *pdata = dev_get_drvdata(dev);
	const char *an_name[] = {"disable", "SGMII MAC Side", "SGMII PHY Side",
				"1000/2500-BaseX"};
	const struct {
		const char *name;
		u32 addr;
	} cl37_regs[] = {
		{"SR_AN_CTRL", AN_CTRL},
		{"SR_MII_CTRL", SR_MII_CTRL},
		{"SR_MII_AN_ADV", SR_MII_AN_ADV},
		{"SR_MII_LP_BABL", SR_MII_LP_BABL},
		{"SR_MII_EXPN", SR_MII_EXPN},
		{"VR_MII_DIG_CTRL1", VR_MII_DIG_CTRL1},
		{"VR_MII_AN_CTRL", VR_MII_AN_CTRL},
		{"VR_MII_AN_INTR_STS", VR_MII_AN_INTR_STS},
		{"VR_MII_LINK_TIMER_CTRL", VR_MII_LINK_TIMER_CTRL},
	};
	ssize_t off = 0, i;
	u32 usxgmii_en, usxgmii_mode;
	u32 mode, phy_side, val;
	const char *str1, *str2, *str3;
	const char *linksts = NULL;

	pdata = dev_get_drvdata(dev);

	if (!pdata->state)
		return sprintf(buf, "\n%s: power off\n", pdata->name);

	off += sprintf(buf, "xpcs-cl37: %u, %s\n", pdata->cl37[pdata->mode],
			an_name[pdata->cl37[pdata->mode]]);

	off += sprintf(buf + off, "CL37 Registers:\n");
	for (i = 0; i < ARRAY_SIZE(cl37_regs); i++)
		off += sprintf(buf + off, "  %22s (0x%06x) - 0x%04x\n",
				cl37_regs[i].name, cl37_regs[i].addr,
				XPCS_RGRD(pdata, cl37_regs[i].addr) & 0xFFFF);

	off += sprintf(buf + off, "CL37 Status:\n");
	str1 = XPCS_RGRD_VAL(pdata, AN_CTRL, AN_EN) ? "enabled" : "disabled";
	off += sprintf(buf + off, "  CL73              - %s\n", str1);
	str1 = XPCS_RGRD_VAL(pdata, PCS_DIG_CTRL1, CL37_BP)
		? "enabled" : "disabled";
	off += sprintf(buf + off, "  CL37 in backplane - %s\n", str1);
	str1 = XPCS_RGRD_VAL(pdata, SR_MII_CTRL, AN_ENABLE)
		? "enabled" : "disabled";
	off += sprintf(buf + off, "  CL37              - %s\n", str1);

	usxgmii_en = XPCS_RGRD_VAL(pdata, PCS_DIG_CTRL1, USXG_EN);
	if (usxgmii_en) {
		mode = 4;
		usxgmii_mode = XPCS_RGRD_VAL(pdata, PCS_KR_CTRL, USXG_MODE);
	} else {
		mode = XPCS_RGRD_VAL(pdata, VR_MII_AN_CTRL, PCS_MODE);
		usxgmii_mode = U32_MAX;
	}
	phy_side = XPCS_RGRD_VAL(pdata, VR_MII_AN_CTRL, TX_CONFIG);
	switch (mode) {
	case 0:	/* 1000BaseX mode */
		off += sprintf(buf + off, "  ANEG Mode         - BaseX\n");
		off += sprintf(buf + off, "    Advertised    15 Next Page %lu\n",
			       XPCS_RGRD_VAL(pdata, SR_MII_AN_ADV, NP));
		val = XPCS_RGRD_VAL(pdata, SR_MII_AN_ADV, RF);
		switch (val) {
		case 0: linksts = "No Error"; break;
		case 1: linksts = "Offline"; break;
		case 2: linksts = "Link Failure"; break;
		case 3: linksts = "ANEG Error"; break;
		default: linksts = "Unknown";
		}
		off += sprintf(buf + off, "               12~13 %s (%u)\n",
				linksts, val);
		val = XPCS_RGRD_VAL(pdata, SR_MII_AN_ADV, PAUSE);
		switch (val) {
		case 0:
			str1 = "No Pause";
			break;
		case 1:
			str1 = "Asymmetric Pause towards the link partner";
			break;
		case 2:
			str1 = "Symmetric Pause";
			break;
		case 3:
			str1 = "Symmetric Pause and Asymmetric Pause towards the local device";
			break;
		default:
			str1 = "Unknown";
		}
		off += sprintf(buf + off, "                 7~8 %s (%u)\n",
				str1, val);
		if (XPCS_RGRD_VAL(pdata, SR_MII_AN_ADV, HD))
			str1 = "Supported";
		else
			str1 = "Not Supported";
		off += sprintf(buf + off, "                   6 Half Duplex %s\n",
				str1);
		if (XPCS_RGRD_VAL(pdata, SR_MII_AN_ADV, FD))
			str1 = "Supported";
		else
			str1 = "Not Supported";
		off += sprintf(buf + off, "                   5 Full Duplex %s\n",
				str1);
		break;
	case 2:	/* SGMII mode */
	case 3:	/* QSGMII mode */
	case 4:	/* USXGMII mode */
		switch (usxgmii_mode) {
		case 0: str1 = "10G-SXGMII"; break;
		case 1: str1 = "5G-SXGMII"; break;
		case 2: str1 = "2.5G-SXGMII"; break;
		case 3: str1 = "10G-DXGMII"; break;
		case 4: str1 = "5G-DXGMII"; break;
		case 5: str1 = "10G-QXGMII"; break;
		case U32_MAX: str1 = mode == 2 ? "SGMII" : "QSGMII"; break;
		default: str1 = "Unknown"; break;
		}
		str2 = phy_side ? "PHY" : "MAC";
		off += sprintf(buf + off, "  ANEG Mode         - %s %s Side\n",
				str1, str2);
		if (!phy_side) {
			off += sprintf(buf + off, "    Advertised Word 0x4001\n");
			break;
		}
		if (XPCS_RGRD_VAL(pdata, VR_MII_DIG_CTRL1, PHY_MODE_CTRL)) {
			off += sprintf(buf + off, "    Advertised    15 xpcs_sgmii_link_sts_i\n");
			off += sprintf(buf + off, "                  14 1\n");
			off += sprintf(buf + off, "                  12 xpcs_sgmii_full_duplex_i\n");
			off += sprintf(buf + off, "               10~11 xpcs_sgmii_link_speed_i[1:0]\n");
			off += sprintf(buf + off, "                   0 1\n");
			break;
		}
		val = XPCS_RGRD_VAL(pdata, VR_MII_AN_CTRL, SGMII_LINK_STS);
		linksts = val ? "Link Up" : "Link Down";
		off += sprintf(buf + off, "    Advertised    15 %s (%u)\n",
				linksts, val);
		off += sprintf(buf + off, "                  14 1\n");
		val = XPCS_RGRD_VAL(pdata, SR_MII_AN_ADV, FD);
		str1 = val ? "Full Duplex" : "Half Duplex";
		off += sprintf(buf + off, "                  12 %s (%u)\n",
				str1, val);
		val = XPCS_RGRD_VAL(pdata, SR_MII_CTRL, SS5) << 2;
		val |= XPCS_RGRD_VAL(pdata, SR_MII_CTRL, SS6) << 1;
		val |= XPCS_RGRD_VAL(pdata, SR_MII_CTRL, SS13);
		switch (val) {
		case 0: str1 = "10Mbps"; break;
		case 1: str1 = "100Mbps"; break;
		case 2: str1 = "1000Mbps"; break;
		case 3: str1 = "10Gbps"; break;
		case 4: str1 = "2.5Gbps"; break;
		case 5: str1 = "5Gbps"; break;
		default: str1 = "Unknown";
		}
		off += sprintf(buf + off, "               10~11 %s (%u)\n",
				str1, val);
		off += sprintf(buf + off, "                   0 1\n");
		break;
	default:
		off += sprintf(buf + off, "  ANEG Mode         - Unknown (%u)\n",
				mode);
	}

	if (XPCS_RGRD_VAL(pdata, VR_MII_DIG_CTRL1, CL37_TMR_OVR_RIDE)) {
		val = XPCS_RGRD(pdata, VR_MII_LINK_TIMER_CTRL) & 0xFFFF;
		off += sprintf(buf + off, "  Link Timer        - 0x%04x\n", val);
	} else
		off += sprintf(buf + off, "  Link Timer        - default\n");
	str1 = XPCS_RGRD_VAL(pdata, VR_MII_AN_CTRL, MII_CONTROL)
		? "8-bit GMII Mode" : "4-bit MII Mode";
	off += sprintf(buf + off, "  MII (100/10Mbps)  - %s\n", str1);
	str1 = XPCS_RGRD_VAL(pdata, VR_MII_AN_CTRL, MII_AN_INTR_EN)
		? "enabled" : "disabled";
	off += sprintf(buf + off, "  ANEG Complete Int - %s\n", str1);
	str1 = XPCS_RGRD_VAL(pdata, VR_MII_AN_CTRL, MII_AN_INTR_EN)
		? "enabled" : "disabled";

	val = XPCS_RGRD(pdata, VR_MII_AN_INTR_STS) & 0xFFFF;
	XPCS_RGWR(pdata, VR_MII_AN_INTR_STS, val & ~1);
	str1 = (val & BIT(0)) ? "completed" : "not completed";
	off += sprintf(buf + off, "  ANEG Result       - %s (clear on read)\n",
		       str1);
	str1 = (val & BIT(usxgmii_en ? 14 : 4)) ? "link up" : "link down";
	off += sprintf(buf + off, "                      %s\n", str1);
	str2 = (val & BIT(usxgmii_en ? 13 : 1)) ? "full duplex" : "half duplex";
	off += sprintf(buf + off, "                      %s\n", str2);
	val = usxgmii_en ? ((val >> 10) & 7) : ((val >> 2) & 3);
	switch (val) {
	case 0: str3 = "10Mbps"; break;
	case 1: str3 = "100Mbps"; break;
	case 2: str3 = "1000Mbps"; break;
	case 3: str3 = "10Gbps"; break;
	case 4: str3 = "2.5Gbps"; break;
	case 5: str3 = "5Gbps"; break;
	default: str3 = NULL;
	}
	if (str3) {
		off += sprintf(buf + off, "                      %s\n", str3);
	} else {
		off += sprintf(buf + off,
			       "                      unknown speed (%u)\n",
			       val);
	}

	if (XPCS_RGRD_VAL(pdata, VR_MII_DIG_CTRL1, PHY_MODE_CTRL)) {
		off += sprintf(buf + off, "  Setting aft. ANEG - input signals\n");
		off += sprintf(buf + off, "                      xpcs_sgmii_link_sts_i\n");
		off += sprintf(buf + off, "                      xpcs_sgmii_full_duplex_i\n");
		off += sprintf(buf + off, "                      xpcs_sgmii_link_speed_i[1:0]\n");
	} else if (XPCS_RGRD_VAL(pdata, VR_MII_DIG_CTRL1, MAC_AUTO_SW)) {
		off += sprintf(buf + off, "  Setting aft. ANEG - ANEG result\n");
		off += sprintf(buf + off, "                      %s\n", str1);
		off += sprintf(buf + off, "                      %s\n", str2);
		if (str3) {
			off += sprintf(buf + off,
				       "                      %s\n",
				       str3);
		} else {
			off += sprintf(buf + off,
				       "                      unknown speed (%u)\n",
				       val);
		}
	} else {
		off += sprintf(buf + off, "  Setting aft. ANEG - MII Settings\n");

		off += sprintf(buf + off, "                      %s\n", linksts ? linksts : "unknown");
		str1 = XPCS_RGRD_VAL(pdata, SR_MII_CTRL, DUPLEX_MODE)
			? "full duplex" : "half duplex";
		off += sprintf(buf + off, "                      %s\n", str1);
		val = XPCS_RGRD_VAL(pdata, SR_MII_CTRL, SS5) << 2;
		val |= XPCS_RGRD_VAL(pdata, SR_MII_CTRL, SS6) << 1;
		val |= XPCS_RGRD_VAL(pdata, SR_MII_CTRL, SS13);
		switch (val) {
		case 0: str1 = "10Mbps"; break;
		case 1: str1 = "100Mbps"; break;
		case 2: str1 = "1000Mbps"; break;
		case 3: str1 = "10Gbps"; break;
		case 4: str1 = "2.5Gbps"; break;
		case 5: str1 = "5Gbps"; break;
		default: str1 = "Unknown";
		}
		off += sprintf(buf + off, "                      %s\n", str1);
	}

	return off;
}

static void cl37_help(void)
{
	pr_info("Usage:\n");
	pr_info("  echo <cmd> [mac|phy] [speed <10|100|1000|2500>] > cl37\n");
	pr_info("  cmd: help    - this page\n");
	pr_info("       disable - disable CL37\n");
	pr_info("       sgmii   - enable SGMII ANEG, take parameters MAC, PHY, speed\n");
	pr_info("       basex   - enable 1000/2500-BaseX ANEG\n");
	pr_info("       restart - restart ANEG\n");
}

static ssize_t cl37_store(struct device * dev, struct device_attribute * attr,
			 const char * buf, size_t count)
{
	const char *ct = " \t\n";
	struct xpcs_prv_data *pdata = dev_get_drvdata(dev);
	char lbuf[256];
	size_t len;
	char *p, *token;

	if (pdata->mode == XPCS_MODE_QUSXGMII)
		pr_warn("Warning: only channel 0 is configured!\n");

	len = min(count, sizeof(lbuf)-1);
	memcpy(lbuf, buf, len);
	lbuf[len] = '\0';

	p = lbuf;
	token = strsep(&p, ct);
	if (!token) {
		cl37_help();
		return count;
	}

	len = strlen(token);
	if (strncasecmp(token, "disable", min_t(size_t, len, 7UL)) == 0) {
		pdata->cl37[pdata->mode] = CONN_TYPE_ANEG_DIS;
		if (pdata->mode == XPCS_MODE_2500BASEX ||
		    pdata->mode == XPCS_MODE_QUSXGMII)
			pdata->speed = SPEED_2500;
		else if (pdata->mode == XPCS_MODE_5GBASER)
			pdata->speed = SPEED_5000;
		else if (pdata->mode == XPCS_MODE_10GBASER ||
			 pdata->mode == XPCS_MODE_SUSXGMII ||
			 pdata->mode == XPCS_MODE_10GKR)
			pdata->speed = SPEED_10000;
		else
			pdata->speed = SPEED_1000;
		XPCS_RGWR_VAL(pdata, SR_MII_CTRL, SS13, 0);
		XPCS_RGWR_VAL(pdata, SR_MII_CTRL, SS6, 1);
		xpcs_disable_an(pdata);
		pr_info("Disable Clause 37\n");
	} else if (strncasecmp(token, "sgmii", min_t(size_t, len, 5UL)) == 0) {
		u32 set_phy = 0, set_speed = 0;
		u32 speed = 2;

		for (token = strsep(&p, ct); token; token = strsep(&p, ct)) {
			len = strlen(token);
			if (!len)
				continue;

			if (set_speed) {
				switch (simple_strtoull(token, NULL, 0)) {
				case 10:  speed = 0; break;
				case 100: speed = 1; break;
				default: break;
				}
				set_speed = 0;
				continue;
			}

			if (!strncasecmp(token, "PHY", min_t(size_t, len, 3UL)))
				set_phy = 1;
			else if (!strncasecmp(token, "speed",
					      min_t(size_t, len, 5UL)))
				set_speed = 1;
		}

		pdata->cl37[pdata->mode] = set_phy
					   ? CONN_TYPE_ANEG_PHY
					   : CONN_TYPE_ANEG_MAC;
		switch (speed) {
		case 0: pdata->speed = SPEED_10; break;
		case 1: pdata->speed = SPEED_100; break;
		default: pdata->speed = (pdata->mode == XPCS_MODE_2500BASEX ||
					 pdata->mode == XPCS_MODE_QUSXGMII) ?
					SPEED_2500 : SPEED_1000;
		}
		XPCS_RGWR_VAL(pdata, SR_MII_CTRL, SS13, speed & BIT(0));
		XPCS_RGWR_VAL(pdata, SR_MII_CTRL, SS6, (speed >> 1) & BIT(0));
		xpcs_cl37_an(pdata);
		if (set_phy)
			pr_info("Enable Clause 37, SGMII PHY Side ANEG\n");
		else
			pr_info("Enable Clause 37, SGMII MAC Side ANEG\n");
	} else if (strncasecmp(token, "basex", min_t(size_t, len, 5UL)) == 0) {
		pdata->cl37[pdata->mode] = CONN_TYPE_ANEG_BX;
		if (pdata->mode == XPCS_MODE_2500BASEX ||
		    pdata->mode == XPCS_MODE_QUSXGMII)
			pdata->speed = SPEED_2500;
		else
			pdata->speed = SPEED_1000;
		XPCS_RGWR_VAL(pdata, SR_MII_CTRL, SS13, 0);
		XPCS_RGWR_VAL(pdata, SR_MII_CTRL, SS6, 1);
		xpcs_cl37_an(pdata);
		pr_info("Enable Clause 37, 1000/2500-BaseX ANEG\n");
	} else if (strncasecmp(token, "restart", min_t(size_t, len, 7UL)) == 0
		|| strncasecmp(token, "reaneg", min_t(size_t, len, 6UL)) == 0) {
		XPCS_RGWR_VAL(pdata, SR_MII_CTRL, RESTART_ANEG, 1);
	} else
		cl37_help();

	return count;
}
static DEVICE_ATTR_RW(cl37);

static struct attribute *xpcs_attrs[] = {
	&dev_attr_cl73_info.attr,
	&dev_attr_status.attr,
	&dev_attr_table.attr,
	&dev_attr_linksts.attr,
	&dev_attr_reg.attr,
	&dev_attr_rxeq.attr,
	&dev_attr_txeq.attr,
	&dev_attr_mode.attr,
	&dev_attr_cl37.attr,
	NULL,
};

ATTRIBUTE_GROUPS(xpcs);

int xpcs_sysfs_init(struct xpcs_prv_data *priv)
{
	return devm_device_add_groups(priv->dev, xpcs_groups);
}

