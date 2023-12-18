/*
*********************************************************************************************************
*                                                AR100 SYSTEM
*                                     AR100 Software System Develop Kits
*                                               interrupt manager
*
*                                    (c) Copyright 2012-2016, Sunny China
*                                             All Rights Reserved
*
* File    : interrupt_manager.c
* By      : Sunny
* Version : v1.0
* Date    : 2012-5-3
* Descript: the manager of interrupt.
* Update  : date                auther      ver     notes
*           2012-5-3 10:45:15   Sunny       1.0     Create this file.
*********************************************************************************************************
*/

#include "intc_i.h"

struct int_isr_node isr_table[IRQ_SOUCE_MAX];

/*
*********************************************************************************************************
*                                       INIT INTERRUPT MANAGER
*
* Description:  initialize interrupt manager.
*
* Arguments  :  none.
*
* Returns    :  OK if initialize interrupt manager succeeded, others if failed.
*********************************************************************************************************
*/
s32 interrupt_init(void)
{
	s32 index;

	/*initialize interrupt controller */
	intc_init();

	/*initialize ISR table */
	for (index = 0; index < IRQ_SOUCE_MAX; index++) {
		isr_table[index].pisr = isr_default;
		isr_table[index].parg = NULL;
	}

	/*interrupt manager initialize succeeded */
	return OK;
}

/*
*********************************************************************************************************
*                                       EXIT INTERRUPT MANAGER
*
* Description:  exit interrupt manager.
*
* Arguments  :  none.
*
* Returns    :  OK if exit interrupt manager succeeded, others if failed.
*********************************************************************************************************
*/
s32 interrupt_exit(void)
{
	intc_exit();

	return OK;
}

/*
*********************************************************************************************************
*                                           ENABLE INTERRUPT
*
* Description:  enable a specific interrupt.
*
* Arguments  :  intno : the number of interrupt which we want to enable.
*
* Returns    :  OK if enable interrupt succeeded, others if failed.
*********************************************************************************************************
*/
s32 interrupt_enable(u32 intno)
{
	return intc_enable_interrupt(intno);
}

/*
*********************************************************************************************************
*                                           DISABLE INTERRUPT
*
* Description:  disable a specific interrupt.
*
* Arguments  :  intno : the number of interrupt which we want to disable.
*
* Returns    :  OK if disable interrupt succeeded, others if failed.
*********************************************************************************************************
*/
s32 interrupt_disable(u32 intno)
{
	return intc_disable_interrupt(intno);
}

/*
*********************************************************************************************************
*                                           SET NMI TRIGGER
*
* Description:  set nmi trigger.
*
* Arguments  :  type : the trigger type.
*
* Returns    :  OK if set trigger type succeeded, others if failed.
*********************************************************************************************************
*/
s32 interrupt_set_nmi_trigger(u32 type)
{
	u32 value;

	pintc_regs->control = type;

	/*mask cpus nmi irq */
	value = pintc_regs->mask;
	value |= 0x1;
	pintc_regs->mask = value;

	return OK;
}

s32 interrupt_set_mask(u32 intno, u32 mask)
{
	return intc_set_mask(intno, mask);
}

s32 interrupt_set_group_config(u32 grp_irq_num, u32 mask)
{
	return intc_set_group_config(grp_irq_num, mask);
}

/*
*********************************************************************************************************
*                                           INSTALL ISR
*
* Description:  install ISR for a specific interrupt.
*
* Arguments  :  intno   : the number of interrupt which we want to install ISR.
*       pisr    : the ISR which to been install.
*       parg    : the argument for the ISR.
*
* Returns    :  OK if install ISR succeeded, others if failed.
*
* Note       :  the ISR execute entironment : CPU disable interrupt response.
*********************************************************************************************************
*/
s32 install_isr(u32 intno, __pISR_t pisr, void *parg)
{
	/*intno can't beyond then IRQ_SOURCE_MAX */
	ASSERT(intno < IRQ_SOUCE_MAX);

	/*default isr, install directly */
	INF("install isr %x\n", intno);
	isr_table[intno].pisr = pisr;
	isr_table[intno].parg = parg;

	return OK;
}

/*
*********************************************************************************************************
*                                           UNINSTALL ISR
*
* Description:  uninstall ISR for a specific interrupt.
*
* Arguments  :  intno   : the number of interrupt which we want to uninstall ISR.
*               pisr    : the ISR which to been uninstall.
*
* Returns    :  OK if uninstall ISR succeeded, others if failed.
*********************************************************************************************************
*/
s32 uninstall_isr(u32 intno, __pISR_t pisr)
{
	/*intno can't beyond then IRQ_SOURCE_MAX */
	ASSERT(intno < IRQ_SOUCE_MAX);

	if (isr_table[intno].pisr == pisr) {
		/*uninstall isr */
		isr_table[intno].pisr = isr_default;
		isr_table[intno].parg = NULL;
	} else {
		/*
		 * don't support shared interrupt now,
		 * by sunny at 2012-5-3 11:20:28.
		 */
		ERR("ISR not installed!\n");
		return -EFAIL;
	}

	return OK;
}

/*
*********************************************************************************************************
*                                           INTERRUPT ENTRY
*
* Description:  the entry of CPU IRQ, mainly for CPU IRQ exception.
*
* Arguments  :  none.
*
* Returns    :  OK if process CPU IRQ succeeded, others if failed.
*********************************************************************************************************
*/
s32 interrupt_entry(void)
{
	u32 intno = intc_get_current_interrupt();

	/*intno can't beyond then IRQ_SOURCE_MAX */
	ASSERT(intno < IRQ_SOUCE_MAX);

	/*
	 * process interrupt by call isr,
	 * not support shared intterrupt.
	 */
	(isr_table[intno].pisr) (isr_table[intno].parg);

	return OK;
}

s32 interrupt_query_pending(u32 intno)
{
	volatile u32 pending;

	if (intno <= 31)
		pending = pintc_regs->pending & (1 << intno);
	else if (intno > 31 && intno <= 63)
		pending = pintc_regs->pending1 & (1 << (intno - 32));
	else
		pending = pintc_regs->pending2 & (1 << (intno - 64));
	return pending;
}

s32 interrupt_clear_pending(u32 intno)
{
	if (intno <= 31)
		pintc_regs->pending = (1 << intno);
	else if (intno > 31 && intno <= 63)
		pintc_regs->pending = (1 << (intno - 32));
	else
		pintc_regs->pending = (1 << (intno - 64));
	return OK;
}

s32 isr_default(void *arg)
{
	ERR("irq [%x] enable before ISR install\n",
	    intc_get_current_interrupt());

	return TRUE;
}

u32 interrupt_get_current_intno(void)
{
	return intc_get_current_interrupt();
}

s32 interrupt_get_enabled(u32 intno)
{
	return intc_interrupt_is_enabled(intno);
}

