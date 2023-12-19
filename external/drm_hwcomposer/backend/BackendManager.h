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

#ifndef ANDROID_BACKEND_MANAGER_H
#define ANDROID_BACKEND_MANAGER_H

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "Backend.h"

#define REGISTER_BACKEND(name_str_, backend_)                               \
  static int                                                                \
      backend = BackendManager::GetInstance()                               \
                    .RegisterBackend(name_str_,                             \
                                     []() -> std::unique_ptr<Backend> {     \
                                       return std::make_unique<backend_>(); \
                                     });

namespace android {

class BackendManager {
 public:
  using backend_constructor_t = std::function<std::unique_ptr<Backend>()>;
  static BackendManager &GetInstance();
  int RegisterBackend(const std::string &name,
                      backend_constructor_t backend_constructor);
  int SetBackendForDisplay(DrmHwcTwo::HwcDisplay *display);
  std::unique_ptr<Backend> GetBackendByName(std::string &name);
  HWC2::Error ValidateDisplay(DrmHwcTwo::HwcDisplay *display,
                              uint32_t *num_types, uint32_t *num_requests);

 private:
  BackendManager() = default;

  static const std::vector<std::string> client_devices_;

  std::map<std::string, backend_constructor_t> available_backends_;
};
}  // namespace android

#endif
