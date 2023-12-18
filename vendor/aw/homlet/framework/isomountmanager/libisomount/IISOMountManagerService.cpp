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
#define LOG_TAG "IISOMountManagerService"
#include "utils/Log.h"

#include <memory.h>
#include <stdint.h>
#include <sys/types.h>

#include <binder/Parcel.h>
#include <binder/IMemory.h>
#include "IISOMountManagerService.h"


#include <utils/Errors.h>  // for status_t
#define DEBUG false

namespace android {

enum {
    MOUNT = IBinder::FIRST_CALL_TRANSACTION,
    UMOUNT,
    UMOUNT_ALL,
    GET_MOUNT_INFO_LIST,
    GET_ISO_PATH,
    GET_MOUNT_POINTS,

    GET_TOTAL_MOUNT_COUNT,
    GET_ISO_MOUNT_COUNT = IBinder::LAST_CALL_TRANSACTION
};

class BpISOMountManagerService: public BpInterface<IISOMountManagerService>
{
public:
    BpISOMountManagerService(const sp<IBinder>& impl)
        : BpInterface<IISOMountManagerService>(impl)
    {
    }

    int    isoMount(const char *mountPath, const char *isoPath){
//        if(DEBUG)
//            ALOGV("isoMount()..mountPath = %s, isoPath = %s", mountPath, isoPath);
        Parcel data, reply;
        data.writeInterfaceToken(IISOMountManagerService::getInterfaceDescriptor());
        data.writeCString(mountPath);
        data.writeCString(isoPath);
        remote()->transact(MOUNT, data, &reply);
        return reply.readInt32();
    }

    int    isoUmount(const char *mountPath){
//        if(DEBUG)
//            ALOGV("isoUmount()..mountPath = %s", mountPath);
        Parcel data, reply;
        data.writeInterfaceToken(IISOMountManagerService::getInterfaceDescriptor());
        data.writeCString(mountPath);
        remote()->transact(UMOUNT, data, &reply);
        return reply.readInt32();
    }

    bool umountAll(){
//        if(DEBUG)
//            ALOGV("umountAll()..");
        Parcel data, reply;
        data.writeInterfaceToken(IISOMountManagerService::getInterfaceDescriptor());
        remote()->transact(UMOUNT_ALL, data, &reply);
        return reply.readInt32();
    }

    int getMountInfoList(ISOMountManager_MountInfo *plist, int length){
//        if(DEBUG)
//            ALOGV("getMountInfoList()..length = %d", length);
        Parcel data, reply;
        data.writeInterfaceToken(IISOMountManagerService::getInterfaceDescriptor());
        data.writeInt32(length);
        remote()->transact(GET_MOUNT_INFO_LIST, data, &reply);
        length = reply.readInt32();
        reply.read(plist, length * sizeof(ISOMountManager_MountInfo));
        return length;
    }

    bool getISOPath(const char *mountPath, char *isoPath){
//        if(DEBUG)
//            ALOGV("getISOPath()..mountPath = %s", mountPath);
        Parcel data, reply;
        data.writeInterfaceToken(IISOMountManagerService::getInterfaceDescriptor());
        data.writeCString(mountPath);
        remote()->transact(GET_ISO_PATH, data, &reply);
        strncpy(isoPath, reply.readCString(), PATH_SIZE_MAX - 1);
        isoPath[PATH_SIZE_MAX - 1] = '\0';
        return reply.readInt32();
    }

    int getMountPoints(const char *isoPath, Path *mountPoints, int length){
//        if(DEBUG)
//            ALOGV("getMountPoints()..isoPath = %s", isoPath);
        Parcel data, reply;
        data.writeInterfaceToken(IISOMountManagerService::getInterfaceDescriptor());
        data.writeCString(isoPath);
        data.writeInt32(length);
        remote()->transact(GET_MOUNT_POINTS, data, &reply);
        length = reply.readInt32();
        for(int i = 0; i < length; i++){
            strncpy(mountPoints[i], reply.readCString(), PATH_SIZE_MAX);
        }
        return length;
    }


