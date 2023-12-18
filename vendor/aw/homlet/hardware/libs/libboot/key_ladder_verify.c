#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "toc0/SBrom_Toc.h"
#include "toc0/SBrom_Certif.h"
#include "toc0/key_ladder.h"

extern s32 _verify_level_2(struct x509_total_info * p_info);

int verify_key1(struct x509_total_info * p_x509_info ,struct SBROM_TOC_KEY_ITEM_info *p_key_item_info)
{
	u32 key1_pk_len=0;
	u32 cert_pk_len=0;
	//u8 cert_pk_buf[PK_MAX_LEN_BYTE]=0;
	u32 mod_len=p_x509_info->certif_info.I_7_PK_para_m_len;
	s32 ret=0;


	key1_pk_len=p_key_item_info->KEY1_PK_mod_len+p_key_item_info->KEY1_PK_e_len;
	if(key1_pk_len > PK_MAX_LEN_BYTE)
	{
		return (0);
	}

	cert_pk_len = mod_len + p_x509_info->certif_info.I_7_PK_para_e_len;
	if(cert_pk_len > (PK_MAX_LEN_BYTE) )
	{
		return (0);
	}

	if(p_key_item_info->KEY1_PK_mod_len==p_x509_info->certif_info.I_7_PK_para_m_len)
	{
		ret=memcmp(p_key_item_info->KEY1_PK,p_x509_info->certif_info.I_7_PK_para_m,p_key_item_info->KEY1_PK_mod_len);
		if(ret!=0)
		{
			return (0);
		}
	}
	else
	{
		return (0);
	}

	if(p_key_item_info->KEY1_PK_e_len==p_x509_info->certif_info.I_7_PK_para_e_len)
	{
		ret=memcmp((p_key_item_info->KEY1_PK+p_key_item_info->KEY1_PK_mod_len),p_x509_info->certif_info.I_7_PK_para_e,p_key_item_info->KEY1_PK_e_len);
		if(ret!=0)
		{
			return (0);
		}
	}
	else
	{
		return (0);
	}

	return (1);
}

s32 key_item_certif_verify(struct x509_total_info * p_x509_info ,struct SBROM_TOC_KEY_ITEM_info *p_key_item_info)
{
	 s32 ret=0;

	 ret=verify_key1(p_x509_info,p_key_item_info);
	 if(ret<=0)
	 {
                printf("In %s: verify_key1 fail.\n", __func__);
                return(0);
	 }

	 ret=_verify_level_2(p_x509_info);
	 if(ret<=0)
	 {
                printf("In %s: verify_level_2 fail.\n", __func__);
	        return(0);
	 }

	return(1);
}

int key_ladder_toc0_traverse(BufferExtractCookie* cookie)
{
	struct SBROM_TOC_total Toc_total;
	struct x509_total_info x509_info;
	struct SBROM_TOC_item_entry item_entry;
	struct SBROM_TOC_KEY_ITEM_info key0_item;
	struct key_item_total_info key_item_total;
	s32 ret = 0;
	u8 * p_item_addr = NULL;
	u32  item_len = 0;


	memset(&Toc_total , 0 ,sizeof(struct SBROM_TOC_total));
	memset(&x509_info,0,sizeof(x509_info));
	memset(&item_entry,0,sizeof(item_entry));
	memset(&key_item_total,0,sizeof(key_item_total));
	memset(&key0_item,0,sizeof(key0_item));


	ret = toc0_init(cookie,&Toc_total);
	if(ret < 0){
		printf(" Toc_init ,error\n");
		goto Toc0_fail;
	}

	ret = Toc_item_open(            &Toc_total,
					ITEM_NAME_SBROMSW_KEY,
					&p_item_addr,
					&item_len,
					&item_entry);
	if(ret < 0){
		printf("Toc open key item error\n");
		goto Toc0_fail;
	}

	//get key_item
	ret= key_item_init(p_item_addr,item_len,&key_item_total);
	if(ret < 0){
		printf("key item init error\n");
		goto Toc0_fail;
	}

	//verify key item
	ret = verify_key_item(&key_item_total.key_item_info);
	if(ret<=0) {
		printf("verify_key_item error\n");
		goto Toc0_fail;
	}

	//get & vetify  Toc0  certif
	ret=Toc_item_open(              &Toc_total,
					ITEM_NAME_SBROMSW_CERTIF,
					&p_item_addr,
					&item_len,
					&item_entry);
	if(ret < 0){
		printf("get toc0 certif error\n");
		goto Toc0_fail;
	}

	//parser certif
	ret = certif_open(p_item_addr, item_len, &x509_info);
	if(ret<0){
		printf("open certif error\n");
		goto Toc0_fail;
	}

	//check certif
	ret = key_item_certif_verify(&x509_info, &key_item_total.key_item_info);
	if(ret <= 0){
		printf("key item verify error\n");
		goto Toc0_fail;
	}

	//get sboot_bin
	ret=Toc_item_open(              &Toc_total,
					ITEM_NAME_SBROMSW_FW,
					&p_item_addr,
					&item_len,
					&item_entry);
	if(ret<0) {
		printf("get sboot error\n");
		goto Toc0_fail;
	}

	//check sboot
	ret = check_SBROMSW_hash(&x509_info, p_item_addr, item_len);
	if(ret <= 0){
		printf("check sboot error\n");
		goto Toc0_fail;
	}

	return 0;


Toc0_fail:
	return -1;
}
