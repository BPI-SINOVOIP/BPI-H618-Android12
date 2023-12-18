
package com.softwinner.utils;

public class HdmiCapability {

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

    public int deviceType;
    public int supportedColorSpace;
    public int supportedColorDepth;
    public int supportedColorimetry;
    public int supportedValueRange;
    public int supportedScanMode;
    public int supportedHdrMode;
    public int supportedAspectRatio;
}
