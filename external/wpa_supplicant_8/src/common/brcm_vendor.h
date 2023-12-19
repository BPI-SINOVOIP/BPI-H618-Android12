/*
 * Broadcom Corporation OUI and vendor specific assignments
 * Copyright (c) 2015, Broadcom Corporation.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#ifndef BRCM_VENDOR_H
#define BRCM_VENDOR_H

/*
 * This file is a registry of identifier assignments from the Broadcom
 * OUI 00:10:18 for purposes other than MAC address assignment. New identifiers
 * can be assigned through normal review process for changes to the upstream
 * hostap.git repository.
 */

#define OUI_BRCM  0x001018

/**
 * enum brcm_nl80211_vendor_subcmds - BRCM nl80211 vendor command identifiers
 *
 * @BRCM_VENDOR_SUBCMD_UNSPEC: Reserved value 0
 *
 * @BRCM_VENDOR_SUBCMD_PRIV_STR: String command/event
 */
enum brcm_nl80211_vendor_subcmds {
	BRCM_VENDOR_SUBCMD_UNSPEC		= 0,
	BRCM_VENDOR_SUBCMD_SET_PMK		= 4,
	BRCM_VENDOR_SUBCMD_SET_MAC		= 6,
	BRCM_VENDOR_SCMD_ACS			= 9,
	BRCM_VENDOR_SCMD_MAX			= 10
};

/**
 * enum brcm_nl80211_vendor_events - BRCM nl80211 asynchoronous event identifiers
 *
 * @BRCM_VENDOR_EVENT_UNSPEC: Reserved value 0
 *
 * @BRCM_VENDOR_EVENT_PRIV_STR: String command/event
 */
enum brcm_nl80211_vendor_events {
        BRCM_VENDOR_EVENT_UNSPEC,
	BRCM_VENDOR_EVENT_ACS			= 42,
	BRCM_VENDOR_EVENT_LAST			= 44
};

enum wl_vendor_attr_acs_offload {
	BRCM_VENDOR_ATTR_ACS_CHANNEL_INVALID = 0,
	BRCM_VENDOR_ATTR_ACS_PRIMARY_FREQ,
	BRCM_VENDOR_ATTR_ACS_SECONDARY_FREQ,
	BRCM_VENDOR_ATTR_ACS_VHT_SEG0_CENTER_CHANNEL,
	BRCM_VENDOR_ATTR_ACS_VHT_SEG1_CENTER_CHANNEL,

	BRCM_VENDOR_ATTR_ACS_HW_MODE,
	BRCM_VENDOR_ATTR_ACS_HT_ENABLED,
	BRCM_VENDOR_ATTR_ACS_HT40_ENABLED,
	BRCM_VENDOR_ATTR_ACS_VHT_ENABLED,
	BRCM_VENDOR_ATTR_ACS_CHWIDTH,
	BRCM_VENDOR_ATTR_ACS_CH_LIST,
	BRCM_VENDOR_ATTR_ACS_FREQ_LIST,

	BRCM_VENDOR_ATTR_ACS_LAST
};

enum brcm_wlan_vendor_attr {
	BRCM_ATTR_DRIVER_CMD            = 0,
	BRCM_ATTR_DRIVER_KEY_PMK        = 1,
	BRCM_ATTR_DRIVER_MAC_ADDR	= 3,
	BRCM_ATTR_DRIVER_AFTER_LAST     = 5,
	BRCM_ATTR_DRIVER_MAX            = BRCM_ATTR_DRIVER_AFTER_LAST - 1,
};
#endif /* BRCM_VENDOR_H */

