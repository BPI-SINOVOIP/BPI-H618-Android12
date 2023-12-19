// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//#define LOG_NDEBUG 0
#define LOG_TAG "H2BGraphicBuferProducer"

#include <v4l2_codec2/plugin_store/H2BGraphicBufferProducer.h>

#include <log/log.h>
#include <types.h>
#include <ui/BufferQueueDefs.h>

namespace android {

using ::android::BufferQueueDefs::BUFFER_NEEDS_REALLOCATION;
using ::android::BufferQueueDefs::RELEASE_ALL_BUFFERS;
using ::android::hardware::Return;

using HBuffer = ::android::hardware::graphics::common::V1_2::HardwareBuffer;
using HStatus = ::android::hardware::graphics::bufferqueue::V2_0::Status;
using HConnectionType = hardware::graphics::bufferqueue::V2_0::ConnectionType;
using HQueueBufferOutput =
        ::android::hardware::graphics::bufferqueue::V2_0::IGraphicBufferProducer::QueueBufferOutput;

using ::android::hardware::graphics::bufferqueue::V2_0::utils::b2h;
using ::android::hardware::graphics::bufferqueue::V2_0::utils::h2b;
using ::android::hardware::graphics::bufferqueue::V2_0::utils::HFenceWrapper;

H2BGraphicBufferProducer::H2BGraphicBufferProducer(sp<HGraphicBufferProducer> base) : mBase(base) {}

status_t H2BGraphicBufferProducer::requestBuffer(int slot, sp<GraphicBuffer>* buf) {
    bool converted = false;
    status_t status = UNKNOWN_ERROR;
    Return<void> transResult = mBase->requestBuffer(
            slot, [&converted, &status, buf](HStatus hStatus, HBuffer const& hBuffer,
                                             uint32_t generationNumber) {
                converted = h2b(hStatus, &status) && h2b(hBuffer, buf);
                if (*buf) {
                    (*buf)->setGenerationNumber(generationNumber);
                }
            });

    if (!transResult.isOk()) {
        ALOGE("%s(): transaction failed: %s", __func__, transResult.description().c_str());
        return FAILED_TRANSACTION;
    }
    if (!converted) {
        ALOGE("%s(): corrupted transaction.", __func__);
        return FAILED_TRANSACTION;
    }
    if (status != OK) {
        ALOGD("%s() failed: %d", __func__, status);
    }
    return status;
}

status_t H2BGraphicBufferProducer::setMaxDequeuedBufferCount(int maxDequeuedBuffers) {
    status_t status = UNKNOWN_ERROR;
    Return<HStatus> transResult =
            mBase->setMaxDequeuedBufferCount(static_cast<int32_t>(maxDequeuedBuffers));

    if (!transResult.isOk()) {
        ALOGE("%s(): transaction failed: %s", __func__, transResult.description().c_str());
        return FAILED_TRANSACTION;
    }
    if (!h2b(static_cast<HStatus>(transResult), &status)) {
        ALOGE("%s(): corrupted transaction.", __func__);
        return FAILED_TRANSACTION;
    }
    if (status != OK) {
        ALOGD("%s() failed: %d", __func__, status);
    }
    return status;
}

status_t H2BGraphicBufferProducer::dequeueBuffer(uint32_t width, uint32_t height,
                                                 uint32_t pixelFormat,
                                                 C2AndroidMemoryUsage androidUsage, int* slot,
                                                 sp<Fence>* fence) {
    using Input = HGraphicBufferProducer::DequeueBufferInput;
    using Output = HGraphicBufferProducer::DequeueBufferOutput;
    Input input{width, height, pixelFormat, androidUsage.asGrallocUsage()};

    bool converted = false;
    status_t status = UNKNOWN_ERROR;
    Return<void> transResult = mBase->dequeueBuffer(
            input, [&converted, &status, &slot, &fence](HStatus hStatus, int32_t hSlot,
                                                        Output const& hOutput) {
                converted = h2b(hStatus, &status);
                if (!converted || status != OK) {
                    return;
                }

                *slot = hSlot;
                if (hOutput.bufferNeedsReallocation) {
                    status = BUFFER_NEEDS_REALLOCATION;
                }
                converted = h2b(hOutput.fence, fence);
            });

    if (!transResult.isOk()) {
        ALOGE("%s(): transaction failed: %s", __func__, transResult.description().c_str());
        return FAILED_TRANSACTION;
    }
    if (!converted) {
        ALOGE("%s(): corrupted transaction.", __func__);
        return FAILED_TRANSACTION;
    }
    // The C2VdaBqBlockPool does not fully own the bufferqueue. After buffers are dequeued here,
    // they are passed into the codec2 framework, processed, and eventually queued into the
    // bufferqueue. The C2VdaBqBlockPool cannot determine exactly when a buffer gets queued.
    // However, if every buffer is being processed by the codec2 framework, then dequeueBuffer()
    // will return INVALID_OPERATION because of an attempt to dequeue too many buffers.
    // The C2VdaBqBlockPool cannot prevent this from happening, so just map it to TIMED_OUT
    // and let the C2VdaBqBlockPool's caller's timeout retry logic handle the failure.
    if (status == INVALID_OPERATION) {
        status = TIMED_OUT;
    }
    if (status != OK && status != BUFFER_NEEDS_REALLOCATION && status != TIMED_OUT) {
        ALOGD("%s() failed: %d", __func__, status);
    }
    return status;
}

status_t H2BGraphicBufferProducer::detachBuffer(int slot) {
    status_t status = UNKNOWN_ERROR;
    Return<HStatus> transResult = mBase->detachBuffer(static_cast<int32_t>(slot));

    if (!transResult.isOk()) {
        ALOGE("%s(): transaction failed: %s", __func__, transResult.description().c_str());
        return FAILED_TRANSACTION;
    }
    if (!h2b(static_cast<HStatus>(transResult), &status)) {
        ALOGE("%s(): corrupted transaction.", __func__);
        return FAILED_TRANSACTION;
    }
    if (status != OK) {
        ALOGD("%s() failed: %d", __func__, status);
    }
    return status;
}

status_t H2BGraphicBufferProducer::attachBuffer(const sp<GraphicBuffer>& buffer, int* outSlot) {
    HBuffer hBuffer;
    uint32_t hGenerationNumber;
    if (!b2h(buffer, &hBuffer, &hGenerationNumber)) {
        ALOGE("%s: invalid input buffer.", __func__);
        return BAD_VALUE;
    }

    bool converted = false;
    status_t status = UNKNOWN_ERROR;
    Return<void> transResult = mBase->attachBuffer(
            hBuffer, hGenerationNumber,
            [&converted, &status, outSlot](HStatus hStatus, int32_t hSlot, bool releaseAllBuffers) {
                converted = h2b(hStatus, &status);
                *outSlot = static_cast<int>(hSlot);
                if (converted && releaseAllBuffers && status == OK) {
                    status = RELEASE_ALL_BUFFERS;
                }
            });

    if (!transResult.isOk()) {
        ALOGE("%s(): transaction failed: %s", __func__, transResult.description().c_str());
        return FAILED_TRANSACTION;
    }
    if (!converted) {
        ALOGE("%s(): corrupted transaction.", __func__);
        return FAILED_TRANSACTION;
    }
    if (status != OK) {
        ALOGD("%s() failed: %d", __func__, status);
    }
    return status;
}

status_t H2BGraphicBufferProducer::cancelBuffer(int slot, const sp<Fence>& fence) {
    HFenceWrapper hFenceWrapper;
    if (!b2h(fence, &hFenceWrapper)) {
        ALOGE("%s(): corrupted input fence.", __func__);
        return UNKNOWN_ERROR;
    }

    status_t status = UNKNOWN_ERROR;
    Return<HStatus> transResult =
            mBase->cancelBuffer(static_cast<int32_t>(slot), hFenceWrapper.getHandle());

    if (!transResult.isOk()) {
        ALOGE("%s(): transaction failed: %s", __func__, transResult.description().c_str());
        return FAILED_TRANSACTION;
    }
    if (!h2b(static_cast<HStatus>(transResult), &status)) {
        ALOGE("%s(): corrupted transaction.", __func__);
        return FAILED_TRANSACTION;
    }
    if (status != OK) {
        ALOGD("%s() failed: %d", __func__, status);
    }
    return status;
}

int H2BGraphicBufferProducer::query(int what, int* value) {
    int result = 0;
    Return<void> transResult =
            mBase->query(static_cast<int32_t>(what), [&result, value](int32_t r, int32_t v) {
                result = static_cast<int>(r);
                *value = static_cast<int>(v);
            });

    if (!transResult.isOk()) {
        ALOGE("%s(): transaction failed: %s", __func__, transResult.description().c_str());
        return FAILED_TRANSACTION;
    }
    return result;
}

status_t H2BGraphicBufferProducer::allowAllocation(bool allow) {
    status_t status = UNKNOWN_ERROR;
    Return<HStatus> transResult = mBase->allowAllocation(allow);

    if (!transResult.isOk()) {
        ALOGE("%s(): transaction failed: %s", __func__, transResult.description().c_str());
        return FAILED_TRANSACTION;
    }
    if (!h2b(static_cast<HStatus>(transResult), &status)) {
        ALOGE("%s(): corrupted transaction.", __func__);
        return FAILED_TRANSACTION;
    }
    if (status != OK) {
        ALOGD("%s() failed: %d", __func__, status);
    }
    return status;
}

status_t H2BGraphicBufferProducer::getUniqueId(uint64_t* outId) const {
    Return<uint64_t> transResult = mBase->getUniqueId();

    if (!transResult.isOk()) {
        ALOGE("%s(): transaction failed: %s", __func__, transResult.description().c_str());
        return FAILED_TRANSACTION;
    }

    *outId = static_cast<uint64_t>(transResult);
    return OK;
}

// android::IProducerListener cannot be depended by vendor library, so we use HProducerListener
// directly.
status_t H2BGraphicBufferProducer::connect(sp<HProducerListener> const& hListener, int32_t api,
                                           bool producerControlledByApp) {
    bool converted = false;
    status_t status = UNKNOWN_ERROR;
    // hack(b/146409777): We pass self-defined api, so we don't use b2h() here.
    Return<void> transResult = mBase->connect(
            hListener, static_cast<HConnectionType>(api), producerControlledByApp,
            [&converted, &status](HStatus hStatus, HQueueBufferOutput const& /* hOutput */) {
                converted = h2b(hStatus, &status);
            });

    if (!transResult.isOk()) {
        ALOGE("%s(): transaction failed: %s", __func__, transResult.description().c_str());
        return FAILED_TRANSACTION;
    }
    if (!converted) {
        ALOGE("%s(): corrupted transaction.", __func__);
        return FAILED_TRANSACTION;
    }
    return status;
}

status_t H2BGraphicBufferProducer::setDequeueTimeout(nsecs_t timeout) {
    status_t status = UNKNOWN_ERROR;
    Return<HStatus> transResult = mBase->setDequeueTimeout(static_cast<int64_t>(timeout));

    if (!transResult.isOk()) {
        ALOGE("%s(): transaction failed: %s", __func__, transResult.description().c_str());
        return FAILED_TRANSACTION;
    }
    if (!h2b(static_cast<HStatus>(transResult), &status)) {
        ALOGE("%s(): corrupted transaction.", __func__);
        return FAILED_TRANSACTION;
    }
    return status;
}

}  // namespace android
