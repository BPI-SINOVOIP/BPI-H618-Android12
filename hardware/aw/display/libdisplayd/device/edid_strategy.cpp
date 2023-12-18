
#include <cutils/properties.h>

#include "device/edid_strategy.h"
#include "edid/cea_vic.h"

struct dispmode2vicMap {
    int vic;
    int dispmode;
};
#define HDMI_1920x1080p24_3D_FP (HDMI_1920x1080p24_16x9 + 0x80)
#define HDMI_1280x720p50_3D_FP  (HDMI_1280x720p50_16x9  + 0x80)
#define HDMI_1280x720p60_3D_FP  (HDMI_1280x720p60_16x9  + 0x80)

const struct dispmode2vicMap dispmodeMapTable[] = {
    {HDMI_720x480i60_4x3,       DISP_TV_MOD_480I},
    {HDMI_720x576i50_4x3,       DISP_TV_MOD_576I},
    {HDMI_720x480p60_4x3,       DISP_TV_MOD_480P},
    {HDMI_720x576p50_4x3,       DISP_TV_MOD_576P},

    {HDMI_1280x720p50_16x9,     DISP_TV_MOD_720P_50HZ},
    {HDMI_1280x720p60_16x9,     DISP_TV_MOD_720P_60HZ},

    {HDMI_1920x1080i50_16x9,    DISP_TV_MOD_1080I_50HZ},
    {HDMI_1920x1080i60_16x9,    DISP_TV_MOD_1080I_60HZ},
    {HDMI_1920x1080p50_16x9,    DISP_TV_MOD_1080P_50HZ},
    {HDMI_1920x1080p60_16x9,    DISP_TV_MOD_1080P_60HZ},
    {HDMI_1920x1080p24_16x9,    DISP_TV_MOD_1080P_24HZ},
    {HDMI_1920x1080p25_16x9,    DISP_TV_MOD_1080P_25HZ},
    {HDMI_1920x1080p30_16x9,    DISP_TV_MOD_1080P_30HZ},

    {HDMI_1920x1080p24_3D_FP,   DISP_TV_MOD_1080P_24HZ_3D_FP},
    {HDMI_1280x720p50_3D_FP,    DISP_TV_MOD_720P_50HZ_3D_FP},
    {HDMI_1280x720p60_3D_FP,    DISP_TV_MOD_720P_60HZ_3D_FP},

    {HDMI_3840x2160p24_16x9,    DISP_TV_MOD_3840_2160P_24HZ},
    {HDMI_3840x2160p25_16x9,    DISP_TV_MOD_3840_2160P_25HZ},
    {HDMI_3840x2160p30_16x9,    DISP_TV_MOD_3840_2160P_30HZ},
    {HDMI_3840x2160p50_16x9,    DISP_TV_MOD_3840_2160P_50HZ},
    {HDMI_3840x2160p60_16x9,    DISP_TV_MOD_3840_2160P_60HZ},

    {HDMI_4096x2160p24_256x135, DISP_TV_MOD_4096_2160P_24HZ},
    {HDMI_4096x2160p25_256x135, DISP_TV_MOD_4096_2160P_25HZ},
    {HDMI_4096x2160p30_256x135, DISP_TV_MOD_4096_2160P_30HZ},
    {HDMI_4096x2160p50_256x135, DISP_TV_MOD_4096_2160P_50HZ},
    {HDMI_4096x2160p60_256x135, DISP_TV_MOD_4096_2160P_60HZ},
};

#define NEMLEN(_array) (sizeof(_array) / sizeof(_array[0]))

int dispmode2vic(int mode)
{
    for (size_t i = 0; i < NEMLEN(dispmodeMapTable); i++) {
        if (dispmodeMapTable[i].dispmode == mode) {
            return dispmodeMapTable[i].vic;
        }
    }
    return -1;
}

int vic2dispmode(int vic)
{
    for (size_t i = 0; i < NEMLEN(dispmodeMapTable); i++) {
        if (dispmodeMapTable[i].vic == vic) {
            return dispmodeMapTable[i].dispmode;
        }
    }
    return -1;
}

EdidStrategy::EdidStrategy()
  : mParser("/sys/class/hdmi/hdmi/attr/edid")
{
}

int EdidStrategy::update()
{
    int ret;
    ret = mParser.reload();
    property_set("vendor.sys.tv_vdid_fex", mParser.mMonitorName);
    return ret;
}

