/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Portions copyright (C) 2017 Broadcom Limited
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
#include <errno.h>

#include <linux/pkt_sched.h>
#include <netlink/object-api.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink-private/object-api.h>
#include <netlink-private/types.h>
#include <unistd.h>
#include <cutils/properties.h>


#include "nl80211_copy.h"
#include "sync.h"

#define LOG_TAG  "WifiHAL"

#include <log/log.h>

#include "wifi_hal.h"
#include "common.h"
#include "cpp_bindings.h"
#include <sys/stat.h>
#include "brcm_version.h"
#define WIFI_HAL_EVENT_SOCK_PORT     645

#define ARRAYSIZE(a)	(u8)(sizeof(a) / sizeof(a[0]))
typedef enum {
    LOGGER_START_LOGGING = ANDROID_NL80211_SUBCMD_DEBUG_RANGE_START,
    LOGGER_TRIGGER_MEM_DUMP,
    LOGGER_GET_MEM_DUMP,
    LOGGER_GET_VER,
    LOGGER_GET_RING_STATUS,
    LOGGER_GET_RING_DATA,
    LOGGER_GET_FEATURE,
    LOGGER_RESET_LOGGING,
    LOGGER_TRIGGER_DRIVER_MEM_DUMP,
    LOGGER_GET_DRIVER_MEM_DUMP,
    LOGGER_START_PKT_FATE_MONITORING,
    LOGGER_GET_TX_PKT_FATES,
    LOGGER_GET_RX_PKT_FATES,
    LOGGER_GET_WAKE_REASON_STATS,
    LOGGER_DEBUG_GET_DUMP,
    LOGGER_FILE_DUMP_DONE_IND,
    LOGGER_SET_HAL_START,
    LOGGER_HAL_STOP,
    LOGGER_SET_HAL_PID
} DEBUG_SUB_COMMAND;

#define MAX_NV_FILE 4
#define MAX_SKU_NAME_LEN 5
#define OTA_PATH "/data/vendor/firmware/wifi/"
#define OTA_CLM_FILE "bcmdhd_clm.blob"
#define OTA_NVRAM_FILE "bcmdhd.cal"
#define HW_DEV_PROP "ro.revision"
#define HW_SKU_PROP "ro.boot.hardware.sku"

typedef enum {
    NVRAM,
    CLM_BLOB
} OTA_TYPE;

char ota_nvram_ext[10];
typedef struct ota_info_buf {
    u32 ota_clm_len;
    const void *ota_clm_buf[1];
    u32 ota_nvram_len;
    const void *ota_nvram_buf[1];
} ota_info_buf_t;
u32 applied_ota_version = 0;

typedef enum {
    LOGGER_ATTRIBUTE_INVALID			= 0,
    LOGGER_ATTRIBUTE_DRIVER_VER			= 1,
    LOGGER_ATTRIBUTE_FW_VER			= 2,
    LOGGER_ATTRIBUTE_RING_ID			= 3,
    LOGGER_ATTRIBUTE_RING_NAME			= 4,
    LOGGER_ATTRIBUTE_RING_FLAGS			= 5,
    LOGGER_ATTRIBUTE_LOG_LEVEL			= 6,
    LOGGER_ATTRIBUTE_LOG_TIME_INTVAL		= 7,
    LOGGER_ATTRIBUTE_LOG_MIN_DATA_SIZE		= 8,
    LOGGER_ATTRIBUTE_FW_DUMP_LEN		= 9,
    LOGGER_ATTRIBUTE_FW_DUMP_DATA		= 10,
    LOGGER_ATTRIBUTE_FW_ERR_CODE		= 11,
    LOGGER_ATTRIBUTE_RING_DATA			= 12,
    LOGGER_ATTRIBUTE_RING_STATUS		= 13,
    LOGGER_ATTRIBUTE_RING_NUM			= 14,
    LOGGER_ATTRIBUTE_DRIVER_DUMP_LEN		= 15,
    LOGGER_ATTRIBUTE_DRIVER_DUMP_DATA		= 16,
    LOGGER_ATTRIBUTE_PKT_FATE_NUM		= 17,
    LOGGER_ATTRIBUTE_PKT_FATE_DATA		= 18,
    LOGGER_ATTRIBUTE_HANG_REASON		= 19,
    /* Add new attributes just above this */
    LOGGER_ATTRIBUTE_MAX
} LOGGER_ATTRIBUTE;

typedef enum {
    DEBUG_OFF = 0,
    DEBUG_NORMAL,
    DEBUG_VERBOSE,
    DEBUG_VERY,
    DEBUG_VERY_VERY,
} LOGGER_LEVEL;

typedef enum {
    GET_FW_VER,
    GET_DRV_VER,
    GET_RING_DATA,
    GET_RING_STATUS,
    GET_FEATURE,
    START_RING_LOG,
} GetCmdType;

typedef enum {
    PACKET_MONITOR_START,
    TX_PACKET_FATE,
    RX_PACKET_FATE,
} PktFateReqType;

enum wake_stat_attributes {
    WAKE_STAT_ATTRIBUTE_INVALID,
    WAKE_STAT_ATTRIBUTE_TOTAL,
    WAKE_STAT_ATTRIBUTE_WAKE,
    WAKE_STAT_ATTRIBUTE_COUNT,
    WAKE_STAT_ATTRIBUTE_CMD_COUNT_USED,
    WAKE_STAT_ATTRIBUTE_TOTAL_DRIVER_FW,
    WAKE_STAT_ATTRIBUTE_DRIVER_FW_WAKE,
    WAKE_STAT_ATTRIBUTE_DRIVER_FW_COUNT,
    WAKE_STAT_ATTRIBUTE_DRIVER_FW_COUNT_USED,
    WAKE_STAT_ATTRIBUTE_TOTAL_RX_DATA_WAKE,
    WAKE_STAT_ATTRIBUTE_RX_UNICAST_COUNT,
    WAKE_STAT_ATTRIBUTE_RX_MULTICAST_COUNT,
    WAKE_STAT_ATTRIBUTE_RX_BROADCAST_COUNT,
    WAKE_STAT_ATTRIBUTE_RX_ICMP_PKT,
    WAKE_STAT_ATTRIBUTE_RX_ICMP6_PKT,
    WAKE_STAT_ATTRIBUTE_RX_ICMP6_RA,
    WAKE_STAT_ATTRIBUTE_RX_ICMP6_NA,
    WAKE_STAT_ATTRIBUTE_RX_ICMP6_NS,
    WAKE_STAT_ATTRIBUTE_IPV4_RX_MULTICAST_ADD_CNT,
    WAKE_STAT_ATTRIBUTE_IPV6_RX_MULTICAST_ADD_CNT,
    WAKE_STAT_ATTRIBUTE_OTHER_RX_MULTICAST_ADD_CNT,
    WAKE_STAT_ATTRIBUTE_RX_MULTICAST_PKT_INFO,
    WAKE_STAT_ATTRIBUTE_MAX
};

typedef enum {
    SET_HAL_START_ATTRIBUTE_DEINIT = 0x0001,
    SET_HAL_START_ATTRIBUTE_PRE_INIT = 0x0002,
    SET_HAL_START_ATTRIBUTE_EVENT_SOCK_PID = 0x0003
} SET_HAL_START_ATTRIBUTE;

typedef enum {
    OTA_DOWNLOAD_CLM_LENGTH_ATTR    = 0x0001,
    OTA_DOWNLOAD_CLM_ATTR           = 0x0002,
    OTA_DOWNLOAD_NVRAM_LENGTH_ATTR  = 0x0003,
    OTA_DOWNLOAD_NVRAM_ATTR         = 0x0004,
    OTA_SET_FORCE_REG_ON            = 0x0005,
    OTA_CUR_NVRAM_EXT_ATTR          = 0x0006,
} OTA_DOWNLOAD_ATTRIBUTE;

#define HAL_START_REQUEST_ID 2
#define HAL_RESTART_ID 3
#define FILE_NAME_LEN 256
///////////////////////////////////////////////////////////////////////////////
class DebugCommand : public WifiCommand
{
    char *mBuff;
    int *mBuffSize;
    u32 *mNumRings;
    wifi_ring_buffer_status *mStatus;
    unsigned int *mSupport;
    u32 mVerboseLevel;
    u32 mFlags;
    u32 mMaxIntervalSec;
    u32 mMinDataSize;
    char *mRingName;
    GetCmdType mType;

public:

