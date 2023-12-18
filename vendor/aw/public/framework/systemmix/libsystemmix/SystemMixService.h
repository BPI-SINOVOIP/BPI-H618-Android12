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

#ifndef ANDRIOD_SYSTEMMIXSERVICE_H
#define ANDRIOD_SYSTEMMIXSERVICE_H

#include <utils/Log.h>
#include <utils/Errors.h>

#include "ISystemMixService.h"

namespace android{

class SystemMixService:public BnSystemMixService{

public:
    static void instantiate();

    virtual int getProperty(const char *key, char *value);
    virtual int setProperty(const char *key, const char *value);
    virtual int getFileData(int8_t *data, int count, const char *filePath);
    virtual int putFileData(int8_t *data, int count, const char *filePath);
    virtual int mountDev(const char *src, const char *mountPoint, const char *fs,
        unsigned int flag, const char *options);
    virtual int umountDev(const char *mountPoint);
    virtual int deleteFile(const char *pathName);
    virtual int canWrite(const char *pathName);
    virtual int renewName(const char *oldPathName, const char *newPathName);
    virtual int makeDir(const char *pathName);
    virtual int canRead(const char *pathName);
    virtual int setBootAnimation(const char *pathName);
    virtual int setBootLogo(const char *pathName);

private:
    SystemMixService();
    int updateReserve0Crc();
    virtual ~SystemMixService();
};
};
#endif
