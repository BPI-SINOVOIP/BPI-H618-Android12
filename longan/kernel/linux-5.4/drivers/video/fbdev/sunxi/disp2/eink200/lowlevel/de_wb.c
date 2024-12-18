/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/**
 *	All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
 *
 *	File name   :       de2_wb_ebios.c
 *
 *	Description :       display engine 2.0 WBC basic function definition
 *
 *	History     :       2014/03/03   wangxuan   initial version
 *	                    2014/04/02   wangxuan   change the register
 *	                                            operation from bits to word
 */
#include "de_wb_reg.h"
#define WB_MODULE_NUMBER 2
#define WB_OFFSET 0x010000

static volatile struct __wb_reg_t *wb_dev[WB_MODULE_NUMBER] = { NULL };

unsigned int wb_lan2coefftab16[16] = {
	0x00004000, 0x00033ffe, 0x00063efc, 0x000a3bfb,
	0xff0f37fb, 0xfe1433fb, 0xfd192ffb, 0xfd1f29fb,
	0xfc2424fc, 0xfb291ffd, 0xfb2f19fd, 0xfb3314fe,
	0xfb370fff, 0xfb3b0a00, 0xfc3e0600, 0xfe3f0300
};

unsigned int wb_lan2coefftab16_down[16] = {
	0x000e240e, 0x0010240c, 0x0013230a, 0x00142309,
	0x00162208, 0x01182106, 0x011a2005, 0x021b1f04,
	0x031d1d03, 0x041e1c02, 0x05201a01, 0x06211801,
	0x07221601, 0x09231400, 0x0a231300, 0x0c231100
};

int rgb2yuv601[3][4] = {
	{0x00bb, 0x0274, 0x003f, 0x04200},
	{0x1f98, 0x1ea5, 0x01c1, 0x20200},
	{0x01c1, 0x1e67, 0x1fd7, 0x20200} };/* rgb2yuv */
int rgb2yuv709[3][4] = {
	{0x00da, 0x02dc, 0x0049, 0x00200},
	{0xff8b, 0xfe76, 0x0200, 0x20200},
	{0x0200, 0xfe30, 0xffd1, 0x20200} };/* rgb2yuv */
int r2gray[3][4] = {
	{0x0155, 0x0155, 0x0156, 0x0200}, /* eink must use full(0x0200) otherwise DU and A2 cant show black (16 / 0)*/
	{0x0000, 0x0000, 0x0000, 0x0000},
	{0x0000, 0x0000, 0x0000, 0x0000} };/* rgb2gray */


/* ************************************************************************* */
/* function       : WB_eink_Set_Reg_Base(unsigned int sel, unsigned int base) */
/* description    : setup write-back controller register base */
/* parameters     : */
/* sel <controller select> */
/* base <register base> */
/* return         : */
/* success */
/* ************************************************************************* */
int wb_eink_set_reg_base(unsigned int sel, void __iomem *base)
{
	wb_dev[sel] = (struct __wb_reg_t *)(base + WB_OFFSET);

	return 0;
}

/* ************************************************************************* */
/* function       : WB_eink_Get_Reg_Base(unsigned int sel) */
/* description    : get write-back controller register base */
/* parameters     : */
/* sel <controller select> */
/*  */
/* return         : */
/* registers base */
/* ************************************************************************* */
unsigned long wb_eink_get_reg_base(unsigned int sel)
{
	unsigned long ret = 0;

	ret = (unsigned long)wb_dev[sel];

	return ret;
}

int wb_eink_disable(unsigned int sel)
{
	wb_dev[sel]->eink_ctl.dwval = 0;
	wb_dev[sel]->gctrl.dwval = 0;
	return 0;
}

/* ************************************************************************* */
/* function       : WB_eink_Writeback_Enable(unsigned int sel) */
/* description    : enable write-back once */
/* parameters     : */
/* sel <controller select>, en<0:disable; 1:enable> */
/* return         : */
/* success */
/* ************************************************************************* */
int wb_eink_enable(unsigned int sel, int fmt)
{
	wb_dev[sel]->gctrl.dwval |= 1;
	if (fmt != EINK_RGB888)
		wb_dev[sel]->eink_ctl.dwval |= 1;

	EINK_DEBUG_MSG("gctrl val=0x%x, eink_ctrl val=0x%x\n",
			wb_dev[sel]->gctrl.dwval, wb_dev[sel]->eink_ctl.dwval);
	return 0;
}