    // constructor for get version
    DebugCommand(wifi_interface_handle iface, char *buffer, int *buffer_size,
            GetCmdType cmdType)
        : WifiCommand("DebugCommand", iface, 0), mBuff(buffer), mBuffSize(buffer_size), mType
        (cmdType)
    {
        mNumRings =  NULL;
        mStatus = NULL;
        mSupport = NULL;
        mVerboseLevel = 0;
        mFlags = 0;
        mMaxIntervalSec = 0;
        mMinDataSize = 0;
        mRingName = NULL;
        memset(mBuff, 0, *mBuffSize);
    }

    // constructor for ring data
    DebugCommand(wifi_interface_handle iface, char *ring_name, GetCmdType cmdType)
        : WifiCommand("DebugCommand", iface, 0), mRingName(ring_name), mType(cmdType)
    {
        mBuff = NULL;
        mBuffSize = NULL;
        mNumRings =  NULL;
        mStatus = NULL;
        mSupport = NULL;
        mVerboseLevel = 0;
        mFlags = 0;
        mMaxIntervalSec = 0;
        mMinDataSize = 0;
    }

    // constructor for ring status
    DebugCommand(wifi_interface_handle iface, u32 *num_rings,
            wifi_ring_buffer_status *status, GetCmdType cmdType)
        : WifiCommand("DebugCommand", iface, 0), mNumRings(num_rings), mStatus(status), mType(cmdType)
    {
        mBuff = NULL;
        mBuffSize = NULL;
        mSupport = NULL;
        mVerboseLevel = 0;
        mFlags = 0;
        mMaxIntervalSec = 0;
        mMinDataSize = 0;
        mRingName = NULL;
        memset(mStatus, 0, sizeof(wifi_ring_buffer_status) * (*mNumRings));
    }

    // constructor for feature set
    DebugCommand(wifi_interface_handle iface, unsigned int *support, GetCmdType cmdType)
        : WifiCommand("DebugCommand", iface, 0), mSupport(support), mType(cmdType)
    {
        mBuff = NULL;
        mBuffSize = NULL;
        mNumRings =  NULL;
        mStatus = NULL;
        mVerboseLevel = 0;
        mFlags = 0;
        mMaxIntervalSec = 0;
        mMinDataSize = 0;
        mRingName = NULL;
    }

    // constructor for ring params
    DebugCommand(wifi_interface_handle iface, u32 verbose_level, u32 flags,
            u32 max_interval_sec, u32 min_data_size, char *ring_name, GetCmdType cmdType)
        : WifiCommand("DebugCommand", iface, 0), mVerboseLevel(verbose_level), mFlags(flags),
        mMaxIntervalSec(max_interval_sec), mMinDataSize(min_data_size),
        mRingName(ring_name), mType(cmdType)
    {
        mBuff = NULL;
        mBuffSize = NULL;
        mNumRings =  NULL;
        mStatus = NULL;
        mSupport = NULL;
    }

    int createRingRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, LOGGER_START_LOGGING);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to create start ring logger request; result = %d", result);
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);

        result = request.put_u32(LOGGER_ATTRIBUTE_LOG_LEVEL, mVerboseLevel);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to put log level; result = %d", result);
            return result;
        }
        result = request.put_u32(LOGGER_ATTRIBUTE_RING_FLAGS, mFlags);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to put ring flags; result = %d", result);
            return result;
        }
        result = request.put_u32(LOGGER_ATTRIBUTE_LOG_TIME_INTVAL, mMaxIntervalSec);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to put log time interval; result = %d", result);
            return result;
        }
        result = request.put_u32(LOGGER_ATTRIBUTE_LOG_MIN_DATA_SIZE, mMinDataSize);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to put min data size; result = %d", result);
            return result;
        }
        result = request.put_string(LOGGER_ATTRIBUTE_RING_NAME, mRingName);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to put ringbuffer name; result = %d", result);
            return result;
        }
        request.attr_end(data);

        return WIFI_SUCCESS;
    }

    int createRequest(WifiRequest &request) {
        int result;

        switch (mType) {
            case GET_FW_VER:
            {
                result = request.create(GOOGLE_OUI, LOGGER_GET_VER);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to create get fw version request; result = %d", result);
                    return result;
                }

                nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);

                // Driver expecting only attribute type, passing mbuff as data with
                // length 0 to avoid undefined state
                result = request.put(LOGGER_ATTRIBUTE_FW_VER, mBuff, 0);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to put get fw version request; result = %d", result);
                    return result;
                }
                request.attr_end(data);
                break;
            }

            case GET_DRV_VER:
            {
                result = request.create(GOOGLE_OUI, LOGGER_GET_VER);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to create get drv version request; result = %d", result);
                    return result;
                }

                nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);

                // Driver expecting only attribute type, passing mbuff as data with
                // length 0 to avoid undefined state
                result = request.put(LOGGER_ATTRIBUTE_DRIVER_VER, mBuff, 0);

                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to put get drv version request; result = %d", result);
                    return result;
                }
                request.attr_end(data);
                break;
            }

            case GET_RING_DATA:
            {
                result = request.create(GOOGLE_OUI, LOGGER_GET_RING_DATA);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to create get ring data request; result = %d", result);
                    return result;
                }

                nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
                result = request.put_string(LOGGER_ATTRIBUTE_RING_NAME, mRingName);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to put ring data request; result = %d", result);
                    return result;
                }
                request.attr_end(data);
                break;
            }

            case GET_RING_STATUS:
            {
                result = request.create(GOOGLE_OUI, LOGGER_GET_RING_STATUS);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to create get ring status request; result = %d", result);
                    return result;
                }
                break;
            }

            case GET_FEATURE:
            {
                result = request.create(GOOGLE_OUI, LOGGER_GET_FEATURE);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to create get feature request; result = %d", result);
                    return result;
                }
                break;
            }

            case START_RING_LOG:
                result = createRingRequest(request);
                break;

            default:
                ALOGE("Unknown Debug command");
                result = WIFI_ERROR_UNKNOWN;
        }
        return result;
    }

    int start() {
        // ALOGD("Start debug command");
        WifiRequest request(familyId(), ifaceId());
        int result = createRequest(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to create debug request; result = %d", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to register debug response; result = %d", result);
        }
        return result;
    }

    virtual int handleResponse(WifiEvent& reply) {
        ALOGD("In DebugCommand::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        switch (mType) {
            case GET_DRV_VER:
            case GET_FW_VER:
            {
                void *data = reply.get_vendor_data();
                int len = reply.get_vendor_data_len();

                ALOGD("len = %d, expected len = %d", len, *mBuffSize);
                memcpy(mBuff, data, min(len, *mBuffSize));
                if (*mBuffSize < len)
                    return NL_SKIP;
                *mBuffSize = len;
                break;
            }

            case START_RING_LOG:
            case GET_RING_DATA:
                break;

            case GET_RING_STATUS:
            {
                nlattr *vendor_data = reply.get_attribute(NL80211_ATTR_VENDOR_DATA);
                int len = reply.get_vendor_data_len();
                wifi_ring_buffer_status *status(mStatus);

                if (vendor_data == NULL || len == 0) {
                    ALOGE("No Debug data found");
                    return NL_SKIP;
                }

                nl_iterator it(vendor_data);
                if (it.get_type() == LOGGER_ATTRIBUTE_RING_NUM) {
                    unsigned int num_rings = it.get_u32();
                    if (*mNumRings < num_rings) {
                        ALOGE("Not enough status buffers provided, available: %d required: %d",
                                *mNumRings, num_rings);
                    } else {
                        *mNumRings = num_rings;
                    }
                } else {
                    ALOGE("Unknown attribute: %d expecting %d",
                            it.get_type(), LOGGER_ATTRIBUTE_RING_NUM);
                    return NL_SKIP;
                }

                it.next();
                for (unsigned int i = 0; it.has_next() && i < *mNumRings; it.next()) {
                    if (it.get_type() == LOGGER_ATTRIBUTE_RING_STATUS) {
                        memcpy(status, it.get_data(), sizeof(wifi_ring_buffer_status));
                        i++;
                        status++;
                    } else {
                        ALOGW("Ignoring invalid attribute type = %d, size = %d",
                                it.get_type(), it.get_len());
                    }
                }
                break;
            }

            case GET_FEATURE:
            {
                void *data = reply.get_vendor_data();
                int len = reply.get_vendor_data_len();

                ALOGD("len = %d, expected len = %lu", len, sizeof(unsigned int));
                memcpy(mSupport, data, sizeof(unsigned int));
                break;
            }

            default:
                ALOGW("Unknown Debug command");
        }
        return NL_OK;
    }

    virtual int handleEvent(WifiEvent& event) {
        /* NO events! */
        return NL_SKIP;
    }
};

