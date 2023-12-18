
#include <vector>
#include <vendor/display/config/1.0/IDisplayConfig.h>

#include "modes.h"
#include "hardware/sunxi_display2.h"

#define __MODE_ITEM(_id) \
    { .vendorid = _id, .name = #_id }

const struct modename modearray[] = {
    __MODE_ITEM(DISP_TV_MOD_480I            ),
    __MODE_ITEM(DISP_TV_MOD_576I            ),
    __MODE_ITEM(DISP_TV_MOD_480P            ),
    __MODE_ITEM(DISP_TV_MOD_576P            ),
    __MODE_ITEM(DISP_TV_MOD_720P_50HZ       ),
    __MODE_ITEM(DISP_TV_MOD_720P_60HZ       ),
    __MODE_ITEM(DISP_TV_MOD_1080I_50HZ      ),
    __MODE_ITEM(DISP_TV_MOD_1080I_60HZ      ),
    __MODE_ITEM(DISP_TV_MOD_1080P_24HZ      ),
    __MODE_ITEM(DISP_TV_MOD_1080P_50HZ      ),
    __MODE_ITEM(DISP_TV_MOD_1080P_60HZ      ),
    __MODE_ITEM(DISP_TV_MOD_1080P_24HZ_3D_FP),
    __MODE_ITEM(DISP_TV_MOD_720P_50HZ_3D_FP ),
    __MODE_ITEM(DISP_TV_MOD_720P_60HZ_3D_FP ),
    __MODE_ITEM(DISP_TV_MOD_1080P_25HZ      ),
    __MODE_ITEM(DISP_TV_MOD_1080P_30HZ      ),
    __MODE_ITEM(DISP_TV_MOD_PAL             ),
    __MODE_ITEM(DISP_TV_MOD_PAL_SVIDEO      ),
    __MODE_ITEM(DISP_TV_MOD_NTSC            ),
    __MODE_ITEM(DISP_TV_MOD_NTSC_SVIDEO     ),
    __MODE_ITEM(DISP_TV_MOD_PAL_M           ),
    __MODE_ITEM(DISP_TV_MOD_PAL_M_SVIDEO    ),
    __MODE_ITEM(DISP_TV_MOD_PAL_NC          ),
    __MODE_ITEM(DISP_TV_MOD_PAL_NC_SVIDEO   ),
    __MODE_ITEM(DISP_TV_MOD_3840_2160P_30HZ ),
    __MODE_ITEM(DISP_TV_MOD_3840_2160P_25HZ ),
    __MODE_ITEM(DISP_TV_MOD_3840_2160P_24HZ ),
    __MODE_ITEM(DISP_TV_MOD_4096_2160P_24HZ ),
    __MODE_ITEM(DISP_TV_MOD_4096_2160P_25HZ ),
    __MODE_ITEM(DISP_TV_MOD_4096_2160P_30HZ ),
    __MODE_ITEM(DISP_TV_MOD_3840_2160P_60HZ ),
    __MODE_ITEM(DISP_TV_MOD_4096_2160P_60HZ ),
    __MODE_ITEM(DISP_TV_MOD_3840_2160P_50HZ ),
    __MODE_ITEM(DISP_TV_MOD_4096_2160P_50HZ ),
};

std::vector<modename> __modelist;
const struct modename __invalidm { -1, NULL };

const std::vector<modename>& getModeList() {
    if (__modelist.empty()) {
        for (unsigned int i = 0; i < sizeof(modearray) / sizeof(modearray[0]); i++)
            __modelist.push_back(modearray[i]);
    }
    return __modelist;
}

modename getModenameByVendorId(uint32_t id) {
    for (unsigned int i = 0; i < sizeof(modearray) / sizeof(modearray[0]); i++) {
        if (modearray[i].vendorid == (int)id)
            return modearray[i];
    }
    return __invalidm;
}

const char* layerModeName(int id) {
    switch (id) {
        case 0:  return "LAYER_2D_ORIGINAL";
        case 1:  return "LAYER_2D_LEFT";
        case 2:  return "LAYER_2D_TOP";
        case 3:  return "LAYER_3D_LEFT_RIGHT_HDMI";
        case 4:  return "LAYER_3D_TOP_BOTTOM_HDMI";
        case 5:  return "LAYER_2D_DUAL_STREAM";
        case 6:  return "LAYER_3D_DUAL_STREAM";
        case 7:  return "LAYER_3D_LEFT_RIGHT_ALL";
        case 8:  return "LAYER_3D_TOP_BOTTOM_ALL";
        default: return "unknow";
    }
}

using ::vendor::display::config::V1_0::Dataspace;
const char* dataspaceName(Dataspace id) {
    switch (id) {
        case Dataspace::DATASPACE_MODE_AUTO:  return "AUTO";
        case Dataspace::DATASPACE_MODE_HDR:   return "HDR";
        case Dataspace::DATASPACE_MODE_WCG:   return "WCG";
        case Dataspace::DATASPACE_MODE_SDR:   return "SDR";
        case Dataspace::DATASPACE_MODE_OTHER: return "OTHER";
        default: return "unknow";
    }
}

using ::vendor::display::config::V1_0::PixelFormat;
const char* pixelformtName(PixelFormat id) {
    switch (id) {
        case PixelFormat::PIXEL_FORMAT_AUTO: return "AUTO";
        case PixelFormat::PIXEL_FORMAT_YUV422_12bit: return "YUV422:12Bit";
        case PixelFormat::PIXEL_FORMAT_YUV420_10bit: return "YUV420:10Bit";
        case PixelFormat::PIXEL_FORMAT_YUV444_8bit:  return "YUV444:8Bit";
        case PixelFormat::PIXEL_FORMAT_RGB888_8bit:  return "RGB888:8Bit";
        default: return "unknow";
    }
}
