/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <chrono>
#include <thread>
#include <log/log.h>
#include <utils/Timers.h>
#include <utils/ThreadDefs.h>
#include "device_port_sink.h"
#include "talsa.h"
#include "audio_ops.h"
#include "ring_buffer.h"
#include "util.h"
#include "debug.h"

namespace android {
namespace hardware {
namespace audio {
namespace V6_0 {
namespace implementation {

namespace {

constexpr int kMaxJitterUs = 3000;  // Enforced by CTS, should be <= 6ms

struct TinyalsaSink : public DevicePortSink {
    TinyalsaSink(unsigned pcmCard, unsigned pcmDevice,
                 const AudioConfig &cfg,
                 uint64_t &frames)
            : mStartNs(systemTime(SYSTEM_TIME_MONOTONIC))
            , mSampleRateHz(cfg.sampleRateHz)
            , mFrameSize(util::countChannels(cfg.channelMask) * sizeof(int16_t))
            , mWriteSizeFrames(cfg.frameCount)
            , mFrames(frames)
            , mRingBuffer(mFrameSize * cfg.frameCount * 3)
            , mMixer(pcmCard)
            , mPcm(talsa::pcmOpen(pcmCard, pcmDevice,
                                  util::countChannels(cfg.channelMask),
                                  cfg.sampleRateHz,
                                  cfg.frameCount,
                                  true /* isOut */)) {
        mConsumeThread = std::thread(&TinyalsaSink::consumeThread, this);
    }

    ~TinyalsaSink() {
        mConsumeThreadRunning = false;
        mConsumeThread.join();
    }

    Result getPresentationPosition(uint64_t &frames, TimeSpec &ts) override {
        const nsecs_t nowNs = systemTime(SYSTEM_TIME_MONOTONIC);
        const uint64_t nowFrames = getPresentationFrames(nowNs);
        mFrames += (nowFrames - mPreviousFrames);
        mPreviousFrames = nowFrames;

        frames = mFrames;
        ts = util::nsecs2TimeSpec(nowNs);
        return Result::OK;
    }

    uint64_t getPresentationFrames(const nsecs_t nowNs) const {
        return uint64_t(mSampleRateHz) * ns2us(nowNs - mStartNs) / 1000000;
    }

    uint64_t getAvailableFrames(const nsecs_t nowNs) const {
        return getPresentationFrames(nowNs) - mReceivedFrames;
    }

    uint64_t getAvailableFramesNow() const {
        return getAvailableFrames(systemTime(SYSTEM_TIME_MONOTONIC));
    }

    size_t getWaitFramesNow(const size_t requestedFrames) const {
        const size_t availableFrames = getAvailableFramesNow();
        return (requestedFrames > availableFrames)
            ? (requestedFrames - availableFrames) : 0;
    }

    size_t write(float volume, size_t bytesToWrite, IReader &reader) {
        size_t framesLost = 0;
        const size_t waitFrames = getWaitFramesNow(bytesToWrite / mFrameSize);
        const auto blockUntil =
            std::chrono::high_resolution_clock::now() +
                + std::chrono::microseconds(waitFrames * 1000000 / mSampleRateHz);

        while (bytesToWrite > 0) {
            if (mRingBuffer.waitForProduceAvailable(blockUntil
                    + std::chrono::microseconds(kMaxJitterUs))) {
                auto produceChunk = mRingBuffer.getProduceChunk();
                if (produceChunk.size >= bytesToWrite) {
                    // Since the ring buffer has more bytes free than we need,
                    // make sure we are not too early here: tinyalsa is jittery,
                    // we don't want to go faster than SYSTEM_TIME_MONOTONIC
                    std::this_thread::sleep_until(blockUntil);
                }

                const size_t szFrames =
                    std::min(produceChunk.size, bytesToWrite) / mFrameSize;
                const size_t szBytes = szFrames * mFrameSize;
                LOG_ALWAYS_FATAL_IF(reader(produceChunk.data, szBytes) < szBytes);

                aops::multiplyByVolume(volume,
                                       static_cast<int16_t *>(produceChunk.data),
                                       szBytes / sizeof(int16_t));

                LOG_ALWAYS_FATAL_IF(mRingBuffer.produce(szBytes) < szBytes);
                mReceivedFrames += szFrames;
                bytesToWrite -= szBytes;
            } else {
                ALOGV("TinyalsaSink::%s:%d pcm_write was late reading "
                      "frames, dropping %zu us of audio",
                      __func__, __LINE__,
                      size_t(1000000 * bytesToWrite / mFrameSize / mSampleRateHz));

                // drop old audio to make room for new
                const size_t bytesLost = mRingBuffer.makeRoomForProduce(bytesToWrite);
                framesLost += bytesLost / mFrameSize;

                while (bytesToWrite > 0) {
                    auto produceChunk = mRingBuffer.getProduceChunk();
                    const size_t szFrames =
                        std::min(produceChunk.size, bytesToWrite) / mFrameSize;
                    const size_t szBytes = szFrames * mFrameSize;
                    LOG_ALWAYS_FATAL_IF(reader(produceChunk.data, szBytes) < szBytes);

                    aops::multiplyByVolume(volume,
                                           static_cast<int16_t *>(produceChunk.data),
                                           szBytes / sizeof(int16_t));

                    LOG_ALWAYS_FATAL_IF(mRingBuffer.produce(szBytes) < szBytes);
                    mReceivedFrames += szFrames;
                    bytesToWrite -= szBytes;
                }
                break;
            }
        }

        return framesLost;
    }