/* API to collect a firmware version string */
wifi_error wifi_get_firmware_version(wifi_interface_handle iface, char *buffer,
        int buffer_size)
{
    if (buffer && (buffer_size > 0)) {
        DebugCommand *cmd = new DebugCommand(iface, buffer, &buffer_size, GET_FW_VER);
        NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
        wifi_error result = (wifi_error)cmd->start();
        cmd->releaseRef();
        return result;
    } else {
        ALOGE("FW version buffer NULL");
        return  WIFI_ERROR_INVALID_ARGS;
    }
}

/* API to collect a driver version string */
wifi_error wifi_get_driver_version(wifi_interface_handle iface, char *buffer, int buffer_size)
{
    if (buffer && (buffer_size > 0)) {
        DebugCommand *cmd = new DebugCommand(iface, buffer, &buffer_size, GET_DRV_VER);
        NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
        wifi_error result = (wifi_error)cmd->start();
        cmd->releaseRef();
        return result;
    } else {
        ALOGE("Driver version buffer NULL");
        return  WIFI_ERROR_INVALID_ARGS;
    }
}

/* API to collect driver records */
wifi_error wifi_get_ring_data(wifi_interface_handle iface, char *ring_name)
{
    DebugCommand *cmd = new DebugCommand(iface, ring_name, GET_RING_DATA);
    NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
    wifi_error result = (wifi_error)cmd->start();
    cmd->releaseRef();
    return result;
}

/* API to get the status of all ring buffers supported by driver */
wifi_error wifi_get_ring_buffers_status(wifi_interface_handle iface,
        u32 *num_rings, wifi_ring_buffer_status *status)
{
    if (status && num_rings) {
        DebugCommand *cmd = new DebugCommand(iface, num_rings, status, GET_RING_STATUS);
        NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
        wifi_error result = (wifi_error)cmd->start();
        cmd->releaseRef();
        return result;
    } else {
        ALOGE("Ring status buffer NULL");
        return  WIFI_ERROR_INVALID_ARGS;
    }
}

/* API to get supportable feature */
wifi_error wifi_get_logger_supported_feature_set(wifi_interface_handle iface,
        unsigned int *support)
{
    if (support) {
        DebugCommand *cmd = new DebugCommand(iface, support, GET_FEATURE);
        NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
        wifi_error result = (wifi_error)cmd->start();
        cmd->releaseRef();
        return result;
    } else {
        ALOGE("Get support buffer NULL");
        return  WIFI_ERROR_INVALID_ARGS;
    }
}

wifi_error wifi_start_logging(wifi_interface_handle iface, u32 verbose_level,
        u32 flags, u32 max_interval_sec, u32 min_data_size, char *ring_name)
{
    if (ring_name) {
        DebugCommand *cmd = new DebugCommand(iface, verbose_level, flags, max_interval_sec,
                    min_data_size, ring_name, START_RING_LOG);
        NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
        wifi_error result = (wifi_error)cmd->start();
        cmd->releaseRef();
        return result;
    } else {
        ALOGE("Ring name NULL");
        return  WIFI_ERROR_INVALID_ARGS;
    }
}


///////////////////////////////////////////////////////////////////////////////
class SetLogHandler : public WifiCommand
{
    wifi_ring_buffer_data_handler mHandler;

public:
    SetLogHandler(wifi_interface_handle iface, int id, wifi_ring_buffer_data_handler handler)
        : WifiCommand("SetLogHandler", iface, id), mHandler(handler)
    { }

    int start() {
        ALOGV("Register loghandler");
        int result;
        uint32_t event_sock_pid = getpid() + (WIFI_HAL_EVENT_SOCK_PORT << 22);
        registerVendorHandler(GOOGLE_OUI, GOOGLE_DEBUG_RING_EVENT);

        WifiRequest request(familyId(), ifaceId());

        /* set hal event socket port to driver */
        result = request.create(GOOGLE_OUI, LOGGER_SET_HAL_PID);
        if (result != WIFI_SUCCESS) {
            ALOGV("Failed to set Hal preInit; result = %d", result);
            return result;
        }
        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u32(SET_HAL_START_ATTRIBUTE_EVENT_SOCK_PID, event_sock_pid);
        if (result != WIFI_SUCCESS) {
            ALOGV("Hal preInit Failed to put pic = %d", result);
            return result;
        }

        if (result != WIFI_SUCCESS) {
            ALOGV("Hal preInit Failed to put pid= %d", result);
            return result;
        } 

        request.attr_end(data);

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to register set Hal preInit; result = %d", result);
            return result;
        }
        return result;
    }

    virtual int cancel() {
        /* Send a command to driver to stop generating logging events */
        ALOGV("Clear loghandler");

        /* unregister event handler */
        unregisterVendorHandler(GOOGLE_OUI, GOOGLE_DEBUG_RING_EVENT);
        wifi_unregister_cmd(wifiHandle(), id());

        WifiRequest request(familyId(), ifaceId());
        int result = request.create(GOOGLE_OUI, LOGGER_RESET_LOGGING);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to create reset request; result = %d", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("failed to request reset; result = %d", result);
            return result;
        }

        ALOGD("Success to clear loghandler");
        return WIFI_SUCCESS;
    }

    virtual int handleEvent(WifiEvent& event) {
        char *buffer = NULL;
        int buffer_size = 0;

        // ALOGD("In SetLogHandler::handleEvent");
        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = event.get_vendor_data_len();
        int event_id = event.get_vendor_subcmd();
        // ALOGI("Got Logger event: %d", event_id);

        if (vendor_data == NULL || len == 0) {
            ALOGE("No Debug data found");
            return NL_SKIP;
        }

        if(event_id == GOOGLE_DEBUG_RING_EVENT) {
            wifi_ring_buffer_status status;
            memset(&status, 0, sizeof(status));

            for (nl_iterator it(vendor_data); it.has_next(); it.next()) {
                if (it.get_type() == LOGGER_ATTRIBUTE_RING_STATUS) {
                    memcpy(&status, it.get_data(), sizeof(status));
                } else if (it.get_type() == LOGGER_ATTRIBUTE_RING_DATA) {
                    buffer_size = it.get_len();
                    buffer = (char *)it.get_data();
                } else {
                    ALOGW("Ignoring invalid attribute type = %d, size = %d",
                            it.get_type(), it.get_len());
                }
            }

            // ALOGI("Retrieved Debug data");
            if (mHandler.on_ring_buffer_data) {
                /* Skip msg header. Retrieved log */
                char *pBuff;
                wifi_ring_buffer_entry *buffer_entry = 
                            (wifi_ring_buffer_entry *) buffer;
                pBuff = (char *) (buffer_entry + 1);
                (*mHandler.on_ring_buffer_data)((char *)status.name, pBuff, 
                    buffer_entry->entry_size, &status);
            }
        } else {
            ALOGE("Unknown Event");
            return NL_SKIP;
        }
        return NL_OK;
    }
};

wifi_error wifi_set_log_handler(wifi_request_id id, wifi_interface_handle iface,
        wifi_ring_buffer_data_handler handler)
{
    wifi_handle handle = getWifiHandle(iface);
    ALOGV("Loghandler start, handle = %p", handle);

    SetLogHandler *cmd = new SetLogHandler(iface, id, handler);
    NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
    wifi_error result = wifi_register_cmd(handle, id, cmd);
    if (result != WIFI_SUCCESS) {
        cmd->releaseRef();
        return result;
    }
    result = (wifi_error)cmd->start();
    if (result != WIFI_SUCCESS) {
        wifi_unregister_cmd(handle, id);
        cmd->releaseRef();
        return result;
    }
    return result;
}