int EdidStrategy::getHDMIVersion()
{
    return mParser.mHDMIVersion;
}

/*
 * Verify current device config by edid, If not supported
 * by sink, change to the recommend value.
 *   - sampling format: RGB/YUV444/YUV422/YUV420
 *   - color depth    : 24bit/30bit
 *   - eotf           : DISP_EOTF_GAMMA22/DISP_EOTF_SMPTE2084
 *   - color space    : DISP_BT601/DISP_BT709/DISP_BT2020NC
 */
int EdidStrategy::checkAndCorrectDeviceConfig(struct disp_device_config *config)
{
    int dirty = 0;

    /* Not every sink support HDR10 */
    if ((config->eotf == DISP_EOTF_SMPTE2084) &&
            !(mParser.mEotf & EOTF_SMPTE_ST_2084)) {
        dirty++;
        config->eotf = DISP_EOTF_GAMMA22;
    }

    /* Colorspace rule:
     *  1. Current output is HDR10 and sink support BT2020: DISP_BT2020NC;
     *  2. BT709 for HD resolution(>=720P);
     *  3. BT601 for SD resolution(< 720P);
     */
    int colorspace = 0;
    if ((config->eotf == DISP_EOTF_SMPTE2084) &&
            (mParser.mColorimetry & BT2020_YCC)) {
        colorspace = DISP_BT2020NC;
    } else {
        colorspace = getColorspaceByResolution(config->mode);
    }
    if (config->cs != colorspace) {
        dirty++;
        config->cs = (enum disp_color_space)colorspace;
    }

    /* Pixelformat detect */
    int pixelformat = config->format;
    if (config->mode == DISP_TV_MOD_3840_2160P_60HZ ||
        config->mode == DISP_TV_MOD_3840_2160P_50HZ ||
        config->mode == DISP_TV_MOD_4096_2160P_60HZ ||
        config->mode == DISP_TV_MOD_4096_2160P_50HZ) {
        /* for 2160P60/2160P50 , perfer to YUV420 */
        if (pixelformat != DISP_CSC_TYPE_YUV420)
            dirty++;
        pixelformat = config->format = DISP_CSC_TYPE_YUV420;
    } else {
        if (config->format != DISP_CSC_TYPE_YUV444 &&
                config->format != DISP_CSC_TYPE_YUV422 &&
                config->format != DISP_CSC_TYPE_YUV420 &&
                config->format != DISP_CSC_TYPE_RGB) {
            dirty++;
            pixelformat = config->format = DISP_CSC_TYPE_YUV444;
        }
    }

    if (config->format == DISP_CSC_TYPE_YUV420) {
        if (!isSupportY420Sampling(config->mode))
            pixelformat = DISP_CSC_TYPE_YUV444;
    }
    if (isOnlySupportY420Sampling(config->mode))
        pixelformat = DISP_CSC_TYPE_YUV420;
    if (mParser.mRGBOnly ||
            (mParser.mSinkType == SINK_TYPE_DVI)) {
        pixelformat = DISP_CSC_TYPE_RGB;
    }
    if (config->format != pixelformat) {
        dirty++;
        config->format = (enum disp_csc_type)pixelformat;
    }

    /* bits detech */
    int bits = config->bits;
    if (config->mode == DISP_TV_MOD_3840_2160P_60HZ ||
        config->mode == DISP_TV_MOD_3840_2160P_50HZ ||
        config->mode == DISP_TV_MOD_4096_2160P_60HZ ||
        config->mode == DISP_TV_MOD_4096_2160P_50HZ) {
        /* for 2160P60/2160P50 , perfer to 8bits */
        if (bits != DISP_DATA_8BITS)
            dirty++;
        bits = config->bits = DISP_DATA_8BITS;
    }
    bits = checkSamplingBits(config->format, config->bits);
    if (config->bits != bits) {
        dirty++;
        config->bits = (enum disp_data_bits)bits;
    }

    /* misc config field */
    if (mParser.mSinkType == SINK_TYPE_DVI)
        config->dvi_hdmi = DISP_DVI;
    else
        config->dvi_hdmi = DISP_HDMI;
    config->range = (config->format == DISP_CSC_TYPE_RGB) ? DISP_COLOR_RANGE_0_255 : DISP_COLOR_RANGE_16_235;
    config->scan  = DISP_SCANINFO_NO_DATA;
    config->aspect_ratio = 8;

    /* TODO: check tmds clock */

    return dirty;
}

