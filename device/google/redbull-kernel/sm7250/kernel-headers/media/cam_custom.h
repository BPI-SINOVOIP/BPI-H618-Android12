/****************************************************************************
 ****************************************************************************
 ***
 ***   This header was automatically generated from a Linux kernel header
 ***   of the same name, to make information necessary for userspace to
 ***   call into the kernel available to libc.  It contains only constants,
 ***   structures, and macros generated from the original header, and thus,
 ***   contains no copyrightable information.
 ***
 ***   To edit the content of this header, modify the corresponding
 ***   source file (e.g. under external/kernel-headers/original/) then
 ***   run bionic/libc/kernel/tools/update_all.py
 ***
 ***   Any manual change here will be lost the next time this script will
 ***   be run. You've been warned!
 ***
 ****************************************************************************
 ****************************************************************************/
#ifndef __UAPI_CAM_CUSTOM_H__
#define __UAPI_CAM_CUSTOM_H__
#include "cam_defs.h"
#define CAM_CUSTOM_DEV_NAME "cam-custom"
#define CAM_CUSTOM_NUM_SUB_DEVICES 2
#define CAM_CUSTOM_HW1 0
#define CAM_CUSTOM_HW2 1
#define CAM_CUSTOM_OUT_RES_UDI_0 1
#define CAM_CUSTOM_OUT_RES_UDI_1 2
#define CAM_CUSTOM_OUT_RES_UDI_2 3
#define CAM_CUSTOM_IN_RES_UDI_0 1
#define CAM_CUSTOM_RES_ID_PORT 0
#define CAM_CUSTOM_PACKET_OP_BASE 0
#define CAM_CUSTOM_PACKET_INIT_DEV 1
#define CAM_CUSTOM_PACKET_UPDATE_DEV 2
#define CAM_CUSTOM_PACKET_OP_MAX 3
#define CAM_CUSTOM_VC_DT_CFG_MAX 4
#define CAM_CUSTOM_IN_RES_BASE 0x5000
#define CAM_CUSTOM_IN_RES_PHY_0 (CAM_CUSTOM_IN_RES_BASE + 1)
#define CAM_CUSTOM_IN_RES_PHY_1 (CAM_CUSTOM_IN_RES_BASE + 2)
#define CAM_CUSTOM_IN_RES_PHY_2 (CAM_CUSTOM_IN_RES_BASE + 3)
#define CAM_CUSTOM_IN_RES_PHY_3 (CAM_CUSTOM_IN_RES_BASE + 4)
struct cam_custom_dev_cap_info {
  uint32_t hw_type;
  uint32_t hw_version;
};
struct cam_custom_query_cap_cmd {
  struct cam_iommu_handle device_iommu;
  struct cam_iommu_handle cdm_iommu;
  int32_t num_dev;
  uint32_t reserved;
  struct cam_custom_dev_cap_info dev_caps[CAM_CUSTOM_NUM_SUB_DEVICES];
};
struct cam_custom_out_port_info {
  uint32_t res_type;
  uint32_t format;
  uint32_t custom_info1;
  uint32_t custom_info2;
  uint32_t custom_info3;
  uint32_t reserved;
};
struct cam_custom_in_port_info {
  uint32_t res_type;
  uint32_t lane_type;
  uint32_t lane_num;
  uint32_t lane_cfg;
  uint32_t vc[CAM_CUSTOM_VC_DT_CFG_MAX];
  uint32_t dt[CAM_CUSTOM_VC_DT_CFG_MAX];
  uint32_t num_valid_vc_dt;
  uint32_t format;
  uint32_t test_pattern;
  uint32_t usage_type;
  uint32_t left_start;
  uint32_t left_stop;
  uint32_t left_width;
  uint32_t right_start;
  uint32_t right_stop;
  uint32_t right_width;
  uint32_t line_start;
  uint32_t line_stop;
  uint32_t height;
  uint32_t pixel_clk;
  uint32_t num_bytes_out;
  uint32_t custom_info1;
  uint32_t custom_info2;
  uint32_t num_out_res;
  struct cam_custom_out_port_info data[1];
};
struct cam_custom_resource {
  uint32_t resource_id;
  uint32_t length;
  uint32_t handle_type;
  uint32_t reserved;
  uint64_t res_hdl;
};
struct cam_custom_acquire_hw_info {
  uint32_t num_inputs;
  uint32_t input_info_size;
  uint32_t input_info_offset;
  uint32_t reserved;
  uint64_t data;
};
struct cam_custom_cmd_buf_type_1 {
  uint32_t custom_info;
  uint32_t reserved;
};
struct cam_custom_cmd_buf_type_2 {
  uint32_t custom_info1;
  uint32_t custom_info2;
  uint32_t custom_info3;
  uint32_t reserved;
};
#endif
