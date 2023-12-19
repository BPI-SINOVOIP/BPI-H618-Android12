// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_V4L2_CODEC2_PLUGIN_STORE_H2B_GRAPHIC_BUFFER_PRODUCER_H
#define ANDROID_V4L2_CODEC2_PLUGIN_STORE_H2B_GRAPHIC_BUFFER_PRODUCER_H

#include <C2Buffer.h>
#include <android/hardware/graphics/bufferqueue/2.0/IGraphicBufferProducer.h>
#include <android/hardware/graphics/bufferqueue/2.0/IProducerListener.h>
#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>
#include <utils/StrongPointer.h>

namespace android {

class H2BGraphicBufferProducer {
public:
    using HGraphicBufferProducer =
            ::android::hardware::graphics::bufferqueue::V2_0::IGraphicBufferProducer;
    using HProducerListener = ::android::hardware::graphics::bufferqueue::V2_0::IProducerListener;

    explicit H2BGraphicBufferProducer(sp<HGraphicBufferProducer> base);
    ~H2BGraphicBufferProducer() = default;

    // Convert HIDL interface of IGraphicBufferProducer.
    status_t requestBuffer(int slot, sp<GraphicBuffer>* buf);
    status_t setMaxDequeuedBufferCount(int maxDequeuedBuffers);
    status_t dequeueBuffer(uint32_t width, uint32_t height, uint32_t pixelFormat,
                           C2AndroidMemoryUsage androidUsage, int* slot, sp<Fence>* fence);
    status_t detachBuffer(int slot);
    status_t attachBuffer(const sp<GraphicBuffer>& buffer, int* outSlot);
    status_t cancelBuffer(int slot, const sp<Fence>& fence);
    int query(int what, int* value);
    status_t allowAllocation(bool allow);
    status_t getUniqueId(uint64_t* outId) const;
    status_t connect(sp<HProducerListener> const& hListener, int32_t api,
                     bool producerControlledByApp);
    status_t setDequeueTimeout(nsecs_t timeout);

    sp<HGraphicBufferProducer> getBase() { return mBase; }

private:
    const sp<HGraphicBufferProducer> mBase;
};

}  // namespace android
#endif  // ANDROID_V4L2_CODEC2_PLUGIN_STORE_H2B_GRAPHIC_BUFFER_PRODUCER_H
