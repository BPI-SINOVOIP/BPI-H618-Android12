/*
* Copyright (c) 2008-2018 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : isp_version.h
* Description :
* History :
* Author  : zhaowei <zhaowei@allwinnertech.com>
* Date    : 2018/02/08
*
*/

#ifndef ISP_VERSION_H
#define ISP_VERSION_H

#include "include/isp_debug.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ISP_VERSION "V1.00"
#define REPO_TAG "v5-sdv-v1.0-1.0.1"
#define REPO_BRANCH "a50_merge"
#define REPO_COMMIT "316ce91396a7de9e23cd01fd4da218dd7a3e2fe9"
#define REPO_DATE "Thu Feb 8 18:39:56 2018 +0800"
#define RELEASE_AUTHOR "zhengjiangwei"

static inline void isp_version_info(void)
{
	ISP_PRINT("\n>>>>>>>>>>>>>>>>>>>> ISP VERSION INFO <<<<<<<<<<<<<<<<<<<\n"
		"version:%s\n"
		"tag   : %s\n"
		"branch: %s\n"
		"commit: %s\n"
		"date  : %s\n"
		"author: %s\n"
		"--------------------------------------------------------\n\n",
		ISP_VERSION, REPO_TAG, REPO_BRANCH, REPO_COMMIT, REPO_DATE, RELEASE_AUTHOR);
}

#ifdef __cplusplus
}
#endif

#endif