wifi_error wifi_reset_log_handler(wifi_request_id id, wifi_interface_handle iface)
{
    wifi_handle handle = getWifiHandle(iface);
    ALOGV("Loghandler reset, wifi_request_id = %d, handle = %p", id, handle);

    if (id == -1) {
        wifi_ring_buffer_data_handler handler;
        memset(&handler, 0, sizeof(handler));

        SetLogHandler *cmd = new SetLogHandler(iface, id, handler);
        NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
        cmd->cancel();
        cmd->releaseRef();
        return WIFI_SUCCESS;
    }

    return wifi_get_cancel_cmd(id, iface);
}

///////////////////////////////////////////////////////////////////////////////
class SetAlertHandler : public WifiCommand
{
    wifi_alert_handler mHandler;
    int mBuffSize;
    char *mBuff;
    int mErrCode;

public:
    SetAlertHandler(wifi_interface_handle iface, int id, wifi_alert_handler handler)
        : WifiCommand("SetAlertHandler", iface, id), mHandler(handler), mBuffSize(0), mBuff(NULL),
            mErrCode(0)
    { }

    int start() {
        ALOGV("Start Alerting");
        registerVendorHandler(GOOGLE_OUI, GOOGLE_DEBUG_MEM_DUMP_EVENT);
        return WIFI_SUCCESS;
    }

    virtual int cancel() {
        ALOGV("Clear alerthandler");

        /* unregister alert handler */
        unregisterVendorHandler(GOOGLE_OUI, GOOGLE_DEBUG_MEM_DUMP_EVENT);
        wifi_unregister_cmd(wifiHandle(), id());
        ALOGD("Success to clear alerthandler");
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        ALOGD("In SetAlertHandler::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        nlattr *vendor_data = reply.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = reply.get_vendor_data_len();

        ALOGD("len = %d", len);
        if (vendor_data == NULL || len == 0) {
            ALOGE("no vendor data in memory dump response; ignoring it");
            return NL_SKIP;
        }

        for (nl_iterator it(vendor_data); it.has_next(); it.next()) {
            if (it.get_type() == LOGGER_ATTRIBUTE_FW_DUMP_DATA) {
                ALOGI("Initiating alert callback");
                if (mHandler.on_alert) {
                    (*mHandler.on_alert)(id(), mBuff, mBuffSize, mErrCode);
                }
                if (mBuff) {
                    free(mBuff);
                    mBuff = NULL;
                }
            }
        }
        return NL_OK;
    }

    virtual int handleEvent(WifiEvent& event) {
        char *buffer = NULL;
        int buffer_size = 0;
        bool is_err_alert = false;

        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = event.get_vendor_data_len();
        int event_id = event.get_vendor_subcmd();
        ALOGI("Got event: %d", event_id);

        if (vendor_data == NULL || len == 0) {
            ALOGE("No Debug data found");
            return NL_SKIP;
        }

        if (event_id == GOOGLE_DEBUG_MEM_DUMP_EVENT) {
            for (nl_iterator it(vendor_data); it.has_next(); it.next()) {
                if (it.get_type() == LOGGER_ATTRIBUTE_FW_DUMP_LEN) {
                    mBuffSize = it.get_u32();
                } else if (it.get_type() == LOGGER_ATTRIBUTE_RING_DATA) {
                    buffer_size = it.get_len();
                    buffer = (char *)it.get_data();
                } else if (it.get_type() == LOGGER_ATTRIBUTE_FW_ERR_CODE) {
                    /* Error code is for error alert event only */
                    mErrCode = it.get_u32();
                    is_err_alert = true;
                } else {
                    ALOGW("Ignoring invalid attribute type = %d, size = %d",
                            it.get_type(), it.get_len());
                }
            }

            if (is_err_alert) {
                mBuffSize = sizeof(mErrCode);
                if (mBuff) free(mBuff);
                mBuff = (char *)malloc(mBuffSize);
                if (!mBuff) {
                  ALOGE("Buffer allocation failed");
                  return NL_SKIP;
                }
                memcpy(mBuff, (char *)&mErrCode, mBuffSize);
                ALOGI("Initiating alert callback");
                if (mHandler.on_alert) {
                  (*mHandler.on_alert)(id(), mBuff, mBuffSize, mErrCode);
                }
                if (mBuff) {
                  free(mBuff);
                  mBuff = NULL;
                }
                mBuffSize = 0;
                return NL_OK;
            }

            if (mBuffSize) {
                ALOGD("dump size: %d meta data size: %d", mBuffSize, buffer_size);
                if (mBuff) free(mBuff);
                mBuff = (char *)malloc(mBuffSize + buffer_size);
                if (!mBuff) {
                    ALOGE("Buffer allocation failed");
                    return NL_SKIP;
                }
                memcpy(mBuff, buffer, buffer_size);

                WifiRequest request(familyId(), ifaceId());
                int result = request.create(GOOGLE_OUI, LOGGER_GET_MEM_DUMP);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to create get memory dump request; result = %d", result);
                    free(mBuff);
                    return NL_SKIP;
                }
                nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
                result = request.put_u32(LOGGER_ATTRIBUTE_FW_DUMP_LEN, mBuffSize);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to put get memory dump request; result = %d", result);
                    return result;
                }

                result = request.put_u64(LOGGER_ATTRIBUTE_FW_DUMP_DATA,
                         (uint64_t)(mBuff+buffer_size));
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to put get memory dump request; result = %d", result);
                    return result;
                }

                request.attr_end(data);
                mBuffSize += buffer_size;

                result = requestResponse(request);

                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to register get momory dump response; result = %d", result);
                }
            } else {
                ALOGE("dump event missing dump length attribute");
                return NL_SKIP;
            }
        }
        return NL_OK;
    }
};

class SetRestartHandler : public WifiCommand
{
    wifi_subsystem_restart_handler mHandler;
    char *mBuff;
public:
    SetRestartHandler(wifi_handle handle, wifi_request_id id, wifi_subsystem_restart_handler handler)
        : WifiCommand("SetRestartHandler", handle, id), mHandler(handler), mBuff(NULL)
    { }
    int start() {
        ALOGI("Start Restart Handler handler:%p", mHandler);
        registerVendorHandler(BRCM_OUI, BRCM_VENDOR_EVENT_HANGED);
        return WIFI_SUCCESS;
    }
    virtual int cancel() {
        ALOGI("Clear Restart Handler");

        /* unregister alert handler */
        unregisterVendorHandler(BRCM_OUI, BRCM_VENDOR_EVENT_HANGED);
        wifi_unregister_cmd(wifiHandle(), id());
        ALOGI("Success to clear restarthandler");
        return WIFI_SUCCESS;
    }

    virtual int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_OK;
    }

    virtual int handleEvent(WifiEvent& event) {
        nlattr *vendor_data = event.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = event.get_vendor_data_len();
        int event_id = event.get_vendor_subcmd();
        ALOGI("Got event: %d", event_id);

        if (vendor_data == NULL || len == 0) {
            ALOGE("No Debug data found");
            return NL_SKIP;
        }
        if (event_id == BRCM_VENDOR_EVENT_HANGED) {
            for (nl_iterator it(vendor_data); it.has_next(); it.next()) {
                if (it.get_type() == LOGGER_ATTRIBUTE_HANG_REASON) {
                    mBuff = (char *)it.get_data();
                } else {
                    ALOGI("Ignoring invalid attribute type = %d, size = %d",
                            it.get_type(), it.get_len());
                }
            }

            if (*mHandler.on_subsystem_restart) {
                (*mHandler.on_subsystem_restart)(mBuff);
                ALOGI("Hang event received. Trigger SSR handler:%p",
                    mHandler.on_subsystem_restart);
            } else {
                ALOGI("No Restart handler registered");
            }
        }
        return NL_OK;
    }
};

///////////////////////////////////////////////////////////////////////////////
class SubSystemRestart : public WifiCommand
{
    public:
    SubSystemRestart(wifi_interface_handle iface)
        : WifiCommand("SubSystemRestart", iface, 0)
    { }

