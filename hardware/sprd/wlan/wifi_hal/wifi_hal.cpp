/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <linux/rtnetlink.h>
#include <netpacket/packet.h>
#include <linux/filter.h>
#include <linux/errqueue.h>

#include <linux/pkt_sched.h>
#include <netlink/object-api.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink-private/object-api.h>
#include <netlink-private/types.h>

#include "nl80211_copy.h"

#include <dirent.h>
#include <net/if.h>
#include <netinet/in.h>
#include <cld80211_lib.h>

#include <sys/types.h>
#include <unistd.h>

#include "sync.h"

#define LOG_TAG  "WifiHAL"

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"
#include "ifaceeventhandler.h"
#include "wifiloggercmd.h"

/*
 BUGBUG: normally, libnl allocates ports for all connections it makes; but
 being a static library, it doesn't really know how many other netlink
 connections are made by the same process, if connections come from different
 shared libraries. These port assignments exist to solve that
 problem - temporarily. We need to fix libnl to try and allocate ports across
 the entire process.
 */

#define WIFI_HAL_CMD_SOCK_PORT       644
#define WIFI_HAL_EVENT_SOCK_PORT     645

#define MAX_HW_VER_LENGTH 100
/*
 * Defines for wifi_wait_for_driver_ready()
 * Specify durations between polls and max wait time
 */
#define POLL_DRIVER_DURATION_US (100000)
#define POLL_DRIVER_MAX_TIME_MS (10000)

static void internal_event_handler(wifi_handle handle, int events,
                                   struct nl_sock *sock);
static int internal_valid_message_handler(nl_msg *msg, void *arg);
static int user_sock_message_handler(nl_msg *msg, void *arg);
static int wifi_get_multicast_id(wifi_handle handle, const char *name,
        const char *group);
static int wifi_add_membership(wifi_handle handle, const char *group);
static wifi_error wifi_init_interfaces(wifi_handle handle);
static wifi_error wifi_set_packet_filter(wifi_interface_handle iface,
                                         const u8 *program, u32 len);
static wifi_error wifi_get_packet_filter_capabilities(wifi_interface_handle handle,
                                              u32 *version, u32 *max_len);
static wifi_error wifi_read_packet_filter(wifi_interface_handle handle,
                                   u32 src_offset, u8 *host_dst, u32 length);
static wifi_error wifi_configure_nd_offload(wifi_interface_handle iface,
                                            u8 enable);
wifi_error wifi_get_wake_reason_stats(wifi_interface_handle iface,
                             WLAN_DRIVER_WAKE_REASON_CNT *wifi_wake_reason_cnt);

/* Initialize/Cleanup */

wifi_interface_handle wifi_get_iface_handle(wifi_handle handle, char *name)
{
    hal_info *info = (hal_info *)handle;
    for (int i=0;i<info->num_interfaces;i++)
    {
        if (!strcmp(info->interfaces[i]->name, name))
        {
            return ((wifi_interface_handle )(info->interfaces)[i]);
        }
    }
    return NULL;
}

void wifi_socket_set_local_port(struct nl_sock *sock, uint32_t port)
{
    /* Release local port pool maintained by libnl and assign a own port
     * identifier to the socket.
     */
    nl_socket_set_local_port(sock, ((uint32_t)getpid() & 0x3FFFFFU) | (port << 22));
}

static nl_sock * wifi_create_nl_socket(int port, int protocol)
{
    // ALOGI("Creating socket");
    struct nl_sock *sock = nl_socket_alloc();
    if (sock == NULL) {
        ALOGE("Failed to create NL socket");
        return NULL;
    }

    wifi_socket_set_local_port(sock, port);

    if (nl_connect(sock, protocol)) {
        ALOGE("Could not connect handle");
        nl_socket_free(sock);
        return NULL;
    }

    return sock;
}

int ack_handler(struct nl_msg *msg, void *arg)
{
    int *err = (int *)arg;
    *err = 0;
    return NL_STOP;
}

int finish_handler(struct nl_msg *msg, void *arg)
{
    int *ret = (int *)arg;
    *ret = 0;
    return NL_SKIP;
}

int error_handler(struct sockaddr_nl *nla,
                  struct nlmsgerr *err, void *arg)
{
    int *ret = (int *)arg;
    *ret = err->error;

    ALOGV("%s invoked with error: %d", __func__, err->error);
    return NL_SKIP;
}
static int no_seq_check(struct nl_msg *msg, void *arg)
{
    return NL_OK;
}

static wifi_error acquire_supported_features(wifi_interface_handle iface,
        feature_set *set)
{
    wifi_error ret;
    interface_info *iinfo = getIfaceInfo(iface);
    wifi_handle handle = getWifiHandle(iface);
    *set = 0;

    WifihalGeneric supportedFeatures(handle, 0,
            OUI_QCA,
            QCA_NL80211_VENDOR_SUBCMD_GET_SUPPORTED_FEATURES);

    /* create the message */
    ret = supportedFeatures.create();
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    ret = supportedFeatures.set_iface_id(iinfo->name);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    ret = supportedFeatures.requestResponse();
    if (ret != WIFI_SUCCESS) {
        ALOGE("%s: requestResponse Error:%d",__func__, ret);
        goto cleanup;
    }

    supportedFeatures.getResponseparams(set);

cleanup:
    return ret;
}

static wifi_error acquire_driver_supported_features(wifi_interface_handle iface,
                                          features_info *driver_features)
{
    wifi_error ret;
    interface_info *iinfo = getIfaceInfo(iface);
    wifi_handle handle = getWifiHandle(iface);

    WifihalGeneric driverFeatures(handle, 0,
            OUI_QCA,
            QCA_NL80211_VENDOR_SUBCMD_GET_FEATURES);

    /* create the message */
    ret = driverFeatures.create();
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    ret = driverFeatures.set_iface_id(iinfo->name);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    ret = driverFeatures.requestResponse();
    if (ret != WIFI_SUCCESS) {
        ALOGE("%s: requestResponse Error:%d",__func__, ret);
        goto cleanup;
    }

    driverFeatures.getDriverFeatures(driver_features);

cleanup:
    return mapKernelErrortoWifiHalError(ret);
}

static wifi_error wifi_get_capabilities(wifi_interface_handle handle)
{
    wifi_error ret;
    int requestId;
    WifihalGeneric *wifihalGeneric;
    wifi_handle wifiHandle = getWifiHandle(handle);
    hal_info *info = getHalInfo(wifiHandle);

    if (!(info->supported_feature_set & WIFI_FEATURE_GSCAN)) {
        ALOGE("%s: GSCAN is not supported by driver", __FUNCTION__);
        return WIFI_ERROR_NOT_SUPPORTED;
    }

    /* No request id from caller, so generate one and pass it on to the driver.
     * Generate it randomly.
     */
    requestId = get_requestid();

    wifihalGeneric = new WifihalGeneric(
                            wifiHandle,
                            requestId,
                            OUI_QCA,
                            QCA_NL80211_VENDOR_SUBCMD_GSCAN_GET_CAPABILITIES);
    if (!wifihalGeneric) {
        ALOGE("%s: Failed to create object of WifihalGeneric class", __FUNCTION__);
        return WIFI_ERROR_OUT_OF_MEMORY;
    }

    ret = wifihalGeneric->wifiGetCapabilities(handle);

    delete wifihalGeneric;
    return ret;
}

static wifi_error get_firmware_bus_max_size_supported(
                                                wifi_interface_handle iface)
{
    wifi_error ret;
    interface_info *iinfo = getIfaceInfo(iface);
    wifi_handle handle = getWifiHandle(iface);
    hal_info *info = (hal_info *)handle;

    WifihalGeneric busSizeSupported(handle, 0,
                                    OUI_QCA,
                                    QCA_NL80211_VENDOR_SUBCMD_GET_BUS_SIZE);

    /* create the message */
    ret = busSizeSupported.create();
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    ret = busSizeSupported.set_iface_id(iinfo->name);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    ret = busSizeSupported.requestResponse();
    if (ret != WIFI_SUCCESS) {
        ALOGE("%s: requestResponse Error:%d", __FUNCTION__, ret);
        goto cleanup;
    }
    info->firmware_bus_max_size = busSizeSupported.getBusSize();

cleanup:
    return ret;
}

