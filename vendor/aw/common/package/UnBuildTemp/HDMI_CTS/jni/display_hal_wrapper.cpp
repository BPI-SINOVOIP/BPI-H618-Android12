
#include <cutils/log.h>
#include "HdmiCtsUtils.h"
#include "libdispclient.h"
#include "sunxi_display2.h"
#include "edid/edid.h"

struct maskMapper {
    int mask;
    int sunxi_def;
};

static const struct maskMapper hdmiDeviceTypeMapper[] = {
    {SINK_DVI_MASK,  DISP_DVI},
    {SINK_HDMI_MASK, DISP_HDMI},
    {-1, -1},
};

static const struct maskMapper colorSpaceMapper[] = {
    {SAMPLING_MODE_RGB888, DISP_CSC_TYPE_RGB},
    {SAMPLING_MODE_YUV444, DISP_CSC_TYPE_YUV444},
    {SAMPLING_MODE_YUV422, DISP_CSC_TYPE_YUV422},
    {SAMPLING_MODE_YUV420, DISP_CSC_TYPE_YUV420},
    {-1, -1},
};

static const struct maskMapper colorDepthMapper[] = {
    {COLOR_DEPTH_24BITS, DISP_DATA_8BITS},
    {COLOR_DEPTH_30BITS, DISP_DATA_10BITS},
    {COLOR_DEPTH_36BITS, DISP_DATA_12BITS},
    {-1, -1},
};

static const struct maskMapper colorimetryMapper[] = {
    {COLORIMETRY_BT601,  DISP_BT601},
    {COLORIMETRY_BT709,  DISP_BT709},
    {COLORIMETRY_BT2020, DISP_BT2020NC},
    {-1, -1},
};

static const struct maskMapper valueRangeMapper[] = {
    {VALUE_RANGE_DEFAULT, DISP_COLOR_RANGE_16_255},
    {VALUE_RANGE_FULL,    DISP_COLOR_RANGE_0_255 },
    {VALUE_RANGE_LIMITED, DISP_COLOR_RANGE_16_235},
    {-1, -1},
};

static const struct maskMapper scanModeMapper[] = {
    {NO_DATA,    DISP_SCANINFO_NO_DATA },
    {OVER_SCAN,  OVERSCAN },
    {UNDER_SCAN, UNDERSCAN},
    {-1, -1},
};

static const struct maskMapper hdrModeMapper[] = {
    {DATASPACE_SDR, DISP_EOTF_GAMMA22},
    {DATASPACE_HDR, DISP_EOTF_SMPTE2084},
    {DATASPACE_HLG, DISP_EOTF_ARIB_STD_B67},
    {-1, -1},
};

template <typename T>
T getSunxiDefValue(const struct maskMapper *mapper, int mask, T default_value)
{
    while (mapper->mask != -1) {
        if (mapper->mask == mask)
            break;
        ++mapper;
    }
    if (mapper->mask == -1) {
        return default_value;
    }
    return static_cast<T>(mapper->sunxi_def);
}

template <typename T>
T getMaskFromSunxiDef(const struct maskMapper *mapper, int sunxi_def, T default_value)
{
    while (mapper->sunxi_def != -1) {
        if (mapper->sunxi_def == sunxi_def)
            break;
        ++mapper;
    }
    if (mapper->sunxi_def == -1) {
        return default_value;
    }
    return static_cast<T>(mapper->mask);
}

int displayHalSetOutputConfig(struct HdmiOutputConfig *config)
{
    displaydClient *client = new displaydClient();
    struct disp_device_config dc;

    dc.type = DISP_OUTPUT_TYPE_HDMI;
    dc.mode = static_cast<disp_tv_mode>(config->hdmiMode);
    dc.format = getSunxiDefValue(colorSpaceMapper, config->colorSpace, DISP_CSC_TYPE_YUV444);
    dc.bits = getSunxiDefValue(colorDepthMapper, config->colorDepth, DISP_DATA_8BITS);
    dc.eotf = getSunxiDefValue(hdrModeMapper, config->hdrMode, DISP_EOTF_GAMMA22);
    dc.cs = getSunxiDefValue(colorimetryMapper, config->colorimetry, DISP_BT709);
    dc.dvi_hdmi = getSunxiDefValue(hdmiDeviceTypeMapper, config->deviceType, DISP_HDMI);
    dc.range = getSunxiDefValue(valueRangeMapper, config->valueRange, DISP_COLOR_RANGE_16_255);
    dc.scan = getSunxiDefValue(scanModeMapper, config->scanMode, DISP_SCANINFO_NO_DATA);
    dc.aspect_ratio = config->aspectRatio;

    client->setDeviceConfig(1, &dc);
    delete client;
    return 0;
}

