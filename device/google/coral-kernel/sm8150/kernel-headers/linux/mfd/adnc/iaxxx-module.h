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
#ifndef __IAXXX_MODULE_H__
#define __IAXXX_MODULE_H__
#define MAX_FILE_CHAR_SIZE 256
enum {
  MPLL_CLK_3000,
  MPLL_CLK_5000,
  MPLL_CLK_6000,
  MPLL_CLK_8000,
  MPLL_CLK_10000,
  MPLL_CLK_15000,
  MPLL_CLK_30000,
  MPLL_CLK_35000,
  MPLL_CLK_40000,
  MPLL_CLK_45000,
  MPLL_CLK_50000,
  MPLL_CLK_55000,
  MPLL_CLK_60000,
  MPLL_CLK_80000,
  MPLL_CLK_120000,
#ifdef EXTENDED_FREQ_SUPPORT
  MPLL_CLK_150000,
  MPLL_CLK_180000,
  MPLL_CLK_200000,
#endif
  NUM_MPLL_CLK_FREQ,
};
enum {
  APLL_CLK_3072,
  APLL_CLK_6144,
  APLL_CLK_12288,
  APLL_CLK_24576,
  APLL_CLK_49152,
  APLL_CLK_98304,
  APLL_CLK_368640,
  NUM_APLL_CLK_FREQ,
};
struct iaxxx_pwr_stats {
  uint64_t mpll_cumulative_cnts[NUM_MPLL_CLK_FREQ];
  uint64_t apll_cumulative_cnts[NUM_APLL_CLK_FREQ];
  uint64_t mpllCumulativeDur[NUM_MPLL_CLK_FREQ];
  uint64_t mpllTimeStamp[NUM_MPLL_CLK_FREQ];
  uint64_t sleepModeTimeStamp;
  uint64_t sleepModeCumulativeDur;
};
enum {
  SENSOR_MODE_OFF,
  SENSOR_MODE_ENTRANCE,
  SENSOR_MODE_INTERACTIVE,
  SENSOR_NUM_MODE,
};
struct iaxxx_sensor_mode_stats {
  uint64_t totalNumEntries;
  uint64_t totalTimeSpentMs;
  uint64_t lastEntryTimeStampMs;
  uint64_t lastExitTimeStampMs;
};
struct iaxxx_osc_trim_period {
  int period;
};
struct iaxxx_sensor_info {
  uint32_t block_id;
  uint32_t inst_id;
};
struct iaxxx_sensor_param {
  uint32_t inst_id;
  uint32_t param_id;
  uint32_t param_val;
  uint8_t block_id;
};
struct iaxxx_sensor_param_blk {
  uint32_t inst_id;
  uint32_t blk_size;
  uint64_t blk_data;
  uint32_t param_blk_id;
  uint8_t block_id;
};
struct iaxxx_script_info {
  char script_name[MAX_FILE_CHAR_SIZE];
  uint32_t script_id;
};
#define IAXXX_IOCTL_MAGIC 'I'
#define MODULE_SENSOR_ENABLE _IO(IAXXX_IOCTL_MAGIC, 0x51)
#define MODULE_SENSOR_DISABLE _IO(IAXXX_IOCTL_MAGIC, 0x52)
#define MODULE_SENSOR_SET_PARAM _IO(IAXXX_IOCTL_MAGIC, 0x53)
#define MODULE_SENSOR_GET_PARAM _IO(IAXXX_IOCTL_MAGIC, 0x54)
#define MODULE_SENSOR_WRITE_PARAM_BLK _IO(IAXXX_IOCTL_MAGIC, 0x55)
#define SCRIPT_LOAD _IO(IAXXX_IOCTL_MAGIC, 0x61)
#define SCRIPT_UNLOAD _IO(IAXXX_IOCTL_MAGIC, 0x62)
#define SCRIPT_TRIGGER _IO(IAXXX_IOCTL_MAGIC, 0x63)
#define IAXXX_POWER_STATS_COUNT _IO(IAXXX_IOCTL_MAGIC, 0x71)
#define IAXXX_SET_OSC_TRIM_PERIOD _IO(IAXXX_IOCTL_MAGIC, 0x72)
#define IAXXX_SENSOR_MODE_STATS _IO(IAXXX_IOCTL_MAGIC, 0x73)
#endif
