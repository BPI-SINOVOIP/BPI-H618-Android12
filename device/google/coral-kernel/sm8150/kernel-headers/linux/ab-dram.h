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
#ifndef __AB_DRAM_UAPI_H__
#define __AB_DRAM_UAPI_H__
#include <linux/ioctl.h>
#include <linux/types.h>
enum ab_dram_alloc_flag {
  ABD_ALLOC_NON_CONTIGUOUS,
  ABD_ALLOC_CONTIGUOUS,
};
struct ab_dram_alloc_request {
  __kernel_size_t size;
  enum ab_dram_alloc_flag flag;
};
#define AB_DRAM_ALLOCATE_MEMORY_LEGACY _IOW('a', 1, __kernel_size_t)
#define AB_DRAM_ALLOCATE_MEMORY _IOW('a', 2, struct ab_dram_alloc_request)
#endif
