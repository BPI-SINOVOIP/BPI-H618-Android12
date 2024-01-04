#include "type_def.h"
#include "toc0/SBrom_Toc.h"
#include <string.h>

//========================================================
//just check valid
s32 toc0_init(BufferExtractCookie* cookie ,struct SBROM_TOC_total * p_total)
{
	struct SBROM_TOC_main_info * 	p_main = NULL;
	char *p_data=cookie->buffer;

	if(p_data == NULL || p_total == NULL){
		printf("PANIC : Toc_init() : input NULL\n");
		return -1;
	}
	memset(p_total , 0,sizeof(struct SBROM_TOC_total));

	p_main = (struct  SBROM_TOC_main_info *)p_data;
	//save info
	p_total->p_Toc_addr = p_data;
	memcpy((u8 *)&(p_total->main_info),p_data,sizeof(struct SBROM_TOC_main_info));

	return 0;
}

s32 Toc_item_open(struct SBROM_TOC_total * p_total ,
						u32 entry_id ,
						u8 ** p_item_addr ,
						u32 * p_item_len , 
						struct SBROM_TOC_item_entry * p_toc_item)
{
	s32 i = 0;
	struct SBROM_TOC_item_entry * p_item = NULL;

	*p_item_addr = NULL;
	*p_item_len = 0;

	p_item = (struct SBROM_TOC_item_entry *)((u32)(p_total->p_Toc_addr) + sizeof(struct SBROM_TOC_main_info));

	for(i == 0; i < p_total->main_info.Toc_items_nr; i++){
		if(p_item[i].Entry_name == entry_id){
			*p_item_addr = p_total->p_Toc_addr + p_item[i].Entry_data_offset;
			*p_item_len = p_item[i].Entry_data_len;
			
			*p_toc_item = p_item[i];

			return 0;
		}
	}
	return -1;
}


