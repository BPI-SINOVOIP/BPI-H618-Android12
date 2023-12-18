/*
*********************************************************************************************************
*                                                AR100 SYSTEM
*                                     AR100 Software System Develop Kits
*                                               standby module
*
*                                    (c) Copyright 2012-2016, Sunny China
*                                             All Rights Reserved
*
* File    : standby.h
* By      : Sunny
* Version : v1.0
* Date    : 2012-5-13
* Descript: standby module public header.
* Update  : date                auther      ver     notes
*           2012-5-13 15:32:01  Sunny       1.0     Create this file.
*********************************************************************************************************
*/

#ifndef __STANDBY_H__
#define __STANDBY_H__

#include "include.h"

/* the wakeup source of main cpu: cpu0 */
#define CPU0_WAKEUP_MSGBOX      (1 << 0)  /* external interrupt, pmu event for ex.    */
#define CPU0_WAKEUP_KEY         (1 << 1)  /* key event    */
#define CPU0_WAKEUP_EXINT       (1 << 2)
#define CPU0_WAKEUP_IR          (1 << 3)
#define CPU0_WAKEUP_ALARM       (1 << 4)
#define CPU0_WAKEUP_USB         (1 << 5)
#define CPU0_WAKEUP_TIMEOUT     (1 << 6)
#define CPU0_WAKEUP_PIO         (1 << 7)

/* the wakeup source of assistant cpu: cpus */
#define CPUS_WAKEUP_HDMI_CEC    (1 << 11)
#define CPUS_WAKEUP_LOWBATT     (1 << 12)
#define CPUS_WAKEUP_USB         (1 << 13)
#define CPUS_WAKEUP_AC          (1 << 14)
#define CPUS_WAKEUP_ASCEND      (1 << 15)
#define CPUS_WAKEUP_DESCEND     (1 << 16)
#define CPUS_WAKEUP_SHORT_KEY   (1 << 17)
#define CPUS_WAKEUP_LONG_KEY    (1 << 18)
#define CPUS_WAKEUP_IR          (1 << 19)
#define CPUS_WAKEUP_ALM0        (1 << 20)
#define CPUS_WAKEUP_ALM1        (1 << 21)
#define CPUS_WAKEUP_TIMEOUT     (1 << 22)
#define CPUS_WAKEUP_GPIO        (1 << 23)
#define CPUS_WAKEUP_USBMOUSE    (1 << 24)
#define CPUS_WAKEUP_LRADC       (1 << 25)
/* null for 1<<26 */
#define CPUS_WAKEUP_CODEC       (1 << 27)
#define CPUS_WAKEUP_BAT_TEMP    (1 << 28)
#define CPUS_WAKEUP_FULLBATT    (1 << 29)
#define CPUS_WAKEUP_HMIC        (1 << 30)
#define CPUS_WAKEUP_POWER_EXP   (1 << 31)
#define CPUS_WAKEUP_KEY         (CPUS_WAKEUP_SHORT_KEY | CPUS_WAKEUP_LONG_KEY)

/* macro for axp wakeup source event */
#define CPUS_WAKEUP_NMI     (CPUS_WAKEUP_LOWBATT   | \
		CPUS_WAKEUP_USB       | \
		CPUS_WAKEUP_AC        | \
		CPUS_WAKEUP_ASCEND    | \
		CPUS_WAKEUP_DESCEND   | \
		CPUS_WAKEUP_SHORT_KEY | \
		CPUS_WAKEUP_LONG_KEY  | \
		CPUS_WAKEUP_BAT_TEMP  | \
		CPUS_WAKEUP_FULLBATT)


#ifdef CFG_STANDBY_SERVICE
/*
*********************************************************************************************************
*                                       INIT STANDBY
*
* Description:  initialize standby module.
*
* Arguments  :  none.
*
* Returns    :  OK if initialize succeeded, others if failed.
*********************************************************************************************************
*/
s32 standby_init(void);

/*
*********************************************************************************************************
*                                       ENTEY OF SETING DRAM CRC PARAS
*
* Description:  set ar100 debug dram crc paras.
*
* Arguments  :  en      :  dram crc enable or disable;
*               srcaddr :  source address of dram crc area
*               len     :  lenght of dram crc area
*
* Returns    :  OK if set ar100 debug dram crc paras successed, others if failed.
*********************************************************************************************************
*/
s32 long_jump(int (*fn)(void *arg), void *arg);
s32 standby_set_dram_crc_paras(u32 enable, u32 src, u32 len);

u32 is_suspend_lock(void);
s32 cpu_op(struct message *pmessage);
s32 sys_op(struct message *pmessage);
s32 fake_poweroff(struct message *pmessage);
#else
static inline u32 standby_init(void) { return -1; }
static inline s32 long_jump(int (*fn)(void *arg), void *arg) { return -1; }
static inline s32 standby_set_dram_crc_paras(u32 enable, u32 src, u32 len) { return -1; }
static inline u32 is_suspend_lock(void) { return -1; }
static inline s32 cpu_op(struct message *pmessage) { return -1; }
static inline s32 sys_op(struct message *pmessage) { return -1; }
static inline s32 fake_poweroff(struct message *pmessage) { return -1; }
#endif

#endif /* __STANDBY_H__ */