/* ************************************************************************* */
/* function       : WB_eink_win_Enable(unsigned int sel) */
/* description    : enable calculate windows information */
/* parameters     : */
/* sel <controller select>, en<0:disable; 1:enable> */
/* return         : */
/* success */
/* ************************************************************************* */
int wb_eink_set_win_en(unsigned int sel, u32 en)
{
	wb_dev[sel]->eink_ctl.bits.win_en = en;

	return 0;
}

int wb_eink_set_a2_mode(unsigned int sel)
{
	EINK_INFO_MSG("A2 mode\n");
	wb_dev[sel]->eink_ctl.bits.a2_mode = 1;

	return 0;
}

int wb_eink_set_gray_level(unsigned int sel, unsigned int val)
{
	EINK_DEBUG_MSG("set gray level val = %d\n", val);
	wb_dev[sel]->eink_ctl.bits.a16_gray = val;

	return 0;
}

int wb_eink_set_dither_mode(unsigned int sel, enum dither_mode dither_mode)
{
	EINK_DEBUG_MSG("dither mode is 0x%x\n", dither_mode);
	wb_dev[sel]->eink_ctl.bits.dither_mode = dither_mode;

	return 0;
}

int wb_eink_set_panel_bit(unsigned int sel, u32 bit_num)
{
	u8 panel_bit = 0;

	panel_bit  = __bits_num_config_to_reg(bit_num);

	EINK_INFO_MSG("bit_num = %d, panel_bit = %d\n", bit_num, panel_bit);
	wb_dev[sel]->eink_ctl.bits.panel_bit_mode = panel_bit;

	return 0;
}

int wb_eink_reset(unsigned int sel)
{
	wb_dev[sel]->gctrl.dwval = 0x20000010;

	return 0;
}