static wifi_error wifi_init_user_sock(hal_info *info)
{
    struct nl_sock *user_sock =
        wifi_create_nl_socket(WIFI_HAL_USER_SOCK_PORT, NETLINK_USERSOCK);
    if (user_sock == NULL) {
        ALOGE("Could not create diag sock");
        return WIFI_ERROR_UNKNOWN;
    }

    /* Set the socket buffer size */
    if (nl_socket_set_buffer_size(user_sock, (256*1024), 0) < 0) {
        ALOGE("Could not set size for user_sock: %s",
                   strerror(errno));
        /* continue anyway with the default (smaller) buffer */
    }
    else {
        ALOGV("nl_socket_set_buffer_size successful for user_sock");
    }

    struct nl_cb *cb = nl_socket_get_cb(user_sock);
    if (cb == NULL) {
        ALOGE("Could not get cb");
        return WIFI_ERROR_UNKNOWN;
    }

    info->user_sock_arg = 1;
    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &info->user_sock_arg);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &info->user_sock_arg);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &info->user_sock_arg);

    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, user_sock_message_handler, info);
    nl_cb_put(cb);

    int ret = nl_socket_add_membership(user_sock, 1);
    if (ret < 0) {
        ALOGE("Could not add membership");
        return WIFI_ERROR_UNKNOWN;
    }

    info->user_sock = user_sock;
    ALOGV("Initiialized diag sock successfully");
    return WIFI_SUCCESS;
}

#ifndef SPRD_PRODUCT_WIFI_DEVICE
static wifi_error wifi_init_cld80211_sock_cb(hal_info *info)
{
    struct nl_cb *cb = nl_socket_get_cb(info->cldctx->sock);
    if (cb == NULL) {
        ALOGE("Could not get cb");
        return WIFI_ERROR_UNKNOWN;
    }

    info->user_sock_arg = 1;
    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &info->user_sock_arg);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &info->user_sock_arg);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &info->user_sock_arg);

    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, user_sock_message_handler, info);
    nl_cb_put(cb);

    return WIFI_SUCCESS;
}
#endif

/*initialize function pointer table with Qualcomm HAL API*/
wifi_error init_wifi_vendor_hal_func_table(wifi_hal_fn *fn) {
    if (fn == NULL) {
        return WIFI_ERROR_UNKNOWN;
    }

    fn->wifi_initialize = wifi_initialize;
    fn->wifi_wait_for_driver_ready = wifi_wait_for_driver_ready;
    fn->wifi_cleanup = wifi_cleanup;
    fn->wifi_event_loop = wifi_event_loop;
    fn->wifi_get_supported_feature_set = wifi_get_supported_feature_set;
    fn->wifi_get_concurrency_matrix = wifi_get_concurrency_matrix;
    fn->wifi_set_scanning_mac_oui =  wifi_set_scanning_mac_oui;
    fn->wifi_get_ifaces = wifi_get_ifaces;
    fn->wifi_get_iface_name = wifi_get_iface_name;
    fn->wifi_set_iface_event_handler = wifi_set_iface_event_handler;
    fn->wifi_reset_iface_event_handler = wifi_reset_iface_event_handler;
    fn->wifi_start_gscan = wifi_start_gscan;
    fn->wifi_stop_gscan = wifi_stop_gscan;
    fn->wifi_get_cached_gscan_results = wifi_get_cached_gscan_results;
    fn->wifi_set_bssid_hotlist = wifi_set_bssid_hotlist;
    fn->wifi_reset_bssid_hotlist = wifi_reset_bssid_hotlist;
    fn->wifi_set_significant_change_handler = wifi_set_significant_change_handler;
    fn->wifi_reset_significant_change_handler = wifi_reset_significant_change_handler;
    fn->wifi_get_gscan_capabilities = wifi_get_gscan_capabilities;
    fn->wifi_set_link_stats = wifi_set_link_stats;
    fn->wifi_get_link_stats = wifi_get_link_stats;
    fn->wifi_clear_link_stats = wifi_clear_link_stats;
    fn->wifi_get_valid_channels = wifi_get_valid_channels;
    fn->wifi_rtt_range_request = wifi_rtt_range_request;
    fn->wifi_rtt_range_cancel = wifi_rtt_range_cancel;
    fn->wifi_get_rtt_capabilities = wifi_get_rtt_capabilities;
    fn->wifi_rtt_get_responder_info = wifi_rtt_get_responder_info;
    fn->wifi_enable_responder = wifi_enable_responder;
    fn->wifi_disable_responder = wifi_disable_responder;
    fn->wifi_set_nodfs_flag = wifi_set_nodfs_flag;
    fn->wifi_start_logging = wifi_start_logging;
    fn->wifi_set_epno_list = wifi_set_epno_list;
    fn->wifi_reset_epno_list = wifi_reset_epno_list;
    fn->wifi_set_country_code = wifi_set_country_code;
    fn->wifi_enable_tdls = wifi_enable_tdls;
    fn->wifi_disable_tdls = wifi_disable_tdls;
    fn->wifi_get_tdls_status = wifi_get_tdls_status;
    fn->wifi_get_tdls_capabilities = wifi_get_tdls_capabilities;
    fn->wifi_get_firmware_memory_dump = wifi_get_firmware_memory_dump;
    fn->wifi_set_log_handler = wifi_set_log_handler;
    fn->wifi_reset_log_handler = wifi_reset_log_handler;
    fn->wifi_set_alert_handler = wifi_set_alert_handler;
    fn->wifi_reset_alert_handler = wifi_reset_alert_handler;
    fn->wifi_get_firmware_version = wifi_get_firmware_version;
    fn->wifi_get_ring_buffers_status = wifi_get_ring_buffers_status;
    fn->wifi_get_logger_supported_feature_set = wifi_get_logger_supported_feature_set;
    fn->wifi_get_ring_data = wifi_get_ring_data;
    fn->wifi_get_driver_version = wifi_get_driver_version;
    fn->wifi_set_passpoint_list = wifi_set_passpoint_list;
    fn->wifi_reset_passpoint_list = wifi_reset_passpoint_list;
    fn->wifi_set_lci = wifi_set_lci;
    fn->wifi_set_lcr = wifi_set_lcr;
    fn->wifi_start_sending_offloaded_packet =
            wifi_start_sending_offloaded_packet;
    fn->wifi_stop_sending_offloaded_packet = wifi_stop_sending_offloaded_packet;
    fn->wifi_start_rssi_monitoring = wifi_start_rssi_monitoring;
    fn->wifi_stop_rssi_monitoring = wifi_stop_rssi_monitoring;
    fn->wifi_nan_enable_request = nan_enable_request;
    fn->wifi_nan_disable_request = nan_disable_request;
    fn->wifi_nan_publish_request = nan_publish_request;
    fn->wifi_nan_publish_cancel_request = nan_publish_cancel_request;
    fn->wifi_nan_subscribe_request = nan_subscribe_request;
    fn->wifi_nan_subscribe_cancel_request = nan_subscribe_cancel_request;
    fn->wifi_nan_transmit_followup_request = nan_transmit_followup_request;
    fn->wifi_nan_stats_request = nan_stats_request;
    fn->wifi_nan_config_request = nan_config_request;
    fn->wifi_nan_tca_request = nan_tca_request;
    fn->wifi_nan_beacon_sdf_payload_request = nan_beacon_sdf_payload_request;
    fn->wifi_nan_register_handler = nan_register_handler;
    fn->wifi_nan_get_version = nan_get_version;
    fn->wifi_set_packet_filter = wifi_set_packet_filter;
    fn->wifi_get_packet_filter_capabilities = wifi_get_packet_filter_capabilities;
    fn->wifi_read_packet_filter = wifi_read_packet_filter;
    fn->wifi_nan_get_capabilities = nan_get_capabilities;
    fn->wifi_nan_data_interface_create = nan_data_interface_create;
    fn->wifi_nan_data_interface_delete = nan_data_interface_delete;
    fn->wifi_nan_data_request_initiator = nan_data_request_initiator;
    fn->wifi_nan_data_indication_response = nan_data_indication_response;
    fn->wifi_nan_data_end = nan_data_end;
    fn->wifi_configure_nd_offload = wifi_configure_nd_offload;
    fn->wifi_get_driver_memory_dump = wifi_get_driver_memory_dump;
    fn->wifi_get_wake_reason_stats = wifi_get_wake_reason_stats;
    fn->wifi_start_pkt_fate_monitoring = wifi_start_pkt_fate_monitoring;
    fn->wifi_get_tx_pkt_fates = wifi_get_tx_pkt_fates;
    fn->wifi_get_rx_pkt_fates = wifi_get_rx_pkt_fates;
    fn->wifi_get_roaming_capabilities = wifi_get_roaming_capabilities;
    fn->wifi_configure_roaming = wifi_configure_roaming;
    fn->wifi_enable_firmware_roaming = wifi_enable_firmware_roaming;
    fn->wifi_select_tx_power_scenario = wifi_select_tx_power_scenario;
    fn->wifi_reset_tx_power_scenario = wifi_reset_tx_power_scenario;
    fn->wifi_set_radio_mode_change_handler = wifi_set_radio_mode_change_handler;
    fn->wifi_virtual_interface_create = wifi_virtual_interface_create;
    fn->wifi_virtual_interface_delete = wifi_virtual_interface_delete;
    fn->wifi_set_latency_mode = wifi_set_latency_mode;

    return WIFI_SUCCESS;
}

