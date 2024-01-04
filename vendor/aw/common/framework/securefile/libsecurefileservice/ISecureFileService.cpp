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
#define LOG_TAG "ISecureService"
#include "utils/Log.h"

#include <memory.h>
#include <stdint.h>
#include <sys/types.h>

#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include "ISecureFileService.h"


#include <utils/Errors.h>  // for status_t
#define DEBUG true

namespace android {

enum {
    CREATE_FILE = IBinder::FIRST_CALL_TRANSACTION,
    DELETE_FILE,
    EXISTS,
    GET_TOTAL_SPACE,
    GET_USABLE_SPACE,
    IS_DIRECTORY,
    IS_FILE,
    LENGTH,
    GET_COUNT,
    LIST,
    MKDIR,
    LAST_MODIFIED,
    RENAME_TO,
    SET_LAST_MODIFIED,
    WRITE_TO_FILE,
    WRITE_IN_DATA,
    READ = IBinder::LAST_CALL_TRANSACTION
};

class BpSecureFileService: public BpInterface<ISecureFileService>
{
public:
    BpSecureFileService(const sp<IBinder>& impl)
        : BpInterface<ISecureFileService>(impl)
    {
    }

    bool createFile(const char *path){
        if(DEBUG)
            ALOGV("ISecureFileService::createFile()  path = %s",path);
        Parcel data, reply;
        data.writeInterfaceToken(ISecureFileService::getInterfaceDescriptor());
        data.writeCString(path);
        remote()->transact(CREATE_FILE, data, &reply);
        return reply.readInt32();
    }

    bool deleteFile(const char *path){
        if(DEBUG)
            ALOGV("ISecureFileService::deleteFile()  path = %s",path);
        Parcel data, reply;
        data.writeInterfaceToken(ISecureFileService::getInterfaceDescriptor());
        data.writeCString(path);
        remote()->transact(DELETE_FILE, data, &reply);
        return reply.readInt32();
    }

    bool exists(const char *path){
        if(DEBUG)
            ALOGV("ISecureFileService::exists()  path = %s",path);
        Parcel data, reply;
        data.writeInterfaceToken(ISecureFileService::getInterfaceDescriptor());
        data.writeCString(path);
        remote()->transact(EXISTS, data, &reply);
        return reply.readInt32();
    }

    long long getTotalSpace(const char *path){
        if(DEBUG)
            ALOGV("ISecureFileService::getTotalSpace()  path = %s",path);
        Parcel data, reply;
        data.writeInterfaceToken(ISecureFileService::getInterfaceDescriptor());
        data.writeCString(path);
        remote()->transact(GET_TOTAL_SPACE, data, &reply);
        return reply.readInt64();
    }

    long long getUsableSpace(const char *path){
        if(DEBUG)
            ALOGV("ISecureFileService::getUsabelSpace()  path = %s",path);
        Parcel data, reply;
        data.writeInterfaceToken(ISecureFileService::getInterfaceDescriptor());
        data.writeCString(path);
        remote()->transact(GET_USABLE_SPACE, data, &reply);
        return reply.readInt64();
    }

    bool isDirectory(const char *path)
    {
        if(DEBUG)
            ALOGV("ISecureFileService::isDirectory()  path = %s",path);
        Parcel data, reply;
        data.writeInterfaceToken(ISecureFileService::getInterfaceDescriptor());
        data.writeCString(path);
        remote()->transact(IS_DIRECTORY, data, &reply);
        return reply.readInt32();
    }

    bool isFile(const char *path){
        if(DEBUG)
            ALOGV("ISecureFileService::isFile()  path = %s",path);
        Parcel data, reply;
        data.writeInterfaceToken(ISecureFileService::getInterfaceDescriptor());
        data.writeCString(path);
        remote()->transact(IS_FILE, data, &reply);
        return reply.readInt32();
    }

    long long length(const char *path){
        if(DEBUG)
            ALOGV("ISecureFileService::length()  path = %s",path);
        Parcel data, reply;
        data.writeInterfaceToken(ISecureFileService::getInterfaceDescriptor());
        data.writeCString(path);
        remote()->transact(LENGTH, data, &reply);
        return reply.readInt64();
    }

    int getCount(const char *path){
        if(DEBUG)
            ALOGV("ISecureFileService::getCount()  path = %s",path);
        Parcel data, reply;
        data.writeInterfaceToken(ISecureFileService::getInterfaceDescriptor());
        data.writeCString(path);
        remote()->transact(GET_COUNT, data, &reply);
        return reply.readInt32();
    }

