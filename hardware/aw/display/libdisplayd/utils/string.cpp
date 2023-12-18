


#include "sunxi_typedef.h"

const char *toString(output_layer_mode_t mode) {
    switch (mode) {
    case LAYER_2D_ORIGINAL        : return "2D-ORIGINAL";
	case LAYER_2D_LEFT            : return "2D-LEFT";
	case LAYER_2D_TOP             : return "2D-TOP";
	case LAYER_3D_LEFT_RIGHT_HDMI : return "3D-LEFT-RIGHT-HDMI";
	case LAYER_3D_TOP_BOTTOM_HDMI : return "3D-TOP-BOTTOM-HDMI";
	case LAYER_2D_DUAL_STREAM     : return "2D-DUAL-STREAM";
	case LAYER_3D_DUAL_STREAM     : return "3D-DUAL-STREAM";
	case LAYER_3D_LEFT_RIGHT_ALL  : return "3D-LEFT-RIGHT-ALL";
	case LAYER_3D_TOP_BOTTOM_ALL  : return "3D-TOP-BOTTOM-ALL";
    default: return "unknow";
    }
}

