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

#ifndef SUNXI_ROTATOR_DEVICE_H
#define SUNXI_ROTATOR_DEVICE_H

#include <memory>
#include "uniquefd.h"
#include <nativebase/nativebase.h>

namespace sunxi {

class Layer;
class RotateTask;

// OutputBuffer_t describe all the necessary information of the rotator output,
// which declare in HardwareRotator.h.
typedef struct OutputBuffer OutputBuffer_t;

class RotatorDevice {
public:
    class Frame {
    public:
        uint32_t mFrameNumber;
        virtual ~Frame() = default;
    };

    virtual int initialize() = 0;

    virtual std::unique_ptr<Frame> createFrame(
            const std::shared_ptr<Layer>& layer,
            const OutputBuffer_t* outbuf) = 0;

    // Rotate one frame and block until the rotation is finished.
    virtual int rotateOneFrame(const Frame* frame) = 0;

    // Rotate capability detect hook,
    // Return true when the input layer is supported by this rotator
    virtual bool capableHook(const std::shared_ptr<Layer>& layer) const = 0;
    virtual void getAlignedWidthAndHeight(const std::shared_ptr<Layer>& layer,
            int* width, int* height, int* align) const = 0;
    virtual uint32_t computeBufferSize(int width, int height, int align, int pixelformat) const = 0;
    virtual ~RotatorDevice() = default;
};

std::unique_ptr<RotatorDevice> createHardwareRotator();

}
#endif