    int    list(const char *path, SecureItemName *itemArray, int count){
        if(DEBUG)
            ALOGV("ISecureFileService::list() path = %s",path);
        Parcel data, reply;
        data.writeInterfaceToken(ISecureFileService::getInterfaceDescriptor());
        data.writeCString(path);
        data.writeInt32(count);
        remote()->transact(LIST, data, &reply);
        int num = reply.readInt32();
        SecureItemName *p = itemArray;
        for (int i = 0; i < num; ++i, p++) {
            strncpy(*p, reply.readCString(), SECUREFILE_FILE_NAME_LEN_MAX - 1);
            (*p)[SECUREFILE_FILE_NAME_LEN_MAX-1] = '\0';
        }
        return num;
    }

    bool mkDir(const char *path)
    {
        if(DEBUG)
            ALOGV("ISecureFileService::mkDir()  path = %s",path);
        Parcel data, reply;
        data.writeInterfaceToken(ISecureFileService::getInterfaceDescriptor());
        data.writeCString(path);
        remote()->transact(MKDIR, data, &reply);
        return reply.readInt32();
    }

    unsigned long lastModified(const char *path){
        if(DEBUG)
            ALOGV("ISecureFileService::lastModified()  path = %s",path);
        Parcel data, reply;
        data.writeInterfaceToken(ISecureFileService::getInterfaceDescriptor());
        data.writeCString(path);
        remote()->transact(LAST_MODIFIED, data, &reply);
        return reply.readInt64();
    }

    bool renameTo(const char *oldPath, const char *newPath){
        if(DEBUG)
            ALOGV("ISecureFileService::renameTo()  oldPath = %s  newPath = %s",oldPath,newPath);
        Parcel data, reply;
        data.writeInterfaceToken(ISecureFileService::getInterfaceDescriptor());
        data.writeCString(oldPath);
        data.writeCString(newPath);
        remote()->transact(RENAME_TO, data, &reply);
        return reply.readInt32();
    }

    bool setLastModified(const char *path, unsigned long time){
        if(DEBUG)
            ALOGV("ISecueFileService::setLastModified()  path = %s  time = %ld",path,time);
        Parcel data, reply;
        data.writeInterfaceToken(ISecureFileService::getInterfaceDescriptor());
        data.writeCString(path);
        data.writeInt64(time);
        remote()->transact(SET_LAST_MODIFIED, data, &reply);
        return reply.readInt32();
    }

    bool writeToFile(const char *srcPath, const char *desPath, bool append){
        if(DEBUG)
            ALOGV("ISecureFileService::writeToFile()  srcPath = %s  desPath = %s  append = %d",
                    srcPath,desPath,append);
        Parcel data, reply;
        data.writeInterfaceToken(ISecureFileService::getInterfaceDescriptor());
        data.writeCString(srcPath);
        data.writeCString(desPath);
        data.writeInt32(append);
        remote()->transact(WRITE_TO_FILE, data, &reply);
        return reply.readInt32();
    }

    bool writeInData(int8_t *dat, int count, const char *desPath, bool append){
        if(DEBUG)
            ALOGV("ISecureFileService::writeInData()  desPath = %s  append = %d  count = %d",
                    desPath,append,count);
        Parcel data, reply;
        data.writeInterfaceToken(ISecureFileService::getInterfaceDescriptor());
        data.writeInt32(count);
        data.writeCString(desPath);
        data.writeInt32(append);
        data.write(dat, count * sizeof(int8_t));
        remote()->transact(WRITE_IN_DATA, data, &reply);
        return reply.readInt32();

    }