    int    getTotalMountCount(){
//        if(DEBUG)
//            ALOGV("getTotalMountCount()..");
        Parcel data, reply;
        data.writeInterfaceToken(IISOMountManagerService::getInterfaceDescriptor());
        remote()->transact(GET_TOTAL_MOUNT_COUNT, data, &reply);
        return reply.readInt32();
    }

    int getISOMountCount(const char *isoPath){
//        if(DEBUG)
//            ALOGV("getISOMountCount()..isoPath = %s", isoPath);
        Parcel data, reply;
        data.writeInterfaceToken(IISOMountManagerService::getInterfaceDescriptor());
        data.writeCString(isoPath);
        remote()->transact(GET_ISO_MOUNT_COUNT, data, &reply);
        return reply.readInt32();
    }
};

IMPLEMENT_META_INTERFACE(ISOMountManagerService, "com.softwinner.IISOMountManagerService");

// ----------------------------------------------------------------------

status_t BnISOMountManagerService::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch(code) {
        case MOUNT: {
            CHECK_INTERFACE(IISOMountManagerService, data, reply);
            const char *mountPath = data.readCString();
            const char *isoPath = data.readCString();
            reply->writeInt32(isoMount(mountPath, isoPath));
            return NO_ERROR;
        }break;
        case UMOUNT: {
            CHECK_INTERFACE(IISOMountManagerService, data, reply);
            const char *mountPath = data.readCString();
            reply->writeInt32(isoUmount(mountPath));
            return NO_ERROR;
        }break;
        case UMOUNT_ALL:{
            CHECK_INTERFACE(IISOMountManagerService, data, reply);
            reply->writeInt32(umountAll());
            return NO_ERROR;
        }break;
        case GET_MOUNT_INFO_LIST:{
            CHECK_INTERFACE(IISOMountManagerService, data, reply);
            int length = data.readInt32();
            ISOMountManager_MountInfo *list = new ISOMountManager_MountInfo[length];
            if(list == NULL){
                ALOGE("Fail in allocating memory.");
                return NO_MEMORY;
            }
            length = getMountInfoList(list, length);
            reply->writeInt32(length);
            reply->write(list, length * sizeof(ISOMountManager_MountInfo));
            delete [] list;
            return NO_ERROR;
        }break;
        case GET_ISO_PATH:{
            CHECK_INTERFACE(IISOMountManagerService, data, reply);
            const char *mountPath = data.readCString();
            char *isoPath = new char[PATH_SIZE_MAX];
            if(isoPath == NULL){
                ALOGE("Fail in allocating memory.");
                return NO_MEMORY;
            }
            bool ret = getISOPath(mountPath, isoPath);
            reply->writeCString(isoPath);
            reply->writeInt32(ret);
            delete [] isoPath;
            return NO_ERROR;
        }break;
        case GET_MOUNT_POINTS:{
            CHECK_INTERFACE(IISOMountManagerService, data, reply);
            const char *isoPath = data.readCString();
            int length = data.readInt32();
            Path *mountPoints = new Path[length];
            if(mountPoints == NULL){
                ALOGE("Fail in allocating memory.");
                return NO_MEMORY;
            }
            int count = getMountPoints(isoPath, mountPoints, length);
            reply->writeInt32(count);
            for(int i = 0; i < count; i++){
                reply->writeCString(mountPoints[i]);
            }
            delete [] mountPoints;
            return NO_ERROR;
        }break;
        case GET_TOTAL_MOUNT_COUNT:{
            CHECK_INTERFACE(IISOMountManagerService, data, reply);
            reply->writeInt32(getTotalMountCount());
            return NO_ERROR;
        }break;
        case GET_ISO_MOUNT_COUNT:{
            CHECK_INTERFACE(IISOMountManagerService, data, reply);
            const char *isoPath = data.readCString();
            reply->writeInt32(getISOMountCount(isoPath));
            return NO_ERROR;
        }break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

// ----------------------------------------------------------------------------

}; // namespace android
