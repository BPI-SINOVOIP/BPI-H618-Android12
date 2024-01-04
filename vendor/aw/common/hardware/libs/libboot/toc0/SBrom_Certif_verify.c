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


#include "type_def.h"
#include <string.h>
#include "SBrom_Certif.h"
//#include "SBrom_key.h"
//#include "sbrom_ss.h"

#define	RSA_MOD_MAX_LEN_BIT		3072	
#define	RSA_MOD_MAX_LEN_BYTE	(RSA_MOD_MAX_LEN_BIT/8)

#define	SS_ALIGN_BYTE			32

u32 _get_align_addr(u32 p_addr ,u32 align_cell)
{
	u32 temp = 0;

	temp = (p_addr +  (align_cell - 1)) & (~(align_cell - 1));

	return temp;
}

//== 0 :not inited
// > 0 :inited
static s32 _rotpk_hash_inited(u32 * p_buff,u32 len_u32)
{
	s32 i = 0;
	u32 first_value = p_buff[0];
	
	for(i = 1; i < len_u32 ;i++){
		if((p_buff[i] == first_value)){
			continue;
		}else{
			return 1;
		}
	}
	return 0;
}


static s32 _combin_cacl_rotpk(struct x509_total_info * p_info,
									u8 * p_buff, 
									u32 buff_len)
{
	s32 i = 0;
	u8 buff_orig[RSA_MOD_MAX_LEN_BYTE * 2 + SS_ALIGN_BYTE *2];
	u8 * p_buff_orig = NULL;
	u32 mod_len = p_info->certif_info.I_7_PK_para_m_len ;
	s32 temp = 0;
	//check
	if(buff_len < 32 || p_buff == NULL){
		printf("PANIC : _combin_cacl_rotpk() : input error\n");
		return -1;
	}

	p_buff_orig = (u8 *)_get_align_addr((u32)(&(buff_orig[0])),(u32)SS_ALIGN_BYTE);

	memcpy(p_buff_orig,
				p_info->certif_info.I_7_PK_para_m,
				p_info->certif_info.I_7_PK_para_m_len);
	memcpy( (void *)((u32)p_buff_orig + mod_len),
				p_info->certif_info.I_7_PK_para_e,
				p_info->certif_info.I_7_PK_para_e_len);


	temp = mod_len + p_info->certif_info.I_7_PK_para_e_len;
	if(temp > (mod_len*2) ){
		printf("PANIC : _combin_cacl_rotpk() : error e,n\n");
		return -1;
	}

	memset((void *)((u32)p_buff_orig + temp),
				0x91,
				(mod_len * 2) - temp);


	if(sunxi_sha256_calc( (u8 *)p_buff, (u8 *)p_buff_orig, mod_len *2 ))
	{
		printf("sunxi_sha_calc: calc  pubkey sha256 with hardware err\n");
		return -1;
	}



	return 0;
				
	
}
//>0 passed verify
static s32 _verify_level_1(struct x509_total_info * p_info)
{
	u8 rotpk_buff_otp[32];
	u8 rotpk_buff_certif[32];
	u32 rotpk_len_u8 = sizeof(rotpk_buff_otp);
	u32 rotpk_len_u32 = rotpk_len_u8 >> 2;
	s32 temp = 0;
	int ret;
        char all_zero[32];
	
	memset(rotpk_buff_otp,0,sizeof(rotpk_buff_otp));

	ret=read_rotpk(rotpk_buff_otp);
	if(ret==0)
	{
		printf("read rotpk error\n");
		return 0;
	}

        memset(all_zero, 0, 32);
        if( !memcmp(all_zero, rotpk_buff_otp, 32) )
        {
                printf("Efuse is empty, do not need to verify toc0.\n");
                return EFUSE_IS_EMPTY;
        }

	//--1
	_rotpk_hash_inited(rotpk_buff_otp,rotpk_len_u32);
	
	
	//--2
	_combin_cacl_rotpk(p_info,
						rotpk_buff_certif,
						32);
	temp = memcmp((u8 *)rotpk_buff_certif,
						(u8 *)rotpk_buff_otp,
						rotpk_len_u8);

	printf("_verify_level_1(): cmp = %d\n",temp);

#ifdef OTADEBUG
        printf(">>>>>>>>>>>>> rotpk in certif<<<<<<<<<<<<<<<<\n");
        ndump(rotpk_buff_certif, 32);
        printf(">>>>>>>>>>>>> rotpk in efuse<<<<<<<<<<<<<<<<\n");
        ndump(rotpk_buff_otp, 32);
#endif

	return (temp == 0) ? 1 : 0;
}


