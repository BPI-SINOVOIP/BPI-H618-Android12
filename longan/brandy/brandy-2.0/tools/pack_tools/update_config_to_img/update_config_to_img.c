#include <malloc.h>
#include <string.h>
#include "types.h"
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include "firmware/imgdecode.h"
#include "firmware/imagefile_new.h"
#include "private_toc.h"

#define debug(fmt, args...)	//printf(fmt, ##args)

#define  MAX_PATH             	(260)
#define BUFFER_LEN			(2*1024*1024)
#define ITEM_COMMON					"COMMON  "
#define SUB_TOC_PKG_NAME				"BOOTPKG-00000000"
#define SUB_DTB_NAME					"DTB_CONFIG000000"
#define SUB_BOARD_CONFIG_NAME		"BOARD_CONFIG_BIN"

static void *imghd = NULL;
static void *imgitemhd = NULL;
static int is_last_item = 0;

extern int IsFullName(const char *FilePath);
extern void GetFullPath(char *dName, const char *sName);
extern int script(char *src_file);
extern int _get_str_length(char *str);
extern u32 randto1k(u32 num);

int img_probe(char *name)
{
	imghd = Img_Open(name);

	if (!imghd)
	{
		return -1;
	}

	return 0;
}

static int check_is_last_item(void)
{
	HIMAGEITEM bootpkg_itemhd;
	long long img_size, item_len;
	uint item_offset;

	if (imghd == NULL)
	{
		printf("imghd is NULL\n");
		return -1;
	}

	img_size = Img_GetSize(imghd);
	bootpkg_itemhd = Img_OpenItem(imghd, SUB_TOC_PKG_NAME);

	if (!bootpkg_itemhd)
	{
		printf("open %s item fail\n", SUB_TOC_PKG_NAME);
		return -1;
	}

	item_offset = Img_GetItemStart(imghd, bootpkg_itemhd);
	item_len = Img_GetItemSize(imghd, bootpkg_itemhd);

	if (item_offset + item_len != img_size)
	{
		debug(" is not last item!\n");
		is_last_item = 0;
	}
	else
	{
		debug(" is last item!\n");
		is_last_item = 1;
	}

	return is_last_item;
}

int gen_dtb_bin_file(char *buffer, uint len)
{
	FILE *pfile;

	if (!buffer || !len)
		return -1;

	pfile = fopen("sunxi.dtb", "wb");

	if (pfile == NULL)
	{
		printf("creat sunxi.dtb fail\n");
		return -1;
	}

	if (!fwrite(buffer, len, 1, pfile))
	{
		printf("write sunxi.dtb fail\n");
	}

	fclose(pfile);
	return 0;
}

char *probe_file_data(char *file_name, uint *file_len, int align_size)
{
	FILE *pfile;
	int   len;
	char *buffer;
	int    align_length;

	pfile = fopen(file_name, "rb");

	if (pfile == NULL)
	{
		printf("file %s cant be opened\n", file_name);

		return NULL;
	}

	fseek(pfile, 0, SEEK_END);
	len = ftell(pfile);

	if (len % align_size)
	{
		align_length  = ((len + align_size - 1) / align_size * align_size);
	}
	else
		align_length = len;

	buffer = malloc(align_length);

	if (buffer == NULL)
	{
		printf("%d buffer cant be malloc\n", align_length);
		fclose(pfile);

		return NULL;
	}

	memset(buffer, 0, align_length);

	fseek(pfile, 0, SEEK_SET);
	fread(buffer, len, 1, pfile);
	fclose(pfile);

	*file_len = align_length;
	return buffer;
}

uint gen_general_checksum(void *buff, uint length)
{
	uint             *buf;
	uint            loop;
	uint            i;
	uint            sum = 0;

	buf = (__u32 *)buff;
	loop = length >> 2;

	for ( i = 0, sum = 0;  i < loop;  i++ )
		sum += buf[i];

	return sum;
}

