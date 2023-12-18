 /*
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * core function of edp driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "edp_core.h"
#include "../edp_configs.h"
#include "../lowlevel/edp_lowlevel.h"
#include <linux/io.h>
#include <linux/delay.h>


/*edp_hal_xxx means xxx function is from lowlevel*/
s32 edp_core_init_early(u32 sel)
{
	return edp_hal_init_early(sel);
}

s32 edp_core_phy_init(u32 sel, struct edp_tx_core *edp_core)
{
	return edp_hal_phy_init(sel, edp_core);
}

void edp_core_set_reg_base(u32 sel, uintptr_t base)
{
	edp_hal_set_reg_base(sel, base);
}

bool edp_core_get_hpd_status(u32 sel, struct edp_tx_core *edp_core)
{
	return edp_hal_get_hpd_status(sel, edp_core);
}

s32 edp_core_read_dpcd(u32 sel, char *dpcd_rx_buf)
{
	return edp_hal_read_dpcd(sel, dpcd_rx_buf);
}

s32 edp_core_set_video_mode(u32 sel, struct edp_tx_core *edp_core)
{
	return edp_hal_set_video_mode(sel, edp_core);
}

s32 edp_core_enable(u32 sel, struct edp_tx_core *edp_core)
{
	return edp_hal_enable(sel, edp_core);
}

s32 edp_core_disable(u32 sel, struct edp_tx_core *edp_core)
{
	return edp_hal_disable(sel, edp_core);
}

s32 edp_get_link_status(u32 sel, char *link_status)
{
	return edp_hal_aux_read(sel, DPCD_0202H, LINK_STATUS_SIZE, link_status);
}


bool edp_sink_support_fast_train(u32 sel)
{

	char tmp_rx_buf[16];

	memset(tmp_rx_buf, 0, sizeof(tmp_rx_buf));

	if (edp_core_aux_read(sel, DPCD_0003H, 1, &tmp_rx_buf[0]) < 0)
		return false;

	if (tmp_rx_buf[0] & DPCD_FAST_TRAIN_MASK)
		return true;
	else
		return false;
}


bool edp_source_support_fast_train(u32 sel)
{
	return edp_hal_support_fast_train(sel);
}

bool edp_sink_support_tps3(u32 sel)
{
	char tmp_rx_buf[16];

	memset(tmp_rx_buf, 0, sizeof(tmp_rx_buf));

	if (edp_core_aux_read(sel, DPCD_0002H, 1, &tmp_rx_buf[0]) < 0)
		return false;

	if (tmp_rx_buf[0] & DPCD_TPS3_SUPPORT_MASK)
		return true;
	else
		return false;
}

bool edp_source_support_tps3(u32 sel)
{
	return edp_hal_support_tps3(sel);
}


void edp_core_set_lane_para(u32 sel, struct edp_tx_core *edp_core)
{
	u64 bit_rate = edp_core->lane_para.bit_rate;
	u32 lane_cnt = edp_core->lane_para.lane_cnt;

	edp_hal_set_lane_rate(sel, bit_rate);
	edp_hal_set_lane_cnt(sel, lane_cnt);
}


void edp_core_set_training_para(u32 sel, struct edp_tx_core *edp_core)
{
	u32 sw[4];
	u32 pre[4];
	u32 lane_count = edp_core->lane_para.lane_cnt;
	u32 i = 0;

	sw[0] = edp_core->lane_para.lane0_sw;
	sw[1] = edp_core->lane_para.lane1_sw;
	sw[2] = edp_core->lane_para.lane2_sw;
	sw[3] = edp_core->lane_para.lane3_sw;
	pre[0] = edp_core->lane_para.lane0_pre;
	pre[1] = edp_core->lane_para.lane1_pre;
	pre[2] = edp_core->lane_para.lane2_pre;
	pre[3] = edp_core->lane_para.lane3_pre;

	for (i = 0; i < lane_count; i++) {
		edp_hal_set_lane_swing_voltage(sel, i, sw[i]);
		edp_hal_sel_lane_pre_emphasis(sel, i, pre[i]);
	}
}

