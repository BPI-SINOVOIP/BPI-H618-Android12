/*
 * include/cfgs.h
 *
 * Descript: system configure header.
 * Copyright (C) 2012-2016 AllWinnertech Ltd.
 * Author: superm <superm@allwinnertech.com>
 *
 */
#ifndef __PLATFORM_CFGS_H__
#define __PLATFORM_CFGS_H__


#define TWI_CLOCK_FREQ                  (200 * 1000)	/* the twi source clock freq */
#define TICK_PER_SEC                    (100)

#define RSB_RTSADDR_AXP2202		(pmu_runtime_addr)
#define RSB_RTSADDR_AXP2202_B	(0x34)
#define RSB_RTSADDR_AXP2202_C	(0x35)

#define RSB_RTSADDR_TCS4838		(0x41)
#define RSB_RTSADDR_SY8827G		(0x60)
#define RSB_RTSADDR_AXP1530		(0x36)
/* uart config */
#define UART_BAUDRATE                   (115200 / 2)

/* devices define */
#define ARISC_DTS_SIZE (0x00100000)

#endif
