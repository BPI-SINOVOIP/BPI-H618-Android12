// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//#define LOG_NDEBUG 0
#define LOG_TAG "V4L2EncodeComponent"

#include <v4l2_codec2/components/V4L2EncodeComponent.h>

#include <inttypes.h>

#include <algorithm>
#include <utility>

#include <C2AllocatorGralloc.h>
#include <C2PlatformSupport.h>
#include <C2Work.h>
#include <android/hardware/graphics/common/1.0/types.h>
#include <base/bind.h>
#include <base/bind_helpers.h>
#include <cutils/properties.h>
#include <log/log.h>
#include <media/stagefright/MediaDefs.h>
#include <ui/GraphicBuffer.h>
#include <ui/Size.h>

#include <v4l2_codec2/common/Common.h>
#include <v4l2_codec2/common/EncodeHelpers.h>
#include <v4l2_codec2/common/FormatConverter.h>
#include <v4l2_codec2/common/VideoPixelFormat.h>
#include <v4l2_codec2/components/BitstreamBuffer.h>
#include <v4l2_codec2/components/V4L2EncodeInterface.h>
#include <v4l2_codec2/components/V4L2Encoder.h>

using android::hardware::graphics::common::V1_0::BufferUsage;

namespace android {

namespace {

const VideoPixelFormat kInputPixelFormat = VideoPixelFormat::NV12;

// Get the video frame layout from the specified |inputBlock|.
// TODO(dstaessens): Clean up code extracting layout from a C2GraphicBlock.
std::optional<std::vector<VideoFramePlane>> getVideoFrameLayout(const C2ConstGraphicBlock& block,
                                                                VideoPixelFormat* format) {
    ALOGV("%s()", __func__);

    // Get the C2PlanarLayout from the graphics block. The C2GraphicView returned by block.map()
    // needs to be released before calling getGraphicBlockInfo(), or the lockYCbCr() call will block
    // Indefinitely.
    C2PlanarLayout layout = block.map().get().layout();

    // The above layout() cannot fill layout information and memset 0 instead if the input format is
    // IMPLEMENTATION_DEFINED and its backed format is RGB. We fill the layout by using
    // ImplDefinedToRGBXMap in the case.
    if (layout.type == C2PlanarLayout::TYPE_UNKNOWN) {
        std::unique_ptr<ImplDefinedToRGBXMap> idMap = ImplDefinedToRGBXMap::Create(block);
        if (idMap == nullptr) {
            ALOGE("Unable to parse RGBX_8888 from IMPLEMENTATION_DEFINED");
            return std::nullopt;
        }
        layout.type = C2PlanarLayout::TYPE_RGB;
        // These parameters would be used in TYPE_GRB case below.
        layout.numPlanes = 3;   // same value as in C2AllocationGralloc::map()
        layout.rootPlanes = 1;  // same value as in C2AllocationGralloc::map()
        layout.planes[C2PlanarLayout::PLANE_R].offset = idMap->offset();
        layout.planes[C2PlanarLayout::PLANE_R].rowInc = idMap->rowInc();
    }

    std::vector<uint32_t> offsets(layout.numPlanes, 0u);
    std::vector<uint32_t> strides(layout.numPlanes, 0u);
    switch (layout.type) {
    case C2PlanarLayout::TYPE_YUV: {
        android_ycbcr ycbcr = getGraphicBlockInfo(block);
        offsets[C2PlanarLayout::PLANE_Y] =
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ycbcr.y));
        offsets[C2PlanarLayout::PLANE_U] =
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ycbcr.cb));
        offsets[C2PlanarLayout::PLANE_V] =
                static_cast<uint32_t>(reinterpret_cast<uintptr_t>(ycbcr.cr));
        strides[C2PlanarLayout::PLANE_Y] = static_cast<uint32_t>(ycbcr.ystride);
        strides[C2PlanarLayout::PLANE_U] = static_cast<uint32_t>(ycbcr.cstride);
        strides[C2PlanarLayout::PLANE_V] = static_cast<uint32_t>(ycbcr.cstride);

        bool crcb = false;
        if (offsets[C2PlanarLayout::PLANE_U] > offsets[C2PlanarLayout::PLANE_V]) {
            // Swap offsets, no need to swap strides as they are identical for both chroma planes.
            std::swap(offsets[C2PlanarLayout::PLANE_U], offsets[C2PlanarLayout::PLANE_V]);
            crcb = true;
        }

        bool semiplanar = false;
        if (ycbcr.chroma_step >
            offsets[C2PlanarLayout::PLANE_V] - offsets[C2PlanarLayout::PLANE_U]) {
            semiplanar = true;
        }