static char * update_dtb(char *config_file_name, uint *dtb_len)
{
	IMAGE_HANDLE* pImage = (IMAGE_HANDLE *)imghd;
	HIMAGEITEM itemhd;
	ITEM_HANDLE * pItem;
	long long img_size, item_size;
	char cmdline[1024];
	uint item_table_size;
	char* tmp_buf = NULL;
	char *dtb_buffer = NULL;

	itemhd  = Img_OpenItem(pImage, SUB_DTB_NAME);

	if (itemhd == NULL)
	{
		printf("open item %s fail\n", SUB_DTB_NAME);
		goto _out;
	}

	pItem  = (ITEM_HANDLE  *)itemhd;
	item_size = Img_GetItemSize(pImage, itemhd);
	img_size = Img_GetSize(imghd);

	tmp_buf = (char *)malloc(item_size);

	if (!tmp_buf)
	{
		printf("malloc tmp buf fail\n");
		goto _out;
	}

	memset(tmp_buf, 0, item_size);

	if (!Img_ReadItem(pImage, itemhd, tmp_buf, item_size))
	{
		printf("read item %s fail\n", SUB_DTB_NAME);
		goto _out;
	}

	if (gen_dtb_bin_file(tmp_buf, item_size))
		goto _out;

	sprintf(cmdline, "./dtc -I dtb sunxi.dtb -F %s -b 0 -O dtb -o new_sunxi.dtb", config_file_name);
	system(cmdline);

	dtb_buffer = probe_file_data("new_sunxi.dtb", dtb_len, 1024);

	if (!is_last_item)
	{
		/* set the original item buffer to 0 */
		memset(tmp_buf, 0, item_size);
		Img_WriteItem(pImage, itemhd, tmp_buf, item_size);

		/* update img header */
		pImage->ItemTable[pItem->index].offsetLo = (u32) (img_size & 0xFFFFFFFF);
		pImage->ItemTable[pItem->index].offsetHi = (u32)(img_size >> 32);
		img_size += *dtb_len;
	}
	else
	{
		img_size = img_size + *dtb_len - item_size;
	}

	free(tmp_buf);

	pImage->ItemTable[pItem->index].filelenLo = *dtb_len;
	pImage->ItemTable[pItem->index].datalenLo= *dtb_len;
	pImage->ImageHead.lenLo = (u32)(img_size & 0xFFFFFFFF);
	pImage->ImageHead.lenHi = (u32) (img_size >> 32);
	system("rm -rf sunxi.dtb");
	system("rm -rf new_sunxi.dtb");

	/* update to img */
	item_table_size = pImage->ImageHead.itemcount * sizeof(ImageItem_t);
	Img_WriteOffset(&pImage->ImageHead, 0, IMAGE_HEAD_SIZE);
	Img_WriteOffset(pImage->ItemTable, IMAGE_HEAD_SIZE, item_table_size);

	Img_WriteItem(pImage, itemhd, dtb_buffer, *dtb_len);

	return dtb_buffer;

_out:

	if (tmp_buf)
		free(tmp_buf);

	return NULL;
}

static char* update_board_config(char * config_file_name, uint *bd_len)
{
	IMAGE_HANDLE* pImage = (IMAGE_HANDLE *)imghd;
	HIMAGEITEM itemhd;
	ITEM_HANDLE * pItem;
	long long img_size, item_size;
	int src_length;
	char config_bin_name[1024];
	char cmdline[1024];
	uint item_table_size;
	char *tmp_buf;
	char *bd_buffer = NULL;

	itemhd  = Img_OpenItem(pImage, SUB_BOARD_CONFIG_NAME);

	if (itemhd == NULL)
	{
		printf("open item %s fail\n", SUB_BOARD_CONFIG_NAME);
		return 0;
	}

	pItem  = (ITEM_HANDLE  *)itemhd;
	item_size = Img_GetItemSize(pImage, itemhd);
	img_size = Img_GetSize(pImage);

	tmp_buf = (char *)malloc(item_size);

	if (!tmp_buf)
	{
		printf("malloc tmp buf fail\n");
		return NULL;
	}

	src_length = _get_str_length(config_file_name);
	memcpy(config_bin_name, config_file_name, src_length);
	config_bin_name[src_length - 0] = '\0';
	config_bin_name[src_length - 1] = 'n';
	config_bin_name[src_length - 2] = 'i';
	config_bin_name[src_length - 3] = 'b';

	bd_buffer = probe_file_data(config_bin_name, bd_len, 4);

	if (!is_last_item)
	{
		/* set the original itemoffset buffer to 0 */
		memset(tmp_buf, 0, item_size);
		Img_WriteItem(pImage, itemhd, tmp_buf, item_size);

		/* update img header */
		pImage->ItemTable[pItem->index].offsetLo = (u32) (img_size & 0xFFFFFFFF);
		pImage->ItemTable[pItem->index].offsetHi = (u32)(img_size >> 32);
		img_size += *bd_len;
	}
	else
	{
		img_size = img_size + *bd_len - item_size;
	}

	free(tmp_buf);

	pImage->ItemTable[pItem->index].filelenLo = *bd_len;
	pImage->ItemTable[pItem->index].datalenLo= *bd_len;
	pImage->ImageHead.lenLo = (u32)(img_size & 0xFFFFFFFF);
	pImage->ImageHead.lenHi = (u32) (img_size >> 32);

	/* update to img */
	item_table_size = pImage->ImageHead.itemcount * sizeof(ImageItem_t);
	Img_WriteOffset(&pImage->ImageHead, 0, IMAGE_HEAD_SIZE);
	Img_WriteOffset(pImage->ItemTable, IMAGE_HEAD_SIZE, item_table_size);

	Img_WriteItem(pImage, itemhd, bd_buffer, *bd_len);

	sprintf(cmdline, "rm -rf  %s", config_bin_name);
	system(cmdline);
	return bd_buffer;

}

