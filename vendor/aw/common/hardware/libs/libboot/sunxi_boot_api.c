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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "type_def.h"
#include "private_boot0.h"
#include "private_toc.h"
#include "private_uboot.h"


/*
 * #define BOOT_DEBUG
 */
#define NAND_BLKREADBOOT0			_IO('v',125)
#define NAND_BLKREADBOOT1			_IO('v',126)
#define NAND_BLKBURNBOOT0			_IO('v',127)
#define NAND_BLKBURNBOOT1			_IO('v',128)

#define DEVNODE_PATH_NAND			"/dev/block/by-name/bootloader"
#define BOOT_EMMC_PATH				"/dev/block/mmcblk0"
#define CMDLINE_FILE_PATH			"/proc/cmdline"

#define CHECK_SOC_SECURE_ATTR		0x00
#define CHECK_BOOT_SECURE_ATTR		0x04
extern int sunxi_bytes_merge(u8 *dst, u32 dst_len, u8 *src, uint src_len);
extern int toc1_traverse(BufferExtractCookie* cookie);


static int spliteKeyAndValue(char* str, char** key, char** value)
{
    int elocation = strcspn(str, "=");
    if (elocation < 0){
        return -1;
    }
    str[elocation] = '\0';
    *key = str;
    *value = str + elocation + 1;
    return 0;
}

static int getInfoFromCmdline(char* key, char* value)
{
    FILE* fp;
    char cmdline[1024];

    if ((fp = fopen(CMDLINE_FILE_PATH, "r")) == NULL) {
        bb_debug("can't open /proc/cmdline \n");
        return -1;
    }
    fgets(cmdline, 1024, fp);
    fclose(fp);

    char* p = NULL;
    char* lkey = NULL;
    char* lvalue = NULL;
    p = strtok(cmdline, " ");
    if (!spliteKeyAndValue(p, &lkey, &lvalue)) {
        if (!strcmp(lkey,key)){
            goto done;
        }
    }

    while ((p = strtok(NULL, " "))) {
        if (!spliteKeyAndValue(p, &lkey, &lvalue)) {
            if (!strcmp(lkey,key)) {
                goto done;
            }
        }
    }

    bb_debug("no key named %s in cmdline.\n", key);
    strcpy(value, "-1");
    return -1;

done:
    strcpy(value, lvalue);
    return 0;
}

int getFlashType(void)
{
    char ctype[8];
    getInfoFromCmdline("boot_type", ctype);
#if defined(BOOT_DEBUG)
    bb_debug("flash type = %s\n", ctype);
#endif

    int flash_type = atoi(ctype);
    //atoi出错时会返回0,当ctype字符串为0时也会返回0，所以这里要判断是否出错.
    if ((flash_type == 0) && (ctype[0] != '0')) {
        return FLASH_TYPE_UNKNOW;
    }

    return flash_type;
}

void clearPageCache(void)
{
    FILE *fp = fopen("/proc/sys/vm/drop_caches", "w+");
    char *num = "1";
    fwrite(num, sizeof(char), 1, fp);
    fclose(fp);
}

/*
 * Check secure solution or not
 * Return 0 if normal , return 1 if secure
 */
int check_soc_is_secure(void)
{
    int fd, ret;

    fd = open("/dev/sunxi_soc_info", O_RDWR);
    if (fd == -1) {
        bb_debug("open /dev/sunxi_soc_info failed!\n");
        return 0 ;
    }

    ret = ioctl(fd, CHECK_SOC_SECURE_ATTR, NULL);
    if (ret) {
        bb_debug("soc is secure. (return value:%x)\n", ret);
    } else {
        bb_debug("soc is normal. (return value:%x)\n", ret);
        //ret = ioctl(fd, CHECK_BOOT_SECURE_ATTR, NULL);
        //if(ret)
        //    bb_debug("secure boot for normal case\n");
    }

    close(fd);
    return ret;
}


#define CHECK_SOC_ROTPK 0x06
int read_rotpk(char efuse_hash[])
{
	int fd, ret, try_cnt = 100;

RETRY:
	fd = open("/dev/sunxi_soc_info", O_RDONLY);
	if (fd == -1) {
		usleep(10);
		printf("open /dev/sunxi_soc_info failed! %d\n", try_cnt);
		if(try_cnt--)
		goto RETRY;

		printf("open /dev/sunxi_soc_info failed!!!! Pls check selinux or device file acl\n");
		return 0 ;
	}

	ret = ioctl(fd, CHECK_SOC_ROTPK, (unsigned long)efuse_hash);
	if (ret < 0) {
		printf("get rotpk failed. (return value:%x)\n", ret);
		return 0;
	} 
	return 32;/*return the read lenth */
}

