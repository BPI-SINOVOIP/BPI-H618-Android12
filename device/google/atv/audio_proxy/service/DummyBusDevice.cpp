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

#include "DummyBusDevice.h"

namespace audio_proxy {
namespace service {

Return<void> DummyBusDevice::openOutputStream(
    int32_t ioHandle, const DeviceAddress &device, const AudioConfig &config,
    hidl_bitfield<AudioOutputFlag> flags, const SourceMetadata &sourceMetadata,
    openOutputStream_cb _hidl_cb) {
  sp<IStreamOut> stream(new DummyOutputStream(config));
  _hidl_cb(Result::OK, stream, config);
  return Void();
}

} // namespace service
} // namespace audio_proxy
