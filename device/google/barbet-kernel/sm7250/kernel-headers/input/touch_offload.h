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
#define HEATMAP_SIZE_PARTIAL 0
#define HEATMAP_SIZE_FULL 1
#define TOUCH_DATA_TYPE_COORD 0x01
#define TOUCH_DATA_TYPE_RAW 0x02
#define TOUCH_DATA_TYPE_FILTERED 0x04
#define TOUCH_DATA_TYPE_BASELINE 0x08
#define TOUCH_DATA_TYPE_STRENGTH 0x10
#define TOUCH_SCAN_TYPE_MUTUAL 0x40
#define TOUCH_SCAN_TYPE_SELF 0x80
struct TouchOffloadCaps {
  __u32 touch_offload_major_version;
  __u32 touch_offload_minor_version;
  __u8 reserved1[8];
  __u32 device_id;
  __u16 display_width;
  __u16 display_height;
  __u16 tx_size;
  __u16 rx_size;
  __u8 bus_type;
  __u32 bus_speed_hz;
  __u8 reserved2[16];
  __u8 heatmap_size;
  __u16 touch_data_types;
  __u16 touch_scan_types;
  __u8 reserved3[16];
  __u8 continuous_reporting;
  __u8 noise_reporting;
  __u8 cancel_reporting;
  __u8 size_reporting;
  __u8 filter_grip;
  __u8 filter_palm;
  __u8 num_sensitivity_settings;
  __u8 reserved4[32];
};
struct TouchOffloadConfig {
  __u8 continuous_reporting;
  __u8 noise_reporting;
  __u8 cancel_reporting;
  __u8 filter_grip;
  __u8 filter_palm;
  __u8 sensitivity_setting;
  __u8 reserved1[16];
  __u8 read_coords;
  __u16 mutual_data_types;
  __u16 self_data_types;
  __u8 reserved2[16];
};
struct TouchOffloadFrameHeader {
  __u32 frame_size;
  __u64 index;
  __u64 timestamp;
  __u8 num_channels;
} __attribute__((packed));
struct TouchOffloadChannelHeader {
  __u8 channel_type;
  __u32 channel_size;
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
  __u32 major;
  __u32 minor;
  __u32 pressure;
  __u8 reserved1[16];
} __attribute__((packed));
struct TouchOffloadDataCoord {
  struct TouchOffloadChannelHeader header;
  struct TouchOffloadCoord coords[MAX_COORDS];
  __u8 reserved1[16];
} __attribute__((packed));
#define TOUCH_OFFLOAD_FRAME_SIZE_COORD (sizeof(struct TouchOffloadDataCoord))
struct TouchOffloadData2d {
  struct TouchOffloadChannelHeader header;
  __u16 tx_size;
  __u16 rx_size;
  __u8 reserved1[16];
  __u8 data[1];
} __attribute__((packed));
#define TOUCH_OFFLOAD_DATA_SIZE_2D(rx,tx) (sizeof(__u16) * (rx) * (tx))
#define TOUCH_OFFLOAD_FRAME_SIZE_2D(rx,tx) (sizeof(struct TouchOffloadData2d) - 1 + TOUCH_OFFLOAD_DATA_SIZE_2D((rx), (tx)))
struct TouchOffloadData1d {
  struct TouchOffloadChannelHeader header;
  __u16 tx_size;
  __u16 rx_size;
  __u8 reserved1[16];
  __u8 data[1];
} __attribute__((packed));
#define TOUCH_OFFLOAD_DATA_SIZE_1D(rx,tx) (sizeof(__u16) * ((rx) + (tx)))
#define TOUCH_OFFLOAD_FRAME_SIZE_1D(rx,tx) (sizeof(struct TouchOffloadData1d) - 1 + TOUCH_OFFLOAD_DATA_SIZE_1D((rx), (tx)))
struct TouchOffloadIocGetCaps {
  struct TouchOffloadCaps caps;
  __u8 reserved1[16];
};
struct TouchOffloadIocConfigure {
  struct TouchOffloadConfig config;
  __u8 reserved1[16];
};
struct TouchOffloadIocReport {
  __u64 index;
  __u64 timestamp;
  __u8 num_coords;
  __u8 reserved1[16];
  struct TouchOffloadCoord coords[MAX_COORDS];
};
#define TOUCH_OFFLOAD_IOC_RD_GETCAPS _IOR(TOUCH_OFFLOAD_MAGIC, 0, struct TouchOffloadIocGetCaps)
#define TOUCH_OFFLOAD_IOC_WR_CONFIGURE _IOW(TOUCH_OFFLOAD_MAGIC, 1, struct TouchOffloadIocConfigure)
#define TOUCH_OFFLOAD_IOC_START _IOC(TOUCH_OFFLOAD_MAGIC, 2)
#define TOUCH_OFFLOAD_IOC_WR_REPORT _IOW(TOUCH_OFFLOAD_MAGIC, 3, struct TouchOffloadIocReport)
#define TOUCH_OFFLOAD_IOC_STOP _IOC(TOUCH_OFFLOAD_MAGIC, 4)
#endif
