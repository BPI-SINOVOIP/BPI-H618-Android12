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

#ifndef __TOC_CONFIG__H__
#define __TOC_CONFIG__H__


typedef struct {
	int  type;
	char item[64];
	char cnf[MAX_PATH];
	char bin[MAX_PATH];
	char key[MAX_PATH];
	char cert[MAX_PATH];
}
toc_descriptor_t;

typedef struct {
	int  type;
	char item[64];
	char cnf[MAX_PATH];
	char boot_bin[MAX_PATH];
	char key_item_bin[MAX_PATH];
	char key0[MAX_PATH];
	char key1[MAX_PATH];
	char cert[MAX_PATH];
}
toc_key_item_descriptor_t;

typedef struct aw_nvc_extension_t {
	int oid;
	int nvc;
} aw_nvc_ext_t;

#define AW_EXTENSION_NVC_OID	0x1

enum TOC0_CORRUPTED_TYPE_EN {
	ALL_GOOD = 0,
	BAD_HASH,
	BAD_MOD,
	BAD_PK,
	BAD_SIGN
};
extern enum TOC0_CORRUPTED_TYPE_EN toc0_corrupted_type;


#endif  /*__TOC_CONFIG__H__*/