    void consumeThread() {
        util::setThreadPriority(PRIORITY_URGENT_AUDIO);
        std::vector<uint8_t> writeBuffer(mWriteSizeFrames * mFrameSize);

        while (mConsumeThreadRunning) {
            if (mRingBuffer.waitForConsumeAvailable(
                    std::chrono::high_resolution_clock::now()
                    + std::chrono::microseconds(100000))) {
                size_t szBytes;
                {
                    auto chunk = mRingBuffer.getConsumeChunk();
                    szBytes = std::min(writeBuffer.size(), chunk.size);
                    // We have to memcpy because the consumer holds the lock
                    // into RingBuffer and pcm_write takes too long to hold
                    // this lock.
                    memcpy(writeBuffer.data(), chunk.data, szBytes);
                    LOG_ALWAYS_FATAL_IF(mRingBuffer.consume(chunk, szBytes) < szBytes);
                }

                int res = ::pcm_write(mPcm.get(), writeBuffer.data(), szBytes);
                if (res < 0) {
                    ALOGW("TinyalsaSink::%s:%d pcm_write failed with res=%d",
                          __func__, __LINE__, res);
                }
            }
        }
    }

    static std::unique_ptr<TinyalsaSink> create(unsigned pcmCard,
                                                unsigned pcmDevice,
                                                const AudioConfig &cfg,
                                                size_t readerBufferSizeHint,
                                                uint64_t &frames) {
        (void)readerBufferSizeHint;
        auto sink = std::make_unique<TinyalsaSink>(pcmCard, pcmDevice,
                                                   cfg, frames);
        if (sink->mMixer && sink->mPcm) {
            return sink;
        } else {
            return FAILURE(nullptr);
        }
    }

private:
    const nsecs_t mStartNs;
    const unsigned mSampleRateHz;
    const unsigned mFrameSize;
    const unsigned mWriteSizeFrames;
    uint64_t &mFrames;
    uint64_t mPreviousFrames = 0;
    uint64_t mReceivedFrames = 0;
    RingBuffer mRingBuffer;
    talsa::Mixer mMixer;
    talsa::PcmPtr mPcm;
    std::thread mConsumeThread;
    std::atomic<bool> mConsumeThreadRunning = true;
};

struct NullSink : public DevicePortSink {
    NullSink(const AudioConfig &cfg, uint64_t &frames)
            : mFrames(frames)
            , mSampleRateHz(cfg.sampleRateHz)
            , mNChannels(util::countChannels(cfg.channelMask))
            , mTimestamp(systemTime(SYSTEM_TIME_MONOTONIC)) {}

    Result getPresentationPosition(uint64_t &frames, TimeSpec &ts) override {
        simulatePresentationPosition();
        frames = mFrames;
        ts = util::nsecs2TimeSpec(mTimestamp);
        return Result::OK;
    }

    size_t write(float volume, size_t bytesToWrite, IReader &reader) override {
        (void)volume;

        while (bytesToWrite > 0) {
            size_t chunkSize = std::min(bytesToWrite, sizeof(mWriteBuffer));
            chunkSize = reader(mWriteBuffer, chunkSize);
            if (chunkSize > 0) {
                bytesToWrite -= chunkSize;
            } else {
                break; // reader failed
            }
        }

        return 0;
    }

    void simulatePresentationPosition() {
        const nsecs_t nowNs = systemTime(SYSTEM_TIME_MONOTONIC);
        const nsecs_t deltaNs = nowNs - mTimestamp;
        const uint64_t deltaFrames = uint64_t(mSampleRateHz) * ns2ms(deltaNs) / 1000;
        const uint64_t f = std::min(deltaFrames, mAvailableFrames);

        mFrames += f;
        mAvailableFrames -= f;
        if (mAvailableFrames) {
            mTimestamp += us2ns(f * 1000000 / mSampleRateHz);
        } else {
            mTimestamp = nowNs;
        }
    }

    static std::unique_ptr<NullSink> create(const AudioConfig &cfg,
                                            size_t readerBufferSizeHint,
                                            uint64_t &frames) {
        (void)readerBufferSizeHint;
        return std::make_unique<NullSink>(cfg, frames);
    }

private:
    uint64_t &mFrames;
    const unsigned mSampleRateHz;
    const unsigned mNChannels;
    uint64_t mAvailableFrames = 0;
    nsecs_t mTimestamp;
    char mWriteBuffer[1024];
};

}  // namespace

std::unique_ptr<DevicePortSink>
DevicePortSink::create(size_t readerBufferSizeHint,
                       const DeviceAddress &address,
                       const AudioConfig &cfg,
                       const hidl_bitfield<AudioOutputFlag> &flags,
                       uint64_t &frames) {
    (void)flags;

    if (cfg.format != AudioFormat::PCM_16_BIT) {
        ALOGE("%s:%d Only PCM_16_BIT is supported", __func__, __LINE__);
        return FAILURE(nullptr);
    }

    switch (address.device) {
    case AudioDevice::OUT_SPEAKER:
        {
            auto sinkptr = TinyalsaSink::create(talsa::kPcmCard, talsa::kPcmDevice, cfg, readerBufferSizeHint, frames);
            if (sinkptr == nullptr) {
                ALOGW("%s:%d failed to create alsa sink; created nullsink instead.", __func__, __LINE__);
                return NullSink::create(cfg, readerBufferSizeHint, frames);
            } else {
                return sinkptr;
            }
        }

    case AudioDevice::OUT_TELEPHONY_TX:
        return NullSink::create(cfg, readerBufferSizeHint, frames);

    default:
        ALOGW("%s:%d unsupported device: %x created nullsink", __func__, __LINE__, address.device);
        return NullSink::create(cfg, readerBufferSizeHint, frames);
    }
}

}  // namespace implementation
}  // namespace V6_0
}  // namespace audio
}  // namespace hardware
}  // namespace android
