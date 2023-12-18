
#ifndef __RESOLUTION_H__

#define HDMI1440_480I       6
#define HDMI1440_576I       21
#define HDMI480P            2
#define HDMI576P            17
#define HDMI720P_50         19
#define HDMI720P_60         4
#define HDMI1080I_50        20
#define HDMI1080I_60        5
#define HDMI1080P_50        31
#define HDMI1080P_60        16
#define HDMI1080P_24        32
#define HDMI1080P_25        33
#define HDMI1080P_30        34
#define HDMI1080P_24_3D_FP  (HDMI1080P_24 + 0x80)
#define HDMI720P_50_3D_FP   (HDMI720P_50  + 0x80)
#define HDMI720P_60_3D_FP   (HDMI720P_60  + 0x80)
#define HDMI3840_2160P_30   95
#define HDMI3840_2160P_25   94
#define HDMI3840_2160P_24   93
#define HDMI4096_2160P_24   98
#define HDMI3840_2160P_50   96
#define HDMI3840_2160P_60   97
#define HDMI4096_2160P_60   107

/*
 * sunxi display output mode define at sunxi_display2.h
 */
enum __disp_tv_mode__
{
    DISP_TV_MOD_480I                = 0,
    DISP_TV_MOD_576I                = 1,
    DISP_TV_MOD_480P                = 2,
    DISP_TV_MOD_576P                = 3,
    DISP_TV_MOD_720P_50HZ           = 4,
    DISP_TV_MOD_720P_60HZ           = 5,
    DISP_TV_MOD_1080I_50HZ          = 6,
    DISP_TV_MOD_1080I_60HZ          = 7,
    DISP_TV_MOD_1080P_24HZ          = 8,
    DISP_TV_MOD_1080P_50HZ          = 9,
    DISP_TV_MOD_1080P_60HZ          = 0xa,
    DISP_TV_MOD_1080P_24HZ_3D_FP    = 0x17,
    DISP_TV_MOD_720P_50HZ_3D_FP     = 0x18,
    DISP_TV_MOD_720P_60HZ_3D_FP     = 0x19,
    DISP_TV_MOD_1080P_25HZ          = 0x1a,
    DISP_TV_MOD_1080P_30HZ          = 0x1b,
    DISP_TV_MOD_PAL                 = 0xb,
    DISP_TV_MOD_PAL_SVIDEO          = 0xc,
    DISP_TV_MOD_NTSC                = 0xe,
    DISP_TV_MOD_NTSC_SVIDEO         = 0xf,
    DISP_TV_MOD_PAL_M               = 0x11,
    DISP_TV_MOD_PAL_M_SVIDEO        = 0x12,
    DISP_TV_MOD_PAL_NC              = 0x14,
    DISP_TV_MOD_PAL_NC_SVIDEO       = 0x15,
    DISP_TV_MOD_3840_2160P_30HZ     = 0x1c,
    DISP_TV_MOD_3840_2160P_25HZ     = 0x1d,
    DISP_TV_MOD_3840_2160P_24HZ     = 0x1e,
    DISP_TV_MOD_4096_2160P_24HZ     = 0x1f,
    DISP_TV_MOD_4096_2160P_25HZ     = 0x20,
    DISP_TV_MOD_4096_2160P_30HZ     = 0x21,
    DISP_TV_MOD_3840_2160P_60HZ     = 0x22,
    DISP_TV_MOD_4096_2160P_60HZ     = 0x23,
    DISP_TV_MOD_3840_2160P_50HZ     = 0x24,
};

/*
 * Video Format Identification Code to sunxi display mode
 */

struct resolution_map {
    int vic;
    int sunxi_disp_mode;
};
const struct resolution_map vic2resolution[] = {
    {HDMI1440_480I     , DISP_TV_MOD_480I},
    {HDMI1440_576I     , DISP_TV_MOD_576I},
    {HDMI480P          , DISP_TV_MOD_480P},
    {HDMI576P          , DISP_TV_MOD_576P},
    {HDMI720P_50       , DISP_TV_MOD_720P_50HZ},
    {HDMI720P_60       , DISP_TV_MOD_720P_60HZ},
    {HDMI1080I_50      , DISP_TV_MOD_1080I_50HZ},
    {HDMI1080I_60      , DISP_TV_MOD_1080I_60HZ},
    {HDMI1080P_50      , DISP_TV_MOD_1080P_50HZ},
    {HDMI1080P_60      , DISP_TV_MOD_1080P_60HZ},
    {HDMI1080P_24      , DISP_TV_MOD_1080P_24HZ},
    {HDMI1080P_25      , DISP_TV_MOD_1080P_25HZ},
    {HDMI1080P_30      , DISP_TV_MOD_1080P_30HZ},
    {HDMI1080P_24_3D_FP, DISP_TV_MOD_1080P_24HZ_3D_FP},
    {HDMI720P_50_3D_FP , DISP_TV_MOD_720P_50HZ_3D_FP},
    {HDMI720P_60_3D_FP , DISP_TV_MOD_720P_60HZ_3D_FP},
    {HDMI3840_2160P_30 , DISP_TV_MOD_3840_2160P_30HZ},
    {HDMI3840_2160P_25 , DISP_TV_MOD_3840_2160P_25HZ},
    {HDMI3840_2160P_24 , DISP_TV_MOD_3840_2160P_24HZ},
//  {HDMI4096_2160P_24 , DISP_TV_MOD_4096_2160P_24HZ},
    {HDMI3840_2160P_50 , DISP_TV_MOD_3840_2160P_50HZ},
    {HDMI3840_2160P_60 , DISP_TV_MOD_3840_2160P_60HZ},
//  {HDMI4096_2160P_60 , DISP_TV_MOD_4096_2160P_60HZ},
};

#endif
