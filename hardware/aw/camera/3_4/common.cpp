#include "common.h"
#include <stdlib.h>
#include <cutils/properties.h>
#include <unistd.h>
#include <stdio.h>

typedef struct camera3_jpeg_blob {
    void * vaddr;
    int share_fd;
} camera_save_buffer_t;


bool saveBuffers(char *str,void *p, unsigned int length,bool is_oneframe)
{
    int fd;
    HAL_LOGD("Debug to save a frame!");
    if((access(str,0) != -1)&&(is_oneframe)) {
        HAL_LOGD("File %s is exists!!!\n",str);
        //return true;
    }
    if(is_oneframe)
        fd = open(str,O_CREAT|O_RDWR|O_TRUNC,0777);        //save one frame data
    else
        fd = open(str,O_CREAT|O_RDWR|O_APPEND,0777);       //save more frames
    if(!fd) {
        HAL_LOGE("Open file error");
        return false;
    }
    if(write(fd,p,length)){
        HAL_LOGD("Write file successfully");
        close(fd);
        return true;
    }
    else {
        HAL_LOGE("Write file fail");
        close(fd);
        return false;
    }
}

bool saveSizes(int width, int height)
{
    int fd;
    char buf[128];
    fd = open("/data/camera/size.txt",O_CREAT|O_RDWR|O_APPEND,0777);
    if(!fd) {
        HAL_LOGE("Open file error");
        return false;
    }
    sprintf(buf,"width:%d height:%d",width,height);
    if(write(fd,(void*)buf,sizeof(buf))) {
        close(fd);
        return true;
    }
    else {
        HAL_LOGE("Write file fail");
        close(fd);
        return false;
    }
}
bool isDebugEnable(int32_t module)
{
    int32_t debug_module;
    static int32_t debug_module_temp = DEBUG_NONE;
    debug_module = property_get_int32("persist.vendor.debugModule", DEBUG_NONE);
    if (debug_module != debug_module_temp) {
        debug_module_temp = debug_module;
        HAL_LOGI("Camera Hal debugModule:%x", module);
    }
    return module & debug_module ? true :false;
}
int getSingleCameraId()
{
   char singleCameraId[PROPERTY_VALUE_MAX];
   if (!property_get("persist.vendor.SingleCameraId", singleCameraId, NULL) > 0){
        return -1;
   } else {
        int id = atoi(singleCameraId);
        return id;
   }

}
int getSupportCameraId(int cameraId)
{
    int res = getSingleCameraId();
       if (res < 0) {
       return cameraId;
    } else {
       return res;
    }
}
