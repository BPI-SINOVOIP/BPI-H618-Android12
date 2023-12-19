// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//#define LOG_NDEBUG 0
#define LOG_TAG "C2VdaBqBlockPool"

#include <v4l2_codec2/plugin_store/C2VdaBqBlockPool.h>

#include <errno.h>
#include <string.h>

#include <chrono>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>

#include <C2AllocatorGralloc.h>
#include <C2BlockInternal.h>
#include <C2SurfaceSyncObj.h>
#include <android/hardware/graphics/bufferqueue/2.0/IProducerListener.h>
#include <base/callback.h>
#include <log/log.h>
#include <ui/BufferQueueDefs.h>

#include <v4l2_codec2/plugin_store/DrmGrallocHelpers.h>
#include <v4l2_codec2/plugin_store/H2BGraphicBufferProducer.h>
#include <v4l2_codec2/plugin_store/V4L2AllocatorId.h>

namespace android {
namespace {

// The wait time for acquire fence in milliseconds. The normal display is 60Hz,
// which period is 16ms. We choose 2x period as timeout.
constexpr int kFenceWaitTimeMs = 32;

// The default maximum dequeued buffer count of IGBP. Currently we don't use
// this value to restrict the count of allocated buffers, so we choose a huge
// enough value here.
constexpr int kMaxDequeuedBufferCount = 32u;

}  // namespace

using namespace std::chrono_literals;

// We use the value of DRM handle as the unique ID of the graphic buffers.
using unique_id_t = uint32_t;
// Type for IGBP slot index.
using slot_t = int32_t;

using ::android::BufferQueueDefs::BUFFER_NEEDS_REALLOCATION;
using ::android::BufferQueueDefs::NUM_BUFFER_SLOTS;
using ::android::hardware::Return;
using HProducerListener = ::android::hardware::graphics::bufferqueue::V2_0::IProducerListener;

static c2_status_t asC2Error(status_t err) {
    switch (err) {
    case OK:
        return C2_OK;
    case NO_INIT:
        return C2_NO_INIT;
    case BAD_VALUE:
        return C2_BAD_VALUE;
    case TIMED_OUT:
        return C2_TIMED_OUT;
    case WOULD_BLOCK:
        return C2_BLOCKING;
    case NO_MEMORY:
        return C2_NO_MEMORY;
    }
    return C2_CORRUPTED;
}

// Convert GraphicBuffer to C2GraphicAllocation and wrap producer id and slot index.
std::shared_ptr<C2GraphicAllocation> ConvertGraphicBuffer2C2Allocation(
        sp<GraphicBuffer> graphicBuffer, const uint64_t igbpId, const slot_t slot,
        C2Allocator* const allocator) {
    ALOGV("%s(idbpId=0x%" PRIx64 ", slot=%d)", __func__, igbpId, slot);

    C2Handle* c2Handle = WrapNativeCodec2GrallocHandle(
            graphicBuffer->handle, graphicBuffer->width, graphicBuffer->height,
            graphicBuffer->format, graphicBuffer->usage, graphicBuffer->stride,
            graphicBuffer->getGenerationNumber(), igbpId, slot);
    if (!c2Handle) {
        ALOGE("WrapNativeCodec2GrallocHandle() failed");
        return nullptr;
    }

    std::shared_ptr<C2GraphicAllocation> allocation;
    const auto err = allocator->priorGraphicAllocation(c2Handle, &allocation);
    if (err != C2_OK) {
        ALOGE("C2Allocator::priorGraphicAllocation() failed: %d", err);
        native_handle_close(c2Handle);
        native_handle_delete(c2Handle);
        return nullptr;
    }

    return allocation;
}

// This class is used to notify the listener when a certain event happens.
class EventNotifier : public virtual android::RefBase {
public:
    class Listener {
    public:
        virtual ~Listener() = default;

        // Called by EventNotifier when a certain event happens.
        virtual void onEventNotified() = 0;
    };

    explicit EventNotifier(std::weak_ptr<Listener> listener) : mListener(std::move(listener)) {}
    virtual ~EventNotifier() = default;

protected:
    void notify() {
        ALOGV("%s()", __func__);
        std::shared_ptr<Listener> listener = mListener.lock();
        if (listener) {
            listener->onEventNotified();
        }
    }

    std::weak_ptr<Listener> mListener;
};

// Notifies the listener when the connected IGBP releases buffers.
class BufferReleasedNotifier : public EventNotifier, public HProducerListener {
public:
    using EventNotifier::EventNotifier;
    ~BufferReleasedNotifier() override = default;

