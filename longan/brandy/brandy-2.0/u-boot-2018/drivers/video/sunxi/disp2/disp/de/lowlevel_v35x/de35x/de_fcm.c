/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/*******************************************************************************
 *  All Winner Tech, All Right Reserved. 2014-2021 Copyright (c)
 *
 *  File name   :  display engine 35x fcm basic function definition
 *
 *  History     :  2021/11/30 v0.1  Initial version
 *
 ******************************************************************************/

#include "de_enhance.h"
#include "de_fcm_type.h"
#include "de_rtmx.h"

#define FCM_PARA_NUM (12)
#define FCM_SUM_NUM (28)
enum { FCM_PARA_REG_BLK = 0,
       FCM_ANGLE_REG_BLK,
       FCM_HUE_REG_BLK,
       FCM_SAT_REG_BLK,
       FCM_LUM_REG_BLK,
       FCM_CSC_REG_BLK,
       FCM_REG_BLK_NUM,
};

struct de_fcm_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[FCM_REG_BLK_NUM];
	void (*set_blk_dirty)(struct de_fcm_private *priv, u32 blk_id,
			      u32 dirty);
};

static struct de_fcm_private fcm_priv[DE_NUM][VI_CHN_NUM];

static inline struct fcm_reg *get_fcm_reg(struct de_fcm_private *priv)
{
	return (struct fcm_reg *)(priv->reg_blks[FCM_PARA_REG_BLK].vir_addr);
}

