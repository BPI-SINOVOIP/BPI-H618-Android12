#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <jni.h>
/* Header for class com_allwinnertech_dragonsn_jni_ReadPrivateJNI */
#include "secure_storage/api.h"

#include <android/log.h>
#define LOG_TAG "dragonenter_so"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

typedef enum __bool { false = 0, true = 1, } bool;

#define VALUE_MAXSIZE  (128)


/*
 * Class:     com_allwinnertech_dragonsn_jni_ReadPrivateJNI
 * Method:    native_init
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_allwinnertech_dragonsn_jni_ReadPrivateJNI_nativeInit
  (JNIEnv *env, jclass thiz){
	LOGD("jni_init");
	return true;
}

/*
 * Class:     com_allwinnertech_dragonsn_jni_ReadPrivateJNI
 * Method:    native_get_parameter
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_allwinnertech_dragonsn_jni_ReadPrivateJNI_nativeGetParameter
  (JNIEnv *env, jclass thiz, jstring name){
    char* value = (char*) malloc(VALUE_MAXSIZE);
    if (value == NULL)
        return NULL;
    memset(value, 0,sizeof(char)*VALUE_MAXSIZE);
	const char *key = (*env)->GetStringUTFChars(env, name, NULL);
    int value_len = 0;
    if (0==private_data_read(key, value, VALUE_MAXSIZE, &value_len)) {
        (*env)->ReleaseStringUTFChars(env, name, key);
        jstring rtstr = (*env)->NewStringUTF(env, value);
        free(value);
        value = NULL;
        return rtstr;
    } else {
	    LOGD("read key: %s fail",key);
        (*env)->ReleaseStringUTFChars(env, name, key);
        free((void *)value);
        value = NULL;
        return NULL;
    }
}

/*
 * Class:     com_allwinnertech_dragonsn_jni_ReadPrivateJNI
 * Method:    native_set_parameter
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_allwinnertech_dragonsn_jni_ReadPrivateJNI_nativeSetParameter
  (JNIEnv *env, jclass thiz, jstring jKey, jstring jValue){
	const char *key = (*env)->GetStringUTFChars(env, jKey, NULL);
	const char *value = (*env)->GetStringUTFChars(env, jValue, NULL);
    if (  (key == NULL && strlen(key) < 1)
        || (value == NULL && strlen(value) < 1)) {
        LOGD("nativeSetParameter : error parameter");
        return false;
    }
    int ret = private_data_write(key, value, strlen(value));
    (*env)->ReleaseStringUTFChars(env, jKey, key);
    (*env)->ReleaseStringUTFChars(env, jValue, value);
    if (ret == 0) {
        return true;
    } else {
        return false;
    }
}

/*
 * Class:     com_allwinnertech_dragonsn_jni_ReadPrivateJNI
 * Method:    natice_release
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_allwinnertech_dragonsn_jni_ReadPrivateJNI_nativeRelease
  (JNIEnv *env, jclass thiz){
	LOGD("release");
}