int verify_sum(void *mem_base, unsigned int size, unsigned int src_sum)
{
    unsigned int *buf;
    unsigned int count;
    unsigned int sum;

    /* 生成校验和 */
    count = size >> 2;                         // 以 字（4bytes）为单位计数
    sum = 0;
    buf = (unsigned int *)mem_base;
    do
    {
        sum += *buf++;                         // 依次累加，求得校验和
        sum += *buf++;                         // 依次累加，求得校验和
        sum += *buf++;                         // 依次累加，求得校验和
        sum += *buf++;                         // 依次累加，求得校验和
    }while( ( count -= 4 ) > (4-1) );

    while( count-- > 0 )
        sum += *buf++;

    sum = sum - src_sum + STAMP_VALUE;                  // 恢复Boot_file_head中的“check_sum”字段的值
    bb_debug("src sum=0x%x, calc sum :0x%x\n",src_sum, sum);

    if( sum == src_sum )
        return 0;                           // 校验成功
    else
        return -1;                          // 校验失败
}

uint generate_check_sum(void *mem_base, uint length, uint src_sum)
{
    uint *buf;
    uint count;
    uint sum;

    /* 生成校验和 */
    count = length >> 2;                       // 以 字（4bytes）为单位计数
    sum = 0;
    buf = (__u32 *)mem_base;
    do
    {
        sum += *buf++;                         // 依次累加，求得校验和
        sum += *buf++;                         // 依次累加，求得校验和
        sum += *buf++;                         // 依次累加，求得校验和
        sum += *buf++;                         // 依次累加，求得校验和
    }while( ( count -= 4 ) > (4-1) );

    while( count-- > 0 )
        sum += *buf++;

    sum = sum - src_sum + STAMP_VALUE;

    return sum;
}

int writewithseek(int fd, void *buf, off_t offset, size_t buf_size)
{
    if (lseek(fd, 0, SEEK_SET) == -1) {
        bb_debug("reset the cursor failed! the error num is %d:%s\n",errno,strerror(errno));
        return -1;
    }

    if (lseek(fd, offset, SEEK_CUR) == -1) {
        bb_debug("lseek failed! the error num is %d:%s\n",errno,strerror(errno));
        return -1;
    }
    bb_debug("Write : offset = %d, len= %zu\n", (int)offset, buf_size);
    long result = write(fd, buf, buf_size);
    fsync(fd);
    if (result == (long)buf_size)
        return 0;
    else {
        bb_debug("Write fail: need write %zu, but write %ld\n", buf_size, result);
        return -1;
    }
}

int readwithseek(int fd, void *buf, off_t offset, size_t buf_size)
{
    if (lseek(fd, 0, SEEK_SET) == -1) {
        bb_debug("reset the cursor failed! the error num is %d:%s\n",errno,strerror(errno));
        return -1;
    }

    if (lseek(fd, offset, SEEK_CUR) == -1) {
        bb_debug("lseek failed! the error num is %d:%s\n",errno,strerror(errno));
        return -1;
    }

    bb_debug("Write : offset = %d, len= %zu\n", (int)offset, buf_size);
    long result = read(fd, buf, buf_size);

    if (result == (long)buf_size)
        return 0;
    else {
        bb_debug("Write fail: need write %zu, but write %ld\n", buf_size, result);
        return -1;
    }
}

static int sunxi_nand_read_boot0(BufferExtractCookie* cookie)
{
	int fd, ret = 0;
	fd = open(DEVNODE_PATH_NAND, O_RDWR);
	if (fd == -1){
		bb_debug("open device node failed ! errno is %d : %s\n", errno, strerror(errno));
		return -1;
	}

	clearPageCache();

	ret = ioctl(fd, NAND_BLKREADBOOT0, (unsigned long)cookie);

	if(ret) {
		bb_debug("readNandboot0 failed ! errno is %d : %s\n", errno, strerror(errno));
	}

	close(fd);
	return ret;
}

