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

#ifndef __CREATE_CERT__H__
#define __CREATE_CERT__H__

int create_cert_for_toc0(char *lpCfg, toc_descriptor_t *toc0, char *keypath);
int create_cert_for_toc1(char *lpCfg, char *section_name, toc_descriptor_t *toc1, char *keypath, char *cnfpath_base);
int createcnf_for_package(char *lpCfg, toc_descriptor_t *package);
int u8_to_str(u8 *p_buff_u8, u32 u8_len, u8 *p_str, u32 str_buff_len);

#endif  /*__CREATE_CERT__H__*/
