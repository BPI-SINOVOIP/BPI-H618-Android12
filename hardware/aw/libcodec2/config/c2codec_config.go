package c2codec_config

import (
    "android/soong/android"
    "android/soong/cc"
    "fmt"
)

func globalDefaults(ctx android.BaseContext) ([]string) {
    var cppflags []string

    sdkVersion := ctx.AConfig().PlatformSdkVersion().FinalOrFutureInt()
    fmt.Printf("codec2-config: sdkVersion[%d]\n", sdkVersion)

    config  := ctx.Config().VendorConfig("gpu").String("public_include_file")
    cppflags = append(cppflags, "-DGPU_PUBLIC_INCLUDE=\"" + config + "\"")
    cppflags = append(cppflags, "-DCONF_USE_IOMMU")
    cppflags = append(cppflags, "-DCONF_ARMV7_A_NEON")

    switch sdkVersion { //after Q
        case 29:
           cppflags = append(cppflags,"-DCONF_Q_AND_NEWER")
        case 30:
           cppflags = append(cppflags,"-DCONF_R_AND_NEWER")
        case 31:
           cppflags = append(cppflags,"-DCONF_S_AND_NEWER")
    }
    return cppflags
}

func configDefaults(ctx android.LoadHookContext) {
    type props struct {
        Cflags []string
    }
    p := &props{}
    p.Cflags = globalDefaults(ctx)
    ctx.AppendProperties(p)
}

func configDefaultsFactory() android.Module {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, configDefaults)
    return module
}

func init() {
    android.RegisterModuleType("codec2_defaults", configDefaultsFactory)
}
