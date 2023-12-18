/*
**********************************************************************************************************************
*											        eGon
*						                     the Embedded System
*									       boot pack sub-system
*
*						  Copyright(C), 2006-2017, SoftWinners Microelectronic Co., Ltd.
*                                           All Rights Reserved
*
* File    : file.c
*
* Version : V2.00
*
* Date	  :
*
* Descript:
**********************************************************************************************************************
*/
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>

#include "sunxi_mbr.h"
#include "script.h"
#include "types.h"
#include "file.h"
#include "part.h"

#define	FILL_DATA_SIZE 			SZ_8K

#define _FILE_OFFSET_BITS		64

int get_file_name(char *path, char *name)
{
	char buff_temp[MAX_PATH];
	char buffer[MAX_PATH];
	int  i, length;
	char *pt;

	memset(buff_temp, 0, MAX_PATH);
	memset(buffer, 0, MAX_PATH);
	if (!IsFullName(path)) {
		if (getcwd(buff_temp, MAX_PATH) == NULL) {
			perror("getcwd error");
			return -1;
		}
		sprintf(buffer, "%s/%s", buff_temp, path);
	} else {
		strcpy(buffer, path);
	}

	length = strlen(buffer);
	pt = buffer + length - 1;

	while ((*pt != '/') && (*pt != '\\')) {
		pt--;
	}
	i = 0;
	pt++;
	while (*pt) {
		if (*pt == '.') {
			name[i++] = '.';
			pt++;
		} else if ((*pt >= 'a') && (*pt <= 'z')) {
			name[i++] = *pt++; /* - ('a' - 'A');*/
		} else {
			name[i++] = *pt++;
		}
		if (i >= 32) {
			break;
		}
	}

	return 0;
}

int IsFullName(const char *FilePath)
{
	if (isalpha(FilePath[0]) && ':' == FilePath[1]) {
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
	    return;
    }

    /* Get the current working directory: */
    if (getcwd(Buffer, MAX_PATH) == NULL) {
	    perror("getcwd error");
	    return;
   }
   sprintf(dName, "%s/%s", Buffer, sName);
}

int write_file_data(FILE *src, off_t *src_size, FILE *dest, off_t byte_addr)
{
	int res = 0;
	off_t file_size = 0;
	unsigned char data[FILL_DATA_SIZE];
	if ((src == NULL) || (dest == NULL)) {
		printf("file src or dest is null\n");
		return -1;
	}

	fseeko(src, 0, SEEK_END);
	file_size = ftello(src);
	*src_size = file_size;

	rewind(src);

	fseeko(dest, byte_addr, SEEK_SET);
	while (1) {
		off_t current = ftello(src);
		off_t leave = file_size - current;
		int read_size = FILL_DATA_SIZE;
		if (leave <= 0) {
			break;
		} else if (leave < FILL_DATA_SIZE) {
			read_size = (int)leave;
		}

		size_t res_t = fread(&data, 1, read_size, src);
		if (res_t == read_size) {
			size_t w_res = fwrite(&data, 1, res_t, dest);
			if (res_t != w_res) {
				res = -1;
				printf("fwrite fail\n");
				break;
			}
		} else if (feof(src) != 0) {
			printf("feof res_t=%zu\n", res_t);
			break;
		} else {
			printf("read fail\n");
			res = -1;
			break;
		}
	}

	return res;
}

int append_file_data(FILE *src, off_t *src_size, FILE *dest)
{
	int res = 0;
	off_t file_size = 0;
	unsigned char data[FILL_DATA_SIZE];

	if ((src == NULL) || (dest == NULL)) {
		printf("file src or dest is null\n");
		return -1;
	}

	fseeko(src, 0, SEEK_END);
	file_size = ftello(src);
	*src_size = file_size;

	rewind(src);

	fseeko(dest, 0, SEEK_END);
	while (1) {
		off_t current = ftello(src);
		off_t leave = file_size - current;
		int read_size = FILL_DATA_SIZE;
		if (leave <= 0) {
			break;
		} else if (leave < FILL_DATA_SIZE) {
			read_size = (int)leave;
		}

		size_t res_t = fread(&data, 1, read_size, src);
		if (res_t == read_size) {
			size_t w_res = fwrite(&data, 1, res_t, dest);
			if (res_t != w_res) {
				res = -1;
				printf("fwrite fail\n");
				break;
			}
		} else if (feof(src) != 0) {
			printf("feof res_t=%zu\n", res_t);
			break;
		} else {
			printf("read fail\n");
			res = -1;
			break;
		}
	}

	return res;
}

int fill_file_data(FILE *dest, off_t fill_size)
{
	off_t count = 0;
	unsigned char data[FILL_DATA_SIZE];

	if (dest == NULL) {
		printf("file dest is null\n");
		return -1;
	}

	if (fill_size <= 0) {
		printf("fill file size err\n");
		return -1;
	}

	memset(data, 0, sizeof(data));

	fseeko(dest, 0, SEEK_END);
	while (1) {
		off_t leave = fill_size - count;
		int write_size = FILL_DATA_SIZE;

		if (leave <= 0) {
			break;
		} else if (leave < FILL_DATA_SIZE) {
			write_size = (int)leave;
		}

		size_t w_res = fwrite(&data, 1, write_size, dest);
		count += w_res;
		if (write_size != w_res) {
			printf("fwrite fail\n");
			return -1;
		}
	}

	return 0;
}

int write_data2file(FILE *dest, off_t byte_addr, char *data, off_t data_size)
{
	size_t w_res;

	if (dest == NULL) {
		printf("file dest is null\n");
		return -1;
	}

	fseeko(dest, byte_addr, SEEK_SET);

	w_res = fwrite(data, 1, data_size, dest);
	if (data_size != w_res) {
		printf("fwrite fail\n");
		return -1;
	}

	return 0;
}
