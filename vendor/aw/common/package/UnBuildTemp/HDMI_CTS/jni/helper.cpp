
#include <stdlib.h>
#include "edid/edid.h"
#include "HdmiCtsUtils.h"

void HdmiCapabilityFromEdidInfo(
        struct HdmiCapability *capability, struct edid_info *info)
{
    capability->deviceType = SINK_DVI_MASK;
    if (info->sink_type == 0x02)
        capability->deviceType |= SINK_HDMI_MASK;

    if (info->fmt[FMT_RGB].supported)
        capability->supportedColorSpace |= SAMPLING_MODE_RGB888;
    if (info->fmt[FMT_YUV444].supported)
        capability->supportedColorSpace |= SAMPLING_MODE_YUV444;
    if (info->fmt[FMT_YUV422].supported)
        capability->supportedColorSpace |= SAMPLING_MODE_YUV422;
    if (info->fmt[FMT_YUV420].supported)
        capability->supportedColorSpace |= SAMPLING_MODE_YUV420;

    if (info->fmt[FMT_YUV444].supported)
        capability->supportedColorDepth = info->fmt[FMT_YUV444].bits;
    else if (info->fmt[FMT_RGB].supported)
        capability->supportedColorDepth = info->fmt[FMT_RGB].bits;

    capability->supportedColorimetry = (COLORIMETRY_BT601 | COLORIMETRY_BT709);
    if (info->colorimetry & (1 << 6))
        capability->supportedColorimetry |= COLORIMETRY_BT2020;

    capability->supportedValueRange = (VALUE_RANGE_FULL | VALUE_RANGE_LIMITED);
    capability->supportedScanMode = (OVER_SCAN | UNDER_SCAN);

    capability->supportedHdrMode = DATASPACE_SDR;
    if (info->eotf & (1 << 2))
        capability->supportedHdrMode |= DATASPACE_HDR;

    capability->supportedAspectRatio = 0;
}