        if (!crcb && !semiplanar) {
            *format = VideoPixelFormat::I420;
        } else if (!crcb && semiplanar) {
            *format = VideoPixelFormat::NV12;
        } else if (crcb && !semiplanar) {
            // HACK: pretend YV12 is I420 now since VEA only accepts I420. (YV12 will be used
            //       for input byte-buffer mode).
            // TODO(dstaessens): Is this hack still necessary now we're not using the VEA directly?
            //format = VideoPixelFormat::YV12;
            *format = VideoPixelFormat::I420;
        } else {
            *format = VideoPixelFormat::NV21;
        }
        break;
    }
    case C2PlanarLayout::TYPE_RGB: {
        offsets[C2PlanarLayout::PLANE_R] = layout.planes[C2PlanarLayout::PLANE_R].offset;
        strides[C2PlanarLayout::PLANE_R] =
                static_cast<uint32_t>(layout.planes[C2PlanarLayout::PLANE_R].rowInc);
        *format = VideoPixelFormat::ARGB;
        break;
    }
    default:
        ALOGW("Unknown layout type: %u", static_cast<uint32_t>(layout.type));
        return std::nullopt;
    }

    std::vector<VideoFramePlane> planes;
    for (uint32_t i = 0; i < layout.rootPlanes; ++i) {
        // The mSize field is not used in our case, so we can safely set it to zero.
        planes.push_back({strides[i], offsets[i], 0});
    }
    return planes;
}

// Get the video frame stride for the specified |format| and |size|.
std::optional<uint32_t> getVideoFrameStride(VideoPixelFormat format, ui::Size size) {
    // Fetch a graphic block from the pool to determine the stride.
    std::shared_ptr<C2BlockPool> pool;
    c2_status_t status = GetCodec2BlockPool(C2BlockPool::BASIC_GRAPHIC, nullptr, &pool);
    if (status != C2_OK) {
        ALOGE("Failed to get basic graphic block pool (err=%d)", status);
        return std::nullopt;
    }

    // Android HAL format doesn't have I420, we use YV12 instead and swap the U and V planes when
    // converting to NV12. YCBCR_420_888 will allocate NV12 by minigbm.
    HalPixelFormat halFormat = (format == VideoPixelFormat::I420) ? HalPixelFormat::YV12
                                                                  : HalPixelFormat::YCBCR_420_888;

    std::shared_ptr<C2GraphicBlock> block;
    status = pool->fetchGraphicBlock(size.width, size.height, static_cast<uint32_t>(halFormat),
                                     C2MemoryUsage(C2MemoryUsage::CPU_READ), &block);
    if (status != C2_OK) {
        ALOGE("Failed to fetch graphic block (err=%d)", status);
        return std::nullopt;
    }

    const C2ConstGraphicBlock constBlock = block->share(C2Rect(size.width, size.height), C2Fence());
    VideoPixelFormat pixelFormat;
    std::optional<std::vector<VideoFramePlane>> planes =
            getVideoFrameLayout(constBlock, &pixelFormat);
    if (!planes || planes.value().empty()) {
        ALOGE("Failed to get video frame layout from block");
        return std::nullopt;
    }

    return planes.value()[0].mStride;
}

// Create an input frame from the specified graphic block.
std::unique_ptr<V4L2Encoder::InputFrame> CreateInputFrame(const C2ConstGraphicBlock& block,
                                                          uint64_t index, int64_t timestamp) {
    VideoPixelFormat format;
    std::optional<std::vector<VideoFramePlane>> planes = getVideoFrameLayout(block, &format);
    if (!planes) {
        ALOGE("Failed to get input block's layout");
        return nullptr;
    }

    std::vector<int> fds;
    const C2Handle* const handle = block.handle();
    for (int i = 0; i < handle->numFds; i++) {
        fds.emplace_back(handle->data[i]);
    }

    return std::make_unique<V4L2Encoder::InputFrame>(std::move(fds), std::move(planes.value()),
                                                     format, index, timestamp);
}

}  // namespace

// static
std::atomic<int32_t> V4L2EncodeComponent::sConcurrentInstances = 0;

// static
std::shared_ptr<C2Component> V4L2EncodeComponent::create(
        C2String name, c2_node_id_t id, std::shared_ptr<C2ReflectorHelper> helper,
        C2ComponentFactory::ComponentDeleter deleter) {
    ALOGV("%s(%s)", __func__, name.c_str());

    static const int32_t kMaxConcurrentInstances =
            property_get_int32("debug.v4l2_codec2.encode.concurrent-instances", -1);

    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    if (kMaxConcurrentInstances >= 0 && sConcurrentInstances.load() >= kMaxConcurrentInstances) {
        ALOGW("Cannot create additional encoder, maximum number of instances reached: %d",
              kMaxConcurrentInstances);
        return nullptr;
    }

    auto interface = std::make_shared<V4L2EncodeInterface>(name, std::move(helper));
    if (interface->status() != C2_OK) {
        ALOGE("Component interface initialization failed (error code %d)", interface->status());
        return nullptr;
    }

    return std::shared_ptr<C2Component>(new V4L2EncodeComponent(name, id, std::move(interface)),
                                        deleter);
}

V4L2EncodeComponent::V4L2EncodeComponent(C2String name, c2_node_id_t id,
                                         std::shared_ptr<V4L2EncodeInterface> interface)
      : mName(name),
        mId(id),
        mInterface(std::move(interface)),
        mComponentState(ComponentState::LOADED) {
    ALOGV("%s(%s)", __func__, name.c_str());

    sConcurrentInstances.fetch_add(1, std::memory_order_relaxed);
}

