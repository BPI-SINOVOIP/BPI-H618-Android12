package omx_core

import (
    "android/soong/android"
    "android/soong/cc"
    "fmt"
)

func init() {
    android.RegisterModuleType("omx_core_defaults", omxCoreDefaultsFactory)
    android.RegisterModuleType("omx_core_head_defaults", omxCoreHeaderFactory)
}

func omxCoreDefaultsFactory() android.Module {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, omxCoreDefaults)

    return module
}

func omxCoreDefaults(ctx android.LoadHookContext) {
    type props struct {
        Srcs       []string
        Export_include_dirs []string
    }

    p := &props{}

    board := ctx.AConfig().VendorConfig("vendor").String("board")
    switch board{
        case "ceres":
            p.Srcs = append(p.Srcs,"tablelist/ceres/aw_registry_table.c")
        case "venus":
            p.Srcs = append(p.Srcs,"tablelist/venus/aw_registry_table.c")
        case "petrel":
            p.Srcs = append(p.Srcs,"tablelist/petrel/aw_registry_table.c")
        case "dolphin":
            p.Srcs = append(p.Srcs,"tablelist/dolphin/aw_registry_table.c")
        case "tulip":
            p.Srcs = append(p.Srcs,"tablelist/tulip/aw_registry_table.c")
        case "cupid":
            p.Srcs = append(p.Srcs,"tablelist/cupid/aw_registry_table.c")
        case "mercury":
            p.Srcs = append(p.Srcs,"tablelist/mercury/aw_registry_table.c")
        case "apollo":
            p.Srcs = append(p.Srcs,"tablelist/apollo/aw_registry_table.c")
        default:
            p.Srcs = append(p.Srcs,"tablelist/default/aw_registry_table.c")
            fmt.Println("registry_table has NOT been set. Please check it.")
    }

    ctx.AppendProperties(p)
}

func omxCoreHeaderFactory() android.Module {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, omxCoreHeaderDefaults)

    return module
}

func omxCoreHeaderDefaults(ctx android.LoadHookContext) {
    type props struct {
        Export_include_dirs []string
    }

    p := &props{}

    board := ctx.AConfig().VendorConfig("vendor").String("board")
    switch board{
        case "ceres":
            p.Export_include_dirs = append(p.Export_include_dirs,"tablelist/ceres")
        case "venus":
            p.Export_include_dirs = append(p.Export_include_dirs,"tablelist/venus")
        case "petrel":
            p.Export_include_dirs = append(p.Export_include_dirs,"tablelist/petrel")
        case "dolphin":
            p.Export_include_dirs = append(p.Export_include_dirs,"tablelist/dolphin")
        case "tulip":
            p.Export_include_dirs = append(p.Export_include_dirs,"tablelist/tulip")
        case "cupid":
            p.Export_include_dirs = append(p.Export_include_dirs,"tablelist/cupid")
        case "mercury":
            p.Export_include_dirs = append(p.Export_include_dirs,"tablelist/mercury")
        case "apollo":
            p.Export_include_dirs = append(p.Export_include_dirs,"tablelist/apollo")
        default:
            p.Export_include_dirs = append(p.Export_include_dirs,"tablelist/default")
            fmt.Println("registry_table has NOT been set. Please check it.")
    }

    ctx.AppendProperties(p)
}
