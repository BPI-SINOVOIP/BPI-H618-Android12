#include "sunxi_mbr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define pr_info(fmt, ...) printf(fmt, ##__VA_ARGS__)

//#define DEBUG

#ifdef DEBUG
#define pr_debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...)
#endif

int mbr_status;
static char mbr_buf[4];

/*************syspartition*****************/
unsigned int sunxi_partition_get_total_num(void)
{
	sunxi_mbr_t *mbr = (sunxi_mbr_t *)mbr_buf;
	if (!mbr_status) {
		return 0;
	}

	return mbr->PartCount;
}

int sunxi_partition_get_name(int index, char *buf)
{
	sunxi_mbr_t *mbr = (sunxi_mbr_t *)mbr_buf;

	if (mbr_status) {
		strncpy(buf, (const char *)mbr->array[index].name, 16);
	} else {
		memset(buf, 0, 16);
	}

	return 0;
}

uint sunxi_partition_get_offset(int part_index)
{
	sunxi_mbr_t *mbr = (sunxi_mbr_t *)mbr_buf;

	if ((!mbr_status) || (part_index >= mbr->PartCount)) {
		return 0;
	}

	return mbr->array[part_index].addrlo;
}

uint sunxi_partition_get_size(int part_index)
{
	sunxi_mbr_t *mbr = (sunxi_mbr_t *)mbr_buf;

	if ((!mbr_status) || (part_index >= mbr->PartCount)) {
		return 0;
	}

	return mbr->array[part_index].lenlo;
}

uint sunxi_partition_get_offset_byname(const char *part_name)
{
	sunxi_mbr_t *mbr = (sunxi_mbr_t *)mbr_buf;
	int i;

	if (!mbr_status) {
		return 0;
	}
	for (i = 0; i < mbr->PartCount; i++) {
		if (!strcmp(part_name, (const char *)mbr->array[i].name)) {
			return mbr->array[i].addrlo;
		}
	}

	return 0;
}

int sunxi_partition_get_partno_byname(const char *part_name)
{
	sunxi_mbr_t *mbr = (sunxi_mbr_t *)mbr_buf;
	int i;

	if (!mbr_status) {
		return -1;
	}
	for (i = 0; i < mbr->PartCount; i++) {
		if (!strcmp(part_name, (const char *)mbr->array[i].name)) {
			return i;
		}
	}

	return -1;
}

uint sunxi_partition_get_size_byname(const char *part_name)
{
	sunxi_mbr_t *mbr = (sunxi_mbr_t *)mbr_buf;
	int i;

	if (!mbr_status) {
		return 0;
	}
	for (i = 0; i < mbr->PartCount; i++) {
		if (!strcmp(part_name, (const char *)mbr->array[i].name)) {
			return mbr->array[i].lenlo;
		}
	}

	return 0;
}

/* get the partition info, offset and size
 * input: partition name
 * output: part_offset and part_size (in byte)
 */
int sunxi_partition_get_info_byname(const char *part_name, uint *part_offset,
				    uint *part_size)
{
	sunxi_mbr_t *mbr = (sunxi_mbr_t *)mbr_buf;
	int i;

	if (!mbr_status) {
		return -1;
	}

	for (i = 0; i < mbr->PartCount; i++) {
		if (!strcmp(part_name, (const char *)mbr->array[i].name)) {
			*part_offset = mbr->array[i].addrlo;
			*part_size = mbr->array[i].lenlo;
			return 0;
		}
	}

	return -1;
}

/******************syspartition***********************/

