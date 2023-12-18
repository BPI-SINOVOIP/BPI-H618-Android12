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
#ifndef __UAPI_CAM_DEFS_H__
#define __UAPI_CAM_DEFS_H__
#include <linux/videodev2.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#define CAM_COMMON_OPCODE_BASE 0x100
#define CAM_QUERY_CAP (CAM_COMMON_OPCODE_BASE + 0x1)
#define CAM_ACQUIRE_DEV (CAM_COMMON_OPCODE_BASE + 0x2)
#define CAM_START_DEV (CAM_COMMON_OPCODE_BASE + 0x3)
#define CAM_STOP_DEV (CAM_COMMON_OPCODE_BASE + 0x4)
#define CAM_CONFIG_DEV (CAM_COMMON_OPCODE_BASE + 0x5)
#define CAM_RELEASE_DEV (CAM_COMMON_OPCODE_BASE + 0x6)
#define CAM_SD_SHUTDOWN (CAM_COMMON_OPCODE_BASE + 0x7)
#define CAM_FLUSH_REQ (CAM_COMMON_OPCODE_BASE + 0x8)
#define CAM_COMMON_OPCODE_MAX (CAM_COMMON_OPCODE_BASE + 0x9)
#define CAM_COMMON_OPCODE_BASE_v2 0x150
#define CAM_ACQUIRE_HW (CAM_COMMON_OPCODE_BASE_v2 + 0x1)
#define CAM_RELEASE_HW (CAM_COMMON_OPCODE_BASE_v2 + 0x2)
#define CAM_DUMP_REQ (CAM_COMMON_OPCODE_BASE_v2 + 0x3)
#define CAM_EXT_OPCODE_BASE 0x200
#define CAM_CONFIG_DEV_EXTERNAL (CAM_EXT_OPCODE_BASE + 0x1)
#define CAM_HANDLE_USER_POINTER 1
#define CAM_HANDLE_MEM_HANDLE 2
#define CAM_GENERIC_BLOB_CMDBUFFER_SIZE_MASK 0xFFFFFF00
#define CAM_GENERIC_BLOB_CMDBUFFER_SIZE_SHIFT 8
#define CAM_GENERIC_BLOB_CMDBUFFER_TYPE_MASK 0xFF
#define CAM_GENERIC_BLOB_CMDBUFFER_TYPE_SHIFT 0
#define CAM_CMD_BUF_DMI 0x1
#define CAM_CMD_BUF_DMI16 0x2
#define CAM_CMD_BUF_DMI32 0x3
#define CAM_CMD_BUF_DMI64 0x4
#define CAM_CMD_BUF_DIRECT 0x5
#define CAM_CMD_BUF_INDIRECT 0x6
#define CAM_CMD_BUF_I2C 0x7
#define CAM_CMD_BUF_FW 0x8
#define CAM_CMD_BUF_GENERIC 0x9
#define CAM_CMD_BUF_LEGACY 0xA
#define CAM_UBWC_CFG_VERSION_1 1
#define CAM_UBWC_CFG_VERSION_2 2
#define CAM_MAX_ACQ_RES 5
#define CAM_MAX_HW_SPLIT 3
enum flush_type_t {
  CAM_FLUSH_TYPE_REQ,
  CAM_FLUSH_TYPE_ALL,
  CAM_FLUSH_TYPE_MAX
};
struct cam_control {
  uint32_t op_code;
  uint32_t size;
  uint32_t handle_type;
  uint32_t reserved;
  uint64_t handle;
};
#define VIDIOC_CAM_CONTROL _IOWR('V', BASE_VIDIOC_PRIVATE, struct cam_control)
struct cam_hw_version {
  uint32_t major;
  uint32_t minor;
  uint32_t incr;
  uint32_t reserved;
};
struct cam_iommu_handle {
  int32_t non_secure;
  int32_t secure;
};
#define CAM_SECURE_MODE_NON_SECURE 0
#define CAM_SECURE_MODE_SECURE 1
#define CAM_FORMAT_BASE 0
#define CAM_FORMAT_MIPI_RAW_6 1
#define CAM_FORMAT_MIPI_RAW_8 2
#define CAM_FORMAT_MIPI_RAW_10 3
#define CAM_FORMAT_MIPI_RAW_12 4
#define CAM_FORMAT_MIPI_RAW_14 5
#define CAM_FORMAT_MIPI_RAW_16 6
#define CAM_FORMAT_MIPI_RAW_20 7
#define CAM_FORMAT_QTI_RAW_8 8
#define CAM_FORMAT_QTI_RAW_10 9
#define CAM_FORMAT_QTI_RAW_12 10
#define CAM_FORMAT_QTI_RAW_14 11
#define CAM_FORMAT_PLAIN8 12
#define CAM_FORMAT_PLAIN16_8 13
#define CAM_FORMAT_PLAIN16_10 14
#define CAM_FORMAT_PLAIN16_12 15
#define CAM_FORMAT_PLAIN16_14 16
#define CAM_FORMAT_PLAIN16_16 17
#define CAM_FORMAT_PLAIN32_20 18
#define CAM_FORMAT_PLAIN64 19
#define CAM_FORMAT_PLAIN128 20
#define CAM_FORMAT_ARGB 21
#define CAM_FORMAT_ARGB_10 22
#define CAM_FORMAT_ARGB_12 23
#define CAM_FORMAT_ARGB_14 24
#define CAM_FORMAT_DPCM_10_6_10 25
#define CAM_FORMAT_DPCM_10_8_10 26
#define CAM_FORMAT_DPCM_12_6_12 27
#define CAM_FORMAT_DPCM_12_8_12 28
#define CAM_FORMAT_DPCM_14_8_14 29
#define CAM_FORMAT_DPCM_14_10_14 30
#define CAM_FORMAT_NV21 31
#define CAM_FORMAT_NV12 32
#define CAM_FORMAT_TP10 33
#define CAM_FORMAT_YUV422 34
#define CAM_FORMAT_PD8 35
#define CAM_FORMAT_PD10 36
#define CAM_FORMAT_UBWC_NV12 37
#define CAM_FORMAT_UBWC_NV12_4R 38
#define CAM_FORMAT_UBWC_TP10 39
#define CAM_FORMAT_UBWC_P010 40
#define CAM_FORMAT_PLAIN8_SWAP 41
#define CAM_FORMAT_PLAIN8_10 42
#define CAM_FORMAT_PLAIN8_10_SWAP 43
#define CAM_FORMAT_YV12 44
#define CAM_FORMAT_Y_ONLY 45
#define CAM_FORMAT_DPCM_12_10_12 46
#define CAM_FORMAT_PLAIN32 47
#define CAM_FORMAT_ARGB_16 48
#define CAM_FORMAT_MAX 49
#define CAM_ROTATE_CW_0_DEGREE 0
#define CAM_ROTATE_CW_90_DEGREE 1
#define CAM_RORATE_CW_180_DEGREE 2
#define CAM_ROTATE_CW_270_DEGREE 3
#define CAM_COLOR_SPACE_BASE 0
#define CAM_COLOR_SPACE_BT601_FULL 1
#define CAM_COLOR_SPACE_BT601625 2
#define CAM_COLOR_SPACE_BT601525 3
#define CAM_COLOR_SPACE_BT709 4
#define CAM_COLOR_SPACE_DEPTH 5
#define CAM_COLOR_SPACE_MAX 6
#define CAM_BUF_INPUT 1
#define CAM_BUF_OUTPUT 2
#define CAM_BUF_IN_OUT 3
#define CAM_PACKET_DEV_BASE 0
#define CAM_PACKET_DEV_IMG_SENSOR 1
#define CAM_PACKET_DEV_ACTUATOR 2
#define CAM_PACKET_DEV_COMPANION 3
#define CAM_PACKET_DEV_EEPOM 4
#define CAM_PACKET_DEV_CSIPHY 5
#define CAM_PACKET_DEV_OIS 6
#define CAM_PACKET_DEV_FLASH 7
#define CAM_PACKET_DEV_FD 8
#define CAM_PACKET_DEV_JPEG_ENC 9
#define CAM_PACKET_DEV_JPEG_DEC 10
#define CAM_PACKET_DEV_VFE 11
#define CAM_PACKET_DEV_CPP 12
#define CAM_PACKET_DEV_CSID 13
#define CAM_PACKET_DEV_ISPIF 14
#define CAM_PACKET_DEV_IFE 15
#define CAM_PACKET_DEV_ICP 16
#define CAM_PACKET_DEV_LRME 17
#define CAM_PACKET_DEV_MAX 18
#define CAM_REG_DUMP_BASE_TYPE_ISP_LEFT 1
#define CAM_REG_DUMP_BASE_TYPE_ISP_RIGHT 2
#define CAM_REG_DUMP_BASE_TYPE_CAMNOC 3
#define CAM_REG_DUMP_READ_TYPE_CONT_RANGE 1
#define CAM_REG_DUMP_READ_TYPE_DMI 2
#define CAM_REG_DUMP_DMI_CONFIG_MAX 5
#define CAM_PACKET_MAX_PLANES 3
struct cam_plane_cfg {
  uint32_t width;
  uint32_t height;
  uint32_t plane_stride;
  uint32_t slice_height;
  uint32_t meta_stride;
  uint32_t meta_size;
  uint32_t meta_offset;
  uint32_t packer_config;
  uint32_t mode_config;
  uint32_t tile_config;
  uint32_t h_init;
  uint32_t v_init;
};
struct cam_ubwc_plane_cfg_v1 {
  uint32_t port_type;
  uint32_t meta_stride;
  uint32_t meta_size;
  uint32_t meta_offset;
  uint32_t packer_config;
  uint32_t mode_config_0;
  uint32_t mode_config_1;
  uint32_t tile_config;
  uint32_t h_init;
  uint32_t v_init;
};
struct cam_ubwc_plane_cfg_v2 {
  uint32_t port_type;
  uint32_t meta_stride;
  uint32_t meta_size;
  uint32_t meta_offset;
  uint32_t packer_config;
  uint32_t mode_config_0;
  uint32_t mode_config_1;
  uint32_t tile_config;
  uint32_t h_init;
  uint32_t v_init;
  uint32_t static_ctrl;
  uint32_t ctrl_2;
  uint32_t stats_ctrl_2;
  uint32_t lossy_threshold_0;
  uint32_t lossy_threshold_1;
  uint32_t lossy_var_offset;
  uint32_t bandwidth_limit;
  uint32_t reserved[3];
};
struct cam_cmd_buf_desc {
  int32_t mem_handle;
  uint32_t offset;
  uint32_t size;
  uint32_t length;
  uint32_t type;
  uint32_t meta_data;
};
struct cam_buf_io_cfg {
  int32_t mem_handle[CAM_PACKET_MAX_PLANES];
  uint32_t offsets[CAM_PACKET_MAX_PLANES];
  struct cam_plane_cfg planes[CAM_PACKET_MAX_PLANES];
  uint32_t format;
  uint32_t color_space;
  uint32_t color_pattern;
  uint32_t bpp;
  uint32_t rotation;
  uint32_t resource_type;
  int32_t fence;
  int32_t early_fence;
  struct cam_cmd_buf_desc aux_cmd_buf;
  uint32_t direction;
  uint32_t batch_size;
  uint32_t subsample_pattern;
  uint32_t subsample_period;
  uint32_t framedrop_pattern;
  uint32_t framedrop_period;
  uint32_t flag;
  uint32_t padding;
};
struct cam_packet_header {
  uint32_t op_code;
  uint32_t size;
  uint64_t request_id;
  uint32_t flags;
  uint32_t padding;
};
struct cam_patch_desc {
  int32_t dst_buf_hdl;
  uint32_t dst_offset;
  int32_t src_buf_hdl;
  uint32_t src_offset;
};
struct cam_packet {
  struct cam_packet_header header;
  uint32_t cmd_buf_offset;
  uint32_t num_cmd_buf;
  uint32_t io_configs_offset;
  uint32_t num_io_configs;
  uint32_t patch_offset;
  uint32_t num_patches;
  uint32_t kmd_cmd_buf_index;
  uint32_t kmd_cmd_buf_offset;
  uint64_t payload[1];
};
struct cam_release_dev_cmd {
  int32_t session_handle;
  int32_t dev_handle;
};
struct cam_start_stop_dev_cmd {
  int32_t session_handle;
  int32_t dev_handle;
};
struct cam_config_dev_cmd {
  int32_t session_handle;
  int32_t dev_handle;
  uint64_t offset;
  uint64_t packet_handle;
};
struct cam_query_cap_cmd {
  uint32_t size;
  uint32_t handle_type;
  uint64_t caps_handle;
};
struct cam_acquire_dev_cmd {
  int32_t session_handle;
  int32_t dev_handle;
  uint32_t handle_type;
  uint32_t num_resources;
  uint64_t resource_hdl;
};
#define CAM_API_COMPAT_CONSTANT 0xFEFEFEFE
#define CAM_ACQUIRE_HW_STRUCT_VERSION_1 1
#define CAM_ACQUIRE_HW_STRUCT_VERSION_2 2
struct cam_acquire_hw_cmd_v1 {
  uint32_t struct_version;
  uint32_t reserved;
  int32_t session_handle;
  int32_t dev_handle;
  uint32_t handle_type;
  uint32_t data_size;
  uint64_t resource_hdl;
};
struct cam_acquired_hw_info {
  uint32_t acquired_hw_id[CAM_MAX_ACQ_RES];
  uint32_t acquired_hw_path[CAM_MAX_ACQ_RES][CAM_MAX_HW_SPLIT];
  uint32_t valid_acquired_hw;
};
struct cam_acquire_hw_cmd_v2 {
  uint32_t struct_version;
  uint32_t reserved;
  int32_t session_handle;
  int32_t dev_handle;
  uint32_t handle_type;
  uint32_t data_size;
  uint64_t resource_hdl;
  struct cam_acquired_hw_info hw_info;
};
#define CAM_RELEASE_HW_STRUCT_VERSION_1 1
struct cam_release_hw_cmd_v1 {
  uint32_t struct_version;
  uint32_t reserved;
  int32_t session_handle;
  int32_t dev_handle;
};
struct cam_flush_dev_cmd {
  uint64_t version;
  int32_t session_handle;
  int32_t dev_handle;
  uint32_t flush_type;
  uint32_t reserved;
  int64_t req_id;
};
struct cam_ubwc_config {
  uint32_t api_version;
  uint32_t num_ports;
  struct cam_ubwc_plane_cfg_v1 ubwc_plane_cfg[1][CAM_PACKET_MAX_PLANES - 1];
};
struct cam_ubwc_config_v2 {
  uint32_t api_version;
  uint32_t num_ports;
  struct cam_ubwc_plane_cfg_v2 ubwc_plane_cfg[1][CAM_PACKET_MAX_PLANES - 1];
};
struct cam_cmd_mem_region_info {
  int32_t mem_handle;
  uint32_t offset;
  uint32_t size;
  uint32_t flags;
};
struct cam_cmd_mem_regions {
  uint32_t version;
  uint32_t num_regions;
  struct cam_cmd_mem_region_info map_info_array[1];
};
struct cam_reg_write_desc {
  uint32_t offset;
  uint32_t value;
};
struct cam_reg_range_read_desc {
  uint32_t offset;
  uint32_t num_values;
};
struct cam_dmi_read_desc {
  uint32_t num_pre_writes;
  uint32_t num_post_writes;
  struct cam_reg_write_desc pre_read_config[CAM_REG_DUMP_DMI_CONFIG_MAX];
  struct cam_reg_range_read_desc dmi_data_read;
  struct cam_reg_write_desc post_read_config[CAM_REG_DUMP_DMI_CONFIG_MAX];
};
struct cam_reg_read_info {
  uint32_t type;
  uint32_t reserved;
  union {
    struct cam_reg_range_read_desc reg_read;
    struct cam_dmi_read_desc dmi_read;
  };
};
struct cam_reg_dump_out_buffer {
  uint64_t req_id;
  uint32_t bytes_written;
  uint32_t dump_data[1];
};
struct cam_reg_dump_desc {
  uint32_t reg_base_type;
  uint32_t dump_buffer_offset;
  uint32_t dump_buffer_size;
  uint32_t num_read_range;
  struct cam_reg_read_info read_range[1];
};
struct cam_reg_dump_input_info {
  uint32_t num_dump_sets;
  uint32_t dump_set_offsets[1];
};
struct cam_dump_req_cmd {
  uint64_t issue_req_id;
  size_t offset;
  uint32_t buf_handle;
  uint32_t error_type;
  int32_t session_handle;
  int32_t link_hdl;
  int32_t dev_handle;
};
#endif