    bool read(int8_t *dat, int count, const char *srcPath){
        if(DEBUG)
            ALOGV("ISecureFileService::read()  srcPath = %s  count = %d",srcPath,count);
        Parcel data, reply;
        data.writeInterfaceToken(ISecureFileService::getInterfaceDescriptor());
        data.writeInt32(count);
        data.writeCString(srcPath);

        remote()->transact(READ, data, &reply);
        reply.read(dat, count * sizeof(int8_t));
        return reply.readInt32();
    }
};

IMPLEMENT_META_INTERFACE(SecureFileService, "com.softwinner.ISecureFileService");

// ----------------------------------------------------------------------

status_t BnSecureFileService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        case CREATE_FILE: {
            CHECK_INTERFACE(ISecureFileService, data, reply);
            const char *path = data.readCString();
            reply->writeInt32(createFile(path));
            return NO_ERROR;
        }break;
        case DELETE_FILE: {
            CHECK_INTERFACE(ISecureFileService, data, reply);
            const char *path = data.readCString();
            reply->writeInt32(deleteFile(path));
            return NO_ERROR;
        }break;
        case EXISTS: {
            CHECK_INTERFACE(ISecureFileService, data, reply);
            const char *path = data.readCString();
            reply->writeInt32(exists(path));
            return NO_ERROR;
        }break;
        case GET_TOTAL_SPACE: {
            CHECK_INTERFACE(ISecureFileService, data, reply);
            const char *path = data.readCString();
            reply->writeInt64(getTotalSpace(path));
            return NO_ERROR;
        }break;
        case GET_USABLE_SPACE: {
            CHECK_INTERFACE(ISecureFileService, data, reply);
            const char *path = data.readCString();
            reply->writeInt64(getUsableSpace(path));
            return NO_ERROR;
        }break;
        case IS_DIRECTORY: {
            CHECK_INTERFACE(ISecureFileService, data, reply);
            const char *path = data.readCString();
            reply->writeInt32(isDirectory(path));
            return NO_ERROR;
        } break;
        case IS_FILE: {
            CHECK_INTERFACE(ISecureFileService, data, reply);
            const char *path = data.readCString();
            reply->writeInt32(isFile(path));
            return NO_ERROR;
        }break;
        case LENGTH: {
            CHECK_INTERFACE(ISecureFileService, data, reply);
            const char *path = data.readCString();
            reply->writeInt64(length(path));
            return NO_ERROR;
        }break;
        case GET_COUNT: {
            CHECK_INTERFACE(ISecureFileService, data, reply);
            const char *path = data.readCString();
            reply->writeInt32(getCount(path));
            return NO_ERROR;
        }break;
        case LIST: {
            CHECK_INTERFACE(ISecureFileService, data, reply);
            const char *path = data.readCString();
            int  count = data.readInt32();
            SecureItemName *array = new SecureItemName[count];
            //char array[count][256];
            int num = list(path,array,count);
            reply->writeInt32(num);
            for(int i = 0; i < num; i++){
                reply->writeCString(array[i]);
            }
            delete [] array;
            return NO_ERROR;
        }break;
        case MKDIR: {
            CHECK_INTERFACE(ISecureFileService, data, reply);
            const char *path = data.readCString();
            reply->writeInt32(mkDir(path));
            return NO_ERROR;
        }break;
        case LAST_MODIFIED: {
            CHECK_INTERFACE(ISecureFileService, data, reply);
            const char *path = data.readCString();
            reply->writeInt64(lastModified(path));
            return NO_ERROR;
        }break;
        case RENAME_TO: {
            CHECK_INTERFACE(ISecureFileService, data, reply);
            const char *oldPath = data.readCString();
            const char *newPath = data.readCString();
            reply->writeInt32(renameTo(oldPath, newPath));
            return NO_ERROR;
        }break;
        case SET_LAST_MODIFIED: {
            CHECK_INTERFACE(ISecureFileService, data, reply);
            const char *path = data.readCString();
            unsigned long time = data.readInt64();
            reply->writeInt32(setLastModified(path, time));
            return NO_ERROR;
        }break;
        case WRITE_TO_FILE: {
            CHECK_INTERFACE(ISecureFileService, data, reply);
            const char *srcPath = data.readCString();
            const char *desPath = data.readCString();
            bool append = data.readInt32();
            reply->writeInt32(writeToFile(srcPath, desPath, append));
            return NO_ERROR;
        }break;
        case WRITE_IN_DATA: {
            CHECK_INTERFACE(ISecureFileService, data, reply);
            int count = data.readInt32();
            const char *desPath = data.readCString();
            bool append = data.readInt32();
            int8_t *d = new int8_t[count];
            data.read(d, count * sizeof(int8_t));
            reply->writeInt32(writeInData(d, count, desPath, append));
            delete [] d;
            return NO_ERROR;
        }break;
        case READ: {
            CHECK_INTERFACE(ISecureFileService, data, reply);
            int count = data.readInt32();
            const char *srcPath = data.readCString();
            int8_t *d = new int8_t[count];
            memset(d, 0, count * sizeof(int8_t));
            ALOGV("init data");
            bool ret = read(d, count, srcPath);
            reply->write(d, count * sizeof(int8_t));
            reply->writeInt32(ret);
            delete [] d;
            return NO_ERROR;
        }break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

}; // namespace android