V4L2EncodeComponent::~V4L2EncodeComponent() {
    ALOGV("%s()", __func__);

    // Stop encoder thread and invalidate pointers if component wasn't stopped before destroying.
    if (mEncoderThread.IsRunning()) {
        mEncoderTaskRunner->PostTask(
                FROM_HERE, ::base::BindOnce(
                                   [](::base::WeakPtrFactory<V4L2EncodeComponent>* weakPtrFactory) {
                                       weakPtrFactory->InvalidateWeakPtrs();
                                   },
                                   &mWeakThisFactory));
        mEncoderThread.Stop();
    }

    sConcurrentInstances.fetch_sub(1, std::memory_order_relaxed);
    ALOGV("%s(): done", __func__);
}

c2_status_t V4L2EncodeComponent::start() {
    ALOGV("%s()", __func__);

    // Lock while starting, to synchronize start/stop/reset/release calls.
    std::lock_guard<std::mutex> lock(mComponentLock);

    // According to the specification start() should only be called in the LOADED state.
    if (mComponentState != ComponentState::LOADED) {
        return C2_BAD_STATE;
    }

    if (!mEncoderThread.Start()) {
        ALOGE("Failed to start encoder thread");
        return C2_CORRUPTED;
    }
    mEncoderTaskRunner = mEncoderThread.task_runner();
    mWeakThis = mWeakThisFactory.GetWeakPtr();

    // Initialize the encoder on the encoder thread.
    ::base::WaitableEvent done;
    bool success = false;
    mEncoderTaskRunner->PostTask(
            FROM_HERE, ::base::Bind(&V4L2EncodeComponent::startTask, mWeakThis, &success, &done));
    done.Wait();

    if (!success) {
        ALOGE("Failed to initialize encoder");
        return C2_CORRUPTED;
    }

    setComponentState(ComponentState::RUNNING);
    return C2_OK;
}

c2_status_t V4L2EncodeComponent::stop() {
    ALOGV("%s()", __func__);

    // Lock while stopping, to synchronize start/stop/reset/release calls.
    std::lock_guard<std::mutex> lock(mComponentLock);

    if (mComponentState != ComponentState::RUNNING && mComponentState != ComponentState::ERROR) {
        return C2_BAD_STATE;
    }

    // Return immediately if the component is already stopped.
    if (!mEncoderThread.IsRunning()) {
        return C2_OK;
    }

    // Wait for the component to stop.
    ::base::WaitableEvent done;
    mEncoderTaskRunner->PostTask(
            FROM_HERE, ::base::BindOnce(&V4L2EncodeComponent::stopTask, mWeakThis, &done));
    done.Wait();
    mEncoderThread.Stop();

    setComponentState(ComponentState::LOADED);

    ALOGV("%s() - done", __func__);
    return C2_OK;
}

c2_status_t V4L2EncodeComponent::reset() {
    ALOGV("%s()", __func__);

    // The interface specification says: "This method MUST be supported in all (including tripped)
    // states other than released".
    if (mComponentState == ComponentState::UNLOADED) {
        return C2_BAD_STATE;
    }

    // TODO(dstaessens): Reset the component's interface to default values.
    stop();

    return C2_OK;
}

c2_status_t V4L2EncodeComponent::release() {
    ALOGV("%s()", __func__);

    // The interface specification says: "This method MUST be supported in stopped state.", but the
    // release method seems to be called in other states as well.
    reset();

    setComponentState(ComponentState::UNLOADED);
    return C2_OK;
}

c2_status_t V4L2EncodeComponent::queue_nb(std::list<std::unique_ptr<C2Work>>* const items) {
    ALOGV("%s()", __func__);

    if (mComponentState != ComponentState::RUNNING) {
        ALOGE("Trying to queue work item while component is not running");
        return C2_BAD_STATE;
    }

    while (!items->empty()) {
        mEncoderTaskRunner->PostTask(FROM_HERE,
                                     ::base::BindOnce(&V4L2EncodeComponent::queueTask, mWeakThis,
                                                      std::move(items->front())));
        items->pop_front();
    }

    return C2_OK;
}

c2_status_t V4L2EncodeComponent::drain_nb(drain_mode_t mode) {
    ALOGV("%s()", __func__);

    if (mode == DRAIN_CHAIN) {
        return C2_OMITTED;  // Tunneling is not supported for now.
    }

    if (mComponentState != ComponentState::RUNNING) {
        return C2_BAD_STATE;
    }

    mEncoderTaskRunner->PostTask(
            FROM_HERE, ::base::BindOnce(&V4L2EncodeComponent::drainTask, mWeakThis, mode));
    return C2_OK;
}

c2_status_t V4L2EncodeComponent::flush_sm(flush_mode_t mode,
                                          std::list<std::unique_ptr<C2Work>>* const flushedWork) {
    ALOGV("%s()", __func__);

    if (mode != FLUSH_COMPONENT) {
        return C2_OMITTED;  // Tunneling is not supported by now
    }

    if (mComponentState != ComponentState::RUNNING) {
        return C2_BAD_STATE;
    }

    // Work that can be immediately discarded should be returned in |flushedWork|. This method may
    // be momentarily blocking but must return within 5ms, which should give us enough time to
    // immediately abandon all non-started work on the encoder thread. We can return all work that
    // can't be immediately discarded using onWorkDone() later.
    ::base::WaitableEvent done;
    mEncoderTaskRunner->PostTask(FROM_HERE, ::base::BindOnce(&V4L2EncodeComponent::flushTask,
                                                             mWeakThis, &done, flushedWork));
    done.Wait();

    return C2_OK;
}

