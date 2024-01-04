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

#ifndef ANDRIOD_GPIOSERVICE_H
#define ANDRIOD_GPIOSERVICE_H

#include <utils/Log.h>
#include <utils/Errors.h>

#include "IGpioService.h"

namespace android{

class GpioService:public BnGpioService{

public:
    static void instantiate();

    virtual int readData(const char * filePath);
    virtual int writeData(const char *data, int count, const char *filePath);

private:
    GpioService();
    virtual ~GpioService();
};
};
#endif