    int createRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, WIFI_SUBCMD_TRIGGER_SSR);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);

        request.attr_end(data);
        return WIFI_SUCCESS;
    }

    int create() {
        WifiRequest request(familyId(), ifaceId());

        int result = createRequest(request);
        if (result < 0) {
            ALOGE("Failed to create ssr request result = %d\n", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to register ssr response; result = %d\n", result);
        }
        return result;
    }

    protected:
    int handleResponse(WifiEvent& reply) {
        /* Nothing to do on response! */
        return NL_OK;
    }

    int handleEvent(WifiEvent& event) {
        /* NO events to handle here! */
        return NL_SKIP;
    }

};
///////////////////////////////////////////////////////////////////////////////
class HalInit : public WifiCommand
{
    int mErrCode;

    public:
    HalInit(wifi_interface_handle iface, int id)
        : WifiCommand("HalInit", iface, id), mErrCode(0)
    { }

    int start() {
        ALOGE("Start Set Hal");
        WifiRequest request(familyId(), ifaceId());

        int result = request.create(GOOGLE_OUI, LOGGER_SET_HAL_START);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to set hal start; result = %d", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to register set hal start response; result = %d", result);
        }
        return result;
    }


    virtual int cancel() {
        ALOGE("Cancel: Stop Hal");
        WifiRequest request(familyId(), ifaceId());

        int result = request.create(GOOGLE_OUI, LOGGER_HAL_STOP);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to stop hal ; result = %d", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to register set hal start response; result = %d", result);
        }
        wifi_unregister_cmd(wifiHandle(), id());
	ALOGV("Stop HAL Successfully Completed, mErrCode = %d\n", mErrCode);
        return result;
    }

    int preInit() {
        ALOGE("Hal preInit");
        WifiRequest request(familyId(), ifaceId());

        int result = request.create(GOOGLE_OUI, LOGGER_SET_HAL_START);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to set Hal preInit; result = %d", result);
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_string(SET_HAL_START_ATTRIBUTE_PRE_INIT, (char *)HAL_VERSION);
        if (result != WIFI_SUCCESS) {
            ALOGE("Hal preInit Failed to put data= %d", result);
            return result;
        }
        request.attr_end(data);

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to register set Hal preInit; result = %d", result);
        }
        return result;
    }

    virtual int handleResponse(WifiEvent& reply) {
        ALOGE("In SetHalStarted::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }
        return NL_OK;
    }

    virtual int handleEvent(WifiEvent& event) {
        /* NO events! */
        return NL_SKIP;
    }
};


wifi_error wifi_start_hal(wifi_interface_handle iface)
{
    wifi_handle handle = getWifiHandle(iface);
    ALOGV("HAL INIT start, handle = %p", handle);

    HalInit *cmd = new HalInit(iface, HAL_START_REQUEST_ID);
    NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
    wifi_error result = wifi_register_cmd(handle, HAL_START_REQUEST_ID, cmd);
    if (result != WIFI_SUCCESS) {
        cmd->releaseRef();
        return result;
    }
    result = (wifi_error)cmd->start();
    if (result != WIFI_SUCCESS) {
        wifi_unregister_cmd(handle,HAL_START_REQUEST_ID);
        cmd->releaseRef();
        return result;
    }
    return result;
}

wifi_error wifi_hal_preInit(wifi_interface_handle iface)
{
    wifi_handle handle = getWifiHandle(iface);
    ALOGV("wifi_hal_preInit, handle = %p", handle);

    HalInit *cmd = new HalInit(iface, HAL_START_REQUEST_ID);
    NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
    wifi_error result = wifi_register_cmd(handle, HAL_START_REQUEST_ID, cmd);
    if (result != WIFI_SUCCESS) {
        cmd->releaseRef();
        return result;
    }
    result = (wifi_error)cmd->preInit();
    if (result != WIFI_SUCCESS) {
        wifi_unregister_cmd(handle,HAL_START_REQUEST_ID);
        cmd->releaseRef();
        return result;
    }
    return result;
}

wifi_error wifi_stop_hal(wifi_interface_handle iface)
{
    HalInit *cmd = new HalInit(iface, HAL_START_REQUEST_ID);
    NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
    cmd->cancel();
    cmd->releaseRef();
    return WIFI_SUCCESS;
}


wifi_error wifi_set_subsystem_restart_handler(wifi_handle handle,
                                              wifi_subsystem_restart_handler handler)
{
    hal_info *info = NULL;

    info = (hal_info *)handle;
    if (info == NULL) {
        ALOGE("Could not find hal info\n");
        return WIFI_ERROR_UNKNOWN;
    }

    SetRestartHandler *cmd = new SetRestartHandler(handle, HAL_RESTART_ID, handler);
    NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
    wifi_error result = wifi_register_cmd(handle, HAL_RESTART_ID, cmd);
    if (result != WIFI_SUCCESS) {
        cmd->releaseRef();
        return result;
    }

    result = (wifi_error)cmd->start();
    if (result != WIFI_SUCCESS) {
        wifi_unregister_cmd(handle, HAL_RESTART_ID);
        cmd->releaseRef();
        return result;
    }

    /* Cache the handler to use it for trigger subsystem restart */
    ALOGI("Register SSR handler:%p", handler);
    info->restart_handler = handler;
    return result;
}

wifi_error wifi_trigger_subsystem_restart(wifi_handle handle)
{
    wifi_error result = WIFI_SUCCESS;
    hal_info *info = NULL;
    char error_str[20];
    SubSystemRestart *cmd = NULL;
    wifi_interface_handle *ifaceHandles = NULL;
    wifi_interface_handle wlan0Handle;
    int numIfaceHandles = 0;

    info = (hal_info *)handle;
    if (handle == NULL || info == NULL) {
        ALOGE("Could not find hal info\n");
        result = WIFI_ERROR_UNKNOWN;
        goto exit;
    }

    ALOGI("Trigger subsystem restart\n");

    wlan0Handle = wifi_get_wlan_interface((wifi_handle)handle, ifaceHandles, numIfaceHandles);

    cmd = new SubSystemRestart(wlan0Handle);
    NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);

    result = (wifi_error)cmd->create();
    if (result != WIFI_SUCCESS) {
        cmd->releaseRef();
        strncpy(error_str, "WIFI_ERROR_UNKNOWN", sizeof(error_str));
        ALOGE("Failed to create SSR");
        goto exit;
    }

    strncpy(error_str, "WIFI_SUCCESS", sizeof(error_str));

exit:
    if (info->restart_handler.on_subsystem_restart) {
        ALOGI("Trigger ssr handler registered handler:%p",
            info->restart_handler.on_subsystem_restart);
        (info->restart_handler.on_subsystem_restart)(error_str);
    } else {
        ALOGI("No trigger ssr handler registered");
    }

    return result;
}

wifi_error wifi_set_alert_handler(wifi_request_id id, wifi_interface_handle iface,
        wifi_alert_handler handler)
{
    wifi_handle handle = getWifiHandle(iface);
    ALOGV("Alerthandler start, handle = %p", handle);

    SetAlertHandler *cmd = new SetAlertHandler(iface, id, handler);
    NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
    wifi_error result = wifi_register_cmd(handle, id, cmd);
    if (result != WIFI_SUCCESS) {
        cmd->releaseRef();
        return result;
    }
    result = (wifi_error)cmd->start();
    if (result != WIFI_SUCCESS) {
        wifi_unregister_cmd(handle, id);
        cmd->releaseRef();
        return result;
    }
    return result;
}

wifi_error wifi_reset_alert_handler(wifi_request_id id, wifi_interface_handle iface)
{
    wifi_handle handle = getWifiHandle(iface);
    ALOGV("Alerthandler reset, wifi_request_id = %d, handle = %p", id, handle);

    if (id == -1) {
        wifi_alert_handler handler;
        memset(&handler, 0, sizeof(handler));

        SetAlertHandler *cmd = new SetAlertHandler(iface, id, handler);
        NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
        cmd->cancel();
        cmd->releaseRef();
        return WIFI_SUCCESS;
    }

    return wifi_get_cancel_cmd(id, iface);
}

///////////////////////////////////////////////////////////////////////////////
class MemoryDumpCommand: public WifiCommand
{
    wifi_firmware_memory_dump_handler mHandler;
    int mBuffSize;
    char *mBuff;

public:
    MemoryDumpCommand(wifi_interface_handle iface, wifi_firmware_memory_dump_handler handler)
        : WifiCommand("MemoryDumpCommand", iface, 0), mHandler(handler), mBuffSize(0), mBuff(NULL)
    { }

