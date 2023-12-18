package cedarx_config

import (
    "android/soong/android"
    "android/soong/cc"
    "fmt"
)
// Add other board config related with Android sdkVersion alike below.
var a100_S_cflags = []string {
    "-DCONF_ANDROID_SUB_VER=1",
    "-DCONF_KERNEL_VERSION_5_4",
    "-DCONF_SIMPLE_ION",
    "-DCONF_GPU_IMG_USE_COMMON_STRUCT",//After ceres, MALI GPU and IMG GPU use the same structure for users
    "-DCONF_KERN_BITWIDE=64",//contradictory with cedarc!!!
    "-DCONF_NEW_BDMV_STREAM",
    "-DCONF_NEW_DISPLAY",
    "-DCONF_H265_4K",
    "-DCONF_ION_HANDLE_INT",
    "-DCONF_VE_PHY_OFFSET=0x00000000",
}
var a100_R_cflags = []string {
    "-DCONF_ANDROID_SUB_VER=1",
    "-DCONF_KERNEL_VERSION_5_4",
    "-DCONF_SIMPLE_ION",
    "-DCONF_GPU_IMG_USE_COMMON_STRUCT",//After ceres, MALI GPU and IMG GPU use the same structure for users
    "-DCONF_KERN_BITWIDE=64",//contradictory with cedarc!!!
    "-DCONF_NEW_BDMV_STREAM",
    "-DCONF_NEW_DISPLAY",
    "-DCONF_H265_4K",
    "-DCONF_ION_HANDLE_INT",
    "-DCONF_VE_PHY_OFFSET=0x00000000",
}
var ceres_cflags = []string {
    "-DCONF_ANDROID_MAJOR_VER=10",
    "-DCONF_ANDROID_SUB_VER=1",
    "-DCONF_KERNEL_VERSION_4_9",
    "-DCONF_GPU_IMG_USE_COMMON_STRUCT",//After ceres, MALI GPU and IMG GPU use the same structure for users
    "-DCONF_KERN_BITWIDE=64",//contradictory with cedarc!!!
    "-DCONF_NEW_BDMV_STREAM",
    "-DCONF_NEW_DISPLAY",
    "-DCONF_H265_4K",
    "-DCONF_ION_HANDLE_INT",
    "-DCONF_VE_PHY_OFFSET=0x00000000",

    "-DCONF_6K",
}
var venus_cflags = []string {
    "-DCONF_ANDROID_MAJOR_VER=10",
    "-DCONF_ANDROID_SUB_VER=1",
    "-DCONF_KERNEL_VERSION_4_9",
    "-DCONF_GPU_MALI",
    "-DCONF_KERN_BITWIDE=64",//contradictory with cedarc!!!
    "-DCONF_NEW_BDMV_STREAM",
    "-DCONF_NEW_DISPLAY",
    "-DCONF_H264_4K_P2P",
    "-DCONF_H265_4K",
    "-DCONF_H265_4K_P2P",
    "-DCONF_ION_HANDLE_INT",
    "-DCONF_VE_PHY_OFFSET=0x00000000",
    //"-DCONF_AFBC_ENABLE",
    "-DCONF_6K",
}
var t3_cflags = []string {
    "-DCONF_ANDROID_MAJOR_VER=10",
    "-DCONF_ANDROID_SUB_VER=1",
    "-DCONF_KERNEL_VERSION_4_9",
    "-DCONF_GPU_MALI",
    "-DCONF_KERN_BITWIDE=32",//contradictory with cedarc!!!
    "-DCONF_NEW_BDMV_STREAM",
    "-DCONF_NEW_DISPLAY",
    "-DCONF_ION_HANDLE_INT",
    "-DCONF_VE_PHY_OFFSET=0x40000000",
    "-DCONF_WMV12VP6_1080P",
    //"-DCONF_AFBC_ENABLE",
    "-DCONF_6K",
}

