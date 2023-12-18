package sstrparser

import (
    "android/soong/android"
    "android/soong/cc"
)

func init() {
    android.RegisterModuleType("sstrparser_defaults", sstrparserDefaultsFactory)
}

func sstrparserDefaultsFactory() android.Module {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, sstrparserDefaults)

    return module
}

func sstrparserDefaults(ctx android.LoadHookContext) {
    type props struct {
        Cflags       []string
    }

    p := &props{}

    playreadytype := ctx.AConfig().VendorConfig("playready").String("playreadytype")
    if playreadytype == "1" {
        p.Cflags = append(p.Cflags, "-DBOARD_PLAYREADY_USE_SECUREOS=0")
    } else if playreadytype == "2" {
        p.Cflags = append(p.Cflags, "-DBOARD_PLAYREADY_USE_SECUREOS=1")
    }

    ctx.AppendProperties(p)
}
