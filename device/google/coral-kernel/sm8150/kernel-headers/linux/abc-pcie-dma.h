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
#ifndef __ABC_PCIE_DMA_H
#define __ABC_PCIE_DMA_H
#include <linux/ioctl.h>
#include <linux/types.h>
#define ABC_PCIE_DMA_IOC_MAGIC 'U'
enum dma_buf_type {
  DMA_BUFFER_USER = 0,
  DMA_BUFFER_DMA_BUF
};
enum abc_dma_data_direction {
  ABC_DMA_TO_DEVICE = 1,
  ABC_DMA_FROM_DEVICE = 2,
};
struct abc_pcie_dma_desc_legacy {
  enum dma_buf_type local_buf_type;
  union {
    void * local_buf;
    int local_dma_buf_fd;
  };
  __u32 local_buf_size;
  enum dma_buf_type remote_buf_type;
  union {
    __u64 remote_buf;
    int remote_dma_buf_fd;
  };
  enum abc_dma_data_direction dir;
  __u8 chan;
};
struct abc_pcie_dma_desc {
  enum dma_buf_type local_buf_type;
  union {
    void * local_buf;
    int local_dma_buf_fd;
  };
  __u64 local_dma_buf_off;
  enum dma_buf_type remote_buf_type;
  union {
    __u64 remote_buf;
    int remote_dma_buf_fd;
  };
  __u64 remote_dma_buf_off;
  __u64 size;
  enum abc_dma_data_direction dir;
};
struct abc_pcie_dma_desc_async {
  struct abc_pcie_dma_desc dma_desc;
  __u64 id;
};
struct abc_pcie_dma_desc_start {
  __u64 id;
  __u32 start_id;
};
struct abc_pcie_dma_desc_wait {
  __u64 id;
  int timeout;
  int error;
  __u32 start_id;
};
#define ABC_PCIE_DMA_IOC_POST_DMA_XFER_LEGACY _IOW(ABC_PCIE_DMA_IOC_MAGIC, 1, struct abc_pcie_dma_desc_legacy *)
#define ABC_PCIE_DMA_IOC_POST_DMA_XFER_SYNC _IOW(ABC_PCIE_DMA_IOC_MAGIC, 2, struct abc_pcie_dma_desc)
#define ABC_PCIE_DMA_IOC_POST_DMA_XFER_CREATE _IOWR(ABC_PCIE_DMA_IOC_MAGIC, 3, struct abc_pcie_dma_desc_async)
#define ABC_PCIE_DMA_IOC_POST_DMA_XFER_START _IOWR(ABC_PCIE_DMA_IOC_MAGIC, 4, struct abc_pcie_dma_desc_start)
#define ABC_PCIE_DMA_IOC_POST_DMA_XFER_WAIT _IOWR(ABC_PCIE_DMA_IOC_MAGIC, 5, struct abc_pcie_dma_desc_wait)
#define ABC_PCIE_DMA_IOC_POST_DMA_XFER_CLEAN _IOW(ABC_PCIE_DMA_IOC_MAGIC, 6, __u64)
#endif