var dolphin_cflags = []string {
    "-DCONF_ANDROID_MAJOR_VER=10",
    "-DCONF_ANDROID_SUB_VER=1",
    "-DCONF_KERNEL_VERSION_4_9",
    "-DCONF_GPU_MALI",
    "-DCONF_KERN_BITWIDE=32",
    "-DCONF_NEW_BDMV_STREAM",
    "-DCONF_NEW_DISPLAY",
    "-DCONF_ION_HANDLE_INT",
    "-DCONF_VE_PHY_OFFSET=0x40000000",
    "-DCONF_NEW_BDMV_STREAM",
    "-DCONF_PRODUCT_STB",
    "-DCONF_PTS_TOSF",
    "-DCONF_H265_4K_P2P",
    "-DCONF_H265_4K",
    "-DCONF_3D_ENABLE",
    "-DCONF_6K",
    "-DCONF_HIGH_QUALITY_DEINTERLACE" ,//deinterlace threshold <high-2048:default-720 >
}

var cupid_cflags = []string {
    "-DCONF_ANDROID_MAJOR_VER=10",
    "-DCONF_ANDROID_SUB_VER=1",
    "-DCONF_KERNEL_VERSION_4_9",
    "-DCONF_GPU_MALI",
    "-DCONF_KERN_BITWIDE=64",
    "-DCONF_NEW_BDMV_STREAM",
    "-DCONF_NEW_DISPLAY",
    "-DCONF_ION_HANDLE_INT",
    "-DCONF_VE_PHY_OFFSET=0x00000000",
    "-DCONF_NEW_BDMV_STREAM",
    "-DCONF_PRODUCT_STB",
    "-DCONF_PTS_TOSF",
    "-DCONF_H265_4K_P2P",
    "-DCONF_H264_4K_P2P",
    "-DCONF_VP9_4K_P2P",
    "-DCONF_AVS2_4K_P2P",
    "-DCONF_H265_4K",
    "-DCONF_3D_ENABLE",
    "-DCONF_USE_IOMMU",
    "-DCONF_AFBC_ENABLE",
    "-DCONF_HIGH_DYNAMIC_RANGE_ENABLE",
    "-DCONF_DI_300_SUPPORT",
    "-DCONF_SCALE_DOWN",
    "-DCONF_6K",
    "-DCONF_HIGH_QUALITY_DEINTERLACE" ,//deinterlace threshold <high-2048:default-720 >
}

var apollo_S_cflags = []string {
    "-DCONF_ANDROID_MAJOR_VER=12",
    "-DCONF_ANDROID_SUB_VER=1",
    "-DCONF_KERNEL_VERSION_5_4",
    "-DCONF_NEW_BDMV_STREAM",
    "-DCONF_DI_300_SUPPORT",
    "-DCONF_NEW_DISPLAY",
    "-DCONF_PRODUCT_STB",
    "-DCONF_PTS_TOSF",
    "-DCONF_H264_4K_P2P",
    "-DCONF_H265_4K",
    "-DCONF_H265_4K_P2P",
    "-DCONF_AVS2_4K_P2P",
    "-DCONF_VP9_4K_P2P",
    "-DCONF_ION_HANDLE_INT",
    "-DCONF_USE_IOMMU",
    "-DCONF_GPU_MALI",
    "-DCONF_KERN_BITWIDE=64",
    "-DCONF_3D_ENABLE",
    "-DCONF_AFBC_ENABLE",
    "-DCONF_HIGH_DYNAMIC_RANGE_ENABLE",
    "-DCONF_SCALE_DOWN",
    "-DCONF_6K",
    "-DCONF_SIMPLE_ION",
    "-DCONF_HIGH_QUALITY_DEINTERLACE" ,//deinterlace threshold <high-2048:default-720 >
    "-DCONF_VE_PHY_OFFSET=0x00000000",
}