static int update_bootpkg(char *new_dtb_buffer, uint dtb_len, char *bd_buffer, uint bd_len)
{
	IMAGE_HANDLE* pImage = (IMAGE_HANDLE *)imghd;
	HIMAGEITEM itemhd;
	ITEM_HANDLE * pItem;
	char *bootpkg_buffer = NULL;
	char *new_bootpkg_buffer = NULL;
	char *tmp_buf = NULL;
	long long img_size, item_size, offset;
	uint item_table_size;
	int i, ret = 0;
	struct sbrom_toc1_head_info  *toc1_head = NULL;
	struct sbrom_toc1_item_info  *item_head = NULL;
	struct sbrom_toc1_item_info  *toc1_item = NULL;

	if ((new_dtb_buffer == NULL) || (bd_buffer == NULL) || (dtb_len < 0) || (bd_len < 0))
	{
		printf("update_bootpkg:input para error\n");
		goto _out;
	}

	img_size = Img_GetSize(pImage);
	itemhd  = Img_OpenItem(pImage, SUB_TOC_PKG_NAME);

	if (itemhd == NULL)
	{
		printf("open item %s fail\n", SUB_TOC_PKG_NAME);
		goto _out;
	}

	pItem  = (ITEM_HANDLE  *)itemhd;
	item_size = Img_GetItemSize(pImage, itemhd);
	bootpkg_buffer = (char *)malloc(item_size);

	if (bootpkg_buffer == NULL)
	{
		printf("malloc bootpkg  buffer fail\n");
		goto _out;
	}

	if (!Img_ReadItem(pImage, itemhd, bootpkg_buffer, item_size))
	{
		printf("read item %s fail\n", SUB_TOC_PKG_NAME);
		goto _out;
	}

	toc1_head = (struct sbrom_toc1_head_info *)bootpkg_buffer;
	item_head = (struct sbrom_toc1_item_info *)(bootpkg_buffer + sizeof(struct sbrom_toc1_head_info));

	new_bootpkg_buffer = (char *)malloc(10 * 1024 * 1024);

	if (new_bootpkg_buffer == NULL)
	{
		printf("malloc new bootpkg  buffer fail\n");
		goto _out;
	}

	memset(new_bootpkg_buffer, 0, (10 * 1024 * 1024));

	/* copy toc header */
	offset = sizeof(struct sbrom_toc1_head_info) + toc1_head->items_nr * sizeof(struct sbrom_toc1_item_info);
	memcpy(new_bootpkg_buffer, bootpkg_buffer, offset);

	toc1_item =  (struct sbrom_toc1_item_info *)(new_bootpkg_buffer + sizeof(struct sbrom_toc1_head_info));

	for (i = 0; i < toc1_head->items_nr; i++, toc1_item++)
	{
		/* copy other item */
		if ((strncmp(toc1_item->name, ITEM_DTB_NAME, sizeof(ITEM_DTB_NAME))) &&
		    (strncmp(toc1_item->name, ITEM_BDCFG_NAME, sizeof(ITEM_BDCFG_NAME))))
		{
			offset = randto1k(offset);
			memcpy(new_bootpkg_buffer + offset, (bootpkg_buffer + toc1_item->data_offset), toc1_item->data_len);
			toc1_item->data_offset = offset;
			offset += toc1_item->data_len;
		}
		else if (strncmp(toc1_item->name, ITEM_DTB_NAME, sizeof(ITEM_DTB_NAME)) == 0)
		{
			/* copy dtb item */
			offset = randto1k(offset);
			memcpy(new_bootpkg_buffer + offset, new_dtb_buffer, dtb_len);
			toc1_item->data_offset  = offset;
			offset += dtb_len;
			offset = randto1k(offset);
			toc1_item->data_len = randto1k(dtb_len);
		}
		else if (strncmp(toc1_item->name, ITEM_BDCFG_NAME, sizeof(ITEM_BDCFG_NAME)) == 0)
		{
			/* copy board config item */
			offset = randto1k(offset);
			memcpy(new_bootpkg_buffer + offset, bd_buffer, bd_len);
			toc1_item->data_offset  = offset;
			offset += bd_len;
			offset = randto1k(offset);
			toc1_item->data_len = randto1k(bd_len);
		}
	}

	toc1_head = (struct sbrom_toc1_head_info *)new_bootpkg_buffer;
	toc1_head->valid_len = (offset + 16 * 1024 - 1) & (~(16 * 1024 - 1));
	toc1_head->add_sum = STAMP_VALUE;
	toc1_head->add_sum = gen_general_checksum(new_bootpkg_buffer, toc1_head->valid_len);

	if (!is_last_item)
	{

		/* set the original itemoffset buffer to zero */
		tmp_buf = (char *)malloc(item_size);

		if (!tmp_buf)
		{
			printf("malloc tmp_buf fail\n");
			goto _out;
		}

		memset(tmp_buf, 0, item_size);
		Img_WriteItem(pImage, itemhd, tmp_buf, item_size);

		/* update img header */
		pImage->ItemTable[pItem->index].offsetLo = (u32) (img_size & 0xFFFFFFFF);
		pImage->ItemTable[pItem->index].offsetHi = (u32)(img_size >> 32);
		img_size += toc1_head->valid_len;
	}
	else
	{
		img_size = img_size + toc1_head->valid_len - item_size;
	}

	pImage->ItemTable[pItem->index].filelenLo =  toc1_head->valid_len;
	pImage->ItemTable[pItem->index].datalenLo=  toc1_head->valid_len;
	pImage->ImageHead.lenLo = (u32)(img_size & 0xFFFFFFFF);
	pImage->ImageHead.lenHi = (u32) (img_size >> 32);

	/* update to img */
	item_table_size = pImage->ImageHead.itemcount * sizeof(ImageItem_t);
	Img_WriteOffset(&pImage->ImageHead, 0, IMAGE_HEAD_SIZE);
	Img_WriteOffset(pImage->ItemTable, IMAGE_HEAD_SIZE, item_table_size);

	ret = Img_WriteItem(pImage, itemhd, new_bootpkg_buffer, toc1_head->valid_len);

_out:

	if (bootpkg_buffer)
		free(bootpkg_buffer);

	if (new_bootpkg_buffer)
		free(new_bootpkg_buffer);

	if (tmp_buf)
		free(tmp_buf);

	return ret;
}

