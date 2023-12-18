#define LOG_TAG "android_pppoe_PppoeNative.cpp"

#include "jni.h"
#include <utils/misc.h>
#include <android_runtime/AndroidRuntime.h>
#include <utils/Log.h>
#include <cutils/properties.h>

#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define LOGD ALOGD
#define LOGE ALOGE
#define LOGV ALOGV

namespace android {

static jint startPppoeNative(JNIEnv* env, jobject clazz,jstring ifname)
{
    int attempt;
    int err = 0;
    char prop_name[PROPERTY_KEY_MAX];
    char prop_value[PROPERTY_VALUE_MAX];
    char prop_svcname[PROPERTY_KEY_MAX];
    char prop_svcvalue[PROPERTY_VALUE_MAX];
    const char *nameStr;
	if (ifname == NULL) {
        LOGD("ifname is null");
		return err;
	}
    nameStr = env->GetStringUTFChars(ifname, NULL);
    snprintf(prop_svcname,sizeof(prop_svcname),"init.svc.start_pppoe");
    property_get(prop_svcname,prop_svcvalue,""); 
    if (!strcmp(prop_svcvalue,"running")) {
        LOGD("already have a pppoe process ,exit");
        err = -1;
        goto end;
    } 
    snprintf(prop_name, sizeof(prop_name), "net.%s-pppoe.status", nameStr);
    property_set(prop_name, "starting");

    if (property_set("ctl.start", "start_pppoe") < 0) {
        LOGE("Failed to start pppoe service with interface %s!", nameStr);
        err = -1;
        goto end;
    }
    for (attempt = 3000; attempt > 0;  attempt--) {
        property_get(prop_name, prop_value, "");
        if (!strcmp(prop_value, "started")) {
            break;
        }
        if (!strcmp(prop_value, "stopped")) { //在pppoe进程退出时，我们会设置该状态。
            attempt = 0;
            err = -1;
            break;
        }
        usleep(100000);  // 100 ms retry delay
    }
    if (attempt == 0) {
        LOGE("%s: Timeout waiting for pppoe-start", __FUNCTION__);
        err = -1;
        goto end;
    }
    err = 0;
end:
    env->ReleaseStringUTFChars(ifname, nameStr);
    return err;
}

static jint stopPppoeNative(JNIEnv* env, jobject clazz,jstring ifname)
{
    int attempt;
    int err = 0;
    char prop_name[PROPERTY_KEY_MAX];
    char prop_value[PROPERTY_VALUE_MAX];
    const char *nameStr;
	if (ifname == NULL) {
        LOGD("ifname is null");
		return err;
	}
    nameStr = env->GetStringUTFChars(ifname, NULL);

    snprintf(prop_name, sizeof(prop_name), "net.%s-pppoe.status", nameStr);
    if (property_get(prop_name, prop_value, "")) {
        if (!strcmp(prop_value, "stopped") || !strcmp(prop_value,"")) {
            err = 0;
            goto end;
        }
    }
    property_set(prop_name, "stopping");
    if (property_set("ctl.start", "stop_pppoe") < 0) {
        LOGE("Failed to stop pppoe with inferface %s", nameStr);
        err = -1;
        goto end;
    }

    for (attempt = 100; attempt > 0;  attempt--) {
        property_get(prop_name, prop_value, "");
        if (!strcmp(prop_value, "stopped") || !strcmp(prop_value, "")) {
            property_set(prop_name, "stopped");
            err = 0;
            goto end;
        }
        usleep(100000);  // 100 ms retry delay
    }
    err = -1;
end:
    env->ReleaseStringUTFChars(ifname, nameStr);
    return err;
}
    
/*---------------------------------------------------------------------------*/

/*
 * JNI registration.
 */
static JNINativeMethod gPppoeMethods[] = {
    /* name,                    method descriptor,                              funcPtr */
    { "startPppoeNative",       "(Ljava/lang/String;)I",                                          (void *)startPppoeNative },
    { "stopPppoeNative",        "(Ljava/lang/String;)I",                                          (void *)stopPppoeNative },
};

int register_android_pppoe_PppoeNative(JNIEnv* env)
{
    return AndroidRuntime::registerNativeMethods(env, "com/android/server/pppoe/PppoeNetworkFactory", gPppoeMethods, NELEM(gPppoeMethods) );
}

/* User to register native functions */
extern "C"
jint Java_com_android_server_pppoe_PppoeNetworkFactory_registerNatives(JNIEnv* env, jclass clazz) {
    return AndroidRuntime::registerNativeMethods(env, "com/android/server/pppoe/PppoeNetworkFactory", gPppoeMethods, NELEM(gPppoeMethods) );
}
	
}