    int start() {
        ALOGD("Start memory dump command");
        WifiRequest request(familyId(), ifaceId());

        int result = request.create(GOOGLE_OUI, LOGGER_TRIGGER_MEM_DUMP);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to create trigger fw memory dump request; result = %d", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to register trigger memory dump response; result = %d", result);
        }
        return result;
    }

    virtual int handleResponse(WifiEvent& reply) {
        ALOGD("In MemoryDumpCommand::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        nlattr *vendor_data = reply.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = reply.get_vendor_data_len();

        ALOGD("len = %d", len);
        if (vendor_data == NULL || len == 0) {
            ALOGE("no vendor data in memory dump response; ignoring it");
            return NL_SKIP;
        }

        for (nl_iterator it(vendor_data); it.has_next(); it.next()) {
            if (it.get_type() == LOGGER_ATTRIBUTE_FW_DUMP_LEN) {
                mBuffSize = it.get_u32();

                if (mBuff)
                    free(mBuff);
                mBuff = (char *)malloc(mBuffSize);
                if (!mBuff) {
                    ALOGE("Buffer allocation failed");
                    return NL_SKIP;
                }
                WifiRequest request(familyId(), ifaceId());
                int result = request.create(GOOGLE_OUI, LOGGER_GET_MEM_DUMP);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to create get memory dump request; result = %d", result);
                    free(mBuff);
                    return NL_SKIP;
                }

                nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
                result = request.put_u32(LOGGER_ATTRIBUTE_FW_DUMP_LEN, mBuffSize);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to put get memory dump request; result = %d", result);
                    return result;
                }

                result = request.put_u64(LOGGER_ATTRIBUTE_FW_DUMP_DATA, (uint64_t)mBuff);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to put get memory dump request; result = %d", result);
                    return result;
                }
                request.attr_end(data);

                result = requestResponse(request);
                if (result != WIFI_SUCCESS) {
                    ALOGE("Failed to register get momory dump response; result = %d", result);
                }
            } else if (it.get_type() == LOGGER_ATTRIBUTE_FW_DUMP_DATA) {
                ALOGI("Initiating memory dump callback");
                if (mHandler.on_firmware_memory_dump) {
                    (*mHandler.on_firmware_memory_dump)(mBuff, mBuffSize);
                }
                if (mBuff) {
                    free(mBuff);
                    mBuff = NULL;
                }
            } else {
                ALOGW("Ignoring invalid attribute type = %d, size = %d",
                        it.get_type(), it.get_len());
            }
        }
        return NL_OK;
    }

    virtual int handleEvent(WifiEvent& event) {
        /* NO events! */
        return NL_SKIP;
    }
};

/* API to collect a firmware memory dump for a given iface */
wifi_error wifi_get_firmware_memory_dump( wifi_interface_handle iface,
        wifi_firmware_memory_dump_handler handler)
{
    MemoryDumpCommand *cmd = new MemoryDumpCommand(iface, handler);
    NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
    wifi_error result = (wifi_error)cmd->start();
    cmd->releaseRef();
    return result;
}

class PacketFateCommand: public WifiCommand
{
    void *mReportBufs;
    size_t mNoReqFates;
    size_t *mNoProvidedFates;
    PktFateReqType mReqType;

public:
    PacketFateCommand(wifi_interface_handle handle)
        : WifiCommand("PacketFateCommand", handle, 0), mReqType(PACKET_MONITOR_START)
    {
        mReportBufs = NULL;
        mNoReqFates = 0;
        mNoProvidedFates = NULL;
    }

    PacketFateCommand(wifi_interface_handle handle, wifi_tx_report *tx_report_bufs,
            size_t n_requested_fates, size_t *n_provided_fates)
        : WifiCommand("PacketFateCommand", handle, 0), mReportBufs(tx_report_bufs),
                  mNoReqFates(n_requested_fates), mNoProvidedFates(n_provided_fates),
                  mReqType(TX_PACKET_FATE)
    { }

    PacketFateCommand(wifi_interface_handle handle, wifi_rx_report *rx_report_bufs,
            size_t n_requested_fates, size_t *n_provided_fates)
        : WifiCommand("PacketFateCommand", handle, 0), mReportBufs(rx_report_bufs),
                  mNoReqFates(n_requested_fates), mNoProvidedFates(n_provided_fates),
                  mReqType(RX_PACKET_FATE)
    { }

    int createRequest(WifiRequest& request) {
        if (mReqType == TX_PACKET_FATE) {
            ALOGD("%s Get Tx packet fate request\n", __FUNCTION__);
            return createTxPktFateRequest(request);
        } else if (mReqType == RX_PACKET_FATE) {
            ALOGD("%s Get Rx packet fate request\n", __FUNCTION__);
            return createRxPktFateRequest(request);
        } else if (mReqType == PACKET_MONITOR_START) {
            ALOGD("%s Monitor packet fate request\n", __FUNCTION__);
            return createMonitorPktFateRequest(request);
        } else {
            ALOGE("%s Unknown packet fate request\n", __FUNCTION__);
            return WIFI_ERROR_NOT_SUPPORTED;
        }
        return WIFI_SUCCESS;
    }