//>0 passed verify
s32 _verify_level_2(struct x509_total_info * p_info)
{
#define	CERTIF_TEXT_LEN_MAX	(1024 *3)		//能预计最大1.5k
	u8 buff[CERTIF_TEXT_LEN_MAX + SS_ALIGN_BYTE];
	u8 * p_buff = NULL;
	u8 hash_txt[32];
	u8 hash_sign[32];
	u32 hash_sign_len = 32;
	int i;
	s32 ret = 0;

	memset(buff,0,sizeof(buff));
	memset(hash_txt,0,sizeof(hash_txt));
	memset(hash_sign,0,sizeof(hash_sign));
	
	p_buff = (u8 *)_get_align_addr((u32)buff,SS_ALIGN_BYTE);
	if(sizeof(buff) - p_info->certif_info.misc_text_len - SS_ALIGN_BYTE <= 0){
		printf("PANIC : _verify_level_2() : certif too big\n");
		return -1;
	}

	memcpy(p_buff ,
				(const void *)(p_info->certif_info.misc_text_saddr),
				p_info->certif_info.misc_text_len);
	
	
	sunxi_sha256_calc(hash_txt,
					p_buff,
					p_info->certif_info.misc_text_len);
	
	
	/*printf("sunxi_certif->pubkey.e_len:%x\n",p_info->certif_info.I_7_PK_para_e_len);
	for(i=0;i<p_info->certif_info.I_7_PK_para_e_len;i++)
			printf("sunxi_certif->pubkey.e:%x\n",(p_info->certif_info.I_7_PK_para_e)[i]);
	printf("sunxi_certif->pubkey.n_len:%x\n",p_info->certif_info.I_7_PK_para_m_len);
	for(i=0;i<p_info->certif_info.I_7_PK_para_m_len;i++)
			printf("sunxi_certif->pubkey.n:%x\n",(p_info->certif_info.I_7_PK_para_m)[i]);
	printf("sunxi_certif->sign_len:%x\n",p_info->certif_info.III_2_sign_len);
	for(i=0;i<p_info->certif_info.III_2_sign_len;i++)
			printf("sign_in_certif:%x\n",(p_info->certif_info.III_2_sign_value)[i]);*/

	sunxi_rsa2048_calc(
					hash_sign,
					(u8 *)p_info->certif_info.I_7_PK_para_e,
					p_info->certif_info.I_7_PK_para_e_len,
					(u8 *)p_info->certif_info.I_7_PK_para_m,
					p_info->certif_info.I_7_PK_para_m_len,
					(u8 *)p_info->certif_info.III_2_sign_value,
					p_info->certif_info.III_2_sign_len);

	//equal
	ret = memcmp(hash_txt, 
						hash_sign,
						32);
	if(ret != 0){
		printf("PANIC : _verify_level_2() : hash txt != sign\n");
		return 0;
	}

	return 1;
}

//>0 passed verify
s32  certif_verify(struct x509_total_info * p_info)
{
	s32 verify_1 = 0;
	s32 verify_2 = 0;

	verify_1 = _verify_level_1(p_info);
	if(verify_1==0)
	{
		return 0;
	}
        else if( verify_1 == EFUSE_IS_EMPTY)
        {
                return EFUSE_IS_EMPTY;
        }
	verify_2 = _verify_level_2(p_info);

	return (verify_2);
}


//>0 pass
s32 check_SBROMSW_hash(struct x509_total_info * p_x509_info ,
								u8 * p_saddr ,
								u32 len)
{
	u8 hash_1[EXT_HASH_WIDTH_BYTE*4 + 8];
	u8 hash_2[EXT_HASH_WIDTH_BYTE*4 + 8];
	s32 ret = 0;


	//check
	/*if( ((u32)p_saddr) & 0x1f){
		printf("PANIC : check_SBROMSW_hash() addr align err\n");
		return -1;
	}*/

	memset(hash_1,0,sizeof(hash_1));
	memset(hash_2,0,sizeof(hash_2));

	//get ext hash 1
	certif_get_hash(p_x509_info,
						hash_1,
						sizeof(p_x509_info->certif_info.II_2_sha256_hash));

	//calc hash 2

	if(sunxi_sha256_calc( (u8 *)hash_2, (u8 *)p_saddr,len ))
	{
		printf("sunxi_sha_calc: calc  pubkey sha256 with hardware err\n");
		return -1;
	}

	

	ret = memcmp(hash_1,hash_2,EXT_HASH_WIDTH_BYTE);

	if(ret == 0){
		return 1;
	}else{
		printf("PANIC : check_SBROMSW_hash() not match\n");
		return 0;
	}

}


