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

#ifndef __CREATE_TOC__H__
#define __CREATE_TOC__H__

#include "toc_config.h"

int splittoc0(char *toc0);
int splittoc1(char *toc1);
int update_toc0_cert(toc_descriptor_t *toc0, char *toc0_name);
int splittoc0_item(char *toc0, int32_t toc0_item_name, char *out_item_file);
int splittoc1_item(char *toc1, char *toc1_item_name, char *out_item_file);

int createtoc0(toc_descriptor_t *toc1, char *toc1_name);
int createtoc1(toc_descriptor_t *toc1, char *toc1_name, int main_v, int sub_v);
int create_package(toc_descriptor_t *package, char *package_name);
int create_toc0_for_key_ladder(toc_key_item_descriptor_t *toc0, char *toc0_name);

#endif  /*__CREATE_TOC__H__*/
