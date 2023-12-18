#include "common.h"
#include "include.h"

int __sunxi_bytes_merge(u8 *dst, u32 dst_len, u8 *src, uint src_len);
int __merge_certif_for_toc0(char *hash_value, u8 *key_n, u8 *key_e, u8 *key_d, char *cert_name);


#define AW_VENDOR_ID	0X00EFE800

int prode_toc_key_ladder(char *lpcfg)
{
	char all_key_item[1024];
	char *mainkey = KEY_LADDER_CONST;
	char lpcfg_full_name[MAX_PATH] = "";
	char line_buff[256], *p_line_buff, *ret_buff;
	char mainkey_in_cfg[128], *p_mainkey_in_cfg;
	FILE *p_file = NULL;

	if (lpcfg == NULL) {
		printf("prode_key_item_ladder err: lpcfg is NULL\n");
		return 0;
	}

	memset(all_key_item, 0, sizeof(all_key_item));

	GetFullPath(lpcfg_full_name, lpcfg);
	p_file = fopen(lpcfg_full_name, "rb");
	if (p_file == NULL) {
		printf("prode_key_item_ladder err: cant open file %s\n", lpcfg_full_name);
		return 0;
	}

	fseek(p_file, 0, SEEK_SET);
	do {
		memset(line_buff, 0, 256);
		ret_buff = fgets(line_buff, 256, p_file);
		if (ret_buff == NULL) {
			if (feof(p_file)) {
				printf("prode_key_item_ladder read to end\n");
				fclose(p_file);
				return 0;
			} else {
				printf("prode_key_item_ladder err: occur a err\n");
				fclose(p_file);
				return 0;
			}
		}
		p_line_buff = line_buff;
		while (1) {
			if ((*p_line_buff == ' ') || (*p_line_buff == '	'))
				p_line_buff++;
			else
				break;
		}

		if ((*p_line_buff == ';') || (*p_line_buff == '#') || (*p_line_buff == 0xa) || (*p_line_buff == 0xd))
			continue;

		if (*p_line_buff == '[') {
			memset(mainkey_in_cfg, 0, 128);
			p_mainkey_in_cfg = mainkey_in_cfg;
			p_line_buff++;
			while (1) {
				if ((*p_line_buff == ' ') || (*p_line_buff == '	'))
					p_line_buff++;
				else
					break;
			}

			while ((*p_line_buff != ']') && (*p_line_buff != ' ') && (*p_line_buff != '	')) {
				*p_mainkey_in_cfg++ = *p_line_buff++;
			}

			if (!strcmp(mainkey_in_cfg, mainkey)) {
				fclose(p_file);
				return 1;
			}
			continue;
		}


	} while (1);

	fclose(p_file);

	return 0;
}


/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/

