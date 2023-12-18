#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <cutils/properties.h>
#include <pthread.h>
#define LOG_TAG "RadioMonitor"
#include <log/log.h>

#include "UEventFramework.h"

static char usb_vid[0x10],usb_pid[0x10];

static void do_coldboot(DIR *d, int lvl)
{
    struct dirent *de;
    int dfd, fd;

    dfd = dirfd(d);

    fd = openat(dfd, "uevent", O_WRONLY);
    if(fd >= 0) {
        write(fd, "add\n", 4);
        close(fd);
    }

    while((de = readdir(d))) {
        DIR *d2;

        if (de->d_name[0] == '.')
            continue;

        if (de->d_type != DT_DIR && lvl > 0)
            continue;

        fd = openat(dfd, de->d_name, O_RDONLY | O_DIRECTORY);
        if(fd < 0)
            continue;

        d2 = fdopendir(fd);
        if(d2 == 0)
            close(fd);
        else {
            do_coldboot(d2, lvl + 1);
            closedir(d2);
        }
    }
}

static void coldboot(const char *path)
{
    DIR *d = opendir(path);
    if(d) {
        do_coldboot(d, 0);
        closedir(d);
    }
}

static int read_vid_pid(char * path)
{
    int fd,size;
    char usb_path[0x60] = {0};

    memset(usb_vid,0,sizeof(usb_vid));
    memset(usb_pid,0,sizeof(usb_pid));

    memset(usb_path,0,0x60);
    strcat(usb_path,path);
    strcat(usb_path,"/idVendor");
    fd=open(usb_path,O_RDONLY);
    size=read(fd,usb_vid,sizeof(usb_vid));
    close(fd);
    if(size<=0)
    {
        RLOGE("Vid :err\n");
        return -1;
    }
    usb_vid[size-1] = 0;

    memset(usb_path,0,0x60);
    strcat(usb_path,path);
    strcat(usb_path,"/idProduct");
    fd=open(usb_path,O_RDONLY);
    size=read(fd,usb_pid,sizeof(usb_pid));
    close(fd);

    if(size<=0)
    {
        RLOGE("Pid :err\n");
        return -1;
    }
    usb_pid[size-1] = 0;

    return 0;
}

static void handleUsbEvent(struct uevent *evt)
{
    const char *devtype = evt->devtype;
    char *p, path[0x60] = {0};
    int ret;
    char file[256];

    if(!strcmp(evt->action, "add") && !strcmp(devtype, "usb_device")) {
        /*call usb mode switch function*/
        p = strstr(evt->path,"usb");
        if(p == NULL)
        {
            return;
        }
        p += sizeof("usb");
        /*  /devices/platform/sw-ehci.1/usb*
            /devices/platform/sw-ehci.1/usb1/1-1/1-1.7
        */
        p = strchr(p,'-');
        if(p == NULL)
        {
            return;
        }

        strcat(path,"/sys");
        strcat(path,evt->path);
        ret = read_vid_pid(path);
        if(ret < 0)
        {
            return;
        }

        sprintf(file, "/vendor/etc/usb_modeswitch.d/%s_%s", usb_vid, usb_pid);
        if(access(file, 0) == 0){
            //wait for usb device ready, zoomdata,StrongRising 3g dongle
            if(!strncmp(usb_vid,"8888",4)&& !strncmp(usb_pid, "6500",4)){
                sleep(8);
            }

            //send usb_modeswitch command
            pid_t pid = fork();
            if (pid < 0) {
                RLOGD("fork fail!");
            } else if (pid == 0) {
                usleep(1000000);
                RLOGD("/vendor/bin/usb_modeswitch -W -I -c %s", file);
                ret = execl("/vendor/bin/usb_modeswitch", "usb_modeswitch",
                            "-W",
                            "-I",
                            "-c", file,
                            NULL);
                exit(ret);
            } else {
                int status;
                ret = wait(&status);
                RLOGD("pid %d exit, status: %d", ret, status);
            }
        }
    }

    return;
}

/* uevent callback function */
static void on_uevent(struct uevent *event)
{
    const char *subsys = event->subsystem;

    if (!strcmp(subsys, "usb")) {
        handleUsbEvent(event);
    }

}

static void *radio_monitor_thread(void *param)
{
    RLOGI("radio_monitor_thread run");

    uevent_init();
    coldboot("/sys/bus/usb/devices");
    RLOGI("change coldboot to /sys/bus/usb/devices");
    uevent_next_event(on_uevent);

    return param;
}

int radio_monitor(void)
{
    int ret;
    pthread_t pid;
    pthread_attr_t attr;

    ret = pthread_attr_init (&attr);
    if (ret != 0) {
        ALOGE("err: pthread_attr_init failed err=%s", strerror(ret));
        goto pthread_attr_init_failed;
    }

    ret = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (ret != 0) {
        ALOGE("err: pthread_attr_setdetachstate failed err=%s", strerror(ret));
        goto pthread_attr_setdetachstate_failed;
    }

    ret = pthread_create(&pid, &attr, radio_monitor_thread, NULL);
    if (ret) {
        ALOGE("err: pthread_create failed, ret=%d\n", ret);
        goto pthread_create_failed;
    }

    return 0;

pthread_create_failed:
pthread_attr_setdetachstate_failed:
pthread_attr_init_failed:
    return -1;
}

int main(int argc __unused, char **argv __unused)
{
    char platform[PROPERTY_VALUE_MAX] = {0};

    RLOGD("add property_get check");

    //is telephony platform?
    if (property_get("ro.vendor.sw.embeded.telephony", platform, "false")) {
        RLOGD("platform = %s",platform);
        if(!strcmp(platform, "false")) {
            RLOGD("platform: wifi-only");
            radio_monitor();
        } else {
            RLOGD("platform: telephony");
        }
    }

    RLOGD("RadioMonitor starting sleep loop");
    while (true) {
        sleep(UINT32_MAX);
    }
}

