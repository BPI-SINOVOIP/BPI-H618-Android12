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
#define LOD_TAG "IGpioService"
#include "utils/Log.h"

#include <memory.h>
#include <stdint.h>
#include <sys/types.h>

#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include "IGpioService.h"

#include <utils/Errors.h>
#define DEBUG false

namespace android{

enum {
    WRITE = IBinder::FIRST_CALL_TRANSACTION,
    READ = IBinder::LAST_CALL_TRANSACTION
};

class BpGpioService: public BpInterface<IGpioService>{

public:
    BpGpioService(const sp<IBinder>& impl)
        : BpInterface<IGpioService>(impl){
    }

    int writeData(const char *dat, int count, const char *filePath){
        if(DEBUG){
            ALOGV("IGpioService::write()  filePath = %s",filePath);
        }
        Parcel data, reply;
        data.writeInterfaceToken(IGpioService::getInterfaceDescriptor());
        data.writeInt32(count);
        data.writeCString(filePath);
        data.writeCString(dat);
        remote()->transact(WRITE, data, &reply);
        return reply.readInt32();
    }

    int readData(const char *filePath){
        if(DEBUG){
            ALOGV("IGpioService::read()  filePath = %s", filePath);
        }
        Parcel data, reply;
        data.writeInterfaceToken(IGpioService::getInterfaceDescriptor());
        data.writeCString(filePath);

        remote()->transact(READ, data, &reply);
        return reply.readInt32();
    }
};

IMPLEMENT_META_INTERFACE(GpioService, "com.softwinner.IGpioService");

status_t BnGpioService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags){

    switch(code){
    case WRITE:{
        CHECK_INTERFACE(IGpioService, data, reply);
        int count = data.readInt32();
        const char *filePath = data.readCString();
        const char *d = data.readCString();
        reply->writeInt32(writeData(d, count, filePath));
        return NO_ERROR;
    }break;
    case READ:{
        CHECK_INTERFACE(ISecureFileService, data, reply);
        const char *srcPath = data.readCString();
        int ret = readData(srcPath);
        reply->writeInt32(ret);
        return NO_ERROR;
    }break;
    default:
        return BBinder::onTransact(code, data, reply, flags);
    }
}

};
