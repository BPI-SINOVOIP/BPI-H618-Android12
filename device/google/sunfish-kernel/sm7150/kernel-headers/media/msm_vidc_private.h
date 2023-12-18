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
#ifndef __MSM_VIDC_PRIVATE_H__
#define __MSM_VIDC_PRIVATE_H__
#include <linux/videodev2.h>
#define VIDIOC_VIDEO_CMD _IOWR('V', BASE_VIDIOC_PRIVATE_VIDEO, struct msm_vidc_arg)
#define MSM_VIDC_CMD_START 0x10000000
#define MSM_CVP_START (MSM_VIDC_CMD_START + 0x1000)
#define MSM_CVP_GET_SESSION_INFO (MSM_CVP_START + 1)
#define MSM_CVP_REQUEST_POWER (MSM_CVP_START + 2)
#define MSM_CVP_REGISTER_BUFFER (MSM_CVP_START + 3)
#define MSM_CVP_UNREGISTER_BUFFER (MSM_CVP_START + 4)
#define MSM_CVP_FLAG_UNSECURE 0x00000000
#define MSM_CVP_FLAG_SECURE 0x00000001
#define MSM_CVP_BUFTYPE_INPUT 0x00000001
#define MSM_CVP_BUFTYPE_OUTPUT 0x00000002
#define MSM_CVP_BUFTYPE_INTERNAL_1 0x00000003
#define MSM_CVP_BUFTYPE_INTERNAL_2 0x00000004
struct msm_cvp_session_info {
  unsigned int session_id;
  unsigned int reserved[10];
};
struct msm_cvp_request_power {
  unsigned int clock_cycles_a;
  unsigned int clock_cycles_b;
  unsigned int ddr_bw;
  unsigned int sys_cache_bw;
  unsigned int reserved[8];
};
struct msm_cvp_buffer {
  unsigned int index;
  unsigned int type;
  unsigned int fd;
  unsigned int size;
  unsigned int offset;
  unsigned int pixelformat;
  unsigned int flags;
  unsigned int reserved[5];
};
struct msm_vidc_arg {
  unsigned int type;
  union data_t {
    struct msm_cvp_session_info session;
    struct msm_cvp_request_power req_power;
    struct msm_cvp_buffer regbuf;
    struct msm_cvp_buffer unregbuf;
  } data;
  unsigned int reserved[12];
};
#endif
