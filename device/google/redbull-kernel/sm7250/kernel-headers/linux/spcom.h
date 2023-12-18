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
#ifndef _SPCOM_H_
#define _SPCOM_H_
#include <linux/types.h>
#ifndef BIT
#define BIT(x) (1 << x)
#endif
#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
#define SPCOM_MAX_CHANNELS 0x20
#define SPCOM_CHANNEL_NAME_SIZE 32
#define SPCOM_GET_NEXT_REQUEST_SIZE (PAGE_SIZE - 1)
enum spcom_cmd_id {
  SPCOM_CMD_LOAD_APP = 0x4C4F4144,
  SPCOM_CMD_RESET_SP = 0x52455354,
  SPCOM_CMD_SEND = 0x53454E44,
  SPCOM_CMD_SEND_MODIFIED = 0x534E444D,
  SPCOM_CMD_LOCK_ION_BUF = 0x4C4F434B,
  SPCOM_CMD_UNLOCK_ION_BUF = 0x554C434B,
  SPCOM_CMD_FSSR = 0x46535352,
  SPCOM_CMD_CREATE_CHANNEL = 0x43524554,
#define SPCOM_CMD_ENABLE_SSR SPCOM_CMD_ENABLE_SSR
  SPCOM_CMD_ENABLE_SSR = 0x45535352,
#define SPCOM_CMD_RESTART_SP SPCOM_CMD_RESTART_SP
  SPCOM_CMD_RESTART_SP = 0x52535452,
};
enum spcom_poll_events {
  SPCOM_POLL_LINK_STATE = BIT(1),
  SPCOM_POLL_CH_CONNECT = BIT(2),
  SPCOM_POLL_READY_FLAG = BIT(14),
  SPCOM_POLL_WAIT_FLAG = BIT(15),
};
struct spcom_user_command {
  enum spcom_cmd_id cmd_id;
  uint32_t arg;
} __attribute__((packed));
struct spcom_send_command {
  enum spcom_cmd_id cmd_id;
  uint32_t timeout_msec;
  uint32_t buf_size;
  char buf[0];
} __attribute__((packed));
struct spcom_user_create_channel_command {
  enum spcom_cmd_id cmd_id;
  char ch_name[SPCOM_CHANNEL_NAME_SIZE];
#define SPCOM_IS_SHARABLE_SUPPORTED
  bool is_sharable;
} __attribute__((packed));
#define SPCOM_USER_RESTART_SP_CMD
struct spcom_user_restart_sp_command {
  enum spcom_cmd_id cmd_id;
  uint32_t arg;
#define SPCOM_IS_UPDATED_SUPPORETED
  uint32_t is_updated;
} __attribute__((packed));
#define SPCOM_MAX_ION_BUF 4
struct spcom_ion_info {
  int32_t fd;
  uint32_t buf_offset;
};
#define SPCOM_ION_FD_UNLOCK_ALL 0xFFFF
struct spcom_ion_handle {
  int32_t fd;
};
struct spcom_user_send_modified_command {
  enum spcom_cmd_id cmd_id;
  struct spcom_ion_info ion_info[SPCOM_MAX_ION_BUF];
  uint32_t timeout_msec;
  uint32_t buf_size;
  char buf[0];
} __attribute__((packed));
enum {
  SPCOM_IONFD_CMD,
  SPCOM_POLL_CMD,
};
enum spcom_poll_cmd_id {
  SPCOM_LINK_STATE_REQ,
  SPCOM_CH_CONN_STATE_REQ,
};
struct spcom_poll_param {
  bool wait;
  enum spcom_poll_cmd_id cmd_id;
  int retval;
} __attribute__((packed));
#define SPCOM_IOCTL_MAGIC 'S'
#define SPCOM_GET_IONFD _IOR(SPCOM_IOCTL_MAGIC, SPCOM_IONFD_CMD, struct spcom_ion_handle)
#define SPCOM_SET_IONFD _IOW(SPCOM_IOCTL_MAGIC, SPCOM_IONFD_CMD, struct spcom_ion_handle)
#define SPCOM_POLL_STATE _IOWR(SPCOM_IOCTL_MAGIC, SPCOM_POLL_CMD, struct spcom_poll_param)
#endif
