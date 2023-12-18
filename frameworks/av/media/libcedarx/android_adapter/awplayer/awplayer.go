package awplayer

import (
    "android/soong/android"
    "android/soong/cc"
)

func init() {
    android.RegisterModuleType("awplayer_defaults", awplayerDefaultsFactory)
}

func awplayerDefaultsFactory() android.Module {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, awplayerDefaults)

    return module
}

func awplayerDefaults(ctx android.LoadHookContext) {
    type props struct {
        Srcs         []string
    }

    p := &props{}
    playreadytype := ctx.AConfig().VendorConfig("playready").String("playreadytype")
    if playreadytype != "0"	{
        p.Srcs = append(p.Srcs, "awPlayReadyLicense.cpp")
    }
    ctx.AppendProperties(p)
}
