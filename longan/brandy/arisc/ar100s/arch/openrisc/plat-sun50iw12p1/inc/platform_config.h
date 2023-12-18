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

/* uart config */
#define UART_BAUDRATE                   (115200 / 2)


/* amp msgbox config */
#define AMP_ARM				(0)
#define AMP_RISC			(1)
#define AMP_MIPS			(2)
#define AMP_LOCAL			(AMP_RISC)
#define AMP_PSCI			(AMP_ARM)

#define MSGBOX_NUM			(3)
#define CHANNEL_MAX			(4)

/* define the specify channel to recieve power manage command from arm*/
#define CHANNEL_PSCI			(3)

/* devices define */
#define ARISC_DTS_BASE (0x44000000)
#define ARISC_DTS_SIZE (0x00100000)

#endif /* __SUN50IW2_CFGS_H__ */