#ifndef SPRD_PRODUCT_WIFI_DEVICE
static void cld80211lib_cleanup(hal_info *info)
{
    if (!info->cldctx)
        return;
    cld80211_remove_mcast_group(info->cldctx, "host_logs");
    cld80211_remove_mcast_group(info->cldctx, "fw_logs");
    cld80211_remove_mcast_group(info->cldctx, "per_pkt_stats");
    cld80211_remove_mcast_group(info->cldctx, "diag_events");
    cld80211_remove_mcast_group(info->cldctx, "fatal_events");
    exit_cld80211_recv(info->cldctx);
    cld80211_deinit(info->cldctx);
    info->cldctx = NULL;
}
#endif

static int wifi_get_iface_id(hal_info *info, const char *iface)
{
    int i;
    for (i = 0; i < info->num_interfaces; i++)
        if (!strcmp(info->interfaces[i]->name, iface))
            return i;
    return -1;
}

wifi_error wifi_initialize(wifi_handle *handle)
{
    wifi_error ret = WIFI_SUCCESS;
    wifi_interface_handle iface_handle;
    struct nl_sock *cmd_sock = NULL;
    struct nl_sock *event_sock = NULL;
    struct nl_cb *cb = NULL;
    int status = 0;
    int index;
    char hw_ver_type[MAX_HW_VER_LENGTH];
    char *hw_name = NULL;

    ALOGI("Initializing wifi");
    hal_info *info = (hal_info *)malloc(sizeof(hal_info));
    if (info == NULL) {
        ALOGE("Could not allocate hal_info");
        return WIFI_ERROR_OUT_OF_MEMORY;
    }

    memset(info, 0, sizeof(*info));

    cmd_sock = wifi_create_nl_socket(WIFI_HAL_CMD_SOCK_PORT,
                                                     NETLINK_GENERIC);
    if (cmd_sock == NULL) {
        ALOGE("Failed to create command socket port");
        ret = WIFI_ERROR_UNKNOWN;
        goto unload;
    }

    /* Set the socket buffer size */
    if (nl_socket_set_buffer_size(cmd_sock, (256*1024), 0) < 0) {
        ALOGE("Could not set nl_socket RX buffer size for cmd_sock: %s",
                   strerror(errno));
        /* continue anyway with the default (smaller) buffer */
    }

    event_sock =
        wifi_create_nl_socket(WIFI_HAL_EVENT_SOCK_PORT, NETLINK_GENERIC);
    if (event_sock == NULL) {
        ALOGE("Failed to create event socket port");
        ret = WIFI_ERROR_UNKNOWN;
        goto unload;
    }

    /* Set the socket buffer size */
    if (nl_socket_set_buffer_size(event_sock, (256*1024), 0) < 0) {
        ALOGE("Could not set nl_socket RX buffer size for event_sock: %s",
                   strerror(errno));
        /* continue anyway with the default (smaller) buffer */
    }

    cb = nl_socket_get_cb(event_sock);
    if (cb == NULL) {
        ALOGE("Failed to get NL control block for event socket port");
        ret = WIFI_ERROR_UNKNOWN;
        goto unload;
    }

    info->event_sock_arg = 1;
    nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
    nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &info->event_sock_arg);
    nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &info->event_sock_arg);
    nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &info->event_sock_arg);

    nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, internal_valid_message_handler,
            info);
    nl_cb_put(cb);

    info->cmd_sock = cmd_sock;
    info->event_sock = event_sock;
    info->clean_up = false;
    info->in_event_loop = false;

    info->event_cb = (cb_info *)malloc(sizeof(cb_info) * DEFAULT_EVENT_CB_SIZE);
    if (info->event_cb == NULL) {
        ALOGE("Could not allocate event_cb");
        ret = WIFI_ERROR_OUT_OF_MEMORY;
        goto unload;
    }
    info->alloc_event_cb = DEFAULT_EVENT_CB_SIZE;
    info->num_event_cb = 0;

    info->nl80211_family_id = genl_ctrl_resolve(cmd_sock, "nl80211");
    if (info->nl80211_family_id < 0) {
        ALOGE("Could not resolve nl80211 familty id");
        ret = WIFI_ERROR_UNKNOWN;
        goto unload;
    }

    pthread_mutex_init(&info->cb_lock, NULL);
    pthread_mutex_init(&info->pkt_fate_stats_lock, NULL);

    *handle = (wifi_handle) info;

    wifi_add_membership(*handle, "scan");
    wifi_add_membership(*handle, "mlme");
    wifi_add_membership(*handle, "regulatory");
    wifi_add_membership(*handle, "vendor");

#ifndef SPRD_PRODUCT_WIFI_DEVICE
    info->cldctx = cld80211_init();
    if (info->cldctx != NULL) {
        info->user_sock = info->cldctx->sock;
        ret = wifi_init_cld80211_sock_cb(info);
        if (ret != WIFI_SUCCESS) {
            ALOGE("Could not set cb for CLD80211 family");
            goto cld80211_cleanup;
        }

        status = cld80211_add_mcast_group(info->cldctx, "host_logs");
        if (status) {
            ALOGE("Failed to add mcast group host_logs :%d", status);
            goto cld80211_cleanup;
        }
        status = cld80211_add_mcast_group(info->cldctx, "fw_logs");
        if (status) {
            ALOGE("Failed to add mcast group fw_logs :%d", status);
            goto cld80211_cleanup;
        }
        status = cld80211_add_mcast_group(info->cldctx, "per_pkt_stats");
        if (status) {
            ALOGE("Failed to add mcast group per_pkt_stats :%d", status);
            goto cld80211_cleanup;
        }
        status = cld80211_add_mcast_group(info->cldctx, "diag_events");
        if (status) {
            ALOGE("Failed to add mcast group diag_events :%d", status);
            goto cld80211_cleanup;
        }
        status = cld80211_add_mcast_group(info->cldctx, "fatal_events");
        if (status) {
            ALOGE("Failed to add mcast group fatal_events :%d", status);
            goto cld80211_cleanup;
        }
    } else
