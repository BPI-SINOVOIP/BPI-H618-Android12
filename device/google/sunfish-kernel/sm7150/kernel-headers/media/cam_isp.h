#ifndef __UAPI_CAM_ISP_H__
#define __UAPI_CAM_ISP_H__

#include "cam_defs.h"
#include "cam_isp_vfe.h"
#include "cam_isp_ife.h"
#ifdef CONFIG_BOARD_SUNFISH
#include "cam_cpas.h"
#endif /*CONFIG_BOARD_SUNFISH*/

#define CAM_ISP_DEV_NAME                        "cam-isp"

#define CAM_ISP_HW_BASE                         0
#define CAM_ISP_HW_CSID                         1
#define CAM_ISP_HW_VFE                          2
#define CAM_ISP_HW_IFE                          3
#define CAM_ISP_HW_ISPIF                        4
#define CAM_ISP_HW_MAX                          5

#define CAM_ISP_PATTERN_BAYER_RGRGRG            0
#define CAM_ISP_PATTERN_BAYER_GRGRGR            1
#define CAM_ISP_PATTERN_BAYER_BGBGBG            2
#define CAM_ISP_PATTERN_BAYER_GBGBGB            3
#define CAM_ISP_PATTERN_YUV_YCBYCR              4
#define CAM_ISP_PATTERN_YUV_YCRYCB              5
#define CAM_ISP_PATTERN_YUV_CBYCRY              6
#define CAM_ISP_PATTERN_YUV_CRYCBY              7
#define CAM_ISP_PATTERN_MAX                     8

#define CAM_ISP_RES_USAGE_SINGLE                0
#define CAM_ISP_RES_USAGE_DUAL                  1
#define CAM_ISP_RES_USAGE_MAX                   2

#define CAM_ISP_RES_ID_PORT                     0
#define CAM_ISP_RES_ID_CLK                      1
#define CAM_ISP_RES_ID_MAX                      2


#define CAM_ISP_LANE_TYPE_DPHY                  0
#define CAM_ISP_LANE_TYPE_CPHY                  1
#define CAM_ISP_LANE_TYPE_MAX                   2

#define CAM_ISP_RES_COMP_GROUP_NONE             0
#define CAM_ISP_RES_COMP_GROUP_ID_0             1
#define CAM_ISP_RES_COMP_GROUP_ID_1             2
#define CAM_ISP_RES_COMP_GROUP_ID_2             3
#define CAM_ISP_RES_COMP_GROUP_ID_3             4
#define CAM_ISP_RES_COMP_GROUP_ID_4             5
#define CAM_ISP_RES_COMP_GROUP_ID_5             6
#define CAM_ISP_RES_COMP_GROUP_ID_MAX           6

#define CAM_ISP_PACKET_OP_BASE                  0
#define CAM_ISP_PACKET_INIT_DEV                 1
#define CAM_ISP_PACKET_UPDATE_DEV               2
#define CAM_ISP_PACKET_OP_MAX                   3

#define CAM_ISP_PACKET_META_BASE                0
#define CAM_ISP_PACKET_META_LEFT                1
#define CAM_ISP_PACKET_META_RIGHT               2
#define CAM_ISP_PACKET_META_COMMON              3
#define CAM_ISP_PACKET_META_DMI_LEFT            4
#define CAM_ISP_PACKET_META_DMI_RIGHT           5
#define CAM_ISP_PACKET_META_DMI_COMMON          6
#define CAM_ISP_PACKET_META_CLOCK               7
#define CAM_ISP_PACKET_META_CSID                8
#define CAM_ISP_PACKET_META_DUAL_CONFIG         9
#define CAM_ISP_PACKET_META_GENERIC_BLOB_LEFT   10
#define CAM_ISP_PACKET_META_GENERIC_BLOB_RIGHT  11
#define CAM_ISP_PACKET_META_GENERIC_BLOB_COMMON 12

#define CAM_ISP_DSP_MODE_NONE                   0
#define CAM_ISP_DSP_MODE_ONE_WAY                1
#define CAM_ISP_DSP_MODE_ROUND                  2

