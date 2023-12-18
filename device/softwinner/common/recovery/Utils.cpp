/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "BootHead.h"
#include "Utils.h"

#define SD_UBOOT_SECTOR_START_DEFAULT 32800

/*
#define MMC_BLOCK_MAJOR         179
typedef struct mmc_ioc_erase_cmd {
    unsigned int flags;
    unsigned int start_sec;
    unsigned int size_sec;

    // For 64-bit machines, the next member, ``__u64 data_ptr``, wants to
    // be 8-byte aligned.  Make sure this struct is the same size when
    // built for 32-bit.
    __u32 __pad;
    // DAT buffer
    __u64 data_ptr;
}mmc_ioc_erase_cmd;
#define MMC_IOC_ERASE_CMD _IOWR(MMC_BLOCK_MAJOR, 1, struct mmc_ioc_erase_cmd)

#define UBOOT_MAGIC				"uboot"
#define BOOT1_MAGIC             "eGON.BT1"
#define BOOT0_MAGIC             "eGON.BT0"
*/

#define CMDLINE_FILE_PATH "/proc/cmdline"
#define VERIFIED_BOOT "androidboot.verifiedbootstate"

static int _is_secure = 0;

static int spliteKeyAndValue(char* str, char** key, char** value){
	int elocation = strcspn(str,"=");
	if (elocation < 0){
		return -1;
	}
	str[elocation] = '\0';
	*key = str;
	*value = str + elocation + 1;
	return 0;
}

static int getInfoFromCmdline(const char* key, char* value) {
	FILE* fp;
    char cmdline[4096];
	//read partition info from /proc/cmdline
    if ((fp = fopen(CMDLINE_FILE_PATH,"r")) == NULL) {
        printf("burnboot: can't open /proc/cmdline \n");
        // goto error;
        return -1;
    }
    fgets(cmdline,4096,fp);
    fclose(fp);
    if (strstr(cmdline, VERIFIED_BOOT))
        _is_secure = 1;
    else
        _is_secure = 0;
    // printf("burnboot: %s\n", cmdline);
    //splite the cmdline by space
    char* p = NULL;
    char* lkey = NULL;
    char* lvalue = NULL;
    p = strtok(cmdline, " ");
    if (!spliteKeyAndValue(p, &lkey, &lvalue)){
    	if (!strcmp(lkey,key)){
    		goto done;
    	}
    }
    // printf("burnboot: the first k-v is %s\n", p);

    while ((p = strtok(NULL, " "))){
    	// printf("burnboot: the other k-v is %s\n", p);
    	if (!spliteKeyAndValue(p, &lkey, &lvalue)){
	    	if (!strcmp(lkey,key)){
	    		goto done;
	    	}
	    }
    }

    printf("burnboot: no key named %s in cmdline.\n", key);
    strcpy(value, "-1");
    return -1;

    done:
    strcpy(value, lvalue);
    return 0;
}

int getFlashType() {
	char ctype[8];
	getInfoFromCmdline("boot_type", ctype);
	printf("burnboot: flash type = %s\n", ctype);

	int flash_type = atoi(ctype);
	//atoi出错时会返回0,当ctype字符串为0时也会返回0，所以这里要判断是否出错.
	if (flash_type == 0 && ctype[0] != '0') {
		return FLASH_TYPE_UNKNOW;
	}

	return flash_type;
}

#define CHECK_SOC_SECURE_ATTR 0x00
#define CHECK_BOOT_SECURE_ATTR 0x04
/*
 * Check secure solution or not
 * Return 0 if normal , return 1 if secure
 */
int check_soc_is_secure(void) {
	return _is_secure;
/*int fd, ret;

	fd = open("/dev/sunxi_soc_info", O_RDWR);
	if (fd == -1) {
		printf("burnboot: open /dev/sunxi_soc_info failed!\n");
		return 0 ;
	}

	ret = ioctl(fd, CHECK_SOC_SECURE_ATTR, NULL);
	if (ret) {// ret=1 in secure case
		printf("burnboot: soc is secure. (return value:%x)\n", ret);
	} else {
		printf("burnboot: soc is normal. (return value:%x)\n", ret);
		ret = ioctl(fd, CHECK_BOOT_SECURE_ATTR, NULL);
		if (ret)
			printf("burnboot: secure boot for normal case\n");
	}

	close(fd);
	return ret;*/
}