#endif
    {
        ret = wifi_init_user_sock(info);
        if (ret != WIFI_SUCCESS) {
            ALOGE("Failed to alloc user socket");
            goto unload;
        }
    }

    ret = wifi_init_interfaces(*handle);
    if (ret != WIFI_SUCCESS) {
        ALOGE("Failed to init interfaces");
        goto unload;
    }

    if (info->num_interfaces == 0) {
        ALOGE("No interfaces found");
        ret = WIFI_ERROR_UNINITIALIZED;
        goto unload;
    }

    index = wifi_get_iface_id(info, "wlan0");
    if (index == -1) {
        int i;
        for (i = 0; i < info->num_interfaces; i++)
        {
            free(info->interfaces[i]);
        }
        ALOGE("%s no iface with wlan0", __func__);
        goto unload;
    }
    iface_handle = (wifi_interface_handle)info->interfaces[index];

    ret = acquire_supported_features(iface_handle,
            &info->supported_feature_set);
    if (ret != WIFI_SUCCESS) {
        ALOGI("Failed to get supported feature set : %d", ret);
        //acquire_supported_features failure is acceptable condition as legacy
        //drivers might not support the required vendor command. So, do not
        //consider it as failure of wifi_initialize
        ret = WIFI_SUCCESS;
    }

    ret = acquire_driver_supported_features(iface_handle,
                                  &info->driver_supported_features);
    if (ret != WIFI_SUCCESS) {
        ALOGI("Failed to get vendor feature set : %d", ret);
        ret = WIFI_SUCCESS;
    }

    ret =  wifi_get_logger_supported_feature_set(iface_handle,
                         &info->supported_logger_feature_set);
    if (ret != WIFI_SUCCESS)
        ALOGE("Failed to get supported logger feature set: %d", ret);

    ret =  wifi_get_firmware_version(iface_handle, hw_ver_type,
                                     MAX_HW_VER_LENGTH);
    if (ret == WIFI_SUCCESS) {
        hw_name = strstr(hw_ver_type, "HW:");
        if (hw_name) {
            hw_name += strlen("HW:");
            if (strncmp(hw_name, "QCA6174", 7) == 0)
               info->pkt_log_ver = PKT_LOG_V1;
            else
               info->pkt_log_ver = PKT_LOG_V2;
        } else {
           info->pkt_log_ver = PKT_LOG_V0;
        }
        ALOGV("%s: hardware version type %d", __func__, info->pkt_log_ver);
    } else {
        ALOGE("Failed to get supported logger feature set: %d", ret);
    }

    ret = get_firmware_bus_max_size_supported(iface_handle);
    if (ret != WIFI_SUCCESS) {
        ALOGE("Failed to get supported bus size, error : %d", ret);
        info->firmware_bus_max_size = 1520;
    }

    ret = wifi_logger_ring_buffers_init(info);
    if (ret != WIFI_SUCCESS)
        ALOGE("Wifi Logger Ring Initialization Failed");

    ret = wifi_get_capabilities(iface_handle);
    if (ret != WIFI_SUCCESS)
        ALOGE("Failed to get wifi Capabilities, error: %d", ret);

    info->pkt_stats = (struct pkt_stats_s *)malloc(sizeof(struct pkt_stats_s));
    if (!info->pkt_stats) {
        ALOGE("%s: malloc Failed for size: %zu",
                __FUNCTION__, sizeof(struct pkt_stats_s));
        ret = WIFI_ERROR_OUT_OF_MEMORY;
        goto unload;
    }

    info->rx_buf_size_allocated = MAX_RXMPDUS_PER_AMPDU * MAX_MSDUS_PER_MPDU
                                  * PKT_STATS_BUF_SIZE;

    info->rx_aggr_pkts =
        (wifi_ring_buffer_entry  *)malloc(info->rx_buf_size_allocated);
    if (!info->rx_aggr_pkts) {
        ALOGE("%s: malloc Failed for size: %d",
                __FUNCTION__, info->rx_buf_size_allocated);
        ret = WIFI_ERROR_OUT_OF_MEMORY;
        info->rx_buf_size_allocated = 0;
        goto unload;
    }
    memset(info->rx_aggr_pkts, 0, info->rx_buf_size_allocated);

    info->exit_sockets[0] = -1;
    info->exit_sockets[1] = -1;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, info->exit_sockets) == -1) {
        ALOGE("Failed to create exit socket pair");
        ret = WIFI_ERROR_UNKNOWN;
        goto unload;
    }

    ALOGV("Initializing Gscan Event Handlers");
    ret = initializeGscanHandlers(info);
    if (ret != WIFI_SUCCESS) {
        ALOGE("Initializing Gscan Event Handlers Failed");
        goto unload;
    }

    ret = initializeRSSIMonitorHandler(info);
    if (ret != WIFI_SUCCESS) {
        ALOGE("Initializing RSSI Event Handler Failed");
        goto unload;
    }

    ALOGV("Initialized Wifi HAL Successfully; vendor cmd = %d Supported"
            " features : %x", NL80211_CMD_VENDOR, info->supported_feature_set);

#ifndef SPRD_PRODUCT_WIFI_DEVICE
cld80211_cleanup:
    if (status != 0 || ret != WIFI_SUCCESS) {
        ret = WIFI_ERROR_UNKNOWN;
        cld80211lib_cleanup(info);
    }
#endif
unload:
    if (ret != WIFI_SUCCESS) {
        if (cmd_sock)
            nl_socket_free(cmd_sock);
        if (event_sock)
            nl_socket_free(event_sock);
        if (info) {
            if (info->cldctx) {
#ifndef SPRD_PRODUCT_WIFI_DEVICE
                cld80211lib_cleanup(info);
#endif
            } else if (info->user_sock) {
                nl_socket_free(info->user_sock);
            }
            if (info->pkt_stats) free(info->pkt_stats);
            if (info->rx_aggr_pkts) free(info->rx_aggr_pkts);
            wifi_logger_ring_buffers_deinit(info);
            cleanupGscanHandlers(info);
            cleanupRSSIMonitorHandler(info);
            free(info->event_cb);
            if (info->driver_supported_features.flags) {
                free(info->driver_supported_features.flags);
                info->driver_supported_features.flags = NULL;
            }
            free(info);
        }
    }

    return ret;
}

wifi_error wifi_wait_for_driver_ready(void)
{
    // This function will wait to make sure basic client netdev is created
    // Function times out after 10 seconds
    int count = (POLL_DRIVER_MAX_TIME_MS * 1000) / POLL_DRIVER_DURATION_US;
    FILE *fd;

    do {
        if ((fd = fopen("/sys/class/net/wlan0", "r")) != NULL) {
            fclose(fd);
            return WIFI_SUCCESS;
        }
        usleep(POLL_DRIVER_DURATION_US);
    } while(--count > 0);

    ALOGE("Timed out wating on Driver ready ... ");
    return WIFI_ERROR_TIMED_OUT;
}

static int wifi_add_membership(wifi_handle handle, const char *group)
{
    hal_info *info = getHalInfo(handle);

    int id = wifi_get_multicast_id(handle, "nl80211", group);
    if (id < 0) {
        ALOGE("Could not find group %s", group);
        return id;
    }

    int ret = nl_socket_add_membership(info->event_sock, id);
    if (ret < 0) {
        ALOGE("Could not add membership to group %s", group);
    }

    return ret;
}

static void internal_cleaned_up_handler(wifi_handle handle)
{
    hal_info *info = getHalInfo(handle);
    wifi_cleaned_up_handler cleaned_up_handler = info->cleaned_up_handler;

    if (info->cmd_sock != 0) {
        nl_socket_free(info->cmd_sock);
        nl_socket_free(info->event_sock);
        info->cmd_sock = NULL;
        info->event_sock = NULL;
    }

    if (info->interfaces) {
        for (int i = 0; i < info->num_interfaces; i++)
            free(info->interfaces[i]);
        free(info->interfaces);
    }

    if (info->cldctx != NULL) {
#ifndef SPRD_PRODUCT_WIFI_DEVICE
        cld80211lib_cleanup(info);
#endif
    } else if (info->user_sock != 0) {
        nl_socket_free(info->user_sock);
        info->user_sock = NULL;
    }

    if (info->pkt_stats)
        free(info->pkt_stats);
    if (info->rx_aggr_pkts)
        free(info->rx_aggr_pkts);
    wifi_logger_ring_buffers_deinit(info);
    cleanupGscanHandlers(info);
    cleanupRSSIMonitorHandler(info);

    if (info->num_event_cb)
        ALOGE("%d events were leftover without being freed",
              info->num_event_cb);
    free(info->event_cb);

    if (info->exit_sockets[0] >= 0) {
        close(info->exit_sockets[0]);
        info->exit_sockets[0] = -1;
    }

    if (info->exit_sockets[1] >= 0) {
        close(info->exit_sockets[1]);
        info->exit_sockets[1] = -1;
    }

    if (info->pkt_fate_stats) {
        free(info->pkt_fate_stats);
        info->pkt_fate_stats = NULL;
    }

    if (info->driver_supported_features.flags) {
        free(info->driver_supported_features.flags);
        info->driver_supported_features.flags = NULL;
    }

    (*cleaned_up_handler)(handle);
    pthread_mutex_destroy(&info->cb_lock);
    pthread_mutex_destroy(&info->pkt_fate_stats_lock);
    free(info);
}

