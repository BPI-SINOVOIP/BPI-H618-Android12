/*
**
** Copyright 2007, The Android Open Source Project
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

#define LOG_NDEBUG 0
#define LOG_TAG "ISOMountManager-JNI"
#include "utils/Log.h"

#include <stdio.h>
#include <assert.h>
#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"
#include "utils/Errors.h"  // for status_t
#include "utils/String8.h"
#include "android_util_Binder.h"
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include "IISOMountManagerService.h"

using namespace android;

static sp<IISOMountManagerService> mIISOMountManagerService;

/*
 *On success, zero is returned.
 *On error, -1 is returned errno is set appropriately.
 *
 * EACCES 13, "Permission denied".
 * EBUSY  16, "Device or resource busy".
 * EFAULT 14, "One of the arguments points outside the user address space".
 * EINVAL 22, "Source had an invalid superblock".
 * ELOOP  40, "Too many links encountered during pathname resolution".
 * EMFILE 24, "In case no block device is required:) Table of dummy devices is full".
 * ENAMETOOLONG 36, "A pathname was longer than MAXPATHLEN".
 * ENODEV 19, "Filesystemtype not configured in the kernel".
 * ENOENT 2,  "A pathname was empty or had a nonexistent component".
 * ENOMEM 12, "Kernel couldn't  allocate a free page to copy filenames or data into".
 * ENOTBLK 15, "Source is not a block device (and a device was required)".
 * ENOTDIR 20, "Target, or a prefix of source, is not a directory".
 * ENXIO  6,  "The major number of the block device source is out of range".
 * EPERM  1,  "The caller does not have the required privileges".
 */
static jint
com_softwinner_ISOMountManager_mount(JNIEnv *env, jobject , jstring mountPath, jstring isoPath)
{
    ALOGD("com_softwinner_ISOMountManager_mount");
    if(mIISOMountManagerService == NULL){
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }

    const char *ctarget = env->GetStringUTFChars(mountPath, NULL);
    const char *csource = env->GetStringUTFChars(isoPath, NULL);
    if(ctarget == NULL || csource == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return -1;
    }

    jint ret = mIISOMountManagerService->isoMount(ctarget, csource);
    env->ReleaseStringUTFChars(mountPath, ctarget);
    env->ReleaseStringUTFChars(isoPath, csource);
    return ret;
}

/*
 *On success, zero is returned.
 *On error, -1 is returned errno is set appropriately.
 *
 * EAGAIN 11
 * EBUSY  16, "Target could not be unmounted because it is busy".
 * EFAULT 14, "Target points outside the user address space".
 * EINVAL 22, "Ttarget is not a mount point".
 * ENAMETOOLONG 36, "A pathname was longer than MAXPATHLEN".
 * ENOENT 2,  "A pathname was empty or had a nonexistent component".
 * ENOMEM 12, "Kernel could not allocate a free page to copy filenames or data into".
 * EPERM  1,  "The caller does not have the required privileges".
 */
static jint
com_softwinner_ISOMountManager_umount(JNIEnv *env, jobject , jstring mountPath){
    ALOGD("com_softwinner_ISOMountManager_umount");
    if(mIISOMountManagerService == NULL){
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return -1;
    }

    const char *ctarget = env->GetStringUTFChars(mountPath, NULL);
    if(ctarget == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return -1;
    }

    jint ret = mIISOMountManagerService->isoUmount(ctarget);
    env->ReleaseStringUTFChars(mountPath, ctarget);
    return ret;
}

static jboolean
com_softwinner_ISOMountManager_umountAll(JNIEnv *env, jobject )
{
//    ALOGV("com_softwinner_ISOMountManager_umountAll");
    if(mIISOMountManagerService == NULL){
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }
    return mIISOMountManagerService->umountAll();
}

static jobject _composeObjMountInfo(JNIEnv *env, jclass classMountInfo,
        jmethodID methodMountInfo, ISOMountManager_MountInfo *info)
{
    jstring mountPath = env->NewStringUTF(info->mMountPath);
    jstring isoPath = env->NewStringUTF(info->mISOPath);
    jobject objMountInfo = env->NewObject(classMountInfo, methodMountInfo, mountPath, isoPath);
    if(objMountInfo == NULL )
        ALOGE("Fail in creating MountInfo object.");

    env->DeleteLocalRef(mountPath);
    env->DeleteLocalRef(isoPath);
    return objMountInfo;
}

static jobjectArray
com_softwinner_ISOMountManager_getMountInfoList(JNIEnv *env, jobject )
{
//    ALOGV("com_softwinner_ISOMountManager_getMountInfoList");

    jobjectArray jmountList = NULL;
    ISOMountManager_MountInfo *cmountList = NULL;
    jclass classMountInfo;
    jmethodID methodMountInfo;

    if(mIISOMountManagerService == NULL){
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return NULL;
    }

    classMountInfo = env->FindClass("com/softwinner/ISOMountManager$MountInfo");
    if(classMountInfo == NULL ){
        ALOGE("Fail in finding class com/softwinner/ISOMountManager$MountInfo");
        return NULL;
    }

    int count = mIISOMountManagerService->getTotalMountCount();
    if(count <= 0)
        return NULL;

    jmountList = env->NewObjectArray(count, classMountInfo, NULL );
    if(jmountList == NULL){
        ALOGE("Fail in creating MountInfo array.");
        goto error;
    }
    cmountList = new ISOMountManager_MountInfo[count];
    if(cmountList == NULL ){
        ALOGE("Fail in allocating memory.");
        goto error;
    }
    count = mIISOMountManagerService->getMountInfoList(cmountList, count);
    if(count <= 0){
        ALOGE("Fail in getting MountInfo list.");
        goto error;
    }
    methodMountInfo = env->GetMethodID(classMountInfo, "<init>",
                                                  "(Ljava/lang/String;Ljava/lang/String;)V");
    if(methodMountInfo == NULL){
        ALOGE("Fail in getting method \"MountInfo\".");
        goto error;
    }
    for(int i = 0; i < count; i++){
        jobject objMountInfo =
                _composeObjMountInfo(env, classMountInfo, methodMountInfo, cmountList+i);
        env->SetObjectArrayElement(jmountList, i, objMountInfo);
        env->DeleteLocalRef(objMountInfo);
    }

    delete [] cmountList;
    return jmountList;

error:
    if(jmountList != NULL)
        env->DeleteLocalRef(jmountList);
    if(cmountList != NULL)
        delete [] cmountList;
    return NULL;
}

