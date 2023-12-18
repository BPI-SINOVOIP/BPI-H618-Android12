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

#include "linkerconfig/basecontext.h"

namespace android {
namespace linkerconfig {
namespace modules {
BaseContext::BaseContext() : strict_(false) {
}

void BaseContext::AddApexModule(ApexInfo apex_module) {
  apex_modules_.push_back(std::move(apex_module));
}

const std::vector<ApexInfo>& BaseContext::GetApexModules() const {
  return apex_modules_;
}

void BaseContext::SetStrictMode(bool strict) {
  strict_ = strict;
}

bool BaseContext::IsStrictMode() const {
  return strict_;
}

Namespace BaseContext::BuildApexNamespace(const ApexInfo& apex_info,
                                          bool visible) const {
  Namespace ns(apex_info.namespace_name,
               /*is_isolated=*/true,
               visible);
  InitializeWithApex(ns, apex_info);
  return ns;
}

void BaseContext::SetSystemConfig(
    const android::linkerconfig::proto::LinkerConfig& config) {
  system_provide_libs_ = {config.providelibs().begin(),
                          config.providelibs().end()};
  system_require_libs_ = {config.requirelibs().begin(),
                          config.requirelibs().end()};
}
const std::vector<std::string>& BaseContext::GetSystemProvideLibs() const {
  return system_provide_libs_;
}
const std::vector<std::string>& BaseContext::GetSystemRequireLibs() const {
  return system_require_libs_;
}

}  // namespace modules
}  // namespace linkerconfig
}  // namespace android
