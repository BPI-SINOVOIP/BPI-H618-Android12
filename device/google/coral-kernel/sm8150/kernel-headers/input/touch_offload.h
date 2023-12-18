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
#ifndef _TOUCH_OFFLOAD_H
#define _TOUCH_OFFLOAD_H
#define TOUCH_OFFLOAD_MAGIC '7'
#define BUS_TYPE_I2C 0
#define BUS_TYPE_SPI 1
#define BUS_TYPE_I3C 2
#define HEATMAP_SIZE_FULL (1 << 31)
#define TOUCH_DATA_TYPE_COORD 0x01
#define TOUCH_DATA_TYPE_RAW 0x02
#define TOUCH_DATA_TYPE_FILTERED 0x04
#define TOUCH_DATA_TYPE_BASELINE 0x08
#define TOUCH_DATA_TYPE_STRENGTH 0x10
#define TOUCH_SCAN_TYPE_MUTUAL 0x40
#define TOUCH_SCAN_TYPE_SELF 0x80
struct TouchOffloadCaps {
  int tx_size;
  int rx_size;
  int bus_type;
  __u32 bus_speed_hz;
  int heatmap_size;
  int touch_data_types;
  int touch_scan_types;
  bool continuous_reporting;
  bool noise_reporting;
  bool cancel_reporting;
  bool size_reporting;
  bool filter_grip;
  bool filter_palm;
  int num_sensitivity_settings;
};
struct TouchOffloadConfig {
  bool continuous_reporting;
  bool noise_reporting;
  bool cancel_reporting;
  bool filter_grip;
  bool filter_palm;
  int sensitivity_setting;
  bool read_coords;
  int mutual_data_types;
  int self_data_types;
};
struct TouchOffloadFrameHeader {
  __u32 frame_size;
  __u64 index;
  __u64 timestamp;
} __attribute__((packed));
enum CoordStatus {
  COORD_STATUS_INACTIVE = 0x00,
  COORD_STATUS_FINGER = 0x01,
  COORD_STATUS_EDGE = 0x02,
  COORD_STATUS_PALM = 0x03,
  COORD_STATUS_CANCEL = 0x04
};
#define MAX_COORDS 10
struct TouchOffloadCoord {
  __u16 x;
  __u16 y;
  enum CoordStatus status;
  __u8 filler[32];
} __attribute__((packed));
struct TouchOffloadDataCoord {
  __u32 size_bytes;
  struct TouchOffloadCoord coords[MAX_COORDS];
} __attribute__((packed));
#define TOUCH_OFFLOAD_FRAME_SIZE_COORD (sizeof(struct TouchOffloadDataCoord))
struct TouchOffloadData2d {
  __u32 size_bytes;
  __u16 tx_size;
  __u16 rx_size;
  __u8 data[1];
} __attribute__((packed));
#define TOUCH_OFFLOAD_DATA_SIZE_2D(rx,tx) (sizeof(__u16) * (rx) * (tx))
#define TOUCH_OFFLOAD_FRAME_SIZE_2D(rx,tx) (sizeof(struct TouchOffloadData2d) - 1 + TOUCH_OFFLOAD_DATA_SIZE_2D((rx), (tx)))
struct TouchOffloadData1d {
  __u32 size_bytes;
  __u16 tx_size;
  __u16 rx_size;
  __u8 data[1];
} __attribute__((packed));
#define TOUCH_OFFLOAD_DATA_SIZE_1D(rx,tx) (sizeof(__u16) * ((rx) + (tx)))
#define TOUCH_OFFLOAD_FRAME_SIZE_1D(rx,tx) (sizeof(struct TouchOffloadData1d) - 1 + TOUCH_OFFLOAD_DATA_SIZE_1D((rx), (tx)))
struct TouchOffloadIocGetCaps {
  struct TouchOffloadCaps caps;
};
struct TouchOffloadIocConfigure {
  struct TouchOffloadConfig config;
};
struct TouchOffloadIocReport {
  __u8 numCoords;
  struct TouchOffloadCoord coords[MAX_COORDS];
};
#define TOUCH_OFFLOAD_IOC_RD_GETCAPS _IOR(TOUCH_OFFLOAD_MAGIC, 0, struct TouchOffloadIocGetCaps)
#define TOUCH_OFFLOAD_IOC_WR_CONFIGURE _IOW(TOUCH_OFFLOAD_MAGIC, 1, struct TouchOffloadIocConfigure)
#define TOUCH_OFFLOAD_IOC_START _IOC(TOUCH_OFFLOAD_MAGIC, 2)
#define TOUCH_OFFLOAD_IOC_WR_REPORT _IOW(TOUCH_OFFLOAD_MAGIC, 3, struct TouchOffloadIocReport)
#define TOUCH_OFFLOAD_IOC_STOP _IOC(TOUCH_OFFLOAD_MAGIC, 4)
#endif
