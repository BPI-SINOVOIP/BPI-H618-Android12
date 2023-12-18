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

#ifndef ANDRIOD_ISYSTEMMIXSERVICE_H
#define ANDRIOD_ISYSTEMMIXSERVICE_H

#include <utils/Errors.h>  // for status_t
#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

namespace android {

class ISystemMixService: public IInterface{

public:
    DECLARE_META_INTERFACE(SystemMixService);

    virtual int getProperty(const char *key, char *value) = 0;
    virtual int setProperty(const char *key, const char *value) = 0;
    virtual int getFileData(int8_t *data, int count, const char *filePath) = 0;
    virtual int putFileData(int8_t *data, int count, const char *filePath) = 0;
    virtual int mountDev(const char *src, const char *mountPoint, const char *fs,
        unsigned int flags, const char *options) = 0;
    virtual int umountDev(const char *mountPoint) = 0;
    virtual int deleteFile(const char *pathName) = 0;
    virtual int canWrite(const char *pathName) = 0;
    virtual int renewName(const char *oldPathName, const char *newPathName) = 0;
    virtual int makeDir(const char *pathName) = 0;
    virtual int canRead(const char *pathName) = 0;
    virtual int setBootAnimation(const char *pathName) = 0;
    virtual int setBootLogo(const char *pathName) = 0;
};

class BnSystemMixService: public BnInterface<ISystemMixService>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};
};

#endif