c2_status_t V4L2EncodeComponent::announce_nb(const std::vector<C2WorkOutline>& items) {
    return C2_OMITTED;  // Tunneling is not supported by now
}

c2_status_t V4L2EncodeComponent::setListener_vb(const std::shared_ptr<Listener>& listener,
                                                c2_blocking_t mayBlock) {
    ALOG_ASSERT(mComponentState != ComponentState::UNLOADED);

    // Lock so we're sure the component isn't currently starting or stopping.
    std::lock_guard<std::mutex> lock(mComponentLock);

    // If the encoder thread is not running it's safe to update the listener directly.
    if (!mEncoderThread.IsRunning()) {
        mListener = listener;
        return C2_OK;
    }

    // The listener should be updated before exiting this function. If called while the component is
    // currently running we should be allowed to block, as we can only change the listener on the
    // encoder thread.
    ALOG_ASSERT(mayBlock == c2_blocking_t::C2_MAY_BLOCK);

    ::base::WaitableEvent done;
    mEncoderTaskRunner->PostTask(FROM_HERE, ::base::BindOnce(&V4L2EncodeComponent::setListenerTask,
                                                             mWeakThis, listener, &done));
    done.Wait();

    return C2_OK;
}

std::shared_ptr<C2ComponentInterface> V4L2EncodeComponent::intf() {
    return std::make_shared<SimpleInterface<V4L2EncodeInterface>>(mName.c_str(), mId, mInterface);
}

void V4L2EncodeComponent::startTask(bool* success, ::base::WaitableEvent* done) {
    ALOGV("%s()", __func__);
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());

    *success = initializeEncoder();
    done->Signal();
}

void V4L2EncodeComponent::stopTask(::base::WaitableEvent* done) {
    ALOGV("%s()", __func__);
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());

    // Flushing the encoder will abort all pending work.
    flush();

    mInputFormatConverter.reset();

    mEncoder.reset();
    mOutputBlockPool.reset();

    // Invalidate all weak pointers so no more functions will be executed on the encoder thread.
    mWeakThisFactory.InvalidateWeakPtrs();

    done->Signal();
}

void V4L2EncodeComponent::queueTask(std::unique_ptr<C2Work> work) {
    ALOGV("%s()", __func__);
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());
    ALOG_ASSERT(mEncoder);

    // Currently only a single worklet per work item is supported. An input buffer should always be
    // supplied unless this is a drain or CSD request.
    ALOG_ASSERT(work->input.buffers.size() <= 1u && work->worklets.size() == 1u);

    // Set the default values for the output worklet.
    work->worklets.front()->output.flags = static_cast<C2FrameData::flags_t>(0);
    work->worklets.front()->output.buffers.clear();
    work->worklets.front()->output.ordinal = work->input.ordinal;

    uint64_t index = work->input.ordinal.frameIndex.peeku();
    int64_t timestamp = static_cast<int64_t>(work->input.ordinal.timestamp.peeku());
    bool endOfStream = work->input.flags & C2FrameData::FLAG_END_OF_STREAM;
    ALOGV("Queuing next encode (index: %" PRIu64 ", timestamp: %" PRId64 ", EOS: %d)", index,
          timestamp, endOfStream);

    // The codec 2.0 framework might queue an empty CSD request, but this is currently not
    // supported. We will return the CSD with the first encoded buffer work.
    if (work->input.buffers.empty() && !endOfStream) {
        ALOGV("Discarding empty CSD request");
        reportWork(std::move(work));
        return;
    }

    // By the time we get an input buffer, the output block pool should be configured.
    if (!mOutputBlockPool && !getBlockPool()) {
        reportError(C2_CORRUPTED);
        return;
    }

    // If conversion is required but no free buffers are available we queue the work item.
    if (mInputFormatConverter && !mInputFormatConverter->isReady()) {
        ALOGV("Input format convertor ran out of buffers");
        mInputConverterQueue.push(std::move(work));
        return;
    }

    // If we have data to encode send it to the encoder. If conversion is required we will first
    // convert the data to the requested pixel format.
    if (!work->input.buffers.empty()) {
        C2ConstGraphicBlock inputBlock =
                work->input.buffers.front()->data().graphicBlocks().front();
        if (mInputFormatConverter) {
            ALOGV("Converting input block (index: %" PRIu64 ")", index);
            c2_status_t status = C2_CORRUPTED;
            inputBlock = mInputFormatConverter->convertBlock(index, inputBlock, &status);
            if (status != C2_OK) {
                ALOGE("Failed to convert input block (index: %" PRIu64 ")", index);
                reportError(status);
                return;
            }
        }
        if (!encode(inputBlock, index, timestamp)) {
            return;
        }
    }

    mWorkQueue.push_back(std::move(work));
    if (endOfStream) {
        mEncoder->drain();
    }
}

void V4L2EncodeComponent::drainTask(drain_mode_t /*drainMode*/) {
    ALOGV("%s()", __func__);
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());

    // We can only start draining if all work has been queued in the encoder, so we mark the last
    // item waiting for conversion as EOS if required.
    if (!mInputConverterQueue.empty()) {
        C2Work* work = mInputConverterQueue.back().get();
        work->input.flags = static_cast<C2FrameData::flags_t>(work->input.flags |
                                                              C2FrameData::FLAG_END_OF_STREAM);
        return;
    }

    // Mark the last item in the output work queue as EOS, so we will only report it as finished
    // after draining has completed.
    if (!mWorkQueue.empty()) {
        ALOGV("Starting drain and marking last item in output work queue as EOS");
        C2Work* work = mWorkQueue.back().get();
        work->input.flags = static_cast<C2FrameData::flags_t>(work->input.flags |
                                                              C2FrameData::FLAG_END_OF_STREAM);
        mEncoder->drain();
    }
}

