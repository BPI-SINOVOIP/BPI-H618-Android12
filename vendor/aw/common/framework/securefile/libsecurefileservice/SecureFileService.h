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

#ifndef ANDROID_SECUREFILESERVICE_H
#define ANDROID_SECUREFILESERVICE_H

#include <utils/Log.h>
#include <utils/Errors.h>

#include "ISecureFileService.h"

namespace android {


class SecureFileService : public BnSecureFileService
{
public:
    static  void     instantiate();

    // ISecureFileService interface
    virtual bool     isDirectory(const char *path);
    virtual bool     mkDir(const char *path);
    virtual bool     createFile(const char *path);
    virtual bool     renameTo(const char * oldPath, const char * newPath);
    virtual bool     deleteFile(const char *path);
    virtual long long   length(const char *path);
    virtual unsigned long     lastModified(const char *path);
    virtual bool     isFile(const char *path);
    virtual bool     exists(const char *path);
    virtual bool     setLastModified(const char *path, unsigned long ms);
    virtual long long    getFreeSpace(const char *path);
    virtual long long    getTotalSpace(const char *path);
    virtual long long   getUsableSpace(const char *path);
    virtual int      getCount(const char *path);
    virtual int      list(const char *path, SecureItemName *itemArray, int count);
    virtual bool     writeToFile(const char *srcPath, const char *desPath, bool append);
    virtual bool     writeInData(int8_t *data, int count, const char *desPath, bool append);
    virtual bool     read(int8_t *data, int count, const char *srcPath);

private:
                                SecureFileService();
    virtual                         ~SecureFileService();
};

// ----------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_SECUREFILESERVICE_H
