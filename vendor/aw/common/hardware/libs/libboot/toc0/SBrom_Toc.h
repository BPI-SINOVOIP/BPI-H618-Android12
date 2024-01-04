/*
************************************************************************************************************************
*                                          Boot rom 
*                                         Seucre Boot
*
*                             Copyright(C), 2006-2013, AllWinners Microelectronic Co., Ltd.
*											       All Rights Reserved
*
* File Name   : Base.h
*
* Author      : glhuang
*
* Version     : 0.0.1
*
* Date        : 2013.09.05
*
* Description :
* 
* Others      : None at present.
*
*
* History     :
*
*  <Author>        <time>       <version>      <description>
*
* glhuang       2013.09.05       0.0.1        build the file
*
************************************************************************************************************************
*/
#ifndef	__SBROM_HW_1639__H__
#define	__SBROM_HW_1639__H__

#include "spare_head.h"

//==============================================================================
#define	TOC_MAIN_INFO_MAGIC	(0x89119800)


#define	ITEM_NAME_SBROMSW_CERTIF				0x010101		//Trusted SBrom-SW-boot		Certif		
#define	ITEM_NAME_SBROMSW_FW					0x010202		//Trusted SBrom-SW-boot		FW			
#define	ITEM_NAME_TRUSTKEY_CERTIF				0x020103		//Trusted Key           		Certif		
#define	ITEM_NAME_TRUSTED_PRIM_DEBUG_CERTIF		0x020104		//Trusted Primary Debug 	 	Certif		
#define	ITEM_NAME_TRUSTED_SOC_PATCH_CERTIF		0x020105		//Trusted Soc patch		 	Certif		
#define ITEM_NAME_SBROMSW_KEY                                   0x010303        //Trusted SBrom-sw-key      key_item


#define	TOC_MAIN_INFO_STATUS_ENCRYP_NOT_USED	0x00
#define	TOC_MAIN_INFO_STATUS_ENCRYP_SSK			0x01
#define	TOC_MAIN_INFO_STATUS_ENCRYP_BSSK		0x02


struct SBROM_TOC_main_info{
	u8 Toc_name[8]	;	//user can modify
	u32 Toc_magic	;	//must equal TOC_U32_MAGIC
	u32 Toc_add_sum	;
	
	u32 Toc_serial_num	;	//user can modify
	u32 Toc_status		;	//user can modify,such as TOC_MAIN_INFO_STATUS_ENCRYP_NOT_USED
	
	u32 Toc_items_nr;	//total entry number
	u32 Toc_valid_len;	
	u8  Toc_platform[4];
	u32 Toc_reserved[2];	//reserved for future
	u32 TOC_MAIN_END;	
};



#define	TOC_ITEM_ENTRY_STATUS_ENCRYP_NOT_USED	0x00
#define	TOC_ITEM_ENTRY_STATUS_ENCRYP_USED		0x01

#define	TOC_ITEM_ENTRY_TYPE_NULL				0x00
#define	TOC_ITEM_ENTRY_TYPE_CERTIF				0x01
#define	TOC_ITEM_ENTRY_TYPE_BIN					0x02
#define TOC_ITEM_ENTRY_TYPE_KEY                                 0x03

struct SBROM_TOC_item_entry{
	u32 Entry_name;				
	u32 Entry_data_offset;		
	u32 Entry_data_len;			
	u32 Entry_status;			
	u32 Entry_type;				
	u32 Entry_call;				
	u32 Entry_reserved_1;		
	u32 Entry_end;
};
//===================================
struct SBROM_TOC_total{
	u8 * p_Toc_addr;
	struct SBROM_TOC_main_info main_info;
};

s32 toc0_init(BufferExtractCookie* cookie ,struct SBROM_TOC_total * p_total);

//p_item_addr:Output:	start addr 
//p_item_len :Output:	len
s32 Toc_item_open(struct SBROM_TOC_total * p_total ,
						u32 entry_id ,
						u8 ** p_item_addr ,
						u32 * p_item_len , 
						struct SBROM_TOC_item_entry * p_toc_item);


#endif	//__SBROM_HW_1639__H__
