package cedarc_config

import (
    "android/soong/android"
    "android/soong/cc"
    "fmt"
)

// Add other board config related with Android sdkVersion alike below.
var a50_Q_cflags = []string {
    "-DCONF_KERNEL_VERSION_4_9",
    "-DCONF_MALI_GPU",
    "-DCONF_USE_IOMMU",
    "-DCONF_KERN_BITWIDE=32",
    "-DCONFIG_VE_IPC_ENABLE",
    "-DGPU_ALIGN_STRIDE=32",
}

var a100_Q_cflags = []string {
    "-DCONF_KERNEL_VERSION_4_9",
    "-DCONF_IMG_GPU_USE_COMMON_STRUCT",//After ceres, MALI GPU and IMG GPU use the same structure for users
    "-DCONF_USE_IOMMU",
    "-DCONF_KERN_BITWIDE=64",
    "-DCONFIG_VE_IPC_ENABLE",
    "-DGPU_ALIGN_STRIDE=32",
    "-DCONF_VE_FREQ_ENABLE_SETUP",
    "-DCONF_CERES_VE_FREQ_ENABLE_SETUP",
}

var t3_Q_cflags = []string {
    "-DCONF_KERNEL_VERSION_4_9",
    "-DCONF_MALI_GPU",
    "-DCONF_KERN_BITWIDE=32",
    "-DCONFIG_VE_IPC_ENABLE",
    "-DGPU_ALIGN_STRIDE=32",
}

var dolphin_Q_cflags = []string {
    "-DCONF_KERNEL_VERSION_4_9",
    "-DCONF_MALI_GPU",
    "-DCONF_ENABLE_OPENMAX_DI_FUNCTION",
    "-DCONF_KERN_BITWIDE=32",
    "-DCONFIG_VE_IPC_ENABLE",
    "-DOMX_DROP_B_FRAME_4K",
    "-DGPU_ALIGN_STRIDE=32",
}

//CONF_VP9_DEC: 0-libawvp9soft; 1-libawvp9Hw; 2-libawvp9HwAL
var cupid_Q_cflags = []string {
    "-DCONF_KERNEL_VERSION_4_9",
    "-DCONF_MALI_GPU",
    "-DCONF_USE_IOMMU",
    "-DCONF_ENABLE_OPENMAX_DI_FUNCTION",
    "-DCONF_KERN_BITWIDE=64",
    "-DCONF_AFBC_ENABLE",
    "-DCONF_HIGH_DYNAMIC_RANGE_ENABLE",
    "-DCONFIG_VE_IPC_ENABLE",
    "-DCONF_VP9_DEC=2",
    "-DGPU_ALIGN_STRIDE=32",
    "-DCONF_DI_300_SUPPORT",
}

var mercury_Q_cflags = []string {
    "-DCONF_KERNEL_VERSION_4_9",
    "-DCONF_MALI_GPU",
    "-DCONF_USE_IOMMU",
    "-DCONF_ENABLE_OPENMAX_DI_FUNCTION",
    "-DCONF_KERN_BITWIDE=64",
    "-DCONF_AFBC_ENABLE",
    "-DCONF_HIGH_DYNAMIC_RANGE_ENABLE",
    "-DCONFIG_VE_IPC_ENABLE",
    "-DCONF_VP9_DEC=2",
    "-DGPU_ALIGN_STRIDE=32",
    "-DCONF_DI_300_SUPPORT",
    "-DCONF_ENABLE_SCALEDOWN_WHEN_RESOLUTION_MOER_THAN_1080P",
}

var a100_R_cflags = []string {
    "-DCONF_KERNEL_VERSION_5_4",
    "-DCONF_SIMPLE_ION",
    "-DCONF_IMG_GPU_USE_COMMON_STRUCT",//After ceres, MALI GPU and IMG GPU use the same structure for users
    "-DCONF_USE_IOMMU",
    "-DCONF_KERN_BITWIDE=64",
    "-DCONFIG_VE_IPC_ENABLE",
    "-DGPU_ALIGN_STRIDE=32",
    "-DCONF_VE_FREQ_ENABLE_SETUP",
    "-DCONF_CERES_VE_FREQ_ENABLE_SETUP",
}

