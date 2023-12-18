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

#ifndef __CONST__DEF__H__
#define __CONST__DEF__H__

#define	KEYPATH_CONST		"key"
#define	CNFPATH_CONST		"toc1/cnf"
#define	CERTPATH_CONST		"toc1/cert"
#define	TOC0PATH_CONST		"toc0"
#define	KEY_ITEM_BIN_PATH_CONST	"key_item_bin"
#define	KEY_LADDER_CONST	"toc0_key_ladder"
#define	ROOT_ROLLBACK_USED_CONST	"ROOT_ROLLBACK_USED"

#define  CNF_BASE_FILE   "cnf_base.cnf"
#define  TOC1_CONST_NAME "toc1.fex"
#define  TOC0_CONST_NAME "toc0.fex"
#define  PACKAGE_CONST_NAME "boot_package.fex"

#define  TOC1_CONFIG_MAX   (16)
#define  PACKAGE_CONFIG_MAX (16)

#define  NORMAL_TYPE       0
#define  ROOTKEY_TYPE      1
#define  ONLYKEY_TYPE      2
#define  LOGO_TYPE         3

#define  ITEM_TYPE_ROOTKEY   0
#define  ITEM_TYPE_KEYCERT   1
#define  ITEM_TYPE_BINKEY    2
#define  ITEM_TYPE_BINFILE   3
#define  ITEM_TYPE_LOGO      4

#define  STAMP_VALUE       0x5F0A6C39

#define	ITEM_NAME_SBROMSW_CERTIF	0x010101		/*Trusted SBrom-SW-boot		Certif*/
#define	ITEM_NAME_SBROMSW_FW		0x010202		/*Trusted SBrom-SW-boot		FW*/
#define	ITEM_NAME_SBROMSW_KEY		0x010303		/*Trusted SBrom-SW-boot	    key_item_bin*/

#define	RSA_BIT_WITDH	2048

#endif  /*__CONST__DEF__H__*/