#define CAM_ISP_GENERIC_BLOB_TYPE_HFR_CONFIG          0
#define CAM_ISP_GENERIC_BLOB_TYPE_CLOCK_CONFIG        1
#define CAM_ISP_GENERIC_BLOB_TYPE_BW_CONFIG           2
#define CAM_ISP_GENERIC_BLOB_TYPE_UBWC_CONFIG         3
#define CAM_ISP_GENERIC_BLOB_TYPE_CSID_CLOCK_CONFIG   4
#define CAM_ISP_GENERIC_BLOB_TYPE_FE_CONFIG           5
#define CAM_ISP_GENERIC_BLOB_TYPE_BW_CONFIG_V2        6


#ifdef CONFIG_BOARD_SUNFISH

 /* ISP Generic Cmd Buffer Blob types */
#define CAM_ISP_GENERIC_BLOB_TYPE_INIT_FRAME_DROP           10
#define CAM_ISP_GENERIC_BLOB_TYPE_SENSOR_DIMENSION_CONFIG   11
#define CAM_ISP_GENERIC_BLOB_TYPE_FPS_CONFIG                12

#define CAM_ISP_USAGE_INVALID     0
#define CAM_ISP_USAGE_LEFT_PX     1
#define CAM_ISP_USAGE_RIGHT_PX    2
#define CAM_ISP_USAGE_RDI         3

struct cam_isp_bw_config_v2 {
       uint32_t                             usage_type;
       uint32_t                             num_paths;
       struct cam_axi_per_path_bw_vote      axi_path[1];
} __attribute__((packed));

struct cam_isp_sensor_dimension {
       uint32_t width;
       uint32_t height;
       uint32_t measure_enabled;
} __attribute__((packed));

struct cam_isp_sensor_config {
       struct cam_isp_sensor_dimension  ppp_path;
       struct cam_isp_sensor_dimension  ipp_path;
       struct cam_isp_sensor_dimension  rdi_path[4];
       uint32_t                   hbi;
       uint32_t                   vbi;
} __attribute__((packed));


struct cam_isp_init_frame_drop_config {
       uint32_t                       init_frame_drop;
} __attribute__((packed));

struct cam_fps_config {
       uint32_t        fps;
} __attribute__((packed));

#endif /*CONFIG_BOARD_SUNFISH*/

struct cam_isp_dev_cap_info {
	uint32_t              hw_type;
	uint32_t              reserved;
	struct cam_hw_version hw_version;
};

struct cam_isp_query_cap_cmd {
	struct cam_iommu_handle       device_iommu;
	struct cam_iommu_handle       cdm_iommu;
	int32_t                       num_dev;
	uint32_t                      reserved;
	struct cam_isp_dev_cap_info   dev_caps[CAM_ISP_HW_MAX];
};

struct cam_isp_out_port_info {
	uint32_t                res_type;
	uint32_t                format;
	uint32_t                width;
	uint32_t                height;
	uint32_t                comp_grp_id;
	uint32_t                split_point;
	uint32_t                secure_mode;
	uint32_t                reserved;
};

struct cam_isp_in_port_info {
	uint32_t                        res_type;
	uint32_t                        lane_type;
	uint32_t                        lane_num;
	uint32_t                        lane_cfg;
	uint32_t                        vc;
	uint32_t                        dt;
	uint32_t                        format;
	uint32_t                        test_pattern;
	uint32_t                        usage_type;
	uint32_t                        left_start;
	uint32_t                        left_stop;
	uint32_t                        left_width;
	uint32_t                        right_start;
	uint32_t                        right_stop;
	uint32_t                        right_width;
	uint32_t                        line_start;
	uint32_t                        line_stop;
	uint32_t                        height;
	uint32_t                        pixel_clk;
	uint32_t                        batch_size;
	uint32_t                        dsp_mode;
	uint32_t                        hbi_cnt;
	uint32_t                        reserved;
	uint32_t                        num_out_res;
	struct cam_isp_out_port_info    data[1];
};

