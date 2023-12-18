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

#define LOG_TAG "com_softwinner_systemmix"
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
#include "ISystemMixService.h"
#include <cutils/properties.h>

using namespace android;

static sp<ISystemMixService> systemmixService;

static void init_native(JNIEnv *env){
    ALOGD("init");
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder;
    do{
        binder = sm->getService(String16("softwinner.systemmix"));
        if(binder != 0){
            break;
        }
        ALOGW("softwinner systemmix service not published, waiting...");
        usleep(500000);
    }while(true);

    systemmixService = interface_cast<ISystemMixService>(binder);
}

static void throw_NullPointerException(JNIEnv *env, const char* msg){
    jclass clazz;
    clazz = env->FindClass("java/lang/NullPointerException");
    env->ThrowNew(clazz, msg);
}

static int setBootAnimation(JNIEnv *env, jobject clazz, jstring jpathName) {
    if(systemmixService == NULL || jpathName == NULL){
        throw_NullPointerException(env, "systemmix service has not start or the filepath is null!");
    }
    const char *pathName = env->GetStringUTFChars(jpathName, NULL);
    int ret;
    ret = systemmixService->setBootAnimation(pathName);
    env->ReleaseStringUTFChars(jpathName, pathName);
    return ret;
}

static int setBootLogo(JNIEnv *env, jobject clazz, jstring jpathName) {
    if(systemmixService == NULL || jpathName == NULL){
        throw_NullPointerException(env, "systemmix service has not start or the filepath is null!");
    }
    const char *pathName = env->GetStringUTFChars(jpathName, NULL);
    int ret;

    ret = systemmixService->setBootLogo(pathName);
    env->ReleaseStringUTFChars(jpathName, pathName);
    return ret;
}


static JNINativeMethod method_table[] = {
    { "nativeInit", "()V", (void*)init_native},
    { "nativeSetBootAnimation", "(Ljava/lang/String;)I", (void*)setBootAnimation },
    { "nativeSetBootLogo", "(Ljava/lang/String;)I", (void*)setBootLogo },
};

static int register_android_os_AdManager(JNIEnv *env){
    return AndroidRuntime::registerNativeMethods(
        env, "com/softwinner/AdManager",method_table, NELEM(method_table));
}

jint JNI_OnLoad(JavaVM* vm, void* reserved){
    JNIEnv* env = NULL;
    jint result = -1;

    ALOGD("AdManager JNI_OnLoad()");

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    if (register_android_os_AdManager(env) < 0) {
        ALOGE("ERROR: AdManager native registration failed\n");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}

