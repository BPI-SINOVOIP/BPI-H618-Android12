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

static jstring getProperty_native(JNIEnv *env, jobject clazz, jstring key){
    jstring value = NULL;
    if(systemmixService == NULL || key == NULL){
        throw_NullPointerException(env,"systemmix service has not start, or key is null!");
    }
    const char *ckey = env->GetStringUTFChars(key, NULL);
    char *cvalue = new char[PROPERTY_VALUE_MAX];
    if(cvalue == NULL){
        env->ReleaseStringUTFChars(key, ckey);
        throw_NullPointerException(env,"fail to allocate memory for value");
    }

    int ret = systemmixService->getProperty(ckey, cvalue);
    if(ret > 0){
        value = env->NewStringUTF(cvalue);
        if(value == NULL){
            ALOGE("Fail in creating java string with %s", cvalue);
        }
    }
    delete [] cvalue;
    env->ReleaseStringUTFChars(key, ckey);
    return value;
}

static int setProperty_native(JNIEnv *env, jobject clazz, jstring key, jstring value){
    if(systemmixService == NULL){
        throw_NullPointerException(env,"systemmix service has not start!");
    }
    if(key == NULL || value == NULL){
        return -1;
    }else{
        const char *ckey = env->GetStringUTFChars(key, NULL);
        const char *cvalue = env->GetStringUTFChars(value, NULL);
        int ret = systemmixService->setProperty(ckey, cvalue);
        env->ReleaseStringUTFChars(key, ckey);
        env->ReleaseStringUTFChars(value, cvalue);
        return ret;
    }
}

static int getFileData_native(JNIEnv *env, jobject clazz, jbyteArray desData,
        jint count, jstring filePath){
    int max = 256;
    int ret = 0;
    if(systemmixService == NULL  || desData == NULL || filePath == NULL){
        throw_NullPointerException(env, "systemmix service has not start or the filepath is null!");
    }
    const char *cFilePath = env->GetStringUTFChars(filePath, NULL);
    if(cFilePath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return 0;
    }
    jbyte *data = env->GetByteArrayElements(desData, NULL);
    if(data == NULL){
        ALOGE("Fail in converting jbyteArray to jbyte.");
        goto error;
    }
    ret = systemmixService->getFileData(data, count, cFilePath);
    env->ReleaseStringUTFChars(filePath, cFilePath);
    env->ReleaseByteArrayElements(desData, data, 0);
    return ret;
error:
    env->ReleaseStringUTFChars(filePath, cFilePath);
    return 0;
}

static int putFileData_native(JNIEnv *env, jobject clazz, jbyteArray desData,
        jint count, jstring filePath){
    int ret = 0;
    if(systemmixService == NULL  || desData == NULL || filePath == NULL){
        throw_NullPointerException(env, "systemmix service has not start or the filepath is null!");
    }
    const char *cFilePath = env->GetStringUTFChars(filePath, NULL);
    if(cFilePath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return 0;
    }
    jbyte *data = env->GetByteArrayElements(desData, NULL);
    if(data == NULL){
        ALOGE("Fail in converting jbyteArray to jbyte.");
        goto error;
    }
    ret = systemmixService->putFileData(data, count, cFilePath);
    env->ReleaseStringUTFChars(filePath, cFilePath);
    env->ReleaseByteArrayElements(desData, data, 0);
    return ret;
error:
    env->ReleaseStringUTFChars(filePath, cFilePath);
    return 0;
}

static int mount_native(JNIEnv *env, jobject clazz, jstring jsource,
        jstring jmountPoint, jstring jfs,
        jint jflags, jstring joptions){
    if(systemmixService == NULL  || jsource == NULL || jmountPoint == NULL || jfs == NULL ||
        joptions == NULL){
        throw_NullPointerException(env, "systemmix service has not start or the filepath is null!");
    }
    const char *source = env->GetStringUTFChars(jsource, NULL);
    const char *mountPoint = env->GetStringUTFChars(jmountPoint, NULL);
    const char *fs = env->GetStringUTFChars(jfs, NULL);
    unsigned int flags = jflags;
    const char *options = env->GetStringUTFChars(joptions, NULL);
    int ret;
    ret = systemmixService->mountDev(source, mountPoint, fs, flags, options);
    env->ReleaseStringUTFChars(jsource, source);
    env->ReleaseStringUTFChars(jmountPoint, mountPoint);
    env->ReleaseStringUTFChars(jfs, fs);
    env->ReleaseStringUTFChars(joptions, options);
    return ret;
}

