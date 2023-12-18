/*
 * (C) Copyright 2008 Semihalf
 *
 * (C) Copyright 2000-2005
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 ********************************************************************
 * NOTE: This header file defines an interface to U-Boot. Including
 * this (unmodified) header file in another file is considered normal
 * use of U-Boot, and does *not* fall under the heading of "derived
 * work".
 ********************************************************************
 */

#ifndef __IMAGE_H__
#define __IMAGE_H__

#define ALIGN(x, a)	(((x) + ((a) - 1)) & ~((a) - 1))
#define DIV_ROUND_UP(n,d)   (((n) + (d) - 1) / (d))

struct andr_img_hdr;
int android_image_check_header(const struct andr_img_hdr *hdr);
int android_image_get_kernel(const struct andr_img_hdr *hdr, int verify,
			     unsigned long *os_data, unsigned long *os_len);
int android_image_get_ramdisk(const struct andr_img_hdr *hdr,
			      unsigned long *rd_data, unsigned long *rd_len);
unsigned long android_image_get_end(const struct andr_img_hdr *hdr);
unsigned long android_image_get_kload(const struct andr_img_hdr *hdr);

#endif	/* __IMAGE_H__ */