static int sunxi_nand_write_boot0(BufferExtractCookie* cookie)
{
	int fd, ret = 0;
	fd = open(DEVNODE_PATH_NAND, O_RDWR);
	if (fd == -1) {
		bb_debug("open device node failed ! errno is %d : %s\n", errno, strerror(errno));
		return -1;
	}

	clearPageCache();

	ret = ioctl(fd, NAND_BLKBURNBOOT0, (unsigned long)cookie);

	if(ret) {
		bb_debug("burnNandboot0 failed ! errno is %d : %s\n", errno, strerror(errno));
	}

	fsync(fd);
	close(fd);
	return ret;
}

static int sunxi_nand_read_uboot(BufferExtractCookie* cookie)
{
	int fd, ret = 0;
	fd = open(DEVNODE_PATH_NAND, O_RDWR);
	if (fd == -1){
		bb_debug("open device node failed ! errno is %d : %s\n", errno, strerror(errno));
		return -1;
	}

	clearPageCache();

	ret = ioctl(fd, NAND_BLKREADBOOT1, (unsigned long)cookie);

	if(ret) {
		bb_debug("readNandboot1 failed ! errno is %d : %s\n", errno, strerror(errno));
	}

	close(fd);
	return ret;
}

static int sunxi_nand_write_uboot(BufferExtractCookie* cookie)
{
	int fd, ret = 0;
	fd = open(DEVNODE_PATH_NAND, O_RDWR);
	if (fd == -1) {
		bb_debug("open device node failed ! errno is %d : %s\n", errno, strerror(errno));
		return -1;
	}

	clearPageCache();

	ret = ioctl(fd, NAND_BLKBURNBOOT1, (unsigned long)cookie);

	if(ret) {
		bb_debug("burnNandboot1 failed ! errno is %d : %s\n", errno, strerror(errno));
	}

	fsync(fd);
	close(fd);
	return ret;
}

static int sunxi_mmc_write_boot0(BufferExtractCookie* cookie, int length)
{
	int fd = open(BOOT_EMMC_PATH, O_RDWR);
    if (fd == -1) {
        bb_debug("open device %s node failed ! errno is %d : %s\n",
			BOOT_EMMC_PATH, errno, strerror(errno));
        return -1;
    }

    if(writewithseek(fd, cookie->buffer, BOOT0_SDMMC_START_ADDR*512, length))
    {
    	bb_debug("write %s node %d failed ! errno is %d : %s\n", BOOT_EMMC_PATH,
			BOOT0_SDMMC_START_ADDR, errno, strerror(errno));
        return -1;
    }

	if(writewithseek(fd, cookie->buffer, BOOT0_SDMMC_BACKUP_START_ADDR*512, length))
    {
    	bb_debug("write %s node %d failed ! errno is %d : %s\n", BOOT_EMMC_PATH,
			BOOT0_SDMMC_BACKUP_START_ADDR, errno, strerror(errno));
        return -1;
    }

	fsync(fd);
	close(fd);
	return 0;
}

static int sunxi_mmc_read_boot0(BufferExtractCookie* cookie, int flag)
{
	int fd = open(BOOT_EMMC_PATH, O_RDWR);
    if (fd == -1) {
        bb_debug("open device %s node failed ! errno is %d : %s\n", BOOT_EMMC_PATH, errno, strerror(errno));
        return -1;
    }

	if (flag == 0) {
		if (lseek(fd, BOOT0_SDMMC_START_ADDR*512, SEEK_SET) < 0) {
			bb_debug("lseek failed! the error num is %d:%s\n",errno,strerror(errno));
			goto err_out;
		}
	} else {
		if (lseek(fd, BOOT0_SDMMC_BACKUP_START_ADDR*512, SEEK_SET) < 0) {
			bb_debug("lseek failed! the error num is %d:%s\n",errno,strerror(errno));
			goto err_out;
		}
	}

    long result = read(fd, cookie->buffer, cookie->len);
    if (result != cookie->len)
    {
        bb_debug("readboot0: fail need read %lu, but write %ld\n", cookie->len, result);
        goto err_out;
    }

	close(fd);
	return 0;

err_out:
	close(fd);
	return -1;
}