int getBufferExtractCookieOfFile(const char* path, BufferExtractCookie* cookie) {

	if (cookie == NULL) {
		// printf("get file stat failed!\n");
		return -1;
	}

	struct stat statbuff;
	if (stat(path, &statbuff) < 0) {
		printf("burnboot: get file stat failed!!\n");
		return -1;
	}
	cookie->len = statbuff.st_size;
	// printf("burnboot: file size is %d\n",(int)cookie->len);

	unsigned char* buffer = reinterpret_cast<unsigned char *>(malloc(cookie->len));

    if (buffer==NULL)
        return -1;

	FILE* fp = fopen(path,"r");
	if (fp == NULL) {
		printf("burnboot: open file failed!\n");
        free(buffer);
		return -1;
	}

	if (!fread(buffer, cookie->len, 1, fp)) {
		printf("burnboot: read file failed!\n");
        free(buffer);
        fclose(fp);
		return -1;
	}

	cookie->buffer = buffer;

    fclose(fp);
	return 0;

}

static int verify_toc_addsum(void *mem_base, unsigned int size, unsigned int *psum) {
	unsigned int *buf;
	unsigned int count;
	unsigned int src_sum;
	unsigned int sum;

	/* 生成校验和 */
	src_sum = *psum;                  // 从Boot_file_head中的“check_sum”字段取出校验和
	printf("burnboot: read sum :0x%x\n",src_sum);
	*psum = STAMP_VALUE;              // 将STAMP_VALUE写入Boot_file_head中的“check_sum”字段

	count = size >> 2;                         // 以 字（4bytes）为单位计数
	sum = 0;
	buf = (unsigned int *)mem_base;
	do {
		sum += *buf++;                         // 依次累加，求得校验和
		sum += *buf++;                         // 依次累加，求得校验和
		sum += *buf++;                         // 依次累加，求得校验和
		sum += *buf++;                         // 依次累加，求得校验和
	} while ((count -= 4) > (4 - 1));

	while (count-- > 0)
		sum += *buf++;

	*psum = src_sum;                  // 恢复Boot_file_head中的“check_sum”字段的值
	printf("burnboot: calc sum :0x%x\n",sum);

	if (sum == src_sum)
		return 0;                           // 校验成功
	else
		return -1;                          // 校验失败
}

int checkBoot0Sum(BufferExtractCookie* cookie) {
	standard_boot_file_head_t  *head_p;
	unsigned int length;
	unsigned int *buf;
	unsigned int loop;
	unsigned int i;
	unsigned int sum;
	unsigned int csum;

	if (check_soc_is_secure()) {
		unsigned int *psum;
		psum = &(((toc0_private_head_t *)(cookie->buffer))->check_sum);
		return verify_toc_addsum(cookie->buffer, cookie->len, psum);

	} else {
		head_p = (standard_boot_file_head_t *)cookie->buffer;

		length = head_p->length;
		if ((length & 0x3) != 0)                   // must 4-byte-aligned
			return -1;
		if ((length > 32 * 1024) != 0 ) {
			printf("burnboot: boot0 file length over size!!\n");
		}
		if ((length & (512 - 1)) != 0) {
			printf("burnboot: boot0 file did not aliged!!\n");
		}
		buf = (unsigned int *)cookie->buffer;
		csum = head_p->check_sum;
		head_p->check_sum = STAMP_VALUE;              // fill stamp
		loop = length >> 2;

		for (i = 0, sum = 0; i < loop; i++)
			sum += buf[i];

		head_p->check_sum = csum;
		printf("burnboot: Boot0 -> File length is %u,original sum is %u,new sum is %u\n", length, head_p->check_sum, sum);
		return !(csum == sum);
	}
}

int checkUbootSum(BufferExtractCookie* cookie) {
	uboot_file_head  *head_p;
	sbrom_toc1_head_info_t *head_toc1;
	unsigned int length;
	unsigned int *buf;
	unsigned int loop;
	unsigned int i;
	unsigned int sum;
	unsigned int csum;
	unsigned int *psum;

	if (check_soc_is_secure()) {
		psum = &(((sbrom_toc1_head_info_t *)(cookie->buffer))->add_sum);
		return verify_toc_addsum(cookie->buffer, cookie->len, psum);
	} else {
		head_toc1 = (sbrom_toc1_head_info_t *)(cookie->buffer);
		if (head_toc1->magic == TOC_MAIN_INFO_MAGIC) {
			psum = &(((sbrom_toc1_head_info_t *)(cookie->buffer))->add_sum);
			return verify_toc_addsum(cookie->buffer, cookie->len, psum);
		} else {
			head_p = (uboot_file_head *)cookie->buffer;
			length = head_p->length;
			if ((length & 0x3) != 0)                   // must 4-byte-aligned
				return -1;

			buf = (unsigned int *)cookie->buffer;
			csum = head_p->check_sum;

			head_p->check_sum = STAMP_VALUE;              // fill stamp
			loop = length >> 2;

			for (i = 0, sum = 0;  i < loop;  i++)
				sum += buf[i];

			head_p->check_sum = csum;
			printf("burnboot: Uboot -> File length is %u,original sum is %u,new sum is %u\n", length, head_p->check_sum, sum);
			return !(csum == sum);
		}
	}
}

