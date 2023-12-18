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

#include "DummyOutputStream.h"

namespace audio_proxy {
namespace service {

DummyOutputStream::DummyOutputStream(const AudioConfig &config)
    : mChannelMask(config.channelMask), mFormat(config.format),
      mSampleRate(config.sampleRateHz) {}

Return<uint64_t> DummyOutputStream::getBufferSize() { return 0; }

Return<uint64_t> DummyOutputStream::getFrameCount() { return 0; }

Return<void> DummyOutputStream::getSupportedSampleRates(
    AudioFormat, getSupportedSampleRates_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, {});
  return Void();
}

Return<uint32_t> DummyOutputStream::getSampleRate() { return mSampleRate; }

Return<void> DummyOutputStream::getSupportedChannelMasks(
    AudioFormat, getSupportedChannelMasks_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, {});
  return Void();
}

Return<hidl_bitfield<AudioChannelMask>> DummyOutputStream::getChannelMask() {
  return mChannelMask;
}

Return<void>
DummyOutputStream::getSupportedFormats(getSupportedFormats_cb _hidl_cb) {
  _hidl_cb({});
  return Void();
}

Return<AudioFormat> DummyOutputStream::getFormat() { return mFormat; }

Return<uint32_t> DummyOutputStream::getLatency() { return 0; }

Return<Result> DummyOutputStream::standby() { return Result::NOT_SUPPORTED; }

Return<Result> DummyOutputStream::pause() { return Result::NOT_SUPPORTED; }

Return<Result> DummyOutputStream::resume() { return Result::NOT_SUPPORTED; }

Return<Result> DummyOutputStream::drain(AudioDrain) {
  return Result::NOT_SUPPORTED;
}

Return<void>
DummyOutputStream::prepareForWriting(uint32_t frameSize, uint32_t framesCount,
                                     prepareForWriting_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, {}, {}, {}, {});
  return Void();
}

Return<Result>
DummyOutputStream::setParameters(const hidl_vec<ParameterValue> &context,
                                 const hidl_vec<ParameterValue> &parameters) {
  // This method is invoked in AudioFlinger::PlaybackThread::preExit which logs
  // an error when the returned result is not OK.
  return Result::OK;
}

Return<void>
DummyOutputStream::getParameters(const hidl_vec<ParameterValue> &context,
                                 const hidl_vec<hidl_string> &keys,
                                 getParameters_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, {});
  return Void();
}

Return<uint64_t> DummyOutputStream::getFrameSize() {
  // This is a randomly picked value.
  return 1024;
}

Return<Result> DummyOutputStream::setSampleRate(uint32_t) {
  return Result::NOT_SUPPORTED;
}

Return<Result>
DummyOutputStream::setChannelMask(hidl_bitfield<AudioChannelMask>) {
  return Result::NOT_SUPPORTED;
}

Return<Result> DummyOutputStream::setFormat(AudioFormat) {
  return Result::NOT_SUPPORTED;
}

Return<void>
DummyOutputStream::getAudioProperties(getAudioProperties_cb _hidl_cb) {
  _hidl_cb(mSampleRate, mChannelMask, mFormat);
  return Void();
}

Return<Result> DummyOutputStream::addEffect(uint64_t) {
  return Result::NOT_SUPPORTED;
}

Return<Result> DummyOutputStream::removeEffect(uint64_t) {
  return Result::NOT_SUPPORTED;
}

Return<void> DummyOutputStream::getDevices(getDevices_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, {});
  return Void();
}

Return<Result> DummyOutputStream::setDevices(const hidl_vec<DeviceAddress> &) {
  return Result::NOT_SUPPORTED;
}

Return<Result> DummyOutputStream::setHwAvSync(uint32_t hwAvSync) {
  return Result::NOT_SUPPORTED;
}

Return<Result> DummyOutputStream::start() { return Result::NOT_SUPPORTED; }

Return<Result> DummyOutputStream::stop() { return Result::NOT_SUPPORTED; }

Return<void> DummyOutputStream::createMmapBuffer(int32_t minSizeFrames,
                                                 createMmapBuffer_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, {});
  return Void();
}

Return<void> DummyOutputStream::getMmapPosition(getMmapPosition_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, {});
  return Void();
}

Return<Result> DummyOutputStream::close() { return Result::NOT_SUPPORTED; }

Return<Result> DummyOutputStream::setVolume(float left, float right) {
  return Result::NOT_SUPPORTED;
}

Return<void> DummyOutputStream::updateSourceMetadata(const SourceMetadata &) {
  return Void();
}

Return<void>
DummyOutputStream::getRenderPosition(getRenderPosition_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, {});
  return Void();
}

Return<void>
DummyOutputStream::getNextWriteTimestamp(getNextWriteTimestamp_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, {});
  return Void();
}

Return<Result> DummyOutputStream::setCallback(const sp<IStreamOutCallback> &) {
  return Result::NOT_SUPPORTED;
}

Return<Result> DummyOutputStream::clearCallback() {
  return Result::NOT_SUPPORTED;
}

Return<void>
DummyOutputStream::supportsPauseAndResume(supportsPauseAndResume_cb _hidl_cb) {
  _hidl_cb({}, {});
  return Void();
}

Return<bool> DummyOutputStream::supportsDrain() { return false; }

Return<Result> DummyOutputStream::flush() { return Result::NOT_SUPPORTED; }

Return<void> DummyOutputStream::getPresentationPosition(
    getPresentationPosition_cb _hidl_cb) {
  _hidl_cb(Result::NOT_SUPPORTED, {}, {});
  return Void();
}

Return<Result> DummyOutputStream::selectPresentation(int32_t, int32_t) {
  return Result::NOT_SUPPORTED;
}

} // namespace service
} // namespace audio_proxy
