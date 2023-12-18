/*
************************************************************************************************************************
*                                          Boot rom 
*                                         Seucre Boot
*
*                             Copyright(C), 2006-2013, AllWinners Microelectronic Co., Ltd.
*											       All Rights Reserved
*
* File Name   : SBrom_certif.h
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

#ifndef	__SBROM_CERTIF_H__
#define	__SBROM_CERTIF_H__


#define	 EXT_HASH_WIDTH_BYTE	32

#define EFUSE_IS_EMPTY  0x10


struct x509_SBROMSW_certif{
	u32 I_1_certif_ver;
	u32 I_2_certif_serial;
	u8	I_3_certif_algorithm_OID[20];	//ÓÐÓÃ
	u8  I_3_OID_len;

	u8  I_7_PK_algorithm_OID[16];			//9
	//u32 I_7_para_item_len ;
	u8  I_7_PK_para_m[(3072>> 3) + 16];
	u32 I_7_PK_para_m_len;
	u8	I_7_PK_para_e[(3072>> 3) + 16];
	u32 I_7_PK_para_e_len;

	u32	II_2_sha256_hash[EXT_HASH_WIDTH_BYTE];		//???,

	u8  III_1_OID[16];
	u8  III_2_sign_value[(3072>> 3) + 16];
	u32 III_2_sign_len;

	u32 misc_text_saddr;
	u32 misc_text_len;
};

struct x509_total_info
{
	u32 is_inited;
	u8 * p_addr;
	u32 certif_len;
	struct x509_SBROMSW_certif certif_info;
};

s32  certif_open(u8 * p_addr , u32 len ,struct x509_total_info * p_info);
s32 certif_close(struct x509_total_info * p_info);

//>0 passed verify
s32  certif_verify(struct x509_total_info * p_info);

s32  certif_get_hash(struct x509_total_info * p_info,
					u8 * p_buff , 
					u32 buff_len );


#endif