static int sunxi_mmc_write_uboot(BufferExtractCookie* cookie, int length)
{
	int fd = open(BOOT_EMMC_PATH, O_RDWR);
    if (fd == -1) {
		bb_debug("open device %s node failed ! errno is %d : %s\n",
			BOOT_EMMC_PATH, errno, strerror(errno));
		return -1;
    }

    if(writewithseek(fd, cookie->buffer, UBOOT_START_SECTOR_IN_SDMMC*512, length))
    {
		bb_debug("write %s node %d failed ! errno is %d : %s\n", BOOT_EMMC_PATH,
			UBOOT_START_SECTOR_IN_SDMMC, errno, strerror(errno));
		return -1;
    }

	if(writewithseek(fd, cookie->buffer, UBOOT_BACKUP_START_SECTOR_IN_SDMMC*512, length))
    {
		bb_debug("write %s node %d failed ! errno is %d : %s\n", BOOT_EMMC_PATH,
			UBOOT_START_SECTOR_IN_SDMMC, errno, strerror(errno));
		return -1;
    }

	fsync(fd);
	close(fd);
	return 0;
}

static int sunxi_mmc_read_uboot(BufferExtractCookie* cookie, int flag)
{
	int fd = open(BOOT_EMMC_PATH, O_RDWR);
    if (fd == -1) {
        bb_debug("open device %s node failed ! errno is %d : %s\n", BOOT_EMMC_PATH, errno, strerror(errno));
        return -1;
    }

	if (flag == 0) {
		if (lseek(fd, UBOOT_START_SECTOR_IN_SDMMC*512, SEEK_SET) < 0) {
			bb_debug("lseek failed! the error num is %d:%s\n",errno,strerror(errno));
			goto err_out;
		}
	} else {
		if (lseek(fd, UBOOT_BACKUP_START_SECTOR_IN_SDMMC*512, SEEK_SET) < 0) {
			bb_debug("lseek failed! the error num is %d:%s\n",errno,strerror(errno));
			goto err_out;
		}
	}

    long result = read(fd, cookie->buffer, cookie->len);
    if (result != cookie->len)
    {
        bb_debug("readboot1: fail need read %lu, but write %ld\n", cookie->len, result);
        goto err_out;
    }

	close(fd);
	return 0;

err_out:
	close(fd);
	return -1;
}

int sunxi_read_boot0(BufferExtractCookie *cookie, int flash_type)
{
	int ret = -1;
	toc0_private_head_t  *toc0_head = NULL;
	boot0_file_head_t *boot0_file = NULL;

	if(flash_type == FLASH_TYPE_NAND) {
		ret = sunxi_nand_read_boot0(cookie);
	}
	else if((flash_type == FLASH_TYPE_SD1) || (flash_type == FLASH_TYPE_SD2)){
		ret =  sunxi_mmc_read_boot0(cookie, 0);
	} else {
		bb_debug("unknow flash_type%d\n", flash_type);
		ret = -1;
	}

	if (check_soc_is_secure() == 0) {
		boot0_file = (boot0_file_head_t *)(cookie->buffer);
		if (strncmp((const char *)boot0_file->boot_head.magic, BOOT0_MAGIC, MAGIC_SIZE)) {
			bb_debug("%s, boot0 magic is error\n", __func__);
			return -1;
		}

		if(verify_sum(cookie->buffer, boot0_file->boot_head.length, boot0_file->boot_head.check_sum))
		{
			bb_debug("%s, verify sum failed\n", __func__);
			return -1;
		}

	} else {
		toc0_head = (toc0_private_head_t*)(cookie->buffer);
		if(strncmp((const char *)toc0_head->name, TOC0_MAGIC, sizeof(TOC0_MAGIC)))
		{
			bb_debug("sunxi sprite: toc0 magic is error\n");
			return -1;
		}

		bb_debug("boot0 size = 0x%x\n", toc0_head->length);
		if(verify_sum(cookie->buffer, toc0_head->length, toc0_head->check_sum))
		{
			bb_debug("%s, verify sum failed\n", __func__);
			return -1;
		}
	}

	return ret;

}

