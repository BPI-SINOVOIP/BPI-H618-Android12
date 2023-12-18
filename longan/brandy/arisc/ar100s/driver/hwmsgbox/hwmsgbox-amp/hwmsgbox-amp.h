/**
 * driver\hwmsgbox\hwmsgbox-amp\hwmsgbox-amp.h
 *
 * Descript: general message-box internal header.
 * Copyright (C) 2012-2016 AllWinnertech Ltd.
 * Author: AWA1442 <huangyongxing@allwinnertech.com>
 *
 */

#ifndef __HWMSGBOX_AMP_H__
#define __HWMSGBOX_AMP_H__

#include "include.h"

#define MSGBOX_MAX_QUEUE 8

#define AMP_MSGBOX_BASE						(HWMSGBOX_REG_BASE)
#define AMP_MSGBOX_VER_REG(local, remote)			(MSGBOX_VER_REG(local, remote))
#define AMP_MSGBOX_DEBUG_REG(local, remote)			(MSGBOX_MSG_DEBUG_REG(local, remote))
#define AMP_MSGBOX_FIFO_STA_REG(local, remote, channel)		(MSGBOX_FIFO_STA_REG(local, remote, channel))
#define AMP_MSGBOX_MSG_STA_REG(local, remote, channel)		(MSGBOX_MSG_STA_REG(local, remote, channel))
#define AMP_MSGBOX_MSG_REG(local, remote, channel)		(MSGBOX_MESG_REG(local, remote, channel))

#endif /*__HWMSGBOX_AMP_H__*/
