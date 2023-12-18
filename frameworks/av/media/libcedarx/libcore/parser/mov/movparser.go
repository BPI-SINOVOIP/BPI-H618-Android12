package movparser

import (
    "android/soong/android"
    "android/soong/cc"
)

func init() {
    android.RegisterModuleType("movparser_defaults", movparserDefaultsFactory)
}

func movparserDefaultsFactory() android.Module {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, movparserDefaults)

    return module
}

func movparserDefaults(ctx android.LoadHookContext) {
    type props struct {
        Include_dirs []string
        Cflags       []string
        Srcs         []string
        Shared_libs  []string
    }

    p := &props{}

    playreadytype := ctx.AConfig().VendorConfig("playready").String("playreadytype")
    //if playreadytype != "0"	{
    if false {
        p.Cflags = append(p.Cflags, "-DBOARD_USE_PLAYREADY_LICENSE=0",
                                    "-DDRM_BUILD_PROFILE=1100",
                                    "-DARM=1",
                                    "-DBOARD_USE_PLAYREADY=1",
                                    "-DPLAYREADY_BUILD_TYPE=CHK",
                                    "-DBOARD_USE_PLAYREADY_NEW=1")

        p.Include_dirs = append(p.Include_dirs,"frameworks/av/media/libmedia/include")
        p.Include_dirs = append(p.Include_dirs,"frameworks/native/include")
        p.Include_dirs = append(p.Include_dirs,"system/core/base/include")
        p.Include_dirs = append(p.Include_dirs,"frameworks/av/drm/libmediadrm/include") //For R
        p.Include_dirs = append(p.Include_dirs,"frameworks/av/drm/libmediadrm/interface") //For R
        p.Include_dirs = append(p.Include_dirs,"frameworks/av/media/libstagefright//include") //For R
        p.Include_dirs = append(p.Include_dirs,"frameworks/av/media/libstagefright/foundation/include") //For R

        p.Srcs = append(p.Srcs, "CdxMovCrypto.cpp")
        p.Shared_libs = append(p.Shared_libs, "libbinder", "libdrmframework", "libMemAdapter")
    }

    if playreadytype == "1" {
        p.Cflags = append(p.Cflags, "-DBOARD_PLAYREADY_USE_SECUREOS=0")
    } else if playreadytype == "2" {
        p.Cflags = append(p.Cflags, "-DBOARD_PLAYREADY_USE_SECUREOS=1")
    }

    ctx.AppendProperties(p)
}
