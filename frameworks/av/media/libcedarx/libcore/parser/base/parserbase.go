package parserbase

import (
    "android/soong/android"
    "android/soong/cc"
)

func init() {
    android.RegisterModuleType("parserbase_defaults", parserbaseDefaultsFactory)
}

func parserbaseDefaultsFactory() android.Module {
    module := cc.DefaultsFactory()
    android.AddLoadHook(module, parserbaseDefaults)

    return module
}

func parserbaseDefaults(ctx android.LoadHookContext) {
    type props struct {
        Shared_libs  []string
        Static_libs  []string
    }

    p := &props{}
    playreadytype := ctx.AConfig().VendorConfig("playready").String("playreadytype")

    if playreadytype != "0"	{
        p.Static_libs = append(p.Static_libs, "libaw_env",)
        p.Shared_libs = append(p.Shared_libs, "libdrmframework",
                                       "libbinder",
                                       "libmedia",
                                       "libstagefright_foundation",
                                       "libmediadrm",
                                       "libMemAdapter")
	}


    ctx.AppendProperties(p)
}
