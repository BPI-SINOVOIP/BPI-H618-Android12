#ifndef _VERIFY_KEY_ITEM_
#define _VERIFY_KEY_ITEM_

//pk len
#define RSA_BIT		 (2048)
#define PK_MAX_LEN_BIT	 (RSA_BIT*2)
#define PK_MAX_LEN_BYTE	 (PK_MAX_LEN_BIT >> 3)

#define SHA256_BIT  	(256)
#define HASH_LEN_BYTE	(SHA256_BIT >> 3)
#define HASH_LEN_WORD  (HASH_LEN_BYTE >> 2)

#define AW_VENDOR_ID	0X00EFE800

#define KEY_ITEM_SIZE	(2048)

#define EFUSE_VENDOR_ID_EMPTY  (0x10)



typedef struct SBROM_TOC_KEY_ITEM_info
{
	u32 vendor_id;
	u32 KEY0_PK_mod_len;
	u32 KEY0_PK_e_len;
	u32 KEY1_PK_mod_len;
	u32 KEY1_PK_e_len;
	u32 sign_len;
	u8  KEY0_PK[PK_MAX_LEN_BYTE];
	u8  KEY1_PK[PK_MAX_LEN_BYTE];
	u8  reserve[32];
	u8  sign[256];

} SBROM_TOC_KEY_ITEM_info_t;

struct key_item_total_info
{
	u32 is_inited;
	u8 * key_item_addr;
	u32 key_item_len;
	struct SBROM_TOC_KEY_ITEM_info key_item_info;
};



s32 verify_key_item(struct SBROM_TOC_KEY_ITEM_info *);
s32 key_item_init(u8 *p_addr,u32 item_len,struct key_item_total_info * p_key_item_total);


#endif
