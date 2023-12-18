/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/


#undef NDEBUG
#define LOG_TAG "GpioService"
#include "GpioService.h"
#include <binder/IServiceManager.h>
#include <utils/misc.h>
#include <utils/Log.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>

#define DEBUG false

namespace android{

void GpioService::instantiate(){
    defaultServiceManager()->addService(
        String16("softwinner.gpio"), new GpioService());
}

GpioService::GpioService(){
    ALOGD("GpioService created");
}

GpioService::~GpioService(){
    ALOGD("GpioService destroyed");
}

int GpioService::writeData(const char * data,int count,const char * filePath){
    if(DEBUG){
        ALOGD("GpioService::write()  file = %s  count = %d  data = %d", filePath, count, data[0]);
    }
    int fd;
    fd = open(filePath, O_RDWR);
    if(fd < 0){
        ALOGE("fail in open file %s", filePath);
        return -1;
    }
    write(fd, data, count);
    close(fd);
    return 0;
}

int GpioService::readData(const char * filePath){
    if(DEBUG){
        ALOGD("GpioService::read()  file = %s", filePath);
    }
    int fd;
    int value;
    fd = open(filePath, O_RDWR);
    if(fd < 0){
        ALOGE("fail in open file %s", filePath);
        return -1;
    }
    char valueStr[32];
    memset(valueStr, 0, sizeof(valueStr));
    read(fd, (void *)valueStr, sizeof(valueStr) - 1);
    char *end;
    value = strtol(valueStr, &end, 0);
    if(end == valueStr){
        ALOGE("Fail in convert string %s to number.", valueStr);
        close(fd);
        return -1;
    }

    close(fd);
    return value;

}

}
