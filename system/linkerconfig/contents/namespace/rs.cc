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

// This namespace is exclusively for Renderscript internal libraries. This
// namespace has slightly looser restriction than the vndk namespace because of
// the genuine characteristics of Renderscript; /data is in the permitted path
// to load the compiled *.so file and libmediandk.so can be used here.

#include "linkerconfig/namespacebuilder.h"

using android::linkerconfig::modules::Namespace;

namespace android {
namespace linkerconfig {
namespace contents {
Namespace BuildRsNamespace([[maybe_unused]] const Context& ctx) {
  Namespace ns(
      "rs", /*is_isolated=*/!ctx.IsUnrestrictedSection(), /*is_visible=*/true);

  ns.AddSearchPath("/odm/${LIB}/vndk-sp");
  ns.AddSearchPath("/vendor/${LIB}/vndk-sp");
  ns.AddSearchPath("/apex/com.android.vndk.v" + Var("VENDOR_VNDK_VERSION") +
                   "/${LIB}");
  ns.AddSearchPath("/odm/${LIB}");
  ns.AddSearchPath("/vendor/${LIB}");

  ns.AddPermittedPath("/odm/${LIB}");
  ns.AddPermittedPath("/vendor/${LIB}");
  ns.AddPermittedPath("/system/vendor/${LIB}");
  ns.AddPermittedPath("/data");

  // Private LLNDK libs (e.g. libft2.so) are exceptionally allowed to this
  // namespace because RS framework libs are using them.
  ns.GetLink(ctx.GetSystemNamespaceName())
      .AddSharedLib({Var("LLNDK_LIBRARIES_VENDOR"),
                     Var("PRIVATE_LLNDK_LIBRARIES_VENDOR", "")});

  ns.AddRequires(std::vector{"libneuralnetworks.so"});

  return ns;
}
}  // namespace contents
}  // namespace linkerconfig
}  // namespace android