struct cam_isp_resource {
	uint32_t                       resource_id;
	uint32_t                       length;
	uint32_t                       handle_type;
	uint32_t                       reserved;
	uint64_t                       res_hdl;
};

struct cam_isp_port_hfr_config {
	uint32_t                       resource_type;
	uint32_t                       subsample_pattern;
	uint32_t                       subsample_period;
	uint32_t                       framedrop_pattern;
	uint32_t                       framedrop_period;
	uint32_t                       reserved;
} __attribute__((packed));

struct cam_isp_resource_hfr_config {
	uint32_t                       num_ports;
	uint32_t                       reserved;
	struct cam_isp_port_hfr_config port_hfr_config[1];
} __attribute__((packed));

struct cam_isp_dual_split_params {
	uint32_t                       split_point;
	uint32_t                       right_padding;
	uint32_t                       left_padding;
	uint32_t                       reserved;
};

struct cam_isp_dual_stripe_config {
	uint32_t                       offset;
	uint32_t                       width;
	uint32_t                       tileconfig;
	uint32_t                       port_id;
};

struct cam_isp_dual_config {
	uint32_t                           num_ports;
	uint32_t                           reserved;
	struct cam_isp_dual_split_params   split_params;
	struct cam_isp_dual_stripe_config  stripes[1];
} __attribute__((packed));

struct cam_isp_clock_config {
	uint32_t                       usage_type;
	uint32_t                       num_rdi;
	uint64_t                       left_pix_hz;
	uint64_t                       right_pix_hz;
	uint64_t                       rdi_hz[1];
} __attribute__((packed));

struct cam_isp_csid_clock_config {
	uint64_t                       csid_clock;
} __attribute__((packed));


struct cam_isp_bw_vote {
	uint32_t                       resource_id;
	uint32_t                       reserved;
	uint64_t                       cam_bw_bps;
	uint64_t                       ext_bw_bps;
} __attribute__((packed));


struct cam_isp_bw_config {
	uint32_t                       usage_type;
	uint32_t                       num_rdi;
	struct cam_isp_bw_vote         left_pix_vote;
	struct cam_isp_bw_vote         right_pix_vote;
	struct cam_isp_bw_vote         rdi_vote[1];
} __attribute__((packed));



struct cam_isp_bw_config_ab {
	uint32_t    usage_type;
	uint32_t    num_rdi;
	uint64_t    left_pix_vote_ab;
	uint64_t    right_pix_vote_ab;
	uint64_t    rdi_vote_ab[1];
} __attribute__((packed));

struct cam_fe_config {
	uint64_t    version;
	uint32_t    min_vbi;
	uint32_t    fs_mode;
	uint32_t    fs_line_sync_en;
	uint32_t    hbi_count;
	uint32_t    fs_sync_enable;
	uint32_t    go_cmd_sel;
	uint32_t    client_enable;
	uint32_t    source_addr;
	uint32_t    width;
	uint32_t    height;
	uint32_t    stride;
	uint32_t    format;
	uint32_t    unpacker_cfg;
	uint32_t    latency_buf_size;
} __attribute__((packed));


struct cam_isp_acquire_hw_info {
	uint16_t                common_info_version;
	uint16_t                common_info_size;
	uint32_t                common_info_offset;
	uint32_t                num_inputs;
	uint32_t                input_info_version;
	uint32_t                input_info_size;
	uint32_t                input_info_offset;
	uint64_t                data;
};

#define CAM_ISP_ACQUIRE_COMMON_VER0         0x1000

#define CAM_ISP_ACQUIRE_COMMON_SIZE_VER0    0x0

#define CAM_ISP_ACQUIRE_INPUT_VER0          0x2000

#define CAM_ISP_ACQUIRE_INPUT_SIZE_VER0     sizeof(struct cam_isp_in_port_info)

#define CAM_ISP_ACQUIRE_OUT_VER0            0x3000

#define CAM_ISP_ACQUIRE_OUT_SIZE_VER0       sizeof(struct cam_isp_out_port_info)

#endif /* __UAPI_CAM_ISP_H__ */
