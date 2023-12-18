/*
*********************************************************************************************************
*                                                AR100 SYSTEM
*                                     AR100 Software System Develop Kits
*                                                cpucfg module
*
*                                    (c) Copyright 2012-2016, Sunny China
*                                             All Rights Reserved
*
* File    : cpucfg_regs.h
* By      : Sunny
* Version : v1.0
* Date    : 2012-5-7
* Descript: cpu config register defines.
* Update  : date                auther      ver     notes
*           2012-5-7 18:54:47   Sunny       1.0     Create this file.
*********************************************************************************************************
*/

#ifndef __CPUCFG_REGS_H__
#define __CPUCFG_REGS_H__

typedef enum cpucfg_cpu_reset_ctrl {
	CPUCFG_C0_CPU0 = 0x0,
	CPUCFG_C0_CPU1 = 0x1,
	CPUCFG_C0_CPU2 = 0x2,
	CPUCFG_C0_CPU3 = 0x3,
	CPUCFG_CPU_NUMBER = 0x4,
} cpucfg_cpu_reset_ctrl_e;

#define CPUCFG_LE_SPACE_NUMBER      (4)

/* TIMESTAMP REGS */
#define CNT_LOW_REG             (TS_STAT_REG_BASE + 0x0000)
#define CNT_HIGH_REG            (TS_STAT_REG_BASE + 0x0004)
#define CNT_CTRL_REG            (TS_CTRL_REG_BASE + 0x0000)
#define CNT_LOW_REG_SET         (TS_CTRL_REG_BASE + 0x0008)
#define CNT_HIGH_REG_SET        (TS_CTRL_REG_BASE + 0x000C)
#define CNT_FREQID_REG          (TS_CTRL_REG_BASE + 0x0020)

/* CLUSTER CFG REG */
#define C0_RST_CTRL_REG         (CPUCFG_REG_BASE + 0x0000)
#define C0_CTRL_REG0            (CPUCFG_REG_BASE + 0x0010)
#define C0_CTRL_REG1            (CPUCFG_REG_BASE + 0x0014)
#define C0_CPU_STATUS0_REG      (CPUCFG_REG_BASE + 0x0030)
#define C0_CPU_STATUS1_REG      (CPUCFG_REG_BASE + 0x0034)
#define C0_CPU_STATUS2_REG      (CPUCFG_REG_BASE + 0x0038)
#define C0_CPU_STATUS3_REG      (CPUCFG_REG_BASE + 0x003C)
#define CO_CPU_CTRL_REG(core)   (CPUCFG_REG_BASE + 0x0050 + (core) * 0x04)
#define SUNXI_CPU_RST_CTRL(cluster)               (CPUCFG_REG_BASE + 0x00 + (cluster<<2))
#define SUNXI_CLUSTER_CPU_STATUS0(cluster)        (CPUCFG_REG_BASE + 0x30 + (cluster<<2))

/* CPU_SUBSYSTEM REG */
#define SUB_CPU_CTRL_REG(core)  (CPUSUBSYS_REG_BASE + 0x20 + (core) * 0x4)
#define SUNXI_CPU_RVBA_L(cpu)   (CPUSUBSYS_REG_BASE + 0x40 + (cpu) * 0x8)
#define SUNXI_CPU_RVBA_H(cpu)   (CPUSUBSYS_REG_BASE + 0x44 + (cpu) * 0x8)

/* CPUSCFG REGS */
#define CPUS_SPACE_LE_CTRL_REG  (R_CPUCFG_REG_BASE + 0x000C)
#define CPUS_SPACE_START_REG(n) (R_CPUCFG_REG_BASE + 0x0010 + n * 8)
#define CPUS_SPACE_END_REG(n)   (R_CPUCFG_REG_BASE + 0x0014 + n * 8)

#define C0_CPUX_CFG_REG              (R_CPUCFG_REG_BASE + 0x0040)
#define C0_CPUX_POWEROFF_GATING_REG  (R_CPUCFG_REG_BASE + 0x0044)
#define C0_CPU0_CFG_REG              (R_CPUCFG_REG_BASE + 0x0070)

#define SUNXI_CLUSTER_PWRON_RESET(cluster)        (R_CPUCFG_REG_BASE + 0x40 + (cluster<<2))
#define SUNXI_CLUSTER_PWROFF_GATING(cluster)      (R_CPUCFG_REG_BASE + 0x44 + (cluster<<2))
#define SUNXI_CPU_PWR_CLAMP(cluster, cpu)         (R_CPUCFG_REG_BASE + 0x50 + (cluster<<4) + (cpu<<2))
#define CPUS_C0_CPU_CFG_REG(core)                 (R_CPUCFG_REG_BASE + 0x70 + (core) * 0x04)

#define SUNXI_CPU_SYS_RESET                       (R_CPUCFG_REG_BASE + 0x0A0)

#endif /* __CPUCFG_REGS_H__ */