s32 edp_core_set_training_pattern(u32 sel, u32 pattern)
{
	edp_hal_set_training_pattern(sel, pattern);
	return RET_OK;
}



s32 edp_dpcd_set_lane_para(u32 sel, struct edp_tx_core *edp_core)
{
	char tmp_tx_buf[16];
	s32 ret = RET_FAIL;

	memset(tmp_tx_buf, 0, sizeof(tmp_tx_buf));
	tmp_tx_buf[0] = edp_core->lane_para.bit_rate / 10000000 / 27;
	tmp_tx_buf[1] = edp_core->lane_para.lane_cnt;

	ret = edp_hal_aux_write(sel, DPCD_0100H, 2,  &tmp_tx_buf[0]);

	return ret;
}

s32 edp_dpcd_set_training_para(u32 sel, struct edp_tx_core *edp_core)
{
	char tmp_tx_buf[16];
	u32 sw[4];
	u32 pre[4];
	u32 lane_count = edp_core->lane_para.lane_cnt;
	u32 i = 0;

	sw[0] = edp_core->lane_para.lane0_sw;
	sw[1] = edp_core->lane_para.lane1_sw;
	sw[2] = edp_core->lane_para.lane2_sw;
	sw[3] = edp_core->lane_para.lane3_sw;
	pre[0] = edp_core->lane_para.lane0_pre;
	pre[1] = edp_core->lane_para.lane1_pre;
	pre[2] = edp_core->lane_para.lane2_pre;
	pre[3] = edp_core->lane_para.lane3_pre;

	memset(tmp_tx_buf, 0, sizeof(tmp_tx_buf));

	for (i = 0; i < lane_count; i++) {
		tmp_tx_buf[i] = (pre[i] << DPCD_PLEVEL_SHIFT) | (sw[i] << DPCD_VLEVEL_SHIFT);
		if (sw[i] == MAX_VLEVEL)
			tmp_tx_buf[i] |= DPCD_MAX_VLEVEL_REACHED_FLAG;
		if (pre[i] == MAX_PLEVEL)
			tmp_tx_buf[i] |= DPCD_MAX_PLEVEL_REACHED_FLAG;
	}

	edp_hal_aux_write(sel, DPCD_0103H, lane_count, &tmp_tx_buf[0]);

	return RET_OK;
}

s32 edp_dpcd_set_training_pattern(u32 sel, u8 pattern)
{
	char tmp_tx_buf[16];

	memset(tmp_tx_buf, 0, sizeof(tmp_tx_buf));

	tmp_tx_buf[0] = pattern;

	if (pattern && (pattern != TRAINING_PATTERN_4))
		tmp_tx_buf[0] |= DPCD_SCRAMBLING_DISABLE_FLAG;

	edp_hal_aux_write(sel, DPCD_0102H, 1, &tmp_tx_buf[0]);

	return RET_OK;
}

s32 edp_training_pattern_clear(u32 sel, struct edp_tx_core *edp_core)
{
	s32 ret = RET_OK;

	edp_core_set_training_pattern(sel, TRAINING_PATTERN_DISABLE);
	ret = edp_dpcd_set_training_pattern(sel, TRAINING_PATTERN_DISABLE);
	if (ret < 0)
		return ret;

	udelay(2 * edp_core->interval_EQ);

	return ret;
}

bool edp_cr_training_done(char *link_status, u32 lane_count)
{
	u32 i = 0;
	u8 lane_status = 0;

	for (i = 0; i < lane_count; i++) {
		lane_status = (link_status[DPCD_LANE_STATUS_OFFSET(i)]) & DPCD_LANE_STATUS_MASK(i);
		EDP_CORE_DBG("DPCD[0x%x] = 0x%x\n", LINK_STATUS_BASE + DPCD_LANE_STATUS_OFFSET(i),\
		       link_status[DPCD_LANE_STATUS_OFFSET(i)]);
		EDP_CORE_DBG("CR_lane%d_status = 0x%x\n", i, lane_status);
		if ((lane_status & DPCD_CR_DONE(i)) != DPCD_CR_DONE(i)) {
			EDP_ERR("CR training fail!\n");
			return false;
		}
	}

	EDP_CORE_DBG("CR training success!\n");
	return true;
}

