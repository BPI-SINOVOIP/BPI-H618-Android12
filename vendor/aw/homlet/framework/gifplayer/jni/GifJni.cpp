//
// Created by huangweibin on 2021/4/26.
//

#include <jni.h>
#include "GifPlayer.h"
#include "log.h"

const char* CLASS_NAME = "com/softwinner/GifPlayer";

JNIEXPORT jlong JNICALL
jni_create(JNIEnv *env, jobject instance) {
    GifPlayer *gifPlayer = new GifPlayer();
    //LOGE("c++ GifPlayer 对象地址:%p",static_cast<void *>(gifPlayer));
    jlong gifPlayerPtr = (jlong) gifPlayer;
    return gifPlayerPtr;
}

JNIEXPORT jboolean JNICALL
jni_load(JNIEnv *env, jobject instance, jlong gifPlayerPtr,
          jstring gifPath) {
    const char *filePath = env->GetStringUTFChars(gifPath, JNI_FALSE);
    GifPlayer *gifPlayer = (GifPlayer *) gifPlayerPtr;
    jboolean res;
    if (gifPlayer) {
        res = gifPlayer->load(env, filePath);
    } else {
        res = JNI_FALSE;
    }
    env->ReleaseStringUTFChars(gifPath, filePath);
    return res;
}

JNIEXPORT void JNICALL
jni_play(JNIEnv *env, jobject instance, jlong gifPlayerPtr, jboolean loop, jobject bitmap,
          jobject runnable) {
    GifPlayer *gifPlayer = (GifPlayer *) gifPlayerPtr;
    if (gifPlayer) {
        gifPlayer->play(env, loop, bitmap, runnable);
    }
}

JNIEXPORT void JNICALL
jni_pause(JNIEnv *env, jobject instance, jlong gifPlayerPtr) {
    GifPlayer *gifPlayer = (GifPlayer *) gifPlayerPtr;
    if (gifPlayer) {
        gifPlayer->pause();
    }
}

JNIEXPORT void JNICALL
jni_resume(JNIEnv *env, jobject instance, jlong gifPlayerPtr) {
    GifPlayer *gifPlayer = (GifPlayer *) gifPlayerPtr;
    if (gifPlayer) {
        gifPlayer->resume();
    }
}

JNIEXPORT jint JNICALL
jni_getWidth(JNIEnv *env, jobject instance, jlong gifPlayerPtr) {
    GifPlayer *gifPlayer = (GifPlayer *) gifPlayerPtr;
    if (gifPlayer) {
        return gifPlayer->getWidth();
    } else {
        return 0;
    }
}

JNIEXPORT jint JNICALL
jni_getHeight(JNIEnv *env, jobject instance, jlong gifPlayerPtr) {
    GifPlayer *gifPlayer = (GifPlayer *) gifPlayerPtr;
    if (gifPlayer) {
        return gifPlayer->getHeight();
    } else {
        return 0;
    }
}

JNIEXPORT void JNICALL
jni_stop(JNIEnv *env, jobject instance, jlong gifPlayerPtr) {
    GifPlayer *gifPlayer = (GifPlayer *) gifPlayerPtr;
    if (gifPlayer) {
        gifPlayer->stop();
    }
}

JNIEXPORT void JNICALL
jni_release(JNIEnv *env, jobject instance, jlong gifPlayerPtr) {
    GifPlayer *gifPlayer = (GifPlayer *) gifPlayerPtr;
    if (gifPlayer) {
        delete gifPlayer;
        gifPlayer = NULL;
    }
}

JNINativeMethod method[] = {
        {"native_create",     "()J",                                                        (void *) jni_create},
        {"native_load",       "(JLjava/lang/String;)Z",                                     (void *) jni_load},
        {"native_play",      "(JZLandroid/graphics/Bitmap;Ljava/lang/Runnable;)V",          (void *) jni_play},
        {"native_pause",      "(J)V",                                                       (void *) jni_pause},
        {"native_resume",     "(J)V",                                                       (void *) jni_resume},
        {"native_getWidth",  "(J)I",                                                        (void *) jni_getWidth},
        {"native_getHeight", "(J)I",                                                        (void *) jni_getHeight},
        {"native_stop",    "(J)V",                                                          (void *) jni_stop},
        {"native_release",    "(J)V",                                                       (void *) jni_release},
};

jint registerNativeMethod(JNIEnv *env) {
    jclass cl = env->FindClass(CLASS_NAME);
    if ((env->RegisterNatives(cl, method, sizeof(method) / sizeof(method[0]))) < 0) {
        return -1;
    }
    return 0;
}

jint unRegisterNativeMethod(JNIEnv *env) {
    jclass cl = env->FindClass(CLASS_NAME);
    env->UnregisterNatives(cl);
    return 0;
}

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) == JNI_OK) {
        registerNativeMethod(env);
        return JNI_VERSION_1_6;
    } else if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) == JNI_OK) {
        registerNativeMethod(env);
        return JNI_VERSION_1_4;
    }
    return JNI_ERR;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) == JNI_OK) {
        unRegisterNativeMethod(env);
    } else if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) == JNI_OK) {
        unRegisterNativeMethod(env);
    }
}

