/*
**********************************************************************************************************************
*											        eGon
*						           the Embedded GO-ON Bootloader System
*									       eGON arm boot sub-system
*
*						  Copyright(C), 2006-2014, Allwinner Technology Co., Ltd.
*                                           All Rights Reserved
*
* File    :
*
* By      : Jerry
*
* Version : V2.00
*
* Date	  :
*
* Descript:
**********************************************************************************************************************
*/

#ifndef __TOC_INCLUDE__H__
#define __TOC_INCLUDE__H__


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "const_def.h"
#include "toc_config.h"
#include "createcert.h"
#include "createtoc.h"
#include "createkey.h"
#include "sha.h"
#include "parser_config.h"
#include "rsa.h"
#include "create_key_ladder.h"

enum enum_toc0_item {
    ENUM_TOC0_ITEM_CTERIF = 0,
    ENUM_TOC0_ITEM_FW,
    ENUM_TOC0_ITEM_KEY,
    ENUM_TOC0_ITEM_MAX
};

#endif  /*__TOC_INCLUDE__H__*/