static jstring
com_softwinner_ISOMountManager_getIsoPath(JNIEnv *env, jobject , jstring mountPath)
{
//    ALOGV("com_softwinner_ISOMountManager_umount");
    jstring jisoPath = NULL;

    if(mIISOMountManagerService == NULL){
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return NULL;
    }

    const char *ctarget = env->GetStringUTFChars(mountPath, NULL);
    if(ctarget == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return NULL;
    }

    char *cisoPath = new char[PATH_SIZE_MAX];
    if(cisoPath == NULL){
        ALOGE("Fail in allocating memory.");
        env->ReleaseStringUTFChars(mountPath, ctarget);
        return NULL;
    }
    bool ret = mIISOMountManagerService->getISOPath(ctarget, cisoPath);
    if(ret){
        jisoPath = env->NewStringUTF(cisoPath);
           if(jisoPath == NULL){
               ALOGE("Fail in creating java string with %s.", cisoPath);
           }
    }
       delete [] cisoPath;
       env->ReleaseStringUTFChars(mountPath, ctarget);
       return jisoPath;
}

static jobjectArray
com_softwinner_ISOMountManager_getMountPoints(JNIEnv *env, jobject , jstring isoPath)
{
//    ALOGV("com_softwinner_ISOMountManager_getMountPoints");
    jobjectArray jmountPoints = NULL;
    Path *cmountPoints = NULL;
    int ret;

    if (mIISOMountManagerService == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return NULL;
    }

    const char *cisoPath = env->GetStringUTFChars(isoPath, NULL);
    if(cisoPath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return NULL;
    }

    int count = mIISOMountManagerService->getISOMountCount(cisoPath);
    if(count <= 0)
    {
        goto error;
    }

    jmountPoints = env->NewObjectArray(count ,env->FindClass("java/lang/String"), NULL);
    if(jmountPoints == NULL){
        ALOGE("Fail in creating array");
        goto error;
    }

    cmountPoints = new Path[count];
    ret = mIISOMountManagerService->getMountPoints(cisoPath, cmountPoints, count);
    if(ret <= 0){
        ALOGE("Fail in getMountPoints.");
        goto error;
    }
    for(int i = 0; i < ret; i++){
        env->SetObjectArrayElement(jmountPoints, i, env->NewStringUTF(cmountPoints[i]));
    }

    delete [] cmountPoints;
    env->ReleaseStringUTFChars(isoPath, cisoPath);
    return jmountPoints;
error:
    if(cmountPoints != NULL){
        delete [] cmountPoints;
    }
    env->ReleaseStringUTFChars(isoPath, cisoPath);
    return NULL;
}

static void
com_softwinner_ISOMountManager_init(JNIEnv *)
{
//    ALOGD("com_softwinner_ISOMountManager_init");

    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder;
    do {
        binder = sm->getService(String16("softwinner.isomountmanager"));
        if (binder != 0) {
            break;
        }
           ALOGW("softwinner isomountmanager service not published, waiting...");
           usleep(500000); // 0.5 s
    } while(true);

    mIISOMountManagerService = interface_cast<IISOMountManagerService>(binder);
}


static JNINativeMethod gMethods[] = {
    {"init",                 "()V",
            (void *)com_softwinner_ISOMountManager_init},
    {"mount",                "(Ljava/lang/String;Ljava/lang/String;)I",
            (void *)com_softwinner_ISOMountManager_mount},
    {"umount",                 "(Ljava/lang/String;)I",
            (void *)com_softwinner_ISOMountManager_umount},
    {"umountAll",            "()V",
            (void *)com_softwinner_ISOMountManager_umountAll},
    {"getMountInfoList",    "()[Lcom/softwinner/ISOMountManager$MountInfo;",
            (void *)com_softwinner_ISOMountManager_getMountInfoList},
    {"getIsoPath",            "(Ljava/lang/String;)Ljava/lang/String;",
            (void *)com_softwinner_ISOMountManager_getIsoPath},
    {"getMountPoints",        "(Ljava/lang/String;)[Ljava/lang/String;",
            (void *)com_softwinner_ISOMountManager_getMountPoints},
};

static const char* const kClassPathName = "com/softwinner/ISOMountManager";

// This function only registers the native methods
static int register_com_softwinner_ISOMountManager(JNIEnv *env)
{
    return AndroidRuntime::registerNativeMethods(env,
                kClassPathName, gMethods, NELEM(gMethods));
}

jint JNI_OnLoad(JavaVM* vm, void* )
{
    JNIEnv* env = NULL;
    jint result = -1;

//    ALOGD("JNI_OnLoad()");

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    if (register_com_softwinner_ISOMountManager(env) < 0) {
        ALOGE("ERROR: ISOMountManager native registration failed\n");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}

// KTHXBYE
