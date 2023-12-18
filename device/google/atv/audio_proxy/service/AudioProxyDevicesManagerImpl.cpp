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

#include "AudioProxyDevicesManagerImpl.h"

#include <utils/Log.h>

using ::android::OK;
using ::android::status_t;
using ::device::google::atv::audio_proxy::AUDIO_PROXY_CPP_VERSION::IBusDevice;

namespace audio_proxy {
namespace service {
namespace {
template<typename D>
class BusDeviceVersionWrapper : public IBusDevice {
  public:
   explicit BusDeviceVersionWrapper(const sp<D>& device) : mDevice(device) {}
   ~BusDeviceVersionWrapper() override = default;

   Return<void> openOutputStream(int32_t ioHandle, const DeviceAddress& device,
                                 const AudioConfig& config,
                                 hidl_bitfield<AudioOutputFlag> flags,
                                 const SourceMetadata& sourceMetadata,
                                 openOutputStream_cb _hidl_cb) override {
     return mDevice->openOutputStream(ioHandle, device, config, flags,
                                      sourceMetadata, std::move(_hidl_cb));
   }

  private:
   const sp<D> mDevice;
};
}  // namespace

AudioProxyDevicesManagerImpl::AudioProxyDevicesManagerImpl() {
  // We need to register the factory service when this is instantiated,
  // rather than when the first client registers in order to not break VTS.
  ensureDevicesFactory();
}

AudioProxyDevicesManagerImpl::~AudioProxyDevicesManagerImpl() = default;

Return<bool> AudioProxyDevicesManagerImpl::registerDevice(
    const hidl_string& address,
    const sp<::device::google::atv::audio_proxy::CPP_VERSION::IBusDevice>&
        device) {
  if (address.empty() || !device) {
    return false;
  }

  sp<IBusDevice> busDevice = IBusDevice::castFrom(device);
  if (!busDevice) {
    ALOGW("Client registers lower version bus device at %s", address.c_str());
    busDevice = new BusDeviceVersionWrapper(device);
  }

  if (!mBusDeviceProvider.add(address, busDevice)) {
    ALOGE("Failed to register bus device with addr %s", address.c_str());
    return false;
  }

  if (!ensureDevicesFactory()) {
    ALOGE("Failed to register audio devices factory.");
    mBusDeviceProvider.removeAll();
    return false;
  }

  return true;
}

bool AudioProxyDevicesManagerImpl::ensureDevicesFactory() {
  sp<DevicesFactoryImpl> factory = mDevicesFactory.promote();
  if (factory) {
    return true;
  }

  factory = new DevicesFactoryImpl(mBusDeviceProvider);
  status_t status = factory->registerAsService("audio_proxy");
  if (status != OK) {
    return false;
  }

  mDevicesFactory = factory;
  return true;
}

}  // namespace service
}  // namespace audio_proxy
