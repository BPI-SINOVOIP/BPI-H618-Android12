/*
 * (C) Copyright 2016
 *Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include "types.h"
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

#define   MAX_PATH                   (260)

static void usage(void)
{
	printf("update_dtb <file name> <reserve size>\n");

	return ;
}

int IsFullName(const char *FilePath)
{
	if ( FilePath[0] == '/') {
		return 1;
	} else {
		return 0;
	}
}

void GetFullPath(char *dName, const char *sName)
{
	char Buffer[MAX_PATH];

	if (IsFullName(sName)) {
		strcpy(dName, sName);
		return ;
	}

	/* Get the current working directory: */
	if (getcwd(Buffer, MAX_PATH ) == NULL) {
		perror( "_getcwd error" );
		return ;
	}

	sprintf(dName, "%s/%s", Buffer, sName);
}

u32 randtoalign(u32 num, u32 align_size)
{
	if (num % align_size) {
		return ((num + align_size - 1) / align_size * align_size);
	} else {
		return num;
	}
}

int main(int argc, char *argv[])
{
	char file_name[MAX_PATH] = "";
	int res_size  = 0;
	FILE  *file = NULL;
	u32 len;
	u32 align_len;
	char *buffer;
	int ret = -1;

	if (argc != 3) {
		usage();
		return -1;
	}

	if (argv[1] == NULL || argv[2] == NULL) {
		printf("input para error!\n");
		ret = -1;
		goto out;
	}

	GetFullPath(file_name, argv[1]);
	res_size = atoi(argv[2]);

	file = fopen(file_name, "a+");

	if (!file) {
		printf("unable to open file %s\n", file_name);
		ret = -1;
		goto out;
	}

	fseek(file, 0, SEEK_END);
	len = ftell(file);

	if (res_size == 0) {
		ret = 0;
		goto out;
	}

	align_len = randtoalign(len + res_size, 512);
	res_size = align_len - len;

	buffer = (char *)malloc(res_size);

	if (!buffer) {
		printf("malloc %d bytes buffer fail!\n", res_size);
		ret = -1;
		goto out;
	}

	memset(buffer, 0, res_size);

	if (!fwrite(buffer, res_size, 1, file)) {
		printf("write to file fail\n");
		ret = -1;
		goto out;
	}

	fclose(file);
	ret = 0;

out:

	if (buffer)
		free(buffer);

	return ret;
}

