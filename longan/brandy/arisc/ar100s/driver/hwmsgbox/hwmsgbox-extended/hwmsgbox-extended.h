/**
 * driver\hwmsgbox-extended\hwmsgbox-extended.h
 *
 * Descript: new hardware message-box internal header.
 * Copyright (C) 2012-2016 AllWinnertech Ltd.
 * Author: Sunny <Sunny@allwinnertech.com>
 *
 */

#ifndef __HWMSGBOX_EXTENDED_H__
#define __HWMSGBOX_EXTENDED_H__

#include "include.h"
/* hardware message-box register list */
#define CPUX_MSGBOX_REG_BASE_EXT                (CPUX_HWMSGBOX_REG_BASE)
#define CPUS_MSGBOX_REG_BASE_EXT                (CPUS_HWMSGBOX_REG_BASE)
#define MSGBOX_ARM_TO_RISC_IRQ_STATUS_REG       (CPUS_MSGBOX_REG_BASE_EXT + 0x24)
#define MSGBOX_ARM_TO_RISC_MSG_STATUS_REG(m)    (CPUS_MSGBOX_REG_BASE_EXT + 0x60 + (0x4 * m))
#define MSGBOX_RISC_TO_ARM_MSG_STATUS_REG(m)    (CPUX_MSGBOX_REG_BASE_EXT + 0x60 + (0x4 * m))
#define MSGBOX_ARM_TO_RISC_MSG_REG(m)           (CPUS_MSGBOX_REG_BASE_EXT + 0x70 + (0x4 * m))
#define MSGBOX_RISC_TO_ARM_MSG_REG(m)           (CPUX_MSGBOX_REG_BASE_EXT + 0x70 + (0x4 * m))

/* the channel of hardware massage queue*/
#define HWMSGBOX_RISC_ASYN_TX_CH       (3)	/* hwmsgbox channels configure */
#define HWMSGBOX_RISC_ASYN_RX_CH       (3)
#define HWMSGBOX_RISC_SYN_TX_CH        (3)
#define HWMSGBOX_RISC_SYN_RX_CH        (3)
#define HWMSGBOX_ARM_SYN_TX_CH         (3)
#define HWMSGBOX_ARM_SYN_RX_CH         (3)

#endif /*__HWMSGBOX_EXTENDED_H__*/