int EdidStrategy::checkAndCorrectDefaultDeviceConfig(struct disp_device_config *config)
{
    int dirty = 0;

    /* Not every sink support HDR10 */
    if ((config->eotf == DISP_EOTF_SMPTE2084) &&
            !(mParser.mEotf & EOTF_SMPTE_ST_2084)) {
        dirty++;
        config->eotf = DISP_EOTF_GAMMA22;
    }

    /* Colorspace rule:
     *  1. Current output is HDR10 and sink support BT2020: DISP_BT2020NC;
     *  2. BT709 for HD resolution(>=720P);
     *  3. BT601 for SD resolution(< 720P);
     */
    int colorspace = 0;
    if ((config->eotf == DISP_EOTF_SMPTE2084) &&
            (mParser.mColorimetry & BT2020_YCC)) {
        colorspace = DISP_BT2020NC;
    } else {
        colorspace = getColorspaceByResolution(config->mode);
    }
    if (config->cs != colorspace) {
        dirty++;
        config->cs = (enum disp_color_space)colorspace;
    }

    /* Pixelformat detect */
    int pixelformat = config->format;
    if (supportedPixelFormat(config->mode, config->format, config->bits) == false) {
        if (config->mode == DISP_TV_MOD_3840_2160P_60HZ ||
                config->mode == DISP_TV_MOD_3840_2160P_50HZ ||
                config->mode == DISP_TV_MOD_4096_2160P_60HZ ||
                config->mode == DISP_TV_MOD_4096_2160P_50HZ) {
            /* for 2160P60/2160P50 , perfer to YUV420 */
            if (pixelformat != DISP_CSC_TYPE_YUV420)
                dirty++;
            pixelformat = config->format = DISP_CSC_TYPE_YUV420;
        } else {
            if (config->format != DISP_CSC_TYPE_YUV444 &&
                    config->format != DISP_CSC_TYPE_YUV422 &&
                    config->format != DISP_CSC_TYPE_YUV420 &&
                    config->format != DISP_CSC_TYPE_RGB) {
                dirty++;
                pixelformat = config->format = DISP_CSC_TYPE_YUV444;
            }
        }
    }

    if (config->format == DISP_CSC_TYPE_YUV420) {
        if (!isSupportY420Sampling(config->mode))
            pixelformat = DISP_CSC_TYPE_YUV444;
    }
    if (isOnlySupportY420Sampling(config->mode))
        pixelformat = DISP_CSC_TYPE_YUV420;
    if (mParser.mRGBOnly ||
            (mParser.mSinkType == SINK_TYPE_DVI)) {
        pixelformat = DISP_CSC_TYPE_RGB;
    }
    if (config->format != pixelformat) {
        dirty++;
        config->format = (enum disp_csc_type)pixelformat;
    }

    /* bits detech */
    int bits = config->bits;
    if (config->mode == DISP_TV_MOD_3840_2160P_60HZ ||
        config->mode == DISP_TV_MOD_3840_2160P_50HZ ||
        config->mode == DISP_TV_MOD_4096_2160P_60HZ ||
        config->mode == DISP_TV_MOD_4096_2160P_50HZ) {
        /* for 2160P60/2160P50 , perfer to 8bits */
        if (bits != DISP_DATA_8BITS)
            dirty++;
        bits = config->bits = DISP_DATA_8BITS;
    }
    bits = checkSamplingBits(config->format, config->bits);
    if (config->bits != bits) {
        dirty++;
        config->bits = (enum disp_data_bits)bits;
    }

    /* misc config field */
    if (mParser.mSinkType == SINK_TYPE_DVI)
        config->dvi_hdmi = DISP_DVI;
    else
        config->dvi_hdmi = DISP_HDMI;
    config->range = (config->format == DISP_CSC_TYPE_RGB) ? DISP_COLOR_RANGE_0_255 : DISP_COLOR_RANGE_16_235;
    config->scan  = DISP_SCANINFO_NO_DATA;
    config->aspect_ratio = 8;

    /* TODO: check tmds clock */

    return dirty;
}

