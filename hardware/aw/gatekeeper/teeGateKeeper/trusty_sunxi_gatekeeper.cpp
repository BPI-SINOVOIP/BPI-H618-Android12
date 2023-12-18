/*
 * Copyright (C) 2015 The Android Open Source Project
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

#define LOG_NDEBUG 0
#define LOG_TAG "SUNXI_TrustyGateKeeper"
#include <cutils/log.h>

#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <type_traits>
#include <tee_client_api.h>

#include "trusty_sunxi_gatekeeper.h"

//UUID for gatekeeper, it is a demo.
static const uint8_t GatekeeperUUID[16] = {
    0x3b,0xb4,0x33,0x22,0xc6,0xce,0x9a,0x44,
    0x95,0x09,0x46,0x9f,0x50,0x23,0xe4,0x25
};

static pthread_mutex_t gGatekeeperMutex = PTHREAD_MUTEX_INITIALIZER;
static TEEC_Context *gGatekeeperContext = NULL;
static TEEC_Session *gGatekeeperSession = NULL;

enum {
    /*keep in sync with definition in secure os*/
    MSG_GATEKEEPER_MIN = 0x300,
    MSG_GATEKEEPER_ENROLL_PASS_UID,
    MSG_GATEKEEPER_ENROLL,
    MSG_GATEKEEPER_VERIFY_PASS_UID,
    MSG_GATEKEEPER_VERIFY,
    MSG_GATEKEEPER_MAX,
};

int Gatekeeper_Initialize(void)
{
    ALOGV("Gatekeeper_Initialize");

    pthread_mutex_lock(&gGatekeeperMutex);
    if(gGatekeeperContext != NULL) {
        pthread_mutex_unlock(&gGatekeeperMutex);
        return 0;
    }
    gGatekeeperContext = (TEEC_Context *)malloc(sizeof(TEEC_Context));
    gGatekeeperSession = (TEEC_Session *)malloc(sizeof(TEEC_Session));
    assert(gGatekeeperContext != NULL);
    assert(gGatekeeperSession != NULL);
    pthread_mutex_unlock(&gGatekeeperMutex);
    TEEC_Result success = TEEC_InitializeContext(NULL, gGatekeeperContext);
    if (success != TEEC_SUCCESS) {
        ALOGE("initialize context failed");

        return -1;
    }
    success = TEEC_OpenSession(gGatekeeperContext, gGatekeeperSession,
                    (const TEEC_UUID *)&GatekeeperUUID[0], TEEC_LOGIN_PUBLIC,
                    NULL, NULL, NULL);
    if(success != TEEC_SUCCESS) {
        ALOGE("open session failed");
        return -1;
    }

    return (int) success;
}

int Gatekeeper_Terminate(void)
{
    ALOGV("Gatekeeper_Terminate");

    pthread_mutex_lock(&gGatekeeperMutex);
    if(gGatekeeperContext == NULL) {
        pthread_mutex_unlock(&gGatekeeperMutex);
        return 0;
    }

    TEEC_CloseSession(gGatekeeperSession);
    TEEC_FinalizeContext(gGatekeeperContext);

    free(gGatekeeperSession);
    free(gGatekeeperContext);
    gGatekeeperSession = NULL;
    gGatekeeperContext = NULL;
    pthread_mutex_unlock(&gGatekeeperMutex);

    return 0;
}

