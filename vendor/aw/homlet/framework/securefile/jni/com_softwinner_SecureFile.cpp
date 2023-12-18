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
#define LOG_TAG "SecureFile-JNI"
#include "utils/Log.h"

//#include <media/SecureFileInterface.h>
#include <stdio.h>
#include <assert.h>
//#include <limits.h>
//#include <unistd.h>
//#include <fcntl.h>
//#include <utils/threads.h>
#include "jni.h"
#include "JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"
#include "utils/Errors.h"  // for status_t
//#include "utils/KeyedVector.h"
#include "utils/String8.h"
#include "android_util_Binder.h"
//#include <binder/Parcel.h>
#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include "ISecureFileService.h"
// ----------------------------------------------------------------------------

using namespace android;

// ----------------------------------------------------------------------------

struct fields_t {
    jfieldID    fieldIDSecureFileService;
    jfieldID    surface;
    jmethodID   post_event;
};
static fields_t fields;

static Mutex sLock;


static sp<ISecureFileService> getSecureFileService(JNIEnv* env, jobject thiz)
{
    Mutex::Autolock l(sLock);
    ISecureFileService* const p =
        (ISecureFileService*)env->GetLongField(thiz, fields.fieldIDSecureFileService);
    return sp<ISecureFileService>(p);
}

static sp<ISecureFileService> setSecureFileService(JNIEnv* env, jobject thiz,
    const sp<ISecureFileService>& secureFileService)
{
    Mutex::Autolock l(sLock);
    sp<ISecureFileService> old =
        (ISecureFileService*)env->GetLongField(thiz, fields.fieldIDSecureFileService);
    if (secureFileService.get()) {
        secureFileService->incStrong(thiz);
    }

    if (old != 0) {
        old->decStrong(thiz);
    }

    long h = (long)secureFileService.get();
    env->SetLongField(thiz, fields.fieldIDSecureFileService, h);

    return old;
}

static jboolean
com_softwinner_SecureFile_native_createFile(JNIEnv *env, jobject thiz, jstring path)
{
    ALOGV("native_createFile");
    sp<ISecureFileService> sfs = getSecureFileService(env,thiz);
    if(sfs == NULL){
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }
    const char *cpath = env->GetStringUTFChars(path, NULL);
    if(cpath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return false;
    }
    jboolean ret = sfs->createFile(cpath);
    env->ReleaseStringUTFChars(path, cpath);

    return ret;
}