bool EdidStrategy::setupConfig(int target, struct disp_device_config *config, int *dirty)
{
    *dirty = 0;
    config->hdr_type = HDR10;
    switch (target) {
    case EdidStrategy::HDRP:
        if (!(mParser.mEotf & EOTF_SMPTE_ST_2084) || mParser.mAppVer <= 0) {
            //dd_error("edid support 2084=%d, support hdr10+=%d",
            //         (mParser.mEotf & EOTF_SMPTE_ST_2084), mParser.mAppVer);
            return false;
        }
        if (config->eotf != DISP_EOTF_SMPTE2084) {
            config->eotf = DISP_EOTF_SMPTE2084;
            *dirty += 1;
        }
        if (config->cs != DISP_BT2020NC) {
            config->cs = DISP_BT2020NC;
            *dirty += 1;
        }
        config->bits = DISP_DATA_10BITS;
        /*config->format = DISP_CSC_TYPE_RGB;
        config->range = DISP_COLOR_RANGE_0_255;*/
        config->hdr_type = HDR10P;
        dd_error("config->hdr_type = %d, cs=%x", config->hdr_type, config->cs);
        return true;
    case EdidStrategy::DV:
        if (!(mParser.mEotf & EOTF_SMPTE_ST_2084))
            return false;
        if (config->eotf != DISP_EOTF_SMPTE2084) {
            config->eotf = DISP_EOTF_SMPTE2084;
            *dirty += 1;
        }
        if (config->cs != DISP_BT2020NC) {
            config->cs = DISP_BT2020NC;
            *dirty += 1;
        }
        return true;
    case EdidStrategy::HDR:
        if (!(mParser.mEotf & EOTF_SMPTE_ST_2084))
            return false;
        if (config->eotf != DISP_EOTF_SMPTE2084) {
            config->eotf = DISP_EOTF_SMPTE2084;
            *dirty += 1;
        }
        if ((mParser.mColorimetry & BT2020_YCC) &&
                (config->cs != DISP_BT2020NC)) {
            config->cs = DISP_BT2020NC;
            *dirty += 1;
        }
        return true;
    case EdidStrategy::WCG:
        if (!(mParser.mColorimetry & BT2020_YCC))
            return false;
        if ((config->cs != DISP_BT2020NC)) {
            config->cs = DISP_BT2020NC;
            *dirty += 1;
        }
        if (config->eotf != DISP_EOTF_GAMMA22) {
            config->eotf = DISP_EOTF_GAMMA22;
            *dirty += 1;
        }
        return true;
    case EdidStrategy::SDR: {
        int cs = getColorspaceByResolution(config->mode);
        if (config->cs != cs) {
            config->cs = (disp_color_space)cs;
            *dirty += 1;
        }
        if (config->eotf != DISP_EOTF_GAMMA22) {
            config->eotf = DISP_EOTF_GAMMA22;
            *dirty += 1;
        }
        return true;}
    default:
        dd_error("setupConfig: unknow dataspace %d", target);
        break;
    }
    return false;
}

bool EdidStrategy::supportedPixelFormat(int mode, int format, int depth)
{
    if (format != DISP_CSC_TYPE_RGB &&
            (mParser.mSinkType == SINK_TYPE_DVI || mParser.mRGBOnly)) {
        /* Not support yuv sampling format */
        dd_error("Sink type is dvi, not support yuv format");
        return false;
    }
    if ((format == DISP_CSC_TYPE_YUV420) &&
                (!isSupportY420Sampling(mode))) {
        dd_error("disp mode %d not support yuv420 format", mode);
        return false;
    }
    if (!mParser.mSupportedFormat.count(format)) {
        dd_error("Sink not support sampling format(%d)", format);
        return false;
    }

    int support = false;
    if (depth == DISP_DATA_8BITS)
        support = true;
    else if (depth == DISP_DATA_10BITS)
        support = (mParser.mSupportedFormat[format]->dc_30bit == 1);
    else if (depth == DISP_DATA_12BITS)
        support = (mParser.mSupportedFormat[format]->dc_36bit == 1);
    else if (depth == DISP_DATA_16BITS)
        support = (mParser.mSupportedFormat[format]->dc_48bit == 1);

    return support;
}

bool EdidStrategy::supportedHDR()
{
    if (mParser.mEotf & EOTF_SMPTE_ST_2084)
        return true;

    return false;
}

bool EdidStrategy::supportedHDR10P()
{
    if (mParser.mAppVer > 0)
        return true;

    return false;
}


bool EdidStrategy::supported3DPresent()
{
    if (mParser.m3Dpresent)
        return true;
    else
        return false;
}