namespace gatekeeper {

TrustyGateKeeperDevice::TrustyGateKeeperDevice(const hw_module_t *module) {
#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
    static_assert(std::is_standard_layout<TrustyGateKeeperDevice>::value,
                  "TrustyGateKeeperDevice must be standard layout");
    static_assert(offsetof(TrustyGateKeeperDevice, device_) == 0,
                  "device_ must be the first member of TrustyGateKeeperDevice");
    static_assert(offsetof(TrustyGateKeeperDevice, device_.common) == 0,
                  "common must be the first member of gatekeeper_device");
#else
    assert(reinterpret_cast<gatekeeper_device_t *>(this) == &device_);
    assert(reinterpret_cast<hw_device_t *>(this) == &(device_.common));
#endif

    memset(&device_, 0, sizeof(device_));
    device_.common.tag = HARDWARE_DEVICE_TAG;
    device_.common.version = 1;
    device_.common.module = const_cast<hw_module_t *>(module);
    device_.common.close = close_device;

    device_.enroll = enroll;
    device_.verify = verify;
    device_.delete_user = nullptr;
    device_.delete_all_users = nullptr;

    int rc = Gatekeeper_Initialize();
    if (rc < 0) {
        ALOGE("Error initializing trusty session: %d", rc);
    }

    error_ = rc;
}

hw_device_t* TrustyGateKeeperDevice::hw_device() {
    return &device_.common;
}

int TrustyGateKeeperDevice::close_device(hw_device_t* dev) {
    delete reinterpret_cast<TrustyGateKeeperDevice *>(dev);
    return 0;
}

TrustyGateKeeperDevice::~TrustyGateKeeperDevice() {
    Gatekeeper_Terminate();
}

int TrustyGateKeeperDevice::Enroll(uint32_t uid,
    const uint8_t *current_password_handle, uint32_t current_password_handle_length,
    const uint8_t *current_password, uint32_t current_password_length,
    const uint8_t *desired_password, uint32_t desired_password_length,
    uint8_t **enrolled_password_handle, uint32_t *enrolled_password_handle_length)
{
    uint32_t flag = 0;
    int ret = -1;

    if (error_ != 0) {
        return error_;
    }

    if ((enrolled_password_handle == NULL) ||
        (enrolled_password_handle_length == 0)) {
        ALOGE("%s enrolled password is NULL", __func__);

        return -1;
    }

    if (desired_password == NULL || desired_password_length == 0) {
        ALOGE("%s desired password is NULL", __func__);

        return -1;
    }

    if ( (current_password_handle != NULL) && (current_password == NULL) ) {
        ALOGE("%s current password and its handle is not the same type", __func__);

        return -1;
    }

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_NONE,
                                    TEEC_NONE, TEEC_NONE);

    operation.started = 1;

    operation.params[0].value.a = uid;

    TEEC_Result success = TEEC_InvokeCommand(gGatekeeperSession,
                            MSG_GATEKEEPER_ENROLL_PASS_UID, &operation, NULL);
    //ALOGV("success=0x%x\n", success);
    if(success) {
        ALOGE("gatekeeper pass enroll uid fail");

        return -1;
    }

    memset(&operation, 0, sizeof(TEEC_Operation));

    TEEC_SharedMemory desired_ps;
    desired_ps.size = desired_password_length;
    desired_ps.flags = TEEC_MEM_INPUT;
    if(TEEC_AllocateSharedMemory(gGatekeeperContext, &desired_ps) != TEEC_SUCCESS) {
        ALOGE("%s: allocate desired password handle share memory fail", __func__);

        goto __enrolled_err;
    }
    memcpy(desired_ps.buffer, desired_password, desired_password_length);
    flag |= (1<<2);

    TEEC_SharedMemory enrolled_handle;
    enrolled_handle.size = 512;
    enrolled_handle.flags = TEEC_MEM_OUTPUT;
    if(TEEC_AllocateSharedMemory(gGatekeeperContext, &enrolled_handle) != TEEC_SUCCESS) {
        ALOGE("%s: allocate enrolled password handle share memory fail", __func__);

        goto __enrolled_err;
    }
    flag |= (1<<3);

    operation.params[2].memref.parent = &desired_ps;
    operation.params[2].memref.offset = 0;
    operation.params[2].memref.size = 0;

    operation.params[3].memref.parent = &enrolled_handle;
    operation.params[3].memref.offset = 0;
    operation.params[3].memref.size = 0;