void V4L2EncodeComponent::onDrainDone(bool success) {
    ALOGV("%s()", __func__);
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());
    ALOG_ASSERT(!mWorkQueue.empty());

    if (!success) {
        ALOGE("draining the encoder failed");
        reportError(C2_CORRUPTED);
        return;
    }

    // Find the first work item marked as EOS. This might not be the first item in the queue, as
    // previous buffers in the queue might still be waiting for their associated input buffers.
    auto it = std::find_if(
            mWorkQueue.cbegin(), mWorkQueue.cend(), [](const std::unique_ptr<C2Work>& work) {
                return ((work->input.flags & C2FrameData::FLAG_END_OF_STREAM) &&
                        !(work->worklets.back()->output.flags & C2FrameData::FLAG_END_OF_STREAM));
            });
    if (it == mWorkQueue.end()) {
        ALOGW("No EOS work item found in queue");
        return;
    }

    // Mark the item in the output work queue as EOS done.
    C2Work* eosWork = it->get();
    eosWork->worklets.back()->output.flags = C2FrameData::FLAG_END_OF_STREAM;

    // Draining is done which means all buffers on the device output queue have been returned, but
    // not all buffers on the device input queue might have been returned yet.
    if ((eosWork != mWorkQueue.front().get()) || !isWorkDone(*eosWork)) {
        ALOGV("Draining done, waiting for input buffers to be returned");
        return;
    }

    ALOGV("Draining done");
    reportWork(std::move(mWorkQueue.front()));
    mWorkQueue.pop_front();
}

void V4L2EncodeComponent::flushTask(::base::WaitableEvent* done,
                                    std::list<std::unique_ptr<C2Work>>* const flushedWork) {
    ALOGV("%s()", __func__);
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());

    // Move all work that can immediately be aborted to flushedWork, and notify the caller.
    if (flushedWork) {
        while (!mInputConverterQueue.empty()) {
            std::unique_ptr<C2Work> work = std::move(mInputConverterQueue.front());
            work->input.buffers.clear();
            flushedWork->push_back(std::move(work));
            mInputConverterQueue.pop();
        }
    }
    done->Signal();

    flush();
}

void V4L2EncodeComponent::setListenerTask(const std::shared_ptr<Listener>& listener,
                                          ::base::WaitableEvent* done) {
    ALOGV("%s()", __func__);
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());

    mListener = listener;
    done->Signal();
}

bool V4L2EncodeComponent::initializeEncoder() {
    ALOGV("%s()", __func__);
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());
    ALOG_ASSERT(!mInputFormatConverter);
    ALOG_ASSERT(!mEncoder);

    mCSDSubmitted = false;

    // Get the requested profile and level.
    C2Config::profile_t outputProfile = mInterface->getOutputProfile();

    std::optional<uint8_t> h264Level;
    if (outputProfile >= C2Config::PROFILE_AVC_BASELINE &&
        outputProfile <= C2Config::PROFILE_AVC_ENHANCED_MULTIVIEW_DEPTH_HIGH) {
        h264Level = c2LevelToV4L2Level(mInterface->getOutputLevel());
    }

    // Get the stride used by the C2 framework, as this might be different from the stride used by
    // the V4L2 encoder.
    std::optional<uint32_t> stride =
            getVideoFrameStride(kInputPixelFormat, mInterface->getInputVisibleSize());
    if (!stride) {
        ALOGE("Failed to get video frame stride");
        reportError(C2_CORRUPTED);
        return false;
    }

    mEncoder = V4L2Encoder::create(
            outputProfile, h264Level, mInterface->getInputVisibleSize(), *stride,
            mInterface->getKeyFramePeriod(),
            ::base::BindRepeating(&V4L2EncodeComponent::fetchOutputBlock, mWeakThis),
            ::base::BindRepeating(&V4L2EncodeComponent::onInputBufferDone, mWeakThis),
            ::base::BindRepeating(&V4L2EncodeComponent::onOutputBufferDone, mWeakThis),
            ::base::BindRepeating(&V4L2EncodeComponent::onDrainDone, mWeakThis),
            ::base::BindRepeating(&V4L2EncodeComponent::reportError, mWeakThis, C2_CORRUPTED),
            mEncoderTaskRunner);
    if (!mEncoder) {
        ALOGE("Failed to create V4L2Encoder (profile: %s)", profileToString(outputProfile));
        return false;
    }

    // Add an input format convertor if the device doesn't support the requested input format.
    ALOGV("Creating input format convertor (%s)",
          videoPixelFormatToString(mEncoder->inputFormat()).c_str());
    mInputFormatConverter =
            FormatConverter::Create(mEncoder->inputFormat(), mEncoder->visibleSize(),
                                    V4L2Encoder::kInputBufferCount, mEncoder->codedSize());
    if (!mInputFormatConverter) {
        ALOGE("Failed to created input format convertor");
        return false;
    }

    return true;
}

