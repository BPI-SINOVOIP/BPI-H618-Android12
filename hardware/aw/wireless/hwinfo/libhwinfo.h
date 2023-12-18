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

#ifndef __LIB_HWINFO_H
#define __LIB_HWINFO_H

#if __cplusplus
extern "C" {
#endif

const char *get_wifi_vendor_name(void);
const char *get_wifi_module_name(void);
const char *get_wifi_driver_name(void);
const char *get_wifi_driver_module_name(void);
const char *get_driver_module_arg(void);
const char *get_wifi_hal_name(void);
const char *get_bluetooth_libbt_name(void);
int   get_bluetooth_is_support(void);

#if __cplusplus
};
#endif

#endif
