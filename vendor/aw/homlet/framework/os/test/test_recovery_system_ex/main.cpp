/*
* (C) Copyright 2007-2013
* Allwinner Technology Co., Ltd. <www.allwinnertech.com>
* Charles <yanjianbo@allwinnertech.com>
*
* See file CREDITS for list of people who contributed to this
* project.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston,
* MA 02111-1307 USA
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <linux/kdev_t.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/mount.h>
#include <errno.h>
#include "android_runtime/AndroidRuntime.h"
#include "md5.h"

#define DEV_BLOCK_DIR          "/dev/block"
#define SYS_BLOCK_DIR          "/sys/dev/block"
#define MOUNT_POINT            "/ex_update"
#define RECOVERY_COMMAND_FILE  "/cache/recovery/command"
#define EX_INSTALL_PACKAGE_CMD "--update_package_ex=%s %s %s"
#define SYSLNK_MAX_TEST_DEPTH (strlen("../../devices/platform/sw-ehci.x/usbx/x-x/x-x.x"))

#ifndef MD5_DIGEST_LENGTH
#define MD5_DIGEST_LENGTH 16
#endif

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

static int do_md5(const char *path, char *outMD5)
{
    unsigned int i;
    int fd;
    MD5_CTX md5_ctx;
    unsigned char md5[MD5_DIGEST_LENGTH];

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr,"could not open %s, %s\n", path, strerror(errno));
        return -1;
    }

    /* Note that bionic's MD5_* functions return void. */
    MD5_Init(&md5_ctx);

    while (1) {
        char buf[4096];
        ssize_t rlen;
        rlen = read(fd, buf, sizeof(buf));
        if (rlen == 0)
            break;
        else if (rlen < 0) {
            (void)close(fd);
            fprintf(stderr,"could not read %s, %s\n", path, strerror(errno));
            return -1;
        }
        MD5_Update(&md5_ctx, buf, rlen);
    }
    if (close(fd)) {
        fprintf(stderr,"could not close %s, %s\n", path, strerror(errno));
        return -1;
    }

    MD5_Final(md5, &md5_ctx);

    memset(outMD5, 0, MD5_DIGEST_LENGTH*2 + 1);

    for(int i=0;i<MD5_DIGEST_LENGTH;i++){
        char tmp[3];
        sprintf(tmp, "%02x", md5[i]);
        strcat(outMD5, tmp);
    }

    return 0;
}

static int ensure_dev_mounted(const char * devPath, const char * mountedPoint) {
    int ret;
    if (devPath == NULL || mountedPoint == NULL) {
        return 0;
    }
    mkdir(mountedPoint, 0755); //in case it doesn't already exist
    ret = mount(devPath, mountedPoint, "vfat",
            MS_NOATIME | MS_NODEV | MS_NODIRATIME, "utf8");
    if (ret == 0) {
        fprintf(stderr,"mount %s with fs 'vfat' success\n", devPath);
        return 1;
    } else {
        ret = mount(devPath, mountedPoint, "ntfs",
                MS_NOATIME | MS_NODEV | MS_NODIRATIME, "utf8");
        if (ret == 0) {
            fprintf(stderr,"mount %s with fs 'ntfs' success\n", devPath);
            return 1;
        } else {
            ret = mount(devPath, mountedPoint, "ext4",
                    MS_NOATIME | MS_NODEV | MS_NODIRATIME, "utf8");
            if (ret == 0) {
                fprintf(stderr,"mount %s with fs 'ext4' success\n", devPath);
                return 1;
            }
        }
        fprintf(stderr,"failed to mount %s (%s)\n", devPath, strerror(errno));
        return 0;
    }
}

static int parseUpdateExCmd(char *relative_path, char *md5, char* syslink) {
    FILE *f;
    char buff[2000] = {0};
    char tmp[2000] = {0};
    char *tmpp;
    if ((f = fopen(RECOVERY_COMMAND_FILE, "r")) == NULL) {
        return 0;
    }

    fgets(buff, 2000, f);

    for (unsigned int i = 0; i < strlen(buff); i++) {
        if (buff[i] == ':')
            buff[i] = ' ';
    }

    int count = sscanf(buff, EX_INSTALL_PACKAGE_CMD, relative_path, md5, syslink);

    fclose(f);

    return count == 3;
}

