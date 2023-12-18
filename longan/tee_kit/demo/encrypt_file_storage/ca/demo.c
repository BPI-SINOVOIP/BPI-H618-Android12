/*
 * Copyright (c) 2016, Linaro Limited
 * Copyright (c) 2014, STMicroelectronics International N.V.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "libstorage.h"

static int sunxi_dump(void *addr, int size)
{
	int i,j;
	char *buf = (char *)addr;

	for (j = 0; j < size; j += 16) {
		for (i = 0; i < 16; i++) {
			printf("%02x ", buf[j+i] & 0xff);
		}
		printf("\n");
	}
	printf("\n");

	return  0;
}

#define OP_FLAG_READ 0
#define OP_FLAG_WRITE 1
#define OP_FLAG_DELETE 2
#define OP_FLAG_CREATE 3

static void usage(char *app)
{
	printf("usage: %s [options] [file name]\n", app);
	printf(" [options]:\n");
	printf(" \t-c\t create a file named [file name] to secure storage\n");
	printf(" \t-r\t read a file named [file name] from secure storage\n");
	printf(" \t-w\t write a file named [file name]  to secure storage\n");
	printf(" \t  \t content is 256 bytes random number\n");
	printf(" \t-d\t delete a file named [file name] from secure storage\n\n");
	printf(" [file name]: file name\n");
}

int main(int argc, char *argv[])
{
	uint32_t tee_res;

	OPTEE_FS_CONTEXT_HANDLER ctx;
	int opflag = -1;
	uint32_t obj = 0;
	int ret = 0;
	int i = 0;

	char *cmd = argv[1];
	char *file_name = argv[2];

	uint8_t random_data[256] = {0};
	uint32_t count = 0;

	if (argc != 3) {
		usage(argv[0]);
		return -1;
	}

	/* set opration flag */
	if (!strcmp(cmd, "-r")) {
		opflag = OP_FLAG_READ;
	} else if (!strcmp(cmd, "-w")) {
		opflag = OP_FLAG_WRITE;
	} else if (!strcmp(cmd, "-d")) {
		opflag = OP_FLAG_DELETE;
	} else if (!strcmp(cmd, "-c")) {
		opflag = OP_FLAG_CREATE;
	} else {
		opflag = -1;
		usage(argv[0]);
		return -1;
	}

	/*init context*/
	tee_res = optee_fs_init_ctx(&ctx);
	if (tee_res != 0) {
		fprintf(stderr, "test failed to init context: 0x%x\n", tee_res);
		ret = -1;
		return ret;
	}

	/* read, write, delete file */
	ret = optee_fs_open(ctx, file_name, &obj);
	if (ret != 0) {
		if (opflag == OP_FLAG_CREATE) {
			ret = optee_fs_create(ctx, file_name, &obj);
			if (ret != 0) {
				printf("Failed to optee_fs_create: %s, ret = 0x%x\n", file_name, ret);
				goto exit;
			}
		} else {
			printf("Failed to optee_fs_open: %s, ret = 0x%x\n", file_name, ret);
		}
		goto exit;
	} else {
		if (opflag == OP_FLAG_CREATE) {
			printf("file: %s is already exist!\n", file_name);
			goto exit;
		}
	}

	switch (opflag) {
	case OP_FLAG_READ:
		ret = optee_fs_read(ctx, obj, random_data, sizeof(random_data),
				    &count);
		if (ret != 0) {
			printf("Failed to fs read: %s, ret = 0x%x\n", file_name, ret);
		} else {
			printf("---- Read file:%s %u Bytes data: ----\n", file_name, count);
			sunxi_dump(random_data, sizeof(random_data));
			printf("---- Read file:%s end! ----\n", file_name);
		}
		break;
	case OP_FLAG_WRITE:
		/* generate 256 Bytes random data */
		srand((unsigned)time(NULL));
		for (i = 0; i < sizeof(random_data); i++)
			random_data[i] = rand() % 0xFF;

		ret = optee_fs_write(ctx, obj, random_data, sizeof(random_data));
		if (ret != 0) {
			printf("Failed to fs write: %s, ret = 0x%x\n", file_name, ret);
		} else {
			printf("---- Write file:%s with %d Bytes data: ----\n", file_name, sizeof(random_data));
			sunxi_dump(random_data, sizeof(random_data));
			printf("---- Write file:%s end! ----\n", file_name);
		}
		break;
	case OP_FLAG_DELETE:
		ret = optee_fs_unlink(ctx, obj);
		if (ret != 0) {
			printf("Failed to fs clean: %s, ret = 0x%x\n", file_name, ret);
		} else {
			printf("Delete file:%s !\n", file_name);
		}
		break;
	default:
		ret = -1;
		break;
	}

exit:
	/* finalize the context */
	optee_fs_finalize_ctx(&ctx);
	return ret;
}
