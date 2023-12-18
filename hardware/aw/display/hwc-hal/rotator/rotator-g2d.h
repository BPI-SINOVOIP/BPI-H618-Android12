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

#ifndef SUNXI_ROTATOR_G2D_H
#define SUNXI_ROTATOR_G2D_H

#include "Layer.h"
#include "private_handle.h"
#include "RotatorDevice.h"
#include "sunxi_g2d.h"

namespace sunxi {

class G2DFrame : public RotatorDevice::Frame {
public:
    explicit G2DFrame(uint32_t id): mBlitInfo() { mFrameNumber = id; }
   ~G2DFrame() = default;
    friend class RotatorG2D;
private:
    uniquefd mCacheBufferHandle[2];
    g2d_blt_h mBlitInfo = {};
};

class RotatorG2D : public RotatorDevice {
public:
    RotatorG2D();
   ~RotatorG2D();

   int initialize() override;
   std::unique_ptr<RotatorDevice::Frame> createFrame(
           const std::shared_ptr<Layer>& layer, const OutputBuffer_t* outbuf) override;
    int rotateOneFrame(const RotatorDevice::Frame* frame) override;
    bool capableHook(const std::shared_ptr<Layer>& layer) const override;
    void getAlignedWidthAndHeight(const std::shared_ptr<Layer>& layer,
            int* width, int* height, int* align) const override;
    uint32_t computeBufferSize(int width, int height, int align, int pixelformat) const override;
private:
    void setupBlitInfo(const std::shared_ptr<Layer>& layer,
            const OutputBuffer_t* outbuf, G2DFrame* frame) const;

    static bool inputStrideSupported(buffer_handle_t buf);
    static int halPixelFormat2DeviceFormat(int format);
    static int deviceFormat2HalPixelFormat(int format);
    static int halTransform2DeviceFlag(HWC2::Transform transform);

    void loadHardwareVersion();

    static const int MaxInputBufferWidth  = 2048;
    static const int MaxInputBufferHeight = 2048;

    uniquefd mG2DHandle;

    // The legacy version G2D has a 64 bytes alignment bug,
    // We need to rely on this to adjust our harware rotate judgment conditions.
    bool mLegacyHardwareVersion;
};

} // namespace sunxi

#endif