void wifi_cleanup(wifi_handle handle, wifi_cleaned_up_handler handler)
{
    if (!handle) {
        ALOGE("Handle is null");
        return;
    }

    hal_info *info = getHalInfo(handle);
    info->cleaned_up_handler = handler;
    info->clean_up = true;

    TEMP_FAILURE_RETRY(write(info->exit_sockets[0], "E", 1));
    ALOGI("Sent msg on exit sock to unblock poll()");
}

static int internal_pollin_handler(wifi_handle handle, struct nl_sock *sock)
{
    struct nl_cb *cb = nl_socket_get_cb(sock);

    int res = nl_recvmsgs(sock, cb);
    if(res)
        ALOGE("Error :%d while reading nl msg", res);
    nl_cb_put(cb);
    return res;
}

static void internal_event_handler(wifi_handle handle, int events,
                                   struct nl_sock *sock)
{
    if (events & POLLERR) {
        ALOGE("Error reading from socket");
        internal_pollin_handler(handle, sock);
    } else if (events & POLLHUP) {
        ALOGE("Remote side hung up");
    } else if (events & POLLIN) {
        //ALOGI("Found some events!!!");
        internal_pollin_handler(handle, sock);
    } else {
        ALOGE("Unknown event - %0x", events);
    }
}

/* Run event handler */
void wifi_event_loop(wifi_handle handle)
{
    hal_info *info = getHalInfo(handle);
    if (info->in_event_loop) {
        return;
    } else {
        info->in_event_loop = true;
    }

    pollfd pfd[3];
    memset(&pfd, 0, 3*sizeof(pfd[0]));

    pfd[0].fd = nl_socket_get_fd(info->event_sock);
    pfd[0].events = POLLIN;

    pfd[1].fd = nl_socket_get_fd(info->user_sock);
    pfd[1].events = POLLIN;

    pfd[2].fd = info->exit_sockets[1];
    pfd[2].events = POLLIN;

    /* TODO: Add support for timeouts */

    do {
        pfd[0].revents = 0;
        pfd[1].revents = 0;
        pfd[2].revents = 0;
        //ALOGI("Polling sockets");
        int result = poll(pfd, 3, -1);
        if (result < 0) {
            ALOGE("Error polling socket");
        } else {
            if (pfd[0].revents & (POLLIN | POLLHUP | POLLERR)) {
                internal_event_handler(handle, pfd[0].revents, info->event_sock);
            }
            if (pfd[1].revents & (POLLIN | POLLHUP | POLLERR)) {
                internal_event_handler(handle, pfd[1].revents, info->user_sock);
            }
        }
        rb_timerhandler(info);
    } while (!info->clean_up);
    internal_cleaned_up_handler(handle);
}

static int user_sock_message_handler(nl_msg *msg, void *arg)
{
    wifi_handle handle = (wifi_handle)arg;
    hal_info *info = getHalInfo(handle);

    diag_message_handler(info, msg);

    return NL_OK;
}

static int internal_valid_message_handler(nl_msg *msg, void *arg)
{
    wifi_handle handle = (wifi_handle)arg;
    hal_info *info = getHalInfo(handle);

    WifiEvent event(msg);
    int res = event.parse();
    if (res < 0) {
        ALOGE("Failed to parse event: %d", res);
        return NL_SKIP;
    }

    int cmd = event.get_cmd();
    uint32_t vendor_id = 0;
    int subcmd = 0;

    if (cmd == NL80211_CMD_VENDOR) {
        vendor_id = event.get_u32(NL80211_ATTR_VENDOR_ID);
        subcmd = event.get_u32(NL80211_ATTR_VENDOR_SUBCMD);
        /* Restrict printing GSCAN_FULL_RESULT which is causing lot
           of logs in bug report */
        if (subcmd != QCA_NL80211_VENDOR_SUBCMD_GSCAN_FULL_SCAN_RESULT) {
            ALOGI("event received %s, vendor_id = 0x%0x, subcmd = 0x%0x",
                  event.get_cmdString(), vendor_id, subcmd);
        }
    } else {
        ALOGV("event received %s", event.get_cmdString());
    }

    // event.log();

    bool dispatched = false;

    pthread_mutex_lock(&info->cb_lock);

    for (int i = 0; i < info->num_event_cb; i++) {
        if (cmd == info->event_cb[i].nl_cmd) {
            if (cmd == NL80211_CMD_VENDOR
                && ((vendor_id != info->event_cb[i].vendor_id)
                || (subcmd != info->event_cb[i].vendor_subcmd)))
            {
                /* event for a different vendor, ignore it */
                continue;
            }

            cb_info *cbi = &(info->event_cb[i]);
            pthread_mutex_unlock(&info->cb_lock);
            if (cbi->cb_func) {
                (*(cbi->cb_func))(msg, cbi->cb_arg);
                dispatched = true;
            }
            return NL_OK;
        }
    }

#ifdef QC_HAL_DEBUG
    if (!dispatched) {
        ALOGI("event ignored!!");
    }
#endif

    pthread_mutex_unlock(&info->cb_lock);
    return NL_OK;
}

////////////////////////////////////////////////////////////////////////////////

class GetMulticastIdCommand : public WifiCommand
{
private:
    const char *mName;
    const char *mGroup;
    int   mId;
public:
    GetMulticastIdCommand(wifi_handle handle, const char *name,
            const char *group) : WifiCommand(handle, 0)
    {
        mName = name;
        mGroup = group;
        mId = -1;
    }

    int getId() {
        return mId;
    }

    virtual wifi_error create() {
        int nlctrlFamily = genl_ctrl_resolve(mInfo->cmd_sock, "nlctrl");
        // ALOGI("ctrl family = %d", nlctrlFamily);
        wifi_error ret = mMsg.create(nlctrlFamily, CTRL_CMD_GETFAMILY, 0, 0);
        if (ret != WIFI_SUCCESS)
            return ret;

        ret = mMsg.put_string(CTRL_ATTR_FAMILY_NAME, mName);
        return ret;
    }

    virtual int handleResponse(WifiEvent& reply) {

        // ALOGI("handling reponse in %s", __func__);

        struct nlattr **tb = reply.attributes();
        struct nlattr *mcgrp = NULL;
        int i;

        if (!tb[CTRL_ATTR_MCAST_GROUPS]) {
            ALOGI("No multicast groups found");
            return NL_SKIP;
        } else {
            // ALOGI("Multicast groups attr size = %d",
            // nla_len(tb[CTRL_ATTR_MCAST_GROUPS]));
        }

        for_each_attr(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], i) {

            // ALOGI("Processing group");
            struct nlattr *tb2[CTRL_ATTR_MCAST_GRP_MAX + 1];
            nla_parse(tb2, CTRL_ATTR_MCAST_GRP_MAX, (nlattr *)nla_data(mcgrp),
                nla_len(mcgrp), NULL);
            if (!tb2[CTRL_ATTR_MCAST_GRP_NAME] || !tb2[CTRL_ATTR_MCAST_GRP_ID])
            {
                continue;
            }

            char *grpName = (char *)nla_data(tb2[CTRL_ATTR_MCAST_GRP_NAME]);
            int grpNameLen = nla_len(tb2[CTRL_ATTR_MCAST_GRP_NAME]);

            // ALOGI("Found group name %s", grpName);

            if (strncmp(grpName, mGroup, grpNameLen) != 0)
                continue;

            mId = nla_get_u32(tb2[CTRL_ATTR_MCAST_GRP_ID]);
            break;
        }

        return NL_SKIP;
    }

};

static int wifi_get_multicast_id(wifi_handle handle, const char *name,
        const char *group)
{
    GetMulticastIdCommand cmd(handle, name, group);
    int res = cmd.requestResponse();
    if (res < 0)
        return res;
    else
        return cmd.getId();
}

/////////////////////////////////////////////////////////////////////////

static bool is_wifi_interface(const char *name)
{
    if (strncmp(name, "wlan", 4) != 0 && strncmp(name, "p2p", 3) != 0) {
        /* not a wifi interface; ignore it */
        return false;
    } else {
        return true;
    }
}