    TEEC_SharedMemory ps_handle;
    TEEC_SharedMemory current_pw;
    //ALOGV("%s %d\n", __FILE__, __LINE__);
    if ((current_password_handle != NULL) && (current_password != NULL)) {
        //ALOGV("%s %d\n", __FILE__, __LINE__);
        ps_handle.size = current_password_handle_length;
        ps_handle.flags = TEEC_MEM_INPUT;
        if(TEEC_AllocateSharedMemory(gGatekeeperContext, &ps_handle) != TEEC_SUCCESS) {
            ALOGE("%s: allocate current password handle share memory fail", __func__);

            goto __enrolled_err;
        }
        memcpy(ps_handle.buffer, current_password_handle, current_password_handle_length);
        flag |= (1<<0);

        //ALOGV("%s %d\n", __FILE__, __LINE__);
        current_pw.size = current_password_length;
        current_pw.flags = TEEC_MEM_INPUT;
        if(TEEC_AllocateSharedMemory(gGatekeeperContext, &current_pw) != TEEC_SUCCESS) {
            ALOGE("%s: allocate current password share memory fail", __func__);

            goto __enrolled_err;
        }
        memcpy(current_pw.buffer, current_password, current_password_length);
        flag |= (1<<1);

        operation.params[0].memref.parent = &ps_handle;
        operation.params[0].memref.offset = 0;
        operation.params[0].memref.size = 0;

        operation.params[1].memref.parent = &current_pw;
        operation.params[1].memref.offset = 0;
        operation.params[1].memref.size = 0;
        //ALOGV("%s %d\n", __FILE__, __LINE__);
        operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE,
                TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE);
    } else {
        //ALOGV("%s %d\n", __FILE__, __LINE__);
        operation.params[0].value.a = 0xffff;
        operation.params[1].value.a = 0xffff;

        operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT,
                TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE);
    }
    operation.started = 1;
    success = TEEC_InvokeCommand(gGatekeeperSession, MSG_GATEKEEPER_ENROLL, &operation, NULL);
    //ALOGV("%s %d %d\n", __func__, __LINE__, success);
    //ALOGV("enrolled password size = %d\n", operation.params[3].memref.size);
    if(!success) {
        uint8_t *password_handle = new uint8_t[operation.params[3].memref.size];

        memcpy(password_handle, enrolled_handle.buffer, operation.params[3].memref.size);
        *enrolled_password_handle = password_handle;
        *enrolled_password_handle_length = operation.params[3].memref.size;

        ret = 0;
    } else {
        //ALOGE("success=%d\n", success);
    }

__enrolled_err:
    if (flag & (1<<3))
        TEEC_ReleaseSharedMemory(&enrolled_handle);
    if (flag & (1<<2))
        TEEC_ReleaseSharedMemory(&desired_ps);
    if (flag & (1<<1))
        TEEC_ReleaseSharedMemory(&current_pw);
    if (flag & (1<<0))
        TEEC_ReleaseSharedMemory(&ps_handle);

    //ALOGV("enroll finish\n");
    //ALOGV("%p\n", *enrolled_password_handle);
    return ret;
}

int TrustyGateKeeperDevice::Verify(uint32_t uid, uint64_t challenge,
        const uint8_t *enrolled_password_handle, uint32_t enrolled_password_handle_length,
        const uint8_t *provided_password, uint32_t provided_password_length,
        uint8_t **auth_token, uint32_t *auth_token_length,
        bool *request_reenroll)
{
    int flag = 0;

    if (error_ != 0) {
        return error_;
    }

    if ((enrolled_password_handle == NULL) ||
            (enrolled_password_handle_length == 0)) {
        ALOGE("%s enrolled password is NULL", __func__);

        return -1;
    }

    if (provided_password == NULL || provided_password_length == 0) {
        ALOGE("%s provided password is NULL", __func__);

        return -1;
    }

    if (auth_token == NULL || auth_token_length == NULL) {
        ALOGE("%s auth token is NULL", __func__);

        return -1;
    }

    TEEC_Operation operation;
    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_VALUE_INPUT, TEEC_VALUE_INPUT,
                TEEC_NONE, TEEC_NONE);

    operation.started = 1;

    operation.params[0].value.a = uid;

    operation.params[1].value.a = (challenge>>0 ) & 0xffffffff;
    operation.params[1].value.b = (challenge>>32) & 0xffffffff;

    //ALOGV("a=0x%x, b=0x%x\n", operation.params[1].value.a, operation.params[1].value.b);
    TEEC_Result success = TEEC_InvokeCommand(gGatekeeperSession,
                                MSG_GATEKEEPER_VERIFY_PASS_UID, &operation, NULL);
    if(success) {
        ALOGE("gatekeeper verify: pass enroll uid fail");

        return -1;
    }

    TEEC_SharedMemory ps_handle;
    ps_handle.size = enrolled_password_handle_length;
    ps_handle.flags = TEEC_MEM_INPUT;
    if(TEEC_AllocateSharedMemory(gGatekeeperContext, &ps_handle) != TEEC_SUCCESS) {
        ALOGE("%s: allocate current password handle share memory fail", __func__);

        goto __enrolled_err;
    }
    memcpy(ps_handle.buffer, enrolled_password_handle, enrolled_password_handle_length);
    flag |= (1<<0);

    TEEC_SharedMemory current_pw;
    current_pw.size = provided_password_length;
    current_pw.flags = TEEC_MEM_INPUT;
    if(TEEC_AllocateSharedMemory(gGatekeeperContext, &current_pw) != TEEC_SUCCESS) {
        ALOGE("%s: allocate provide password share memory fail", __func__);

        goto __enrolled_err;
    }
    memcpy(current_pw.buffer, provided_password, provided_password_length);
    flag |= (1<<1);

    TEEC_SharedMemory authtoken;
    authtoken.size = 512;
    authtoken.flags = TEEC_MEM_OUTPUT;
    if(TEEC_AllocateSharedMemory(gGatekeeperContext, &authtoken) != TEEC_SUCCESS) {
        ALOGE("%s: allocate desired password handle share memory fail", __func__);

        goto __enrolled_err;
    }
    flag |= (1<<2);

    memset(&operation, 0, sizeof(TEEC_Operation));
    operation.paramTypes = TEEC_PARAM_TYPES(TEEC_MEMREF_WHOLE, TEEC_MEMREF_WHOLE,
                TEEC_MEMREF_WHOLE, TEEC_VALUE_OUTPUT);
    operation.started = 1;

    operation.params[0].memref.parent = &ps_handle;
    operation.params[0].memref.offset = 0;
    operation.params[0].memref.size = 0;

    operation.params[1].memref.parent = &current_pw;
    operation.params[1].memref.offset = 0;
    operation.params[1].memref.size = 0;

    operation.params[2].memref.parent = &authtoken;
    operation.params[2].memref.offset = 0;
    operation.params[2].memref.size = 0;

    success = TEEC_InvokeCommand(gGatekeeperSession, MSG_GATEKEEPER_VERIFY, &operation, NULL);
    //ALOGV("%s %d %d\n", __func__, __LINE__, success);
    //ALOGV("enrolled password size = %d\n", operation.params[2].memref.size);
    if(!success) {
        uint8_t *auth_token_buf = new uint8_t[operation.params[2].memref.size];

        memcpy(auth_token_buf, authtoken.buffer, operation.params[2].memref.size);
        *auth_token        = auth_token_buf;
        *auth_token_length = operation.params[2].memref.size;

        if (request_reenroll)
            *request_reenroll = operation.params[3].value.a;
        //ALOGV("request_reenroll=%d\n", *request_reenroll);
    } else {
        //ALOGE("success=%d\n", success);
        *auth_token = NULL;
        *auth_token_length = 0;
    }