bool V4L2EncodeComponent::updateEncodingParameters() {
    ALOGV("%s()", __func__);
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());

    // Query the interface for the encoding parameters requested by the codec 2.0 framework.
    C2StreamBitrateInfo::output bitrateInfo;
    C2StreamFrameRateInfo::output framerateInfo;
    c2_status_t status =
            mInterface->query({&bitrateInfo, &framerateInfo}, {}, C2_DONT_BLOCK, nullptr);
    if (status != C2_OK) {
        ALOGE("Failed to query interface for encoding parameters (error code: %d)", status);
        reportError(status);
        return false;
    }

    // Ask device to change bitrate if it's different from the currently configured bitrate.
    uint32_t bitrate = bitrateInfo.value;
    if (mBitrate != bitrate) {
        ALOG_ASSERT(bitrate > 0u);
        ALOGV("Setting bitrate to %u", bitrate);
        if (!mEncoder->setBitrate(bitrate)) {
            reportError(C2_CORRUPTED);
            return false;
        }
        mBitrate = bitrate;
    }

    // Ask device to change framerate if it's different from the currently configured framerate.
    uint32_t framerate = static_cast<uint32_t>(std::round(framerateInfo.value));
    if (mFramerate != framerate) {
        ALOG_ASSERT(framerate > 0u);
        ALOGV("Setting framerate to %u", framerate);
        if (!mEncoder->setFramerate(framerate)) {
            ALOGE("Requesting framerate change failed");
            reportError(C2_CORRUPTED);
            return false;
        }
        mFramerate = framerate;
    }

    // Check whether an explicit key frame was requested, if so reset the key frame counter to
    // immediately request a key frame.
    C2StreamRequestSyncFrameTuning::output requestKeyFrame;
    status = mInterface->query({&requestKeyFrame}, {}, C2_DONT_BLOCK, nullptr);
    if (status != C2_OK) {
        ALOGE("Failed to query interface for key frame request (error code: %d)", status);
        reportError(status);
        return false;
    }
    if (requestKeyFrame.value == C2_TRUE) {
        mEncoder->requestKeyframe();
        requestKeyFrame.value = C2_FALSE;
        std::vector<std::unique_ptr<C2SettingResult>> failures;
        status = mInterface->config({&requestKeyFrame}, C2_MAY_BLOCK, &failures);
        if (status != C2_OK) {
            ALOGE("Failed to reset key frame request on interface (error code: %d)", status);
            reportError(status);
            return false;
        }
    }

    return true;
}

bool V4L2EncodeComponent::encode(C2ConstGraphicBlock block, uint64_t index, int64_t timestamp) {
    ALOGV("%s()", __func__);
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());
    ALOG_ASSERT(mEncoder);

    ALOGV("Encoding input block (index: %" PRIu64 ", timestamp: %" PRId64 ", size: %dx%d)", index,
          timestamp, block.width(), block.height());

    // Update dynamic encoding parameters (bitrate, framerate, key frame) if requested.
    if (!updateEncodingParameters()) return false;

    // Create an input frame from the graphic block.
    std::unique_ptr<V4L2Encoder::InputFrame> frame = CreateInputFrame(block, index, timestamp);
    if (!frame) {
        ALOGE("Failed to create video frame from input block (index: %" PRIu64
              ", timestamp: %" PRId64 ")",
              index, timestamp);
        reportError(C2_CORRUPTED);
        return false;
    }

    if (!mEncoder->encode(std::move(frame))) {
        return false;
    }

    return true;
}

void V4L2EncodeComponent::flush() {
    ALOGV("%s()", __func__);
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());

    mEncoder->flush();

    // Report all queued work items as aborted.
    std::list<std::unique_ptr<C2Work>> abortedWorkItems;
    while (!mInputConverterQueue.empty()) {
        std::unique_ptr<C2Work> work = std::move(mInputConverterQueue.front());
        work->result = C2_NOT_FOUND;
        work->input.buffers.clear();
        abortedWorkItems.push_back(std::move(work));
        mInputConverterQueue.pop();
    }
    while (!mWorkQueue.empty()) {
        std::unique_ptr<C2Work> work = std::move(mWorkQueue.front());
        // Return buffer to the input format convertor if required.
        if (mInputFormatConverter && work->input.buffers.empty()) {
            mInputFormatConverter->returnBlock(work->input.ordinal.frameIndex.peeku());
        }
        work->result = C2_NOT_FOUND;
        work->input.buffers.clear();
        abortedWorkItems.push_back(std::move(work));
        mWorkQueue.pop_front();
    }
    if (!abortedWorkItems.empty()) {
        mListener->onWorkDone_nb(shared_from_this(), std::move(abortedWorkItems));
    }
}

void V4L2EncodeComponent::fetchOutputBlock(uint32_t size,
                                           std::unique_ptr<BitstreamBuffer>* buffer) {
    ALOGV("Fetching linear block (size: %u)", size);
    std::shared_ptr<C2LinearBlock> block;
    c2_status_t status = mOutputBlockPool->fetchLinearBlock(
            size,
            C2MemoryUsage(C2MemoryUsage::CPU_READ |
                          static_cast<uint64_t>(BufferUsage::VIDEO_ENCODER)),
            &block);
    if (status != C2_OK) {
        ALOGE("Failed to fetch linear block (error: %d)", status);
        reportError(status);
    }

    // Store a reference to the block to keep the fds alive.
    int fd = block->handle()->data[0];
    ALOG_ASSERT(!mOutputBuffersMap[fd]);
    mOutputBuffersMap[fd] = std::move(block);

    // TODO(dstaessens) Store the C2LinearBlock directly into the BitstreamBuffer.
    *buffer = std::make_unique<BitstreamBuffer>(fd, fd, 0, size);
}

