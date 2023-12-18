package tvdvideo_config

import (
    "android/soong/android"
    "android/soong/java"
    "fmt"
)

func init() {
    android.RegisterModuleType("tvdvideo_defaults", tvdvideoDefaultsFactory)
}

func tvdvideoDefaultsFactory() android.Module {
    module := java.DefaultsFactory()
    android.AddLoadHook(module, tvdvideoDefaults)
    return module
}

func tvdvideoDefaults(ctx android.LoadHookContext) {
    type props struct {
        Srcs    []string
        Static_libs     []string
    }
    p := &props{}

    platform := ctx.AConfig().VendorConfig("vendor").String("characteristics")
    if (platform == "tv") {
        p.Static_libs = append(p.Static_libs,"pqcontrol")
        p.Srcs = append(p.Srcs,"src/com/softwinner/TvdVideo/TvDisplayController.java")
    }
    fmt.Printf("tvdvideo_config: platform[%s]\n",platform);
    ctx.AppendProperties(p)
}