static int umount_native(JNIEnv *env, jobject clazz, jstring jmountPoint){
    if(systemmixService == NULL || jmountPoint == NULL){
        throw_NullPointerException(env, "systemmix service has not start or the filepath is null!");
    }
    const char *mountPoint = env->GetStringUTFChars(jmountPoint, NULL);
    int ret;
    ret = systemmixService->umountDev(mountPoint);
    env->ReleaseStringUTFChars(jmountPoint, mountPoint);
    return ret;
}

static int delete_native(JNIEnv *env, jobject clazz, jstring jpathName) {
    if(systemmixService == NULL || jpathName == NULL){
        throw_NullPointerException(env, "systemmix service has not start or the filepath is null!");
    }
    const char *pathName = env->GetStringUTFChars(jpathName, NULL);
    int ret;
    ret = systemmixService->deleteFile(pathName);
    env->ReleaseStringUTFChars(jpathName, pathName);
    return ret;
}

static int canWrite_native(JNIEnv *env, jobject clazz, jstring jpathName) {
    if(systemmixService == NULL || jpathName == NULL){
        throw_NullPointerException(env, "systemmix service has not start or the filepath is null!");
    }
    const char *pathName = env->GetStringUTFChars(jpathName, NULL);
    int ret;
    ret = systemmixService->canWrite(pathName);
    env->ReleaseStringUTFChars(jpathName, pathName);
    return ret;
}

static int canRead_native(JNIEnv *env, jobject clazz, jstring jpathName) {
    if(systemmixService == NULL || jpathName == NULL){
        throw_NullPointerException(env, "systemmix service has not start or the filepath is null!");
    }
    const char *pathName = env->GetStringUTFChars(jpathName, NULL);
    int ret;
    ret = systemmixService->canRead(pathName);
    env->ReleaseStringUTFChars(jpathName, pathName);
    return ret;
}

static int rename_native(JNIEnv *env, jobject clazz, jstring joldPathName, jstring jnewPathName) {
    if(systemmixService == NULL || joldPathName == NULL || jnewPathName == NULL){
        throw_NullPointerException(env, "systemmix service has not start or the filepath is null!");
    }
    const char *oldPathName = env->GetStringUTFChars(joldPathName, NULL);
    const char *newPathName = env->GetStringUTFChars(jnewPathName, NULL);
    int ret;
    ret = systemmixService->renewName(oldPathName, newPathName);
    env->ReleaseStringUTFChars(joldPathName, oldPathName);
    env->ReleaseStringUTFChars(jnewPathName, newPathName);
    return ret;
}

static int mkdir_native(JNIEnv *env, jobject clazz, jstring jpathName) {
    if(systemmixService == NULL || jpathName == NULL){
        throw_NullPointerException(env, "systemmix service has not start or the filepath is null!");
    }
    const char *pathName = env->GetStringUTFChars(jpathName, NULL);
    int ret;
    ret = systemmixService->makeDir(pathName);
    env->ReleaseStringUTFChars(jpathName, pathName);
    return ret;
}

static JNINativeMethod method_table[] = {
    { "nativeInit", "()V", (void*)init_native},
    { "nativeSetProperty", "(Ljava/lang/String;Ljava/lang/String;)I", (void*)setProperty_native },
    { "nativeGetProperty", "(Ljava/lang/String;)Ljava/lang/String;", (void*)getProperty_native },
    { "nativeGetFileData", "([BILjava/lang/String;)I", (void*)getFileData_native },
    { "nativePutFileData", "([BILjava/lang/String;)I", (void*)putFileData_native },
    { "nativeMount", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;)I",
            (void*)mount_native },
    { "nativeUmount", "(Ljava/lang/String;)I", (void*)umount_native },
    { "nativeDelete", "(Ljava/lang/String;)I", (void*)delete_native },
    { "nativeCanWrite", "(Ljava/lang/String;)I", (void*)canWrite_native },
    { "nativeRename", "(Ljava/lang/String;Ljava/lang/String;)I", (void*)rename_native },
    { "nativeMkdir", "(Ljava/lang/String;)I", (void*)mkdir_native},
    { "nativeCanRead", "(Ljava/lang/String;)I", (void*)canRead_native },
};

static int register_android_os_SystemMix(JNIEnv *env){
    return AndroidRuntime::registerNativeMethods(
        env, "com/softwinner/SystemMix",method_table, NELEM(method_table));
}

jint JNI_OnLoad(JavaVM* vm, void* reserved){
    JNIEnv* env = NULL;
    jint result = -1;

    ALOGD("SystemMix JNI_OnLoad()");

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    if (register_android_os_SystemMix(env) < 0) {
        ALOGE("ERROR: SystemMix native registration failed\n");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}