bool edp_eq_training_done(char *link_status, u32 lane_count)
{
	u32 i = 0;
	u8 lane_status = 0;
	u8 align_status = 0;

	align_status = (link_status[DPCD_0204H - LINK_STATUS_BASE]);
	EDP_CORE_DBG("Align_status = 0x%x\n", align_status);
	if ((align_status & DPCD_ALIGN_DONE) != DPCD_ALIGN_DONE) {
		EDP_ERR("EQ training align fail!\n");
		return false;
	}

	for (i = 0; i < lane_count; i++) {
		lane_status = (link_status[DPCD_LANE_STATUS_OFFSET(i)]) & DPCD_LANE_STATUS_MASK(i);
		EDP_CORE_DBG("DPCD[0x%x] = 0x%x\n", LINK_STATUS_BASE + DPCD_LANE_STATUS_OFFSET(i),\
		       link_status[DPCD_LANE_STATUS_OFFSET(i)]);
		EDP_CORE_DBG("EQ_lane_status = 0x%x\n", lane_status);
		if ((lane_status & DPCD_EQ_TRAIN_DONE(i)) !=  DPCD_EQ_TRAIN_DONE(i)) {
			EDP_ERR("EQ training fail!\n");
			return false;
		}
	}

	EDP_CORE_DBG("EQ training success!\n");
	return true;
}

bool edp_swing_level_reach_max(struct edp_tx_core *edp_core)
{
	u32 lane_count = edp_core->lane_para.lane_cnt;
	u32 sw[4];
	u32 i = 0;

	sw[0] = edp_core->lane_para.lane0_sw;
	sw[1] = edp_core->lane_para.lane1_sw;
	sw[2] = edp_core->lane_para.lane2_sw;
	sw[3] = edp_core->lane_para.lane3_sw;

	for (i = 0; i < lane_count; i++) {
		if (sw[i] == MAX_VLEVEL)
			return true;
	}

	return false;
}

u32 edp_adjust_train_para(char *link_status, struct edp_tx_core *edp_core)
{
	u32 old_sw[4];
	u32 old_pre[4];
	u32 adjust_sw[4];
	u32 adjust_pre[4];
	u32 i = 0;
	struct edp_lane_para *lane_para = &edp_core->lane_para;
	u32 lane_count = lane_para->lane_cnt;
	u32 change;

	change = 0;

	old_sw[0] = lane_para->lane0_sw;
	old_sw[1] = lane_para->lane1_sw;
	old_sw[2] = lane_para->lane2_sw;
	old_sw[3] = lane_para->lane3_sw;
	old_pre[0] = lane_para->lane0_pre;
	old_pre[1] = lane_para->lane1_pre;
	old_pre[2] = lane_para->lane2_pre;
	old_pre[3] = lane_para->lane3_pre;

	for (i = 0; i < lane_count; i++) {
		adjust_sw[i] = (link_status[DPCD_LANE_ADJ_OFFSET(i)] & VADJUST_MASK(i)) >> VADJUST_SHIFT(i);
		EDP_CORE_DBG("DPCD[0x%x] = 0x%x, adjust_sw[%d] = 0x%x\n", LINK_STATUS_BASE + DPCD_LANE_ADJ_OFFSET(i),\
		       link_status[DPCD_LANE_ADJ_OFFSET(i)], i, adjust_sw[i]);

		adjust_pre[i] = (link_status[DPCD_LANE_ADJ_OFFSET(i)] & PADJUST_MASK(i)) >> PADJUST_SHIFT(i);
		EDP_CORE_DBG("DPCD[0x%x] = 0x%x, adjust_pre[%d] = 0x%x\n", LINK_STATUS_BASE + DPCD_LANE_ADJ_OFFSET(i),\
		       link_status[DPCD_LANE_ADJ_OFFSET(i)], i, adjust_pre[i]);
	}

	lane_para->lane0_sw = adjust_sw[0];
	lane_para->lane1_sw = adjust_sw[1];
	lane_para->lane2_sw = adjust_sw[2];
	lane_para->lane3_sw = adjust_sw[3];

	lane_para->lane0_pre = adjust_pre[0];
	lane_para->lane1_pre = adjust_pre[1];
	lane_para->lane2_pre = adjust_pre[2];
	lane_para->lane3_pre = adjust_pre[3];

	for (i = 0; i < lane_count; i++) {
		if (old_sw[i] != adjust_sw[i])
			change |= SW_VOLTAGE_CHANGE_FLAG;
	}

	for (i = 0; i < lane_count; i++) {
		if (old_pre[i] != adjust_pre[i])
			change |= PRE_EMPHASIS_CHANGE_FLAG;
	}

	EDP_CORE_DBG("edp lane para change: %d\n", change);
	return change;
}

