#include "CameraDebug.h"
#include <stdio.h>
#include <unistd.h>
#define LOG_TAG "CameraDebug"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <android/log.h>
#ifdef USE_SUNXI_CAMERA_H
#include <sunxi_camera.h>
#endif
#ifdef USE_CSI_VIN_DRIVER
#include <sunxi_camera_v2.h>
#endif

#define PATH "/data/camera/"

bool saveframe(char *str,void *p, unsigned int length,bool is_oneframe)
{
    int fd;
    LOGD("Debug to save a frame!");
    if((access(str,0) != -1) && is_oneframe) {
        LOGW("File %s is exists!!!\n",str);
        return true;
    }
    if(is_oneframe)
        fd = open(str,O_CREAT|O_RDWR|O_TRUNC,0777);        //save one frame data
    else
        fd = open(str,O_CREAT|O_RDWR|O_APPEND,0777);       //save more frames
    if(!fd) {
        LOGE("Open file error");
        return false;
    }
    if(write(fd,p,length)){
        //LOGD("Write file successfully");
        close(fd);
        return true;
    }
    else {
        LOGE("Write file fail");
        close(fd);
        return false;
    }
}

bool saveSize(int width, int height)
{
    int fd;
    char buf[128];
    fd = open("/data/camera/size.txt",O_CREAT|O_RDWR|O_APPEND,0777);
    if(!fd) {
        LOGE("Open file error");
        return false;
    }
    sprintf(buf,"width:%d height:%d",width,height);
    if(write(fd,(void*)buf,sizeof(buf))) {
        close(fd);
        return true;
    }
    else {
        LOGE("Write file fail");
        close(fd);
        return false;
    }
}

