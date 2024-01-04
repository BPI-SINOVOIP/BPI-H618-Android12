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

#ifndef ANDROID_ISOMOUNTMANAGERSERVICE_H
#define ANDROID_ISOMOUNTMANAGERSERVICE_H

#include <utils/Log.h>
#include <utils/Errors.h>

#include "IISOMountManagerService.h"

namespace android {

class ISOMountManagerService : public BnISOMountManagerService
{
public:
    static  void     instantiate();

    // IISOMountManagerService interface
    virtual int        isoMount(const char *mountPath, const char *isoPath);
    virtual int        isoUmount(const char *mountPath);
    virtual bool     umountAll();
    virtual int        getMountInfoList(ISOMountManager_MountInfo *plist, int length);
    virtual bool     getISOPath(const char *mountPath, char *isoPath);
    virtual int     getMountPoints(const char *isoPath, Path *mountPoints, int length);
    
    virtual int        getTotalMountCount();
    virtual int     getISOMountCount(const char *isoPath);

private:
    ISOMountManager_MountInfo **mInfoArray;
    int mCursor;
    
    virtual bool     isMountPointUsed(const char *mountPath);
    virtual void     addMountInfo(const char *mountPath, const char *isoPath, const char *devPath);
    virtual void     deleteMountInfo(const char *mountPath);
    virtual int     set_loop(char **device, const char *file, unsigned long long offset);
    virtual const char *getDevPath(const char *mountPath);
    
                                ISOMountManagerService();
    virtual                         ~ISOMountManagerService();
};

// ----------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_ISOMOUNTMANAGERSERVICE_H