s32 edp_link_cr_training(u32 sel, struct edp_tx_core *edp_core)
{
	char link_status[16];
	u32 try_cnt = 0;
	u32 lane_count = edp_core->lane_para.lane_cnt;
	u32 change;
	u32 timeout = 0;

	edp_core_set_training_pattern(sel, TRAINING_PATTERN_1);
	edp_dpcd_set_training_pattern(sel, TRAINING_PATTERN_1);

	for (try_cnt = 0; try_cnt < TRY_CNT_MAX; try_cnt++) {
		udelay(10 * edp_core->interval_CR);

		memset(link_status, 0, sizeof(link_status));
		edp_get_link_status(sel, link_status);
		if (edp_cr_training_done(link_status, lane_count)) {
			EDP_CORE_DBG("edp training1(clock recovery training) success!\n");
			return RET_OK;
		}

		if (timeout >= TRY_CNT_TIMEOUT) {
			EDP_ERR("edp training1(clock recovery training) timeout!\n");
			return RET_FAIL;
		}

		if (edp_swing_level_reach_max(edp_core)) {
			EDP_ERR("swing voltage reach max level, training1(clock recovery training) fail!\n");
			return RET_FAIL;
		}

		change = edp_adjust_train_para(link_status, edp_core);
		if (change & SW_VOLTAGE_CHANGE_FLAG) {
			try_cnt = 0;
		}

		if ((change & SW_VOLTAGE_CHANGE_FLAG) || (change & PRE_EMPHASIS_CHANGE_FLAG)) {
		}
			edp_core_set_training_para(sel, edp_core);
			edp_dpcd_set_training_para(sel, edp_core);

		timeout++;
	}

	EDP_ERR("retry 5 times but still fail, training1(clock recovery training) fail!\n");
	return RET_FAIL;
}

s32 edp_link_eq_training(u32 sel, struct edp_tx_core *edp_core)
{
	char link_status[16];
	u32 lane_count = edp_core->lane_para.lane_cnt;
	u32 try_cnt = 0;
	bool change;

	if (edp_source_support_tps3(sel) && edp_sink_support_tps3(sel)) {
		edp_core_set_training_pattern(sel, TRAINING_PATTERN_3);
		edp_dpcd_set_training_pattern(sel, TRAINING_PATTERN_3);
	} else {
		edp_core_set_training_pattern(sel, TRAINING_PATTERN_2);
		edp_dpcd_set_training_pattern(sel, TRAINING_PATTERN_2);
	}

	for (try_cnt = 0; try_cnt < TRY_CNT_MAX; try_cnt++) {
		udelay(10 * edp_core->interval_EQ);
		memset(link_status, 0, sizeof(link_status));
		edp_get_link_status(sel, link_status);
		if (edp_eq_training_done(link_status, lane_count)) {
			EDP_CORE_DBG("edp training2 (equalization training) success!\n");
			return RET_OK;
		}

		change = edp_adjust_train_para(link_status, edp_core);
		if ((change & SW_VOLTAGE_CHANGE_FLAG) || (change & PRE_EMPHASIS_CHANGE_FLAG)) {
		}
			edp_core_set_training_para(sel, edp_core);
			edp_dpcd_set_training_para(sel, edp_core);
	}

	EDP_ERR("retry 5 times but still fail, training2(equalization training) fail!\n");
	return RET_FAIL;
}

