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
package mpsproduct

import (
    "android/soong/android"
    "android/soong/cc"
)

func init() {
    android.RegisterModuleType("mpsproduct_defaults", productDefaultsFactory)
}

func productDefaultsFactory() android.Module {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, productDefaults)

    return module
}

func productDefaults(ctx android.LoadHookContext) {
    type props struct {
        Cflags       []string
        Include_dirs []string
        Shared_libs []string
    }

    p := &props{}

    if ctx.AConfig().VendorConfig("vendor").String("platform") != "" {
        //bpi, both tablet and homlet support
	//var keyval string
        //keyval = ctx.AConfig().VendorConfig("vendor").String("platform")
        //if keyval == "homlet" {
            p.Cflags = append(p.Cflags,"-DSUPPORT_BDMV")
            p.Include_dirs = append(p.Include_dirs,"vendor/aw/homlet/framework/isomountmanager/include")
            p.Shared_libs = append(p.Shared_libs,"libisomountmanagerservice")
        //}
    }

    ctx.AppendProperties(p)
}
