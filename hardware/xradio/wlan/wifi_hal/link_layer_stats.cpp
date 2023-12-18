/*
 * Copyright (C) 2017 The Android Open Source Project
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
#include <netlink/handlers.h>

#include "sync.h"

#define LOG_TAG  "WifiHAL"

#include <utils/Log.h>

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"

/* Internal radio statistics structure in the driver */
typedef struct {
	wifi_radio radio;
	uint32_t on_time;
	uint32_t tx_time;
	uint32_t rx_time;
	uint32_t on_time_scan;
	uint32_t on_time_nbd;
	uint32_t on_time_gscan;
	uint32_t on_time_roam_scan;
	uint32_t on_time_pno_scan;
	uint32_t on_time_hs20;
	uint32_t num_channels;
	wifi_channel_stat channels[];
} wifi_radio_stat_internal;

enum {
    LSTATS_SUBCMD_GET_INFO = ANDROID_NL80211_SUBCMD_LSTATS_RANGE_START,
	LSTATS_SUBCMD_SET_INFO,
	LSTATS_SUBCMD_CLEAR_INFO,
};

class GetLinkStatsCommand : public WifiCommand
{
    wifi_stats_result_handler mHandler;
public:
    GetLinkStatsCommand(wifi_interface_handle iface, wifi_stats_result_handler handler)
        : WifiCommand("GetLinkStatsCommand", iface, 0), mHandler(handler)
    { }

    virtual int create() {
        // ALOGI("Creating message to get link statistics; iface = %d", mIfaceInfo->id);

        int ret = mMsg.create(GOOGLE_OUI, LSTATS_SUBCMD_GET_INFO);
        if (ret < 0) {
            ALOGV("Failed to create %x - %d", LSTATS_SUBCMD_GET_INFO, ret);
            return ret;
        }

        return ret;
    }

protected:
    virtual int handleResponse(WifiEvent& reply) {

        // ALOGI("In GetLinkStatsCommand::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        int id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();

        // ALOGI("Id = %0x, subcmd = %d", id, subcmd);

        void *data = reply.get_vendor_data();
        int len = reply.get_vendor_data_len();
        wifi_radio_stat *radio_stat = (wifi_radio_stat *)data;
        if (!radio_stat) {
            ALOGV("Invalid stats pointer received");
            return NL_SKIP;
        }
		ALOGV("radio: = %d", radio_stat->radio);
        ALOGV("on_time: = %u ms", radio_stat->on_time);
        ALOGV("tx_time: = %u ms", radio_stat->tx_time);
		ALOGV("num_tx_levels: = %u", radio_stat->num_tx_levels);
		radio_stat->tx_time_per_levels = (u32*)((char*)data + sizeof(wifi_radio_stat) + sizeof(wifi_iface_stat));
		ALOGV("tx_time_per_levels: = %u ms", radio_stat->tx_time_per_levels[0]);
        ALOGV("rx_time: = %u ms", radio_stat->rx_time);
        ALOGV("on_time_scan: = %u ms", radio_stat->on_time_scan);
        ALOGV("on_time_nbd: = %u ms", radio_stat->on_time_nbd);
        ALOGV("on_time_gscan: = %u ms", radio_stat->on_time_gscan);
        ALOGV("on_time_pno_scan: = %u ms", radio_stat->on_time_pno_scan);
        ALOGV("on_time_hs20: = %u ms", radio_stat->on_time_hs20);
        if (radio_stat->num_channels > 11) {
            ALOGV("Incorrect number of channels = %d", radio_stat->num_channels);
            // dump data before num_channels
            ALOGV("radio: = %d", radio_stat->radio);
            ALOGV("on_time: = %u ms", radio_stat->on_time);
            ALOGV("tx_time: = %u ms", radio_stat->tx_time);
            ALOGV("rx_time: = %u ms", radio_stat->rx_time);
            ALOGV("on_time_scan: = %u ms", radio_stat->on_time_scan);
            ALOGV("on_time_nbd: = %u ms", radio_stat->on_time_nbd);
            ALOGV("on_time_gscan: = %u ms", radio_stat->on_time_gscan);
            ALOGV("on_time_pno_scan: = %u ms", radio_stat->on_time_pno_scan);
            ALOGV("on_time_hs20: = %u ms", radio_stat->on_time_hs20);
            free(radio_stat);
            return NL_SKIP;
        }
 		wifi_iface_stat *iface_stat = NULL;
 		iface_stat = (wifi_iface_stat *)((char* )data + sizeof(wifi_radio_stat));

		if(*mHandler.on_link_stats_results == NULL) {
			ALOGV("*mHandler.on_link_stats_results is NULL");
		} else {
        	(*mHandler.on_link_stats_results)(id, iface_stat, 1, radio_stat);
		}
        //free(radio_stat);
        return NL_OK;
    }

};

class SetLinkStatsCommand : public WifiCommand
{
    wifi_stats_result_handler mHandler;
public:
    SetLinkStatsCommand(wifi_interface_handle iface)
        : WifiCommand("SetLinkStatsCommand", iface, 0)
    { }

    virtual int create() {
        // ALOGI("Creating message to get link statistics; iface = %d", mIfaceInfo->id);

        int ret = mMsg.create(GOOGLE_OUI, LSTATS_SUBCMD_SET_INFO);
        if (ret < 0) {
            ALOGV("Failed to create %x - %d", LSTATS_SUBCMD_SET_INFO, ret);
            return ret;
        }

        return ret;
    }

protected:
    virtual int handleResponse(WifiEvent& reply) {

        // ALOGI("In GetLinkStatsCommand::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        int id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();

        // ALOGI("Id = %0x, subcmd = %d", id, subcmd);

        void *data = reply.get_vendor_data();
        int len = reply.get_vendor_data_len();
         return NL_OK;
    }

};

class ClearLinkStatsCommand : public WifiCommand
{
    wifi_stats_result_handler mHandler;
public:
    ClearLinkStatsCommand(wifi_interface_handle iface)
        : WifiCommand("ClearLinkStatsCommand", iface, 0)
    { }

    virtual int create() {
        // ALOGI("Creating message to get link statistics; iface = %d", mIfaceInfo->id);

        int ret = mMsg.create(GOOGLE_OUI, LSTATS_SUBCMD_CLEAR_INFO);
        if (ret < 0) {
            ALOGV("Failed to create %x - %d", LSTATS_SUBCMD_CLEAR_INFO, ret);
            return ret;
        }

        return ret;
    }

protected:
    virtual int handleResponse(WifiEvent& reply) {

        // ALOGI("In GetLinkStatsCommand::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        int id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();

        // ALOGI("Id = %0x, subcmd = %d", id, subcmd);

        void *data = reply.get_vendor_data();
        int len = reply.get_vendor_data_len();
         return NL_OK;
    }

};

wifi_error wifi_get_link_stats(wifi_request_id id,
        wifi_interface_handle iface, wifi_stats_result_handler handler)
{
    GetLinkStatsCommand command(iface, handler);
    return (wifi_error) command.requestResponse();
}

wifi_error wifi_set_link_stats(
        wifi_interface_handle iface, wifi_link_layer_params params)
{

	SetLinkStatsCommand command(iface);
    return (wifi_error) command.requestResponse();
    //return WIFI_SUCCESS;
}

wifi_error wifi_clear_link_stats(wifi_interface_handle iface,
      u32 stats_clear_req_mask, u32 *stats_clear_rsp_mask, u8 stop_req, u8 *stop_rsp)
{

	ClearLinkStatsCommand command(iface);
    return (wifi_error) command.requestResponse();
    //return WIFI_SUCCESS;
}