void __dump_mbr(sunxi_mbr_t *mbr_info)
{
	sunxi_partition *part_info;
	unsigned int i;
	char buffer[32];

	pr_info("*************MBR DUMP***************\n");
	pr_info("total mbr part %u\n", mbr_info->PartCount);
	pr_info("\n");
	for (part_info = mbr_info->array, i = 0; i < mbr_info->PartCount;
	     i++, part_info++) {
		memset(buffer, 0, 32);
		memcpy(buffer, part_info->name, 16);
		pr_info("part[%u] name      :%s\n", i, buffer);
		memset(buffer, 0, 32);
		memcpy(buffer, part_info->classname, 16);
		pr_info("part[%u] classname :%s\n", i, buffer);
		pr_info("part[%u] addrlo    :0x%x\n", i, part_info->addrlo);
		pr_info("part[%u] lenlo     :0x%x\n", i, part_info->lenlo);
		pr_info("part[%u] user_type :0x%x\n", i, part_info->user_type);
		pr_info("part[%u] keydata   :0x%x\n", i, part_info->keydata);
		pr_info("part[%u] ro        :0x%x\n", i, part_info->ro);
		pr_info("\n");
	}
}

int sunxi_partition_init(char *sunxi_mbr_fex)
{
	FILE *stream;
	stream = fopen(sunxi_mbr_fex, "r");
	fread(mbr_buf, sizeof(sunxi_mbr_t), 4, stream);
	fclose(stream);
	mbr_status = 1;
	return 0;
}

void Usage(void)
{
	printf("\n");
	printf("Usage:\n");
	printf("	parser_mbr sunxi_mbr.fex cmd cmd_arg\n\n");
	printf("	cmd:\n");
	printf("		get_total_num\n");
	printf("		get_name_by_index\n");
	printf("		get_offset_by_index\n");
	printf("		get_size_by_index\n");
	printf("		get_index_by_name\n");
	printf("		get_offset_by_name\n");
	printf("		get_size_by_name\n");
	printf("	cmd_arg:\n");
	printf("		name(eg:boot) or index(eg:1) or "
	       "empty(eg:get_total_num don't need cmd_arg)\n");
	return;
}

int main(int argc, char *argv[])
{
	int ret;
	unsigned int size, index, offset, total_num;
	char name_buff[16];
	if (argc == 3 && strcmp(argv[2], "get_total_num") == 0) {
		; // it's ok
	} else if (argc != 4) {
		Usage();
		return -1;
	}

	sunxi_partition_init(argv[1]);
#ifdef DEBUG
	__dump_mbr(&mbr_buff);
#endif
	if (strcmp(argv[2], "get_total_num") == 0) {
		total_num = sunxi_partition_get_total_num();
		pr_debug("total num : %u\n", total_num);
		printf("%u", total_num);
	} else if (strcmp(argv[2], "get_name_by_index") == 0) {
		index = atoi(argv[3]);
		ret = sunxi_partition_get_name(index, name_buff);
		pr_debug("part name of index %u, %s\n", index, name_buff);
		printf("%s", name_buff);
	} else if (strcmp(argv[2], "get_offset_by_index") == 0) {
		index = atoi(argv[3]);
		offset = sunxi_partition_get_offset(index);
		pr_debug("part offset of index %u, %x\n", index, offset);
		printf("%u", offset);
	} else if (strcmp(argv[2], "get_size_by_index") == 0) {
		index = atoi(argv[3]);
		size = sunxi_partition_get_size(index);
		pr_debug("part name of index %u, %x\n", index, size);
		printf("%u", size);
	} else if (strcmp(argv[2], "get_index_by_name") == 0) {
		index = sunxi_partition_get_partno_byname(argv[3]);
		pr_debug("index of name %s, %u\n", argv[3], index);
		printf("%u", index);
	} else if (strcmp(argv[2], "get_offset_by_name") == 0) {
		offset = sunxi_partition_get_offset_byname(argv[3]);
		pr_debug("offset of name %s, %u\n", argv[3], offset);
		printf("%u", offset);
	} else if (strcmp(argv[2], "get_size_by_name") == 0) {
		size = sunxi_partition_get_size_byname(argv[3]);
		pr_debug("size of name %s, %u\n", argv[3], size);
		printf("%u", size);
	}
	return 0;
}
