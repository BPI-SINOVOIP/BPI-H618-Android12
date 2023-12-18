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
 *  File name   :  display engine 35x dci basic function definition
 *
 *  History     :  2021/11/30 v0.1  Initial version
 *
 ******************************************************************************/

#include "de_dci_type.h"
#include "de_rtmx.h"
#include "de_enhance.h"

enum {
	DCI_PARA_REG_BLK = 0,
	DCI_CDF_REG_BLK,
	DCI_PDF_REG_BLK,
	DCI_REG_BLK_NUM,
};

struct de_dci_private {
	struct de_reg_mem_info reg_mem_info;
	u32 reg_blk_num;
	struct dci_status status;
	struct de_reg_block reg_blks[DCI_REG_BLK_NUM];
	void (*set_blk_dirty)(struct de_dci_private *priv,
		u32 blk_id, u32 dirty);
};

static struct de_dci_private dci_priv[DE_NUM][VI_CHN_NUM];


static inline struct dci_reg *get_dci_reg(struct de_dci_private *priv)
{
	return (struct dci_reg *)(priv->reg_blks[DCI_PARA_REG_BLK].vir_addr);
}

static void dci_set_block_dirty(
	struct de_dci_private *priv, u32 blk_id, u32 dirty)
{
	priv->reg_blks[blk_id].dirty = dirty;
}

static void dci_set_rcq_head_dirty(
	struct de_dci_private *priv, u32 blk_id, u32 dirty)
{
	if (priv->reg_blks[blk_id].rcq_hd) {
		priv->reg_blks[blk_id].rcq_hd->dirty.dwval = dirty;
	} else {
		DE_WRN("rcq_head is null ! blk_id=%d\n", blk_id);
	}
}

#define NUM_BLKS 16
#define HIST_BINS 16
#define PDF_REC_NUM 2 /*need two record to calculate*/
static u32 *g_pdf[DE_NUM][VI_CHN_NUM][PDF_REC_NUM];
static u32 *g_cur_pdf[DE_NUM][VI_CHN_NUM];

