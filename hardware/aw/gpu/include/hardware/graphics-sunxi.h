/*
 *  Copyright (C) 2015-2019 Allwinner Technology
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef GRAPHIC_VENDOE_H
#define GRAPHIC_VENDOE_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_DIV 8
#define MAX_LUT_SIZE 729

typedef enum {
    /*
     * 0x100 - 0x1FF
     *
     * This range is reserved for pixel formats that are specific to the HAL
     * implementation.  Implementations can use any value in this range to
     * communicate video pixel formats between their HAL modules.  These formats
     * must not have an alpha channel.  Additionally, an EGLimage created from a
     * gralloc buffer of one of these formats must be supported for use with the
     * GL_OES_EGL_image_external OpenGL ES extension.
     */

    HAL_PIXEL_FORMAT_AW_BASE          = 0x200,
    HAL_PIXEL_FORMAT_AW_NV12,
    HAL_PIXEL_FORMAT_AW_MB420,
    HAL_PIXEL_FORMAT_AW_MB411,
    HAL_PIXEL_FORMAT_AW_MB422,
    HAL_PIXEL_FORMAT_AW_YUV_PLANNER420, // YU12
    HAL_PIXEL_FORMAT_AW_YVU_PLANNER420, // YV12
    HAL_PIXEL_FORMAT_AW_NV21, /* HAL_PIXEL_FORMAT_YCrCb_420_SP should be used instead. */

    HAL_PIXEL_FORMAT_AW_NV12_10bit, /* yuv420: [8bit: yyyy  uvuv] [2bit: yyyy uvuv] */
    HAL_PIXEL_FORMAT_AW_NV21_10bit, /* yuv420: [8bit: yyyy  vuvu] [2bit: yyyy uvuv] */
    HAL_PIXEL_FORMAT_AW_YV12_10bit, /* yuv420: [8bit: yyyy  vv uu] [2bit: yyyy uvuv] */
    HAL_PIXEL_FORMAT_AW_I420_10bit, /* yuv420: [8bit: yyyy  uu vv] [2bit: yyyy uvuv] */

    HAL_PIXEL_FORMAT_AW_P010_UV, /* Planar, 4:2:0, 10bit, (yyyy, uvuv) */
    HAL_PIXEL_FORMAT_AW_P010_VU, /* Planar, 4:2:0, 10bit, (yyyy, vuvu) */
    HAL_PIXEL_FORMAT_AW_P210_UV, /* Planar, 4:2:2, 10bit, (yyyy, uvuv) */
    HAL_PIXEL_FORMAT_AW_P210_VU, /* Planar, 4:2:2, 10bit, (yyyy, vuvu) */

    HAL_PIXEL_FORMAT_BGRX_8888,// 0x1ff  for IMG gpu becareful for A100,modify it in gpu ddk

} vendor_pixel_format_t;

/* sunxi private usage */
enum{
	GRALLOC_USAGE_AFBC_MODE = 1ULL << 30,
};

enum {
	/* hdr static metadata is available */
	SUNXI_METADATA_FLAG_HDR_SATIC_METADATA   = 0x00000001,
	/* hdr dynamic metadata is available */
	SUNXI_METADATA_FLAG_HDR_DYNAMIC_METADATA = 0x00000002,
	/* afbc header data is available */
	SUNXI_METADATA_FLAG_HDRP_HEADER          = 0x00000004,

	/* afbc header data is available */
	SUNXI_METADATA_FLAG_AFBC_HEADER          = 0x00000010,
	/* afbc buffer  has a 1080P buffer*/
	SUNXI_METADATA_FLAG_AFBC_BIG_BUFFER      = 0x80000000,
};

struct display_master_data {

    /* display primaries */
    uint16_t display_primaries_x[3];
    uint16_t display_primaries_y[3];

    /* white_point */
    uint16_t white_point_x;
    uint16_t white_point_y;

    /* max/min display mastering luminance */
    uint32_t max_display_mastering_luminance;
    uint32_t min_display_mastering_luminance;

};

/* static metadata type 1 */
struct hdr_static_metadata {
    struct display_master_data disp_master;

    uint16_t maximum_content_light_level;//* maxCLL
    uint16_t maximum_frame_average_light_level;//* maxFALL
};

struct hdr_10Plus_metadata {
    /*ex: 400, (-1: when dynamic metadata is not available)*/
    int32_t  targeted_system_display_maximum_luminance;
    uint16_t application_version; // 0 or 1
    uint16_t num_distributions; //ex: 9
    uint32_t maxscl[3]; //0x00000-0x186A0, (display side will divided by 10)
    uint32_t average_maxrgb; //0x00000-0x186A0, (display side will divided by 10)
    /*0x00000-0x186A0(i=0,1,3-9), 0-100(i=2), (display side will divided by 10 for i=0,1,3-9)*/
    uint32_t distribution_values[10];
    uint16_t knee_point_x; //0-4095, (display side will divided by 4095)
    uint16_t knee_point_y; //0-4095, (display side will divided by 4095)
    uint16_t num_bezier_curve_anchors; //0-9
    uint16_t bezier_curve_anchors[9]; //0-1023, (display side will divided by 1023)
    uint32_t divLut[NUM_DIV][MAX_LUT_SIZE];

    //uint32_t flag;
};

struct afbc_header {
    uint32_t signature;
    uint16_t filehdr_size;
    uint16_t version;
    uint32_t body_size;
    uint8_t ncomponents;
    uint8_t header_layout;
    uint8_t yuv_transform;
    uint8_t block_split;
    uint8_t inputbits[4];
    uint16_t block_width;
    uint16_t block_height;
    uint16_t width;
    uint16_t height;
    uint8_t left_crop;
    uint8_t top_crop;
    uint16_t block_layout;
};

/* sunxi metadata for ve and de */
struct sunxi_metadata {
    struct hdr_static_metadata hdr_smetada;
    struct hdr_10Plus_metadata hdr10_plus_smetada;
    struct afbc_header afbc_head;

    // flag must be at last always
    uint32_t flag;
};


#ifdef __cplusplus
}
#endif

#endif