int create_key_ladder(toc_key_item_descriptor_t *toc0)
{
	SBROM_TOC_KEY_ITEM_info_t key_item_bin;
	char key0_n[560], key0_e[560], key0_d[560];
	char key1_n[560], key1_e[560], key1_d[560];
	char current_path[MAX_PATH];
	char  cmdline[1024];
	unsigned int key0_n_len = 0, key0_e_len = 0;
	unsigned int key1_n_len = 0, key1_e_len = 0;
	unsigned char key0_n_digital[RSA_BIT_WITDH/8];
	unsigned char key0_e_digital[RSA_BIT_WITDH/8];
	unsigned char key1_n_digital[RSA_BIT_WITDH/8];
	unsigned char key1_e_digital[RSA_BIT_WITDH/8];
	unsigned char key_item_bin_buff[KEY_ITEM_SIZE];
	unsigned char hash_buff[HASH_LEN_BYTE];
	unsigned char hash_str[HASH_LEN_BYTE*2+8];
	unsigned char sign_buff[(RSA_BIT_WITDH >> 3)];
	unsigned int 	key_item_bin_txt_len = 0;
	char *p_key0_n_tmp = NULL, *p_key1_n_tmp = NULL;
	int ret = 0;
	FILE *fp = NULL;

	/*获取对应的key的公钥*/
	memset(key0_n, 0, 560);
	memset(key0_e, 0, 560);
	memset(key0_d, 0, 560);
	memset(current_path, 0, MAX_PATH);
	sprintf(current_path, "%s.bin", toc0->key0);
	printf("key0.bin_patch=%s\n", current_path);

	ret = getallkey(current_path, key0_n, key0_e, key0_d);
	if (ret < 0) {
		printf("key_item_bin_init err in getpublickey key0.bin\n");
		return -1;
	}

	p_key0_n_tmp = key0_n;
	while (*p_key0_n_tmp == '0') {
		p_key0_n_tmp++;
	}
	key0_n_len = strlen((const char *)p_key0_n_tmp);
	key0_e_len = strlen((const char *)key0_e);
	memset(key0_n_digital, 0, RSA_BIT_WITDH/8);
	memset(key0_e_digital, 0, RSA_BIT_WITDH/8);
	if (__sunxi_bytes_merge(key0_n_digital, RSA_BIT_WITDH/8, (u8 *)p_key0_n_tmp, key0_n_len)) {
		printf("key_item_bin_init err in sunxi_bytes_merge for keyn value\n");
		return -1;
	}

	if (__sunxi_bytes_merge(key0_e_digital, RSA_BIT_WITDH/8, (u8 *)key0_e, key0_e_len)) {
		printf("key_item_bin_init err in sunxi_bytes_merge for keye value\n");
		return -1;
	}

	memset(key1_n, 0, 560);
	memset(key1_e, 0, 560);
	memset(key1_d, 0, 560);
	memset(current_path, 0, MAX_PATH);
	sprintf(current_path, "%s.bin", toc0->key1);
	printf("key1.bin_patch=%s\n", current_path);

	ret = getallkey(current_path, key1_n, key1_e, key1_d);
	if (ret < 0) {
		printf("key_item_bin_init err in getpublickey key1.bin\n");
		return -1;
	}
	p_key1_n_tmp = key1_n;
	while (*p_key1_n_tmp == '0') {
		p_key1_n_tmp++;
	}
	key1_n_len = strlen((const char *)p_key1_n_tmp);
	key1_e_len = strlen((const char *)key1_e);
	memset(key1_n_digital, 0, RSA_BIT_WITDH/8);
	memset(key1_e_digital, 0, RSA_BIT_WITDH/8);
	if (__sunxi_bytes_merge(key1_n_digital, RSA_BIT_WITDH/8, (u8 *)p_key1_n_tmp, key1_n_len)) {
		printf("key_item_bin_init err in sunxi_bytes_merge for keyn value\n");
		return -1;
	}

	if (__sunxi_bytes_merge(key1_e_digital, RSA_BIT_WITDH/8, (u8 *)key1_e, key1_e_len)) {
		printf("key_item_bin_init err in sunxi_bytes_merge for keye value\n");
		return -1;
	}

	memset(key_item_bin_buff, 0, KEY_ITEM_SIZE);
	memset((void *)&key_item_bin, 0, sizeof(SBROM_TOC_KEY_ITEM_info_t));
	memset(key_item_bin.reserve, 0, sizeof(key_item_bin.reserve));
	key_item_bin.vendor_id = AW_VENDOR_ID;
	key_item_bin.KEY0_PK_mod_len = (key0_n_len+1)>>1;
	key_item_bin.KEY0_PK_e_len = (key0_e_len+1)>>1;
	key_item_bin.KEY1_PK_mod_len = (key1_n_len+1)>>1;
	key_item_bin.KEY1_PK_e_len = (key1_e_len+1)>>1;
	key_item_bin.sign_len = (RSA_BIT_WITDH >> 3);
	key_item_bin_txt_len = sizeof(SBROM_TOC_KEY_ITEM_info_t)-sizeof(key_item_bin.sign);

	memcpy(key_item_bin.KEY0_PK, key0_n_digital, key_item_bin.KEY0_PK_mod_len);
	memcpy((key_item_bin.KEY0_PK+key_item_bin.KEY0_PK_mod_len), key0_e_digital, key_item_bin.KEY0_PK_e_len);
	memcpy(key_item_bin.KEY1_PK, key1_n_digital, key_item_bin.KEY1_PK_mod_len);
	memcpy((key_item_bin.KEY1_PK+key_item_bin.KEY1_PK_mod_len), key1_e_digital, key_item_bin.KEY1_PK_e_len);
	memcpy(key_item_bin_buff, ((unsigned char *)&key_item_bin), key_item_bin_txt_len);

	sha256(key_item_bin_buff, key_item_bin_txt_len, hash_buff);
	memset(hash_str, 0, sizeof(hash_str));
	u8_to_str(hash_buff, 32, hash_str, sizeof(hash_str));
	printf("key_item_bin_txt hash is %s\n", hash_str);

	memset(sign_buff, 0, sizeof(sign_buff));
	rsa_sign_main(
			(char *) p_key0_n_tmp,
			(char *) key0_d,
			(char *) key0_e,
			(char *)hash_str,
			sign_buff,
			sizeof(sign_buff),
			RSA_BIT_WITDH
			);
	memcpy(key_item_bin.sign, sign_buff, key_item_bin.sign_len);
	memcpy(key_item_bin_buff, (unsigned char *)&key_item_bin, sizeof(SBROM_TOC_KEY_ITEM_info_t));

	memset(current_path , 0, MAX_PATH);
	GetFullPath(current_path, KEY_ITEM_BIN_PATH_CONST);

	memset(cmdline, 0, 1024);
	sprintf(cmdline, "rm -rf %s", current_path);
	system(cmdline);

	memset(cmdline, 0, 1024);
	sprintf(cmdline, "mkdir -p %s", current_path);
	system(cmdline);

	sprintf(toc0->key_item_bin, "%s/%s.bin", current_path, KEY_ITEM_BIN_PATH_CONST);
	printf("key_item_bin_patch=%s\n", toc0->key_item_bin);

	fp = fopen(toc0->key_item_bin, "wb");
	if (fp == NULL) {
		printf("key_item_bin_init err in:can not create %s\n", toc0->key_item_bin);
		return -1;
	}
	fwrite(&key_item_bin_buff, 1, KEY_ITEM_SIZE, fp);
	fclose(fp);
	return 0;
}

