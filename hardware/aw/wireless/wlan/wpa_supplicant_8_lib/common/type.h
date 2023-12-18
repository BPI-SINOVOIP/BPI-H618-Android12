#ifndef __TYPE_H
#define __TYPE_H

#include <stdbool.h>
#include "driver_nl80211.h"
#include "driver_wext.h"

typedef struct {
	int (*wpa_driver_nl80211_driver_cmd)
		(void *priv, char *cmd, char *buf, size_t buf_len);

	int (*wpa_driver_set_p2p_noa)
		(void *priv, u8 count, int start, int duration);

	int (*wpa_driver_get_p2p_noa)
		(void *priv, u8 *buf, size_t len);

	int (*wpa_driver_set_p2p_ps)
		(void *priv, int legacy_ps, int opp_ps, int ctwindow);

	int (*wpa_driver_set_ap_wps_p2p_ie)
		(void *priv, const struct wpabuf *beacon,
		 const struct wpabuf *proberesp,
		 const struct wpabuf *assocresp);
} driver_cmd_nl80211_cb;

typedef struct {
	int (*wpa_driver_wext_combo_scan)
		(void *priv, struct wpa_driver_scan_params *params);

	int (*wpa_driver_wext_driver_cmd)
		(void *priv, char *cmd, char *buf, size_t buf_len);

	int (*wpa_driver_signal_poll)
		(void *priv, struct wpa_signal_info *si);
} driver_cmd_wext_cb;

struct driver_nl80211_cb_info {
	driver_cmd_nl80211_cb *cb;
	const char *vendor;
};

#endif

