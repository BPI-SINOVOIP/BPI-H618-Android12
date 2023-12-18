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
#ifndef __UAPI_CAM_CPAS_H__
#define __UAPI_CAM_CPAS_H__
#include <media/cam_defs.h>
#define CAM_FAMILY_CAMERA_SS 1
#define CAM_FAMILY_CPAS_SS 2
#define CAM_AXI_BW_VOTING_V2 2
#define CAM_AXI_TRANSACTION_READ 0
#define CAM_AXI_TRANSACTION_WRITE 1
#define CAM_AXI_PATH_DATA_IFE_START_OFFSET 0
#define CAM_AXI_PATH_DATA_IFE_LINEAR (CAM_AXI_PATH_DATA_IFE_START_OFFSET + 0)
#define CAM_AXI_PATH_DATA_IFE_VID (CAM_AXI_PATH_DATA_IFE_START_OFFSET + 1)
#define CAM_AXI_PATH_DATA_IFE_DISP (CAM_AXI_PATH_DATA_IFE_START_OFFSET + 2)
#define CAM_AXI_PATH_DATA_IFE_STATS (CAM_AXI_PATH_DATA_IFE_START_OFFSET + 3)
#define CAM_AXI_PATH_DATA_IFE_RDI0 (CAM_AXI_PATH_DATA_IFE_START_OFFSET + 4)
#define CAM_AXI_PATH_DATA_IFE_RDI1 (CAM_AXI_PATH_DATA_IFE_START_OFFSET + 5)
#define CAM_AXI_PATH_DATA_IFE_RDI2 (CAM_AXI_PATH_DATA_IFE_START_OFFSET + 6)
#define CAM_AXI_PATH_DATA_IFE_RDI3 (CAM_AXI_PATH_DATA_IFE_START_OFFSET + 7)
#define CAM_AXI_PATH_DATA_IFE_PDAF (CAM_AXI_PATH_DATA_IFE_START_OFFSET + 8)
#define CAM_AXI_PATH_DATA_IFE_PIXEL_RAW (CAM_AXI_PATH_DATA_IFE_START_OFFSET + 9)
#define CAM_AXI_PATH_DATA_IFE_MAX_OFFSET (CAM_AXI_PATH_DATA_IFE_START_OFFSET + 31)
#define CAM_AXI_PATH_DATA_IPE_START_OFFSET 32
#define CAM_AXI_PATH_DATA_IPE_RD_IN (CAM_AXI_PATH_DATA_IPE_START_OFFSET + 0)
#define CAM_AXI_PATH_DATA_IPE_RD_REF (CAM_AXI_PATH_DATA_IPE_START_OFFSET + 1)
#define CAM_AXI_PATH_DATA_IPE_WR_VID (CAM_AXI_PATH_DATA_IPE_START_OFFSET + 2)
#define CAM_AXI_PATH_DATA_IPE_WR_DISP (CAM_AXI_PATH_DATA_IPE_START_OFFSET + 3)
#define CAM_AXI_PATH_DATA_IPE_WR_REF (CAM_AXI_PATH_DATA_IPE_START_OFFSET + 4)
#define CAM_AXI_PATH_DATA_IPE_MAX_OFFSET (CAM_AXI_PATH_DATA_IPE_START_OFFSET + 31)
#define CAM_AXI_PATH_DATA_ALL 256
struct cam_cpas_query_cap {
  uint32_t camera_family;
  uint32_t reserved;
  struct cam_hw_version camera_version;
  struct cam_hw_version cpas_version;
};
struct cam_axi_per_path_bw_vote {
  uint32_t usage_data;
  uint32_t transac_type;
  uint32_t path_data_type;
  uint32_t reserved;
  uint64_t camnoc_bw;
  uint64_t mnoc_ab_bw;
  uint64_t mnoc_ib_bw;
  uint64_t ddr_ab_bw;
  uint64_t ddr_ib_bw;
};
#endif
