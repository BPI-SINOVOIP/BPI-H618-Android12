// Copyright (C) 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

// clang-format off
#include PATH(android/hardware/audio/FILE_VERSION/IStreamOut.h)
// clang-format on
#include <hidl/Status.h>

namespace audio_proxy {
namespace service {

using ::android::sp;
using ::android::hardware::hidl_bitfield;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::audio::common::CPP_VERSION::AudioChannelMask;
using ::android::hardware::audio::common::CPP_VERSION::AudioConfig;
using ::android::hardware::audio::common::CPP_VERSION::AudioFormat;
using ::android::hardware::audio::common::CPP_VERSION::DeviceAddress;
using ::android::hardware::audio::common::CPP_VERSION::SourceMetadata;
using ::android::hardware::audio::CPP_VERSION::AudioDrain;
using ::android::hardware::audio::CPP_VERSION::IStreamOut;
using ::android::hardware::audio::CPP_VERSION::IStreamOutCallback;
using ::android::hardware::audio::CPP_VERSION::ParameterValue;
using ::android::hardware::audio::CPP_VERSION::Result;

class DummyOutputStream : public IStreamOut {
public:
  DummyOutputStream(const AudioConfig &config);

private:
  Return<uint64_t> getBufferSize() override;
  Return<uint64_t> getFrameCount() override;

  Return<void>
  getSupportedSampleRates(AudioFormat,
                          getSupportedSampleRates_cb _hidl_cb) override;
  Return<uint32_t> getSampleRate() override;

  Return<void>
  getSupportedChannelMasks(AudioFormat,
                           getSupportedChannelMasks_cb _hidl_cb) override;
  Return<hidl_bitfield<AudioChannelMask>> getChannelMask() override;

  Return<void> getSupportedFormats(getSupportedFormats_cb _hidl_cb) override;
  Return<AudioFormat> getFormat() override;

  Return<uint32_t> getLatency() override;

  Return<Result> standby() override;
  Return<Result> pause() override;
  Return<Result> resume() override;
  Return<Result> drain(AudioDrain) override;

  Return<void> prepareForWriting(uint32_t frameSize, uint32_t framesCount,
                                 prepareForWriting_cb _hidl_cb) override;

  Return<Result>
  setParameters(const hidl_vec<ParameterValue> &context,
                const hidl_vec<ParameterValue> &parameters) override;
  Return<void> getParameters(const hidl_vec<ParameterValue> &context,
                             const hidl_vec<hidl_string> &keys,
                             getParameters_cb _hidl_cb) override;

  Return<uint64_t> getFrameSize() override;
  Return<Result> setSampleRate(uint32_t) override;
  Return<Result> setChannelMask(hidl_bitfield<AudioChannelMask>) override;
  Return<Result> setFormat(AudioFormat) override;
  Return<void> getAudioProperties(getAudioProperties_cb _hidl_cb) override;
  Return<Result> addEffect(uint64_t) override;
  Return<Result> removeEffect(uint64_t) override;
  Return<void> getDevices(getDevices_cb _hidl_cb) override;
  Return<Result> setDevices(const hidl_vec<DeviceAddress> &) override;
  Return<Result> setHwAvSync(uint32_t hwAvSync) override;
  Return<Result> start() override;
  Return<Result> stop() override;
  Return<void> createMmapBuffer(int32_t minSizeFrames,
                                createMmapBuffer_cb _hidl_cb) override;
  Return<void> getMmapPosition(getMmapPosition_cb _hidl_cb) override;
  Return<Result> close() override;
  Return<Result> setVolume(float left, float right) override;
  Return<void> updateSourceMetadata(const SourceMetadata &) override;
  Return<void> getRenderPosition(getRenderPosition_cb _hidl_cb) override;
  Return<void>
  getNextWriteTimestamp(getNextWriteTimestamp_cb _hidl_cb) override;
  Return<Result> setCallback(const sp<IStreamOutCallback> &) override;
  Return<Result> clearCallback() override;
  Return<void>
  supportsPauseAndResume(supportsPauseAndResume_cb _hidl_cb) override;
  Return<bool> supportsDrain() override;
  Return<Result> flush() override;
  Return<void>
  getPresentationPosition(getPresentationPosition_cb _hidl_cb) override;
  Return<Result> selectPresentation(int32_t, int32_t) override;

private:
  const hidl_bitfield<AudioChannelMask> mChannelMask;
  const AudioFormat mFormat;
  const uint32_t mSampleRate;
};

} // namespace service
} // namespace audio_proxy
