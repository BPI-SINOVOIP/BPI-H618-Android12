/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include <log/log.h>
#include <system/audio.h>
#include "primary_device.h"
#include "stream_in.h"
#include "stream_out.h"
#include "util.h"
#include "debug.h"

namespace android {
namespace hardware {
namespace audio {
namespace V6_0 {
namespace implementation {

constexpr size_t kInBufferDurationMs = 15;
constexpr size_t kOutBufferDurationMs = 22;

using ::android::hardware::Void;

PrimaryDevice::PrimaryDevice() {}

Return<Result> PrimaryDevice::initCheck() {
    return Result::OK;
}

Return<Result> PrimaryDevice::setMasterVolume(float volume) {
    if (isnan(volume) || volume < 0 || volume > 1.0) {
        return FAILURE(Result::INVALID_ARGUMENTS);
    }

    mMasterVolume = volume;
    updateOutputStreamVolume(mMasterMute ? 0.0f : volume);
    return Result::OK;
}

Return<void> PrimaryDevice::getMasterVolume(getMasterVolume_cb _hidl_cb) {
    _hidl_cb(Result::OK, mMasterVolume);
    return Void();
}

Return<Result> PrimaryDevice::PrimaryDevice::setMicMute(bool mute) {
    mMicMute = mute;
    updateInputStreamMicMute(mute);
    return Result::OK;
}

Return<void> PrimaryDevice::getMicMute(getMicMute_cb _hidl_cb) {
    _hidl_cb(Result::OK, mMicMute);
    return Void();
}

Return<Result> PrimaryDevice::setMasterMute(bool mute) {
    mMasterMute = mute;
    updateOutputStreamVolume(mute ? 0.0f : mMasterVolume);
    return Result::OK;
}

Return<void> PrimaryDevice::getMasterMute(getMasterMute_cb _hidl_cb) {
    _hidl_cb(Result::OK, mMasterMute);
    return Void();
}

Return<void> PrimaryDevice::getInputBufferSize(const AudioConfig& config,
                                               getInputBufferSize_cb _hidl_cb) {
    AudioConfig suggestedConfig;
    if (util::checkAudioConfig(false, kInBufferDurationMs, config, suggestedConfig)) {
        const size_t sz =
            suggestedConfig.frameCount
            * util::countChannels(suggestedConfig.channelMask)
            * util::getBytesPerSample(suggestedConfig.format);

        _hidl_cb(Result::OK, sz);
    } else {
        ALOGE("PrimaryDevice::%s:%d failed", __func__, __LINE__);
        _hidl_cb(FAILURE(Result::INVALID_ARGUMENTS), 0);
    }

    return Void();
}

Return<void> PrimaryDevice::openOutputStream(int32_t ioHandle,
                                             const DeviceAddress& device,
                                             const AudioConfig& config,
                                             hidl_bitfield<AudioOutputFlag> flags,
                                             const SourceMetadata& sourceMetadata,
                                             openOutputStream_cb _hidl_cb) {
    AudioConfig suggestedConfig;
    if (util::checkAudioConfig(true, kOutBufferDurationMs, config, suggestedConfig)) {
        auto stream = std::make_unique<StreamOut>(
            this, ioHandle, device, suggestedConfig, flags, sourceMetadata);

        stream->setMasterVolume(mMasterMute ? 0.0f : mMasterVolume);

        {
            std::lock_guard<std::mutex> guard(mMutex);
            LOG_ALWAYS_FATAL_IF(!mOutputStreams.insert(stream.get()).second);
        }

        _hidl_cb(Result::OK, stream.release(), config);
    } else {
        ALOGE("PrimaryDevice::%s:%d failed", __func__, __LINE__);
        _hidl_cb(FAILURE(Result::INVALID_ARGUMENTS), nullptr, suggestedConfig);
    }

    return Void();
}

Return<void> PrimaryDevice::openInputStream(int32_t ioHandle,
                                            const DeviceAddress& device,
                                            const AudioConfig& config,
                                            hidl_bitfield<AudioInputFlag> flags,
                                            const SinkMetadata& sinkMetadata,
                                            openInputStream_cb _hidl_cb) {
    AudioConfig suggestedConfig;
    if (util::checkAudioConfig(false, kInBufferDurationMs, config, suggestedConfig)) {
        auto stream = std::make_unique<StreamIn>(
            this, ioHandle, device, suggestedConfig, flags, sinkMetadata);

        stream->setMicMute(mMicMute);

        {
            std::lock_guard<std::mutex> guard(mMutex);
            LOG_ALWAYS_FATAL_IF(!mInputStreams.insert(stream.get()).second);
        }

        _hidl_cb(Result::OK, stream.release(), config);
    } else {
        ALOGE("PrimaryDevice::%s:%d failed", __func__, __LINE__);
        _hidl_cb(FAILURE(Result::INVALID_ARGUMENTS), nullptr, suggestedConfig);
    }

    return Void();
}

Return<bool> PrimaryDevice::supportsAudioPatches() {
    return true;
}

Return<void> PrimaryDevice::createAudioPatch(const hidl_vec<AudioPortConfig>& sources,
                                             const hidl_vec<AudioPortConfig>& sinks,
                                             createAudioPatch_cb _hidl_cb) {
    if (sources.size() == 1 && sinks.size() == 1) {
        AudioPatch patch;
        patch.source = sources[0];
        patch.sink = sinks[0];

        AudioPatchHandle handle;
        while (true) {
            handle = mNextAudioPatchHandle;
            mNextAudioPatchHandle = std::max(handle + 1, 0);
            if (mAudioPatches.insert({handle, patch}).second) {
                break;
            }
        }

        _hidl_cb(Result::OK, handle);
    } else {
        _hidl_cb(FAILURE(Result::NOT_SUPPORTED), 0);
    }

    return Void();
}

Return<void> PrimaryDevice::updateAudioPatch(AudioPatchHandle previousPatchHandle,
                                             const hidl_vec<AudioPortConfig>& sources,
                                             const hidl_vec<AudioPortConfig>& sinks,
                                             updateAudioPatch_cb _hidl_cb) {
    const auto i = mAudioPatches.find(previousPatchHandle);
    if (i == mAudioPatches.end()) {
        _hidl_cb(FAILURE(Result::INVALID_ARGUMENTS), previousPatchHandle);
    } else {
        if (sources.size() == 1 && sinks.size() == 1) {
            AudioPatch patch;
            patch.source = sources[0];
            patch.sink = sinks[0];
            i->second = patch;

            _hidl_cb(Result::OK, previousPatchHandle);
        } else {
            _hidl_cb(Result::NOT_SUPPORTED, previousPatchHandle);
        }
    }

    return Void();
}

Return<Result> PrimaryDevice::releaseAudioPatch(AudioPatchHandle patchHandle) {
    return (mAudioPatches.erase(patchHandle) == 1) ? Result::OK : FAILURE(Result::INVALID_ARGUMENTS);
}

Return<void> PrimaryDevice::getAudioPort(const AudioPort& port, getAudioPort_cb _hidl_cb) {
    _hidl_cb(FAILURE(Result::NOT_SUPPORTED), port);
    return Void();
}

Return<Result> PrimaryDevice::setAudioPortConfig(const AudioPortConfig& config) {
    (void)config;
    return FAILURE(Result::NOT_SUPPORTED);
}

Return<Result> PrimaryDevice::setScreenState(bool turnedOn) {
    (void)turnedOn;
    return Result::OK;
}

Return<void> PrimaryDevice::getHwAvSync(getHwAvSync_cb _hidl_cb) {
    _hidl_cb(FAILURE(Result::NOT_SUPPORTED), {});
    return Void();
}

Return<void> PrimaryDevice::getParameters(const hidl_vec<ParameterValue>& context,
                                          const hidl_vec<hidl_string>& keys,
                                          getParameters_cb _hidl_cb) {
    (void)context;
    if (keys.size() == 0) {
        _hidl_cb(Result::OK, {});
    } else {
        _hidl_cb(FAILURE(Result::NOT_SUPPORTED), {});
    }
    return Void();
}

Return<Result> PrimaryDevice::setParameters(const hidl_vec<ParameterValue>& context,
                                            const hidl_vec<ParameterValue>& parameters) {
    (void)context;
    (void)parameters;
    return Result::OK;
}

Return<void> PrimaryDevice::getMicrophones(getMicrophones_cb _hidl_cb) {
    _hidl_cb(Result::OK, {util::getMicrophoneInfo()});
    return Void();
}

Return<Result> PrimaryDevice::setConnectedState(const DeviceAddress& dev_addr, bool connected) {
    (void)dev_addr;
    (void)connected;
    return FAILURE(Result::NOT_SUPPORTED);
}

Return<Result> PrimaryDevice::close() {
    std::lock_guard<std::mutex> guard(mMutex);

    return (mInputStreams.empty() && mOutputStreams.empty())
        ? Result::OK : FAILURE(Result::INVALID_STATE);
}

Return<Result> PrimaryDevice::addDeviceEffect(AudioPortHandle device, uint64_t effectId) {
    (void)device;
    (void)effectId;
    return FAILURE(Result::NOT_SUPPORTED);
}

Return<Result> PrimaryDevice::removeDeviceEffect(AudioPortHandle device, uint64_t effectId) {
    (void)device;
    (void)effectId;
    return FAILURE(Result::NOT_SUPPORTED);
}

Return<Result> PrimaryDevice::setVoiceVolume(float volume) {
    return (volume >= 0 && volume <= 1.0) ? Result::OK : FAILURE(Result::INVALID_ARGUMENTS);
}

Return<Result> PrimaryDevice::setMode(AudioMode mode) {
    switch (mode) {
    case AudioMode::NORMAL:
    case AudioMode::RINGTONE:
    case AudioMode::IN_CALL:
    case AudioMode::IN_COMMUNICATION:
        return Result::OK;

    default:
        return FAILURE(Result::INVALID_ARGUMENTS);
    }
}

Return<Result> PrimaryDevice::setBtScoHeadsetDebugName(const hidl_string& name) {
    (void)name;
    return FAILURE(Result::NOT_SUPPORTED);
}

Return<void> PrimaryDevice::getBtScoNrecEnabled(getBtScoNrecEnabled_cb _hidl_cb) {
    _hidl_cb(FAILURE(Result::NOT_SUPPORTED), false);
    return Void();
}

Return<Result> PrimaryDevice::setBtScoNrecEnabled(bool enabled) {
    (void)enabled;
    return FAILURE(Result::NOT_SUPPORTED);
}

Return<void> PrimaryDevice::getBtScoWidebandEnabled(getBtScoWidebandEnabled_cb _hidl_cb) {
    _hidl_cb(FAILURE(Result::NOT_SUPPORTED), false);
    return Void();
}

Return<Result> PrimaryDevice::setBtScoWidebandEnabled(bool enabled) {
    (void)enabled;
    return FAILURE(Result::NOT_SUPPORTED);
}

Return<void> PrimaryDevice::getTtyMode(getTtyMode_cb _hidl_cb) {
    _hidl_cb(FAILURE(Result::NOT_SUPPORTED), TtyMode::OFF);
    return Void();
}

Return<Result> PrimaryDevice::setTtyMode(IPrimaryDevice::TtyMode mode) {
    (void)mode;
    return FAILURE(Result::NOT_SUPPORTED);
}

Return<void> PrimaryDevice::getHacEnabled(getHacEnabled_cb _hidl_cb) {
    _hidl_cb(FAILURE(Result::NOT_SUPPORTED), false);
    return Void();
}

Return<Result> PrimaryDevice::setHacEnabled(bool enabled) {
    (void)enabled;
    return FAILURE(Result::NOT_SUPPORTED);
}

Return<void> PrimaryDevice::getBtHfpEnabled(getBtHfpEnabled_cb _hidl_cb) {
    _hidl_cb(FAILURE(Result::NOT_SUPPORTED), false);
    return Void();
}

Return<Result> PrimaryDevice::setBtHfpEnabled(bool enabled) {
    (void)enabled;
    return FAILURE(Result::NOT_SUPPORTED);
}

Return<Result> PrimaryDevice::setBtHfpSampleRate(uint32_t sampleRateHz) {
    (void)sampleRateHz;
    return FAILURE(Result::NOT_SUPPORTED);
}

Return<Result> PrimaryDevice::setBtHfpVolume(float volume) {
    (void)volume;
    return FAILURE(Result::NOT_SUPPORTED);
}

Return<Result> PrimaryDevice::updateRotation(IPrimaryDevice::Rotation rotation) {
    (void)rotation;
    return FAILURE(Result::NOT_SUPPORTED);
}

void PrimaryDevice::unrefDevice(StreamIn *sin) {
    std::lock_guard<std::mutex> guard(mMutex);
    LOG_ALWAYS_FATAL_IF(mInputStreams.erase(sin) < 1);
}

void PrimaryDevice::unrefDevice(StreamOut *sout) {
    std::lock_guard<std::mutex> guard(mMutex);
    LOG_ALWAYS_FATAL_IF(mOutputStreams.erase(sout) < 1);
}

void PrimaryDevice::updateOutputStreamVolume(float masterVolume) const {
    std::lock_guard<std::mutex> guard(mMutex);
    for (StreamOut *stream : mOutputStreams) {
        stream->setMasterVolume(masterVolume);
    }
}

void PrimaryDevice::updateInputStreamMicMute(bool micMute) const {
    std::lock_guard<std::mutex> guard(mMutex);
    for (StreamIn *stream : mInputStreams) {
        stream->setMicMute(micMute);
    }
}

}  // namespace implementation
}  // namespace V6_0
}  // namespace audio
}  // namespace hardware
}  // namespace android
