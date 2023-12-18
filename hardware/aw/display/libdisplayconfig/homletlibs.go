// Copyright (C) 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
package homletlibs

import (
    "android/soong/android"
    "android/soong/cc"
)

func init() {
    android.RegisterModuleType(
        "rule_homletlibs_defaults",
        func() android.Module {
            module := cc.DefaultsFactory()
            android.AddLoadHook(module, sampleLoadHook)
            return module
        })
    }

func sampleLoadHook(ctx android.LoadHookContext) {
    type props struct {
        Shared_libs  []string
    }

    p := &props{}

    var config string
    // string of TARGET_PLATFORM
    config = ctx.Config().VendorConfig("vendor").String("platform")
    if (config == "homlet") {
        var sharedlib []string
        sharedlib = append(sharedlib, "libdisplayd")
        sharedlib = append(sharedlib, "libedid")
        p.Shared_libs = sharedlib
    }

    ctx.AppendProperties(p)
}

