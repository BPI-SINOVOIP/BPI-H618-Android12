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
#ifndef _MSM_NPU_H_
#define _MSM_NPU_H_
#include <linux/types.h>
#define MSM_NPU_IOCTL_MAGIC 'n'
#define MSM_NPU_GET_INFO _IOWR(MSM_NPU_IOCTL_MAGIC, 1, struct msm_npu_get_info_ioctl)
#define MSM_NPU_MAP_BUF _IOWR(MSM_NPU_IOCTL_MAGIC, 2, struct msm_npu_map_buf_ioctl)
#define MSM_NPU_UNMAP_BUF _IOWR(MSM_NPU_IOCTL_MAGIC, 3, struct msm_npu_unmap_buf_ioctl)
#define MSM_NPU_LOAD_NETWORK _IOWR(MSM_NPU_IOCTL_MAGIC, 4, struct msm_npu_load_network_ioctl)
#define MSM_NPU_UNLOAD_NETWORK _IOWR(MSM_NPU_IOCTL_MAGIC, 5, struct msm_npu_unload_network_ioctl)
#define MSM_NPU_EXEC_NETWORK _IOWR(MSM_NPU_IOCTL_MAGIC, 6, struct msm_npu_exec_network_ioctl)
#define MSM_NPU_LOAD_NETWORK_V2 _IOWR(MSM_NPU_IOCTL_MAGIC, 7, struct msm_npu_load_network_ioctl_v2)
#define MSM_NPU_EXEC_NETWORK_V2 _IOWR(MSM_NPU_IOCTL_MAGIC, 8, struct msm_npu_exec_network_ioctl_v2)
#define MSM_NPU_RECEIVE_EVENT _IOR(MSM_NPU_IOCTL_MAGIC, 9, struct msm_npu_event)
#define MSM_NPU_SET_PROP _IOW(MSM_NPU_IOCTL_MAGIC, 10, struct msm_npu_property)
#define MSM_NPU_GET_PROP _IOW(MSM_NPU_IOCTL_MAGIC, 11, struct msm_npu_property)
#define MSM_NPU_EVENT_TYPE_START 0x10000000
#define MSM_NPU_EVENT_TYPE_EXEC_DONE (MSM_NPU_EVENT_TYPE_START + 1)
#define MSM_NPU_EVENT_TYPE_EXEC_V2_DONE (MSM_NPU_EVENT_TYPE_START + 2)
#define MSM_NPU_EVENT_TYPE_SSR (MSM_NPU_EVENT_TYPE_START + 3)
#define MSM_NPU_MAX_INPUT_LAYER_NUM 8
#define MSM_NPU_MAX_OUTPUT_LAYER_NUM 4
#define MSM_NPU_MAX_PATCH_LAYER_NUM (MSM_NPU_MAX_INPUT_LAYER_NUM + MSM_NPU_MAX_OUTPUT_LAYER_NUM)
#define MSM_NPU_PROP_ID_START 0x100
#define MSM_NPU_PROP_ID_FW_STATE (MSM_NPU_PROP_ID_START + 0)
#define MSM_NPU_PROP_ID_PERF_MODE (MSM_NPU_PROP_ID_START + 1)
#define MSM_NPU_PROP_ID_PERF_MODE_MAX (MSM_NPU_PROP_ID_START + 2)
#define MSM_NPU_PROP_ID_DRV_VERSION (MSM_NPU_PROP_ID_START + 3)
#define MSM_NPU_PROP_ID_HARDWARE_VERSION (MSM_NPU_PROP_ID_START + 4)
#define MSM_NPU_PROP_ID_IPC_QUEUE_INFO (MSM_NPU_PROP_ID_START + 5)
#define MSM_NPU_PROP_ID_DRV_FEATURE (MSM_NPU_PROP_ID_START + 6)
#define MSM_NPU_FW_PROP_ID_START 0x1000
#define MSM_NPU_PROP_ID_DCVS_MODE (MSM_NPU_FW_PROP_ID_START + 0)
#define MSM_NPU_PROP_ID_DCVS_MODE_MAX (MSM_NPU_FW_PROP_ID_START + 1)
#define MSM_NPU_PROP_ID_CLK_GATING_MODE (MSM_NPU_FW_PROP_ID_START + 2)
#define MSM_NPU_PROP_ID_HW_VERSION (MSM_NPU_FW_PROP_ID_START + 3)
#define MSM_NPU_PROP_ID_FW_VERSION (MSM_NPU_FW_PROP_ID_START + 4)
#define MSM_NPU_PROP_ID_FW_GETCAPS (MSM_NPU_FW_PROP_ID_START + 5)
#define MSM_NPU_FEATURE_MULTI_EXECUTE 0x1
#define MSM_NPU_FEATURE_ASYNC_EXECUTE 0x2
#define PROP_PARAM_MAX_SIZE 8
struct msm_npu_patch_info {
  uint32_t chunk_id;
  uint16_t instruction_size_in_bytes;
  uint16_t variable_size_in_bits;
  uint16_t shift_value_in_bits;
  uint32_t loc_offset;
};
struct msm_npu_layer {
  uint32_t layer_id;
  struct msm_npu_patch_info patch_info;
  int32_t buf_hdl;
  uint32_t buf_size;
  uint64_t buf_phys_addr;
};
struct msm_npu_patch_info_v2 {
  uint32_t value;
  uint32_t chunk_id;
  uint32_t instruction_size_in_bytes;
  uint32_t variable_size_in_bits;
  uint32_t shift_value_in_bits;
  uint32_t loc_offset;
};
struct msm_npu_patch_buf_info {
  uint64_t buf_phys_addr;
  uint32_t buf_id;
};
struct msm_npu_map_buf_ioctl {
  int32_t buf_ion_hdl;
  uint32_t size;
  uint64_t npu_phys_addr;
};
struct msm_npu_unmap_buf_ioctl {
  int32_t buf_ion_hdl;
  uint64_t npu_phys_addr;
};
struct msm_npu_get_info_ioctl {
  uint32_t firmware_version;
  uint32_t flags;
};
struct msm_npu_load_network_ioctl {
  int32_t buf_ion_hdl;
  uint64_t buf_phys_addr;
  uint32_t buf_size;
  uint32_t first_block_size;
  uint32_t flags;
  uint32_t network_hdl;
  uint32_t priority;
  uint32_t perf_mode;
};
struct msm_npu_load_network_ioctl_v2 {
  uint64_t buf_phys_addr;
  uint64_t patch_info;
  int32_t buf_ion_hdl;
  uint32_t buf_size;
  uint32_t first_block_size;
  uint32_t flags;
  uint32_t network_hdl;
  uint32_t priority;
  uint32_t perf_mode;
  uint32_t num_layers;
  uint32_t patch_info_num;
  uint32_t reserved;
};
struct msm_npu_unload_network_ioctl {
  uint32_t network_hdl;
};
struct msm_npu_exec_network_ioctl {
  uint32_t network_hdl;
  uint32_t input_layer_num;
  struct msm_npu_layer input_layers[MSM_NPU_MAX_INPUT_LAYER_NUM];
  uint32_t output_layer_num;
  struct msm_npu_layer output_layers[MSM_NPU_MAX_OUTPUT_LAYER_NUM];
  uint32_t patching_required;
  uint32_t async;
  uint32_t flags;
};
struct msm_npu_exec_network_ioctl_v2 {
  uint64_t stats_buf_addr;
  uint64_t patch_buf_info;
  uint32_t network_hdl;
  uint32_t async;
  uint32_t flags;
  uint32_t stats_buf_size;
  uint32_t patch_buf_info_num;
  uint32_t reserved;
};
struct msm_npu_event_execute_done {
  uint32_t network_hdl;
  int32_t exec_result;
};
struct msm_npu_event_execute_v2_done {
  uint32_t network_hdl;
  int32_t exec_result;
  uint32_t stats_buf_size;
};
struct msm_npu_event_ssr {
  uint32_t network_hdl;
};
struct msm_npu_event {
  uint32_t type;
  union {
    struct msm_npu_event_execute_done exec_done;
    struct msm_npu_event_execute_v2_done exec_v2_done;
    struct msm_npu_event_ssr ssr;
    uint8_t data[128];
  } u;
  uint32_t reserved[4];
};
struct msm_npu_property {
  uint32_t prop_id;
  uint32_t num_of_params;
  uint32_t network_hdl;
  uint32_t prop_param[PROP_PARAM_MAX_SIZE];
};
#endif
