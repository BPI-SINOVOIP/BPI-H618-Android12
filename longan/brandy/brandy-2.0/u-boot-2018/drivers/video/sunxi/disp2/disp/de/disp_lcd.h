/*
 * drivers/video/sunxi/disp2/disp/de/disp_lcd.h
 *
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __DISP_LCD_H__
#define __DISP_LCD_H__

#include "disp_private.h"

#define LCD_GPIO_NUM 8
#define LCD_POWER_NUM 4
#define LCD_POWER_STR_LEN 32
#define LCD_GPIO_REGU_NUM 3
#define LCD_GPIO_SCL (LCD_GPIO_NUM-2)
#define LCD_GPIO_SDA (LCD_GPIO_NUM-1)

typedef struct
{
	bool                  lcd_used;

	bool                  lcd_bl_en_used;
	disp_gpio_set_t       lcd_bl_en;
	char                  lcd_bl_en_power[LCD_POWER_STR_LEN];

	u32                   lcd_power_used[LCD_POWER_NUM];
	char                  lcd_power[LCD_POWER_NUM][LCD_POWER_STR_LEN];

	u32                   lcd_fix_power_used[LCD_POWER_NUM];/* 0: invalid, 1: gpio, 2: regulator */
	char                  lcd_fix_power[LCD_POWER_NUM][LCD_POWER_STR_LEN];

	bool                  lcd_gpio_used[LCD_GPIO_NUM];  //index4: scl;  index5: sda
	disp_gpio_set_t       lcd_gpio[LCD_GPIO_NUM];       //index4: scl; index5: sda
	u32                   gpio_hdl[LCD_GPIO_NUM];
	char                  lcd_gpio_power[LCD_GPIO_REGU_NUM][LCD_POWER_STR_LEN];

	char                  lcd_pin_power[LCD_GPIO_REGU_NUM][LCD_POWER_STR_LEN];

	u32                   backlight_bright;
	u32                   backlight_dimming;//IEP-drc backlight dimming rate: 0 -256 (256: no dimming; 0: the most dimming)
	u32                   backlight_curve_adjust[101];

	u32                   lcd_bright;
	u32                   lcd_contrast;
	u32                   lcd_saturation;
	u32                   lcd_hue;
}disp_lcd_cfg;

s32 disp_init_lcd(disp_bsp_init_para * para);
u32 get_lcd_device_num(void);
s32 disp_lcd_gpio_init(struct disp_device* lcd);
s32 disp_lcd_gpio_exit(struct disp_device* lcd);
s32 disp_lcd_gpio_set_direction(struct disp_device* lcd, u32 io_index, u32 direction);
s32 disp_lcd_gpio_get_value(struct disp_device* lcd,u32 io_index);
s32 disp_lcd_gpio_set_value(struct disp_device* lcd, u32 io_index, u32 data);
s32 disp_lcd_is_enabled(struct disp_device* lcd);

#if defined(SUPPORT_EINK) && defined(CONFIG_EINK_PANEL_USED)
extern int diplay_finish_flag;
#endif

struct disp_lcd_private_data {
	disp_lcd_flow             open_flow;
	disp_lcd_flow             close_flow;
	u32                       compat_panel_index;
	u32                       switch_to_compat_panel_index;
	disp_panel_para           panel_info;
	panel_extend_para         panel_extend_info;
	disp_lcd_cfg              lcd_cfg;
	disp_lcd_panel_fun        lcd_panel_fun;
	bool                      need_open_again;
	bool                      enabling;
	bool                      disabling;
	bool                      bl_enabled;
	u32                       irq_no;
	u32                       irq_no_dsi;
	u32                       irq_no_edp;
	u32                       enabled;
	u32                       power_enabled;
	u32                       bl_need_enabled;
	u32                       frame_per_sec;
	u32                       usec_per_line;
	u32                       judge_line;
	u32                       tri_finish_fail;
	struct {
		uintptr_t               dev;
		u32                     channel;
		u32                     polarity;
		u32                     period_ns;
		u32                     duty_ns;
		u32                     enabled;
	} pwm_info;
	struct clk *clk;
	struct clk *lvds_clk;
	struct clk *clk_tcon_lcd;
	struct clk *dsi_clk[CLK_DSI_NUM];
	struct clk *edp_clk;
	struct clk *clk_parent;
#if defined(DE_VERSION_V35X)
	/* In v35x, TCON can control all DSIS. */
	struct clk *clk_mipi_dsi_combphy[CLK_DSI_NUM];
#endif  /* DE_VERSION_V35X */
};

#endif
