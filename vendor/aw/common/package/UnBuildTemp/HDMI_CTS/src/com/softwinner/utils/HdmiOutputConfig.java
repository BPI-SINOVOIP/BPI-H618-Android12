
package com.softwinner.utils;

/*
 * HDMI output mode valu:
 *  DISP_TV_MOD_480I                = 0x00;
 *  DISP_TV_MOD_576I                = 0x01;
 *  DISP_TV_MOD_480P                = 0x02;
 *  DISP_TV_MOD_576P                = 0x03;
 *  DISP_TV_MOD_720P_50HZ           = 0x04;
 *  DISP_TV_MOD_720P_60HZ           = 0x05;
 *  DISP_TV_MOD_1080I_50HZ          = 0x06;
 *  DISP_TV_MOD_1080I_60HZ          = 0x07;
 *  DISP_TV_MOD_1080P_24HZ          = 0x08;
 *  DISP_TV_MOD_1080P_50HZ          = 0x09;
 *  DISP_TV_MOD_1080P_60HZ          = 0x0a;
 *  DISP_TV_MOD_1080P_24HZ_3D_FP    = 0x17;
 *  DISP_TV_MOD_720P_50HZ_3D_FP     = 0x18;
 *  DISP_TV_MOD_720P_60HZ_3D_FP     = 0x19;
 *  DISP_TV_MOD_1080P_25HZ          = 0x1a;
 *  DISP_TV_MOD_1080P_30HZ          = 0x1b;
 *  DISP_TV_MOD_3840_2160P_30HZ     = 0x1c;
 *  DISP_TV_MOD_3840_2160P_25HZ     = 0x1d;
 *  DISP_TV_MOD_3840_2160P_24HZ     = 0x1e;
 *  DISP_TV_MOD_4096_2160P_24HZ     = 0x1f;
 *  DISP_TV_MOD_4096_2160P_25HZ     = 0x20;
 *  DISP_TV_MOD_4096_2160P_30HZ     = 0x21;
 *  DISP_TV_MOD_3840_2160P_60HZ     = 0x22;
 *  DISP_TV_MOD_4096_2160P_60HZ     = 0x23;
 *  DISP_TV_MOD_3840_2160P_50HZ     = 0x24;
 */

public class HdmiOutputConfig {
    public int deviceType;
    public int hdmiMode;
    public int colorSpace;
    public int colorDepth;
    public int colorimetry;
    public int valueRange;
    public int scanMode;
    public int hdrMode;

    /*
     * 0x08 - same as picture aspect ratio
     * 0x09 -  4:3
     * 0x0a - 16:9
     * 0x0b - 14:9
     */
    public int aspectRatio;

    /* sink device type */
    public static final int SINK_DVI_MASK        = 0x01;
    public static final int SINK_HDMI_MASK       = 0x02;

    /* color space */
    public static final int SAMPLING_MODE_RGB888 = 0x01;
    public static final int SAMPLING_MODE_YUV444 = 0x02;
    public static final int SAMPLING_MODE_YUV422 = 0x04;
    public static final int SAMPLING_MODE_YUV420 = 0x08;

    /* color depth */
    public static final int COLOR_DEPTH_24BITS   = 0x01;
    public static final int COLOR_DEPTH_30BITS   = 0x02;
    public static final int COLOR_DEPTH_36BITS   = 0x04;

    /* colorimetry */
    public static final int COLORIMETRY_BT601    = 0x01;
    public static final int COLORIMETRY_BT709    = 0x02;
    public static final int COLORIMETRY_BT2020   = 0x04;

    /* value range */
    public static final int VALUE_RANGE_DEFAULT  = 0x00;
    public static final int VALUE_RANGE_FULL     = 0x01;
    public static final int VALUE_RANGE_LIMITED  = 0x02;

    /* scan mode */
    public static final int DEFAULT_SCAN         = 0x00;
    public static final int OVER_SCAN            = 0x01;
    public static final int UNDER_SCAN           = 0x02;

    /* dataspace mode */
    public static final int DATASPACE_SDR        = 0x01;
    public static final int DATASPACE_HDR        = 0x02;
    public static final int DATASPACE_HLG        = 0x04;

    /*ratio*/
    public static final int RATIO_SAME           = 0x08;
    public static final int RATIO_4_3            = 0x09;
    public static final int RATIO_16_9           = 0x0a;
    public static final int RATIO_14_9           = 0x0b;
}