    int createMonitorPktFateRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, LOGGER_START_PKT_FATE_MONITORING);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        request.attr_end(data);
        return result;
    }

    int createTxPktFateRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, LOGGER_GET_TX_PKT_FATES);
        if (result < 0) {
            return result;
        }

        memset(mReportBufs, 0, (mNoReqFates * sizeof(wifi_tx_report)));
        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u32(LOGGER_ATTRIBUTE_PKT_FATE_NUM, mNoReqFates);
        if (result < 0) {
            return result;
        }
        result = request.put_u64(LOGGER_ATTRIBUTE_PKT_FATE_DATA, (uint64_t)mReportBufs);
        if (result < 0) {
            return result;
        }
        request.attr_end(data);
        return result;
    }

    int createRxPktFateRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, LOGGER_GET_RX_PKT_FATES);
        if (result < 0) {
            return result;
        }

        memset(mReportBufs, 0, (mNoReqFates * sizeof(wifi_rx_report)));
        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);
        result = request.put_u32(LOGGER_ATTRIBUTE_PKT_FATE_NUM, mNoReqFates);
        if (result < 0) {
            return result;
        }
        result = request.put_u64(LOGGER_ATTRIBUTE_PKT_FATE_DATA, (uint64_t)mReportBufs);
        if (result < 0) {
            return result;
        }
        request.attr_end(data);
        return result;
    }

    int start() {
        ALOGD("Start get packet fate command\n");
        WifiRequest request(familyId(), ifaceId());

        int result = createRequest(request);
        if (result < 0) {
            ALOGE("Failed to create get pkt fate request; result = %d\n", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to register get pkt fate response; result = %d\n", result);
        }
        return result;
    }

    int handleResponse(WifiEvent& reply) {
        ALOGD("In GetPktFateCommand::handleResponse\n");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGI("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        int id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();
        nlattr *vendor_data = reply.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = reply.get_vendor_data_len();

        ALOGI("Id = %0x, subcmd = %d, len = %d", id, subcmd, len);

        if (mReqType == TX_PACKET_FATE) {
            ALOGI("Response recieved for get TX pkt fate command\n");
        } else if (mReqType == RX_PACKET_FATE) {
            ALOGI("Response recieved for get RX pkt fate command\n");
        } else if (mReqType == PACKET_MONITOR_START) {
            ALOGI("Response recieved for monitor pkt fate command\n");
            return NL_OK;
        } else {
            ALOGE("Response recieved for unknown pkt fate command\n");
            return NL_SKIP;
        }

        if (vendor_data == NULL || len == 0) {
            ALOGE("no vendor data in GetPktFateCommand response; ignoring it\n");
            return NL_SKIP;
        }

        for (nl_iterator it(vendor_data); it.has_next(); it.next()) {
            if (it.get_type() == LOGGER_ATTRIBUTE_PKT_FATE_NUM) {
                *mNoProvidedFates = it.get_u32();
                ALOGI("No: of pkt fates provided is %zu\n", *mNoProvidedFates);
            } else {
                ALOGE("Ignoring invalid attribute type = %d, size = %d\n",
                        it.get_type(), it.get_len());
            }
        }

        return NL_OK;
    }

    int handleEvent(WifiEvent& event) {
        /* NO events to handle here! */
        return NL_SKIP;
    }
};

class GetWakeReasonCountCommand : public WifiCommand {
    WLAN_DRIVER_WAKE_REASON_CNT *mWakeReasonCnt;
    void *mCmdEventWakeCount;
    public:
    GetWakeReasonCountCommand(wifi_interface_handle handle,
        WLAN_DRIVER_WAKE_REASON_CNT *wlanDriverWakeReasonCount) :
        WifiCommand("GetWakeReasonCountCommand", handle, 0),
        mWakeReasonCnt(wlanDriverWakeReasonCount)
    {
        mCmdEventWakeCount = mWakeReasonCnt->cmd_event_wake_cnt;
    }

    int createRequest(WifiRequest& request) {
        int result = request.create(GOOGLE_OUI, LOGGER_GET_WAKE_REASON_STATS);
        if (result < 0) {
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);

        request.attr_end(data);
        return WIFI_SUCCESS;
    }

    int start() {
        ALOGD("Start get wake stats command\n");
        WifiRequest request(familyId(), ifaceId());

        int result = createRequest(request);
        if (result < 0) {
            ALOGE("Failed to create request result = %d\n", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to register wake stats  response; result = %d\n", result);
        }
        return result;
    }

    protected:
    int handleResponse(WifiEvent& reply) {
        ALOGE("In GetWakeReasonCountCommand::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        int id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();

        nlattr *vendor_data = reply.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = reply.get_vendor_data_len();

        ALOGV("Id = %0x, subcmd = %d, len = %d", id, subcmd, len);
        if (vendor_data == NULL || len == 0) {
            ALOGE("no vendor data in GetGetWakeReasonCountCommand response; ignoring it");
            return NL_SKIP;
        }

        for (nl_iterator it(vendor_data); it.has_next(); it.next()) {
            switch (it.get_type()) {
                case WAKE_STAT_ATTRIBUTE_TOTAL_DRIVER_FW:
                    mWakeReasonCnt->total_driver_fw_local_wake =
                        it.get_u32();
                    break;
                case WAKE_STAT_ATTRIBUTE_TOTAL:
                    mWakeReasonCnt->total_cmd_event_wake =
                        it.get_u32();
                    break;
                case WAKE_STAT_ATTRIBUTE_CMD_COUNT_USED:
                    mWakeReasonCnt->cmd_event_wake_cnt_used =
                        it.get_u32();
                    break;
                case WAKE_STAT_ATTRIBUTE_WAKE:
                    memcpy(mCmdEventWakeCount, it.get_data(),
                            (mWakeReasonCnt->cmd_event_wake_cnt_used * sizeof(int)));
                    break;
                case WAKE_STAT_ATTRIBUTE_TOTAL_RX_DATA_WAKE:
                    mWakeReasonCnt->total_rx_data_wake =
                        it.get_u32();
                    break;
                case WAKE_STAT_ATTRIBUTE_RX_UNICAST_COUNT:
                    mWakeReasonCnt->rx_wake_details.rx_unicast_cnt =
                        it.get_u32();
                    break;
                case WAKE_STAT_ATTRIBUTE_RX_MULTICAST_COUNT:
                    mWakeReasonCnt->rx_wake_details.rx_multicast_cnt =
                        it.get_u32();
                    break;
                case WAKE_STAT_ATTRIBUTE_RX_BROADCAST_COUNT:
                    mWakeReasonCnt->rx_wake_details.rx_broadcast_cnt =
                        it.get_u32();
                    break;
                case WAKE_STAT_ATTRIBUTE_RX_ICMP_PKT:
                    mWakeReasonCnt->rx_wake_pkt_classification_info.icmp_pkt =
                        it.get_u32();
                    break;
                case WAKE_STAT_ATTRIBUTE_RX_ICMP6_PKT:
                    mWakeReasonCnt->rx_wake_pkt_classification_info.icmp6_pkt =
                        it.get_u32();
                    break;
                case WAKE_STAT_ATTRIBUTE_RX_ICMP6_RA:
                    mWakeReasonCnt->rx_wake_pkt_classification_info.icmp6_ra =
                        it.get_u32();
                    break;
                case WAKE_STAT_ATTRIBUTE_RX_ICMP6_NA:
                    mWakeReasonCnt->rx_wake_pkt_classification_info.icmp6_na =
                        it.get_u32();
                    break;
                case WAKE_STAT_ATTRIBUTE_RX_ICMP6_NS:
                    mWakeReasonCnt->rx_wake_pkt_classification_info.icmp6_ns =
                        it.get_u32();
                    break;
                case WAKE_STAT_ATTRIBUTE_IPV4_RX_MULTICAST_ADD_CNT:
                    mWakeReasonCnt->rx_multicast_wake_pkt_info.ipv4_rx_multicast_addr_cnt =
                        it.get_u32();
                    break;
                case WAKE_STAT_ATTRIBUTE_IPV6_RX_MULTICAST_ADD_CNT:
                    mWakeReasonCnt->rx_multicast_wake_pkt_info.ipv6_rx_multicast_addr_cnt =
                        it.get_u32();
                    break;
                case WAKE_STAT_ATTRIBUTE_OTHER_RX_MULTICAST_ADD_CNT:
                    mWakeReasonCnt->rx_multicast_wake_pkt_info.other_rx_multicast_addr_cnt =
                        it.get_u32();
                    break;
                default:
                    break;
            }

        }
        return NL_OK;
    }
};

wifi_error wifi_start_pkt_fate_monitoring(wifi_interface_handle handle)
{
    PacketFateCommand *cmd = new PacketFateCommand(handle);
    NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
    wifi_error result = (wifi_error)cmd->start();
    cmd->releaseRef();
    return result;
}

wifi_error wifi_get_tx_pkt_fates(wifi_interface_handle handle,
        wifi_tx_report *tx_report_bufs, size_t n_requested_fates,
        size_t *n_provided_fates)
{
    PacketFateCommand *cmd = new PacketFateCommand(handle, tx_report_bufs,
                n_requested_fates, n_provided_fates);
    NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
    wifi_error result = (wifi_error)cmd->start();
    cmd->releaseRef();
    return result;
}

wifi_error wifi_get_rx_pkt_fates(wifi_interface_handle handle,
        wifi_rx_report *rx_report_bufs, size_t n_requested_fates,
        size_t *n_provided_fates)
{
    PacketFateCommand *cmd = new PacketFateCommand(handle, rx_report_bufs,
                n_requested_fates, n_provided_fates);
    NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);
    wifi_error result = (wifi_error)cmd->start();
    cmd->releaseRef();
    return result;
}

wifi_error wifi_get_wake_reason_stats(wifi_interface_handle handle,
        WLAN_DRIVER_WAKE_REASON_CNT *wifi_wake_reason_cnt)
{
    GetWakeReasonCountCommand *cmd =
        new GetWakeReasonCountCommand(handle, wifi_wake_reason_cnt);
    wifi_error result = (wifi_error)cmd->start();
    cmd->releaseRef();
    return result;
}

///////////////////////////////////////////////////////////////////////////////
class OtaUpdateCommand : public WifiCommand
{
    int mErrCode;

    public:
    OtaUpdateCommand(wifi_interface_handle iface)
        : WifiCommand("OtaUpdateCommand", iface, 0), mErrCode(0)
    { }

    int start() {
        ALOGE("Start OtaUpdateCommand");
        WifiRequest request(familyId(), ifaceId());

        int result = request.create(GOOGLE_OUI, WIFI_SUBCMD_GET_OTA_CURRUNT_INFO);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to set hal start; result = %d", result);
            return result;
        }

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to register set hal start response; result = %d", result);
        }
        return result;
    }

    int otaDownload(ota_info_buf_t* buf, uint32_t ota_version) {
        u32 force_reg_on = false;
        WifiRequest request(familyId(), ifaceId());
        int result = request.create(GOOGLE_OUI, WIFI_SUBCMD_OTA_UPDATE);

        ALOGE("Download the OTA configuration");
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to set Hal preInit; result = %d", result);
            return result;
        }

        nlattr *data = request.attr_start(NL80211_ATTR_VENDOR_DATA);

        result = request.put_u32(OTA_DOWNLOAD_CLM_LENGTH_ATTR, buf->ota_clm_len);
        if (result != WIFI_SUCCESS) {
            ALOGE("otaDownload Failed to put data= %d", result);
            return result;
        }

        result = request.put(OTA_DOWNLOAD_CLM_ATTR, buf->ota_clm_buf, sizeof(*buf->ota_clm_buf));
        if (result != WIFI_SUCCESS) {
            ALOGE("otaDownload Failed to put data= %d", result);
            return result;
        }

        result = request.put_u32(OTA_DOWNLOAD_NVRAM_LENGTH_ATTR, buf->ota_nvram_len);
        if (result != WIFI_SUCCESS) {
            ALOGE("otaDownload Failed to put data= %d", result);
            return result;
        }

        result = request.put(OTA_DOWNLOAD_NVRAM_ATTR,
                buf->ota_nvram_buf, sizeof(*buf->ota_nvram_buf));
        if (result != WIFI_SUCCESS) {
            ALOGE("otaDownload Failed to put data= %d", result);
            return result;
        }

        if (applied_ota_version != ota_version) {
            force_reg_on = true;
            applied_ota_version = ota_version;
        }
        result = request.put_u32(OTA_SET_FORCE_REG_ON, force_reg_on);
        if (result != WIFI_SUCCESS) {
            ALOGE("otaDownload Failed to put data= %d", result);
            return result;
        }

        request.attr_end(data);

        result = requestResponse(request);
        if (result != WIFI_SUCCESS) {
            ALOGE("Failed to register set otaDownload; result = %d", result);
        }

        return result;
    }

    virtual int handleResponse(WifiEvent& reply) {
        ALOGD("In OtaUpdateCommand::handleResponse");

        if (reply.get_cmd() != NL80211_CMD_VENDOR) {
            ALOGD("Ignoring reply with cmd = %d", reply.get_cmd());
            return NL_SKIP;
        }

        int id = reply.get_vendor_id();
        int subcmd = reply.get_vendor_subcmd();
        nlattr *vendor_data = reply.get_attribute(NL80211_ATTR_VENDOR_DATA);
        int len = reply.get_vendor_data_len();

        ALOGI("Id = %0x, subcmd = %d, len = %d", id, subcmd, len);

        if (vendor_data == NULL || len == 0) {
            ALOGE("no vendor data in GetPktFateCommand response; ignoring it\n");
            return NL_SKIP;
        }

        for (nl_iterator it(vendor_data); it.has_next(); it.next()) {
            switch (it.get_type()) {
                case OTA_CUR_NVRAM_EXT_ATTR:
                    strncpy(ota_nvram_ext, (char*)it.get_string(), it.get_len());
                    ALOGI("Current Nvram ext [%s]\n", ota_nvram_ext);
                    break;
                default:
                    ALOGE("Ignoring invalid attribute type = %d, size = %d\n",
                            it.get_type(), it.get_len());
                    break;
            }
        }
        return NL_OK;
    }

    virtual int handleEvent(WifiEvent& event) {
        /* NO events! */
        return NL_SKIP;
    }
};

