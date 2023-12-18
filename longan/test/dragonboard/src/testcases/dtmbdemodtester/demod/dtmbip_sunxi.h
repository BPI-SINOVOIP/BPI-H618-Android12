/* DTMBIP_sunxi.h
 *
 * Copyright (c)    2021 Allwinnertech Co., Ltd.
 *                    2021 Zepeng Wu
 *
 * @ DTMBIP driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 */

#ifndef __DTMBIP_SUNXI_H
#define __DTMBIP_SUNXI_H

#include <sys/types.h>
#include <sys/ioctl.h>
#include "DataType.h"

#define DTMBIP_DEV	"/dev/dtmbip"

struct cache_range {
    long long start;
    long long end;
};

struct dtmb_regsetting {
	UINT16 reg;
	UINT8  val;
};

struct dtmb_user_iommu_param {
    int             fd;
    char            memsize;
    unsigned int    iommu_addr;
};

/* this cmd is  unused now */
#define DTMBIP_CMD_HWRESET
#define DTMBIP_CMD_SFRESET
#define DTMBIP_CMD_INIT
#define DTMBIP_CMD_SETWORKMODE
#define DTMBIP_CMD_SETTUNERTYPE
#define DTMBIP_CMD_SETTSFORMAT
#define DTMBIP_CMD_SETAUTOMODE
#define DTMBIP_CMD_SETFREQUENCY
#define DTMBIP_CMD_SETMANUALMODE
#define DTMBIP_CMD_SETMANUALFREQUENCY
#define DTMBIP_CMD_GETPARAMETERS
#define DTMBIP_CMD_GETQAMMODE
#define DTMBIP_CMD_CHECKDEMODLOCKED
#define DTMBIP_CMD_SETNCO
#define DTMBIP_CMD_GETNCO
#define DTMBIP_CMD_SETTIMEFRE
#define DTMBIP_CMD_SETSPECTRUM
#define DTMBIP_CMD_GETSNRMC
#define DTMBIP_CMD_GETSNRSC
#define DTMBIP_CMD_GETSNR
#define DTMBIP_CMD_GETLDPCBER
#define DTMBIP_CMD_GETBER
#define DTMBIP_CMD_GETFIELDSTRENGTH
/* end of unused */

#define DTMBIP_CMD_WRITEREG             _IOW('a', 1, struct dtmb_regsetting)
#define DTMBIP_CMD_READREG              _IOR('a', 2, struct dtmb_regsetting)
#define DTMBIP_CMD_SETAXIBUS            _IO('a', 3)
#define DTMBIP_CMD_MALLOC_IOMMU_ADDR    _IOR('a', 4, struct dtmb_user_iommu_param)
#define DTMBIP_CMD_FREE_IOMMU_ADDR      _IOW('a', 5, struct dtmb_user_iommu_param)
#define DTMBIP_CMD_FLUSH_CACHE_RANGE    _IOR('a', 6, struct cache_range)
#define DTMBIP_CMD_ADC_STANDBY          _IO('a', 7)
#define DTMBIP_CMD_ADC_WORKON           _IO('a', 8)
#define DTMBIP_CMD_FREE_AXIBUS          _IO('a', 9)
#define DTMBIP_CMD_SETADC4DTMB          _IO('a', 10)
#define DTMBIP_CMD_RESETADC4DTMB        _IO('a', 11)
#endif /* __DTMBIP_SUNXI_H */
