#ifndef _CREATE_KEY_LADDER_
#define _CREATE_KEY_LADDER_

/*pk len*/
#define RSA_BIT		(2048)
#define PK_MAX_LEN_BIT	(RSA_BIT*2)
#define PK_MAX_LEN_BYTE	(PK_MAX_LEN_BIT >> 3)

/*hash*/
#define SHA256_BIT	(256)
#define HASH_LEN_BYTE	(SHA256_BIT >> 3)
#define HASH_LEN_WORD	(HASH_LEN_BYTE >> 2)

#define KEY_ITEM_SIZE	(2048)

typedef struct {
	unsigned int  vendor_id;
	unsigned int  KEY0_PK_mod_len;
	unsigned int  KEY0_PK_e_len;
	unsigned int  KEY1_PK_mod_len;
	unsigned int  KEY1_PK_e_len;
	unsigned int  sign_len;
	unsigned char  KEY0_PK[PK_MAX_LEN_BYTE];
	unsigned char  KEY1_PK[PK_MAX_LEN_BYTE];
	unsigned char  reserve[32];
	unsigned char  sign[256];
} SBROM_TOC_KEY_ITEM_info_t;

extern int prode_toc_key_ladder(char *lpCfg);
extern int create_key_ladder(toc_key_item_descriptor_t *toc0);
extern int create_all_file_for_toc0(char *lpCfg, toc_key_item_descriptor_t *toc0, char *keypath);

#endif
