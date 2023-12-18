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
#define LOD_TAG "ISystemMixService"
#include "utils/Log.h"

#include <memory.h>
#include <stdint.h>
#include <sys/types.h>

#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include "ISystemMixService.h"

#include <utils/Errors.h>
#include <stdlib.h>
#include <properties.h>
#define DEBUG true

namespace android{

enum {
    GET_PROPERTY = IBinder::FIRST_CALL_TRANSACTION,
    GET_FILEDATA,
    PUT_FILEDATA,
    MOUNT,
    UMOUNT,
    DELETE,
    CANWRITE,
    RENAME,
    MKDIR,
    CANREAD,
    SETBOOTANIMATION,
    SETBOOTLOGO,
    SET_PROPERTY = IBinder::LAST_CALL_TRANSACTION
};

/**
 * Read a byte array from in. The data at *data is still owned by the parcel
 */
//static void readByteArray(const Parcel& in, const uint8_t** data, size_t* length) {
//    ssize_t slength = in.readInt32();
//    if (slength > 0) {
//        *data = reinterpret_cast<const uint8_t*>(in.readInplace(slength));
//        if (*data) {
//            *length = static_cast<size_t>(slength);
//        } else {
//            *length = 0;
//        }
//    } else {
//        *data = NULL;
//        *length = 0;
//    }
//}

class BpSystemMixService: public BpInterface<ISystemMixService>{

public:
    BpSystemMixService(const sp<IBinder>& impl)
        : BpInterface<ISystemMixService>(impl){
    }

    int setProperty(const char *key, const char *value){
        if(DEBUG){
            ALOGV("ISystemMixService::setproperty()  key=%s value=%s ", key, value);
        }
        Parcel data, reply;
        data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
        data.writeCString(key);
        data.writeCString(value);
        remote()->transact(SET_PROPERTY, data, &reply);
        return reply.readInt32();
    }

    int getProperty(const char *key, char *value){
        if(DEBUG){
            ALOGV("ISystemMixService::getproperty()  key = %s", key);
        }
        Parcel data, reply;
        data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
        data.writeCString(key);

        remote()->transact(GET_PROPERTY, data, &reply);
        int ret = reply.readInt32();
        ALOGD("get() ret=%d", ret);
        if(ret > 0){
            const char* rpy = reply.readCString();
            strcpy(value, rpy);
            ALOGD("get() value=%s  rpy=%s",value, rpy);
        }
        return ret;
    }

    int getFileData(int8_t *dat, int count, const char *filePath){
        if(DEBUG){
            ALOGV("ISystemMixService::getFileData()  filePath = %s, count = %d", filePath, count);
        }
        Parcel data, reply;
        data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
        data.writeInt32(count);
        data.writeCString(filePath);

        remote()->transact(GET_FILEDATA, data, &reply);
        int ret = reply.readInt32();
        ALOGD("getfiledata() ret=%d", ret);
        reply.read(dat, count * sizeof(int8_t));
        return ret;
    }

    int putFileData(int8_t *dat, int count, const char *filePath){
        if(DEBUG){
            ALOGV("ISystemMixService::putFileData()  filePath = %s, count = %d", filePath, count);
        }
        Parcel data, reply;
        data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
        data.writeInt32(count);
        data.write(dat,count);
        data.writeCString(filePath);

        remote()->transact(PUT_FILEDATA, data, &reply);
        int ret = reply.readInt32();
        ALOGD("putfiledata() ret=%d", ret);
        return ret;
    }

    int mountDev(const char *src, const char *mountPoint, const char *fs,
        unsigned int flag, const char *options){
        if(DEBUG){
            ALOGV("ISystemMixService::mount() source=%s, mountPoint=%s, fs=%s, flag=%d, options=%s",
                src, mountPoint, fs, flag, options);
        }
        Parcel data, reply;
        data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
        data.writeCString(src);
        data.writeCString(mountPoint);
        data.writeCString(fs);
        data.writeInt32(flag);
        data.writeCString(options);
        remote()->transact(MOUNT, data, &reply);
        int ret = reply.readInt32();
        return ret;
    }

    int umountDev(const char *mountPoint){
        if(DEBUG){
            ALOGD("ISystemMixService::umount() mountPoint=%s", mountPoint);
        }
        Parcel data, reply;
        data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
        data.writeCString(mountPoint);
        remote()->transact(UMOUNT, data, &reply);
        int ret = reply.readInt32();
        return ret;
    }
    int deleteFile(const char *pathName) {
    Parcel data, reply;
    data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
    data.writeCString(pathName);
    remote()->transact(DELETE, data, &reply);
    int ret = reply.readInt32();
    return ret;
 }

 int canWrite(const char *pathName) {
    Parcel data, reply;
    data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
    data.writeCString(pathName);
    remote()->transact(CANWRITE, data, &reply);
    int ret = reply.readInt32();
    return ret;
 }

 int canRead(const char *pathName) {
    Parcel data, reply;
    data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
    data.writeCString(pathName);
    remote()->transact(CANREAD, data, &reply);
    int ret = reply.readInt32();
    return ret;
 }
 int renewName(const char *oldPathName, const char *newPathName) {
    Parcel data, reply;
    data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
    data.writeCString(oldPathName);
    data.writeCString(newPathName);
    remote()->transact(RENAME, data, &reply);
    int ret = reply.readInt32();
    return ret;
 }

