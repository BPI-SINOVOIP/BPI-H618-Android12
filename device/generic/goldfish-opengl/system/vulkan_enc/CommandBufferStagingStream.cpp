/*
* Copyright (C) 2021 The Android Open Source Project
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
#include "CommandBufferStagingStream.h"

#if PLATFORM_SDK_VERSION < 26
#include <cutils/log.h>
#else
#include <log/log.h>
#endif
#include <cutils/properties.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

static const size_t kReadSize = 512 * 1024;
static const size_t kWriteOffset = kReadSize;

CommandBufferStagingStream::CommandBufferStagingStream() :
    IOStream(1048576), m_buf(nullptr), m_size(1048576), m_writePos(0) { }

CommandBufferStagingStream::~CommandBufferStagingStream() { flush(); if (m_buf) free(m_buf); }

size_t CommandBufferStagingStream::idealAllocSize(size_t len) {
    if (len > 1048576) return len;
    return 1048576;
}

void *CommandBufferStagingStream::allocBuffer(size_t minSize) {
    size_t allocSize =
        (1048576 < minSize ? minSize : 1048576);

    // Initial case: blank
    if (!m_buf) {
        m_buf = (unsigned char *)malloc(allocSize);
        m_size = allocSize;
        return (void*)m_buf;
    }

    // Calculate remaining
    size_t remaining = m_size - m_writePos;

    if (remaining < allocSize) {
        size_t newAllocSize = m_size * 2 + allocSize;
        unsigned char *p = (unsigned char *)realloc(m_buf, newAllocSize);
        m_buf = p;
        m_size = newAllocSize;
        return (void*)(m_buf + m_writePos);

    }

    return (void*)(m_buf + m_writePos);
};

int CommandBufferStagingStream::commitBuffer(size_t size)
{
    m_writePos += size;
    return 0;
}

const unsigned char *CommandBufferStagingStream::readFully(void*, size_t) {
    // Not supported
    ALOGE("CommandBufferStagingStream::%s: Fatal: not supported\n", __func__);
    abort();
    return nullptr;
}

const unsigned char *CommandBufferStagingStream::read(void*, size_t*) {
    // Not supported
    ALOGE("CommandBufferStagingStream::%s: Fatal: not supported\n", __func__);
    abort();
    return nullptr;
}

int CommandBufferStagingStream::writeFully(const void*, size_t)
{
    // Not supported
    ALOGE("CommandBufferStagingStream::%s: Fatal: not supported\n", __func__);
    abort();
    return 0;
}

const unsigned char *CommandBufferStagingStream::commitBufferAndReadFully(
    size_t, void *, size_t) {

    // Not supported
    ALOGE("CommandBufferStagingStream::%s: Fatal: not supported\n", __func__);
    abort();
    return nullptr;
}

void CommandBufferStagingStream::getWritten(unsigned char** bufOut, size_t* sizeOut) {
    *bufOut = m_buf;
    *sizeOut = m_writePos;
}

void CommandBufferStagingStream::reset() {
    m_writePos = 0;
    IOStream::rewind();
}