static int get_interface(const char *name, interface_info *info)
{
    strlcpy(info->name, name, (IFNAMSIZ + 1));
    info->id = if_nametoindex(name);
    // ALOGI("found an interface : %s, id = %d", name, info->id);
    return WIFI_SUCCESS;
}

wifi_error wifi_init_interfaces(wifi_handle handle)
{
    hal_info *info = (hal_info *)handle;

    struct dirent *de;

    DIR *d = opendir("/sys/class/net");
    if (d == 0)
        return WIFI_ERROR_UNKNOWN;

    int n = 0;
    while ((de = readdir(d))) {
        if (de->d_name[0] == '.')
            continue;
        if (is_wifi_interface(de->d_name) ) {
            n++;
        }
    }

    closedir(d);

    d = opendir("/sys/class/net");
    if (d == 0)
        return WIFI_ERROR_UNKNOWN;

    info->interfaces = (interface_info **)malloc(sizeof(interface_info *) * n);
    if (info->interfaces == NULL) {
        ALOGE("%s: Error info->interfaces NULL", __func__);
        closedir(d);
        return WIFI_ERROR_OUT_OF_MEMORY;
    }

    int i = 0;
    while ((de = readdir(d))) {
        if (de->d_name[0] == '.')
            continue;
        if (is_wifi_interface(de->d_name)) {
            interface_info *ifinfo
                = (interface_info *)malloc(sizeof(interface_info));
            if (ifinfo == NULL) {
                ALOGE("%s: Error ifinfo NULL", __func__);
                while (i > 0) {
                    free(info->interfaces[i-1]);
                    i--;
                }
                free(info->interfaces);
                closedir(d);
                return WIFI_ERROR_OUT_OF_MEMORY;
            }
            if (get_interface(de->d_name, ifinfo) != WIFI_SUCCESS) {
                free(ifinfo);
                continue;
            }
            ifinfo->handle = handle;
            info->interfaces[i] = ifinfo;
            i++;
        }
    }

    closedir(d);

    info->num_interfaces = n;

    return WIFI_SUCCESS;
}

wifi_error wifi_get_ifaces(wifi_handle handle, int *num,
        wifi_interface_handle **interfaces)
{
    hal_info *info = (hal_info *)handle;

    *interfaces = (wifi_interface_handle *)info->interfaces;
    *num = info->num_interfaces;

    return WIFI_SUCCESS;
}

wifi_error wifi_get_iface_name(wifi_interface_handle handle, char *name,
        size_t size)
{
    interface_info *info = (interface_info *)handle;
    strlcpy(name, info->name, size);
    return WIFI_SUCCESS;
}

/* Get the supported Feature set */
wifi_error wifi_get_supported_feature_set(wifi_interface_handle iface,
        feature_set *set)
{
    int ret = 0;
    wifi_handle handle = getWifiHandle(iface);
    *set = 0;
    hal_info *info = getHalInfo(handle);

    ret = acquire_supported_features(iface, set);
    if (ret != WIFI_SUCCESS) {
        *set = info->supported_feature_set;
        ALOGV("Supported feature set acquired at initialization : %x", *set);
    } else {
        info->supported_feature_set = *set;
        ALOGV("Supported feature set acquired : %x", *set);
    }
    return WIFI_SUCCESS;
}

wifi_error wifi_get_concurrency_matrix(wifi_interface_handle handle,
                                       int set_size_max,
                                       feature_set set[], int *set_size)
{
    wifi_error ret;
    struct nlattr *nlData;
    WifihalGeneric *vCommand = NULL;
    interface_info *ifaceInfo = getIfaceInfo(handle);
    wifi_handle wifiHandle = getWifiHandle(handle);

    if (set == NULL) {
        ALOGE("%s: NULL set pointer provided. Exit.",
            __func__);
        return WIFI_ERROR_INVALID_ARGS;
    }

    vCommand = new WifihalGeneric(wifiHandle, 0,
            OUI_QCA,
            QCA_NL80211_VENDOR_SUBCMD_GET_CONCURRENCY_MATRIX);
    if (vCommand == NULL) {
        ALOGE("%s: Error vCommand NULL", __func__);
        return WIFI_ERROR_OUT_OF_MEMORY;
    }

    /* Create the message */
    ret = vCommand->create();
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    ret = vCommand->set_iface_id(ifaceInfo->name);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = vCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = vCommand->put_u32(
          QCA_WLAN_VENDOR_ATTR_GET_CONCURRENCY_MATRIX_CONFIG_PARAM_SET_SIZE_MAX,
          set_size_max);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    vCommand->attr_end(nlData);

    /* Populate the input received from caller/framework. */
    vCommand->setMaxSetSize(set_size_max);
    vCommand->setSizePtr(set_size);
    vCommand->setConcurrencySet(set);

    ret = vCommand->requestResponse();
    if (ret != WIFI_SUCCESS)
        ALOGE("%s: requestResponse() error: %d", __func__, ret);

cleanup:
    delete vCommand;
    if (ret)
        *set_size = 0;
    return ret;
}


wifi_error wifi_set_nodfs_flag(wifi_interface_handle handle, u32 nodfs)
{
    wifi_error ret;
    struct nlattr *nlData;
    WifiVendorCommand *vCommand = NULL;
    interface_info *ifaceInfo = getIfaceInfo(handle);
    wifi_handle wifiHandle = getWifiHandle(handle);

    vCommand = new WifiVendorCommand(wifiHandle, 0,
            OUI_QCA,
            QCA_NL80211_VENDOR_SUBCMD_NO_DFS_FLAG);
    if (vCommand == NULL) {
        ALOGE("%s: Error vCommand NULL", __func__);
        return WIFI_ERROR_OUT_OF_MEMORY;
    }

    /* Create the message */
    ret = vCommand->create();
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    ret = vCommand->set_iface_id(ifaceInfo->name);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = vCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    /* Add the fixed part of the mac_oui to the nl command */
    ret = vCommand->put_u32(QCA_WLAN_VENDOR_ATTR_SET_NO_DFS_FLAG, nodfs);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    vCommand->attr_end(nlData);

    ret = vCommand->requestResponse();
    /* Don't check response since we aren't expecting one */

cleanup:
    delete vCommand;
    return ret;
}

wifi_error wifi_start_sending_offloaded_packet(wifi_request_id id,
                                               wifi_interface_handle iface,
                                               u16 ether_type,
                                               u8 *ip_packet,
                                               u16 ip_packet_len,
                                               u8 *src_mac_addr,
                                               u8 *dst_mac_addr,
                                               u32 period_msec)
{
    wifi_error ret;
    struct nlattr *nlData;
    WifiVendorCommand *vCommand = NULL;

    ret = initialize_vendor_cmd(iface, id,
                                QCA_NL80211_VENDOR_SUBCMD_OFFLOADED_PACKETS,
                                &vCommand);
    if (ret != WIFI_SUCCESS) {
        ALOGE("%s: Initialization failed", __func__);
        return ret;
    }

    ALOGV("ether type 0x%04x\n", ether_type);
    ALOGV("ip packet length : %u\nIP Packet:", ip_packet_len);
    hexdump(ip_packet, ip_packet_len);
    ALOGV("Src Mac Address: " MAC_ADDR_STR "\nDst Mac Address: " MAC_ADDR_STR
          "\nPeriod in msec : %u", MAC_ADDR_ARRAY(src_mac_addr),
          MAC_ADDR_ARRAY(dst_mac_addr), period_msec);

    /* Add the vendor specific attributes for the NL command. */
    nlData = vCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = vCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_SENDING_CONTROL,
            QCA_WLAN_OFFLOADED_PACKETS_SENDING_START);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    ret = vCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_REQUEST_ID,
            id);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    ret = vCommand->put_u16(
            QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_ETHER_PROTO_TYPE,
            ether_type);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    ret = vCommand->put_bytes(
            QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_IP_PACKET_DATA,
            (const char *)ip_packet, ip_packet_len);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    ret = vCommand->put_addr(
            QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_SRC_MAC_ADDR,
            src_mac_addr);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    ret = vCommand->put_addr(
            QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_DST_MAC_ADDR,
            dst_mac_addr);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    ret = vCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_PERIOD,
            period_msec);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    vCommand->attr_end(nlData);

    ret = vCommand->requestResponse();
    if (ret != WIFI_SUCCESS)
        goto cleanup;