    // HProducerListener implementation
    Return<void> onBuffersReleased(uint32_t count) override {
        ALOGV("%s(%u)", __func__, count);
        if (count > 0) {
            notify();
        }
        return {};
    }
};

// IGBP expects its user (e.g. C2VdaBqBlockPool) to keep the mapping from dequeued slot index to
// graphic buffers. Also, C2VdaBqBlockPool guaratees to fetch N fixed set of buffers with buffer
// identifier. So this class stores the mapping from slot index to buffers and the mapping from
// buffer unique ID to buffers.
// This class also implements functionalities for buffer migration when surface switching. Buffers
// are owned by either component (i.e. local buffers) or CCodec framework (i.e. remote buffers).
// When switching surface, the ccodec framework migrates remote buffers to the new surfaces. Then
// C2VdaBqBlockPool migrates local buffers. However, some buffers might be lost during migration.
// We assume that there are enough buffers migrated to the new surface to continue the playback.
// After |NUM_BUFFER_SLOTS| amount of buffers are dequeued from new surface, all buffers should
// be dequeued at least once. Then we treat the missing buffer as lost, and attach these bufers to
// the new surface.
class TrackedGraphicBuffers {
public:
    using value_type = std::tuple<slot_t, unique_id_t, std::shared_ptr<C2GraphicAllocation>>;

    TrackedGraphicBuffers() = default;
    ~TrackedGraphicBuffers() = default;

    void reset() {
        mSlotId2GraphicBuffer.clear();
        mSlotId2PoolData.clear();
        mAllocationsRegistered.clear();
        mAllocationsToBeMigrated.clear();
        mMigrateLostBufferCounter = 0;
        mGenerationToBeMigrated = 0;
    }

    void registerUniqueId(unique_id_t uniqueId, std::shared_ptr<C2GraphicAllocation> allocation) {
        ALOGV("%s(uniqueId=%u)", __func__, uniqueId);
        ALOG_ASSERT(allocation != nullptr);

        mAllocationsRegistered[uniqueId] = std::move(allocation);
    }

    std::shared_ptr<C2GraphicAllocation> getRegisteredAllocation(unique_id_t uniqueId) {
        const auto iter = mAllocationsRegistered.find(uniqueId);
        ALOG_ASSERT(iter != mAllocationsRegistered.end());

        return iter->second;
    }

    bool hasUniqueId(unique_id_t uniqueId) const {
        return mAllocationsRegistered.find(uniqueId) != mAllocationsRegistered.end() ||
               mAllocationsToBeMigrated.find(uniqueId) != mAllocationsToBeMigrated.end();
    }

    void updateSlotBuffer(slot_t slotId, unique_id_t uniqueId, sp<GraphicBuffer> slotBuffer) {
        ALOGV("%s(slotId=%d)", __func__, slotId);
        ALOG_ASSERT(slotBuffer != nullptr);

        mSlotId2GraphicBuffer[slotId] = std::make_pair(uniqueId, std::move(slotBuffer));
    }

    std::pair<unique_id_t, sp<GraphicBuffer>> getSlotBuffer(slot_t slotId) const {
        const auto iter = mSlotId2GraphicBuffer.find(slotId);
        ALOG_ASSERT(iter != mSlotId2GraphicBuffer.end());

        return iter->second;
    }

    bool hasSlotId(slot_t slotId) const {
        return mSlotId2GraphicBuffer.find(slotId) != mSlotId2GraphicBuffer.end();
    }

    void updatePoolData(slot_t slotId, std::weak_ptr<C2BufferQueueBlockPoolData> poolData) {
        ALOGV("%s(slotId=%d)", __func__, slotId);
        ALOG_ASSERT(hasSlotId(slotId));

        mSlotId2PoolData[slotId] = std::move(poolData);
    }

    bool migrateLocalBuffers(H2BGraphicBufferProducer* const producer, uint64_t producerId,
                             uint32_t generation, uint64_t usage) {
        ALOGV("%s(producerId=%" PRIx64 ", generation=%u, usage=%" PRIx64 ")", __func__, producerId,
              generation, usage);

        mGenerationToBeMigrated = generation;
        mUsageToBeMigrated = usage;

        // Move all buffers to mAllocationsToBeMigrated.
        for (auto& pair : mAllocationsRegistered) {
            if (!mAllocationsToBeMigrated.insert(pair).second) {
                ALOGE("%s() duplicated uniqueId=%u", __func__, pair.first);
                return false;
            }
        }
        mAllocationsRegistered.clear();

        ALOGV("%s(producerId=%" PRIx64 ", generation=%u, usage=%" PRIx64 ") before %s", __func__,
              producerId, generation, usage, debugString().c_str());

        // Migrate local buffers.
        std::map<slot_t, std::pair<unique_id_t, sp<GraphicBuffer>>> newSlotId2GraphicBuffer;
        std::map<slot_t, std::weak_ptr<C2BufferQueueBlockPoolData>> newSlotId2PoolData;
        for (const auto& pair : mSlotId2PoolData) {
            auto oldSlot = pair.first;
            auto poolData = pair.second.lock();
            if (!poolData) {
                continue;
            }

            unique_id_t uniqueId;
            sp<GraphicBuffer> slotBuffer;
            std::shared_ptr<C2SurfaceSyncMemory> syncMem;
            std::tie(uniqueId, slotBuffer) = getSlotBuffer(oldSlot);
            slot_t newSlot = poolData->migrate(producer->getBase(), mGenerationToBeMigrated,
                                               mUsageToBeMigrated, producerId, slotBuffer,
                                               slotBuffer->getGenerationNumber(),
                                               syncMem);
            if (newSlot < 0) {
                ALOGW("%s() Failed to migrate local buffer: uniqueId=%u, oldSlot=%d", __func__,
                      uniqueId, oldSlot);
                continue;
            }

            ALOGV("%s() migrated buffer: uniqueId=%u, oldSlot=%d, newSlot=%d", __func__, uniqueId,
                  oldSlot, newSlot);
            newSlotId2GraphicBuffer[newSlot] = std::make_pair(uniqueId, std::move(slotBuffer));
            newSlotId2PoolData[newSlot] = std::move(poolData);

            if (!moveBufferToRegistered(uniqueId)) {
                ALOGE("%s() failed to move buffer to registered, uniqueId=%u", __func__, uniqueId);
                return false;
            }
        }
        mSlotId2GraphicBuffer = std::move(newSlotId2GraphicBuffer);
        mSlotId2PoolData = std::move(newSlotId2PoolData);

        // Choose a big enough number to ensure all buffer should be dequeued at least once.
        mMigrateLostBufferCounter = NUM_BUFFER_SLOTS;
        ALOGD("%s() migrated %zu local buffers", __func__, mAllocationsRegistered.size());
        return true;
    }

