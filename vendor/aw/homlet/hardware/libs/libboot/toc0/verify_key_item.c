#include "type_def.h"
#include <string.h>
#include "SBrom_Certif.h"
#include "key_ladder.h"
#include "sunxi_boot_api.h"

#define	SS_ALIGN_BYTE			32
#define ROTPK_LEN_WORD                  8
#define ERR_ROTPK_NO_INIT
#define SHA2_256_MODE

//#define OTADEBUG

extern void sid_read_rotpk(void *dst) ;

int _verify_vendor_id(u32 vendor_id)
{
	u32 efuse_vendor_id=0;

	efuse_vendor_id=eFuse_read_VEN_ID();
	//--需要判断是否初始化?
	if(efuse_vendor_id==0)
	{
		return EFUSE_VENDOR_ID_EMPTY;
	}

	if(efuse_vendor_id==vendor_id)
	{
		return 1;
	}
	return (0);
}

s32 _verify_key0_pk(u8 *key0_pk_buf,u32 key0_pk_mod_len,u32 key0_pk_e_len){
	u8  key0_pk_arry[PK_MAX_LEN_BYTE+SS_ALIGN_BYTE]={0};
	u8 *p_key0_pk_buf=NULL;
	u32 key0_sha_otp[ROTPK_LEN_WORD]={0};
	u32 key0_sha_buff[ROTPK_LEN_WORD]={0};
	u32 key0_sha_len_u8 = sizeof(key0_sha_otp);
	u32 key0_sha_len_u32 = key0_sha_len_u8 >> 2;
	s32 temp = 0;
	u32 key0_pk_len=0;
        char all_zero[32] = {0};

	key0_pk_len=key0_pk_mod_len+key0_pk_e_len;
	if(key0_pk_len>PK_MAX_LEN_BYTE)
	{
                printf("In %s: err: key0_pk_len is larger than 512.\n ", __func__);
		return (0);
	}

	memset(key0_sha_otp,0,sizeof(key0_sha_otp));
	memset(key0_sha_buff,0,sizeof(key0_sha_buff));
	memset(key0_pk_arry,0,sizeof(key0_pk_arry));

	int ret = read_rotpk(key0_sha_otp);
        if( ret == 0)
        {
            printf("read rotpk error!\n");
            return 0;
        }
        memset(all_zero, 0, 32);
        if( !memcmp(all_zero, key0_sha_otp, 32) )
            return 1;       /*Don't check if rotpk efuse is empty'*/

#ifdef OTADEBUG
        printf("In %s: rotpk read from efuse is :\n", __func__);
        ndump((u8 *)key0_sha_otp, 32);
#endif

/*
	//--1 read efuse key0 sha
	temp = _rotpk_hash_inited(key0_sha_otp,key0_sha_len_u32);
	if(temp == 0){
		SBHW_INFO_ERR(ERR_ROTPK_NO_INIT);
                return 1;
	}
*/
	//--2 calu item key0 sha
	//--pk need extern to 512byte
	p_key0_pk_buf=(u8 *)_get_align_addr((u32)(&(key0_pk_arry[0])),(u32)SS_ALIGN_BYTE);
	memcpy(p_key0_pk_buf,key0_pk_buf,key0_pk_len);
	memset((void *)(        (u32)p_key0_pk_buf + key0_pk_len),
				0x91,
				((PK_MAX_LEN_BYTE) - key0_pk_len));

	sunxi_sha256_calc(      (u8*)key0_sha_buff,
                                p_key0_pk_buf,
				PK_MAX_LEN_BYTE);

#ifdef OTADEBUG
        printf("In %s: the hash of key0_pk is: \n", __func__);
        ndump((u8 *)key0_sha_buff, 32);
        printf("In %s: the buff of keyo_pk is: \n", __func__);
        ndump((u8 *)p_key0_pk_buf, key0_pk_len);
#endif

	//--3
	temp = memcmp(          (u8*)key0_sha_buff,
				(u8*)key0_sha_otp,
				key0_sha_len_u8);

	if(temp == 0)
	{
		return 1;
	}
	else
	{
                printf("In %s: memcmp result = %d, key0_pk hash compare fail!\n", __func__, temp);
		return 0;
	}

}

