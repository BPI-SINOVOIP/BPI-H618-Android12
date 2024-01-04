#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "toc0/SBrom_Toc.h"
#include "toc0/SBrom_Certif.h"


int toc0_traverse(BufferExtractCookie* cookie)
{
	struct SBROM_TOC_total Toc_total;
	s32 ret = 0;
	u8 * p_item_addr = NULL;
	u32  item_len = 0;
	struct x509_total_info x509_info;
	struct SBROM_TOC_item_entry item_entry;

	memset(&Toc_total , 0 ,sizeof(struct SBROM_TOC_total));
	memset(&x509_info,0,sizeof(x509_info));
	memset(&item_entry,0,sizeof(item_entry));

	ret = toc0_init(cookie,&Toc_total);
	if(ret < 0){
	    printf("Parser_ Toc0(): Toc_init ,error\n");
	    goto Toc0_fail;
	}

	//get & vetify  Toc0  certif
        if(Toc_total.main_info.Toc_items_nr == 3) {
            printf("In %s: toc0 with two key!\n", __func__);
            ret =  key_ladder_toc0_traverse(cookie);
            return ret;
        }
        else {
            printf("In %s: toc0 with one key!\n", __func__);
            ret=Toc_item_open(  &Toc_total,
                                ITEM_NAME_SBROMSW_CERTIF,
                                &p_item_addr,
                                &item_len,
                                &item_entry);
            if(ret < 0){
                printf("Parser_ Toc0(): Toc open certif error\n");
                goto Toc0_fail;
            }

            //parser certif
            ret = certif_open(p_item_addr, item_len, &x509_info);

            //check certif
            ret = certif_verify(&x509_info);
            if(ret <= 0){
                printf("Parser_ Toc0(): Toc certif verify ,error\n");
                goto Toc0_fail;
            }
            else if( ret == EFUSE_IS_EMPTY )
            {
                return 0;
            }

            //check  SBROMSW
            Toc_item_open(  &Toc_total,
                            ITEM_NAME_SBROMSW_FW,
                            &p_item_addr,
                            &item_len,
                            &item_entry);
            //check
            ret = check_SBROMSW_hash(&x509_info, p_item_addr, item_len);
            if(ret <= 0){
                printf("Parser_ Toc0(): Toc SBROMSW verify ,error\n");
                goto Toc0_fail;
            }

            return 0;
        }

Toc0_fail:
	return -1;
}
