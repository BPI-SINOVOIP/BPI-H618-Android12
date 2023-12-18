/*
*********************************************************************************************************
*                                                AR100 SYSTEM
*                                     AR100 Software System Develop Kits
*                                         clock control unit module
*
*                                    (c) Copyright 2012-2016, Sunny China
*                                             All Rights Reserved
*
* File    : mclk.c
* By      : Superm
* Version : v1.0
* Date    : 2016-7-13
* Descript: module clock management
* Update  : date                auther      ver     notes
*           2016-7-13 8:43:10   Superm     1.0     Create this file.
*********************************************************************************************************
*/

#include "ccu_i.h"

s32 calc_clk_ratio_nm(u32 div, u32 *n, u32 *m)
{
	if (div < 33) {
		*n = 0;
		*m = div;
	} else if (div < 65) {
		*n = 1;
		*m = (div + 1) / 2;
	} else if (div < 129) {
		*n = 2;
		*m = (div + 3) / 4;
	} else if (div < 257) {
		*n = 3;
		*m = (div + 7) / 8;
	} else {
		ERR("icd %x\n", div);
		return -EINVAL;
	}

	return OK;
}

/*
*********************************************************************************************************
*                                    SET ON-OFF STATUS OF MODULE CLOCK
*
* Description:  set the on-off status of a specific module clock.
*
* Arguments  :  mclk    : the module clock ID which we want to set on-off status.
*               onoff   : the on-off status which we want to set, the detail please
*                         refer to the clock status of on-off.
*
* Returns    :  OK if set module clock on-off status succeeded, others if failed.
*********************************************************************************************************
*/
s32 ccu_set_mclk_onoff(u32 mclk, s32 onoff)
{
	u32 value;

	switch (mclk) {
	case CCU_MOD_CLK_R_TWI:
		{
			value = readl(R_TWI_BUS_GATE_RST_REG);
			value &= (~(1 << 0));
			value |= (onoff << 0);
			writel(value, R_TWI_BUS_GATE_RST_REG);
			return OK;
		}
	case CCU_MOD_CLK_R_UART:
		{
			value = readl(R_UART_BUS_GATE_RST_REG);
			value &= (~(1 << 0));
			value |= (onoff << 0);
			writel(value, R_UART_BUS_GATE_RST_REG);
			return OK;
		}
	case CCU_MOD_CLK_R_TIMER0_1:
		{
			value = readl(R_TIMER_BUS_GATE_RST_REG);
			value &= (~(1 << 0));
			value |= (onoff << 0);
			writel(value, R_TIMER_BUS_GATE_RST_REG);
			return OK;
		}
	case CCU_MOD_CLK_R_RTC:
		{
			value = readl(RTC_BUS_GATE_RST_REG);
			value &= (~(1 << 0));
			value |= (onoff << 0);
			writel(value, RTC_BUS_GATE_RST_REG);
			return OK;
		}
	case CCU_MOD_CLK_MSGBOX:
		{
			writel(((readl(CCU_MSGBOX_BGR_REG) & (~(0x1 << 0)))
				| (onoff << 0)), CCU_MSGBOX_BGR_REG);
			writel(((readl(R_MSGBOX_GATE_RST_REG) & (~(0x1 << 0)))
				| (onoff << 0)), R_MSGBOX_GATE_RST_REG);
			return OK;
		}
	case CCU_MOD_CLK_APBS_R_CIR:
		{
			writel(((readl(R_IR_RX_BUS_GATE_RST_REG) & (~(0x1 << 0)))
				| (onoff << 0)), R_IR_RX_BUS_GATE_RST_REG);
			return OK;
		}
	case CCU_MOD_CLK_R_CIR:
		{
			writel(((readl(R_IR_RX_CLOCK_REG) & (~(0x1 << 31)))
				| (onoff << 31)), R_IR_RX_CLOCK_REG);
			return OK;
		}
	default:
		{
			WRN("invaid module clock id (%d) when set divider\n", mclk);
			return -EINVAL;
		}
	}
	/* un-reached */
}


