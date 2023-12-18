/*
*********************************************************************************************************
*                                                AR100 SYSTEM
*                                     AR100 Software System Develop Kits
*                                                cpucfg module
*
*                                    (c) Copyright 2012-2016, Sunny China
*                                             All Rights Reserved
*
* File    : cpucfg.c
* By      : Sunny
* Version : v1.0
* Date    : 2012-5-7
* Descript: cpu config module.
* Update  : date                auther      ver     notes
*           2012-5-7 17:24:32   Sunny       1.0     Create this file.
*********************************************************************************************************
*/

#include "../cpucfg_i.h"

static void cpucfg_counter_init(void);
volatile s32 little_endian_used;
/*
*********************************************************************************************************
*                                       INITIALIZE CPUCFG
*
* Description:  initialize cpu configure module.
*
* Arguments  :  none.
*
* Returns    :  OK if initialize cpu configure succeeded, others if failed.
*********************************************************************************************************
*/
s32 cpucfg_init(void)
{
	cpucfg_counter_init();
	little_endian_used = 0;

	return OK;
}

/*
*********************************************************************************************************
*                                       EXIT CPUCFG
*
* Description:  exit cpu configure module.
*
* Arguments  :  none.
*
* Returns    :  OK if exit cpu configure succeeded, others if failed.
*********************************************************************************************************
*/
s32 cpucfg_exit(void)
{
	little_endian_used = 0;
	return OK;
}

/*
*********************************************************************************************************
*                                      SET LE ADDRESS SPACE
*
* Description:  set little-endian address space.
*
* Arguments  :  start   : the start address of little-endian space.
*               end     : the end address of little-endian space.
*
* Returns    :  OK if set little-endian succeeded, others if failed.
*********************************************************************************************************
*/
s32 cpucfg_set_little_endian_address(void *start, void *end)
{
	volatile u32 value;

	if (little_endian_used >= CPUCFG_LE_SPACE_NUMBER) {
		WRN("no space for little-endian config\n");
		return -ENOSPC;
	}

	/* config space0 as little-endian */
	value = readl(CPUS_SPACE_LE_CTRL_REG);
	value |= (0x1 << little_endian_used);
	writel(value, CPUS_SPACE_LE_CTRL_REG);

	/* config start address and end address */
	writel((volatile u32)start,
	       CPUS_SPACE_START_REG(little_endian_used));
	writel((volatile u32)end, CPUS_SPACE_END_REG(little_endian_used));

	/* the number of used little-endian space increase */
	little_endian_used++;

	/* set little-endian succeeded */
	return OK;
}

s32 cpucfg_remove_little_endian_address(void *start, void *end)
{
	int i;
	volatile u32 value;

	if (little_endian_used <= 0) {
		WRN("no space for little-endian remove\n");
		return -ENODEV;
	}

	for (i = little_endian_used; i; i--)
		if (readl(CPUS_SPACE_START_REG(i)) >> 9 == (int)start >> 9)
			break;
	INF("remove:%d\n", i);
	/* disable space0 as little-endian */
	value = readl(CPUS_SPACE_LE_CTRL_REG);
	value &= ~(0x1 << i);
	writel(value, CPUS_SPACE_LE_CTRL_REG);

	/* clean start address and end address */
	writel((volatile u32)0, CPUS_SPACE_START_REG(i));
	writel((volatile u32)0, CPUS_SPACE_END_REG(i));

	/* the number of used little-endian space decrease */
	little_endian_used--;

	/*remove little-endian succeeded */
	return OK;
}

/*
*********************************************************************************************************
*                                      CLEAR 64BIT COUNTER
*
* Description:  clear 64bit counter, after this operation,
*               the value of conuter will reset to zero.
*
* Arguments  :  none.
*
* Returns    :  OK if clear counter succeeded, others if failed.
*********************************************************************************************************
*/
s32 cpucfg_counter_clear(void)
{
	writel(0, CNT_LOW_REG_SET);
	writel(0, CNT_HIGH_REG_SET);

	return OK;
}

/*
*********************************************************************************************************
*                                      READ 64BIT COUNTER
*
* Description:  read 64bit counter, the counter value base on us.
*
* Arguments  :  none.
*
* Returns    :  the value of counter, base on us.
*********************************************************************************************************
*/
u64 cpucfg_counter_read(void)
{
	volatile u32 high;
	volatile u32 low;
	u64 counter;

	do {
		low = readl(CNT_LOW_REG);
		high = readl(CNT_HIGH_REG);
	} while ((high != readl(CNT_HIGH_REG))
		 || (low > readl(CNT_LOW_REG)));

	counter = high;
	counter = (counter << 32) + low;

	return counter;
}

void cpucfg_counter_ctrl(bool enable)
{
	writel(enable, CNT_CTRL_REG);
}

static void cpucfg_counter_init(void)
{
	writel(CCU_HOSC_FREQ, CNT_FREQID_REG);
}