int EdidStrategy::getColorspaceByResolution(int mode)
{
    switch (mode) {
    case DISP_TV_MOD_480I:
    case DISP_TV_MOD_576I:
	case DISP_TV_MOD_480P:
	case DISP_TV_MOD_576P:
        return DISP_BT601;
    case DISP_TV_MOD_720P_50HZ       :
	case DISP_TV_MOD_720P_60HZ       :
	case DISP_TV_MOD_1080I_50HZ      :
	case DISP_TV_MOD_1080I_60HZ      :
	case DISP_TV_MOD_1080P_24HZ      :
	case DISP_TV_MOD_1080P_50HZ      :
	case DISP_TV_MOD_1080P_60HZ      :
	case DISP_TV_MOD_1080P_24HZ_3D_FP:
	case DISP_TV_MOD_720P_50HZ_3D_FP :
	case DISP_TV_MOD_720P_60HZ_3D_FP :
	case DISP_TV_MOD_1080P_25HZ      :
	case DISP_TV_MOD_1080P_30HZ      :
	case DISP_TV_MOD_3840_2160P_30HZ :
	case DISP_TV_MOD_3840_2160P_25HZ :
	case DISP_TV_MOD_3840_2160P_24HZ :
	case DISP_TV_MOD_4096_2160P_24HZ :
	case DISP_TV_MOD_4096_2160P_25HZ :
	case DISP_TV_MOD_4096_2160P_30HZ :
	case DISP_TV_MOD_3840_2160P_60HZ :
	case DISP_TV_MOD_4096_2160P_60HZ :
	case DISP_TV_MOD_3840_2160P_50HZ :
	case DISP_TV_MOD_4096_2160P_50HZ :
        return DISP_BT709;
    default:
        return DISP_BT709;
    }
}

bool EdidStrategy::isSupportY420Sampling(int mode)
{
    int vic = dispmode2vic(mode);
    for (size_t i = 0; i < mParser.mSupportedVIC.size(); i++) {
        if ((mParser.mSupportedVIC[i]->vic == vic) &&
                mParser.mSupportedVIC[i]->ycbcr420_sampling)
            return true;
    }
    for (size_t i = 0; i < mParser.mY420VIC.size(); i++) {
        if (mParser.mY420VIC[i]->vic == vic)
            return true;
    }
    return false;
}

bool EdidStrategy::isOnlySupportY420Sampling(int mode)
{
    int vic = dispmode2vic(mode);
    for (size_t i = 0; i < mParser.mY420VIC.size(); i++) {
        if (mParser.mY420VIC[i]->vic == vic)
            return true;
    }
    return false;
}


int EdidStrategy::checkSamplingBits(int format, int bits)
{
    if (bits == DISP_DATA_8BITS) {
        return DISP_DATA_8BITS;
    }

    if (!mParser.mSupportedFormat.count(format)) {
        dd_error("Sink not support sampling format(%d)", format);

        if (bits != DISP_DATA_8BITS &&
                bits != DISP_DATA_10BITS &&
                bits != DISP_DATA_12BITS &&
                bits != DISP_DATA_16BITS) {
            bits = DISP_DATA_8BITS;
        }

        return bits;
    }
    if (bits == DISP_DATA_10BITS) {
        if (!mParser.mSupportedFormat[format]->dc_30bit)
            return DISP_DATA_8BITS;
        return bits;
    }
    if (bits == DISP_DATA_12BITS) {
        if (!mParser.mSupportedFormat[format]->dc_36bit)
            return DISP_DATA_8BITS;
        return bits;
    }
    if (bits == DISP_DATA_16BITS) {
        if (!mParser.mSupportedFormat[format]->dc_48bit)
            return DISP_DATA_8BITS;
        return bits;
    }
    return DISP_DATA_8BITS;
}

/**
 * @name       :getNative
 * @brief      :get best resolution from edid
 * @param[IN]  :NONE
 * @param[OUT] :NONE
 * @return     :- resolution mode id(disp mode define in sunxi_display2.h)
 *		- -1 if can't not find dispmode in vic2dispmode map
 *		- -2 if vic was not found in edid
 */
int EdidStrategy::getNative()
{
    for (auto it = mParser.mSupportedVIC.begin();
         it != mParser.mSupportedVIC.end(); ++it) {
        EdidParser::videoInformationCode *v = *it;
        if (v->native)
            return vic2dispmode(v->vic);
    }
    return -2;
}