void V4L2EncodeComponent::onInputBufferDone(uint64_t index) {
    ALOGV("%s(): Input buffer done (index: %" PRIu64 ")", __func__, index);
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());
    ALOG_ASSERT(mEncoder);

    // There are no guarantees the input buffers are returned in order, so we need to find the work
    // item which this buffer belongs to.
    C2Work* work = getWorkByIndex(index);
    if (!work) {
        ALOGE("Failed to find work associated with input buffer %" PRIu64, index);
        reportError(C2_CORRUPTED);
        return;
    }

    // We're done using the input block, release reference to return the block to the client.
    LOG_ASSERT(!work->input.buffers.empty());
    work->input.buffers.front().reset();

    // Return the block to the convertor if required. If we have buffers awaiting conversion, we can
    // now attempt to convert and encode them again.
    if (mInputFormatConverter) {
        c2_status_t status = mInputFormatConverter->returnBlock(index);
        if (status != C2_OK) {
            reportError(status);
            return;
        }
        while (!mInputConverterQueue.empty() && mInputFormatConverter->isReady()) {
            std::unique_ptr<C2Work> work = std::move(mInputConverterQueue.front());
            mInputConverterQueue.pop();
            queueTask(std::move(work));
        }
    }

    // Return all completed work items. The work item might have been waiting for it's input buffer
    // to be returned, in which case we can report it as completed now. As input buffers are not
    // necessarily returned in order we might be able to return multiple ready work items now.
    while (!mWorkQueue.empty() && isWorkDone(*mWorkQueue.front())) {
        reportWork(std::move(mWorkQueue.front()));
        mWorkQueue.pop_front();
    }
}

void V4L2EncodeComponent::onOutputBufferDone(size_t dataSize, int64_t timestamp, bool keyFrame,
                                             std::unique_ptr<BitstreamBuffer> buffer) {
    ALOGV("%s(): output buffer done (timestamp: %" PRId64 ", size: %zu, keyframe: %d)", __func__,
          timestamp, dataSize, keyFrame);
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());

    std::shared_ptr<C2LinearBlock> outputBlock = std::move(mOutputBuffersMap[buffer->id]);
    mOutputBuffersMap.erase(buffer->id);
    ALOG_ASSERT(outputBlock);

    C2ConstLinearBlock constBlock = outputBlock->share(outputBlock->offset(), dataSize, C2Fence());

    // If no CSD (content-specific-data, e.g. SPS for H.264) has been submitted yet, we expect this
    // output block to contain CSD. We only submit the CSD once, even if it's attached to each key
    // frame.
    if (!mCSDSubmitted) {
        ALOGV("No CSD submitted yet, extracting CSD");
        std::unique_ptr<C2StreamInitDataInfo::output> csd;
        C2ReadView view = constBlock.map().get();
        extractCSDInfo(&csd, view.data(), view.capacity());
        if (!csd) {
            ALOGE("Failed to extract CSD");
            reportError(C2_CORRUPTED);
            return;
        }

        // Attach the CSD to the first item in our output work queue.
        LOG_ASSERT(!mWorkQueue.empty());
        C2Work* work = mWorkQueue.front().get();
        work->worklets.front()->output.configUpdate.push_back(std::move(csd));
        mCSDSubmitted = true;
    }

    // Get the work item associated with the timestamp.
    C2Work* work = getWorkByTimestamp(timestamp);
    if (!work) {
        // It's possible we got an empty CSD request with timestamp 0, which we currently just
        // discard.
        if (timestamp != 0) {
            reportError(C2_CORRUPTED);
        }
        return;
    }

    std::shared_ptr<C2Buffer> linearBuffer = C2Buffer::CreateLinearBuffer(std::move(constBlock));
    if (!linearBuffer) {
        ALOGE("Failed to create linear buffer from block");
        reportError(C2_CORRUPTED);
        return;
    }

    if (keyFrame) {
        linearBuffer->setInfo(
                std::make_shared<C2StreamPictureTypeMaskInfo::output>(0u, C2Config::SYNC_FRAME));
    }
    work->worklets.front()->output.buffers.emplace_back(std::move(linearBuffer));

    // We can report the work item as completed if its associated input buffer has also been
    // released. As output buffers are not necessarily returned in order we might be able to return
    // multiple ready work items now.
    while (!mWorkQueue.empty() && isWorkDone(*mWorkQueue.front())) {
        reportWork(std::move(mWorkQueue.front()));
        mWorkQueue.pop_front();
    }
}

C2Work* V4L2EncodeComponent::getWorkByIndex(uint64_t index) {
    ALOGV("%s(): getting work item (index: %" PRIu64 ")", __func__, index);
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());

    auto it = std::find_if(mWorkQueue.begin(), mWorkQueue.end(),
                           [index](const std::unique_ptr<C2Work>& w) {
                               return w->input.ordinal.frameIndex.peeku() == index;
                           });
    if (it == mWorkQueue.end()) {
        ALOGE("Failed to find work (index: %" PRIu64 ")", index);
        return nullptr;
    }
    return it->get();
}