    bool needMigrateLostBuffers() const {
        return mMigrateLostBufferCounter == 0 && !mAllocationsToBeMigrated.empty();
    }

    status_t migrateLostBuffer(C2Allocator* const allocator,
                               H2BGraphicBufferProducer* const producer, const uint64_t producerId,
                               slot_t* newSlot) {
        ALOGV("%s() %s", __func__, debugString().c_str());

        if (!needMigrateLostBuffers()) {
            return NO_INIT;
        }

        auto iter = mAllocationsToBeMigrated.begin();
        const unique_id_t uniqueId = iter->first;
        const C2Handle* c2Handle = iter->second->handle();

        // Convert C2GraphicAllocation to GraphicBuffer, and update generation and usage.
        uint32_t width, height, format, stride, igbpSlot, generation;
        uint64_t usage, igbpId;
        _UnwrapNativeCodec2GrallocMetadata(c2Handle, &width, &height, &format, &usage, &stride,
                                           &generation, &igbpId, &igbpSlot);
        native_handle_t* grallocHandle = UnwrapNativeCodec2GrallocHandle(c2Handle);
        sp<GraphicBuffer> graphicBuffer =
                new GraphicBuffer(grallocHandle, GraphicBuffer::CLONE_HANDLE, width, height, format,
                                  1, mUsageToBeMigrated, stride);
        native_handle_delete(grallocHandle);
        if (graphicBuffer->initCheck() != android::NO_ERROR) {
            ALOGE("Failed to create GraphicBuffer: %d", graphicBuffer->initCheck());
            return false;
        }
        graphicBuffer->setGenerationNumber(mGenerationToBeMigrated);

        // Attach GraphicBuffer to producer.
        const auto attachStatus = producer->attachBuffer(graphicBuffer, newSlot);
        if (attachStatus == TIMED_OUT || attachStatus == INVALID_OPERATION) {
            ALOGV("%s(): No free slot yet.", __func__);
            return TIMED_OUT;
        }
        if (attachStatus != OK) {
            ALOGE("%s(): Failed to attach buffer to new producer: %d", __func__, attachStatus);
            return attachStatus;
        }
        ALOGD("%s(), migrated lost buffer uniqueId=%u to slot=%d", __func__, uniqueId, *newSlot);
        updateSlotBuffer(*newSlot, uniqueId, graphicBuffer);

        // Wrap the new GraphicBuffer to C2GraphicAllocation and register it.
        std::shared_ptr<C2GraphicAllocation> allocation =
                ConvertGraphicBuffer2C2Allocation(graphicBuffer, producerId, *newSlot, allocator);
        if (!allocation) {
            return UNKNOWN_ERROR;
        }
        registerUniqueId(uniqueId, std::move(allocation));

        // Note: C2ArcProtectedGraphicAllocator releases the protected buffers if all the
        // corrresponding C2GraphicAllocations are released. To prevent the protected buffer is
        // released and then allocated again, we release the old C2GraphicAllocation after the new
        // one has been created.
        mAllocationsToBeMigrated.erase(iter);

        return OK;
    }

    void onBufferDequeued(slot_t slotId) {
        ALOGV("%s(slotId=%d)", __func__, slotId);
        unique_id_t uniqueId;
        std::tie(uniqueId, std::ignore) = getSlotBuffer(slotId);

        moveBufferToRegistered(uniqueId);
        if (mMigrateLostBufferCounter > 0) {
            --mMigrateLostBufferCounter;
        }
    }

    size_t size() const { return mAllocationsRegistered.size() + mAllocationsToBeMigrated.size(); }

    std::string debugString() const {
        std::stringstream ss;
        ss << "tracked size: " << size() << std::endl;
        ss << "  registered uniqueIds: ";
        for (const auto& pair : mAllocationsRegistered) {
            ss << pair.first << ", ";
        }
        ss << std::endl;
        ss << "  to-be-migrated uniqueIds: ";
        for (const auto& pair : mAllocationsToBeMigrated) {
            ss << pair.first << ", ";
        }
        ss << std::endl;
        ss << "  Count down for lost buffer migration: " << mMigrateLostBufferCounter;
        return ss.str();
    }

private:
    bool moveBufferToRegistered(unique_id_t uniqueId) {
        ALOGV("%s(uniqueId=%u)", __func__, uniqueId);
        auto iter = mAllocationsToBeMigrated.find(uniqueId);
        if (iter == mAllocationsToBeMigrated.end()) {
            return false;
        }
        if (!mAllocationsRegistered.insert(*iter).second) {
            ALOGE("%s() duplicated uniqueId=%u", __func__, uniqueId);
            return false;
        }
        mAllocationsToBeMigrated.erase(iter);

        return true;
    }

