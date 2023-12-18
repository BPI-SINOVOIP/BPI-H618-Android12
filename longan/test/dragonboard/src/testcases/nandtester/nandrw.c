/*
 * \file        nandrw.c
 * \brief
 *
 * \version     2.0.0
 * \date        2014年10月17日
 * \author      luoweijian<luoweijian@allwinnertech.com>
 *
 * \date        2022年04月19日
 * \author      cuizhikui<cuizhikui@allwinnertech.com>
 *
 * Copyright (c) 2022 Allwinner Technology. All Rights Reserved.
 *
 */

#include <string.h>
#include <errno.h>
#include "dragonboard_inc.h"
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

/* just define an ioctl cmd for nand test*/
#define DRAGON_BOARD_TEST    _IO('V',55)
int raw_test_nand(char *node)
{
	int ret = -1;
	int fd = -1;

	fd = open(node, O_RDWR);
	if (fd < 0) {
		db_error("can't open %s(%s)\n", node, strerror(errno));
		goto out;
	}

	ret = ioctl(fd, DRAGON_BOARD_TEST);
	if(ret < 0) {
		db_error("error in ioctl(%s)\n", strerror(errno));
	}

out:
	return ret;
}

int file_test_nand(char *file, int size)
{
    FILE *fp = NULL;
    int retval = -1;
    unsigned char *wbuffer = NULL;
    unsigned char *rbuffer = NULL;
    const int len = 4;
    int rest_len;
    int i, j;

    /* allocate memory for write */
    wbuffer = malloc(len << 20);
    if (wbuffer == NULL) {
        db_error("can't allocate memory(%s) for write\n", strerror(errno));
    }

    for (i = 0; i < (len << 20); i += 256) {
        for (j = 0; j < 256; j++) {
            *(wbuffer + i + j) = (unsigned char)j;
        }
    }

    /* allocate memory for read */
    rbuffer = malloc(len << 20);
    if (rbuffer == NULL) {
        db_error("can't allocate memory(%s) for read\n", strerror(errno));
        retval = -1;
        goto out;
    }

    /* open file */
    fp = fopen(file, "wb+");
    if (fp == NULL) {
        db_error("can't open %s(%s)\n", file, strerror(errno));
        retval = -1;
        goto out;
    }

    /* write 0~256 to file */
    for (i = 0; i < size / len; i++) {
        int nwrite = len << 20;
        if (fwrite(wbuffer, 1, nwrite, fp) < nwrite) {
            db_error("error in write(%s)\n", strerror(errno));
            retval = -1;
            goto out;
        }
    }

    rest_len = (size % len) << 20;
    if (rest_len) {
        if (fwrite(wbuffer, 1, rest_len, fp) < rest_len) {
            db_error("error in write(%s)\n", strerror(errno));
            retval = -1;
            goto out;
        }
    }

    /* flush */
    if (fflush(fp) == EOF) {
        db_error("error in flush(%s)\n", strerror(errno));
        retval = -1;
        goto out;
    }

    /* read back and compare */
    fseek(fp, 0, SEEK_SET);
    for (i = 0; i < size / len; i++) {
        int nread = len << 20;
        memset(rbuffer, 0, nread);
        fread(rbuffer, 1, nread, fp);

        if (memcmp(rbuffer, wbuffer, nread)) {
            db_error("data error!!!\n");
            retval = -1;
            goto out;
        }
    }

    rest_len = (size % len) << 20;
    if (rest_len) {
        memset(rbuffer, 0, len << 20);
        fread(rbuffer, 1, rest_len, fp);

        if (memcmp(rbuffer, wbuffer, rest_len)) {
            db_error("data error!!!\n");
            retval = -1;
            goto out;
        }
    }

    /* TEST OK */
    retval = 0;

out:
    if (wbuffer)
        free(wbuffer);
    if (rbuffer)
        free(rbuffer);
    if (fp)
        fclose(fp);

    return retval;
}

int main(int argc, char *argv[])
{
	char filename[256];
	int size;
	int retval = 0;

	if (argc == 2) {
		strncpy(filename, argv[1], 256);
		retval = raw_test_nand(filename);
	} else if (argc == 3){
		strncpy(filename, argv[1], 256);
		size = atoi(argv[2]);
		retval = file_test_nand(filename, size << 20);
	} else {
		db_error("Usage: nandrw FILE SIZE(MB)\n");
		retval = -1;
		goto out;
	}

out:
	return retval;
}