s32 edp_fast_link_train(u32 sel, struct edp_tx_core *edp_core)
{
	char link_status[LINK_STATUS_SIZE];
	u32 lane_count = edp_core->lane_para.lane_cnt;

	edp_core_set_lane_para(sel, edp_core);

	edp_core_set_training_para(sel, edp_core);

	edp_core_set_training_pattern(sel, TRAINING_PATTERN_1);
	udelay(10 * edp_core->interval_CR);

	if (edp_source_support_tps3(sel) && edp_sink_support_tps3(sel)) {
		edp_core_set_training_pattern(sel, TRAINING_PATTERN_3);
	} else {
		edp_core_set_training_pattern(sel, TRAINING_PATTERN_2);
	}
	udelay(10 * edp_core->interval_EQ);

	memset(link_status, 0, sizeof(link_status));
	if (loglevel_debug & 0x2) {
		edp_get_link_status(sel, link_status);
		if (!edp_cr_training_done(link_status, lane_count)) {
			EDP_ERR("edp fast train fail in training1");
			return RET_FAIL;
		}

		if (!edp_eq_training_done(link_status, lane_count)) {
			EDP_ERR("edp fast train fail in training2");
			return RET_FAIL;
		}
	}

	return RET_OK;
}

s32 edp_full_link_train(u32 sel, struct edp_tx_core *edp_core)
{
	s32 ret = RET_OK;;

	edp_core_set_lane_para(sel, edp_core);
	ret = edp_dpcd_set_lane_para(sel, edp_core);
	if (ret < 0)
		return ret;

	edp_core_set_training_para(sel, edp_core);
	ret = edp_dpcd_set_training_para(sel, edp_core);
	if (ret < 0)
		return ret;

	ret = edp_link_cr_training(sel, edp_core);
	if (ret < 0)
		return ret;

	ret = edp_link_eq_training(sel, edp_core);
	if (ret < 0)
		return ret;

	return ret;
}


s32 edp_link_training(u32 sel, struct edp_tx_core *edp_core)
{
	if (edp_source_support_fast_train(sel) \
		&& edp_sink_support_fast_train(sel))
		return edp_fast_link_train(sel, edp_core);
	else
		return edp_full_link_train(sel, edp_core);
}

s32 edp_main_link_setup(u32 sel, struct edp_tx_core *edp_core)
{
	s32 ret = -1;

	ret = edp_link_training(sel, edp_core);
	if (ret < 0)
		return ret;
	ret = edp_training_pattern_clear(sel, edp_core);

	return ret;
}

s32 edp_core_link_start(u32 sel)
{
	return edp_hal_link_start(sel);
}

s32 edp_low_power_en(u32 sel, bool en)
{
	char tmp_tx_buf[16];

	memset(tmp_tx_buf, 0, sizeof(tmp_tx_buf));

	if (en)
		tmp_tx_buf[0] = DPCD_LOW_POWER_ENTER;
	else
		tmp_tx_buf[0] = DPCD_LOW_POWER_EXIT;

	edp_hal_aux_write(sel, DPCD_0600H, 1, &tmp_tx_buf[0]);

	return RET_OK;
}

void edp_core_irq_handler(u32 sel, struct edp_tx_core *edp_core)
{
	edp_hal_irq_handler(sel, edp_core);
}

s32 edp_core_irq_enable(u32 sel, u32 irq_id, bool en)
{
	if (en)
		return edp_hal_irq_enable(sel, irq_id);
	else
		return edp_hal_irq_disable(sel, irq_id);
}