wifi_error read_ota_file(char* file, char** buffer, uint32_t* size)
{
    FILE* fp = NULL;
    int file_size, count;
    char* buf;
    fp = fopen(file, "r");

    if (fp == NULL) {
        ALOGI("File [%s] doesn't exist.", file);
        return WIFI_ERROR_NOT_AVAILABLE;
    }

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);

    buf = (char *)malloc(file_size + 1);
    if (buf == NULL) {
        fclose(fp);
        return WIFI_ERROR_UNKNOWN;
    }
    memset(buf, 0, file_size + 1);
    fseek(fp, 0, SEEK_SET);
    count = fread(buf, file_size, 1, fp);

    *buffer = (char*) buf;
    *size = file_size;
    fclose(fp);
    return WIFI_SUCCESS;
}

wifi_error check_multiple_nvram_clm(uint32_t type, char* hw_revision, char* hw_sku,
        char** buffer, uint32_t* buffer_len)
{
    char file_name[MAX_NV_FILE][FILE_NAME_LEN];
    char nvram_clmblob_default_file[FILE_NAME_LEN] = {0,};
    wifi_error result = WIFI_SUCCESS;

    if (type == CLM_BLOB) {
        sprintf(nvram_clmblob_default_file, "%s%s", OTA_PATH, OTA_CLM_FILE);
    }
    else if (type == NVRAM) {
        sprintf(nvram_clmblob_default_file, "%s%s", OTA_PATH, OTA_NVRAM_FILE);
    }
    for (unsigned int i = 0; i < MAX_NV_FILE; i++) {
        memset(file_name[i], 0, FILE_NAME_LEN);
    }

    sprintf(file_name[0], "%s_%s_%s", nvram_clmblob_default_file, hw_revision, hw_sku);
    sprintf(file_name[1], "%s_%s", nvram_clmblob_default_file, hw_revision);
    sprintf(file_name[2], "%s_%s", nvram_clmblob_default_file, hw_sku);
    sprintf(file_name[3], "%s", nvram_clmblob_default_file);

    for (unsigned int i = 0; i < MAX_NV_FILE; i++) {
        result = read_ota_file(file_name[i], buffer, buffer_len);
        if (result == WIFI_SUCCESS) {
            ALOGI("[OTA] %s PATH %s", type == NVRAM ? "NVRAM" : "CLM", file_name[i]);
            break;
        }
    }
    return result;
}

wifi_error wifi_hal_ota_update(wifi_interface_handle iface, uint32_t ota_version)
{
    wifi_handle handle = getWifiHandle(iface);
    wifi_error result = WIFI_SUCCESS;
    ota_info_buf_t buf;
    char *buffer_nvram = NULL;
    char *buffer_clm = NULL;
    char prop_revision_buf[PROPERTY_VALUE_MAX] = {0,};
    char prop_sku_buf[PROPERTY_VALUE_MAX] = {0,};
    char sku_name[MAX_SKU_NAME_LEN] = {0,};

    OtaUpdateCommand *cmd = new OtaUpdateCommand(iface);
    NULL_CHECK_RETURN(cmd, "memory allocation failure", WIFI_ERROR_OUT_OF_MEMORY);

    ALOGD("wifi_hal_ota_update, handle = %p, ota_version %d\n", handle, ota_version);

    result = (wifi_error)cmd->start();
    if (result != WIFI_SUCCESS) {
        cmd->releaseRef();
        return result;
    }

    property_get(HW_DEV_PROP, prop_revision_buf, NULL);
    property_get(HW_SKU_PROP, prop_sku_buf, NULL);

    if (strcmp(prop_sku_buf, "G9S9B") == 0 ||
        strcmp(prop_sku_buf, "G8V0U") == 0 ||
        strcmp(prop_sku_buf, "GFQM1") == 0) {
        strncpy(sku_name, "MMW", MAX_SKU_NAME_LEN);
    } else if (strcmp(prop_sku_buf, "GR1YH") == 0 ||
               strcmp(prop_sku_buf, "GF5KQ") == 0 ||
               strcmp(prop_sku_buf, "GPQ72") == 0) {
        strncpy(sku_name, "JPN", MAX_SKU_NAME_LEN);
    } else if (strcmp(prop_sku_buf, "GB7N6") == 0 ||
               strcmp(prop_sku_buf, "GLU0G") == 0 ||
               strcmp(prop_sku_buf, "GNA8F") == 0) {
        strncpy(sku_name, "ROW", MAX_SKU_NAME_LEN);
    } else {
        strncpy(sku_name, "NA", MAX_SKU_NAME_LEN);
    }

    check_multiple_nvram_clm(CLM_BLOB, prop_revision_buf, sku_name, &buffer_clm, &buf.ota_clm_len);
    if (buffer_clm == NULL) {
        ALOGE("buffer_clm is null");
        goto exit;
    }
    buf.ota_clm_buf[0] = buffer_clm;

    check_multiple_nvram_clm(NVRAM, prop_revision_buf, sku_name,
            &buffer_nvram, &buf.ota_nvram_len);
    if (buffer_nvram == NULL) {
        ALOGE("buffer_nvram is null");
        goto exit;
    }
    buf.ota_nvram_buf[0] = buffer_nvram;
    cmd->otaDownload(&buf, ota_version);

exit:
    if (buffer_clm != NULL) {
        free(buffer_clm);
    }
    if (buffer_nvram != NULL) {
        free(buffer_nvram);
    }

    cmd->releaseRef();

    return result;
}
