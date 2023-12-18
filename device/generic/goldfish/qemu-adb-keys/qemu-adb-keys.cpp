/*
 * Copyright (C) 2021 The Android Open Source Project
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

/* this program is used to read a set of system properties and their values
 * from the emulator program and set them in the currently-running emulated
 * system. It does so by connecting to the 'boot-properties' qemud service.
 *
 * This program should be run as root and called from
 * /system/etc/init.ranchu.rc exclusively.
 */

#define LOG_TAG  "qemu-adb-keys"

#define DEBUG  0
//#define LOG_NDEBUG 0

#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <string.h>

#include <log/log.h>

#if DEBUG
#  define  DD(...)    ALOGD(__VA_ARGS__)
#else
#  define  DD(...)    ((void)0)
#endif

#define ADB_PUBKEY_PROP "qemu.adb.pubkey"
// init will copy over this file to /data/misc/adb/adb_keys
#define ADB_KEYS_FILE "/data/vendor/adb/adb_keys"

extern bool fs_mgr_get_boot_config(const std::string& key, std::string* out_val);

int main(void) {
    struct stat st;
    if (stat(ADB_KEYS_FILE, &st) == 0) {
        ALOGV("%s already exists", ADB_KEYS_FILE);
        return 0;
    }

    std::string adbkey_pub;
    if (!fs_mgr_get_boot_config(ADB_PUBKEY_PROP, &adbkey_pub)) {
        ALOGE("Failed to read %s bootconfig prop", ADB_PUBKEY_PROP);
        exit(1);
    }

    std::ofstream f;
    f.open(ADB_KEYS_FILE);
    if (!f.is_open()) {
        ALOGE("Failed to open %s\n", ADB_KEYS_FILE);
        exit(1);
    }

    ALOGV("Got %s=[%s]", ADB_PUBKEY_PROP, adbkey_pub.c_str());
    f << adbkey_pub;
    f.close();
    return 0;
}
