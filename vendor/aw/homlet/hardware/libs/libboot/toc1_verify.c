#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "libboot.h"
#include "libboot_recovery.h"
#include "sunxi_boot_api.h"
#include "private_uboot.h"
#include "private_boot0.h"
#include "private_toc.h"
#include "type_def.h"

#include "toc1/openssl_ext.h"
#include "sbrom_toc.h"
#include "spare_head.h"

#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))

static int sunxi_root_certif_pk_verify(sunxi_certif_info_t *sunxi_certif, u8 *buf, u32 len);
//extern void sunxi_certif_mem_reset(void);
//extern int sunxi_certif_probe_pubkey(X509 *x, sunxi_key_t *pubkey);


typedef struct _toc_name_addr_tbl
{
	char name[32];
	uint addr;
}toc_name_addr_tbl;

//addr is 1 just to indicate that this item no need verify
static toc_name_addr_tbl toc_name_addr[] = {
	{ITEM_SCP_NAME,1},
	{ITEM_PARAMETER_NAME,1},
	{ITEM_ESM_IMG_NAME,1},
	{ITEM_LOGO_NAME,1},
	{ITEM_SHUTDOWNCHARGE_LOGO_NAME,1},
	{ITEM_ANDROIDCHARGE_LOGO_NAME,1},
	{ITEM_DTB_NAME,1},
	{ITEM_SOCCFG_NAME,1},
	{ITEM_BDCFG_NAME,1},
	{ITEM_BDCFG_FEX_NAME,1}
};

void ndump(u8 *buf, int count)
{
	int i,j=0;
	int rest;
	char c;

	rest = count;
	if(count > 16)
	{
		for(j=0;j<count-16;j+=16)
		{
			for(i=0;i<16;i++)
			{
				c = buf[j+i] & 0xff;
				printf("%02x  ", c);
			}
			rest -= 16;
			printf("\n");
		}
	}
	for(i=0;i<rest;i++)
	{
		c = buf[j+i] & 0xff;
		printf("%02x  ", c);
	}

	printf("\n");
}

static int get_item_addr(char* name, uint* addr)
{
	uint i = 0;
	for(i = 0; i < ARRAY_SIZE(toc_name_addr); i++)
	{
		if(0 == strcmp(toc_name_addr[i].name, name))
		{
			*addr =  toc_name_addr[i].addr;
			return 0;
		}
	}
	return -1;
}