int wb_eink_dereset(unsigned int sel)
{
	wb_dev[sel]->gctrl.dwval = 0x00000000;

	return 0;
}
/* ************************************************************************* */
/* function: WB_eink_Set_Para(unsigned int sel,disp_capture_config *cfg) */
/* description: setup write-back controller parameters */
/* parameters : */
/* sel <controller select> */
/* eink_wb_config <write-back information,*/
/*include input_fb and output_fb information> */
/* return: */
/* 0	--	success */
/* -1	--	fail */
/* note		  : Don't support YUV input yet 14-02-28 */
/* when write-back format is yuv, default 16-235 output */
/* ************************************************************************* */
int wb_eink_set_para(unsigned int sel, __eink_wb_config_t *cfg)
{
	unsigned int in_w, in_h;
	unsigned int crop_x, crop_y, crop_w, crop_h;
	unsigned int out_addr;
	unsigned int out_buf_w, out_buf_h;
	unsigned int out_fmt;
	unsigned int out_window_w, out_window_h, out_window_x, out_window_y;
	unsigned int csc_std;
	unsigned int self_sync;
	unsigned int upd_in_pitch = 0;
	enum dither_mode dither_mode;
	bool win_en = false;
	u32 bypass_val = 0;

	/* get para */
	in_w         = cfg->frame.size.width;
	in_h         = cfg->frame.size.height;
	crop_x       = cfg->frame.crop.x;
	crop_y       = cfg->frame.crop.y;
	crop_w       = cfg->frame.crop.width;
	crop_h       = cfg->frame.crop.height;
	out_addr     = cfg->frame.addr;
	out_buf_w    = in_w;
	out_buf_h    = in_h;
	out_fmt      = cfg->out_fmt;/* DISP_FORMAT_Y;//cfg->format; fixed format */
	out_window_x = 0;
	out_window_y = 0;
	out_window_w = in_w;
	out_window_h = in_h;
	upd_in_pitch = in_w;
	if (out_fmt == EINK_RGB888)
		upd_in_pitch = upd_in_pitch * 3;

	csc_std	     = cfg->csc_std;
	win_en	     = cfg->win_en;
	dither_mode  = cfg->dither_mode;

	self_sync    = 1; /*eink default 1*/

	wb_eink_set_win_en(sel, win_en);
	wb_eink_set_dither_mode(sel, dither_mode);

	wb_dev[sel]->size.dwval = (in_w-1)|((in_h-1)<<16);
	/* input crop window */
	wb_dev[sel]->crop_coord.dwval = crop_x|((crop_y)<<16);
	wb_dev[sel]->crop_size.dwval = (crop_w - 1)|((crop_h - 1)<<16);
	/* output fmt */
	wb_dev[sel]->fmt.dwval = out_fmt;

	wb_dev[sel]->wb_addr_a0.dwval = out_addr;
	wb_dev[sel]->wb_addr_a1.dwval = 0;
	wb_dev[sel]->wb_addr_a2.dwval = 0;
	if (out_fmt == EINK_RGB888)
		wb_dev[sel]->wb_pitch0.dwval = out_buf_w * 3;
	else
		wb_dev[sel]->wb_pitch0.dwval = out_buf_w;
	wb_dev[sel]->wb_pitch1.dwval = 0;

	/* upd win in pitch*/
	wb_dev[sel]->eink_in_pitch.dwval = upd_in_pitch;

	/* CSC */
	bypass_val |= 0x1;
//	wb_dev[sel]->bypass.dwval |= 0x1;
	if (csc_std == 0 || csc_std > 2) {
		wb_dev[sel]->c00.bits.coff = rgb2yuv601[0][0];
		wb_dev[sel]->c01.bits.coff = rgb2yuv601[0][1];
		wb_dev[sel]->c02.bits.coff = rgb2yuv601[0][2];
		wb_dev[sel]->c03.bits.cont = rgb2yuv601[0][3];

		wb_dev[sel]->c10.bits.coff = rgb2yuv601[1][0];
		wb_dev[sel]->c11.bits.coff = rgb2yuv601[1][1];
		wb_dev[sel]->c12.bits.coff = rgb2yuv601[1][2];
		wb_dev[sel]->c13.bits.cont = rgb2yuv601[1][3];

		wb_dev[sel]->c20.bits.coff = rgb2yuv601[2][0];
		wb_dev[sel]->c21.bits.coff = rgb2yuv601[2][1];
		wb_dev[sel]->c22.bits.coff = rgb2yuv601[2][2];
		wb_dev[sel]->c23.bits.cont = rgb2yuv601[2][3];
	} else if (csc_std == 1) {
		wb_dev[sel]->c00.bits.coff = rgb2yuv709[0][0];
		wb_dev[sel]->c01.bits.coff = rgb2yuv709[0][1];
		wb_dev[sel]->c02.bits.coff = rgb2yuv709[0][2];
		wb_dev[sel]->c03.bits.cont = rgb2yuv709[0][3];

		wb_dev[sel]->c10.bits.coff = rgb2yuv709[1][0];
		wb_dev[sel]->c11.bits.coff = rgb2yuv709[1][1];
		wb_dev[sel]->c12.bits.coff = rgb2yuv709[1][2];
		wb_dev[sel]->c13.bits.cont = rgb2yuv709[1][3];

		wb_dev[sel]->c20.bits.coff = rgb2yuv709[2][0];
		wb_dev[sel]->c21.bits.coff = rgb2yuv709[2][1];
		wb_dev[sel]->c22.bits.coff = rgb2yuv709[2][2];
		wb_dev[sel]->c23.bits.cont = rgb2yuv709[2][3];
	} else if (csc_std == 2) {
		wb_dev[sel]->c00.bits.coff = r2gray[0][0];
		wb_dev[sel]->c01.bits.coff = r2gray[0][1];
		wb_dev[sel]->c02.bits.coff = r2gray[0][2];
		wb_dev[sel]->c03.bits.cont = r2gray[0][3];

		wb_dev[sel]->c10.bits.coff = r2gray[1][0];
		wb_dev[sel]->c11.bits.coff = r2gray[1][1];
		wb_dev[sel]->c12.bits.coff = r2gray[1][2];
		wb_dev[sel]->c13.bits.cont = r2gray[1][3];

		wb_dev[sel]->c20.bits.coff = r2gray[2][0];
		wb_dev[sel]->c21.bits.coff = r2gray[2][1];
		wb_dev[sel]->c22.bits.coff = r2gray[2][2];
		wb_dev[sel]->c23.bits.cont = r2gray[2][3];
	}
	/*fine scal*/

	bypass_val &= 0xfffffffb;
//	wb_dev[sel]->bypass.dwval &= 0xfffffffb;
	/* because of A100 read reg bug,val is 0, so we cant & */
	wb_dev[sel]->bypass.dwval = bypass_val;

	wb_dev[sel]->fs_hstep.dwval = 1 << 20;
	wb_dev[sel]->fs_vstep.dwval = 1 << 20;
	wb_dev[sel]->fs_insize.dwval =
		(out_window_w - 1) | ((out_window_h - 1) << 16);
	wb_dev[sel]->fs_outsize.dwval =
		(out_window_w - 1) | ((out_window_h - 1) << 16);

	/* (0x30000000|(sel << 16)|(self_sync == 1)?0x20:0x0); */
	wb_dev[sel]->gctrl.dwval |= 0x30000020;
	return 0;
}

