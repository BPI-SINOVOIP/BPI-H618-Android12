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
#ifndef _MSM_AUDIO_G711_H
#define _MSM_AUDIO_G711_H
#include <linux/msm_audio.h>
struct msm_audio_g711_dec_config {
  uint32_t sample_rate;
};
#define AUDIO_SET_G711_DEC_CONFIG _IOW(AUDIO_IOCTL_MAGIC, (AUDIO_MAX_COMMON_IOCTL_NUM + 0), struct msm_audio_g711_dec_config)
#define AUDIO_GET_G711_DEC_CONFIG _IOR(AUDIO_IOCTL_MAGIC, (AUDIO_MAX_COMMON_IOCTL_NUM + 1), struct msm_audio_g711_dec_config)
#endif
