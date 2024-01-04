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

#ifndef ANDROID_ISECUREFILESERVICE_H
#define ANDROID_ISECUREFILESERVICE_H

#include <utils/Errors.h>  // for status_t
#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

namespace android {

#define SECUREFILE_FILE_NAME_LEN_MAX    256

typedef char SecureItemName[SECUREFILE_FILE_NAME_LEN_MAX];

class ISecureFileService: public IInterface
{
public:
    DECLARE_META_INTERFACE(SecureFileService);

    virtual bool    createFile(const char *path) = 0;
    virtual bool    deleteFile(const char *path) = 0;
    virtual bool     exists(const char *path) = 0;
    virtual long long     getTotalSpace(const char *path) = 0;
    virtual long long    getUsableSpace(const char *path) = 0;
    virtual bool    isDirectory(const char *path) = 0;
    virtual bool     isFile(const char *path) = 0;
    virtual long long    length(const char *path) = 0;
    virtual int        getCount(const char *path) = 0;
    virtual int        list(const char *path, SecureItemName *itemArray, int count) = 0;
    virtual bool    mkDir(const char *path) = 0;
    virtual unsigned long    lastModified(const char *path) = 0;
    virtual bool    renameTo(const char *oldPath, const char *newPath) = 0;
    virtual bool    setLastModified(const char *path, unsigned long time) = 0;
    virtual bool     writeToFile(const char *srcPath, const char *desPath, bool append) = 0;
    virtual bool    writeInData(int8_t *data, int count, const char *desPath, bool append) = 0;
    virtual bool    read(int8_t *data, int count, const char *srcPath) = 0;

};

// ----------------------------------------------------------------------------

class BnSecureFileService: public BnInterface<ISecureFileService>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

}; // namespace android

#endif // ANDROID_ISECUREFILESERVICE_H
