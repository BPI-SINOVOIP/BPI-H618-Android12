/*
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

#define LOG_TAG "JNIMultiir"
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
#include "IMultiirService.h"

using namespace android;

static sp<IMultiirService> multiirService;

static int init_native(JNIEnv *env) {
    int time = 100;
    ALOGD("init");
    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder;
    do {
        binder = sm->getService(String16("softwinner.multi_ir"));
        if(binder != 0){
            multiirService = interface_cast<IMultiirService>(binder);
            return 0;
        }
        ALOGW("softwinner multiir service not published, waiting...");
        usleep(500000);
    } while(time--);

    return -1;
}

static void throw_NullPointerException(JNIEnv *env, const char* msg) {
    jclass clazz;
    clazz = env->FindClass("java/lang/NullPointerException");
    env->ThrowNew(clazz, msg);
}

static int enterMouseMode(JNIEnv *env, jobject clazz) {
    if(multiirService == NULL){
        ALOGW("softwinner multiir service is not fount");
        return -1;
    }
    int ret = multiirService->enterMouseMode();
    return ret;
}

static int exitMouseMode(JNIEnv *env, jobject clazz) {
    if(multiirService == NULL){
        ALOGW("softwinner multiir service is not fount");
        return -1;
    }
    int ret = multiirService->exitMouseMode();
    return ret;
}

static int getDefaultPointerSpeed(JNIEnv *env, jobject clazz) {
    if(multiirService == NULL){
        ALOGW("softwinner multiir service is not fount");
        return -1;
    }
    int ret = multiirService->getDefaultPointerSpeed();
    return ret;
}

static int getDefaultStepDistance(JNIEnv *env, jobject clazz) {
    if(multiirService == NULL){
        ALOGW("softwinner multiir service is not fount");
        return -1;
    }
    int ret = multiirService->getDefaultStepDistance();
    return ret;
}

static int reset(JNIEnv *env, jobject clazz) {
    if(multiirService == NULL){
        ALOGW("softwinner multiir service is not fount");
        return -1;
    }
    int ret = multiirService->reset();
    return ret;
}

static int setPointerSpeed(JNIEnv *env, jobject clazz, jint ms) {
    if(multiirService == NULL){
        ALOGW("softwinner multiir service is not fount");
        return -1;
    }
    int ret = multiirService->setPointerSpeed(ms);
    return ret;
}

static int setStepDistance(JNIEnv *env, jobject clazz, jint px) {
    if(multiirService == NULL){
        ALOGW("softwinner multiir service is not fount");
        return -1;
    }
    int ret = multiirService->setStepDistance(px);
    return ret;
}

static int reportMouseKeyEvent(JNIEnv *env, jobject clazz, jint scanCode, jint keyState) {
    if(multiirService == NULL){
        ALOGW("softwinner multiir service is not fount");
        return -1;
    }
    int ret = multiirService->reportMouseKeyEvent(scanCode, keyState);
    return ret;
}

static JNINativeMethod method_table[] = {
    { "_nativeInit", "()I", (void*)init_native},
    { "_enterMouseMode", "()I", (void*)enterMouseMode },
    { "_exitMouseMode", "()I", (void*)exitMouseMode },
    { "_getDefaultPointerSpeed", "()I", (void*)getDefaultPointerSpeed },
    { "_getDefaultStepDistance", "()I", (void*)getDefaultStepDistance },
    { "_reset", "()I", (void*)reset },
    { "_setPointerSpeed", "(I)I", (void*)setPointerSpeed },
    { "_setStepDistance", "(I)I", (void*)setStepDistance },
    { "_reportMouseKeyEvent", "(II)I", (void*)reportMouseKeyEvent },
};

static int register_android_os_Multiir(JNIEnv *env){
    return AndroidRuntime::registerNativeMethods(
        env, "com/softwinner/Multiir",method_table, NELEM(method_table));
}

jint JNI_OnLoad(JavaVM* vm, void* reserved){
    JNIEnv* env = NULL;
    jint result = -1;

    ALOGD("Multiir JNI_OnLoad()");

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    if (register_android_os_Multiir(env) < 0) {
        ALOGE("ERROR: Multiir native registration failed\n");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}