//CONF_VP9_DEC: 0-libawvp9soft; 1-libawvp9Hw; 2-libawvp9HwAL
var apollo_S_cflags = []string {
    "-DCONF_KERNEL_VERSION_5_4",
    "-DCONF_MALI_GPU",
    "-DCONF_USE_IOMMU",
    "-DCONF_ENABLE_OPENMAX_DI_FUNCTION",
    "-DCONF_KERN_BITWIDE=64",
    "-DCONF_AFBC_ENABLE",
    "-DCONF_HIGH_DYNAMIC_RANGE_ENABLE",
    "-DCONFIG_VE_IPC_ENABLE",
    "-DCONF_VP9_DEC=2",
    "-DCONF_SIMPLE_ION",
    "-DGPU_ALIGN_STRIDE=32",
    "-DCONF_DI_300_SUPPORT",
    "-DCONF_VE_MULTIPLE_OUTPUT",
    "-DCONF_SUPPORT_8K",
}

var mercury_S_cflags = []string {
    "-DCONF_KERNEL_VERSION_5_4",
    "-DCONF_MALI_GPU",
    "-DCONF_USE_IOMMU",
    "-DCONF_ENABLE_OPENMAX_DI_FUNCTION",
    "-DCONF_KERN_BITWIDE=64",
    "-DCONF_AFBC_ENABLE",
    "-DCONF_HIGH_DYNAMIC_RANGE_ENABLE",
    "-DCONFIG_VE_IPC_ENABLE",
    "-DCONF_VP9_DEC=2",
    "-DCONF_SIMPLE_ION",
    "-DGPU_ALIGN_STRIDE=32",
    "-DCONF_DI_300_SUPPORT",
    "-DCONF_VE_MULTIPLE_OUTPUT",
}

var default_cflags = []string {
    "-DCONF_KERNEL_VERSION_4_9",
    "-DCONF_MALI_GPU",
    "-DCONF_USE_IOMMU",
    "-DCONF_KERN_BITWIDE=32",
    "-DCONFIG_VE_IPC_ENABLE",
    "-DGPU_ALIGN_STRIDE=32",
    //"-DCONF_ENABLE_OPENMAX_DI_FUNCTION", //if open di support
    //"-DCONF_DI_V2X_SUPPORT", //if di processes 3 input pictures
    //"-DCONF_DI_300_SUPPORT", //if di300 supported
}

type cdcCfgType struct {
    board string
    cflags []string
}

type cdcCfgTableType struct {
    sdkVersion int
    cflags []string
    boardCfg []cdcCfgType
}