var mercury_S_cflags = []string {
    "-DCONF_ANDROID_MAJOR_VER=12",
    "-DCONF_ANDROID_SUB_VER=1",
    "-DCONF_KERNEL_VERSION_5_4",
    "-DCONF_NEW_BDMV_STREAM",
    "-DCONF_DI_300_SUPPORT",
    "-DCONF_NEW_DISPLAY",
    "-DCONF_PRODUCT_STB",
    "-DCONF_PTS_TOSF",
    //"-DCONF_H264_4K_P2P",
    "-DCONF_H265_4K",
    //"-DCONF_H265_4K_P2P",
    //"-DCONF_AVS2_4K_P2P",
    //"-DCONF_VP9_4K_P2P",
    "-DCONF_ION_HANDLE_INT",
    "-DCONF_USE_IOMMU",
    "-DCONF_GPU_MALI",
    "-DCONF_KERN_BITWIDE=64",
    "-DCONF_3D_ENABLE",
    "-DCONF_AFBC_ENABLE",
    "-DCONF_HIGH_DYNAMIC_RANGE_ENABLE",
    "-DCONF_SCALE_DOWN",
    "-DCONF_6K",
    "-DCONF_SIMPLE_ION",
    "-DCONF_HIGH_QUALITY_DEINTERLACE" ,//deinterlace threshold <high-2048:default-720 >
    "-DCONF_VE_PHY_OFFSET=0x00000000",
}

var mercury_cflags = []string {
    "-DCONF_ANDROID_MAJOR_VER=10",
    "-DCONF_ANDROID_SUB_VER=1",
    "-DCONF_KERNEL_VERSION_4_9",
    "-DCONF_GPU_MALI",
    "-DCONF_KERN_BITWIDE=64",
    "-DCONF_NEW_BDMV_STREAM",
    "-DCONF_NEW_DISPLAY",
    "-DCONF_ION_HANDLE_INT",
    "-DCONF_VE_PHY_OFFSET=0x00000000",
    "-DCONF_NEW_BDMV_STREAM",
    "-DCONF_PRODUCT_STB",
    "-DCONF_PTS_TOSF",
    //"-DCONF_H265_4K_P2P",
    //"-DCONF_H264_4K_P2P",
    //"-DCONF_VP9_4K_P2P",
    //"-DCONF_AVS2_4K_P2P",
    "-DCONF_H265_4K",
    "-DCONF_3D_ENABLE",
    "-DCONF_USE_IOMMU",
    "-DCONF_AFBC_ENABLE",
    "-DCONF_HIGH_DYNAMIC_RANGE_ENABLE",
    "-DCONF_DI_300_SUPPORT",
    "-DCONF_SCALE_DOWN",
}

var default_cflags = []string {
    "-DCONF_ANDROID_MAJOR_VER=10",
    "-DCONF_ANDROID_SUB_VER=1",
    "-DCONF_KERNEL_VERSION_4_9",
    "-DCONF_GPU_MALI",
    "-DCONF_KERN_BITWIDE=32",//contradictory with cedarc!!!
    "-DCONF_NEW_BDMV_STREAM",
    "-DCONF_NEW_DISPLAY",
    "-DCONF_ION_HANDLE_INT",
    "-DCONF_VE_PHY_OFFSET=0x40000000",
    //"-DCONF_AFBC_ENABLE",
    "-DCONF_6K",
}

type cdxCfgType struct {
    board string
    cflags []string
}

type cdxCfgTableType struct {
    sdkVersion int
    cflags []string
    boardCfg []cdxCfgType
}

