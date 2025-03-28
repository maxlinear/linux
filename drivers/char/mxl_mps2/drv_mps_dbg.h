#ifndef _DRV_MPS_VMMC_DBG_H
#define _DRV_MPS_VMMC_DBG_H
/******************************************************************************

               Copyright (c) 2017 - 2019 Intel Corporation
               Copyright (c) 2011 - 2016 Lantiq Beteiligungs-GmbH & Co. KG


  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

****************************************************************************
   Module      : drv_mps_vmmc_dbg.h
   Description : This file contains the macro definitions for runtime trace.
*******************************************************************************/
#define DBG_LEVEL_OFF      4
#define DBG_LEVEL_HIGH     3
#define DBG_LEVEL_NORMAL   2
#define DBG_LEVEL_LOW      1

#define CREATE_TRACE_GROUP(name) unsigned int G_nTraceGroup##name = DBG_LEVEL_HIGH
#define DECLARE_TRACE_GROUP(name) extern unsigned int G_nTraceGroup##name
#define PRINTF printk
#define TRACE_MPS(name, level, message) do { if (level >= G_nTraceGroup##name) \
      {PRINTF message ; } } while (0)

#endif  /* _DRV_MPS_VMMC_DBG_H */