/*
*********************************************************************************************************
*                                    SET SOURCE OF MODULE CLOCK
*
* Description:  set the source of a specific module clock.
*
* Arguments  :  mclk    : the module clock ID which we want to set source.
*               sclk    : the source clock ID whick we want to set as source.
*
* Returns    :  OK if set source succeeded, others if failed.
*********************************************************************************************************
*/
s32 ccu_set_mclk_src(u32 mclk, u32 sclk)
{
	u32 value;

	switch (mclk) {
	case CCU_MOD_CLK_R_CIR:
		{
			switch (sclk) {
			case CCU_SYS_CLK_LOSC:
				{
					value = readl(R_IR_RX_CLOCK_REG);
					value &= (~(1 << 24));
					value |= (0 << 24);
					writel(value, R_IR_RX_CLOCK_REG);
					return OK;
				}
			case CCU_SYS_CLK_HOSC:
				{
					value = readl(R_IR_RX_CLOCK_REG);
					value &= (~(1 << 24));
					value |= (1 << 24);
					writel(value, R_IR_RX_CLOCK_REG);
					return OK;
				}
			default:
				{
					//invalid source id for module clock
					return -EINVAL;
				}
			}
		}
	default:
		{
			WRN("invaid module clock id (%d) when set source\n", mclk);
			return -EINVAL;
		}
	}
	//un-reached
}

/*
*********************************************************************************************************
*                                    GET SOURCE OF MODULE CLOCK
*
* Description:  get the source of a specific module clock.
*
* Arguments  :  mclk    : the module clock ID which we want to get source.
*
* Returns    :  the source clock ID of source clock.
*********************************************************************************************************
*/
s32 ccu_get_mclk_src(u32 mclk)
{
	switch (mclk) {
	case CCU_MOD_CLK_R_CIR:
		{
			switch ((readl(R_IR_RX_CLOCK_REG) & (1 << 24)) >> 24) {
			case 0:
				{
					return CCU_SYS_CLK_LOSC;
				}
			case 1:
				{
					return CCU_SYS_CLK_HOSC;
				}
			default:
				{
					//invalid source id for module clock
					return CCU_SYS_CLK_NONE;
				}
			}
		}
	default:
		{
			WRN("invaid module clock id (%d) when set source\n", mclk);
			return CCU_SYS_CLK_NONE;
		}
	}
	//un-reached
}

/*
*********************************************************************************************************
*                                    SET DIVIDER OF MODULE CLOCK
*
* Description:  set the divider of a specific module clock.
*
* Arguments  :  mclk    : the module clock ID which we want to set divider.
*               div     : the divider whick we want to set as source.
*
* Returns    :  OK if set divider succeeded, others if failed.
*********************************************************************************************************
*/
s32 ccu_set_mclk_div(u32 mclk, u32 div)
{
	u32 value;

	switch (mclk) {
	case CCU_MOD_CLK_R_CIR:
		{
			u32 factor_n;
			u32 factor_m;

			if (calc_clk_ratio_nm(div, &factor_n, &factor_m) != OK) {
				return -EFAIL;
			}
			value = readl(R_IR_RX_CLOCK_REG);
			value &= (~(0x1f << 0));
			value &= (~(0x3 << 8));
			value |= ((factor_m - 1) << 0);
			value |= (factor_n << 8);
			writel(value, R_IR_RX_CLOCK_REG);
			return OK;
		}
	default:
		{
			WRN("invaid module clock id (%d) when set divider\n", mclk);
			return -EINVAL;
		}
	}
	//un-reached
}

/*
*********************************************************************************************************
*                                    GET DIVIDER OF MODULE CLOCK
*
* Description:  get the divider of a specific module clock.
*
* Arguments  :  mclk    : the module clock ID which we want to get divider.
*
* Returns    :  the divider of the specific module clock.
*********************************************************************************************************
*/
s32 ccu_get_mclk_div(u32 mclk)
{
	u32 value;

	switch (mclk) {
	case CCU_MOD_CLK_R_CIR:
		{
			u32 factor_n;
			u32 factor_m;

			value = readl(R_IR_RX_CLOCK_REG);
			factor_m = (value & (0x1f << 0)) >> 0;
			factor_n = (value & (0x3 << 8)) >> 8;
			return (factor_m + 1) * (1 << factor_n);
		}
	default:
		{
			WRN("invaid module clock id (%d) when get divider\n", mclk);
			return -EINVAL;
		}
	}
	//un-reached
}
