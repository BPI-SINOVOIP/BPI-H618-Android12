/*
* Copyright (C) 2016 The Android Open Source Project
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

#include "ProcessPipe.h"
#include "HostConnection.h"
#include "renderControl_enc.h"

#include <qemu_pipe_bp.h>

#if PLATFORM_SDK_VERSION < 26
#include <cutils/log.h>
#else
#include <log/log.h>
#endif
#include <pthread.h>
#include <errno.h>

#ifdef __Fuchsia__
#include <fuchsia/hardware/goldfish/llcpp/fidl.h>
#include <lib/zx/vmo.h>

#include "services/service_connector.h"

#define GET_STATUS_SAFE(result, member) \
    ((result).ok() ? ((result).Unwrap()->member) : ZX_OK)

static QEMU_PIPE_HANDLE   sProcDevice = 0;
#else // __Fuchsia__

#include "VirtioGpuPipeStream.h"
static VirtioGpuPipeStream* sVirtioGpuPipeStream = 0;

#endif // !__Fuchsia__

static QEMU_PIPE_HANDLE   sProcPipe = 0;
static pthread_once_t     sProcPipeOnce = PTHREAD_ONCE_INIT;
// sProcUID is a unique ID per process assigned by the host.
// It is different from getpid().
static uint64_t           sProcUID = 0;
static volatile HostConnectionType sConnType = HOST_CONNECTION_VIRTIO_GPU_PIPE;

static uint32_t* sSeqnoPtr = 0;

// Meant to be called only once per process.
static void initSeqno() {
    // So why do we reinitialize here? It's for testing purposes only;
    // we have a unit test that exercise the case where this sequence
    // number is reset as a result of guest process kill.
    if (sSeqnoPtr) delete sSeqnoPtr;
    sSeqnoPtr = new uint32_t;
    *sSeqnoPtr = 0;
}

// processPipeInitOnce is used to generate a process unique ID (puid).
// processPipeInitOnce will only be called at most once per process.
// Use it with pthread_once for thread safety.
// The host associates resources with process unique ID (puid) for memory cleanup.
// It will fallback to the default path if the host does not support it.
// Processes are identified by acquiring a per-process 64bit unique ID from the
// host.
#ifdef __Fuchsia__
static void processPipeInitOnce() {
    initSeqno();

    fidl::ClientEnd<fuchsia_hardware_goldfish::PipeDevice> channel{
        zx::channel(GetConnectToServiceFunction()(QEMU_PIPE_PATH))};
    if (!channel) {
        ALOGE("%s: failed to open " QEMU_PIPE_PATH,
              __FUNCTION__);
        return;
    }

    fidl::WireSyncClient<fuchsia_hardware_goldfish::PipeDevice> device(
        std::move(channel));

    auto pipe_ends =
        fidl::CreateEndpoints<::fuchsia_hardware_goldfish::Pipe>();
    if (!pipe_ends.is_ok()) {
        ALOGE("%s: zx_channel_create failed: %d", __FUNCTION__, pipe_ends.status_value());
        return;
    }

    fidl::WireSyncClient<fuchsia_hardware_goldfish::Pipe> pipe(
        std::move(pipe_ends->client));
    device.OpenPipe(std::move(pipe_ends->server));

    zx::vmo vmo;
    {
        auto result = pipe.GetBuffer();
        if (!result.ok() || result.Unwrap()->res != ZX_OK) {
            ALOGE("%s: failed to get buffer: %d:%d", __FUNCTION__,
                  result.status(), GET_STATUS_SAFE(result, res));
            return;
        }
        vmo = std::move(result.Unwrap()->vmo);
    }

    size_t len = strlen("pipe:GLProcessPipe");
    zx_status_t status = vmo.write("pipe:GLProcessPipe", 0, len + 1);
    if (status != ZX_OK) {
        ALOGE("%s: failed write pipe name", __FUNCTION__);
        return;
    }

    {
        auto result = pipe.Write(len + 1, 0);
        if (!result.ok() || result.Unwrap()->res != ZX_OK) {
            ALOGD("%s: connecting to pipe service failed: %d:%d", __FUNCTION__,
                  result.status(), GET_STATUS_SAFE(result, res));
            return;
        }
    }

    // Send a confirmation int to the host and get per-process unique ID back
    int32_t confirmInt = 100;
    status = vmo.write(&confirmInt, 0, sizeof(confirmInt));
    if (status != ZX_OK) {
        ALOGE("%s: failed write confirm int", __FUNCTION__);
        return;
    }

    {
        auto result = pipe.DoCall(sizeof(confirmInt), 0, sizeof(sProcUID), 0);
        if (!result.ok() || result.Unwrap()->res != ZX_OK) {
            ALOGD("%s: failed to get per-process ID: %d:%d", __FUNCTION__,
                  result.status(), GET_STATUS_SAFE(result, res));
            return;
        }
    }

    status = vmo.read(&sProcUID, 0, sizeof(sProcUID));
    if (status != ZX_OK) {
        ALOGE("%s: failed read per-process ID: %d", __FUNCTION__, status);
        return;
    }
    sProcDevice = device.mutable_channel()->release();
    sProcPipe = pipe.mutable_channel()->release();
}
#else // __Fuchsia__

static void sQemuPipeInit() {
    sProcPipe = qemu_pipe_open("GLProcessPipe");
    if (!qemu_pipe_valid(sProcPipe)) {
        sProcPipe = 0;
        ALOGW("Process pipe failed");
        return;
    }
    // Send a confirmation int to the host
    int32_t confirmInt = 100;
    if (qemu_pipe_write_fully(sProcPipe, &confirmInt, sizeof(confirmInt))) { // failed
        qemu_pipe_close(sProcPipe);
        sProcPipe = 0;
        ALOGW("Process pipe failed");
        return;
    }

    // Ask the host for per-process unique ID
    if (qemu_pipe_read_fully(sProcPipe, &sProcUID, sizeof(sProcUID))) {
        qemu_pipe_close(sProcPipe);
        sProcPipe = 0;
        sProcUID = 0;
        ALOGW("Process pipe failed");
        return;
    }
}

static void processPipeInitOnce() {
    initSeqno();

#if defined(HOST_BUILD) || !defined(GFXSTREAM)
    sQemuPipeInit();
#else // HOST_BUILD
    switch (sConnType) {
        // TODO: Move those over too
        case HOST_CONNECTION_QEMU_PIPE:
        case HOST_CONNECTION_ADDRESS_SPACE:
        case HOST_CONNECTION_TCP:
        case HOST_CONNECTION_VIRTIO_GPU:
            sQemuPipeInit();
            break;
        case HOST_CONNECTION_VIRTIO_GPU_PIPE:
        case HOST_CONNECTION_VIRTIO_GPU_ADDRESS_SPACE: {
            sVirtioGpuPipeStream = new VirtioGpuPipeStream(4096);
            sProcUID = sVirtioGpuPipeStream->initProcessPipe();
            break;
        }
    }
#endif // !HOST_BUILD
}
#endif // !__Fuchsia__

bool processPipeInit(HostConnectionType connType, renderControl_encoder_context_t *rcEnc) {
    sConnType = connType;
    pthread_once(&sProcPipeOnce, processPipeInitOnce);
    bool pipeHandleInvalid = !sProcPipe;
#ifndef __Fuchsia__
    pipeHandleInvalid = pipeHandleInvalid && !sVirtioGpuPipeStream;
#endif // !__Fuchsia__
    if (pipeHandleInvalid) return false;
    rcEnc->rcSetPuid(rcEnc, sProcUID);
    return true;
}

uint64_t getPuid() {
    return sProcUID;
}

void processPipeRestart() {
    ALOGW("%s: restarting process pipe\n", __func__);
    bool isPipe = false;

    switch (sConnType) {
        // TODO: Move those over too
        case HOST_CONNECTION_QEMU_PIPE:
        case HOST_CONNECTION_ADDRESS_SPACE:
        case HOST_CONNECTION_TCP:
        case HOST_CONNECTION_VIRTIO_GPU:
            isPipe = true;
            break;
        case HOST_CONNECTION_VIRTIO_GPU_PIPE:
        case HOST_CONNECTION_VIRTIO_GPU_ADDRESS_SPACE: {
            isPipe = false;
            break;
        }
    }

    sProcUID = 0;

#ifdef __Fuchsia__
    zx_handle_close(sProcPipe);
    sProcPipe = ZX_HANDLE_INVALID;
#else
    if (isPipe) {
        if (qemu_pipe_valid(sProcPipe)) {
            qemu_pipe_close(sProcPipe);
            sProcPipe = 0;
        }
    } else {
        delete sVirtioGpuPipeStream;
        sVirtioGpuPipeStream = nullptr;
    }
#endif // __Fuchsia__

    processPipeInitOnce();
};

void refreshHostConnection() {
    HostConnection* hostConn = HostConnection::get();
    ExtendedRCEncoderContext* rcEnc = hostConn->rcEncoder();
    rcEnc->rcSetPuid(rcEnc, sProcUID);
}

uint32_t* getSeqnoPtrForProcess() {
    // It's assumed process pipe state has already been initialized.
    return sSeqnoPtr;
}