cleanup:
    delete vCommand;
    return ret;
}

wifi_error wifi_stop_sending_offloaded_packet(wifi_request_id id,
                                              wifi_interface_handle iface)
{
    wifi_error ret;
    struct nlattr *nlData;
    WifiVendorCommand *vCommand = NULL;

    ret = initialize_vendor_cmd(iface, id,
                                QCA_NL80211_VENDOR_SUBCMD_OFFLOADED_PACKETS,
                                &vCommand);
    if (ret != WIFI_SUCCESS) {
        ALOGE("%s: Initialization failed", __func__);
        return ret;
    }

    /* Add the vendor specific attributes for the NL command. */
    nlData = vCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = vCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_SENDING_CONTROL,
            QCA_WLAN_OFFLOADED_PACKETS_SENDING_STOP);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    ret = vCommand->put_u32(
            QCA_WLAN_VENDOR_ATTR_OFFLOADED_PACKETS_REQUEST_ID,
            id);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    vCommand->attr_end(nlData);

    ret = vCommand->requestResponse();
    if (ret != WIFI_SUCCESS)
        goto cleanup;

cleanup:
    delete vCommand;
    return ret;
}

#define PACKET_FILTER_ID 0

static wifi_error wifi_set_packet_filter(wifi_interface_handle iface,
                                         const u8 *program, u32 len)
{
    wifi_error ret;
    struct nlattr *nlData;
    WifiVendorCommand *vCommand = NULL;
    u32 current_offset = 0;
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    /* len=0 clears the filters in driver/firmware */
    if (len != 0 && program == NULL) {
        ALOGE("%s: No valid program provided. Exit.",
            __func__);
        return WIFI_ERROR_INVALID_ARGS;
    }

    do {
        ret = initialize_vendor_cmd(iface, get_requestid(),
                                    QCA_NL80211_VENDOR_SUBCMD_PACKET_FILTER,
                                    &vCommand);
        if (ret != WIFI_SUCCESS) {
            ALOGE("%s: Initialization failed", __FUNCTION__);
            return ret;
        }

        /* Add the vendor specific attributes for the NL command. */
        nlData = vCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
        if (!nlData)
            goto cleanup;

        ret = vCommand->put_u32(QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_SUB_CMD,
                                QCA_WLAN_SET_PACKET_FILTER);
        if (ret != WIFI_SUCCESS)
            goto cleanup;
        ret = vCommand->put_u32(QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_ID,
                                PACKET_FILTER_ID);
        if (ret != WIFI_SUCCESS)
            goto cleanup;
        ret = vCommand->put_u32(QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_SIZE,
                                len);
        if (ret != WIFI_SUCCESS)
            goto cleanup;
        ret = vCommand->put_u32(
                            QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_CURRENT_OFFSET,
                            current_offset);
        if (ret != WIFI_SUCCESS)
            goto cleanup;

        if (len) {
            ret = vCommand->put_bytes(
                                     QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_PROGRAM,
                                     (char *)&program[current_offset],
                                     min(info->firmware_bus_max_size,
                                     len-current_offset));
            if (ret!= WIFI_SUCCESS) {
                ALOGE("%s: failed to put program", __FUNCTION__);
                goto cleanup;
            }
        }

        vCommand->attr_end(nlData);

        ret = vCommand->requestResponse();
        if (ret != WIFI_SUCCESS) {
            ALOGE("%s: requestResponse Error:%d",__func__, ret);
            goto cleanup;
        }

        /* destroy the object after sending each fragment to driver */
        delete vCommand;
        vCommand = NULL;

        current_offset += min(info->firmware_bus_max_size, len);
    } while (current_offset < len);

    info->apf_enabled = !!len;

cleanup:
    if (vCommand)
        delete vCommand;
    return ret;
}

static wifi_error wifi_get_packet_filter_capabilities(
                wifi_interface_handle handle, u32 *version, u32 *max_len)
{
    wifi_error ret;
    struct nlattr *nlData;
    WifihalGeneric *vCommand = NULL;
    interface_info *ifaceInfo = getIfaceInfo(handle);
    wifi_handle wifiHandle = getWifiHandle(handle);

    if (version == NULL || max_len == NULL) {
        ALOGE("%s: NULL version/max_len pointer provided. Exit.",
            __FUNCTION__);
        return WIFI_ERROR_INVALID_ARGS;
    }

    vCommand = new WifihalGeneric(wifiHandle, 0,
            OUI_QCA,
            QCA_NL80211_VENDOR_SUBCMD_PACKET_FILTER);
    if (vCommand == NULL) {
        ALOGE("%s: Error vCommand NULL", __FUNCTION__);
        return WIFI_ERROR_OUT_OF_MEMORY;
    }

    /* Create the message */
    ret = vCommand->create();
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    ret = vCommand->set_iface_id(ifaceInfo->name);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    /* Add the vendor specific attributes for the NL command. */
    nlData = vCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = vCommand->put_u32(QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_SUB_CMD,
                            QCA_WLAN_GET_PACKET_FILTER);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    vCommand->attr_end(nlData);

    ret = vCommand->requestResponse();
    if (ret != WIFI_SUCCESS) {
        ALOGE("%s: requestResponse() error: %d", __FUNCTION__, ret);
        if (ret == WIFI_ERROR_NOT_SUPPORTED) {
            /* Packet filtering is not supported currently, so return version
             * and length as 0
             */
            ALOGI("Packet filtering is not supprted");
            *version = 0;
            *max_len = 0;
            ret = WIFI_SUCCESS;
        }
        goto cleanup;
    }

    *version = vCommand->getFilterVersion();
    *max_len = vCommand->getFilterLength();
cleanup:
    delete vCommand;
    return ret;
}


static wifi_error wifi_configure_nd_offload(wifi_interface_handle iface,
                                            u8 enable)
{
    wifi_error ret;
    struct nlattr *nlData;
    WifiVendorCommand *vCommand = NULL;

    ret = initialize_vendor_cmd(iface, get_requestid(),
                                QCA_NL80211_VENDOR_SUBCMD_ND_OFFLOAD,
                                &vCommand);
    if (ret != WIFI_SUCCESS) {
        ALOGE("%s: Initialization failed", __func__);
        return ret;
    }

    ALOGV("ND offload : %s", enable?"Enable":"Disable");

    /* Add the vendor specific attributes for the NL command. */
    nlData = vCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    ret = vCommand->put_u8(QCA_WLAN_VENDOR_ATTR_ND_OFFLOAD_FLAG, enable);
    if (ret != WIFI_SUCCESS)
        goto cleanup;

    vCommand->attr_end(nlData);

    ret = vCommand->requestResponse();

cleanup:
    delete vCommand;
    return ret;
}

/**
 * Copy 'len' bytes of raw data from host memory at source address 'program'
 * to APF (Android Packet Filter) working memory starting at offset 'dst_offset'.
 * The size of the program lenght passed to the interpreter is set to
 * 'progaram_lenght'
 *
 * The implementation is allowed to tranlate this wrtie into a series of smaller
 * writes,but this function is not allowed to return untill all write operations
 * have been completed
 * additionally visible memory not targeted by this function must remain
 * unchanged

 * @param dst_offset write offset in bytes relative to the beginning of the APF
 * working memory with logical address 0X000. Must be a multiple of 4
 *
 * @param program host memory to copy bytes from. Must be 4B aligned
 *
 * @param len the number of bytes to copy from the bost into the APF working
 * memory
 *
 * @param program_length new length of the program instructions in bytes to pass
 * to the interpreter
 */