var cdcCfgTable []cdcCfgTableType = []cdcCfgTableType {
    cdcCfgTableType {
        31,
        []string {"-DCONF_Q_AND_NEWER", "-DCONF_R_AND_NEWER"},
        []cdcCfgType {
            cdcCfgType {"ceres",   a100_R_cflags},
            cdcCfgType {"epic",    a100_R_cflags},
            cdcCfgType {"apollo",  apollo_S_cflags},
            cdcCfgType {"mercury", mercury_S_cflags},
        },
    },

    cdcCfgTableType {
        30,
        []string {"-DCONF_Q_AND_NEWER", "-DCONF_R_AND_NEWER"},
        []cdcCfgType {
            cdcCfgType {"ceres",   a100_R_cflags},
        },
    },

    cdcCfgTableType {
        29,
        []string {"-DCONF_Q_AND_NEWER",},
        []cdcCfgType {
            cdcCfgType {"venus",   a50_Q_cflags},
            cdcCfgType {"ceres",   a100_Q_cflags},
            cdcCfgType {"t3",      t3_Q_cflags},
            cdcCfgType {"dolphin", dolphin_Q_cflags},
            cdcCfgType {"cupid",   cupid_Q_cflags},
            cdcCfgType {"mercury", mercury_Q_cflags},
        },
    },

    cdcCfgTableType {sdkVersion: 28, cflags: []string {"-DCONF_PIE_AND_NEWER"},},           // after P
    cdcCfgTableType {sdkVersion: 26, cflags: []string {"-DCONF_OREO_AND_NEWER"},},          // after O
    cdcCfgTableType {sdkVersion: 24, cflags: []string {"-DCONF_NOUGAT_AND_NEWER"},},        // after N
    cdcCfgTableType {sdkVersion: 23, cflags: []string {"-DCONF_MARSHMALLOW_AND_NEWER"},},   // after M
    cdcCfgTableType {sdkVersion: 21, cflags: []string {"-DCONF_LOLLIPOP_AND_NEWER"},},      // after L
    cdcCfgTableType {sdkVersion: 19, cflags: []string {"-DCONF_KITKAT_AND_NEWER"},},        // after KK
    cdcCfgTableType {sdkVersion: 17, cflags: []string {"-DCONF_JB42_AND_NEWER"},},          // after JB42
}

func globalDefaults(ctx android.BaseContext) ([]string) {
    var cppflags []string

    sdkVersion := ctx.AConfig().PlatformSdkVersion().FinalOrFutureInt() //PlatformSdkVersionInt()
    //sdkVersion := ctx.AConfig().PlatformSdkVersionInt()
    platformconfig := "NOT SET"

    board := ctx.AConfig().VendorConfig("vendor").String("board")
    cppflags = append(cppflags,"-DTARGET_BOARD_PLATFORM="+board)

    for i := 0; i < len(cdcCfgTable); i++ {
        if sdkVersion >= cdcCfgTable[i].sdkVersion && cdcCfgTable[i].sdkVersion > 28 {
            cppflags = append(cppflags, cdcCfgTable[i].cflags...)
            for j := 0; j < len(cdcCfgTable[i].boardCfg); j++ {
                if board == cdcCfgTable[i].boardCfg[j].board {
                    cppflags = append(cppflags, cdcCfgTable[i].boardCfg[j].cflags...)
                    platformconfig = "YES"
                    break
                }
            }
            break
        }
    }

    for i := 0; i < len(cdcCfgTable); i++ {
        if sdkVersion >= cdcCfgTable[i].sdkVersion && cdcCfgTable[i].sdkVersion <= 28 {
            cppflags = append(cppflags, cdcCfgTable[i].cflags...)
        }
    }

    if platformconfig == "NOT SET" {
        cppflags = append(cppflags, default_cflags...)
    }

    config  := ctx.Config().VendorConfig("gpu").String("public_include_file")
    cppflags = append(cppflags, "-DGPU_PUBLIC_INCLUDE=\"" + config + "\"")

    cryptolevel := ctx.AConfig().VendorConfig("widevine").String("cryptolevel")
    playreadytype := ctx.AConfig().VendorConfig("playready").String("playreadytype")
    if cryptolevel == "1" || playreadytype == "2"{
         cppflags = append(cppflags,"-DPLATFORM_SURPPORT_SECURE_OS=1",
                                    "-DSECURE_OS_OPTEE=1",
                                    "-DADJUST_ADDRESS_FOR_SECURE_OS_OPTEE=1")
    }

    //cppflags = append(cppflags,"-DCONF_ARMV7_A_NEON")

    fmt.Printf("cedarc-config: sdkVersion[%d], board[%s], platformconfig[%s], " +
               "cryptolevel[%s], playreadytype[%s]\n",
                sdkVersion, board, platformconfig, cryptolevel, playreadytype)

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
    android.RegisterModuleType("config_defaults", configDefaultsFactory)
}
