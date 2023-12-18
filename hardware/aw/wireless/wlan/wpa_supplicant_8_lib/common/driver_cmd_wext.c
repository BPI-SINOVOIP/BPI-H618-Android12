#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "type.h"

#include "libhwinfo.h"

extern struct   driver_wext_cb_info *driver_wext_cb_info_list[];
extern uint32_t driver_wext_cb_info_count;

static driver_cmd_wext_cb *pcb = NULL;

static int check_initialed(void)
{
	const char *vendor_name = NULL;

	if (pcb != NULL)
		return 0;

	if ((vendor_name = get_wifi_vendor_name()) == NULL)
		return -1;

	wpa_printf(MSG_WARNING, "%s for wext, use %s wpa_supplicant_8_lib.", __func__, vendor_name);
	for (uint32_t i = 0; i < driver_wext_cb_info_count; i++) {
		if (strcmp(vendor_name, driver_wext_cb_info_list[i]->vendor) == 0) {
			pcb = driver_wext_cb_info_list[i]->cb;
			wpa_printf(MSG_WARNING, "%s found wpa_supplicant_8_lib vendor implement for %s.", __func__, vendor_name);
			return 0;
		}
	}

	wpa_printf(MSG_WARNING, "%s cannot found wpa_supplicant_8_lib vendor implement for %s.", __func__, vendor_name);

	return -1;
}

int wpa_driver_wext_combo_scan(void *priv, struct wpa_driver_scan_params *params)
{
	if (check_initialed() == 0)
		return pcb->wpa_driver_wext_combo_scan(priv, params);

	return 0;
}

int wpa_driver_wext_driver_cmd(void *priv, char *cmd, char *buf, size_t buf_len)
{
	if (check_initialed() == 0)
		return pcb->wpa_driver_wext_driver_cmd(priv, cmd, buf, buf_len);

	return 0;
}

int wpa_driver_signal_poll(void *priv, struct wpa_signal_info *si)
{
	if (check_initialed() == 0)
		return pcb->wpa_driver_signal_poll(priv, si);

	return 0;
}

