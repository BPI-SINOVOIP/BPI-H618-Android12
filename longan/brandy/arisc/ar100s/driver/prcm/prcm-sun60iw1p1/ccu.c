/*
*********************************************************************************************************
*                                                AR100 SYSTEM
*                                     AR100 Software System Develop Kits
*                                         clock control unit module
*
*                                    (c) Copyright 2012-2016, Sunny China
*                                             All Rights Reserved
*
* File    : ccu.c
* By      : Sunny
* Version : v1.0
* Date    : 2012-5-7
* Descript: clock control unit module.
* Update  : date                auther      ver     notes
*           2012-5-7 8:43:10    Sunny       1.0     Create this file.
*********************************************************************************************************
*/

#include "ccu_i.h"
#include "cpucfg_regs.h"

/* apb clock change notifier list */
struct notifier *apbs2_notifier_head;
u32 iosc_freq = 16000000;
u32 losc_freq = 32768;

volatile static u32 already_init_osc_freq;

void iosc_freq_init(void)
{
	u64 cnt1, cnt2;
	u32 xt, ht, xf;
	u32 cpus_src;

	if (already_init_osc_freq == 0) {
		cpus_src = readl(CPUS_CFG_REG) & CPUS_CLK_SRC_SEL_MASK;
		/* set cpus clock source to hosc */
		writel((readl(CPUS_CFG_REG) & (~CPUS_CLK_SRC_SEL_MASK)) | CPUS_CLK_SRC_SEL(0), CPUS_CFG_REG);
		time_cdelay(1600);
		//cpucfg_counter_clear();
		cnt1 = cpucfg_counter_read();
		time_cdelay(1000000);
		cnt2 = cpucfg_counter_read() - cnt1;
		ht = ((u32)(cnt2 & 0xffffffff))/24000;

		/* set cpus clock source to iosc */
		writel((readl(CPUS_CFG_REG) & (~CPUS_CLK_SRC_SEL_MASK)) | CPUS_CLK_SRC_SEL(2), CPUS_CFG_REG);
		time_cdelay(1600);
		//cpucfg_counter_clear();
		cnt1 = cpucfg_counter_read();
		time_cdelay(1000000);
		cnt2 = cpucfg_counter_read() - cnt1;
		xt = ((u32)(cnt2 & 0xffffffff))/24000;
		xf = (24000 * ht)/xt;
		/* recovery cpus clock source */
		writel((readl(CPUS_CFG_REG) & (~CPUS_CLK_SRC_SEL_MASK)) | cpus_src, CPUS_CFG_REG);

		iosc_freq = xf * 1000;
		LOG("iosc_freq: %d\n", iosc_freq);
		losc_freq = iosc_freq/512;

		already_init_osc_freq = 1;
	}
}

/*
*********************************************************************************************************
*                                       INITIALIZE CCU
*
* Description:  initialize clock control unit.
*
* Arguments  :  none.
*
* Returns    :  OK if initialize ccu succeeded, others if failed.
*********************************************************************************************************
*/
s32 ccu_init(void)
{
#ifndef CFG_FPGA_PLATFORM
	/* setup cpus post div source to 200M */
	writel((readl(CPUS_CFG_REG) & (~CPUS_FACTOR_M_MASK)) | CPUS_FACTOR_M(0), CPUS_CFG_REG);

	/* set cpus clock source to pll_peri0 */
	writel((readl(CPUS_CFG_REG) & (~CPUS_CLK_SRC_SEL_MASK)) | CPUS_CLK_SRC_SEL(3), CPUS_CFG_REG);

	/* setup apbs0 post div source to 100M */
	writel((readl(APBS0_CFG_REG) & (~APBS0_FACTOR_M_MASK)) | APBS0_FACTOR_M(1), APBS0_CFG_REG);

	/* set apbs0 clock source to pll_peri0 */
	writel((readl(APBS0_CFG_REG) & (~APBS0_CLK_SRC_SEL_MASK)) | APBS0_CLK_SRC_SEL(3), APBS0_CFG_REG);

	/* setup apbs1 post div source to 24M */
	writel((readl(APBS1_CFG_REG) & (~APBS1_FACTOR_M_MASK)) | APBS1_FACTOR_M(0), APBS1_CFG_REG);

	/* set apbs1 clock source to 24M */
	writel((readl(APBS1_CFG_REG) & (~APBS1_CLK_SRC_SEL_MASK)) | APBS1_CLK_SRC_SEL(0), APBS1_CFG_REG);
#endif

	/* initialize apb notifier list */
	apbs2_notifier_head = NULL;

	/* ccu initialize succeeded */
	return OK;
}

/*
*********************************************************************************************************
*                                       EXIT CCU
*
* Description:  exit clock control unit.
*
* Arguments  :  none.
*
* Returns    :  OK if exit ccu succeeded, others if failed.
*********************************************************************************************************
*/
s32 ccu_exit(void)
{
	return OK;
}

void save_state_flag(u32 value)
{
	writel(value, RTC_RECORD_REG);
}

u32 read_state_flag(void)
{
	return readl(RTC_RECORD_REG);
}

u32 read_dtb_base(void)
{
	return readl(RTC_DTB_BASE_STORE_REG);
}

