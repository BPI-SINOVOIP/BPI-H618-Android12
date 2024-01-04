/*
 * Copyright(c) 2013-2016 Allwinnertech Co., Ltd.
 *
 *		http://www.allwinnertech.com
 *		Author: Ryan <ryanchen@allwinnertech.com>
 *
 * Allwinner IP protction library
 *
 */
#include <stdio.h>
#include <string.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/bn.h>
#include "type_def.h"

#define RSA_PK_N_MAX 512
#define RSA_PK_E_MAX 512

static void __rsa_padding(u8 *dst_buf, u8 *src_buf, u32 data_len, u32 group_len)
{
	int i = 0;

	memset(dst_buf, 0, group_len);
	for(i = group_len - data_len; i < group_len; i++)
	{
		dst_buf[i] = src_buf[group_len - 1 - i];
	}
}

static void get_hashofsign(u8 *dst_buf, u8 *src_buf, u32 src_len)
{
	int i = 0;

	memset(dst_buf, 0, 32);
	for(i = 0; i < 32; i++)
	{
		dst_buf[32-1-i] = src_buf[src_len - 1 - i];
	}
}



int  sunxi_sha256_calc(char *dst_buf,
					char *src_buf, unsigned int  len)

{
	unsigned int ofset ;
    unsigned int bufSize = 256 ;
    SHA256_CTX sha256;

	/*if( len % bufSize ){
		printf("ERROR: sha256 length is not %d align\n", bufSize);
		return -1 ;
	}*/

    SHA256_Init(&sha256);
    SHA256_Update(
		&sha256,
		(void *)(src_buf),
		len);
    SHA256_Final(dst_buf, &sha256);

    return 0;
}

int sunxi_rsa2048_calc(char *dst_buf,
				   char *pk_e, unsigned int pk_e_len,
				   char *pk_n, unsigned int pk_n_len,
				   char *src_buf, unsigned int len)
{
	int outlen = 0;
	int i=0;
	char outbuf[256];
	u8 *n = NULL;
	u8 *e = NULL;
	n = malloc(RSA_PK_N_MAX + 1);
	e = malloc(RSA_PK_E_MAX);
	int cp_size = pk_n_len;
	RSA *key = NULL;
	key = RSA_new();
	if (!(src_buf&&dst_buf&&pk_n))
	{
		printf("input NULL\n");
		if (n) {
			free(n);
		}
		if (e) {
			free(e);
		}
		return -1;
	}

	if (!(key&&n&&e))
	{
		printf("malloc error\n");
		goto errout;
	}

	memset(n,0,sizeof(n));
	if(pk_n[0] >= 0x80)
	{
		memcpy(n+1,pk_n,pk_n_len);
		cp_size++;
	}
	else
	{
		memcpy(n,pk_n,pk_n_len);
	}
	memset(e,0,sizeof(e));
	memcpy(e,pk_e,pk_e_len);
	key->n = BN_bin2bn(n, cp_size, key->n);
	key->e = BN_bin2bn(e, pk_e_len, key->e);

    outlen = RSA_public_decrypt(len, src_buf, outbuf, key,RSA_NO_PADDING);
	//printf("RSA_public_decrypt end \n \noutlen is %d \n\n",outlen);
	get_hashofsign(dst_buf,outbuf,outlen);
	//for(i=0;i<32;i++)
	//	printf("rsa dst:%x\n",dst_buf[i]);
errout:
	if(n)
		free(n);
	if(e)
		free(e);
	if(key)
		RSA_free(key);
	return 0;
}