wifi_error wifi_write_packet_filter(wifi_interface_handle iface,
                                         u32 dst_offset, const u8 *program,
                                         u32 len, u32 program_length)
{
    wifi_error ret;
    struct nlattr *nlData;
    WifiVendorCommand *vCommand = NULL;
    u32 current_offset = 0;
    wifi_handle wifiHandle = getWifiHandle(iface);
    hal_info *info = getHalInfo(wifiHandle);

    /* len=0 clears the filters in driver/firmware */
    if (len != 0 && program == NULL) {
        ALOGE("%s: No valid program provided. Exit.",
            __func__);
        return WIFI_ERROR_INVALID_ARGS;
    }

    do {
        ret = initialize_vendor_cmd(iface, get_requestid(),
                                    QCA_NL80211_VENDOR_SUBCMD_PACKET_FILTER,
                                    &vCommand);
        if (ret != WIFI_SUCCESS) {
            ALOGE("%s: Initialization failed", __FUNCTION__);
            return ret;
        }

        /* Add the vendor specific attributes for the NL command. */
        nlData = vCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
        if (!nlData)
             goto cleanup;

        ret = vCommand->put_u32(QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_SUB_CMD,
                                 QCA_WLAN_WRITE_PACKET_FILTER);
        if (ret != WIFI_SUCCESS)
            goto cleanup;
        ret = vCommand->put_u32(QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_ID,
                                PACKET_FILTER_ID);
        if (ret != WIFI_SUCCESS)
            goto cleanup;
        ret = vCommand->put_u32(QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_SIZE,
                                len);
        if (ret != WIFI_SUCCESS)
            goto cleanup;
        ret = vCommand->put_u32(
                            QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_CURRENT_OFFSET,
                            dst_offset + current_offset);
        if (ret != WIFI_SUCCESS)
            goto cleanup;
        ret = vCommand->put_u32(
                           QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_PROG_LENGTH,
                            program_length);
        if (ret != WIFI_SUCCESS)
            goto cleanup;

        ret = vCommand->put_bytes(
                                 QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_PROGRAM,
                                 (char *)&program[current_offset],
                                 min(info->firmware_bus_max_size,
                                 len - current_offset));
        if (ret!= WIFI_SUCCESS) {
            ALOGE("%s: failed to put program", __FUNCTION__);
            goto cleanup;
        }

        vCommand->attr_end(nlData);

        ret = vCommand->requestResponse();
       if (ret != WIFI_SUCCESS) {
            ALOGE("%s: requestResponse Error:%d",__func__, ret);
            goto cleanup;
        }

        /* destroy the object after sending each fragment to driver */
        delete vCommand;
        vCommand = NULL;

        current_offset += min(info->firmware_bus_max_size,
                                         len - current_offset);
    } while (current_offset < len);

cleanup:
    if (vCommand)
        delete vCommand;
    return ret;
}

wifi_error wifi_enable_packet_filter(wifi_interface_handle handle,
                                        u32 enable)
{
    wifi_error ret;
    struct nlattr *nlData;
    WifiVendorCommand *vCommand = NULL;
    u32 subcmd;
    wifi_handle wifiHandle = getWifiHandle(handle);
    hal_info *info = getHalInfo(wifiHandle);

    ret = initialize_vendor_cmd(handle, get_requestid(),
                                QCA_NL80211_VENDOR_SUBCMD_PACKET_FILTER,
                                &vCommand);

    if (ret != WIFI_SUCCESS) {
        ALOGE("%s: Initialization failed", __func__);
        return ret;
    }
    /* Add the vendor specific attributes for the NL command. */
    nlData = vCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
    if (!nlData)
        goto cleanup;

    subcmd = enable ? QCA_WLAN_ENABLE_PACKET_FILTER :
                      QCA_WLAN_DISABLE_PACKET_FILTER;
    ret = vCommand->put_u32(QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_SUB_CMD,
                            subcmd);
    if (ret != WIFI_SUCCESS)
            goto cleanup;

    vCommand->attr_end(nlData);
    ret = vCommand->requestResponse();

    if (ret != WIFI_SUCCESS) {
        ALOGE("%s: requestResponse() error: %d", __FUNCTION__, ret);
        goto cleanup;
    }

    info->apf_enabled = !!enable;

cleanup:
    /*
     * coverity bug1159023:
     * vCommand alway be allocated as initialize_vendor_cmd() return WIFI_SUCCESS.
     * if (vCommand)
     */
        delete vCommand;
    return ret;

}

/**
 * Copy 'length' bytes of raw data from APF (Android Packet Filter) working
 * memory  to host memory starting at offset src_offset into host memory
 * pointed to by host_dst.
 * Memory can be text, data or some combination of the two. The implementiion is
 * allowed to translate this read into a series of smaller reads, but this
 * function is not allowed to return untill all the reads operations
 * into host_dst have been completed.
 *
 * @param src_offset offset in bytes of destination memory within APF working
 * memory
 *
 * @param host_dst host memory to copy into. Must be 4B aligned.
 *
 * @param length the number of bytes to copy from the APF working memory to the
 * host.
 */

static wifi_error wifi_read_packet_filter(wifi_interface_handle handle,
                                          u32 src_offset, u8 *host_dst, u32 length)
{
    wifi_error ret = WIFI_SUCCESS;
    struct nlattr *nlData;
    WifihalGeneric *vCommand = NULL;
    interface_info *ifaceInfo = getIfaceInfo(handle);
    wifi_handle wifiHandle = getWifiHandle(handle);
    hal_info *info = getHalInfo(wifiHandle);

    /* Length to be passed to this function should be non-zero
     * Return invalid argument if length is passed as zero
     */
    if (length == 0)
        return  WIFI_ERROR_INVALID_ARGS;

    /*Temporary varibles to support the read complete length in chunks */
    u8 *temp_host_dst;
    u32 remainingLengthToBeRead, currentLength;
    u8 apf_locally_disabled = 0;

    /*Initializing the temporary variables*/
    temp_host_dst = host_dst;
    remainingLengthToBeRead = length;

    if (info->apf_enabled) {
        /* Disable APF only when not disabled by framework before calling
         * wifi_read_packet_filter()
         */
        ret = wifi_enable_packet_filter(handle, 0);
        if (ret != WIFI_SUCCESS) {
            ALOGE("%s: Failed to disable APF", __FUNCTION__);
            return ret;
        }
        apf_locally_disabled = 1;
    }
    /**
     * Read the complete length in chunks of size less or equal to firmware bus
     * max size
     */
    while (remainingLengthToBeRead)
    {
        vCommand = new WifihalGeneric(wifiHandle, 0, OUI_QCA,
                                      QCA_NL80211_VENDOR_SUBCMD_PACKET_FILTER);

        if (vCommand == NULL) {
            ALOGE("%s: Error vCommand NULL", __FUNCTION__);
            ret = WIFI_ERROR_OUT_OF_MEMORY;
            break;
        }

        /* Create the message */
        ret = vCommand->create();
        if (ret != WIFI_SUCCESS)
            break;
        ret = vCommand->set_iface_id(ifaceInfo->name);
        if (ret != WIFI_SUCCESS)
            break;
        /* Add the vendor specific attributes for the NL command. */
        nlData = vCommand->attr_start(NL80211_ATTR_VENDOR_DATA);
        if (!nlData)
            break;
        ret = vCommand->put_u32(QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_SUB_CMD,
                                QCA_WLAN_READ_PACKET_FILTER);
        if (ret != WIFI_SUCCESS)
            break;

        currentLength = min(remainingLengthToBeRead, info->firmware_bus_max_size);

        ret = vCommand->put_u32(QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_SIZE,
                                currentLength);
        if (ret != WIFI_SUCCESS)
            break;
        ret = vCommand->put_u32(QCA_WLAN_VENDOR_ATTR_PACKET_FILTER_CURRENT_OFFSET,
                                src_offset);
        if (ret != WIFI_SUCCESS)
            break;

        vCommand->setPacketBufferParams(temp_host_dst, currentLength);
        vCommand->attr_end(nlData);
        ret = vCommand->requestResponse();

        if (ret != WIFI_SUCCESS) {
            ALOGE("%s: requestResponse() error: %d current_len = %u, src_offset = %u",
                  __FUNCTION__, ret, currentLength, src_offset);
            break;
        }

        remainingLengthToBeRead -= currentLength;
        temp_host_dst += currentLength;
        src_offset += currentLength;
        delete vCommand;
        vCommand = NULL;
    }

    /* Re enable APF only when disabled above within this API */
    if (apf_locally_disabled) {
        wifi_error status;
        status = wifi_enable_packet_filter(handle, 1);
        if (status != WIFI_SUCCESS)
            ALOGE("%s: Failed to enable APF", __FUNCTION__);
        /* Prefer to return read status if read fails */
        if (ret == WIFI_SUCCESS)
            ret = status;
    }

    delete vCommand;
    return ret;
}