int sunxi_write_boot0(BufferExtractCookie* cookie, int flash_type, int generate_checksum)
{
	toc0_private_head_t  *toc0_head = NULL;
	boot0_file_head_t *boot0_file = NULL;
	int ret;
	bb_debug("burn boot0 start\n");
	if (check_soc_is_secure() == 0) {
		boot0_file = (boot0_file_head_t *)(cookie->buffer);
		if (strncmp((const char *)boot0_file->boot_head.magic, BOOT0_MAGIC, MAGIC_SIZE)) {
			bb_debug("%s, boot0 magic is error\n", __func__);
			return -1;
		}

		bb_debug("boot0 size = 0x%x\n", boot0_file->boot_head.length);
		if(verify_sum(cookie->buffer, boot0_file->boot_head.length, boot0_file->boot_head.check_sum))
		{
			bb_debug("%s, verify sum failed\n", __func__);
			return -1;
		}

		if(generate_checksum)
		{
			boot0_file->boot_head.check_sum = generate_check_sum(cookie->buffer, boot0_file->boot_head.length, boot0_file->boot_head.check_sum);
		}

		cookie->len = boot0_file->boot_head.length;
	} else {
		toc0_head = (toc0_private_head_t*)(cookie->buffer);

#if defined(BOOT_DEBUG)
		bb_debug("*******************TOC0 Head Message*************************\n");
		bb_debug("Toc_name          = %s\n",   toc0_head->name);
		bb_debug("Toc_magic         = 0x%x\n", toc0_head->magic);
		bb_debug("Toc_check_sum     = 0x%x\n", toc0_head->check_sum);

		bb_debug("Toc_serial_num    = 0x%x\n", toc0_head->serial_num);
		bb_debug("Toc_status        = 0x%x\n", toc0_head->status);

		bb_debug("Toc_items_nr      = 0x%x\n", toc0_head->items_nr);
		bb_debug("Toc_length        = 0x%x\n", toc0_head->length);
		bb_debug("TOC_MAIN_END      = 0x%x\n", toc0_head->end);
		bb_debug("***************************************************************\n\n");
#endif

		if(strncmp((const char *)toc0_head->name, TOC0_MAGIC, sizeof(TOC0_MAGIC)))
		{
			bb_debug("sunxi sprite: toc0 magic is error\n");
			return -1;
		}

		ret=toc0_traverse(cookie);  //verify toc0
		if(ret<0)
		{
			bb_debug("toc0 verify fail\n");
			return -1;
		}
		bb_debug("boot0 size = 0x%x\n", toc0_head->length);
		if(verify_sum(cookie->buffer, toc0_head->length, toc0_head->check_sum))
		{
			bb_debug("%s, verify sum failed\n", __func__);
			return -1;
		}

		if(generate_checksum)
		{
			toc0_head->check_sum = generate_check_sum(cookie->buffer, toc0_head->length, toc0_head->check_sum);
		}

		cookie->len = toc0_head->length;
	}

	if(flash_type == FLASH_TYPE_NAND) {
		return sunxi_nand_write_boot0(cookie);
	}
	else if((flash_type == FLASH_TYPE_SD1) || (flash_type == FLASH_TYPE_SD2)){
		return sunxi_mmc_write_boot0(cookie, cookie->len);
	}

	bb_debug("unknow flash_type%d\n", flash_type);
	return -1;
}

int sunxi_read_uboot(BufferExtractCookie* cookie, int flash_type)
{
	if(flash_type == FLASH_TYPE_NAND) {
		return sunxi_nand_read_uboot(cookie);
	}
	else if((flash_type == FLASH_TYPE_SD1) || (flash_type == FLASH_TYPE_SD2)){
		return sunxi_mmc_read_uboot(cookie, 0);
	}

	bb_debug("unknow flash_type%d\n", flash_type);
	return -1;
}

int sunxi_write_uboot(BufferExtractCookie* cookie, int flash_type, int generate_checksum)
{
	struct sbrom_toc1_head_info  *toc1_head = NULL;
    int ret;
	toc1_head = (struct sbrom_toc1_head_info  *)(cookie->buffer);

    if(toc1_head->magic != TOC_MAIN_INFO_MAGIC)
	{
		bb_debug("sunxi sprite: toc magic is error\n");
		return -1;
	}
	if (check_soc_is_secure() == 1)
	{
		ret=toc1_traverse(cookie);  //verify toc1
    	if(ret<0)
   		{
			bb_debug("toc1 verify fail\n");
			return -1;
    	}
	}
	bb_debug("uboot size = 0x%x\n", toc1_head->valid_len);
	if(verify_sum(cookie->buffer, toc1_head->valid_len, toc1_head->add_sum))
	{
		bb_debug("%s, verify sum failed\n", __func__);
		return -1;
	}

	if(generate_checksum)
	{
		toc1_head->add_sum = generate_check_sum(cookie->buffer, toc1_head->valid_len, toc1_head->add_sum);
	}

	cookie->len = toc1_head->valid_len;
	if(flash_type == FLASH_TYPE_NAND) {
		return sunxi_nand_write_uboot(cookie);
	}
	else if((flash_type == FLASH_TYPE_SD1) || (flash_type == FLASH_TYPE_SD2)){
		return sunxi_mmc_write_uboot(cookie, toc1_head->valid_len);
	}

	bb_debug("unknow flash_type%d\n", flash_type);
	return -1;
}