static void fcm_set_block_dirty(struct de_fcm_private *priv, u32 blk_id,
				u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void fcm_set_rcq_head_dirty(struct de_fcm_private *priv, u32 blk_id,
				   u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WRN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_fcm_init(u32 disp, u32 chn, uintptr_t reg_base, u8 __iomem **phy_addr,
		u8 **vir_addr, u32 *size)
{
	struct de_fcm_private *priv = &fcm_priv[disp][chn];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *reg_blk;
	uintptr_t base;
	u32 phy_chn;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	phy_chn = de_feat_get_phy_chn_id(disp, chn);
	base = reg_base + DE_CHN_OFFSET(phy_chn) + CHN_FCM_OFFSET;

	reg_mem_info->phy_addr = *phy_addr;
	reg_mem_info->vir_addr = *vir_addr;
	reg_mem_info->size = DE_FCM_REG_MEM_SIZE;

	priv->reg_blk_num = FCM_REG_BLK_NUM;

	reg_blk = &(priv->reg_blks[FCM_PARA_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = 0x20;
	reg_blk->reg_addr = (u8 __iomem *)base;

	reg_blk = &(priv->reg_blks[FCM_ANGLE_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x20;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x20;
	reg_blk->size = 0x100;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x20);

	reg_blk = &(priv->reg_blks[FCM_HUE_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x120;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x120;
	reg_blk->size = 0x1e0;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x120);

	reg_blk = &(priv->reg_blks[FCM_SAT_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x300;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x300;
	reg_blk->size = 0x1200;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x300);

	reg_blk = &(priv->reg_blks[FCM_LUM_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x1500;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x1500;
	reg_blk->size = 0x1b00;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x1500);

	reg_blk = &(priv->reg_blks[FCM_CSC_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x3000;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x3000;
	reg_blk->size = 0x80;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x3000);

	*phy_addr += DE_FCM_REG_MEM_SIZE;
	*vir_addr += DE_FCM_REG_MEM_SIZE;
	*size -= DE_FCM_REG_MEM_SIZE;

	if (rcq_used)
		priv->set_blk_dirty = fcm_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = fcm_set_block_dirty;

	return 0;
}

s32 de_fcm_exit(u32 disp, u32 chn) { return 0; }

s32 de_fcm_get_reg_blocks(u32 disp, u32 chn, struct de_reg_block **blks,
			  u32 *blk_num)
{
	struct de_fcm_private *priv = &(fcm_priv[disp][chn]);
	u32 i, num;

	if (blks == NULL) {
		*blk_num = priv->reg_blk_num;
		return 0;
	}

	if (*blk_num >= priv->reg_blk_num) {
		num = priv->reg_blk_num;
	} else {
		num = *blk_num;
		DE_WRN("should not happen\n");
	}
	for (i = 0; i < num; ++i)
		blks[i] = priv->reg_blks + i;

	*blk_num = i;
	return 0;
}

s32 de_fcm_set_size(u32 disp, u32 chn, u32 width, u32 height)
{
	struct de_fcm_private *priv = &(fcm_priv[disp][chn]);
	struct fcm_reg *reg = get_fcm_reg(priv);

	reg->size.bits.width = width - 1;
	reg->size.bits.height = height - 1;
	priv->set_blk_dirty(priv, FCM_PARA_REG_BLK, 1);

	return 0;
}

s32 de_fcm_set_window(u32 disp, u32 chn, u32 win_enable,
		      struct de_rect_o window)
{
	struct de_fcm_private *priv = &(fcm_priv[disp][chn]);
	struct fcm_reg *reg = get_fcm_reg(priv);

	reg->ctl.bits.window_en = win_enable;
	if (win_enable) {
		reg->win0.bits.win_left = window.x;
		reg->win1.bits.win_right = window.x + window.w - 1;
		reg->win0.bits.win_top = window.y;
		reg->win1.bits.win_bot = window.y + window.h - 1;
	}
	priv->set_blk_dirty(priv, FCM_PARA_REG_BLK, 1);

	return 0;
}

/*static s32 de_fcm_apply(u32 disp, u32 chn, u32 fcm_en)
{
	struct de_fcm_private *priv = &(fcm_priv[disp][chn]);
	struct fcm_reg *reg = get_fcm_reg(priv);

    reg->ctl.bits.fcm_en = fcm_en;
    priv->set_blk_dirty(priv, FCM_PARA_REG_BLK, 1);

	if (fcm_en == 1) {
	} else {
	}

	return 0;
}*/

s32 de_fcm_info2para(u32 disp, u32 chn, u32 fmt, u32 dev_type,
		     struct fcm_config_data *para, u32 bypass)
{
	struct de_fcm_private *priv = &(fcm_priv[disp][chn]);
	struct fcm_reg *reg = get_fcm_reg(priv);
	s32 sgain;
	u32 en;
	int i = 0;
	static u8 FCM_SAT_GAIN[FCM_PARA_NUM][2] = {
	    /* lcd/hdmi */
	    {-4, -4}, /* 00 gain for yuv */
	    {-3, -4}, /* 01              */
	    {-2, -4}, /* 02              */
	    {-1, -4}, /* 03              */
	    {0, 0},   /* 04              */
	    {1, 1},   /* 05              */
	    {2, 2},   /* 06              */
	    {3, 3},   /* 07              */
	    {4, 4},   /* 08              */
	    {5, 5},   /* 09              */
	    {6, 6},   /* 10 gain for yuv */
	    {0, 0},   /* 11 gain for rgb */
	};

	static u8 FCM_SAT_SUM[FCM_SUM_NUM] = {
	    /*   0,  1,  2,  3   4   5   6   7   8   9 */
	    20, 20, 20, 20, 22, 23, 24, 24, 26, 26, 26, 26, 26, 28,
	    28, 28, 28, 26, 26, 26, 26, 24, 24, 24, 24, 22, 22, 20,
	};
	if (fmt == 1)
		sgain = FCM_SAT_GAIN[FCM_PARA_NUM - 1][dev_type];
	else
		sgain = FCM_SAT_GAIN[para->level][dev_type];

	en = (((fmt == 0) && (sgain == 0)) || (bypass != 0)) ? 0 : 1;

	reg->ctl.bits.fcm_en = en;
	if (!en) {
		priv->set_blk_dirty(priv, FCM_PARA_REG_BLK, 1);
		return 0;
	}
	for (i = 0; i < FCM_SUM_NUM; i++) {
		reg->sbh_hue_lut[i].bits.sbh_hue_lut_high =
		    reg->sbh_hue_lut[i].bits.sbh_hue_lut_high +
		    sgain * FCM_SAT_SUM[i];
		reg->sbh_hue_lut[i].bits.sbh_hue_lut_low =
		    reg->sbh_hue_lut[i].bits.sbh_hue_lut_low +
		    sgain * FCM_SAT_SUM[i];
	}
	priv->set_blk_dirty(priv, FCM_PARA_REG_BLK, 1);
	return 0;
}

s32 de_fcm_enable(u32 disp, u32 chn, u32 en)
{
	struct de_fcm_private *priv = &(fcm_priv[disp][chn]);
	struct fcm_reg *reg = get_fcm_reg(priv);

	reg->ctl.bits.fcm_en = en;
	priv->set_blk_dirty(priv, FCM_PARA_REG_BLK, 1);

	return 0;
}

s32 de_fcm_init_para(u32 disp, u32 chn)
{
	struct de_fcm_private *priv = &(fcm_priv[disp][chn]);
	struct fcm_reg *reg = get_fcm_reg(priv);

	reg->ctl.dwval = 0x80000001;
	priv->set_blk_dirty(priv, FCM_PARA_REG_BLK, 1);

	return 0;
}

