/**
 * include\driver\hwspinlock.h
 *
 * Descript: include\driver\hwmsgbox.h
 * Copyright (C) 2012-2016 AllWinnertech Ltd.
 * Author: Sunny <Sunny@allwinnertech.com>
 *
 */

#ifndef __HWMSGBOX_H__
#define __HWMSGBOX_H__

#include "stdint.h"
/*
*********************************************************************************************************
*                                           INITIALIZE HWMSGBOX
*
* Description:  initialize hwmsgbox.
*
* Arguments  :  none.
*
* Returns    :  OK if initialize hwmsgbox succeeded, others if failed.
*********************************************************************************************************
*/
s32 hwmsgbox_init(void);

/*
*********************************************************************************************************
*                                           EXIT HWMSGBOX
*
* Description:  exit hwmsgbox.
*
* Arguments  :  none.
*
* Returns    :  OK if exit hwmsgbox succeeded, others if failed.
*********************************************************************************************************
*/
s32 hwmsgbox_exit(void);

/*
*********************************************************************************************************
*                                       SEND MESSAGE BY HWMSGBOX
*
* Description:  send one message to another processor by hwmsgbox.
*
* Arguments  :  pmessage    : the pointer of sended message frame.
*               timeout     : the wait time limit when message fifo is full,
*                             it is valid only when parameter mode = SEND_MESSAGE_WAIT_TIMEOUT.
*
* Returns    :  OK if send message succeeded, other if failed.
*********************************************************************************************************
*/
s32 hwmsgbox_send_message(struct message *pmessage, u32 timeout);


/*
*********************************************************************************************************
*                                        QUERY MESSAGE
*
* Description:  query message of hwmsgbox by hand, mainly for.
*
* Arguments  :  none.
*
* Returns    :  the point of message, NULL if timeout.
*********************************************************************************************************
*/
s32 hwmsgbox_query_message(struct message *pmessage, u32 timeout);

int hwmsgbox_feedback_message(struct message *pmessage, u32 timeout);

s32 hwmsgbox_super_standby_init(void);
s32 hwmsgbox_super_standby_exit(void);

struct amp_msg {
	uint32_t local_amp;
	uint32_t remote_amp;
	uint32_t write_ch;
	uint32_t read_ch;
};

#ifdef CFG_HW_MSGBOX_AMP_USED
/*
*********************************************************************************************************
*                                           CALCAULTE N
* Description:  calculate factor n which is used to select magbox base offset address.
*
* Arguments  :  local:   local msgbox number.
*               remote:  remote msgbox number.
*
* Returns    :  factor n that use to select magbox base offset address.
*********************************************************************************************************
*/
extern int calculte_n(int local, int remote);


/*
*********************************************************************************************************
*                                           AMP_MSGBOX_REGISTER_SERVICE
* Description:  register a callback function into msg_rec_list.
*
* Arguments  :  pcb:    the callback function that need to register .
*
* Returns    :  OK if callback func is registered, ENOSPC if other.
*********************************************************************************************************
*/
extern s32 amp_msgbox_register_service(__pNotifier_t pcb);

extern s32 amp_msgbox_init(void);

/*
*********************************************************************************************************
*                                           AMP_MSGBOX_QUERY_MESSAGE
* Description:  query message that was send from other remote cpu, except power manage command message
*               send by PSCI channel.
*
* Arguments  :  none.
*
* Returns    :  OK.
*********************************************************************************************************
*/
extern s32 amp_msgbox_query_message(void);

/*
*********************************************************************************************************
*                                           AMP_MSGBOX_CHANNEL_SEND_DATA
* Description:  send data to remote cpu in some specify channel.
*
* Arguments  :  *msg:   an amp_msg pointer which can describe msgbox number and channel.
*               data:   data that need to be sned.
*
* Returns    :  none.
*********************************************************************************************************
*/
extern s32 amp_msgbox_channel_send_data(struct amp_msg *msg, u32 data, u32 timeout);

/*
*********************************************************************************************************
*                                           AMP_MSGBOX_CHANNEL_SEND_MESSAGE
* Description:  send data from local cpu to remote cpu, whole data size is len * 8bit. the data that need
*               to send should be define as a series of u8 type such as an array.
*
* Arguments  :  *msg:   an amp_msg pointer which can describe msgbox number and channel.
*               *buff:  data buff that need to be sned.
*               len:    indicate the u8 type data number that whole include.
*
* Returns    :  none.
*********************************************************************************************************
*/
extern s32 amp_msgbox_send_message(struct amp_msg *msg, u8 *buff, int len, u32 timeout);
extern u32 amp_msgbox_read_message(u32 remote, u32 channel);
extern u32 amp_msgbox_remote_fifo_is_empty(u32 remote, u32 channel);
extern u32 amp_msgbox_remote_fifo_is_full(u32 remote, u32 channel);
#else
static inline int calculte_n(int local, int remote) { return -1; }
static inline s32 amp_msgbox_init(void) { return -1; }
static inline s32 amp_msgbox_register_service(__pNotifier_t pcb) { return -1; }
static inline void amp_msgbox_channel_send_data(struct amp_msg *msg, u32 data, u32 timeout) { return; }
static inline s32 amp_msgbox_query_message(void) { return -1; }
static inline s32 amp_msgbox_send_message(struct amp_msg *msg, u8 *buff, int len, u32 timeout) { return -1; }
static inline u32 amp_msgbox_read_message(u32 remote, u32 channel) { return -1; }
static inline u32 amp_msgbox_remote_fifo_is_empty(u32 remote, u32 channel) { return -1; }
static inline u32 amp_msgbox_remote_fifo_is_full(u32 remote, u32 channel) { return -1; }
#endif

#endif /* __HWMSGBOX_H__ */
