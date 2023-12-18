/*
* Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
* All rights reserved.
*
* File : sunxi_metadata.h
* Description :
* History :
*   Author  : xyliu <xyliu@allwinnertech.com>
*   Date    : 2017/04/13
*   Comment :
*
*
*/

#ifndef __SUNXI_METADATA_DEF_H__
#define __SUNXI_METADATA_DEF_H__

#include <stdint.h>

enum {
    /* hdr static metadata is available */
    SUNXI_METADATA_FLAG_HDR_SATIC_METADATA   = 0x00000001,
    /* hdr dynamic metadata is available */
    SUNXI_METADATA_FLAG_HDR_DYNAMIC_METADATA = 0x00000002,

    /* hdr plus metadata is available */
    SUNXI_METADATA_FLAG_HDR_PLUS_METADATA   = 0x00000004,

    /* afbc header data is available */
    SUNXI_METADATA_FLAG_AFBC_HEADER          = 0x00000010,
};

#define NUM_DIV 8
#define MAX_LUT_SIZE 729

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
    int32_t  targeted_system_display_maximum_luminance;  //ex: 400, (-1: when dynamic metadata is not available)
    uint16_t application_version; // 0 or 1
    uint16_t num_distributions; //ex: 9
    uint32_t maxscl[3]; //0x00000-0x186A0, (display side will divided by 10)
    uint32_t average_maxrgb; //0x00000-0x186A0, (display side will divided by 10)
    uint32_t distribution_values[10]; //0x00000-0x186A0(i=0,1,3-9), 0-100(i=2), (display side will divided by 10 for i=0,1,3-9)
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

#endif /* __SUNXI_METADATA_DEF_H__ */