/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :
*
*
************************************************************************************************************
*/

int create_all_file_for_toc0(char *lpCfg, toc_key_item_descriptor_t *toc0, char *keypath)
{
	char all_key_item_bin[1024];
	char line_info[256];
	char type_name[32];
	char *all_key_item_line[2];
	int  ret = 0, i = 0;
	char hash_value[128];
	char key_n[560], key_e[560], key_d[560];
	char current_path[MAX_PATH];
	char item_name[64], key_name[64];

	memset(all_key_item_bin, 0, 1024);
	memset(all_key_item_line, 0, 2 * sizeof(char *));
	memset(line_info, 0, 128);

	if (GetPrivateProfileSection(KEY_LADDER_CONST, all_key_item_bin, 1024, lpCfg)) {
		printf("create_all_file_for_toc0 err in no find [toc0_key_ladder] \n");
		return -1;
	}

	if (all_key_item_bin[0] == '\0') {
		printf("create_all_file_for_toc0 err no content match toc0\n");
		return -1;
	}

	printf("all_key_ladder_bin=%s\n", all_key_item_bin);
	if (GetPrivateProfileAllLine(all_key_item_bin, all_key_item_line)) {
		printf("create_all_file_for_toc0 err in GetPrivateProfileAllLine\n");
		return -1;
	}


	for (i = 0; i < 2; i++) {
	   if (all_key_item_line[i]) {

		memset(type_name, 0, 32);
		memset(line_info, 0, 256);
		GetPrivateProfileLineInfo(all_key_item_line[i], type_name, line_info);

		printf("type_name=%s\n", type_name);
		printf("line_info=%s\n", line_info);

		memset(item_name , 0, 64);
		memset(key_name , 0, 64);
		memset(toc0->item , 0, 64);
		GetPrivateEachItem(line_info, toc0->item, item_name, key_name);
		printf("item=%s\n", toc0->item);

		if (!strcmp(toc0->item, KEY_ITEM_BIN_PATH_CONST)) {
			printf("key0_name=%s\n", item_name);
			printf("key1_name=%s\n", key_name);

			sprintf(toc0->key0, "%s/%s", keypath, item_name);
			printf("key0_patch=%s\n", toc0->key0);
			sprintf(toc0->key1, "%s/%s", keypath, key_name);
			printf("key1_patch=%s\n", toc0->key1);

			ret = create_key_ladder(toc0);
			if (ret < 0) {
				printf("create_key_ladder err\n");
				return -1;
			}

		} else if (!strcmp(toc0->item, TOC0PATH_CONST)) {
			printf("boot_bin_name=%s\n", item_name);
			printf("key_name=%s\n", key_name);

			memset(current_path , 0, MAX_PATH);
			GetFullPath(current_path, TOC0PATH_CONST);
			sprintf(toc0->cert, "%s/%s", current_path, toc0->item);
			printf("toc0_cert_patch=%s\n", toc0->cert);

			GetFullPath(toc0->boot_bin, item_name);
			memset(hash_value, 0, 128);

			ret = calchash_in_hex(toc0->boot_bin, hash_value);
			if (ret < 0) {
				printf("create_all_file_for_toc0 err in calchash\n");
				return -1;
			}

			/*获取对应的key的公钥*/
			memset(key_n, 0, 560);
			memset(key_e, 0, 560);
			memset(key_d, 0, 560);
			memset(current_path, 0, MAX_PATH);
			sprintf(current_path, "%s.bin", toc0->key1);
			printf("key1 path=%s\n", current_path);
			ret = getallkey(current_path, key_n, key_e, key_d);
			if (ret < 0) {
				printf("dragoncreate_toc0_certif err in getpublickey bin\n");
				return -1;
			}
			printf("%s %d %s\n", __FILE__, __LINE__, __func__);

			ret = __merge_certif_for_toc0(hash_value, (u8 *)key_n, (u8 *)key_e, (u8 *)key_d, toc0->cert);
			if (ret < 0) {
				printf("dragoncreate_toc0_certif err in dragoncreatetoc0certif rootkey\n");
				return -1;
			}
			printf("toc0_cert_patch=%s\n", toc0->cert);
		}
	} else {
		;
		}
	}
return 1;
}

