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
#ifndef __COMMAND_BUFFER_STAGING_STREAM_H
#define __COMMAND_BUFFER_STAGING_STREAM_H

#include "IOStream.h"

class CommandBufferStagingStream : public IOStream {
public:
    explicit CommandBufferStagingStream();
    ~CommandBufferStagingStream();

    virtual size_t idealAllocSize(size_t len);
    virtual void *allocBuffer(size_t minSize);
    virtual int commitBuffer(size_t size);
    virtual const unsigned char *readFully( void *buf, size_t len);
    virtual const unsigned char *read( void *buf, size_t *inout_len);
    virtual int writeFully(const void *buf, size_t len);
    virtual const unsigned char *commitBufferAndReadFully(size_t size, void *buf, size_t len);

    void getWritten(unsigned char** bufOut, size_t* sizeOut);
    void reset();

private:
    unsigned char* m_buf;
    size_t m_size;
    uint32_t m_writePos;
};

#endif