    // Mapping from IGBP slots to the corresponding graphic buffers.
    std::map<slot_t, std::pair<unique_id_t, sp<GraphicBuffer>>> mSlotId2GraphicBuffer;

    // Mapping from IGBP slots to the corresponding pool data.
    std::map<slot_t, std::weak_ptr<C2BufferQueueBlockPoolData>> mSlotId2PoolData;

    // Track the buffers registered at the current producer.
    std::map<unique_id_t, std::shared_ptr<C2GraphicAllocation>> mAllocationsRegistered;

    // Track the buffers that should be migrated to the current producer.
    std::map<unique_id_t, std::shared_ptr<C2GraphicAllocation>> mAllocationsToBeMigrated;

    // The counter for migrating lost buffers. Count down when a buffer is
    // dequeued from IGBP. When it goes to 0, then we treat the remaining
    // buffers at |mAllocationsToBeMigrated| lost, and migrate them to
    // current IGBP.
    size_t mMigrateLostBufferCounter = 0;

    // The generation and usage of the current IGBP, used to migrate buffers.
    uint32_t mGenerationToBeMigrated = 0;
    uint64_t mUsageToBeMigrated = 0;
};

class DrmHandleManager {
public:
    DrmHandleManager() { mRenderFd = openRenderFd(); }

    ~DrmHandleManager() {
        closeAllHandles();
        if (mRenderFd) {
            close(*mRenderFd);
        }
    }

    std::optional<unique_id_t> getHandle(int primeFd) {
        if (!mRenderFd) {
            return std::nullopt;
        }

        std::optional<unique_id_t> handle = getDrmHandle(*mRenderFd, primeFd);
        // Defer closing the handle until we don't need the buffer to keep the returned DRM handle
        // the same.
        if (handle) {
            mHandles.insert(*handle);
        }
        return handle;
    }

