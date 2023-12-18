#ifndef _LOWI_HEADER_
#define _LOWI_HEADER_

#include "wifi_hal.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define LOWI_IFNAME "wlan0"

#define ONE_SIDED_RANGING_SUPPORTED 0x00000001
#define DUAL_SIDED_RANGING_SUPPORED 0x00000002
/*
 * This structure is a table of function pointers to the functions
 * used by the wifihal to interface with LOWI
 */
typedef struct
{
  /* lowi-client interface functions */
  int (*init)();
  int (*destroy)();
  /* rtt functions */
  int (*get_rtt_capabilities)(wifi_interface_handle iface,
                              wifi_rtt_capabilities *capabilities);
  int (*rtt_range_request)(u32 request_id,
                           wifi_interface_handle iface,
                           u32 num_rtt_config,
                           wifi_rtt_config rtt_config[],
                           wifi_rtt_event_handler handler);
  int (*rtt_range_cancel)(u32 request_id,
                          u32 num_devices,
                          mac_addr addr[]);
  /* Additional lowi-client interface functions */
  int (*get_lowi_version) (u16* major_version,
                           u16* minor_version,
                           u16* micro_version);
  int (*get_lowi_capabilities)(u32* capabilities);
 /* gscan functions */
  wifi_error (*get_valid_channels)(wifi_interface_handle iface,
                                   u32 band,
                                   u32 max_channels,
                                   wifi_channel *channels,
                                   int *num_channels);

  wifi_error (*get_gscan_capabilities)(wifi_interface_handle handle,
                                       wifi_gscan_capabilities *capabilities);

  wifi_error (*start_gscan)(wifi_request_id request_id,
                            wifi_interface_handle iface,
                            wifi_scan_cmd_params params,
                            wifi_scan_result_handler handler);

  wifi_error (*stop_gscan)(wifi_request_id request_id,
                           wifi_interface_handle iface);

  wifi_error (*get_cached_gscan_results)(wifi_interface_handle iface,
                                         byte flush,
                                         u32 max,
                                         wifi_cached_scan_results *results,
                                         int *num);

  wifi_error (*set_bssid_hotlist)(wifi_request_id request_id,
                                  wifi_interface_handle iface,
                                  wifi_bssid_hotlist_params params,
                                  wifi_hotlist_ap_found_handler handler);

  wifi_error (*reset_bssid_hotlist)(wifi_request_id request_id,
                                    wifi_interface_handle iface);

  wifi_error (*set_significant_change_handler)(wifi_request_id id,
                                               wifi_interface_handle iface,
                                               wifi_significant_change_params params,
                                               wifi_significant_change_handler handler);

  wifi_error (*reset_significant_change_handler)(wifi_request_id id,
                                                 wifi_interface_handle iface);

  wifi_error (*set_ssid_hotlist)(wifi_request_id id,
                                 wifi_interface_handle iface,
                                 wifi_ssid_hotlist_params params,
                                 wifi_hotlist_ssid_handler handler);

  wifi_error (*reset_ssid_hotlist)(wifi_request_id id,
                                   wifi_interface_handle iface);

  // API to configure the LCI. Used in RTT Responder mode only
  wifi_error (*rtt_set_lci)(wifi_request_id id,
                            wifi_interface_handle iface,
                            wifi_lci_information *lci);

  // API to configure the LCR. Used in RTT Responder mode only.
  wifi_error (*rtt_set_lcr)(wifi_request_id id,
                            wifi_interface_handle iface,
                            wifi_lcr_information *lcr);

  /**
   * Get RTT responder information e.g. WiFi channel to enable responder on.
   */
  wifi_error (*rtt_get_responder_info)(wifi_interface_handle iface,
                                       wifi_rtt_responder *responder_info);

  /**
   * Enable RTT responder mode.
   * channel_hint - hint of the channel information where RTT responder should
   *                be enabled on.
   * max_duration_seconds - timeout of responder mode.
   * responder_info - responder information e.g. channel used for RTT responder,
   *                  NULL if responder is not enabled.
   */
  wifi_error (*enable_responder)(wifi_request_id id,
                                 wifi_interface_handle iface,
                                 wifi_channel_info channel_hint,
                                 unsigned max_duration_seconds,
                                 wifi_rtt_responder *responder_info);

  /**
   * Disable RTT responder mode.
   */
  wifi_error (*disable_responder)(wifi_request_id id,
                                  wifi_interface_handle iface);

} lowi_cb_table_t;

/*
  * This is a function pointer to a function that gets the table
  * of callback functions populated by LOWI and to be used by wifihal
  */
typedef lowi_cb_table_t* (getCbTable_t)();


struct lowi_wifi_info;
struct lowi_wifi_interface_info;
typedef struct lowi_wifi_info *lowi_wifi_handle;
typedef struct lowi_wifi_interface_info *lowi_wifi_interface_handle;

typedef void (*lowi_wifi_cleaned_up_handler) (lowi_wifi_handle handle);

wifi_error lowi_wifi_initialize(lowi_wifi_handle * handle);

void lowi_wifi_cleanup(lowi_wifi_handle handle, lowi_wifi_cleaned_up_handler handler);

void lowi_wifi_event_loop(lowi_wifi_handle handle);

wifi_error lowi_wifi_get_ifaces(lowi_wifi_handle handle, int *num, lowi_wifi_interface_handle **interfaces);

wifi_error lowi_wifi_get_iface_name(lowi_wifi_interface_handle handle, char *name, size_t size);

wifi_error lowi_wifi_get_rtt_capabilities(lowi_wifi_interface_handle iface,
        wifi_rtt_capabilities *capabilities);

wifi_error lowi_wifi_rtt_range_request(wifi_request_id id, lowi_wifi_interface_handle iface,
        unsigned num_rtt_config, wifi_rtt_config rtt_config[], wifi_rtt_event_handler handler);

wifi_error lowi_wifi_rtt_range_cancel(wifi_request_id id,  lowi_wifi_interface_handle iface,
        unsigned num_devices, mac_addr addr[]);

wifi_error lowi_wifi_rtt_get_responder_info(lowi_wifi_interface_handle iface,
        wifi_rtt_responder* responderInfo);

wifi_error lowi_wifi_enable_responder(wifi_request_id id, lowi_wifi_interface_handle iface,
	wifi_channel_info channel_hint, unsigned max_duration_seconds,
	wifi_rtt_responder* responderInfo);

wifi_error lowi_wifi_disable_responder(wifi_request_id id, lowi_wifi_interface_handle iface);
wifi_error lowi_wifi_set_lci(wifi_request_id id, lowi_wifi_interface_handle iface,
                                wifi_lci_information *lci);
wifi_error lowi_wifi_set_lcr(wifi_request_id id, lowi_wifi_interface_handle iface,
								wifi_lcr_information *lci);
#endif
