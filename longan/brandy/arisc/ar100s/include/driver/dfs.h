/*
*******************************************************************************
*                                                AR100 SYSTEM
*                                     AR100 Software System Develop Kits
*                                                dram module
*
*                                    (c) Copyright 2012-2016, Sunny China
*                                             All Rights Reserved
*
* File    : dfs.h
* By      : Fanqh
* Version : v1.0
* Date    : 2022-1-10
* Descript: dram dfs header.
* Update  : date                auther      ver     notes
*           2022-1-10 10:28:51  Fanqh       1.0     Create this file.
********************************************************************************
*/
#ifndef __DFS_H__
#define __DFS_H__

#include <driver/timer.h>
#include <driver/dram.h>
#include <system/para.h>

#ifdef CFG_DRAMFREQ_USED
extern int mctl_mdfs_software(__dram_para_t *para, unsigned int freq_id);
#else
static inline int mctl_mdfs_software(__dram_para_t *para, unsigned int freq_id) { return -1; }
#endif
#endif
