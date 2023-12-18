/*
 * \file        gsensertester.c
 * \brief
 *
 * \version     1.0.0
 * \date        2012年07月10日
 * \author      zhengjiewen  <zhengjiewen@allwinnertech.com>
 *
 * Copyright (c) 2012 Allwinner Technology. All Rights Reserved.
 *
 */


#include <linux/input.h>

#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include  <dirent.h>
#include "dragonboard_inc.h"

#define INPUT_DIR                       "/dev/input"

//打开sensor设备操作节点，
//返回值：正确时为节点的fd,错误时为-1.

static int open_input_device(char* sensor_name)
{
    char *filename;
    int fd;
    DIR *dir;
    struct dirent *de;
    char name[80];
    char devname[256];
    dir = opendir(INPUT_DIR);
    if (dir == NULL)
        return -1;

    strcpy(devname, INPUT_DIR);
    filename = devname + strlen(devname);
    *filename++ = '/';

    while ((de = readdir(dir))) {
        if (de->d_name[0] == '.' &&
                (de->d_name[1] == '\0' ||
                 (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        fd = open(devname, O_RDONLY);
        if (fd < 0) {
            continue;
        }


        if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
            name[0] = '\0';
        }

        if (!strcmp(name, sensor_name)) {
#ifdef DEBUG_SENSOR
            db_msg("devname is %s \n",devname);
#endif
        } else {
            close(fd);
            continue;
        }
        closedir(dir);

        return fd;

    }
    closedir(dir);

    return -1;
}


int main(int argc, char *argv[])
{
    int fd;
    struct input_event event;
    char buf[64];
    int ret;

    INIT_CMD_PIPE();

    fd = open_input_device(argv[4]);
    if (fd == -1) {
        SEND_CMD_PIPE_FAIL_EX("无法获取设备");
        db_error("can't open %s(%s)\n",argv[4], strerror(errno));
        goto err;
    }
    SEND_CMD_PIPE_EX("请使用磁铁进行测试");
    while (1) {
        ret = read(fd, &event, sizeof(event));
        if (ret == -1) {
            sleep(1);
        }
        if(event.type == EV_KEY && event.value == 0) {
            switch (event.code) {
            case KEY_WAKEUP:
                SEND_CMD_PIPE_OK_EX("磁铁已远离");
                break;
            case KEY_SLEEP:
                SEND_CMD_PIPE_OK_EX("检测到磁铁");
                break;
            default:
                db_debug("unknown key %d\n", event.code);
                break;
            }

        }
    }
    close(fd);
err:
    EXIT_CMD_PIPE();
    return 0;
}