int sunxi_verify_toc0(void* buf)
{
    if( check_soc_is_secure() == 0){
        bb_debug("In %s : No need to verify normal system.\n", __func__);
        return 0;
    }

    int ret = 0;
	toc0_private_head_t  *toc0_head = NULL;
    BufferExtractCookie * cookie = (BufferExtractCookie *)malloc( sizeof(BufferExtractCookie) );
    cookie->buffer = buf;
	toc0_head = (toc0_private_head_t*)(cookie->buffer);

    bb_debug("In %s : verify toc0 start\n", __func__);

#if defined(BOOT_DEBUG)
    bb_debug("*******************TOC0 Head Message*************************\n");
    bb_debug("Toc_name          = %s\n",   toc0_head->name);
    bb_debug("Toc_magic         = 0x%x\n", toc0_head->magic);
    bb_debug("Toc_check_sum     = 0x%x\n", toc0_head->check_sum);

    bb_debug("Toc_serial_num    = 0x%x\n", toc0_head->serial_num);
    bb_debug("Toc_status        = 0x%x\n", toc0_head->status);

    bb_debug("Toc_items_nr      = 0x%x\n", toc0_head->items_nr);
    bb_debug("Toc_length        = 0x%x\n", toc0_head->length);
    bb_debug("TOC_MAIN_END      = 0x%x\n", toc0_head->end);
    bb_debug("***************************************************************\n\n");
#endif

    if(strncmp((const char *)toc0_head->name, TOC0_MAGIC, sizeof(TOC0_MAGIC))) {
        bb_debug("sunxi sprite: toc0 magic is error\n");
        free(cookie);
        return -1;
    }

    ret = toc0_traverse(cookie);
    if( ret < 0 ) {
        bb_debug("toc0 verify fail, will exit from the upgrade process.");
        free(cookie);
        return -1;
    }
    else {
        bb_debug("toc0 verify succeed, continue the upgrade process.");
    }

    bb_debug("boot0 size = 0x%x\n", toc0_head->length);
    if(verify_sum(cookie->buffer, toc0_head->length, toc0_head->check_sum)) {
        bb_debug("%s, verify sum failed\n", __func__);
        free(cookie);
        return -1;
    }

    free(cookie);
    return 0;
}

int sunxi_verify_toc1(void* buf)
{
    if( check_soc_is_secure() == 0) {
        bb_debug("In %s : No need to verify normal system.", __func__);
        return 0;
    }

    int ret = 0;
    struct sbrom_toc1_head_info  *toc1_head = NULL;
    BufferExtractCookie * cookie = (BufferExtractCookie *)malloc( sizeof(BufferExtractCookie) );
    cookie->buffer = buf;
	toc1_head = (struct sbrom_toc1_head_info  *)(cookie->buffer);

    bb_debug("In %s : verify toc1 start.\n", __func__);
    if(toc1_head->magic != TOC_MAIN_INFO_MAGIC) {
		bb_debug("%s : toc1 magic is error\n", __func__);
        free(cookie);
		return -1;
	}

    ret = toc1_traverse(cookie);
    if( ret < 0 ) {
        bb_debug("toc1 verify fail, will exit from the upgrade process.");
        free(cookie);
        return -1;
    }
    else {
        bb_debug("toc1 verify succeed, continue the upgrade process.");
    }

    bb_debug("uboot size = 0x%x\n", toc1_head->valid_len);
	if(verify_sum(cookie->buffer, toc1_head->valid_len, toc1_head->add_sum)) {
		bb_debug("%s, verify sum failed\n", __func__);
        free(cookie);
		return -1;
    }

    free(cookie);
    return 0;
}
