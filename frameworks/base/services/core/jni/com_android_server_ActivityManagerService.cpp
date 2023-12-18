/*
 * Copyright (C) 2008 The Android Open Source Project
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

#define TAG "ActivityManagerService-jni"

#include <nativehelper/JNIHelp.h>
#include <jni.h>
#include <utils/Log.h>
#include <utils/misc.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/ioctl.h>

#include "com_android_server_ActivityManagerService.h"

namespace android {
    int inited = 0;

    jint android_server_ActivityManagerService_checkFileName(JNIEnv* env,jobject clazz,jstring jname) {
        const char *name= env->GetStringUTFChars(jname, NULL);
        int ret;
        // use your string
        ret = checkfile(name);
        env->ReleaseStringUTFChars(jname, name);
        return ret;
    }


    static JNINativeMethod sMethods[] = {
        /* name,signature,funcPtr */
        {"checkFileName","(Ljava/lang/String;)I",(void*)android_server_ActivityManagerService_checkFileName},
    };

    int register_dp_android_server_ActivityManagerService(JNIEnv* env) {
        return jniRegisterNativeMethods(env, "com/android/server/wm/Task", sMethods, NELEM(sMethods));
    }

}