 int makeDir(const char *pathName) {
    Parcel data, reply;
    data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
    data.writeCString(pathName);
    remote()->transact(MKDIR, data, &reply);
    int ret = reply.readInt32();
    return ret;
 }

 int setBootAnimation(const char *pathName) {
    Parcel data, reply;
    data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
    data.writeCString(pathName);
    remote()->transact(SETBOOTANIMATION, data, &reply);
    int ret = reply.readInt32();
    return ret;
 }

 int setBootLogo(const char *pathName) {
    Parcel data, reply;
    data.writeInterfaceToken(ISystemMixService::getInterfaceDescriptor());
    data.writeCString(pathName);
    remote()->transact(SETBOOTLOGO, data, &reply);
    int ret = reply.readInt32();
    return ret;
 }

};

IMPLEMENT_META_INTERFACE(SystemMixService, "com.softwinner.ISystemMixService");

status_t BnSystemMixService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags){

    switch(code){
    case SET_PROPERTY:{
        CHECK_INTERFACE(ISystemMixService, data, reply);
        const char *key = data.readCString();
        const char *value = data.readCString();
        reply->writeInt32(setProperty(key, value));
        return NO_ERROR;
    }break;
    case GET_PROPERTY:{
        CHECK_INTERFACE(ISystemMixService, data, reply);
        const char *key = data.readCString();
        char *value = new char[PROPERTY_VALUE_MAX];
        ALOGD("BnSystemMixService key=%s", key);
        int ret = getProperty(key, value);
        ALOGD("BnSystemMixService value=%s, ret=%d", value, ret);
        reply->writeInt32(ret);
        if(ret > 0){
            reply->writeCString(value);
        }
        return NO_ERROR;
    }break;
    case GET_FILEDATA:{
        CHECK_INTERFACE(ISystemMixService, data, reply);
        int count = data.readInt32();
        const char *filePath = data.readCString();
        int8_t *d = new int8_t[count];
        memset(d, 0, count * sizeof(int8_t));
        int ret = getFileData(d, count, filePath);
        ALOGD("read data is %s", d);
        reply->writeInt32(ret);
        reply->write(d, count * sizeof(int8_t));
        delete [] d;
        return NO_ERROR;
    }break;
    case PUT_FILEDATA:{
        CHECK_INTERFACE(ISystemMixService, data, reply);
        //const uint8_t* bytes = NULL;
        //size_t size = 0;
        int count = data.readInt32();
        //readByteArray(data,&bytes,&size); 
        int8_t *bytes = new int8_t[count];
        data.read(bytes,count);
        const char *filePath = data.readCString();
        int ret = putFileData(bytes, count, filePath);
        ALOGD("write data is %s", bytes);
        reply->writeInt32(ret);
        delete [] bytes;
        return NO_ERROR;
    }break;
    case MOUNT:{
        CHECK_INTERFACE(ISystemMixService, data, reply);
        const char *src = data.readCString();
        const char *mountpoint = data.readCString();
        const char *fs = data.readCString();
        unsigned int flag = data.readInt32();
        const char *options = data.readCString();
        reply->writeInt32(mountDev(src, mountpoint, fs, flag, options));
        return NO_ERROR;
    }break;
    case UMOUNT:{
        CHECK_INTERFACE(ISystemMixService, data, reply);
        const char *umountpoint = data.readCString();
        reply->writeInt32(umountDev(umountpoint));
        return NO_ERROR;
    }break;
    case DELETE:{
        CHECK_INTERFACE(ISystemMixService, data, reply);
        const char *pathName = data.readCString();
        reply->writeInt32(deleteFile(pathName));
        return NO_ERROR;
    }break;
    case CANWRITE:{
        CHECK_INTERFACE(ISystemMixService, data, reply);
        const char *pathName = data.readCString();
        reply->writeInt32(canWrite(pathName));
        return NO_ERROR;
    }break;
    case RENAME:{
        CHECK_INTERFACE(ISystemMixService, data, reply);
        const char *oldName = data.readCString();
        const char *newName = data.readCString();
        reply->writeInt32(renewName(oldName, newName));
        return NO_ERROR;
    }break;
    case MKDIR:{
        CHECK_INTERFACE(ISystemMixService, data, reply);
        const char *pathName = data.readCString();
        reply->writeInt32(makeDir(pathName));
        return NO_ERROR;
    }break;
    case CANREAD:{
        CHECK_INTERFACE(ISystemMixService, data, reply);
        const char *pathName = data.readCString();
        reply->writeInt32(canRead(pathName));
        return NO_ERROR;
    }
    case SETBOOTANIMATION:{
        CHECK_INTERFACE(ISystemMixService, data, reply);
        const char *pathName = data.readCString();
        reply->writeInt32(setBootAnimation(pathName));
        return NO_ERROR;
    }break;
    case SETBOOTLOGO:{
        CHECK_INTERFACE(ISystemMixService, data, reply);
        const char *pathName = data.readCString();
        reply->writeInt32(setBootLogo(pathName));
        return NO_ERROR;
    }break;
    default:
        return BBinder::onTransact(code, data, reply, flags);
    }
}

};
