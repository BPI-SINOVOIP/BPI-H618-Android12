/*
 * Copyright (C) 2011 The Android Open Source Project
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

#define LOG_TAG "SW_Config"
#define LOG_NDEBUG 0
#include "JNIHelp.h"
#include "jni.h"
#include "android_runtime/AndroidRuntime.h"
#include "utils/Errors.h"
#include "utils/String8.h"
#include <stdio.h>
#include <assert.h>

#include "../libswconfig/swconfig.h"

using namespace android;

#define TARGET_BOARD_PLATFORM 0x00
#define TARGET_BUSINESS_PLATFORM 0x01

static jint getTargetPlatform(JNIEnv *env, jobject clazz, jint type){
    switch(type){
    case TARGET_BOARD_PLATFORM:
        return getBoardPlatform();

    case TARGET_BUSINESS_PLATFORM:
        return getBusinessPlatform();

    default:
        return UNKNOWN_PLATFORM;
    }
}

static JNINativeMethod method_table[] = {
    { "nativeGetTargetPlatform", "(I)I", (void*)getTargetPlatform },
};

static int register_method(JNIEnv *env){
    return AndroidRuntime::registerNativeMethods(
        env, "com/softwinner/utils/Config", method_table, NELEM(method_table)
    );
}

jint JNI_OnLoad(JavaVM* vm, void* reserved){
    JNIEnv* env = NULL;
    jint result = -1;
    ALOGD("Config JNI_OnLoad()");
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    if (register_method(env) < 0) {
        ALOGE("ERROR: Config native registration failed\n");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}
