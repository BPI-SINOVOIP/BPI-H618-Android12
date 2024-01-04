/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_IISOMOUNTMANAGERSERVICE_H
#define ANDROID_IISOMOUNTMANAGERSERVICE_H

#include <utils/Errors.h>  // for status_t
#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

#define PATH_SIZE_MAX 512

typedef char Path[PATH_SIZE_MAX];

typedef struct _ISOMountManager_MountInfo{
    char    mMountPath[PATH_SIZE_MAX];
    char    mISOPath[PATH_SIZE_MAX];
    char    mDevPath[PATH_SIZE_MAX];
}ISOMountManager_MountInfo;

namespace android {

class IISOMountManagerService: public IInterface
{
public:
    DECLARE_META_INTERFACE(ISOMountManagerService);

    virtual int        isoMount(const char *mountPath, const char *isoPath) = 0;
    virtual int        isoUmount(const char *mountPath) = 0;
    virtual bool     umountAll() = 0;
    virtual int        getMountInfoList(ISOMountManager_MountInfo *plist, int length) = 0;
    virtual bool     getISOPath(const char *mountPath, char *isoPath) = 0;
    virtual int     getMountPoints(const char *isoPath, Path *mountPoints, int length) = 0;

    virtual int        getTotalMountCount() = 0;
    virtual int     getISOMountCount(const char *isoPath) = 0;
};

// ----------------------------------------------------------------------------

class BnISOMountManagerService: public BnInterface<IISOMountManagerService>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

}; // namespace android

#endif // ANDROID_IISOMOUNTMANAGERSERVICE_H
