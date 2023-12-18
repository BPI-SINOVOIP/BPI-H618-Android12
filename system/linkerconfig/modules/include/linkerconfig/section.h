/*
 * Copyright (C) 2019 The Android Open Source Project
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
#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <android-base/result.h>

#include "linkerconfig/basecontext.h"
#include "linkerconfig/configwriter.h"
#include "linkerconfig/namespace.h"

namespace android {
namespace linkerconfig {
namespace modules {

// LibProvider is a provider for alias of library requirements.
// When "foo" namespace requires "alias" (formatted ":name"),
// you would expect
//   foo.GetLink(<ns>).AddSharedLib(<shared_libs);
// which is equivalent to
//   namespace.foo.link.<ns>.shared_libs = <shared_libs>
// The referenced namespace (<ns>) is created via <ns_builder> and added
// in the current section.
struct LibProvider {
  std::string ns;
  std::function<Namespace()> ns_builder;
  std::vector<std::string> shared_libs;
};

using LibProviders = std::unordered_map<std::string, LibProvider>;

class Section {
 public:
  Section(std::string name, std::vector<Namespace> namespaces)
      : name_(std::move(name)), namespaces_(std::move(namespaces)) {
  }

  Section(const Section&) = delete;
  Section(Section&&) = default;

  void WriteConfig(ConfigWriter& writer);
  std::vector<std::string> GetBinaryPaths();
  std::string GetName();

  android::base::Result<void> Resolve(const BaseContext& ctx,
                                      const LibProviders& lib_providers = {});
  Namespace* GetNamespace(const std::string& namespace_name);

  template <class _Function>
  void ForEachNamespaces(_Function f) {
    for (auto& ns : namespaces_) {
      f(ns);
    }
  }

 private:
  const std::string name_;
  std::vector<Namespace> namespaces_;
};
}  // namespace modules
}  // namespace linkerconfig
}  // namespace android