    void closeAllHandles() {
        if (!mRenderFd) {
            return;
        }

        for (const unique_id_t& handle : mHandles) {
            closeDrmHandle(*mRenderFd, handle);
        }
        mHandles.clear();
    }

private:
    std::optional<int> mRenderFd;
    std::set<unique_id_t> mHandles;
};

class C2VdaBqBlockPool::Impl : public std::enable_shared_from_this<C2VdaBqBlockPool::Impl>,
                               public EventNotifier::Listener {
public:
    using HGraphicBufferProducer = C2VdaBqBlockPool::HGraphicBufferProducer;

    explicit Impl(const std::shared_ptr<C2Allocator>& allocator);
    // TODO: should we detach buffers on producer if any on destructor?
    ~Impl() = default;

    // EventNotifier::Listener implementation.
    void onEventNotified() override;

    c2_status_t fetchGraphicBlock(uint32_t width, uint32_t height, uint32_t format,
                                  C2MemoryUsage usage,
                                  std::shared_ptr<C2GraphicBlock>* block /* nonnull */);
    void setRenderCallback(const C2BufferQueueBlockPool::OnRenderCallback& renderCallback);
    void configureProducer(const sp<HGraphicBufferProducer>& producer);
    c2_status_t requestNewBufferSet(int32_t bufferCount, uint32_t width, uint32_t height,
                                    uint32_t format, C2MemoryUsage usage);
    bool setNotifyBlockAvailableCb(::base::OnceClosure cb);
    std::optional<unique_id_t> getBufferIdFromGraphicBlock(const C2Block2D& block);

private:
    // Requested buffer formats.
    struct BufferFormat {
        BufferFormat(uint32_t width, uint32_t height, uint32_t pixelFormat,
                     C2AndroidMemoryUsage androidUsage)
              : mWidth(width), mHeight(height), mPixelFormat(pixelFormat), mUsage(androidUsage) {}
        BufferFormat() = default;

        uint32_t mWidth = 0;
        uint32_t mHeight = 0;
        uint32_t mPixelFormat = 0;
        C2AndroidMemoryUsage mUsage = C2MemoryUsage(0);
    };

    status_t getFreeSlotLocked(uint32_t width, uint32_t height, uint32_t format,
                               C2MemoryUsage usage, slot_t* slot, sp<Fence>* fence);

    // Queries the generation and usage flags from the given producer by dequeuing and requesting a
    // buffer (the buffer is then detached and freed).
    status_t queryGenerationAndUsageLocked(uint32_t width, uint32_t height, uint32_t pixelFormat,
                                           C2AndroidMemoryUsage androidUsage, uint32_t* generation,
                                           uint64_t* usage);

    // Wait the fence. If any error occurs, cancel the buffer back to the producer.
    status_t waitFence(slot_t slot, sp<Fence> fence);

    // Call mProducer's allowAllocation if needed.
    status_t allowAllocation(bool allow);

    const std::shared_ptr<C2Allocator> mAllocator;

    std::unique_ptr<H2BGraphicBufferProducer> mProducer;
    uint64_t mProducerId = 0;
    bool mAllowAllocation = false;

    C2BufferQueueBlockPool::OnRenderCallback mRenderCallback;

    // Function mutex to lock at the start of each API function call for protecting the
    // synchronization of all member variables.
    std::mutex mMutex;

    TrackedGraphicBuffers mTrackedGraphicBuffers;

    // We treat DRM handle as uniqueId of GraphicBuffer.
    DrmHandleManager mDrmHandleManager;

    // Number of buffers requested on requestNewBufferSet() call.
    size_t mBuffersRequested = 0u;
    // Currently requested buffer formats.
    BufferFormat mBufferFormat;

    // Listener for buffer release events.
    sp<EventNotifier> mFetchBufferNotifier;

    std::mutex mBufferReleaseMutex;
    // Set to true when the buffer release event is triggered after dequeueing buffer from IGBP
    // times out. Reset when fetching new slot times out, or |mNotifyBlockAvailableCb| is executed.
    bool mBufferReleasedAfterTimedOut GUARDED_BY(mBufferReleaseMutex) = false;
    // The callback to notify the caller the buffer is available.
    ::base::OnceClosure mNotifyBlockAvailableCb GUARDED_BY(mBufferReleaseMutex);

    // Set to true if any error occurs at previous configureProducer().
    bool mConfigureProducerError = false;
};

C2VdaBqBlockPool::Impl::Impl(const std::shared_ptr<C2Allocator>& allocator)
      : mAllocator(allocator) {}

c2_status_t C2VdaBqBlockPool::Impl::fetchGraphicBlock(
        uint32_t width, uint32_t height, uint32_t format, C2MemoryUsage usage,
        std::shared_ptr<C2GraphicBlock>* block /* nonnull */) {
    ALOGV("%s(%ux%u)", __func__, width, height);
    std::lock_guard<std::mutex> lock(mMutex);

    if (width != mBufferFormat.mWidth || height != mBufferFormat.mHeight ||
        format != mBufferFormat.mPixelFormat || usage.expected != mBufferFormat.mUsage.expected) {
        ALOGE("%s(): buffer format (%ux%u, format=%u, usage=%" PRIx64
              ") is different from requested format (%ux%u, format=%u, usage=%" PRIx64 ")",
              __func__, width, height, format, usage.expected, mBufferFormat.mWidth,
              mBufferFormat.mHeight, mBufferFormat.mPixelFormat, mBufferFormat.mUsage.expected);
        return C2_BAD_VALUE;
    }
    if (mConfigureProducerError || !mProducer) {
        ALOGE("%s(): error occurred at previous configureProducer()", __func__);
        return C2_CORRUPTED;
    }

    slot_t slot;
    sp<Fence> fence = new Fence();
    const auto status = getFreeSlotLocked(width, height, format, usage, &slot, &fence);
    if (status != OK) {
        return asC2Error(status);
    }

    unique_id_t uniqueId;
    sp<GraphicBuffer> slotBuffer;
    std::tie(uniqueId, slotBuffer) = mTrackedGraphicBuffers.getSlotBuffer(slot);
    ALOGV("%s(): dequeued slot=%d uniqueId=%u", __func__, slot, uniqueId);

    if (!mTrackedGraphicBuffers.hasUniqueId(uniqueId)) {
        if (mTrackedGraphicBuffers.size() >= mBuffersRequested) {
            // The dequeued slot has a pre-allocated buffer whose size and format is as same as
            // currently requested (but was not dequeued during allocation cycle). Just detach it to
            // free this slot. And try dequeueBuffer again.
            ALOGD("dequeued a new slot %d but already allocated enough buffers. Detach it.", slot);

            if (mProducer->detachBuffer(slot) != OK) {
                return C2_CORRUPTED;
            }

            const auto allocationStatus = allowAllocation(false);
            if (allocationStatus != OK) {
                return asC2Error(allocationStatus);
            }
            return C2_TIMED_OUT;
        }

        std::shared_ptr<C2GraphicAllocation> allocation =
                ConvertGraphicBuffer2C2Allocation(slotBuffer, mProducerId, slot, mAllocator.get());
        if (!allocation) {
            return C2_CORRUPTED;
        }
        mTrackedGraphicBuffers.registerUniqueId(uniqueId, std::move(allocation));

        ALOGV("%s(): mTrackedGraphicBuffers.size=%zu", __func__, mTrackedGraphicBuffers.size());
        if (mTrackedGraphicBuffers.size() == mBuffersRequested) {
            ALOGV("Tracked IGBP slots: %s", mTrackedGraphicBuffers.debugString().c_str());
            // Already allocated enough buffers, set allowAllocation to false to restrict the
            // eligible slots to allocated ones for future dequeue.
            const auto allocationStatus = allowAllocation(false);
            if (allocationStatus != OK) {
                return asC2Error(allocationStatus);
            }
        }
    }

    std::shared_ptr<C2SurfaceSyncMemory> syncMem;
    std::shared_ptr<C2GraphicAllocation> allocation =
            mTrackedGraphicBuffers.getRegisteredAllocation(uniqueId);
    auto poolData = std::make_shared<C2BufferQueueBlockPoolData>(
            slotBuffer->getGenerationNumber(), mProducerId, slot,
            mProducer->getBase(), syncMem, 0);
    mTrackedGraphicBuffers.updatePoolData(slot, poolData);
    *block = _C2BlockFactory::CreateGraphicBlock(std::move(allocation), std::move(poolData));
    if (*block == nullptr) {
        ALOGE("failed to create GraphicBlock: no memory");
        return C2_NO_MEMORY;
    }

    // Wait for acquire fence at the last point of returning buffer.
    if (fence) {
        const auto fenceStatus = waitFence(slot, fence);
        if (fenceStatus != OK) {
            return asC2Error(fenceStatus);
        }

        if (mRenderCallback) {
            nsecs_t signalTime = fence->getSignalTime();
            if (signalTime >= 0 && signalTime < INT64_MAX) {
                mRenderCallback(mProducerId, slot, signalTime);
            } else {
                ALOGV("got fence signal time of %" PRId64 " nsec", signalTime);
            }
        }
    }

    return C2_OK;
}

status_t C2VdaBqBlockPool::Impl::getFreeSlotLocked(uint32_t width, uint32_t height, uint32_t format,
                                                   C2MemoryUsage usage, slot_t* slot,
                                                   sp<Fence>* fence) {
    if (mTrackedGraphicBuffers.needMigrateLostBuffers()) {
        slot_t newSlot;
        if (mTrackedGraphicBuffers.migrateLostBuffer(mAllocator.get(), mProducer.get(), mProducerId,
                                                     &newSlot) == OK) {
            ALOGV("%s(): migrated buffer: slot=%d", __func__, newSlot);
            *slot = newSlot;
            return OK;
        }
    }

    // Dequeue a free slot from IGBP.
    ALOGV("%s(): try to dequeue free slot from IGBP.", __func__);
    const auto dequeueStatus = mProducer->dequeueBuffer(width, height, format, usage, slot, fence);
    if (dequeueStatus == TIMED_OUT) {
        std::lock_guard<std::mutex> lock(mBufferReleaseMutex);
        mBufferReleasedAfterTimedOut = false;
    }
    if (dequeueStatus != OK && dequeueStatus != BUFFER_NEEDS_REALLOCATION) {
        return dequeueStatus;
    }

    // Call requestBuffer to update GraphicBuffer for the slot and obtain the reference.
    if (!mTrackedGraphicBuffers.hasSlotId(*slot) || dequeueStatus == BUFFER_NEEDS_REALLOCATION) {
        sp<GraphicBuffer> slotBuffer = new GraphicBuffer();
        const auto requestStatus = mProducer->requestBuffer(*slot, &slotBuffer);
        if (requestStatus != OK) {
            mProducer->cancelBuffer(*slot, *fence);
            return requestStatus;
        }

        const auto uniqueId = mDrmHandleManager.getHandle(slotBuffer->handle->data[0]);
        if (!uniqueId) {
            ALOGE("%s(): failed to get uniqueId of GraphicBuffer from slot=%d", __func__, *slot);
            return UNKNOWN_ERROR;
        }
        mTrackedGraphicBuffers.updateSlotBuffer(*slot, *uniqueId, std::move(slotBuffer));
    }

    ALOGV("%s(%ux%u): dequeued slot=%d", __func__, mBufferFormat.mWidth, mBufferFormat.mHeight,
          *slot);
    mTrackedGraphicBuffers.onBufferDequeued(*slot);
    return OK;
}

void C2VdaBqBlockPool::Impl::onEventNotified() {
    ALOGV("%s()", __func__);
    ::base::OnceClosure outputCb;
    {
        std::lock_guard<std::mutex> lock(mBufferReleaseMutex);

        mBufferReleasedAfterTimedOut = true;
        if (mNotifyBlockAvailableCb) {
            mBufferReleasedAfterTimedOut = false;
            outputCb = std::move(mNotifyBlockAvailableCb);
        }
    }

    // Calling the callback outside the lock to avoid the deadlock.
    if (outputCb) {
        std::move(outputCb).Run();
    }
}

status_t C2VdaBqBlockPool::Impl::queryGenerationAndUsageLocked(uint32_t width, uint32_t height,
                                                               uint32_t pixelFormat,
                                                               C2AndroidMemoryUsage androidUsage,
                                                               uint32_t* generation,
                                                               uint64_t* usage) {
    ALOGV("%s()", __func__);

    sp<Fence> fence = new Fence();
    slot_t slot;
    const auto dequeueStatus =
            mProducer->dequeueBuffer(width, height, pixelFormat, androidUsage, &slot, &fence);
    if (dequeueStatus != OK && dequeueStatus != BUFFER_NEEDS_REALLOCATION) {
        return dequeueStatus;
    }

    // Call requestBuffer to allocate buffer for the slot and obtain the reference.
    // Get generation number here.
    sp<GraphicBuffer> slotBuffer = new GraphicBuffer();
    const auto requestStatus = mProducer->requestBuffer(slot, &slotBuffer);

    // Detach and delete the temporary buffer.
    const auto detachStatus = mProducer->detachBuffer(slot);
    if (detachStatus != OK) {
        return detachStatus;
    }

    // Check requestBuffer return flag.
    if (requestStatus != OK) {
        return requestStatus;
    }

    // Get generation number and usage from the slot buffer.
    *usage = slotBuffer->getUsage();
    *generation = slotBuffer->getGenerationNumber();
    ALOGV("Obtained from temp buffer: generation = %u, usage = %" PRIu64 "", *generation, *usage);
    return OK;
}

status_t C2VdaBqBlockPool::Impl::waitFence(slot_t slot, sp<Fence> fence) {
    const auto fenceStatus = fence->wait(kFenceWaitTimeMs);
    if (fenceStatus == OK) {
        return OK;
    }

    const auto cancelStatus = mProducer->cancelBuffer(slot, fence);
    if (cancelStatus != OK) {
        ALOGE("%s(): failed to cancelBuffer(slot=%d)", __func__, slot);
        return cancelStatus;
    }

    if (fenceStatus == -ETIME) {  // fence wait timed out
        ALOGV("%s(): buffer (slot=%d) fence wait timed out", __func__, slot);
        return TIMED_OUT;
    }
    ALOGE("buffer fence wait error: %d", fenceStatus);
    return fenceStatus;
}

void C2VdaBqBlockPool::Impl::setRenderCallback(
        const C2BufferQueueBlockPool::OnRenderCallback& renderCallback) {
    ALOGV("setRenderCallback");
    std::lock_guard<std::mutex> lock(mMutex);
    mRenderCallback = renderCallback;
}

c2_status_t C2VdaBqBlockPool::Impl::requestNewBufferSet(int32_t bufferCount, uint32_t width,
                                                        uint32_t height, uint32_t format,
                                                        C2MemoryUsage usage) {
    ALOGV("%s(bufferCount=%d, size=%ux%u, format=0x%x, usage=%" PRIu64 ")", __func__, bufferCount,
          width, height, format, usage.expected);

    if (bufferCount <= 0) {
        ALOGE("Invalid requested buffer count = %d", bufferCount);
        return C2_BAD_VALUE;
    }

    std::lock_guard<std::mutex> lock(mMutex);
    if (!mProducer) {
        ALOGD("No HGraphicBufferProducer is configured...");
        return C2_NO_INIT;
    }
    if (mBuffersRequested == static_cast<size_t>(bufferCount) && mBufferFormat.mWidth == width &&
        mBufferFormat.mHeight == height && mBufferFormat.mPixelFormat == format &&
        mBufferFormat.mUsage.expected == usage.expected) {
        ALOGD("%s() Request the same format and amount of buffers, skip", __func__);
        return C2_OK;
    }

    const auto status = allowAllocation(true);
    if (status != OK) {
        return asC2Error(status);
    }

    // Release all remained slot buffer references here. CCodec should either cancel or queue its
    // owned buffers from this set before the next resolution change.
    mTrackedGraphicBuffers.reset();
    mDrmHandleManager.closeAllHandles();

    mBuffersRequested = static_cast<size_t>(bufferCount);

    // Store buffer formats for future usage.
    mBufferFormat = BufferFormat(width, height, format, C2AndroidMemoryUsage(usage));

    return C2_OK;
}

void C2VdaBqBlockPool::Impl::configureProducer(const sp<HGraphicBufferProducer>& producer) {
    ALOGV("%s(producer=%p)", __func__, producer.get());

    std::lock_guard<std::mutex> lock(mMutex);
    if (producer == nullptr) {
        ALOGI("input producer is nullptr...");

        mProducer = nullptr;
        mProducerId = 0;
        mTrackedGraphicBuffers.reset();
        mDrmHandleManager.closeAllHandles();
        return;
    }

    auto newProducer = std::make_unique<H2BGraphicBufferProducer>(producer);
    uint64_t newProducerId;
    if (newProducer->getUniqueId(&newProducerId) != OK) {
        ALOGE("%s(): failed to get IGBP ID", __func__);
        mConfigureProducerError = true;
        return;
    }
    if (newProducerId == mProducerId) {
        ALOGI("%s(): configure the same producer, ignore", __func__);
        return;
    }

    ALOGI("Producer (Surface) is going to switch... ( 0x%" PRIx64 " -> 0x%" PRIx64 " )",
          mProducerId, newProducerId);
    mProducer = std::move(newProducer);
    mProducerId = newProducerId;
    mConfigureProducerError = false;
    mAllowAllocation = false;

    // Set allowAllocation to new producer.
    if (allowAllocation(true) != OK) {
        ALOGE("%s(): failed to allowAllocation(true)", __func__);
        mConfigureProducerError = true;
        return;
    }
    if (mProducer->setDequeueTimeout(0) != OK) {
        ALOGE("%s(): failed to setDequeueTimeout(0)", __func__);
        mConfigureProducerError = true;
        return;
    }
    if (mProducer->setMaxDequeuedBufferCount(kMaxDequeuedBufferCount) != OK) {
        ALOGE("%s(): failed to setMaxDequeuedBufferCount(%d)", __func__, kMaxDequeuedBufferCount);
        mConfigureProducerError = true;
        return;
    }

    // Migrate existing buffers to the new producer.
    if (mTrackedGraphicBuffers.size() > 0) {
        uint32_t newGeneration = 0;
        uint64_t newUsage = 0;
        const status_t err = queryGenerationAndUsageLocked(
                mBufferFormat.mWidth, mBufferFormat.mHeight, mBufferFormat.mPixelFormat,
                mBufferFormat.mUsage, &newGeneration, &newUsage);
        if (err != OK) {
            ALOGE("failed to query generation and usage: %d", err);
            mConfigureProducerError = true;
            return;
        }

        if (!mTrackedGraphicBuffers.migrateLocalBuffers(mProducer.get(), mProducerId, newGeneration,
                                                        newUsage)) {
            ALOGE("%s(): failed to migrateLocalBuffers()", __func__);
            mConfigureProducerError = true;
            return;
        }

        if (mTrackedGraphicBuffers.size() == mBuffersRequested) {
            if (allowAllocation(false) != OK) {
                ALOGE("%s(): failed to allowAllocation(false)", __func__);
                mConfigureProducerError = true;
                return;
            }
        }
    }

    // hack(b/146409777): Try to connect ARC-specific listener first.
    sp<BufferReleasedNotifier> listener = new BufferReleasedNotifier(weak_from_this());
    if (mProducer->connect(listener, 'ARC\0', false) == OK) {
        ALOGI("connected to ARC-specific IGBP listener.");
        mFetchBufferNotifier = listener;
    }

    // There might be free buffers at the new producer, notify the client if needed.
    onEventNotified();
}

bool C2VdaBqBlockPool::Impl::setNotifyBlockAvailableCb(::base::OnceClosure cb) {
    ALOGV("%s()", __func__);
    if (mFetchBufferNotifier == nullptr) {
        return false;
    }

    ::base::OnceClosure outputCb;
    {
        std::lock_guard<std::mutex> lock(mBufferReleaseMutex);

        // If there is any buffer released after dequeueBuffer() timed out, then we could notify the
        // caller directly.
        if (mBufferReleasedAfterTimedOut) {
            mBufferReleasedAfterTimedOut = false;
            outputCb = std::move(cb);
        } else {
            mNotifyBlockAvailableCb = std::move(cb);
        }
    }

    // Calling the callback outside the lock to avoid the deadlock.
    if (outputCb) {
        std::move(outputCb).Run();
    }
    return true;
}

std::optional<unique_id_t> C2VdaBqBlockPool::Impl::getBufferIdFromGraphicBlock(
        const C2Block2D& block) {
    return mDrmHandleManager.getHandle(block.handle()->data[0]);
}

status_t C2VdaBqBlockPool::Impl::allowAllocation(bool allow) {
    ALOGV("%s(%d)", __func__, allow);

    if (!mProducer) {
        ALOGW("%s() mProducer is not initiailzed", __func__);
        return NO_INIT;
    }
    if (mAllowAllocation == allow) {
        return OK;
    }

    const auto status = mProducer->allowAllocation(allow);
    if (status == OK) {
        mAllowAllocation = allow;
    }
    return status;
}

C2VdaBqBlockPool::C2VdaBqBlockPool(const std::shared_ptr<C2Allocator>& allocator,
                                   const local_id_t localId)
      : C2BufferQueueBlockPool(allocator, localId), mLocalId(localId), mImpl(new Impl(allocator)) {}

c2_status_t C2VdaBqBlockPool::fetchGraphicBlock(
        uint32_t width, uint32_t height, uint32_t format, C2MemoryUsage usage,
        std::shared_ptr<C2GraphicBlock>* block /* nonnull */) {
    if (mImpl) {
        return mImpl->fetchGraphicBlock(width, height, format, usage, block);
    }
    return C2_NO_INIT;
}

void C2VdaBqBlockPool::setRenderCallback(
        const C2BufferQueueBlockPool::OnRenderCallback& renderCallback) {
    if (mImpl) {
        mImpl->setRenderCallback(renderCallback);
    }
}

c2_status_t C2VdaBqBlockPool::requestNewBufferSet(int32_t bufferCount, uint32_t width,
                                                  uint32_t height, uint32_t format,
                                                  C2MemoryUsage usage) {
    if (mImpl) {
        return mImpl->requestNewBufferSet(bufferCount, width, height, format, usage);
    }
    return C2_NO_INIT;
}

void C2VdaBqBlockPool::configureProducer(const sp<HGraphicBufferProducer>& producer) {
    if (mImpl) {
        mImpl->configureProducer(producer);
    }
}

bool C2VdaBqBlockPool::setNotifyBlockAvailableCb(::base::OnceClosure cb) {
    if (mImpl) {
        return mImpl->setNotifyBlockAvailableCb(std::move(cb));
    }
    return false;
}

std::optional<unique_id_t> C2VdaBqBlockPool::getBufferIdFromGraphicBlock(const C2Block2D& block) {
    if (mImpl) {
        return mImpl->getBufferIdFromGraphicBlock(block);
    }
    return std::nullopt;
}

}  // namespace android