s32 _verify_key_item_sign(struct SBROM_TOC_KEY_ITEM_info *p_key_item)
{
	u32 text_len=0;
	u8	key_item_arry[KEY_ITEM_SIZE+SS_ALIGN_BYTE]={0};
	u8 *p_key_item_arry=NULL;
	u8 hash_txt[HASH_LEN_BYTE]={0};
	u8 hash_sign[HASH_LEN_BYTE]={0};
	u8 pk_mod_buf[RSA_BIT >> 3]={0};
	u8 pk_e_buf[RSA_BIT >> 3]={0};
	s32 ret=0;
	u32 hash_sign_len=HASH_LEN_BYTE;
	u32 pk_mod_bit=0;

	pk_mod_bit=(p_key_item->KEY0_PK_mod_len <<3);

	memset(hash_txt,0,sizeof(hash_txt));
	memset(hash_sign,0,sizeof(hash_sign));
	memset(pk_mod_buf,0,sizeof(pk_mod_buf));
	memset(pk_e_buf,0,sizeof(pk_e_buf));
	memset(key_item_arry,0,sizeof(key_item_arry));

	memcpy(pk_mod_buf,p_key_item->KEY0_PK,p_key_item->KEY0_PK_mod_len);
	memcpy(pk_e_buf,(p_key_item->KEY0_PK+p_key_item->KEY0_PK_mod_len),p_key_item->KEY0_PK_e_len);

	//--?
	text_len=sizeof(struct SBROM_TOC_KEY_ITEM_info)-sizeof(p_key_item->sign);

	p_key_item_arry=(u8 *)_get_align_addr((u32)(&(key_item_arry[0])),(u32)SS_ALIGN_BYTE);
	memcpy(p_key_item_arry,(u8*)p_key_item,text_len);

#ifdef OTADEBUG
        printf("In %s: key_item_sign : \n", __func__);
        ndump(p_key_item_arry, text_len);
#endif

	//--cal key item sha1
	sunxi_sha256_calc(              hash_txt,
                                        p_key_item_arry,
					text_len
					);

#ifdef OTADEBUG
        printf("In %s: the hash of key_item_sign :\n",__func__);
        ndump(hash_txt, 32);
#endif
	//--cal sigan sha2
	sunxi_rsa2048_calc(
					hash_sign,
					(u8 *)pk_e_buf,
					p_key_item->KEY0_PK_e_len,
					(u8 *)pk_mod_buf,
                                        p_key_item->KEY1_PK_mod_len,
					(u8 *)p_key_item->sign,
					p_key_item->sign_len
					);

	//--cmp
#ifdef OTADEBUG
        printf("In %s: the hash from key_item_sign cerit :\n", __func__);
        ndump(hash_sign, 32);
#endif

	ret = memcmp(hash_txt,hash_sign,sizeof(hash_txt));
	if(ret == 0)
	{
		return(1);
	}
	else
	{
                printf("In %s: memcmp result = %d, key_item_sign hash compare fail.\n", __func__, ret);
		return(0);
	}


}

s32 verify_key_item(struct SBROM_TOC_KEY_ITEM_info *p_key_item)
{
	s32 ret=0;
	//u8* key0_buf=NULL;
	u32 pk_len=0;

	pk_len=p_key_item->KEY0_PK_mod_len+p_key_item->KEY0_PK_e_len;

	if(pk_len>512)
	{
                printf("In %s: err: pk_len is larger than 512.\n", __func__);
		return (-1);
	}

/*
	//--verify_vendor_id
	ret=_verify_vendor_id(p_key_item->vendor_id);
	if(!ret)
	{
		return(-1);
	}
*/

	//--verify KEY0_PK
	ret=_verify_key0_pk(p_key_item->KEY0_PK,p_key_item->KEY0_PK_mod_len,p_key_item->KEY0_PK_e_len);
	if(!ret)
	{
                printf("In %s : _verify_key0_pk fail.\n", __func__);
		return(-1);
	}

	//--verify key item sign
	ret=_verify_key_item_sign(p_key_item);
	if(!ret)
	{
                printf("In %s: _verify_key_item_sign fail.\n", __func__);
                return(-1);
	}

	return(1);
}

s32 key_item_init(u8 *p_addr,u32 item_len,struct key_item_total_info *p_key_item_total)
{
	if(p_addr==NULL ||p_key_item_total==NULL )
	{
		return (-1);
	}
	p_key_item_total->key_item_addr=p_addr;
	p_key_item_total->key_item_len=item_len;
	memcpy((u8*)(&(p_key_item_total->key_item_info)),p_addr,sizeof(struct SBROM_TOC_KEY_ITEM_info));

	return (1);
}
