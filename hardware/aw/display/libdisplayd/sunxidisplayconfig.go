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
package sunxidisplayconfig

import (
    "android/soong/android"
    "android/soong/cc"
)

func init() {
    android.RegisterModuleType(
        "rule_sunxidisplayconfig_defaults",
        func() android.Module {
            module := cc.DefaultsFactory()
            android.AddLoadHook(module, sampleLoadHook)
            return module
        })
    }

func sampleLoadHook(ctx android.LoadHookContext) {
    type props struct {
        Cflags       []string
        Include_dirs []string
    }

    p := &props{}

    var config string
    var cflags string
    // string of TARGET_BOARD_PLATFORM
    config = ctx.Config().VendorConfig("vendor").String("board")
    cflags = "-D_board_" + config + "_";
    p.Cflags = append(p.Cflags, cflags)

    cflags = "-DTARGET_BOARD_PLATFORM=" + config;
    p.Cflags = append(p.Cflags, cflags)

    // string of TARGET_PLATFORM
    //bpi, both tablet and homlet use homlet config
    //config = ctx.Config().VendorConfig("vendor").String("platform")
    config = "homlet"
    cflags = "-D_platform_" + config + "_";
    p.Cflags = append(p.Cflags, cflags)

    // string of public_include_file
    config = ctx.Config().VendorConfig("gpu").String("public_include_file")
    p.Cflags = append(p.Cflags, "-DGPU_PUBLIC_INCLUDE=\"" + config + "\"")

    // strint of writebackMode
    config = ctx.Config().VendorConfig("disp").String("writebackMode")
    if len(config) > 0 {
        p.Cflags = append(p.Cflags, "-DWRITE_BACK_MODE=\"" + config + "\"")
    }

    sdkVersion := ctx.AConfig().PlatformSdkVersion().FinalOrFutureInt()
    if sdkVersion >= 30 {
        // after Android-R, the sw_sync can not access from debugfs,
        // use sunxi syncfence for instead.
        p.Cflags = append(p.Cflags, "-DSUNXI_SYNCFENCE_ENABLED")
    }

    ctx.AppendProperties(p)
}

