/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * Allwinner boot verify trust-chain
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "private_toc.h"
#include "type_def.h"
#include "toc1/openssl_ext.h"
#include "sbrom_toc.h"
#include "spare_head.h"

#include "sunxi_boot_api.h"
void sunxi_dump(u8 *buf, int count)
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
static int check_public_in_rootcert(const char *name, sunxi_certif_info_t *sub_certif, void *toc1_buf )
{
	struct sbrom_toc1_item_info  *toc1_item;
	sunxi_certif_info_t  root_certif;
	u8 *buf;
	int ret, i;

	BufferExtractCookie *cookie = (BufferExtractCookie *)malloc( sizeof(BufferExtractCookie) );
        cookie->buffer = (BufferExtractCookie *)toc1_buf;
	toc1_item = (struct sbrom_toc1_item_info *)(cookie->buffer+ \
							sizeof(struct sbrom_toc1_head_info));
	/*Parse root certificate*/
	buf = (u8 *)(cookie->buffer+ toc1_item->data_offset);
	ret =  sunxi_certif_probe_ext(&root_certif, buf, toc1_item->data_len );
	if(ret < 0)
	{
                free(cookie);
		printf("fail to create root certif\n");
		return -1;
	}

	for(i=0;i<root_certif.extension.extension_num;i++)
	{
		if(!strcmp((const char *)root_certif.extension.name[i], name))
		{
			printf("find %s key stored in root certif\n", name);

			if(memcmp(root_certif.extension.value[i],
						sub_certif->pubkey.n+1, sub_certif->pubkey.n_len-1))
			{
				printf("%s key n is incompatible\n", name);
				printf(">>>>>>>key in rootcertif<<<<<<<<<<\n");
				sunxi_dump((u8 *)root_certif.extension.value[i], sub_certif->pubkey.n_len-1);
				printf(">>>>>>>key in certif<<<<<<<<<<\n");
				sunxi_dump((u8 *)sub_certif->pubkey.n+1, sub_certif->pubkey.n_len-1);

                                free(cookie);
				return -1;
			}
			if(memcmp(root_certif.extension.value[i] + sub_certif->pubkey.n_len-1,
						sub_certif->pubkey.e, sub_certif->pubkey.e_len))
			{
				printf("%s key e is incompatible\n", name);
				printf(">>>>>>>key in rootcertif<<<<<<<<<<\n");
				sunxi_dump((u8 *)root_certif.extension.value[i] + sub_certif->pubkey.n_len-1, sub_certif->pubkey.e_len);
				printf(">>>>>>>key in certif<<<<<<<<<<\n");
				sunxi_dump((u8 *)sub_certif->pubkey.e, sub_certif->pubkey.e_len);

				free(cookie);
				return -1;
			}
			break;
		}
	}

	free(cookie);
	return 0 ;

}

int sunxi_verify_embed_signature(void *buff, void* toc1_buf, uint len, const char *cert_name, void *cert, unsigned cert_len)
{
    u8 hash_of_file[256];
	int ret;
	sunxi_certif_info_t  sub_certif;
	void *cert_buf;

	cert_buf = malloc(cert_len);
	if(!cert_buf){
		printf("out of memory\n");
		return -1;
	}
	memcpy(cert_buf, cert,cert_len);

	memset(hash_of_file, 0, 32);
	//sunxi_ss_open();
	ret = sunxi_sha256_calc(hash_of_file, buff, len);
	if(ret)
	{
		printf("sunxi_verify_signature err: calc hash failed\n");
		free(cert_buf);

		return -1;
	}

	//printf("cert dump\n");
	//sunxi_dump(cert_buf,cert_len);
	if(sunxi_certif_verify_itself(&sub_certif, cert_buf, cert_len)){
		printf("%s error: cant verify the content certif\n", __func__);
		free(cert_buf);
		return -1;
	}

	if(memcmp(hash_of_file, sub_certif.extension.value[0], 32))
	{
		printf("hash compare is not correct\n");
		printf(">>>>>>>hash of file<<<<<<<<<<\n");
		sunxi_dump(hash_of_file, 32);
		printf(">>>>>>>hash in certif<<<<<<<<<<\n");
		sunxi_dump(sub_certif.extension.value[0], 32);

		free(cert_buf);
		return -1;
	}
	/*Approvel certificate by trust-chain*/
        /*Use rootcert from toc1.fex in ota package*/
	if( check_public_in_rootcert(cert_name, &sub_certif, toc1_buf) ){
		printf("check rootpk[%s] in rootcert fail\n",cert_name);
		free(cert_buf);
		return -1;
	}
	free(cert_buf);

	return 0;
}

