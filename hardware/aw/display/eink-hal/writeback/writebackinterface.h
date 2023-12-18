/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef SUNXI_WRITEBACK_H
#define SUNXI_WRITEBACK_H

#include <memory>
#include <nativebase/nativebase.h>
#include "hardware/sunxi_display2.h"

namespace sunxi {

class WritebackInterface {
public:
    virtual ~WritebackInterface() { }

    // writeback type
    static const int WRITEBACK_TYPE_LEGACY  = 1;
    static const int WRITEBACK_TYPE_SELF_WB = 2;

    virtual int getWritebackModuleType() const = 0;

    struct SourceInfo {
        // indicate writeback api type current using
        int type;

        union {
            // These fields are used for legacy writeback api
            struct {
                int DEIndex;
                int width;
                int height;
                int pixelFormat;
                int pendingFence;
            };

            // These fields are used for self-writeback api
            struct {
                disp_layer_config2* configs;
                int count;
                int acquireFence;
            };
        };
    };

    virtual int writebackOneFrame(const SourceInfo *info) = 0;

    struct OutputBuf {
        buffer_handle_t buffer;
        int width;
        int height;
        int pixelFormat;
        int acquireFence;
    };

    virtual int acquireBuffer(OutputBuf *buf) = 0;
    virtual int releaseBuffer(buffer_handle_t buf, int releaseFence) = 0;
};

WritebackInterface* createWritebackInterface();
void destroyWritebackInterface(WritebackInterface *inf);

} // namespace sunxi

#endif