int displayHalGetOutputConfig(struct HdmiOutputConfig *config)
{
    displaydClient *client = new displaydClient();
    struct disp_device_config dc;

    if (client->getDeviceConfig(1, &dc) != 0)
        ALOGD("displayHalGetOutputConfig return error");
    config->deviceType = getMaskFromSunxiDef(hdmiDeviceTypeMapper, dc.dvi_hdmi, SINK_HDMI_MASK);
    config->hdmiMode = dc.mode;
    ALOGD("hdmi: bill hdmiMode = %d\n",config->hdmiMode);
    config->colorSpace = getMaskFromSunxiDef(colorSpaceMapper, dc.format, SAMPLING_MODE_YUV444);
    config->colorDepth = getMaskFromSunxiDef(colorDepthMapper, dc.bits, COLOR_DEPTH_24BITS);
    config->colorimetry = getMaskFromSunxiDef(colorSpaceMapper, dc.cs, COLORIMETRY_BT709);
    config->valueRange = getMaskFromSunxiDef(valueRangeMapper, dc.range, VALUE_RANGE_DEFAULT);
    config->scanMode = getMaskFromSunxiDef(scanModeMapper, dc.scan, NO_DATA);
    config->hdrMode = getMaskFromSunxiDef(hdrModeMapper, dc.eotf, DATASPACE_SDR);
    config->aspectRatio = dc.aspect_ratio;
    return 0;
}

static void setColorDepth(struct HdmiCapability *cap, struct video_format *from)
{
    if (from->bits & DEEP_COLOR_24BIT)
        cap->supportedColorDepth |= COLOR_DEPTH_24BITS;
    if (from->bits & DEEP_COLOR_30BIT)
        cap->supportedColorDepth |= COLOR_DEPTH_30BITS;
    if (from->bits & DEEP_COLOR_36BIT)
        cap->supportedColorDepth |= COLOR_DEPTH_36BITS;
}

int displayHalGetConstraintByHdmiMode(
        int mode, struct edid_info *info, struct HdmiCapability *capability)
{
    int regular_sampling = 0;
    memset(capability, 0, sizeof(*capability));
    for (int i = 0; i < info->total_ycbcr420_vic; i++) {
        if (mode == info->ycbcr420_vic[i].sunxi_disp_mode) {
            capability->supportedColorSpace = SAMPLING_MODE_YUV420;
            setColorDepth(capability, &info->fmt[FMT_YUV420]);

            /* not support regular sampling mode, just return */
            if (info->ycbcr420_vic[i].support_regular_sampling_mode)
                regular_sampling = 1;
        }
    }

    if (!regular_sampling) {
        for (int i = 0; i < info->total_vic; i++) {
            if (mode == info->supported_vic[i].sunxi_disp_mode) {
                regular_sampling = 1;
                break;
            }
        }
    }
    if (!regular_sampling)
        return 0;

    /* default value for regular sampling */
    if (info->fmt[FMT_RGB].supported) {
        capability->supportedColorSpace |= SAMPLING_MODE_RGB888;
        setColorDepth(capability, &info->fmt[FMT_RGB]);
    }
    if (info->fmt[FMT_YUV444].supported) {
        capability->supportedColorSpace |= SAMPLING_MODE_YUV444;
        setColorDepth(capability, &info->fmt[FMT_YUV444]);
    }
    if (info->fmt[FMT_YUV422].supported) {
        capability->supportedColorSpace |= SAMPLING_MODE_YUV422;
        setColorDepth(capability, &info->fmt[FMT_YUV422]);
    }
    return 0;
}

