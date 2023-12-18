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
#ifndef __MSM_HDMI_HDCP_MGR_H
#define __MSM_HDMI_HDCP_MGR_H
enum DS_TYPE {
  DS_UNKNOWN,
  DS_RECEIVER,
  DS_REPEATER,
};
enum {
  MSG_ID_IDX,
  RET_CODE_IDX,
  HEADER_LEN,
};
enum RET_CODE {
  HDCP_NOT_AUTHED,
  HDCP_AUTHED,
  HDCP_DISABLE,
};
enum MSG_ID {
  DOWN_CHECK_TOPOLOGY,
  UP_REQUEST_TOPOLOGY,
  UP_SEND_TOPOLOGY,
  DOWN_REQUEST_TOPOLOGY,
  MSG_NUM,
};
enum SOURCE_ID {
  HDCP_V1_TX,
  HDCP_V1_RX,
  HDCP_V2_RX,
  HDCP_V2_TX,
  SRC_NUM,
};
struct HDCP_V2V1_MSG_TOPOLOGY {
  uint32_t ds_type;
  uint8_t bksv[5];
  uint8_t dev_count;
  uint8_t depth;
  uint8_t ksv_list[5 * 127];
  uint32_t max_cascade_exceeded;
  uint32_t max_dev_exceeded;
};
#endif
