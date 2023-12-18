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
#ifndef __IAXXX_SENSOR_TUNNEL_H__
#define __IAXXX_SENSOR_TUNNEL_H__
#define IAXXX_SENSOR_TUNNEL_IOCTL_MAGIC 'K'
#define FLICKER_TUNNEL_SETUP _IOWR(IAXXX_SENSOR_TUNNEL_IOCTL_MAGIC, 0x022, struct tunlMsg)
#define FLICKER_TUNNEL_TERMINATE _IOWR(IAXXX_SENSOR_TUNNEL_IOCTL_MAGIC, 0x023, struct tunlMsg)
#define FLICKER_ROUTE_SETUP _IO(IAXXX_SENSOR_TUNNEL_IOCTL_MAGIC, 0x24)
#define FLICKER_ROUTE_TERMINATE _IO(IAXXX_SENSOR_TUNNEL_IOCTL_MAGIC, 25)
#endif