static int update_img(char *config_file_name)
{
	char *dtb_buffer = NULL;
	char *bd_buffer = NULL;
	uint dtb_len = 0, bd_len = 0;
	int ret = -1;

	dtb_buffer  = update_dtb(config_file_name, &dtb_len);
	if (!dtb_buffer)
	{
		printf("***update dtb fail***\n");
		goto _out;
	}

	bd_buffer = update_board_config(config_file_name, &bd_len);
	if (!bd_buffer )
	{
		printf("***update board config fail***\n");
		goto _out;
	}

	if (!update_bootpkg(dtb_buffer, dtb_len, bd_buffer, bd_len))
	{
		printf("***update bootpkg fail***\n");
		goto _out;
	}

	ret = 0;

_out:

	if (dtb_buffer)
		free(dtb_buffer);

	if (bd_buffer)
		free(bd_buffer);

	return ret;
}

void Usage(void)
{
	printf("\n");
	printf("Usage:\n");
	printf("update_sysconfig  img_file  sysconfig_file\n");
}

int main(int argc, char* argv[])
{
	char   img_name[MAX_PATH];
	char   config_name[MAX_PATH];
	char  cmdline[1024];
	int ret = -1;

	if (argc == 3)
	{
		if ((argv[1] == NULL) || (argv[2] == NULL) )
		{
			printf(" one of the input file names is empty\n");
			Usage();
			goto _out;
		}
	}
	else
	{
		Usage();
		goto _out;
	}

	GetFullPath(img_name,  argv[1]);
	GetFullPath(config_name,   argv[2]);

	/* convert sysconfig file to bin file*/
	sprintf(cmdline, "busybox unix2dos  %s", config_name);
	system(cmdline);
	if (script(config_name))
	{
		printf("convert sysconfig file to bin file fail\n");
		goto _out;
	}

	if (img_probe(img_name))
	{
		printf("probe img file fail\n");
		goto _out;
	}

	check_is_last_item();

	if (update_img(config_name))
	{
		printf("update sysconfig fail\n");
		ret = -1;
		goto _out;
	}

	printf("\n----------update sysconfig finish----------\n");
	ret = 0;

_out:

	if (imghd)
	{
		Img_Close(imghd);
	}

	if (imgitemhd)
	{
		Img_CloseItem(imghd, imgitemhd);
	}

	return ret;
}


