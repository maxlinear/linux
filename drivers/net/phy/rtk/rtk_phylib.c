/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (c) 2023 Realtek Semiconductor Corp. All rights reserved.
 */

#include "rtk_phylib.h"
#include <linux/phy.h>


/* OSAL */

void rtk_phylib_mdelay(uint32 msec)
{
#if defined(RTK_PHYDRV_IN_LINUX)
    mdelay(msec);
#else
    osal_time_mdelay(msec);
#endif
}


void rtk_phylib_udelay(uint32 usec)
{
#if defined(RTK_PHYDRV_IN_LINUX)
    if (1000 <= usec)
    {
        mdelay(usec/1000);
        usec = usec % 1000;
    }
    udelay(usec);
#else
    osal_time_udelay(usec);
#endif
}

int32 rtk_phylib_time_usecs_get(uint32 *pUsec)
{
    struct timespec64 ts;

    if(NULL == pUsec)
        return RTK_PHYLIB_ERR_INPUT;

    ktime_get_ts64(&ts);
    *pUsec = ((ts.tv_sec * USEC_PER_SEC) + (ts.tv_nsec / NSEC_PER_USEC));
    return 0;
}

/* Register Access APIs */
int32 rtk_phylib_mmd_write(rtk_phydev *phydev, uint32 mmd, uint32 reg, uint8 msb, uint8 lsb, uint32 data)
{
    int32  ret = 0;
    uint32 mask = 0;
    mask = UINT32_BITS_MASK(msb,lsb);

#if defined(RTK_PHYDRV_IN_LINUX)
    ret = phy_modify_mmd(phydev, mmd, reg, mask, (data << lsb));
#else
    {
        uint32 rData = 0, wData = 0;
        if ((msb != 15) || (lsb != 0))
        {
            if ((ret = phy_common_general_reg_mmd_get(phydev->unit, phydev->port, page, reg, &rData)) != RT_ERR_OK)
                return ret;
        }
        wData = REG32_FIELD_SET(rData, data, lsb, mask);
        ret = phy_common_general_reg_mmd_set(phydev->unit, phydev->port, page, reg, wData);
    }
#endif

    return ret;
}

int32 rtk_phylib_mmd_read(rtk_phydev *phydev, uint32 mmd, uint32 reg, uint8 msb, uint8 lsb, uint32 *pData)
{
    int32  ret = 0;
    uint32 rData = 0;
    uint32 mask = 0;
    mask = UINT32_BITS_MASK(msb,lsb);

#if defined(RTK_PHYDRV_IN_LINUX)
    rData =  phy_read_mmd(phydev, mmd, reg);
#else
    {
        ret = phy_common_general_reg_mmd_get(phydev->unit, phydev->port, page, reg, &rData);
    }
#endif

    *pData = REG32_FIELD_GET(rData, lsb, mask);
    return ret;
}

/* Function Driver */

int32 rtk_phylib_c45_power_normal(rtk_phydev *phydev)
{
    int32  ret = 0;

    RTK_PHYLIB_ERR_CHK(rtk_phylib_mmd_write(phydev, 1, 0, 11, 11, 0));
    rtk_phylib_mdelay(50);

    /* Enable serdes after PHY status become normal */
    RTK_PHYLIB_ERR_CHK(rtk_phylib_mmd_write(phydev, MDIO_MMD_VEND1, 0x141,
					    15, 0, 0xC0));
    RTK_PHYLIB_ERR_CHK(rtk_phylib_mmd_write(phydev, MDIO_MMD_VEND1, 0x143,
					    15, 0, 0x88a0));

    return 0;
}

int32 rtk_phylib_c45_power_low(rtk_phydev *phydev)
{
    int32  ret = 0;

    /* disable serdes before PHY link down to stop traffic from MAC to PHY */
    RTK_PHYLIB_ERR_CHK(rtk_phylib_mmd_write(phydev, MDIO_MMD_VEND1, 0x141,
					    15, 0, 0x40));
    RTK_PHYLIB_ERR_CHK(rtk_phylib_mmd_write(phydev, MDIO_MMD_VEND1, 0x143,
					    15, 0, 0x88a0));

    rtk_phylib_mdelay(50);
    RTK_PHYLIB_ERR_CHK(rtk_phylib_mmd_write(phydev, 1, 0, 11, 11, 1));

    return 0;
}

int32 rtk_phylib_c45_pcs_loopback(rtk_phydev *phydev, uint32 enable)
{
    int32  ret = 0;
    RTK_PHYLIB_ERR_CHK(rtk_phylib_mmd_write(phydev, 3, 0, 14, 14, (enable == 0) ? 0 : 1));

    return 0;
}