C2Work* V4L2EncodeComponent::getWorkByTimestamp(int64_t timestamp) {
    ALOGV("%s(): getting work item (timestamp: %" PRId64 ")", __func__, timestamp);
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());
    ALOG_ASSERT(timestamp >= 0);

    // Find the work with specified timestamp by looping over the output work queue. This should be
    // very fast as the output work queue will never be longer then a few items. Ignore empty work
    // items that are marked as EOS, as their timestamp might clash with other work items.
    auto it = std::find_if(
            mWorkQueue.begin(), mWorkQueue.end(), [timestamp](const std::unique_ptr<C2Work>& w) {
                return !(w->input.flags & C2FrameData::FLAG_END_OF_STREAM) &&
                       w->input.ordinal.timestamp.peeku() == static_cast<uint64_t>(timestamp);
            });
    if (it == mWorkQueue.end()) {
        ALOGE("Failed to find work (timestamp: %" PRIu64 ")", timestamp);
        return nullptr;
    }
    return it->get();
}

bool V4L2EncodeComponent::isWorkDone(const C2Work& work) const {
    ALOGV("%s()", __func__);
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());

    if ((work.input.flags & C2FrameData::FLAG_END_OF_STREAM) &&
        !(work.worklets.front()->output.flags & C2FrameData::FLAG_END_OF_STREAM)) {
        ALOGV("Work item %" PRIu64 " is marked as EOS but draining has not finished yet",
              work.input.ordinal.frameIndex.peeku());
        return false;
    }

    if (!work.input.buffers.empty() && work.input.buffers.front()) {
        ALOGV("Input buffer associated with work item %" PRIu64 " not returned yet",
              work.input.ordinal.frameIndex.peeku());
        return false;
    }

    // If the work item had an input buffer to be encoded, it should have an output buffer set.
    if (!work.input.buffers.empty() && work.worklets.front()->output.buffers.empty()) {
        ALOGV("Output buffer associated with work item %" PRIu64 " not returned yet",
              work.input.ordinal.frameIndex.peeku());
        return false;
    }

    return true;
}

void V4L2EncodeComponent::reportWork(std::unique_ptr<C2Work> work) {
    ALOG_ASSERT(work);
    ALOGV("%s(): Reporting work item as finished (index: %llu, timestamp: %llu)", __func__,
          work->input.ordinal.frameIndex.peekull(), work->input.ordinal.timestamp.peekull());
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());

    work->result = C2_OK;
    work->workletsProcessed = static_cast<uint32_t>(work->worklets.size());

    std::list<std::unique_ptr<C2Work>> finishedWorkList;
    finishedWorkList.emplace_back(std::move(work));
    mListener->onWorkDone_nb(shared_from_this(), std::move(finishedWorkList));
}

bool V4L2EncodeComponent::getBlockPool() {
    C2BlockPool::local_id_t poolId = mInterface->getBlockPoolId();
    if (poolId == C2BlockPool::BASIC_LINEAR) {
        ALOGW("Using unoptimized linear block pool");
    }
    c2_status_t status = GetCodec2BlockPool(poolId, shared_from_this(), &mOutputBlockPool);
    if (status != C2_OK || !mOutputBlockPool) {
        ALOGE("Failed to get output block pool, error: %d", status);
        return false;
    }
    return true;
}

void V4L2EncodeComponent::reportError(c2_status_t error) {
    ALOGV("%s()", __func__);
    ALOG_ASSERT(mEncoderTaskRunner->RunsTasksInCurrentSequence());

    // TODO(dstaessens): Report all pending work items as finished upon failure.
    std::lock_guard<std::mutex> lock(mComponentLock);
    if (mComponentState != ComponentState::ERROR) {
        setComponentState(ComponentState::ERROR);
        mListener->onError_nb(shared_from_this(), static_cast<uint32_t>(error));
    }
}

void V4L2EncodeComponent::setComponentState(ComponentState state) {
    // Check whether the state change is valid.
    switch (state) {
    case ComponentState::UNLOADED:
        ALOG_ASSERT(mComponentState == ComponentState::LOADED);
        break;
    case ComponentState::LOADED:
        ALOG_ASSERT(mComponentState == ComponentState::UNLOADED ||
                    mComponentState == ComponentState::RUNNING ||
                    mComponentState == ComponentState::ERROR);
        break;
    case ComponentState::RUNNING:
        ALOG_ASSERT(mComponentState == ComponentState::LOADED);
        break;
    case ComponentState::ERROR:
        break;
    }

    ALOGV("Changed component state from %s to %s", componentStateToString(mComponentState),
          componentStateToString(state));
    mComponentState = state;
}

const char* V4L2EncodeComponent::componentStateToString(V4L2EncodeComponent::ComponentState state) {
    switch (state) {
    case ComponentState::UNLOADED:
        return "UNLOADED";
    case ComponentState::LOADED:
        return "LOADED";
    case ComponentState::RUNNING:
        return "RUNNING";
    case ComponentState::ERROR:
        return "ERROR";
    }
}

}  // namespace android
