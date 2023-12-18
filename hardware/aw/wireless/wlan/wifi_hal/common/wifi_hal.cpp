/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG  "wifi_hal-auto-detect"

#include <stdint.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <log/log.h>

#include "wifi_hal.h"
#include "libhwinfo.h"

/* initialize function pointer table with vendor HAL */
wifi_error init_wifi_vendor_hal_func_table(wifi_hal_fn *fn)
{
    if (fn == NULL) {
        return WIFI_ERROR_UNKNOWN;
    }

    const char *wifi_hal_name = get_wifi_hal_name();
    void *lib_handle;
    wifi_error (*init_wifi_vendor_hal_func_table_)(wifi_hal_fn *);

    if (wifi_hal_name == NULL) {
        ALOGE("unknown wifi hal name");
        return WIFI_ERROR_UNKNOWN;
    }

    ALOGD("libwifi hal name: %s", wifi_hal_name);
    lib_handle = dlopen(wifi_hal_name, RTLD_NOW);
    if (lib_handle == NULL) {
        ALOGE("dlopen %s fail", wifi_hal_name);
        return WIFI_ERROR_UNKNOWN;
    }

    init_wifi_vendor_hal_func_table_ = (wifi_error (*)(wifi_hal_fn *))dlsym(lib_handle, "init_wifi_vendor_hal_func_table");
    if (init_wifi_vendor_hal_func_table_ == NULL) {
        ALOGE("dlsym get interface fail");
        return WIFI_ERROR_UNKNOWN;
    }

    return init_wifi_vendor_hal_func_table_(fn);
}