int wb_eink_set_last_img(unsigned int sel, unsigned int last_img_addr)
{
	unsigned int addr;

	EINK_DEBUG_MSG("input !\n");
	addr = last_img_addr;
	wb_dev[sel]->eink_in_laddr.dwval = addr;
	return 0;
}

int wb_eink_get_upd_win(unsigned int sel, struct upd_win *upd_win)
{
	upd_win->left = wb_dev[sel]->eink_upd_win0.bits.win_left & 0xfff;
	upd_win->right = wb_dev[sel]->eink_upd_win1.bits.win_right & 0xfff;
	upd_win->top = wb_dev[sel]->eink_upd_win0.bits.win_top & 0xfff;
	upd_win->bottom = wb_dev[sel]->eink_upd_win1.bits.win_bottom & 0xfff;

	EINK_INFO_MSG("(%d, %d)~(%d, %d)\n", upd_win->left, upd_win->top,
						upd_win->right, upd_win->bottom);
	return 0;
}

int wb_eink_get_status(unsigned int sel)
{
	unsigned int status;

	status = wb_dev[sel]->status.dwval & 0x71;

	EINK_DEBUG_MSG("status = 0x%x\n", status);
	if (status == 0x11)
		return EWB_OK;
	else if (status & 0x20)
		return EWB_OVFL;
	else if (status & 0x40)
		return EWB_TIMEOUT;
	else if (status & 0x100)
		return EWB_BUSY;
	else
		return EWB_ERR;
}
/*
 * when user set auto_mode,we need get hist val to calc which mode to refresh
 * just care DU and GU16
 */
/* fix me */
enum upd_mode wb_eink_auto_mode_select(unsigned int sel, unsigned int gray_cnt,
		struct eink_img *last_img, struct eink_img *cur_img)
{
	unsigned int cur_gray_pixel_cnt = 0, last_gray_pixel_cnt = 0, total_cnt = 0;
	enum upd_mode mode = EINK_GU16_MODE;
	unsigned int width = 0, height = 0;
	unsigned int *cur_eink_hist = NULL, *last_eink_hist = NULL;

	width = cur_img->size.width;
	height = cur_img->size.height;

	cur_eink_hist = cur_img->eink_hist;
	last_eink_hist = last_img->eink_hist;

	total_cnt = width * height;
	cur_gray_pixel_cnt = total_cnt - cur_eink_hist[gray_cnt] - cur_eink_hist[0];/* gray_cnt最大灰阶值 */
	last_gray_pixel_cnt = total_cnt - last_eink_hist[gray_cnt] - last_eink_hist[0];

	EINK_INFO_MSG("total_cnt = %d, cur_gray = %d, last_gray = %d\n",
			total_cnt, cur_gray_pixel_cnt, last_gray_pixel_cnt);
	/* fix me */
	if ((cur_gray_pixel_cnt * 100) > (total_cnt * 80)) {/* 灰阶占比80%,说明是灰度图像 */
		mode = EINK_GU16_MODE;
	} else {
		if (last_gray_pixel_cnt * 100 > total_cnt * 80) {
			mode = EINK_DU_MODE;
		} else {
			mode = EINK_A2_MODE;
		}
	}
	return mode;
}

/*
 * when user set auto_mode,we need get cur hist to last hist
 * hist是每个灰阶占的像素个数
 */
int wb_eink_get_hist_val(unsigned int sel, unsigned int gray_level_cnt, unsigned int *eink_hist)
{
	int i = 0;

	EINK_DEBUG_MSG("\n");
	for (i = 0; i < gray_level_cnt; i++) {
		eink_hist[i] = wb_dev[sel]->eink_hist[i].dwval;
	}

	return 0;
}

/* INTERRUPT */
int wb_eink_interrupt_enable(unsigned int sel)
{
	/* register irq routine */
	/* os_request_irq(); */

	EINK_DEBUG_MSG("WB IRQ ENABLE\n");
	wb_dev[sel]->intr.dwval |= 0x00000001;
	return 0;
}

int wb_eink_interrupt_disable(unsigned int sel)
{
	/* unregister irq routine */

	wb_dev[sel]->intr.dwval &= 0xfffffffe;
	return 0;
}

/* write 1 to clear */
int wb_eink_interrupt_clear(unsigned int sel)
{
	wb_dev[sel]->status.dwval |= (WB_END_IE | WB_FINISH_IE |
					WB_FIFO_OVERFLOW_ERROR_IE |
					WB_TIMEOUT_ERROR_IE);
	return 0;
}
