
#ifndef _HDMI_CTS_UTILS_H_
#define _HDMI_CTS_UTILS_H_
#ifdef __cplusplus
extern "C" {
#endif

struct HdmiOutputConfig {
    int deviceType;
    int hdmiMode;
    int colorSpace;
    int colorDepth;
    int colorimetry;
    int valueRange;
    int scanMode;
    int hdrMode;
    int aspectRatio;
};

enum sink_device_type_mask {
    SINK_DVI_MASK   = 0x01,
    SINK_HDMI_MASK  = 0x02,
};

enum color_space_mask {
    SAMPLING_MODE_RGB888 = 0x01,
    SAMPLING_MODE_YUV444 = 0x02,
    SAMPLING_MODE_YUV422 = 0x04,
    SAMPLING_MODE_YUV420 = 0x08,
};

enum color_depth_mask {
    COLOR_DEPTH_24BITS = 0x01,
    COLOR_DEPTH_30BITS = 0x02,
    COLOR_DEPTH_36BITS = 0x04,
};

enum colorimetry_mask {
    COLORIMETRY_BT601  = 0x01,
    COLORIMETRY_BT709  = 0x02,
    COLORIMETRY_BT2020 = 0x04,
};

enum value_range_mask {
    VALUE_RANGE_DEFAULT = 0x00,
    VALUE_RANGE_FULL    = 0x01,
    VALUE_RANGE_LIMITED = 0x02,
};

enum scan_mode_mask {
    NO_DATA    = 0x00,
    OVER_SCAN  = 0x01,
    UNDER_SCAN = 0x02,
};

enum dataspace_mode_mask {
    DATASPACE_SDR = 0x01,
    DATASPACE_HDR = 0x02,
    DATASPACE_HLG = 0x04,
};

struct HdmiCapability {
    /*
     * Sink type.
     *  1: dvi
     *  2: hdmi
     */
    int deviceType;

    /*
     * Sampling mode supported by sink.
     *  0x01: RGB888
     *  0x02: YUV444
     *  0x04: YUV422
     *  0x08: YUV420
     */
    int supportedColorSpace;

    /*
     * Bits per pixel.
     *  0x01: 24bit
     *  0x02: 30bit
     *  0x04: 36bit
     */
    int supportedColorDepth;

    /*
     * Colorimetry.
     *  0x01: BT601
     *  0x02: BT709
     *  0x04: BT2020
     */
    int supportedColorimetry;

    /*
     * 0x00: default (depend on device)
     * 0x01: Full    ( 0 ~ 255)
     * 0x02: limited (16 ~ 235)
     */
    int supportedValueRange;

    /*
     * 0x01: overscan
     * 0x02: underscan
     */
    int supportedScanMode;

    /*
     * 0x01: SDR
     * 0x02: HDR
     * 0x04: HLG
     */
    int supportedHdrMode;
    int supportedAspectRatio;
};


void HdmiCapabilityFromEdidInfo(
        struct HdmiCapability *capability, struct edid_info *info);
int toSunxiOutputResolution(int vic);
int displayHalSetOutputConfig(struct HdmiOutputConfig *config);
int displayHalGetOutputConfig(struct HdmiOutputConfig *config);
int displayHalGetConstraintByHdmiMode(int mode, struct edid_info *info, struct HdmiCapability *capability);

#ifdef __cplusplus
}
#endif
#endif
