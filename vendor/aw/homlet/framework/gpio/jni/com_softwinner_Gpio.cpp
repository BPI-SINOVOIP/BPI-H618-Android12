/* //device/libs/android_runtime/android_os_Gpio.cpp
**
** Copyright 2006, The Android Open Source Project
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

#define LOG_TAG "com_softwinner_gpio"
#define LOG_NDEBUG 0

#include "JNIHelp.h"
#include "jni.h"
#include "android_runtime/AndroidRuntime.h"
#include "utils/Errors.h"
#include "utils/String8.h"
#include "android_util_Binder.h"
#include <stdio.h>
#include <assert.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include "IGpioService.h"

using namespace android;

static sp<IGpioService> gpioService;

static int init_native(JNIEnv *env){
    int time = 100;
    ALOGD("init");
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder;
    do {
        binder = sm->getService(String16("softwinner.gpio"));
        if(binder != 0){
            gpioService = interface_cast<IGpioService>(binder);
            return 0;
        }
        ALOGW("softwinner gpio service not published, waiting...");
        usleep(500000);
    } while(time--);

    return -1;
}

static void throw_NullPointerException(JNIEnv *env, const char* msg){
    jclass clazz;
    clazz = env->FindClass("java/lang/NullPointerException");
    env->ThrowNew(clazz, msg);
}

static int readGpio_native(JNIEnv *env, jobject clazz, jstring path){
    if(gpioService == NULL){
        throw_NullPointerException(env,"gpio service has not start!");
    }
    if(path == NULL){
        return -1;
    }
    const char *chars = env->GetStringUTFChars(path, NULL);
    int ret = gpioService->readData(chars);
    env->ReleaseStringUTFChars(path, chars);
    return ret;
}

static int writeGpio_native(JNIEnv *env, jobject clazz, jstring path, jstring value){
    if(gpioService == NULL){
        throw_NullPointerException(env,"gpio service has not start!");
    }
    if(path == NULL){
        return -1;
    }
    else{
        const char *chars = env->GetStringUTFChars(path, NULL);
        const char *valueStr = env->GetStringUTFChars(value, NULL);
        int ret = gpioService->writeData(valueStr, strlen(valueStr), chars);
        env->ReleaseStringUTFChars(path, chars);
        env->ReleaseStringUTFChars(value, valueStr);
        return ret;
    }
}

static JNINativeMethod method_table[] = {
    { "nativeInit", "()I", (void*)init_native},
    { "nativeWriteGpio", "(Ljava/lang/String;Ljava/lang/String;)I", (void*)writeGpio_native },
    { "nativeReadGpio", "(Ljava/lang/String;)I", (void*)readGpio_native },
};

static int register_android_os_Gpio(JNIEnv *env){
    return AndroidRuntime::registerNativeMethods(
        env, "com/softwinner/Gpio",method_table, NELEM(method_table));
}

jint JNI_OnLoad(JavaVM* vm, void* reserved){
    JNIEnv* env = NULL;
    jint result = -1;

    ALOGD("Gpio JNI_OnLoad()");

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    if (register_android_os_Gpio(env) < 0) {
        ALOGE("ERROR: Gpio native registration failed\n");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}

