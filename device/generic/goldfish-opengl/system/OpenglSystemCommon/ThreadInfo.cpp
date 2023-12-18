/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "ThreadInfo.h"

#ifdef HOST_BUILD
#include "android/base/threads/AndroidThread.h"
#else
#include "cutils/threads.h"
#endif

#include <pthread.h>

#if defined(HOST_BUILD) || defined(GFXSTREAM)

static thread_local EGLThreadInfo sEglThreadInfoThreadLocal;

EGLThreadInfo *goldfish_get_egl_tls()
{
    return &sEglThreadInfoThreadLocal;
}

EGLThreadInfo* getEGLThreadInfo() {
    return goldfish_get_egl_tls();
}

int32_t getCurrentThreadId() {
#ifdef HOST_BUILD
    return (int32_t)android::base::guest::getCurrentThreadId();
#else
    return (int32_t)gettid();
#endif
}

void setTlsDestructor(tlsDtorCallback func) {
    getEGLThreadInfo()->dtor = func;
}

#else // GFXSTREAM

#ifdef __BIONIC__
#include <bionic/tls.h>
#endif

#ifdef __ANDROID__
// Are we missing an actual set of TLS defs?
#ifdef GOLDFISH_OPENGL_NO_PLATFORM_BIONIC_INCLUDES
#include <bionic_tls.h>
#endif
#endif

static bool sDefaultTlsDestructorCallback(__attribute__((__unused__)) void* ptr) {
  return true;
}
static bool (*sTlsDestructorCallback)(void*) = sDefaultTlsDestructorCallback;

static void tlsDestruct(void *ptr)
{
    sTlsDestructorCallback(ptr);
    if (ptr
#ifdef __ANDROID__
         && ((void **)__get_tls())[TLS_SLOT_OPENGL]
#endif
        ) {
        EGLThreadInfo *ti = (EGLThreadInfo *)ptr;
        delete ti;
#ifdef __ANDROID__
        ((void **)__get_tls())[TLS_SLOT_OPENGL] = NULL;
#endif
    }
}

void setTlsDestructor(tlsDtorCallback func) {
    sTlsDestructorCallback = func;
}

static pthread_key_t s_tls;

static void init_key()
{
    pthread_key_create(&s_tls, tlsDestruct);
    pthread_setspecific(s_tls, new EGLThreadInfo);
}

EGLThreadInfo *goldfish_get_egl_tls()
{
   static pthread_once_t once = PTHREAD_ONCE_INIT;
   pthread_once(&once, init_key);

   return (EGLThreadInfo *) pthread_getspecific(s_tls);
}

EGLThreadInfo* getEGLThreadInfo() {
#ifdef __ANDROID__
    EGLThreadInfo *tInfo =
        (EGLThreadInfo *)(((uintptr_t *)__get_tls())[TLS_SLOT_OPENGL]);
    if (!tInfo) {
        tInfo = goldfish_get_egl_tls();
        ((uintptr_t *)__get_tls())[TLS_SLOT_OPENGL] = (uintptr_t)tInfo;
    }
    return tInfo;
#else
    return goldfish_get_egl_tls();
#endif
}

int32_t getCurrentThreadId() {
    return (int32_t)gettid();
}

#endif // !GFXSTREAM
