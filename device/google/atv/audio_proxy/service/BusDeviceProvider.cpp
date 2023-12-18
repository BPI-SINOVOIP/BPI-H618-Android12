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

#include "BusDeviceProvider.h"

#include <utils/Log.h>

#include <algorithm>

#include "DummyBusDevice.h"

#undef LOG_TAG
#define LOG_TAG "BusDeviceProvider"

namespace audio_proxy {
namespace service {

class BusDeviceProvider::DeathRecipient : public hidl_death_recipient {
 public:
  explicit DeathRecipient(BusDeviceProvider& owner) : mOwner(owner) {}
  ~DeathRecipient() override = default;

  void serviceDied(
      uint64_t token,
      const android::wp<::android::hidl::base::V1_0::IBase>& who) override {
    sp<BusDeviceProvider::Handle> handle = mOwner.removeByToken(token);

    // If the stopped client still has opened stream, audioserver may still hold
    // a dead IStreamOut object. This may prevent the client to create a new
    // IStreamOut when it's rebooted. Crash the audioserver as a temporary
    // solution to fix this.
    if (handle && handle->getStreamCount() > 0) {
      ALOGW("Device at %s crashed with opened stream. Crash audioserver.",
            handle->getAddress().c_str());
      // Avoid calling atexit handlers, as this code runs on a thread from RPC
      // threadpool.
      _exit(-3);
    }
  }

 private:
  BusDeviceProvider& mOwner;
};

BusDeviceProvider::Handle::Handle(sp<IBusDevice> device,
                                  const hidl_string& address,
                                  uint64_t token)
  : mDevice(std::move(device)),
    mAddress(address),
    mToken(token) {}

BusDeviceProvider::Handle::~Handle() = default;

const sp<IBusDevice>& BusDeviceProvider::Handle::getDevice() const {
  return mDevice;
}

const hidl_string& BusDeviceProvider::Handle::getAddress() const {
  return mAddress;
}

uint64_t BusDeviceProvider::Handle::getToken() const {
  return mToken;
}

int BusDeviceProvider::Handle::getStreamCount() const {
  return mStreamCount;
}

void BusDeviceProvider::Handle::onStreamOpen() {
  mStreamCount++;
}

void BusDeviceProvider::Handle::onStreamClose() {
  mStreamCount--;
}

BusDeviceProvider::BusDeviceProvider()
    : mDeathRecipient(new DeathRecipient(*this)) {}
BusDeviceProvider::~BusDeviceProvider() = default;

bool BusDeviceProvider::add(const hidl_string& address, sp<IBusDevice> device) {
  std::lock_guard<std::mutex> guard(mDevicesLock);
  auto it = std::find_if(mBusDevices.begin(), mBusDevices.end(),
                         [&address](const sp<Handle>& handle) {
                           return handle->getAddress() == address;
                         });

  if (it != mBusDevices.end()) {
    return false;
  }

  uint64_t token = mNextToken++;

  mBusDevices.emplace_back(new Handle(device, address, token));

  device->linkToDeath(mDeathRecipient, token);

  return true;
}

sp<BusDeviceProvider::Handle> BusDeviceProvider::get(const hidl_string& address) {
  std::lock_guard<std::mutex> guard(mDevicesLock);
  auto it = std::find_if(mBusDevices.begin(), mBusDevices.end(),
                         [&address](const sp<Handle>& handle) {
                           return handle->getAddress() == address;
                         });

  if (it == mBusDevices.end()) {
    // When AudioPolicyManager first opens this HAL, it iterates through the
    // devices and quickly opens and closes the first device (as specified in
    // the audio configuration .xml). However, it is possible that the first
    // device has not registered with the audio proxy HAL yet. In this case, we
    // will return a dummy device, which is going to create a dummy output
    // stream. Since this HAL only supports direct outputs, the dummy output
    // will be immediately closed until it is reopened on use -- and by that
    // time the actual device must have registered itself.
    return new Handle(new DummyBusDevice(), "placeholder", 1234);
  }

  return *it;
}

void BusDeviceProvider::removeAll() {
  std::lock_guard<std::mutex> guard(mDevicesLock);
  mBusDevices.clear();
}

sp<BusDeviceProvider::Handle> BusDeviceProvider::removeByToken(uint64_t token) {
  std::lock_guard<std::mutex> guard(mDevicesLock);
  auto it = std::find_if(mBusDevices.begin(), mBusDevices.end(),
                         [token](const sp<Handle>& handle) {
                           return handle->getToken() == token;
                         });

  if (it == mBusDevices.end()) {
    return nullptr;
  }

  sp<Handle> handle = std::move(*it);
  mBusDevices.erase(it);
  return handle;
}

}  // namespace service
}  // namespace audio_proxy