int checkBoot1Sum(BufferExtractCookie* cookie) {
    boot1_file_head  *head_p;
    unsigned int length;
    unsigned int *buf;
    unsigned int loop;
    unsigned int i;
    unsigned int sum;
    unsigned int csum;

    head_p = (boot1_file_head *)cookie->buffer;
    length = head_p->length;
    if ((length & 0x3) != 0)                   // must 4-byte-aligned
        return -1;

    buf = (unsigned int *)cookie->buffer;
    csum = head_p->check_sum;

    head_p->check_sum = STAMP_VALUE;              // fill stamp
    loop = length >> 2;

    for (i = 0, sum = 0;  i < loop;  i++)
        sum += buf[i];

    head_p->check_sum = csum;
    printf("burnboot: boot1:File length is %u,old sum is %u,new sum is %u\n", length, head_p->check_sum, sum);
    return !(csum == sum);
}

int getUbootstartsector(BufferExtractCookie* cookie) {
	if(check_soc_is_secure()){
        return -1;
    }
	uboot_file_head_t  *head_p = NULL;
	unsigned int  start_sector = 0;
	head_p = (uboot_file_head_t *)cookie->buffer;

    if (head_p == NULL)
        return -1;

    start_sector = head_p->prvt_head.uboot_start_sector_in_mmc;

    if(start_sector == 0)
    {
        start_sector = SD_UBOOT_SECTOR_START_DEFAULT;
    }

    return (int)start_sector;
}

int genBoot0CheckSum(void *cookie)
{
	standard_boot_file_head_t  *head_p;
	unsigned int length;
	unsigned int *buf;
	unsigned int loop;
	unsigned int i;
	unsigned int sum;
	unsigned int *psum;

	if(check_soc_is_secure()){
		toc0_private_head_t *head_t = (toc0_private_head_t *)cookie;
		psum = &(head_t->check_sum);
		length = head_t->length;

	}else{
		head_p = (standard_boot_file_head_t *)cookie;
		psum = &head_p->check_sum ;
		length = head_p->length;
	}

	if( ( length & 0x3 ) != 0 )                   // must 4-byte-aligned
		return -1;
	buf = (unsigned int *)cookie;
	*psum = STAMP_VALUE;              // fill stamp
	loop = length >> 2;
	sum = 0 ;
	for( i = 0, sum = 0;  i < loop;  i++ )
		sum += buf[i];

	/* write back check sum */
	*psum = sum;
	return 0 ;
}

int readWithSeek(int fd ,off_t offset, size_t bootsize, void *buffer){
	memset(buffer, 0, bootsize);
	if (lseek(fd, 0, SEEK_SET) == -1) {
		printf("burnboot: reset the cursor failed! the error num is %d:%s\n", errno, strerror(errno));
		return -1;
	}

	if (lseek(fd, offset, SEEK_CUR) == -1) {
		printf("burnboot: lseek failed! the error num is %d:%s\n", errno, strerror(errno));
		return -1;
	}

	return read(fd,buffer,bootsize);
	long result = read(fd,buffer,bootsize);
    if (result==(long)bootsize)
        return 0;
    else {
	    printf("burnboot: readWithSeek : fail need read %zu, but write %ld\n", bootsize, result);
        return -1;
    }

}

int writeWithSeek(int fd, void *buf, off_t offset, size_t bootsize){
	if (lseek(fd, 0, SEEK_SET) == -1) {
		printf("burnboot: reset the cursor failed! the error num is %d:%s\n", errno, strerror(errno));
		return -1;
	}

	if (lseek(fd, offset, SEEK_CUR) == -1) {
		printf("burnboot: lseek failed! the error num is %d:%s\n", errno, strerror(errno));
		return -1;
	}
	printf("burnboot: Write : offset = 0x%lx, len= %zu\n", offset, bootsize);
	long result = write(fd, buf, bootsize);
	fsync(fd);
    if (result==(long)bootsize)
        return 0;
    else {
	    printf("burnboot: Write fail: need write %zu, but write %ld\n", bootsize, result);
        return -1;
    }
}