s32 de_dci_init(u32 disp, u32 chn, uintptr_t reg_base,
	u8 __iomem **phy_addr, u8 **vir_addr, u32 *size)
{
	struct de_dci_private *priv = &dci_priv[disp][chn];
	struct de_reg_mem_info *reg_mem_info = &(priv->reg_mem_info);
	struct de_reg_block *reg_blk;
	uintptr_t base;
	u32 phy_chn;
    int i;
	u32 rcq_used = de_feat_is_using_rcq(disp);

	phy_chn = de_feat_get_phy_chn_id(disp, chn);
	base = reg_base + DE_CHN_OFFSET(phy_chn) + CHN_DCI_OFFSET;

	reg_mem_info->phy_addr = *phy_addr;
	reg_mem_info->vir_addr = *vir_addr;
	reg_mem_info->size = DE_DCI_REG_MEM_SIZE;

	priv->reg_blk_num = DCI_REG_BLK_NUM;

	reg_blk = &(priv->reg_blks[DCI_PARA_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr;
	reg_blk->vir_addr = reg_mem_info->vir_addr;
	reg_blk->size = 0x100;
	reg_blk->reg_addr = (u8 __iomem *)base;

	reg_blk = &(priv->reg_blks[DCI_CDF_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x100;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x100;
	reg_blk->size = 0x400;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x100);

	reg_blk = &(priv->reg_blks[DCI_PDF_REG_BLK]);
	reg_blk->phy_addr = reg_mem_info->phy_addr + 0x500;
	reg_blk->vir_addr = reg_mem_info->vir_addr + 0x500;
	reg_blk->size = 0x400;
	reg_blk->reg_addr = (u8 __iomem *)(base + 0x500);

	*phy_addr += DE_DCI_REG_MEM_SIZE;
	*vir_addr += DE_DCI_REG_MEM_SIZE;
	*size -= DE_DCI_REG_MEM_SIZE;

	if (rcq_used)
		priv->set_blk_dirty = dci_set_rcq_head_dirty;
	else
		priv->set_blk_dirty = dci_set_block_dirty;

	g_cur_pdf[disp][chn] = kmalloc(NUM_BLKS * HIST_BINS * sizeof(u32),
				       GFP_KERNEL | __GFP_ZERO);
	if (g_cur_pdf[disp][chn] == NULL) {
		__wrn("malloc g_cur_pdf[%d][%d] memory fail! \n", disp, chn);
		return -1;
	}

	for (i = 0; i < PDF_REC_NUM; i++) {
		g_pdf[disp][chn][i] =
			kmalloc(NUM_BLKS * HIST_BINS * sizeof(u32),
				GFP_KERNEL | __GFP_ZERO);
		if (g_pdf[disp][chn][i] == NULL) {
			__wrn("malloc g_pdf[%d][%d] memory fail!\n", disp, chn);
			return -1;
		}
	}

	return 0;
}

s32 de_dci_exit(u32 disp, u32 chn)
{
	int i;
	kfree(g_cur_pdf[disp][chn]);
	for (i = 0; i < PDF_REC_NUM; i++) {
		kfree(g_pdf[disp][chn][i]);
	}

	return 0;
}

s32 de_dci_get_reg_blocks(u32 disp, u32 chn,
	struct de_reg_block **blks, u32 *blk_num)
{
	struct de_dci_private *priv = &(dci_priv[disp][chn]);
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

s32 de_dci_set_size(u32 disp, u32 chn, u32 width,
		    u32 height)
{
	struct de_dci_private *priv = &(dci_priv[disp][chn]);
	struct dci_reg *reg = get_dci_reg(priv);

	reg->size.bits.width = width - 1;
	reg->size.bits.height = height - 1;
	priv->set_blk_dirty(priv, DCI_PARA_REG_BLK, 1);

	return 0;
}

s32 de_dci_set_window(u32 disp, u32 chn,
		      u32 win_enable, struct de_rect_o window)
{
	struct de_dci_private *priv = &(dci_priv[disp][chn]);
	struct dci_reg *reg = get_dci_reg(priv);

	reg->ctl.bits.demo_en = win_enable;
	if (win_enable) {
		reg->demo_horz.bits.demo_horz_start = window.x;
		reg->demo_horz.bits.demo_horz_end = window.x + window.w - 1;
		reg->demo_vert.bits.demo_vert_start = window.y;
		reg->demo_vert.bits.demo_vert_end = window.y + window.h - 1;
	}
	priv->set_blk_dirty(priv, DCI_PARA_REG_BLK, 1);

	return 0;
}

/*static s32 de_dci_apply(u32 disp, u32 chn, u32 dci_en)
{
	struct de_dci_private *priv = &dci_priv[disp][chn];
	struct dci_reg *reg = get_dci_reg(priv);

	reg->ctl.bits.en = dci_en;
	priv->status.isenable = dci_en;
	priv->set_blk_dirty(priv, DCI_PARA_REG_BLK, 1);

	if (dci_en == 1) {
	} else {
	}

	return 0;
}*/

#define DCI_LEVEL_NUM 10
#define DCI_DATA_NUM 15
s32 de_dci_info2para(u32 disp, u32 chn,
		     u32 fmt, u32 dev_type,
		     struct dci_config *para, u32 bypass)
{
	static u8 BRIGHTEN[DCI_LEVEL_NUM][DCI_DATA_NUM] = {
		/*   0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 */
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},/* 0 */
		{0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1,},/* 1 */
		{0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,},/* 2 */
		{0, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,},/* 3 */
		{0, 1, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4,},/* 4 */
		{0, 1, 2, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 6, 6,},/* 5 */
		{0, 1, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 7, 7,},/* 6 */
		{0, 1, 4, 6, 6, 6, 6, 7, 7, 7, 8, 8, 8, 8, 8,},/* 7 */
		{0, 2, 6, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8,},/* 8 */
		{2, 4, 6, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,},/* 9 */
	};

	static u8 DARKEN[DCI_LEVEL_NUM][DCI_DATA_NUM] = {
		/*   0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 */
		{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},/* 0 */
		{1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,},/* 1 */
		{2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0,},/* 2 */
		{3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 0,},/* 3 */
		{4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 2, 1, 0,},/* 4 */
		{6, 6, 6, 6, 6, 5, 5, 5, 4, 4, 4, 3, 2, 1, 0,},/* 5 */
		{7, 7, 7, 7, 7, 6, 6, 6, 5, 5, 5, 5, 4, 1, 0,},/* 6 */
		{8, 8, 8, 8, 8, 7, 7, 7, 6, 6, 6, 6, 4, 1, 0,},/* 7 */
		{8, 8, 8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 6, 2, 0,},/* 8 */
		{8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 6, 4, 2,},/* 9 */
	};

	struct de_dci_private *priv = &(dci_priv[disp][chn]);
	struct dci_reg *reg = get_dci_reg(priv);
	u8 level = 0;

	if (bypass) {
		reg->ctl.bits.en = 0;
		priv->status.isenable = 0;
		priv->set_blk_dirty(priv, DCI_PARA_REG_BLK, 1);
		return 0;
	}

	level = para->contrast_level % DCI_LEVEL_NUM;

	reg->ctl.bits.en = 1;
	priv->status.isenable = 1;
	reg->brighten_level0.bits.brighten_level_0  = BRIGHTEN[level][0];
	reg->brighten_level0.bits.brighten_level_1  = BRIGHTEN[level][1];
	reg->brighten_level0.bits.brighten_level_2  = BRIGHTEN[level][2];
	reg->brighten_level0.bits.brighten_level_3  = BRIGHTEN[level][3];
	reg->brighten_level1.bits.brighten_level_4  = BRIGHTEN[level][4];
	reg->brighten_level1.bits.brighten_level_5  = BRIGHTEN[level][5];
	reg->brighten_level1.bits.brighten_level_6  = BRIGHTEN[level][6];
	reg->brighten_level1.bits.brighten_level_7  = BRIGHTEN[level][7];
	reg->brighten_level2.bits.brighten_level_8  = BRIGHTEN[level][8];
	reg->brighten_level2.bits.brighten_level_9  = BRIGHTEN[level][9];
	reg->brighten_level2.bits.brighten_level_10 = BRIGHTEN[level][10];
	reg->brighten_level2.bits.brighten_level_11 = BRIGHTEN[level][11];
	reg->brighten_level3.bits.brighten_level_12 = BRIGHTEN[level][12];
	reg->brighten_level3.bits.brighten_level_13 = BRIGHTEN[level][13];
	reg->brighten_level3.bits.brighten_level_14 = BRIGHTEN[level][14];

	reg->darken_level0.bits.darken_level_0  = DARKEN[level][0];
	reg->darken_level0.bits.darken_level_1  = DARKEN[level][1];
	reg->darken_level0.bits.darken_level_2  = DARKEN[level][2];
	reg->darken_level0.bits.darken_level_3  = DARKEN[level][3];
	reg->darken_level1.bits.darken_level_4  = DARKEN[level][4];
	reg->darken_level1.bits.darken_level_5  = DARKEN[level][5];
	reg->darken_level1.bits.darken_level_6  = DARKEN[level][6];
	reg->darken_level1.bits.darken_level_7  = DARKEN[level][7];
	reg->darken_level2.bits.darken_level_8  = DARKEN[level][8];
	reg->darken_level2.bits.darken_level_9  = DARKEN[level][9];
	reg->darken_level2.bits.darken_level_10 = DARKEN[level][10];
	reg->darken_level2.bits.darken_level_11 = DARKEN[level][11];
	reg->darken_level3.bits.darken_level_12 = DARKEN[level][12];
	reg->darken_level3.bits.darken_level_13 = DARKEN[level][13];
	reg->darken_level3.bits.darken_level_14 = DARKEN[level][14];

	priv->set_blk_dirty(priv, DCI_PARA_REG_BLK, 1);
	return 0;
}

s32 de_dci_enable(u32 disp, u32 chn, u32 en)
{
	struct de_dci_private *priv = &(dci_priv[disp][chn]);
	struct dci_reg *reg = get_dci_reg(priv);

	reg->ctl.bits.en = en;
	 priv->status.isenable = en;
	priv->set_blk_dirty(priv, DCI_PARA_REG_BLK, 1);

	return 0;
}

s32 de_dci_init_para(u32 disp, u32 chn)
{
	struct de_dci_private *priv = &(dci_priv[disp][chn]);
	struct dci_reg *reg = get_dci_reg(priv);

	reg->ctl.dwval = 0x80000001;
	reg->color_range.dwval = 0x1;
	reg->skin_protect.dwval = 0x00800000;
	reg->pdf_radius.dwval = 0x0;
	reg->count_bound.dwval = 0x03ff0000;
	reg->border_map_mode.dwval = 0x1;
	reg->brighten_level0.dwval = 0x05030100;
	reg->brighten_level1.dwval = 0x08080808;
	reg->brighten_level2.dwval = 0x08080808;
	reg->brighten_level3.dwval = 0x80808;
	reg->darken_level0.dwval = 0x8080808;
	reg->darken_level1.dwval = 0x8080808;
	reg->darken_level2.dwval = 0x8080807;
	reg->darken_level3.dwval = 0x30207;
	reg->chroma_comp_br_th0.dwval = 0x32281e14;
	reg->chroma_comp_br_th1.dwval = 0xff73463c;
	reg->chroma_comp_br_gain0.dwval = 0x0c0a0804;
	reg->chroma_comp_br_gain1.dwval = 0x10100e0e;
	reg->chroma_comp_br_slope0.dwval = 0x06060603;
	reg->chroma_comp_br_slope1.dwval = 0x00010606;
	reg->chroma_comp_dark_th0.dwval = 0x5a46321e;
	reg->chroma_comp_dark_th1.dwval = 0xffaa9682;
	reg->chroma_comp_dark_gain0.dwval = 0x02020201;
	reg->chroma_comp_dark_gain1.dwval = 0x02020202;
	reg->chroma_comp_dark_slope0.dwval = 0x03030302;
	reg->chroma_comp_dark_slope1.dwval = 0x00030301;

	reg->inter_frame_para.dwval = 0x0001001e;
	reg->ftd_hue_thr.dwval = 0x96005a;
	reg->ftd_chroma_thr.dwval = 0x28000a;
	reg->ftd_slp.dwval = 0x4040604;

	priv->set_blk_dirty(priv, DCI_PARA_REG_BLK, 1);

	return 0;
}

s32 de_dci_tasklet(u32 disp, u32 chn, u32 frame_cnt)
{
	struct de_dci_private *priv = &(dci_priv[disp][chn]);
	struct dci_reg *hw_reg = (struct dci_reg *) (priv->reg_blks[0].reg_addr);
	int blk_idx = 0;
	int bin_idx = 0;
	u32 *cur_pdf = g_cur_pdf[disp][chn];
	u32 cur_cdfs[NUM_BLKS][HIST_BINS];
	u32 *pdf_pre0 = NULL;
	u32 *pdf_pre1 = NULL;
	u32 *g_pdf_pre0 = NULL;
	u32 *g_pdf_pre1 = NULL;
	int cum_diff_pdfs = 0;
	int cum_pdf_pre0 = 0;
	int diff_pdfs = 0;
	int scene_change_th = 30;

	if (priv->status.isenable < 1)
		return 0;

	if (frame_cnt % 2) {
		g_pdf_pre0 = g_pdf[disp][chn][0];
		g_pdf_pre1 = g_pdf[disp][chn][1];
	} else {
		g_pdf_pre0 = g_pdf[disp][chn][1];
		g_pdf_pre1 = g_pdf[disp][chn][0];
	}

	memset(cur_cdfs, 0, sizeof(u32) * NUM_BLKS * HIST_BINS);
	/* Read histogram to pdf[256] */
	memcpy(g_pdf_pre0, hw_reg->pdf_stats, sizeof(u32) * NUM_BLKS * HIST_BINS);

	if (priv->status.runtime < 1) {
		priv->status.runtime++;
		return 0;
	}

	for (blk_idx = 0; blk_idx < NUM_BLKS; ++blk_idx) {
		pdf_pre0 = g_pdf_pre0 + blk_idx * HIST_BINS;
		pdf_pre1 = g_pdf_pre1 + blk_idx * HIST_BINS;

		cum_diff_pdfs = 0;
		cum_pdf_pre0 = 0;
		for (bin_idx = 0; bin_idx < HIST_BINS; ++bin_idx) {
			cum_diff_pdfs += abs(pdf_pre0[bin_idx] - pdf_pre1[bin_idx]);
			cum_pdf_pre0 += pdf_pre0[bin_idx];
		}

		diff_pdfs = (cum_diff_pdfs * 100) / cum_pdf_pre0;
		if (diff_pdfs > scene_change_th) {
			memcpy(cur_pdf + blk_idx * HIST_BINS, pdf_pre0,
			       HIST_BINS * sizeof(int));
		} else {
			for (bin_idx = 0; bin_idx < HIST_BINS; ++bin_idx) {
				cur_pdf[blk_idx * HIST_BINS + bin_idx] = (pdf_pre0[bin_idx]
					+ pdf_pre1[bin_idx] + 1) >> 1;
				pdf_pre0[bin_idx] = cur_pdf[blk_idx * HIST_BINS + bin_idx];
			}
		}
	}

	for (blk_idx = 0; blk_idx < NUM_BLKS; ++blk_idx) {
		cur_cdfs[blk_idx][0] = cur_pdf[blk_idx * HIST_BINS + 0];
		for (bin_idx = 1; bin_idx < HIST_BINS; ++bin_idx) {
			cur_cdfs[blk_idx][bin_idx] = cur_cdfs[blk_idx][bin_idx - 1]
			+ cur_pdf[blk_idx * HIST_BINS + bin_idx];
		}
	}

	memcpy(hw_reg->cdf_config, cur_cdfs, sizeof(u32) * 256);
	priv->set_blk_dirty(priv, DCI_CDF_REG_BLK, 1);
	return 0;
}