static jboolean
com_softwinner_SecureFile_native_delete(JNIEnv * env,jobject thiz,jstring path){
    ALOGV("native_delete");

    sp<ISecureFileService> sfs = getSecureFileService(env, thiz);
    if (sfs == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    const char *cpath = env->GetStringUTFChars(path, NULL);
    if(cpath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return false;
    }

    jboolean ret = sfs->deleteFile(cpath);

    env->ReleaseStringUTFChars(path, cpath);

    return ret;
}

static jboolean
com_softwinner_SecureFile_native_exists(JNIEnv * env,jobject thiz,jstring path){
    ALOGV("native_exists");

    sp<ISecureFileService> sfs = getSecureFileService(env, thiz);
    if (sfs == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    const char *cpath = env->GetStringUTFChars(path, NULL);
    if(cpath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return false;
    }

    jboolean ret = sfs->exists(cpath);

    env->ReleaseStringUTFChars(path, cpath);

    return ret;
}

static jlong
com_softwinner_SecureFile_native_getTotalSpace(JNIEnv * env,jobject thiz,jstring path){
    ALOGV("native_getTotalSpace");

    sp<ISecureFileService> sfs = getSecureFileService(env, thiz);
    if (sfs == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    const char *cpath = env->GetStringUTFChars(path, NULL);
    if(cpath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return false;
    }

    jlong ret = sfs->getTotalSpace(cpath);

    env->ReleaseStringUTFChars(path, cpath);

    return ret;
}

static jlong
com_softwinner_SecureFile_native_getUsableSpace(JNIEnv * env,jobject thiz,jstring path){
    ALOGV("native_getUsableSpace");

    sp<ISecureFileService> sfs = getSecureFileService(env, thiz);
    if (sfs == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    const char *cpath = env->GetStringUTFChars(path, NULL);
    if(cpath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return false;
    }

    jlong ret = sfs->getUsableSpace(cpath);

    env->ReleaseStringUTFChars(path, cpath);

    return ret;
}

static jboolean
com_softwinner_SecureFile_native_isDirectory(JNIEnv *env, jobject thiz, jstring path)
{
    ALOGV("native_isDirectory");

    sp<ISecureFileService> sfs = getSecureFileService(env, thiz);
    if (sfs == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    const char *cpath = env->GetStringUTFChars(path, NULL);
    if(cpath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return false;
    }

    jboolean ret = sfs->isDirectory(cpath);

    env->ReleaseStringUTFChars(path, cpath);

    return ret;
}

static jboolean
com_softwinner_SecureFile_native_isFile(JNIEnv *env, jobject thiz, jstring path)
{
    ALOGV("native_isFile");

    sp<ISecureFileService> sfs = getSecureFileService(env, thiz);
    if (sfs == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    const char *cpath = env->GetStringUTFChars(path, NULL);
    if(cpath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return false;
    }

    jboolean ret = sfs->isFile(cpath);

    env->ReleaseStringUTFChars(path, cpath);

    return ret;
}

static jlong
com_softwinner_SecureFile_native_length(JNIEnv *env, jobject thiz, jstring path)
{
    ALOGV("native_length");

    sp<ISecureFileService> sfs = getSecureFileService(env, thiz);
    if (sfs == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    const char *cpath = env->GetStringUTFChars(path, NULL);
    if(cpath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return false;
    }

    jlong ret = sfs->length(cpath);

    env->ReleaseStringUTFChars(path, cpath);

    return ret;
}

static jobjectArray
com_softwinner_SecureFile_native_list(JNIEnv *env, jobject thiz, jstring path)
{
    ALOGV("native_list");
    jobjectArray jsubList = NULL;
    SecureItemName *csubList = NULL;

    sp<ISecureFileService> sfs = getSecureFileService(env, thiz);
    if (sfs == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return NULL;
    }

    const char *cpath = env->GetStringUTFChars(path, NULL);
    if(cpath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return NULL;
    }
    int count = sfs->getCount(cpath);
    if(count <= 0)
    {
        goto error;
    }

    jsubList = env->NewObjectArray(count ,env->FindClass("java/lang/String"), NULL);
    if(jsubList == NULL){
        ALOGE("Fail in creating array");
        goto error;
    }
    csubList = new SecureItemName[count];
    count = sfs->list(cpath, csubList, count);
    for(int i = 0; i < count; i++){
        if(env->ExceptionCheck()){
            goto error;
        }
        env->SetObjectArrayElement(jsubList, i, env->NewStringUTF(csubList[i]));
        if(env->ExceptionCheck()){
            goto error;
        }
    }
    delete [] csubList;
    env->ReleaseStringUTFChars(path, cpath);
    return jsubList;
error:
    if(jsubList != NULL)
        env->DeleteLocalRef(jsubList);
    if(csubList != NULL)
        delete [] csubList;
    env->ReleaseStringUTFChars(path, cpath);
    return NULL;
}

static jboolean
com_softwinner_SecureFile_native_mkdir(JNIEnv *env, jobject thiz, jstring path)
{
    ALOGV("native_mkdir");

    sp<ISecureFileService> sfs = getSecureFileService(env, thiz);
    if (sfs == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    const char *cpath = env->GetStringUTFChars(path, NULL);
    if(cpath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return false;
    }

    jboolean ret = sfs->mkDir(cpath);

    env->ReleaseStringUTFChars(path, cpath);

    return ret;
}

static jlong
com_softwinner_SecureFile_native_lastModified(JNIEnv *env, jobject thiz, jstring path)
{
    ALOGV("native_lastModified");

    sp<ISecureFileService> sfs = getSecureFileService(env, thiz);
    if (sfs == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    const char *cpath = env->GetStringUTFChars(path, NULL);
    if(cpath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return false;
    }

    jlong ret = static_cast<jlong>(sfs->lastModified(cpath)) * 1000L;

    env->ReleaseStringUTFChars(path, cpath);

    return ret;
}

static jboolean
com_softwinner_SecureFile_native_renameTo(JNIEnv *env, jobject thiz,
        jstring oldPath, jstring newPath)
{
    ALOGV("native_renameTo");

    jboolean ret = false;
    sp<ISecureFileService> sfs = getSecureFileService(env, thiz);
    if (sfs == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    const char *cNewPath = env->GetStringUTFChars(newPath, NULL);
    const char *cOldPath = env->GetStringUTFChars(oldPath, NULL);
    if(cNewPath == NULL || cOldPath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        goto error;
    }

    ret = sfs->renameTo(cOldPath, cNewPath);

    env->ReleaseStringUTFChars(newPath, cNewPath);
    env->ReleaseStringUTFChars(oldPath, cOldPath);
    return ret;

error:
    if(cNewPath != NULL)
        env->ReleaseStringUTFChars(newPath, cNewPath);
    if(cOldPath != NULL)
        env->ReleaseStringUTFChars(oldPath, cOldPath);
    return false;
}

static jboolean
com_softwinner_SecureFile_native_setLastModified(JNIEnv *env, jobject thiz,
        jstring path, jlong time)
{
    ALOGV("native_setLastModified");

    sp<ISecureFileService> sfs = getSecureFileService(env, thiz);
    if (sfs == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    const char *cpath = env->GetStringUTFChars(path, NULL);
    if(cpath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return false;
    }

    jboolean ret = sfs->setLastModified(cpath,static_cast<unsigned long>(time/1000));

    env->ReleaseStringUTFChars(path, cpath);

    return ret;
}

static jboolean
com_softwinner_SecureFile_native_writeToFile(JNIEnv * env,jobject thiz,
        jstring srcPath, jstring desPath, jboolean append)
{
    ALOGV("native_writeToFile");

    jboolean ret = false;
    sp<ISecureFileService> sfs = getSecureFileService(env, thiz);
    if (sfs == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    const char *csrcPath = env->GetStringUTFChars(srcPath, NULL);
    const char *cdesPath = env->GetStringUTFChars(desPath, NULL);
    if(csrcPath == NULL || cdesPath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        goto error;
    }

    ret = sfs->writeToFile(csrcPath, cdesPath, append);
    env->ReleaseStringUTFChars(srcPath, csrcPath);
    env->ReleaseStringUTFChars(desPath, cdesPath);
    return ret;
error:
    if(csrcPath != NULL)
        env->ReleaseStringUTFChars(srcPath, csrcPath);
    if(cdesPath != NULL)
        env->ReleaseStringUTFChars(desPath, cdesPath);
    return false;
}

static jboolean
com_softwinner_SecureFile_native_writeInData(JNIEnv * env, jobject thiz,
        jbyteArray srcData, jint count, jstring desPath, jboolean append)
{
    ALOGV("native_writeInData");
    jboolean ret = false;
    sp<ISecureFileService> sfs = getSecureFileService(env, thiz);
    if (sfs == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    const char *cdesPath = env->GetStringUTFChars(desPath, NULL);
    if(cdesPath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return false;
    }
    jbyte *data = env->GetByteArrayElements(srcData, NULL);
    if(data == NULL){
        ALOGE("Fail in converting jbyteArray to jbyte * .");
        goto error;
    }
    ret = sfs->writeInData(data, count, cdesPath, append);
    env->ReleaseStringUTFChars(desPath, cdesPath);
    env->ReleaseByteArrayElements(srcData, data, 0);
    return ret;
error:
    env->ReleaseStringUTFChars(desPath, cdesPath);
    return false;
}

static jboolean
com_softwinner_SecureFile_native_read(JNIEnv *env, jobject thiz,
        jbyteArray desData, jint count, jstring srcPath){
    ALOGV("native_read");
    jboolean ret = false;
    sp<ISecureFileService> sfs = getSecureFileService(env, thiz);
    if (sfs == NULL ) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return false;
    }

    const char *csrcPath = env->GetStringUTFChars(srcPath, NULL);
    if(csrcPath == NULL){
        ALOGE("Fail in converting jstring to cstring.");
        return false;
    }
    jbyte *data = env->GetByteArrayElements(desData, NULL);
    if(data == NULL){
        ALOGE("Fail in converting jbyteArray to jbyte * .");
        goto error;
    }
    ret = sfs->read(data, count, csrcPath);
    env->ReleaseStringUTFChars(srcPath, csrcPath);
    env->ReleaseByteArrayElements(desData, data, 0);
    return ret;
error:
    env->ReleaseStringUTFChars(srcPath, csrcPath);
    return false;

}

// ----------------------------------------------------------------------------

// This function gets some field IDs, which in turn causes class initialization.
// It is called from a static block in SecureFile, which won't run until the
// first time an instance of this class is used.
static void
com_softwinner_SecureFile_native_init(JNIEnv *env)
{
    jclass clazz;

    ALOGD("native_init");

    clazz = env->FindClass("com/softwinner/SecureFile");
    if (clazz == NULL) {
        jniThrowException(env, "java/lang/RuntimeException",
                "Can't find com/softwinner/SecureFile");
        return;
    }

    fields.fieldIDSecureFileService = env->GetFieldID(clazz, "mSecureFileService", "I");
    if (fields.fieldIDSecureFileService == NULL) {
        jniThrowException(env, "java/lang/RuntimeException",
                "Can't find SecureFileService.mSecureFileService");
        return;
    }
}

static void
com_softwinner_SecureFile_native_setup(JNIEnv *env, jobject thiz, jobject weak_this)
{
    ALOGD("native_setup");

    sp<IServiceManager> sm = defaultServiceManager();
    sp<IBinder> binder;
    do {
        binder = sm->getService(String16("softwinner.securefile"));
        if (binder != 0) {
            break;
         }
         ALOGW("softwinner securefile service not published, waiting...");
         usleep(500000); // 0.5 s
    } while(true);

    sp<ISecureFileService> secureFileService = interface_cast<ISecureFileService>(binder);

    // Stow ISecureFileService in an opaque field in the Java object.
    setSecureFileService(env, thiz, secureFileService);
}

static void
com_softwinner_SecureFile_native_finalize(JNIEnv *env, jobject thiz)
{
    ALOGD("native_finalize");
    setSecureFileService(env, thiz, 0);
}

// ----------------------------------------------------------------------------

static JNINativeMethod gMethods[] = {
    {"nativeInit",         "()V",
        (void *)com_softwinner_SecureFile_native_init},
    {"nativeSetup",        "(Ljava/lang/Object;)V",
        (void *)com_softwinner_SecureFile_native_setup},
    {"nativeFinalize",     "()V",
        (void *)com_softwinner_SecureFile_native_finalize},
    {"nativeCreateFile",    "(Ljava/lang/String;)Z",
        (void *)com_softwinner_SecureFile_native_createFile},
    {"nativeDelete",        "(Ljava/lang/String;)Z",
        (void *)com_softwinner_SecureFile_native_delete},
    {"nativeExists",        "(Ljava/lang/String;)Z",
        (void *)com_softwinner_SecureFile_native_exists},
    {"nativeGetTotalSpace",    "(Ljava/lang/String;)J",
        (void *)com_softwinner_SecureFile_native_getTotalSpace},
    {"nativeGetUsableSpace",    "(Ljava/lang/String;)J",
        (void *)com_softwinner_SecureFile_native_getUsableSpace},
    {"nativeIsDirectory",  "(Ljava/lang/String;)Z",
        (void *)com_softwinner_SecureFile_native_isDirectory},
    {"nativeIsFile",        "(Ljava/lang/String;)Z",
        (void *)com_softwinner_SecureFile_native_isFile},
    {"nativeLength",        "(Ljava/lang/String;)J",
        (void *)com_softwinner_SecureFile_native_length},
    {"nativeList",            "(Ljava/lang/String;)[Ljava/lang/String;",
        (void *)com_softwinner_SecureFile_native_list},
    {"nativeMkdir",        "(Ljava/lang/String;)Z",
        (void *)com_softwinner_SecureFile_native_mkdir},
    {"nativeLastModified",    "(Ljava/lang/String;)J",
        (void *)com_softwinner_SecureFile_native_lastModified},
    {"nativeRenameTo",        "(Ljava/lang/String;Ljava/lang/String;)Z",
        (void *)com_softwinner_SecureFile_native_renameTo},
    {"nativeSetLastModified",    "(Ljava/lang/String;J)Z",
        (void *)com_softwinner_SecureFile_native_setLastModified},
    {"nativeWriteToFile",    "(Ljava/lang/String;Ljava/lang/String;Z)Z",
        (void *)com_softwinner_SecureFile_native_writeToFile},
    {"nativeWriteInData",    "([BILjava/lang/String;Z)Z",
        (void *)com_softwinner_SecureFile_native_writeInData},
    {"nativeRead",            "([BILjava/lang/String;)Z",
        (void *)com_softwinner_SecureFile_native_read},

};

static const char* const kClassPathName = "com/softwinner/SecureFile";

// This function only registers the native methods
static int register_com_softwinner_SecureFile(JNIEnv *env)
{
    return AndroidRuntime::registerNativeMethods(env,
                kClassPathName, gMethods, NELEM(gMethods));
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;

    ALOGD("JNI_OnLoad()");

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    if (register_com_softwinner_SecureFile(env) < 0) {
        ALOGE("ERROR: SecureFile native registration failed\n");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}

// KTHXBYE