__enrolled_err:
    if (flag & (1<<2))
        TEEC_ReleaseSharedMemory(&authtoken);
    if (flag & (1<<1))
        TEEC_ReleaseSharedMemory(&current_pw);
    if (flag & (1<<0))
        TEEC_ReleaseSharedMemory(&ps_handle);

    return (int)success;
}

static inline TrustyGateKeeperDevice *convert_device(const gatekeeper_device *dev) {
    return reinterpret_cast<TrustyGateKeeperDevice *>(const_cast<gatekeeper_device *>(dev));
}

/* static */
int TrustyGateKeeperDevice::enroll(const struct gatekeeper_device *dev, uint32_t uid,
            const uint8_t *current_password_handle, uint32_t current_password_handle_length,
            const uint8_t *current_password, uint32_t current_password_length,
            const uint8_t *desired_password, uint32_t desired_password_length,
            uint8_t **enrolled_password_handle, uint32_t *enrolled_password_handle_length) {

    if (dev == NULL ||
            enrolled_password_handle == NULL || enrolled_password_handle_length == NULL ||
            desired_password == NULL || desired_password_length == 0)
        return -EINVAL;

    // Current password and current password handle go together
    if (current_password_handle == NULL || current_password_handle_length == 0 ||
            current_password == NULL || current_password_length == 0) {
        current_password_handle = NULL;
        current_password_handle_length = 0;
        current_password = NULL;
        current_password_length = 0;
    }

    return convert_device(dev)->Enroll(uid, current_password_handle, current_password_handle_length,
            current_password, current_password_length, desired_password, desired_password_length,
            enrolled_password_handle, enrolled_password_handle_length);
}

/* static */
int TrustyGateKeeperDevice::verify(const struct gatekeeper_device *dev, uint32_t uid,
        uint64_t challenge, const uint8_t *enrolled_password_handle,
        uint32_t enrolled_password_handle_length, const uint8_t *provided_password,
        uint32_t provided_password_length, uint8_t **auth_token, uint32_t *auth_token_length,
        bool *request_reenroll) {

    if (dev == NULL || enrolled_password_handle == NULL ||
            provided_password == NULL) {
        return -EINVAL;
    }

    return convert_device(dev)->Verify(uid, challenge, enrolled_password_handle,
            enrolled_password_handle_length, provided_password, provided_password_length,
            auth_token, auth_token_length, request_reenroll);
}
};
