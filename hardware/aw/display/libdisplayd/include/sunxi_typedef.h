
#ifndef __SUNXI_TYPEDEF_H__
#define __SUNXI_TYPEDEF_H__

typedef enum {
    DATASPACE_MODE_AUTO       = 0,
    DATASPACE_MODE_HDR        = 1,
    DATASPACE_MODE_WCG        = 2,
    DATASPACE_MODE_SDR        = 3,
    DATASPACE_MODE_HDRP       = 4,
    DATASPACE_MODE_DV         = 5,
    DATASPACE_MODE_OTHER      = 6,
    DATASPACE_MODE_NUM,
} dataspace_t;

typedef enum {
    PIXEL_FORMAT_AUTO         = 0,
    PIXEL_FORMAT_YUV422_10bit = 1,
    PIXEL_FORMAT_YUV420_10bit = 2,
    PIXEL_FORMAT_YUV444_8bit  = 3,
    PIXEL_FORMAT_RGB888_8bit  = 4,
    PIXEL_FORMAT_NUM,
} pixelformat_t;

typedef enum
{
    ASPECT_RATIO_AUTO         = 0,
    ASPECT_RATIO_FULL         = 1,
    ASPECT_RATIO_4_3          = 2,
    ASPECT_RATIO_16_9         = 3,
    ASPECT_RATIO_WIDTH        = 4,
    ASPECT_RATIO_HEIGHT       = 5,
    ASPECT_RATIO_FULL_ONCE    = 7,
    ASPECT_RATIO_RATIO_LOAD   = 8,
}aspect_ratio_t;


typedef enum {
	LAYER_2D_ORIGINAL         = 0,
	LAYER_2D_LEFT             = 1,
	LAYER_2D_TOP              = 2,
	LAYER_3D_LEFT_RIGHT_HDMI  = 3,
	LAYER_3D_TOP_BOTTOM_HDMI  = 4,
	LAYER_2D_DUAL_STREAM      = 5,
	LAYER_3D_DUAL_STREAM      = 6,
	LAYER_3D_LEFT_RIGHT_ALL   = 7,
	LAYER_3D_TOP_BOTTOM_ALL   = 8,
} output_layer_mode_t;

#endif
