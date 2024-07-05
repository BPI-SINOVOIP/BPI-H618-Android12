/******************************************************************************
 *
 * Copyright(c) 2013 - 2017 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/
/*
 * Description:
 *	This file can be applied to following platforms:
 *	CONFIG_PLATFORM_ARM_SUNXI Series platform
 *
 */

#include <drv_types.h>

#ifdef CONFIG_PLATFORM_ARM_SUNxI
extern int sunxi_usb_disable_hcd(__u32 usbc_no);
extern int sunxi_usb_enable_hcd(__u32 usbc_no);
//static int usb_wifi_host = 1;

extern void sunxi_wlan_set_power(bool on);
extern int sunxi_wlan_get_bus_index(void);
#endif

int platform_wifi_power_on(void)
{
	//int ret = 0;
	//int wlan_usb_index = 0;

	pr_info("========== [BPI] %s =========\n", __func__);

#ifdef CONFIG_PLATFORM_ARM_SUNxI
	sunxi_wlan_set_power(1);
	mdelay(50);

	//wlan_usb_index = sunxi_wlan_get_bus_index();
	//if (wlan_usb_index < 0)
	//	return wlan_usb_index;
	//sunxi_usb_enable_hcd(wlan_usb_index);
#endif
	return 0;
}

void platform_wifi_power_off(void)
{
	//int wlan_usb_index = 0;

#ifdef CONFIG_PLATFORM_ARM_SUNxI
	//wlan_usb_index = sunxi_wlan_get_bus_index();
        //if (wlan_usb_index < 0)
        //        return;
	//sunxi_usb_disable_hcd(wlan_usb_index);

	sunxi_wlan_set_power(0);
#endif
	pr_info("========= [BPI] %s ==========\n", __func__);

}