#define  SUNXI_X509_CERTIFF_MAX_LEN   (4096)
int toc1_traverse(BufferExtractCookie* cookie)
{
	sbrom_toc1_item_group item_group;
	int ret;
	uint len, i;
	u8 buffer[SUNXI_X509_CERTIFF_MAX_LEN];
	char*binfile_buffer=NULL; 

	sunxi_certif_info_t  root_certif;
	sunxi_certif_info_t  sub_certif;
	u8  hash_of_file[256];

    toc1_init(cookie);

	if (toc1_item_traverse())
		return -1;

	printf("probe root certif\n");

	memset(buffer, 0, SUNXI_X509_CERTIFF_MAX_LEN);
	len = toc1_item_read_rootcertif(buffer, SUNXI_X509_CERTIFF_MAX_LEN);
	if(!len)
	{
		printf("%s error: cant read rootkey certif\n", __func__);

		return -1;
	}
	if(sunxi_root_certif_pk_verify(&root_certif, buffer, len))
	{
		printf("certif invalid: root certif verify itself failed\n");

		return -1;
	}
	else
	{
		printf("certif valid: the root key is valid\n");
	}
	if(sunxi_certif_verify_itself(&root_certif, buffer, len))
	{
		printf("certif invalid: root certif verify itself failed\n");

		return -1;
	}
	do
	{
		memset(&item_group, 0, sizeof(sbrom_toc1_item_group));
		ret = toc1_item_probe_next(&item_group);
		if(ret < 0)
		{
			printf("sbromsw_toc1_traverse err in toc1_item_probe_next\n");

			return -1;
		}
		else if(ret == 0)
		{
			printf("sbromsw_toc1_traverse find out all items\n");
			printf("toc1 verify success\n");

			return 0;
		}
		if(item_group.bin_certif)
		{
			memset(buffer, 0, SUNXI_X509_CERTIFF_MAX_LEN);
			len = toc1_item_read(item_group.bin_certif, buffer, SUNXI_X509_CERTIFF_MAX_LEN);
			if(!len)
			{
				printf("%s error: cant read content key certif\n", __func__);

				return -1;
			}
			if(sunxi_certif_verify_itself(&sub_certif, buffer, len))
			{
				printf("%s error: cant verify the content certif\n", __func__);

				return -1;
			}
			for(i=0;i<root_certif.extension.extension_num;i++)
			{
				if(!strcmp((const char *)root_certif.extension.name[i], item_group.bin_certif->name))
				{
					printf("find %s key stored in root certif\n", item_group.bin_certif->name);
					if(memcmp(root_certif.extension.value[i], sub_certif.pubkey.n+1, sub_certif.pubkey.n_len-1))
					{
						printf("%s key n is incompatible\n", item_group.bin_certif->name);
						printf(">>>>>>>key in rootcertif<<<<<<<<<<\n");
						ndump((u8 *)root_certif.extension.value[i], sub_certif.pubkey.n_len-1);
						printf(">>>>>>>key in certif<<<<<<<<<<\n");
						ndump((u8 *)sub_certif.pubkey.n+1, sub_certif.pubkey.n_len-1);

						return -1;
					}
					if(memcmp(root_certif.extension.value[i] + sub_certif.pubkey.n_len-1, sub_certif.pubkey.e, sub_certif.pubkey.e_len))
					{
						printf("%s key e is incompatible\n", item_group.bin_certif->name);
						printf(">>>>>>>key in rootcertif<<<<<<<<<<\n");
						ndump((u8 *)root_certif.extension.value[i] + sub_certif.pubkey.n_len-1, sub_certif.pubkey.e_len);
						printf(">>>>>>>key in certif<<<<<<<<<<\n");
						ndump((u8 *)sub_certif.pubkey.e, sub_certif.pubkey.e_len);

						return -1;
					}
					break;
				}
			}
			if(i==root_certif.extension.extension_num)
			{
				printf("cant find %s key stored in root certif", item_group.bin_certif->name);

				return -1;
			}
		}

		if(item_group.binfile)
		{
			uint addr = 0;
			if(0 == get_item_addr(item_group.binfile->name, &addr))
			{
		         //this item no need verify
			}
			else
			{
				binfile_buffer=(char *)malloc(item_group.binfile->data_len+511);
				memset(binfile_buffer, 0, item_group.binfile->data_len+511);
				//读出bin文件内容到内存
				len = toc1_item_read(item_group.binfile, (void *)binfile_buffer,item_group.binfile->data_len);
				if(!len)
				{
					printf("%s error: cant read bin file\n", __func__);

					return -1;
				}
				//计算文件hash
				memset(hash_of_file, 0, sizeof(hash_of_file));
				//ret = sunxi_sha_calc(hash_of_file, sizeof(hash_of_file), (u8 *)item_group.binfile->run_addr, item_group.binfile->data_len);
				ret = sunxi_sha256_calc(hash_of_file,binfile_buffer, item_group.binfile->data_len);
				if(ret)
				{
					printf("sunxi_sha_calc: calc sha256 with hardware err\n");

					return -1;
				}
				//使用内容证书的扩展项，和文件hash进行比较
				//开始比较文件hash(小机端阶段计算得到)和证书hash(PC端计算得到)
				if(memcmp(hash_of_file, sub_certif.extension.value[0], 32))
				{
					printf("%s:hash compare is not correct\n",item_group.binfile->name);
					printf(">>>>>>>hash of file<<<<<<<<<<\n");
					ndump((u8 *)hash_of_file, 32);
					printf(">>>>>>>hash in certif<<<<<<<<<<\n");
					ndump((u8 *)sub_certif.extension.value[0], 32);

					return -1;
				}
				free(binfile_buffer);

			}
		}
	}
	while(1);

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
#define RSA_BIT_WITDH 2048
static int sunxi_certif_pubkey_check( sunxi_key_t  *pubkey )
{
	char efuse_hash[256]={0} , rotpk_hash[256];
	char all_zero[32];
    int ret;
	char pk[RSA_BIT_WITDH/8 * 2 + 256]; /*For the stupid sha padding */

	ret=read_rotpk(efuse_hash);
	if(ret==0)
	{
		printf("read rotpk error\n");
		return -1;
	}
	memset(all_zero, 0, 32);
	if( ! memcmp(all_zero, efuse_hash,32 ) )
		return 0 ; /*Don't check if rotpk efuse is empty*/
	else{
		memset(pk, 0x91, sizeof(pk));
		char *align = (char *)(((u32)pk+31)&(~31));
		if( *(pubkey->n) ){
			memcpy(align, pubkey->n, pubkey->n_len);
			memcpy(align+pubkey->n_len, pubkey->e, pubkey->e_len);
		}else{
			memcpy(align, pubkey->n+1, pubkey->n_len-1);
			memcpy(align+pubkey->n_len-1, pubkey->e, pubkey->e_len);
		}

		if(sunxi_sha256_calc( (u8 *)rotpk_hash, (u8 *)align, RSA_BIT_WITDH/8*2 ))
		{
			printf("sunxi_sha_calc: calc  pubkey sha256 with hardware err\n");
			return -1;
		}

		if(memcmp(rotpk_hash, efuse_hash, 32)){
			printf("certif pk dump\n");
			ndump((u8 *)align , RSA_BIT_WITDH/8*2 );

			printf("calc certif pk hash dump\n");
			ndump((u8 *)rotpk_hash,32);

			printf("efuse pk dump\n");
			ndump((u8 *)efuse_hash,32);

			printf("sunxi_certif_pubkey_check: pubkey hash check err\n");
			return -1;
		}
		return 0 ;
	}

}
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :  buf: 证书存放起始   len：数据长度
*
*    return        :
*
*    note          :  证书自校验
*
*
************************************************************************************************************
*/
static int sunxi_root_certif_pk_verify(sunxi_certif_info_t *sunxi_certif, u8 *buf, u32 len)
{
	X509 *certif;
	int  ret;

	//内存初始化
	sunxi_certif_mem_reset();
	//创建证书
	ret = sunxi_certif_create(&certif, buf, len);

	if(ret < 0)
	{
		printf("fail to create a certif\n");

		return -1;
	}
	//获取证书公钥
	ret = sunxi_certif_probe_pubkey(certif, &sunxi_certif->pubkey);
	if(ret)
	{
		printf("fail to probe the public key\n");

		return -1;
	}

/*
	ret = sunxi_certif_pubkey_check(&sunxi_certif->pubkey);
	if(ret){
		printf("fail to check the public key hash against efuse\n");

		return -1;
	}
*/

	sunxi_certif_free(certif);

	return 0;
}