s32 edp_core_irq_query(u32 sel)
{
	return edp_hal_irq_query(sel);
}

s32 edp_core_irq_clear(u32 sel)
{
	return edp_hal_irq_clear(sel);
}

s32 edp_core_get_cur_line(u32 sel)
{
	return edp_hal_get_cur_line(sel);
}

s32 edp_core_get_start_dly(u32 sel)
{
	return edp_hal_get_start_dly(sel);
}

void edp_core_show_builtin_patten(u32 sel, u32 pattern)
{
	edp_hal_show_builtin_patten(sel, pattern);
}

s32 edp_core_aux_read(u32 sel, s32 addr, s32 lenth, char *buf)
{
	return edp_hal_aux_read(sel, addr, lenth, buf);
}

s32 edp_core_aux_write(u32 sel, s32 addr, s32 lenth, char *buf)
{
	return edp_hal_aux_write(sel, addr, lenth, buf);
}

s32 edp_core_aux_i2c_read(u32 sel, s32 i2c_addr, s32 lenth, char *buf)
{
	return edp_hal_aux_i2c_read(sel, i2c_addr, lenth, buf);
}

s32 edp_core_aux_i2c_write(u32 sel, s32 i2c_addr, s32 lenth, char *buf)
{
	return edp_hal_aux_i2c_write(sel, i2c_addr, lenth, buf);
}

s32 edp_core_audio_set_para(u32 sel, edp_audio_t *para)
{
	return edp_hal_audio_set_para(sel, para);
}

s32 edp_core_audio_enable(u32 sel)
{
	return edp_hal_audio_enable(sel);
}

s32 edp_core_audio_disable(u32 sel)
{
	return edp_hal_audio_disable(sel);
}

s32 edp_core_ssc_enable(u32 sel, bool enable)
{
	return edp_hal_ssc_enable(sel, enable);
}

bool edp_core_ssc_is_enabled(u32 sel)
{
	return edp_hal_ssc_is_enabled(sel);
}

s32 edp_core_ssc_get_mode(u32 sel)
{
	return edp_hal_ssc_get_mode(sel);
}

s32 edp_core_ssc_set_mode(u32 sel, u32 mode)
{
	return edp_hal_ssc_set_mode(sel, mode);
}

s32 edp_core_psr_enable(u32 sel, bool enable)
{
	return edp_hal_psr_enable(sel, enable);
}

bool edp_core_psr_is_enabled(u32 sel)
{
	return edp_hal_psr_is_enabled(sel);
}

s32 edp_core_get_color_fmt(u32 sel)
{
	return edp_hal_get_color_fmt(sel);
}

s32 edp_core_get_pixclk(u32 sel)
{
	return edp_hal_get_pixclk(sel);
}

s32 edp_core_get_train_pattern(u32 sel)
{
	return edp_hal_get_train_pattern(sel);
}

s32 edp_core_get_lane_para(u32 sel, struct edp_lane_para *tmp_lane_para)
{
	return edp_hal_get_lane_para(sel, tmp_lane_para);
}

s32 edp_core_get_tu_size(u32 sel)
{
	return edp_hal_get_tu_size(sel);
}

s32 edp_core_get_valid_symbol_per_tu(u32 sel)
{
	return edp_hal_get_valid_symbol_per_tu(sel);
}

bool edp_core_audio_is_enabled(u32 sel)
{
	return edp_hal_audio_is_enabled(sel);
}

s32 edp_core_get_audio_if(u32 sel)
{
	return edp_hal_get_audio_if(sel);
}

s32 edp_core_audio_is_mute(u32 sel)
{
	return edp_hal_audio_is_mute(sel);
}

s32 edp_core_get_audio_chn_cnt(u32 sel)
{
	return edp_hal_get_audio_chn_cnt(sel);
}

s32 edp_core_get_audio_date_width(u32 sel)
{
	return edp_hal_get_audio_date_width(sel);
}
