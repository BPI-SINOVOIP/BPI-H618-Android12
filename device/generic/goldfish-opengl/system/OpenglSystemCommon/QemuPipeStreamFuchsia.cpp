/*
* Copyright (C) 2019 The Android Open Source Project
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
#include "QemuPipeStream.h"

#include <cutils/log.h>
#include <errno.h>
#include <lib/zx/channel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zircon/process.h>

#include <utility>

#include "services/service_connector.h"

#define GET_STATUS_SAFE(result, member) \
    ((result).ok() ? ((result).Unwrap()->member) : ZX_OK)

constexpr size_t kReadSize = 512 * 1024;
constexpr size_t kWriteOffset = kReadSize;

QemuPipeStream::QemuPipeStream(size_t bufSize) :
    IOStream(bufSize),
    m_sock(-1),
    m_bufsize(bufSize),
    m_buf(nullptr),
    m_read(0),
    m_readLeft(0)
{
}

QemuPipeStream::QemuPipeStream(QEMU_PIPE_HANDLE sock, size_t bufSize) :
    IOStream(bufSize),
    m_sock(sock),
    m_bufsize(bufSize),
    m_buf(nullptr),
    m_read(0),
    m_readLeft(0)
{
}

QemuPipeStream::~QemuPipeStream()
{
    if (m_device) {
        flush();
    }
    if (m_buf) {
        zx_status_t status = zx_vmar_unmap(zx_vmar_root_self(),
                                           reinterpret_cast<zx_vaddr_t>(m_buf),
                                           m_bufsize);
        if (status != ZX_OK) {
            ALOGE("zx_vmar_unmap failed: %d\n", status);
            abort();
        }
    }
}

int QemuPipeStream::connect(void)
{
    fidl::ClientEnd<fuchsia_hardware_goldfish::PipeDevice> channel{
        zx::channel(GetConnectToServiceFunction()(QEMU_PIPE_PATH))};
    if (!channel) {
        ALOGE("%s: failed to get service handle for " QEMU_PIPE_PATH,
              __FUNCTION__);
        return -1;
    }

    m_device = std::make_unique<
        fidl::WireSyncClient<fuchsia_hardware_goldfish::PipeDevice>>(
        std::move(channel));

    auto pipe_ends =
        fidl::CreateEndpoints<::fuchsia_hardware_goldfish::Pipe>();
    if (!pipe_ends.is_ok()) {
        ALOGE("zx::channel::create failed: %d", pipe_ends.status_value());
        return ZX_HANDLE_INVALID;
    }
    m_device->OpenPipe(std::move(pipe_ends->server));
    m_pipe =
        std::make_unique<fidl::WireSyncClient<fuchsia_hardware_goldfish::Pipe>>(
            std::move(pipe_ends->client));

    zx::event event;
    zx_status_t status = zx::event::create(0, &event);
    if (status != ZX_OK) {
        ALOGE("%s: failed to create event: %d", __FUNCTION__, status);
        return -1;
    }
    zx::event event_copy;
    status = event.duplicate(ZX_RIGHT_SAME_RIGHTS, &event_copy);
    if (status != ZX_OK) {
        ALOGE("%s: failed to duplicate event: %d", __FUNCTION__, status);
        return -1;
    }

    {
        auto result = m_pipe->SetEvent(std::move(event_copy));
        if (!result.ok()) {
            ALOGE("%s: failed to set event: %d:%d", __FUNCTION__,
                  result.status());
            return -1;
        }
    }

    if (!allocBuffer(m_bufsize)) {
        ALOGE("%s: failed allocate initial buffer", __FUNCTION__);
        return -1;
    }

    size_t len = strlen("pipe:opengles");
    status = m_vmo.write("pipe:opengles", 0, len + 1);
    if (status != ZX_OK) {
        ALOGE("%s: failed write pipe name", __FUNCTION__);
        return -1;
    }

    {
        auto result = m_pipe->Write(len + 1, 0);
        if (!result.ok() || result.Unwrap()->res != ZX_OK) {
            ALOGD("%s: connecting to pipe service failed: %d:%d", __FUNCTION__,
                  result.status(), GET_STATUS_SAFE(result, res));
            return -1;
        }
    }

    m_event = std::move(event);
    return 0;
}

void *QemuPipeStream::allocBuffer(size_t minSize)
{
    // Add dedicated read buffer space at the front of buffer.
    minSize += kReadSize;

    zx_status_t status;
    if (m_buf) {
        if (minSize <= m_bufsize) {
            return m_buf + kWriteOffset;
        }
        status = zx_vmar_unmap(zx_vmar_root_self(),
                               reinterpret_cast<zx_vaddr_t>(m_buf),
                               m_bufsize);
        if (status != ZX_OK) {
          ALOGE("zx_vmar_unmap failed: %d\n", status);
          abort();
        }
        m_buf = nullptr;
    }

    size_t allocSize = m_bufsize < minSize ? minSize : m_bufsize;

    {
        auto result = m_pipe->SetBufferSize(allocSize);
        if (!result.ok() || result.Unwrap()->res != ZX_OK) {
            ALOGE("%s: failed to get buffer: %d:%d", __FUNCTION__,
                  result.status(), GET_STATUS_SAFE(result, res));
            return nullptr;
        }
    }

    zx::vmo vmo;
    {
        auto result = m_pipe->GetBuffer();
        if (!result.ok() || result.Unwrap()->res != ZX_OK) {
            ALOGE("%s: failed to get buffer: %d:%d", __FUNCTION__,
                  result.status(), GET_STATUS_SAFE(result, res));
            return nullptr;
        }
        vmo = std::move(result.Unwrap()->vmo);
    }

    zx_vaddr_t mapped_addr;
    status =
        zx_vmar_map(zx_vmar_root_self(), ZX_VM_PERM_READ | ZX_VM_PERM_WRITE, 0,
                    vmo.get(), 0, allocSize, &mapped_addr);
    if (status != ZX_OK) {
        ALOGE("%s: failed to map buffer: %d", __FUNCTION__, status);
        return nullptr;
    }

    m_buf = reinterpret_cast<unsigned char*>(mapped_addr);
    m_bufsize = allocSize;
    m_vmo = std::move(vmo);
    return m_buf + kWriteOffset;
}

int QemuPipeStream::commitBuffer(size_t size)
{
    if (size == 0) return 0;

    auto result = m_pipe->DoCall(size, kWriteOffset, 0, 0);
    if (!result.ok() || result.Unwrap()->res != ZX_OK) {
        ALOGD("%s: Pipe call failed: %d:%d", __FUNCTION__, result.status(),
              GET_STATUS_SAFE(result, res));
        return -1;
    }

    return 0;
}

int QemuPipeStream::writeFully(const void *buf, size_t len)
{
    ALOGE("%s: unsupported", __FUNCTION__);
    abort();
    return -1;
}

QEMU_PIPE_HANDLE QemuPipeStream::getSocket() const {
    return m_sock;
}

const unsigned char *QemuPipeStream::readFully(void *buf, size_t len)
{
    return commitBufferAndReadFully(0, buf, len);
}

const unsigned char *QemuPipeStream::commitBufferAndReadFully(size_t size, void *buf, size_t len)
{
    if (!m_device)
        return nullptr;

    if (!buf) {
        if (len > 0) {
            ALOGE("QemuPipeStream::commitBufferAndReadFully failed, buf=NULL, len %zu, lethal"
                    " error, exiting.", len);
            abort();
        }
        if (!size) {
            return nullptr;
        }
    }

    // Advance buffered read if not yet consumed.
    size_t remaining = len;
    size_t readSize = m_readLeft < remaining ? m_readLeft : remaining;
    if (readSize) {
        memcpy(static_cast<char*>(buf), m_buf + (m_read - m_readLeft), readSize);
        remaining -= readSize;
        m_readLeft -= readSize;
    }

    // Early out if nothing left to do.
    if (!size && !remaining) {
        return static_cast<const unsigned char *>(buf);
    }

    // Read up to kReadSize bytes if all buffered read has been consumed.
    size_t maxRead = (m_readLeft || !remaining) ? 0 : kReadSize;

    auto result = m_pipe->DoCall(size, kWriteOffset, maxRead, 0);
    if (!result.ok()) {
        ALOGD("%s: Pipe call failed: %d", __FUNCTION__, result.status());
        return nullptr;
    }

    // Updated buffered read size.
    if (result.Unwrap()->actual) {
        m_read = m_readLeft = result.Unwrap()->actual;
    }

    // Consume buffered read and read more if neccessary.
    while (remaining) {
        readSize = m_readLeft < remaining ? m_readLeft : remaining;
        if (readSize) {
            memcpy(static_cast<char*>(buf) + (len - remaining),
                   m_buf + (m_read - m_readLeft),
                   readSize);
            remaining -= readSize;
            m_readLeft -= readSize;
            continue;
        }

        auto result = m_pipe->Read(kReadSize, 0);
        if (!result.ok()) {
            ALOGD("%s: Failed reading from pipe: %d:%d", __FUNCTION__,
                  result.status());
            return nullptr;
        }

        if (result.Unwrap()->actual) {
            m_read = m_readLeft = result.Unwrap()->actual;
            continue;
        }
        if (result.Unwrap()->res != ZX_ERR_SHOULD_WAIT) {
            ALOGD("%s: Error reading from pipe: %d", __FUNCTION__,
                  result.Unwrap()->res);
            return nullptr;
        }

        zx_signals_t observed = ZX_SIGNAL_NONE;
        zx_status_t status = m_event.wait_one(
            fuchsia_hardware_goldfish::wire::kSignalReadable |
                fuchsia_hardware_goldfish::wire::kSignalHangup,
            zx::time::infinite(), &observed);
        if (status != ZX_OK) {
            ALOGD("%s: wait_one failed: %d", __FUNCTION__, status);
            return nullptr;
        }
        if (observed & fuchsia_hardware_goldfish::wire::kSignalHangup) {
            ALOGD("%s: Remote end hungup", __FUNCTION__);
            return nullptr;
        }
    }

    return static_cast<const unsigned char *>(buf);
}

const unsigned char *QemuPipeStream::read(void *buf, size_t *inout_len)
{
    ALOGE("%s: unsupported", __FUNCTION__);
    abort();
    return nullptr;
}

int QemuPipeStream::recv(void *buf, size_t len)
{
    ALOGE("%s: unsupported", __FUNCTION__);
    abort();
    return -1;
}
