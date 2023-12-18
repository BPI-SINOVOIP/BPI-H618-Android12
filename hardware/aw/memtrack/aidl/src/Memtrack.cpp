/*
 * Copyright (C) 2020 The Android Open Source Project
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
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <log/log.h>
#include <hardware/memtrack.h>
#include <dmabufinfo/dmabufinfo.h>
#include <cutils/properties.h>

#include "Memtrack.h"

namespace aidl {
namespace android {
namespace hardware {
namespace memtrack {

#define MEMTRACK_LOWLEVEL_LIB_PATH "/vendor/lib/hw/memtrack_aidl.sunxi.so"

#define INIT_INTERNAL_FUNC "hal_init"
#define GET_MEMORY "get_memory"

typedef void (*INIT_FUNC)(void);
typedef void (*EXIT_FUNC)(void);
typedef int (*GET_MEMORY_FUNC)(pid_t pid, int type,
        struct memtrack_record *records, size_t *num_records);

static GET_MEMORY_FUNC getMemoryFunc;

//debug property
static unsigned long long iDebugLevel;
static unsigned char iDebugLevelHasParsed;

static unsigned char get_memtrack_debug(void)
{
    char szPropDebugLevel[100];
    char *pcValueEnd;
    unsigned int iPropLen = 0;

    if (!iDebugLevelHasParsed) {
        iPropLen = property_get("vendor.product.memtrack.debug",
                        szPropDebugLevel, NULL);
        iDebugLevelHasParsed = 1;
        if (iPropLen > 0) {
            iDebugLevel = strtol(szPropDebugLevel, &pcValueEnd, 0);
        }
    }

    return (unsigned char)iDebugLevel;
}


static void *handle;
int Memtrack::init() {
    //char *error;

    handle = dlopen(MEMTRACK_LOWLEVEL_LIB_PATH, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        ALOGE("%s dlopen %s failed!\n", dlerror(), MEMTRACK_LOWLEVEL_LIB_PATH);
        return -1;
    }

    dlerror();

    getMemoryFunc = (GET_MEMORY_FUNC)dlsym(handle, GET_MEMORY);
    if (!getMemoryFunc) {
       ALOGE("dlsym get_memory failed!\n");
       return -1;
    }

    return 0;
}

void Memtrack::exit()
{
    dlclose(handle);
}

ndk::ScopedAStatus Memtrack::getMemory(int pid, MemtrackType type,
                                       std::vector<MemtrackRecord>* _aidl_return) {
    if (pid < 0) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
    }

    if (type != MemtrackType::OTHER && type != MemtrackType::GL && type != MemtrackType::GRAPHICS &&
        type != MemtrackType::MULTIMEDIA && type != MemtrackType::CAMERA) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }

    if (type < MemtrackType::GL || type > MemtrackType::GRAPHICS)
        return ndk::ScopedAStatus::ok();

    _aidl_return->clear();

    return ndk::ScopedAStatus::ok();

    size_t num_records = 0;
    struct memtrack_record *record;

    int ret = getMemoryFunc(pid, (int)type, NULL, &num_records);
    if (ret < 0) {
        ALOGE("getMemoryFunc count failed\n");
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_PARCELABLE));
    }

    if (get_memtrack_debug())
        ALOGD("[%s] pid:%d type:%d num_records:%d\n",
            __func__, pid, (int)type, num_records);

    record = (struct memtrack_record *)
                    malloc(num_records * sizeof(struct memtrack_record));

    ret = getMemoryFunc(pid, (int)type, record, &num_records);
    if (ret < 0) {
        ALOGE("getMemoryFunc failed\n");
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_PARCELABLE));
    }

    for (int i = 0; i < num_records; i++) {
        MemtrackRecord instance;
        instance.flags = record[i].flags;
        instance.sizeInBytes = record[i].size_in_bytes;

        _aidl_return->push_back(instance);
    }

    free(record);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Memtrack::getGpuDeviceInfo(std::vector<DeviceInfo>* _aidl_return) {
    _aidl_return->clear();
    DeviceInfo dev_info = {.id = 0, .name = "gpu"};
    _aidl_return->emplace_back(dev_info);
    return ndk::ScopedAStatus::ok();
}

}  // namespace memtrack
}  // namespace hardware
}  // namespace android
}  // namespace aidl
