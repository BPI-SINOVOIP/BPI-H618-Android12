/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2021 Copyright (c)
 *
 *  File name   :   de_enhance.h
 *
 *  Description :   display engine 35x enhance basic function declaration
 *
 *  History     :   2021/12/20 v0.1  Initial version
 ******************************************************************************/

#ifndef _DE_ENHANCE_H_
#define _DE_ENHANCE_H_

#include "../../include.h"
#include "de_dci_type.h"
#include "de_sharp_type.h"
#include "de_deband_type.h"
#include "de_fcm_type.h"
#include "de_gamma_type.h"
#include "de_dither_type.h"
#include "de_csc_table.h"
#include "de_vsu.h"

/* 8K for enahnce reg mem */
#define DE_FCM_REG_MEM_SIZE (0x4000)
#define DE_DCI_REG_MEM_SIZE (0x1000)
#define DE_SHARP_REG_MEM_SIZE (0x400)
#define DE_DEBAND_REG_MEM_SIZE (0x1000)
#define DE_DITHER_REG_MEM_SIZE (0x1000)
#define DE_GAMMA_REG_MEM_SIZE (0x2000)
#define DE_ENHANCE_REG_MEM_SIZE \
	(DE_FCM_REG_MEM_SIZE + DE_DCI_REG_MEM_SIZE \
	 + DE_SHARP_REG_MEM_SIZE + DE_DEBAND_REG_MEM_SIZE \
	 + DE_DITHER_REG_MEM_SIZE + DE_GAMMA_REG_MEM_SIZE)

#define MODE_NUM 4
#define PARA_NUM 6
#define FORMAT_NUM 2
#define ENHANCE_MODE_NUM 3

#define ENHANCE_MIN_OVL_WIDTH  32
#define ENHANCE_MIN_OVL_HEIGHT 32
#define ENHANCE_MIN_BLD_WIDTH 32
#define ENHANCE_MIN_BLD_HEIGHT 4

#define SHARP_OFST  0x6000	/* sharp offset based on channel */
#define DCI_OFST  0x10000	/* LTI offset based on channel */
#define FCM_OFST  0x11000	/* BLS offset based on channel */
#define DEBAND_OFST  0x7000	/* deband offset based on RTMX */
#define DITHER_OFST  0x8000	/* dither offset based on RTMX */
#define GAMMA_OFST  0x9000	/* gamma offset based on RTMX */

/* vep bypass mask */
#define USER_BYPASS 0x00000001
#define USER_BYPASS_MASK 0xfffffffe
#define SIZE_BYPASS 0x00000002
#define SIZE_BYPASS_MASK 0xfffffffd
#define LAYER_BYPASS 0x00000004
#define LAYER_BYPASS_MASK 0xfffffffb

#define DE_CLIP(x, low, high) max(low, min(x, high))

struct disp_enhance_layer_info {
	/* layer framebuffer size width/height */
	struct disp_rectsz fb_size;
	/* layer framebuffer crop retangle, don't care w/h at all */
	struct disp_rect fb_crop;
	/* layer enable */
	u32 en;
	/* layer format */
	u32 format;
};

struct disp_enhance_chn_info {
	/* overlay size (scale in size) */
	/* CAUTION: overlay size can be the output size of */
	/* video overlay/ de-interlace overlay/ FBD overlay */
	struct disp_rectsz ovl_size;
	/* bld size (scale out size) */
	struct disp_rectsz bld_size;
	/* layer info */
	struct disp_enhance_layer_info layer_info[MAX_LAYER_NUM_PER_CHN];
};

/* parameters of vep module */
struct vep_para {
	disp_manager_dirty_flags flags;
	struct de_asu_peaking_config peaking_para;
	struct dci_config dci_para;
	struct fcm_config_data fcm_para;
	struct sharp_config sharp_para;
	struct gamma_data gamma_para;
	u32 demo_enable;
	u32 bypass;
	u32 dev_type;
	u32 fmt;
};

/* system configuration of vep */
struct vep_config_info {
	u32 device_num;
	u32 vep_num[DE_NUM];
	u32 dci_exist[DE_NUM][VI_CHN_NUM];
	u32 fcm_exist[DE_NUM][VI_CHN_NUM];
	u32 peaking_exist[DE_NUM][VI_CHN_NUM];
	u32 sharp_exist[DE_NUM][VI_CHN_NUM];
};

/* algorithm variable */
struct vep_alg_var {
	u32 frame_cnt;
};

/* asu peak function declaration */
s32 de_asu_peak_enable(u32 disp, u32 chn, u32 en);
s32 de_asu_peak_set_size(u32 disp, u32 chn, u32 width,
	u32 height);
s32 de_asu_peak_set_window(u32 disp, u32 chn,
	u32 win_enable, struct de_rect_o window);
s32 de_asu_peak_init_para(u32 disp, u32 chn);
s32 de_asu_peak_info2para(u32 disp, u32 chn,
	u32 fmt, u32 dev_type,
	struct de_asu_peaking_config *para, u32 bypass);

/* sharp function declaration */
s32 de_sharp_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size);
s32 de_sharp_exit(u32 disp, u32 chn);
s32 de_sharp_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num);
s32 de_sharp_enable(u32 disp, u32 chn, u32 en);
s32 de_sharp_set_size(u32 disp, u32 chn, u32 width,
	u32 height);