var cdxCfgTable []cdxCfgTableType = []cdxCfgTableType {
    cdxCfgTableType {
        31,
        []string {"-DCONF_S_AND_NEWER", "-DCONF_ANDROID_MAJOR_VER=12"},
        []cdxCfgType {
            cdxCfgType {"venus",   venus_cflags},
            cdxCfgType {"ceres",   a100_S_cflags},
            cdxCfgType {"epic",    a100_S_cflags},
            cdxCfgType {"t3",      t3_cflags},
            cdxCfgType {"dolphin", dolphin_cflags},
            cdxCfgType {"cupid",   cupid_cflags},
            cdxCfgType {"apollo",  apollo_S_cflags},
            cdxCfgType {"mercury",  mercury_S_cflags},
        },
    },

    cdxCfgTableType {
        30,
        []string {"-DCONF_Q_AND_NEWER", "-DCONF_R_AND_NEWER", "-DCONF_ANDROID_MAJOR_VER=11"},
        []cdxCfgType {
            cdxCfgType {"venus",   venus_cflags},
            cdxCfgType {"ceres",   a100_R_cflags},
            cdxCfgType {"t3",      t3_cflags},
            cdxCfgType {"dolphin", dolphin_cflags},
            cdxCfgType {"cupid",   cupid_cflags},
        },
    },

    cdxCfgTableType {
        29,
        []string {"-DCONF_Q_AND_NEWER", "-DCONF_ANDROID_MAJOR_VER=10"},
        []cdxCfgType {
            cdxCfgType {"venus",   venus_cflags},
            cdxCfgType {"ceres",   ceres_cflags},
            cdxCfgType {"t3",      t3_cflags},
            cdxCfgType {"dolphin", dolphin_cflags},
            cdxCfgType {"cupid",   cupid_cflags},
            cdxCfgType {"mercury", mercury_cflags},
        },
    },

    cdxCfgTableType {sdkVersion: 28, cflags: []string {"-DCONF_PIE_AND_NEWER"},},           // after P
    cdxCfgTableType {sdkVersion: 26, cflags: []string {"-DCONF_OREO_AND_NEWER"},},          // after O
    cdxCfgTableType {sdkVersion: 24, cflags: []string {"-DCONF_NOUGAT_AND_NEWER"},},        // after N
    cdxCfgTableType {sdkVersion: 23, cflags: []string {"-DCONF_MARSHMALLOW_AND_NEWER"},},   // after M
    cdxCfgTableType {sdkVersion: 21, cflags: []string {"-DCONF_LOLLIPOP_AND_NEWER"},},      // after L
    cdxCfgTableType {sdkVersion: 19, cflags: []string {"-DCONF_KITKAT_AND_NEWER"},},        // after KK
    cdxCfgTableType {sdkVersion: 17, cflags: []string {"-DCONF_JB42_AND_NEWER"},},          // after JB42
}

func globalDefaults(ctx android.BaseContext) ([]string) {
    var cppflags []string

    sdkVersion := ctx.AConfig().PlatformSdkVersion().FinalOrFutureInt()
    platformconfig := "NOT SET"

    board   := ctx.AConfig().VendorConfig("vendor").String("board")
    cppflags = append(cppflags,"-DTARGET_BOARD_PLATFORM=" + board)

    for i := 0; i < len(cdxCfgTable); i++ {
        if sdkVersion >= cdxCfgTable[i].sdkVersion && cdxCfgTable[i].sdkVersion > 28 {
            cppflags = append(cppflags, cdxCfgTable[i].cflags...)
            for j := 0; j < len(cdxCfgTable[i].boardCfg); j++ {
                if board == cdxCfgTable[i].boardCfg[j].board {
                    cppflags = append(cppflags, cdxCfgTable[i].boardCfg[j].cflags...)
                    platformconfig = "YES"
                    break
                }
            }
            break
        }
    }

    for i := 0; i < len(cdxCfgTable); i++ {
        if sdkVersion >= cdxCfgTable[i].sdkVersion && cdxCfgTable[i].sdkVersion <= 28 {
            cppflags = append(cppflags, cdxCfgTable[i].cflags...)
        }
    }

    if platformconfig == "NOT SET" {
        cppflags = append(cppflags, default_cflags...)
    }

    config  := ctx.Config().VendorConfig("gpu").String("public_include_file")
    cppflags = append(cppflags, "-DGPU_PUBLIC_INCLUDE=\"" + config + "\"")
    fmt.Printf("cedarx-config: sdkVersion[%d], board[%s], platformconfig[%s]\n",
                sdkVersion, board, platformconfig)

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
    android.RegisterModuleType("cedarx_config_defaults", configDefaultsFactory)
}
