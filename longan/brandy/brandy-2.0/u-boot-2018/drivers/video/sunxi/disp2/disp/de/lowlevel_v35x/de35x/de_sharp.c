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
 *  File name   :  display engine 35x sharp basic function definition
 *
 *  History     :  2021/11/30 v0.1  Initial version
 *
 ******************************************************************************/

#include "de_sharp_type.h"
#include "de_rtmx.h"
#include "de_enhance.h"

enum {
	SHARP_PARA_REG_BLK = 0,
	SHARP_REG_BLK_NUM,
};

struct de_sharp_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct de_reg_block reg_blks[SHARP_REG_BLK_NUM];
	void (*set_blk_dirty)(struct de_sharp_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_sharp_private sharp_priv[DE_NUM][VI_CHN_NUM];


static inline struct sharp_reg *get_sharp_reg(struct de_sharp_private *priv)
{
	return (struct sharp_reg *)(priv->reg_blks[SHARP_PARA_REG_BLK].vir_addr);
}

static void sharp_set_block_dirty(
	struct de_sharp_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void sharp_set_rcq_head_dirty(
	struct de_sharp_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WRN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

s32 de_sharp_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size)
{
	struct de_sharp_private *priv = &sharp_priv[disp][chn];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *reg_blk;
	uintptr_t base;
	u32 phy_chn;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	phy_chn = de_feat_get_phy_chn_id(disp, chn);
	base = reg_base + DE_CHN_OFFSET(phy_chn) + CHN_SHARP_OFFSET;

	reg_mem_info->phy_addr = *phy_addr;
	reg_mem_info->vir_addr = *vir_addr;
	reg_mem_info->size = DE_SHARP_REG_MEM_SIZE;

	priv->reg_blk_num = SHARP_REG_BLK_NUM;

	reg_blk = &(priv->reg_blks[SHARP_PARA_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = DE_SHARP_REG_MEM_SIZE;
	reg_blk->reg_addr = (u8 __iomem *)base;

	*phy_addr += DE_SHARP_REG_MEM_SIZE;
	*vir_addr += DE_SHARP_REG_MEM_SIZE;
	*size -= DE_SHARP_REG_MEM_SIZE;

	if (rcq_used)
		priv->set_blk_dirty = sharp_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = sharp_set_block_dirty;

	return 0;
}

s32 de_sharp_exit(u32 disp, u32 chn)
{
	return 0;
}

s32 de_sharp_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num)
{
	struct de_sharp_private *priv = &(sharp_priv[disp][chn]);
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

s32 de_sharp_set_size(u32 disp, u32 chn, u32 width,
		    u32 height)
{
	struct de_sharp_private *priv = &(sharp_priv[disp][chn]);
	struct sharp_reg *reg = get_sharp_reg(priv);

	reg->size.bits.width = width - 1;
	reg->size.bits.height = height - 1;
	priv->set_blk_dirty(priv, SHARP_PARA_REG_BLK, 1);

	return 0;
}

s32 de_sharp_set_window(u32 disp, u32 chn,
		      u32 win_enable, struct de_rect_o window)
{
	struct de_sharp_private *priv = &(sharp_priv[disp][chn]);
	struct sharp_reg *reg = get_sharp_reg(priv);

    reg->ctrl.bits.demo_en = win_enable;
	if (win_enable) {
		reg->demo_horz.bits.demo_horz_start = window.x;
		reg->demo_horz.bits.demo_horz_end = window.x + window.w - 1;
		reg->demo_vert.bits.demo_vert_start = window.y;
		reg->demo_vert.bits.demo_vert_end = window.y + window.h - 1;
	}
	priv->set_blk_dirty(priv, SHARP_PARA_REG_BLK, 1);

	return 0;
}

/*static s32 de_sharp_apply(u32 disp, u32 chn, u32 sharp_en)
{
	struct de_sharp_private *priv = &sharp_priv[disp][chn];
	struct sharp_reg *reg = get_sharp_reg(priv);

    reg->ctrl.bits.en = sharp_en;
    priv->set_blk_dirty(priv, SHARP_PARA_REG_BLK, 1);
	return 0;
}*/

#define SHARP_LEVEL_NUM 10
#define SHARP_CONFIG_NUM 3
s32 de_sharp_info2para(u32 disp, u32 chn,
		     u32 fmt, u32 dev_type,
		     struct sharp_config *para, u32 bypass)
{
	static u8 LTI[SHARP_LEVEL_NUM][SHARP_CONFIG_NUM] = {
		{0,  0,  0,},
		{2,  2,  2,},
		{2,  4,  4,},
		{4,  4,  8,},
		{8,  8,  8,},
		{8,  16, 16,},
		{16, 16, 16,},
		{16, 32, 32,},
		{32, 32, 32,},
		{64, 64, 64,},
	};

	static u8 PEAK[SHARP_LEVEL_NUM][SHARP_CONFIG_NUM] = {
		{16, 16, 0,},
		{16, 16, 2,},
		{16, 16, 4,},
		{16, 16, 8,},
		{16, 16, 16,},
		{16, 16, 24,},
		{16, 16, 32,},
		{16, 16, 40,},
		{16, 16, 52,},
		{16, 16, 64,},
	};

	struct de_sharp_private *priv = &(sharp_priv[disp][chn]);
	struct sharp_reg *reg = get_sharp_reg(priv);

	if (bypass || fmt == 1) {
		reg->ctrl.bits.en = 0;
		priv->set_blk_dirty(priv, SHARP_PARA_REG_BLK, 1);
		return 0;
	}
	reg->ctrl.bits.en = 1;
	para->lti_level = para->lti_level % SHARP_LEVEL_NUM;
	para->peak_level = para->peak_level % SHARP_LEVEL_NUM;
	reg->strengths.bits.strength_bp0 = LTI[para->lti_level][0];
	reg->strengths.bits.strength_bp1 = LTI[para->lti_level][1];
	reg->strengths.bits.strength_bp2 = LTI[para->lti_level][2];
	reg->d0_boosting.bits.d0_pst_level = PEAK[para->peak_level][0];
	reg->d0_boosting.bits.d0_neg_level = PEAK[para->peak_level][1];
	reg->d0_boosting.bits.d0_gain = PEAK[para->peak_level][2];
	priv->set_blk_dirty(priv, SHARP_PARA_REG_BLK, 1);
	return 0;
}

s32 de_sharp_enable(u32 disp, u32 chn, u32 en)
{
	struct de_sharp_private *priv = &(sharp_priv[disp][chn]);
	struct sharp_reg *reg = get_sharp_reg(priv);

	reg->ctrl.bits.en = en;
	priv->set_blk_dirty(priv, SHARP_PARA_REG_BLK, 1);

	return 0;
}

s32 de_sharp_init_para(u32 disp, u32 chn)
{
	struct de_sharp_private *priv = &(sharp_priv[disp][chn]);
	struct sharp_reg *reg = get_sharp_reg(priv);

	/*reg->ctrl.dwval = 0;*/
	reg->strengths.dwval = 0x101010;

	reg->extrema.dwval = 0x5;
	reg->edge_adaptive.dwval = 0x60810;
	reg->coring.dwval = 0x20002;
	reg->detail0_weight.dwval = 0x28;
	reg->horz_smooth.dwval = 0x6;
	reg->gaussian_coefs0.dwval = 0x31d40;
	reg->gaussian_coefs1.dwval = 0x20a1e2c;
	reg->gaussian_coefs2.dwval = 0x40e1c24;
	reg->gaussian_coefs3.dwval = 0x911181c;

	priv->set_blk_dirty(priv, SHARP_PARA_REG_BLK, 1);

	return 0;
}