s32 de_sharp_set_window(u32 disp, u32 chn,
	u32 win_enable, struct de_rect_o window);
s32 de_sharp_init_para(u32 disp, u32 chn);
s32 de_sharp_info2para(u32 disp, u32 chn,
	u32 fmt, u32 dev_type,
	struct sharp_config *para, u32 bypass);

/* dci function declaration */
s32 de_dci_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size);
s32 de_dci_exit(u32 disp, u32 chn);
s32 de_dci_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num);
s32 de_dci_enable(u32 disp, u32 chn, u32 en);
s32 de_dci_set_size(u32 disp, u32 chn, u32 width,
	u32 height);
s32 de_dci_set_window(u32 disp, u32 chn,
	u32 win_enable, struct de_rect_o window);
s32 de_dci_init_para(u32 disp, u32 chn);
s32 de_dci_info2para(u32 disp, u32 chn,
	u32 fmt, u32 dev_type,
	struct dci_config *para, u32 bypass);
s32 de_dci_tasklet(u32 disp, u32 chn,
	u32 frame_cnt);
s32 de_dci_set_icsc_coeff(u32 disp, u32 chn,
	struct de_csc_info *in_info, struct de_csc_info *out_info);

/* fcm function declaration */
s32 de_fcm_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size);
s32 de_fcm_enable(u32 disp, u32 chn, u32 en);
s32 de_fcm_exit(u32 disp, u32 chn);
s32 de_fcm_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num);
s32 de_fcm_set_size(u32 disp, u32 chn, u32 width,
	u32 height);
s32 de_fcm_set_window(u32 disp, u32 chn,
	u32 win_enable, struct de_rect_o window);
s32 de_fcm_init_para(u32 disp, u32 chn);
s32 de_fcm_info2para(u32 disp, u32 chn,
	u32 fmt, u32 dev_type,
	struct fcm_config_data *para, u32 bypass);

/* gamma function declaration */
s32 de_gamma_init(u32 disp, uintptr_t reg_base);
s32 de_gamma_exit(u32 disp);
s32 de_gamma_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num);
s32 de_gamma_enable(u32 disp, u32 en);
s32 de_gamma_set_size(u32 disp, u32 width, u32 height);
s32 de_gamma_set_window(u32 disp, u32 win_en,
	struct de_rect_o window);
s32 de_gamma_init_para(u32 disp);
s32 de_gamma_info2para(u32 disp, u32 fmt,
	u32 dev_type, struct gamma_data *para,
	u32 bypass);

/* dither function declaration */
s32 de_dither_init(u32 disp, uintptr_t reg_base);
s32 de_dither_exit(u32 disp);
s32 de_dither_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num);
s32 de_dither_enable(u32 disp, u32 en);
s32 de_dither_set_size(u32 disp, u32 width, u32 height);
s32 de_dither_set_window(u32 disp, u32 win_en,
	struct de_rect_o window);
s32 de_dither_init_para(u32 disp);
s32 de_dither_info2para(u32 disp, u32 fmt,
	u32 dev_type, struct dither_config *para,
	u32 bypass);

/* deband function declaration */
s32 de_deband_init(u32 disp, uintptr_t reg_base);
s32 de_deband_exit(u32 disp);
s32 de_deband_get_reg_blocks(u32 disp,
	struct de_reg_block **blks, u32 *blk_num);
s32 de_deband_enable(u32 disp, u32 en);
s32 de_deband_set_size(u32 disp, u32 width, u32 height);
s32 de_deband_set_window(u32 disp, u32 win_en,
	struct de_rect_o window);
s32 de_deband_init_para(u32 disp);
s32 de_deband_info2para(u32 disp, u32 fmt,
	u32 dev_type, u32 bypass);
s32 de_deband_set_outinfo(int disp, enum de_color_space cs,
			  enum de_data_bits bits,
			  enum de_format_space fmt);

s32 de_vsu_init_peaking_para(u32 disp, u32 chn);
s32 de_vsu_set_peaking_para(u32 disp, u32 chn,
	u32 fmt, u32 dev_type,
	struct de_asu_peaking_config *para, u32 bypass);
s32 de_vsu_set_peaking_window(u32 disp, u32 chn,
		      u32 win_enable, struct de_rect_o window);

/* ENHANCE function declaration */
s32 de_enhance_update_regs(u32 disp);
s32 de_enhance_init(disp_bsp_init_para *para);
s32 de_enhance_exit(void);
s32 de_enhance_get_reg_blocks(u32 disp,
	struct de_reg_block **blk, u32 *blk_num);
s32 de_enhance_layer_apply(u32 disp,
	struct disp_enhance_chn_info *info);
s32 de_enhance_apply(u32 disp,
	struct disp_enhance_config *config);
s32 de_enhance_sync(u32 disp);
s32 de_enhance_tasklet(u32 disp);

/* module public function declaration */
void de_set_bits(u32 *reg_addr, u32 bits_val,
	u32 shift, u32 width);

#endif /* #ifndef _DE_ENHANCE_H_ */