static int getNodeBySysLnk(char *syslink, char *outNode) {
    dirent *dirp;
    DIR *dp;
    char file[MAX_PATH] = {0};
    struct stat file_stat;

    //根据设备符号连接查找设备号
    FILE *fp;
    unsigned long long major = 0, minor = 0;
    int count = 0;

    if ((dp = opendir(SYS_BLOCK_DIR)) == NULL)
        return 0;
    while ((dirp = readdir(dp)) != NULL) {
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
            continue;

        sprintf(file, SYS_BLOCK_DIR"/%s", dirp->d_name);

        lstat(file, &file_stat);
        if (S_ISLNK(file_stat.st_mode)) {
            char tmplink[MAX_PATH];
            size_t size = 0;
            memset(tmplink, 0, MAX_PATH);
            size = readlink(file, tmplink, MAX_PATH);
            fprintf(stdout,"getNodeBySysLnk:%s %s\n", file, tmplink);
            //比较其设备路径和设备号
            if(strncmp(syslink, tmplink, SYSLNK_MAX_TEST_DEPTH) == 0
                    && strcmp(syslink+strlen(syslink)-1, tmplink+strlen(tmplink)-1) == 0){
                count = sscanf(dirp->d_name, "%llu:%llu", &major, &minor);
                break;
            }
        }
    }
    closedir(dp);
    fprintf(stdout,"getNodeBySysLnk:The device number is %lld:%lld\n", major, minor);
    if(count != 2) return 0;
    dev_t devNum = MKDEV(major, minor);

    //根据设备号查找设备节点
    if ((dp = opendir(DEV_BLOCK_DIR)) == NULL)
        return 0;
    while ((dirp = readdir(dp)) != NULL) {
        if (strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
            continue;

        sprintf(file, DEV_BLOCK_DIR"/%s", dirp->d_name);

        lstat(file, &file_stat);
        fprintf(stdout,"getNodeBySysLnk:%s %lld %lld\n",
                file, MAJOR(file_stat.st_rdev), MINOR(file_stat.st_rdev));
        if (S_ISBLK(file_stat.st_mode) && file_stat.st_rdev == devNum) {
            strcpy(outNode, file);
            closedir(dp);
            return 1;
        }
    }
    closedir(dp);
    return 0;
}

static int mountNode(char *node) {
    return ensure_dev_mounted(node, MOUNT_POINT);
}

static int testFile(char *relative_path, char *md5) {
    char full_path[MAX_PATH];
    sprintf(full_path, MOUNT_POINT"%s", relative_path);

    char file_md5[MD5_DIGEST_LENGTH * 2 + 1];
    if(do_md5(full_path, file_md5) == 0){
        return strcmp(file_md5, md5) == 0;
    }

    return 0;
}

int main(int argc, char **argv) {

    //char relative_path[MAX_PATH] = {0};
    char relative_path[MAX_PATH] = "/sdcard/update.zip";
    char md5[MD5_DIGEST_LENGTH * 2 + 1] = {0};
    char syslink[MAX_PATH] = {0};

    // 解析命令
    if (parseUpdateExCmd(relative_path, md5, syslink) == 0) {
        fprintf(stderr,"Parse %s failed\n", RECOVERY_COMMAND_FILE);
        return 0;
    }
    ALOGD("Parse:%s %s %s", relative_path, md5, syslink);

    // 根据设备符号链接获得设备节点
    char node[MAX_PATH];
    int times = 5;
    while (times--) {
        if (getNodeBySysLnk(syslink, node) == 0) {
            fprintf(stderr, "Get node by syslink %s failed, try again!Left %d\n", syslink, times);
            sleep(1);
            continue;
        }
        fprintf(stdout, "Get Node: %s\n", node);
        break;
    }
    if(times < 0) return 0;

    // 挂载设备到指定目录
    if (mountNode(node) == 0) {
        fprintf(stderr,"Mount node %s falied\n", node);
        return 0;
    }

    // 根据md5测试文件的正确性
    if (testFile(relative_path, md5) == 0) {
        fprintf(stderr,"There is not the correct file.\n");
        return 0;
    }

    fprintf(stdout,"OK!Ready to start update.\n");

    umount(MOUNT_POINT);
    return 1;
}
