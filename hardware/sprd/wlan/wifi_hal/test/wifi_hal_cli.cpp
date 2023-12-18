#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <linux/prctl.h>
#include <linux/capability.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <private/android_filesystem_config.h>
#include <android/log.h>
#include <pthread.h>


#define ALOG_TAG  "WifiHAL_CLI"

#include "common.h"
#include <utils/Log.h>
#include <cutils/log.h>



/************************************************************************************
**  Constants & Macros
************************************************************************************/

#define UNUSED __attribute__((unused))
typedef void (t_console_cmd_handler) (char *p);
typedef void (t_console_cmd_handler) (char *p);

#define is_cmd(str) ((strlen(str) == strlen(cmd)) && strncmp((const char *)&cmd, str, strlen(str)) == 0)

static unsigned char main_done = 0;


void get_str(char **p, char *Buffer);
static int create_cmdjob(char *cmd);
void skip_blanks(char **p);
void bdt_log(const char *fmt_str, ...);
static void cmdjob_handler(void *param);
int get_signed_int(char **p, int DefaultValue);
int split(char dst[][80], char* str, const char* spl);
void onHotlistApFound(wifi_request_id id,unsigned num_results, wifi_scan_result *results);
void onHotlistApLost(wifi_request_id id,unsigned num_results, wifi_scan_result *results);
void onSignificantWifiChange(wifi_request_id id,unsigned num_results, wifi_significant_change_result **results);
void onScanResultsAvailable(wifi_request_id id, unsigned num_results);
void onFullScanResult(wifi_request_id id, wifi_scan_result *result, unsigned buckets_scanned);
void onScanEvent(wifi_request_id id, wifi_scan_event event);
void onLinkStatsResults(wifi_request_id id, wifi_iface_stat *iface_stat,int num_radios, wifi_radio_stat *radio_stats);

float (*cosf_method)(float);
void do_help(char UNUSED *p);
void do_quit(char UNUSED *p);

void wifi_hal_set_link_stats(char *p);
void wifi_hal_set_country_code(char *p);
void wifi_hal_get_link_stats(char *p);
void wifi_hal_clear_link_stats(char *p);
void wifi_hal_get_logger_feature_set(char *p);
void wifi_hal_get_valid_channels(char *p);
void wifi_hal_stop_gscan(char *p);
void wifi_hal_get_cached_gscan_results(char *p);
void wifi_hal_get_gscan_capabilities(char *p);
void wifi_hal_set_bssid_hotlist(char *p);
void wifi_hal_reset_bssid_hotlist(char *p);
void wifi_hal_set_significant_change_handler(char *p);
void wifi_hal_reset_significant_change_handler(char *p);
//void wifi_hal_set_ssid_white_list(char *p);
//void wifi_hal_set_gscan_roam_params(char *p);
//void wifi_hal_set_bssid_preference(char *p);
//void wifi_hal_set_bssid_blacklist(char *p);
//void wifi_hal_enable_lazy_roam(char *p);
void wifi_hal_start_gscan(char *p);
int get_gscan_capabilities_internal(wifi_gscan_capabilities *capabilities);
int get_valid_channels_internal(int band,int &num_channels,wifi_channel *channels);


int get_wifi_handle_interface();
void* event_loop_thread(void *arg);

const char* wlan0_name = "wlan0";
wifi_handle m_wifi_handle;
wifi_hal_fn hal_fn;
wifi_interface_handle * m_wifi_interface_handle;
wifi_interface_handle wlan0_interface_handle;
wifi_iface_stat link_stat;
wifi_radio_stat radio_stat; // L release has support for only one radio


interface_info m_iface_info;
static int id = 0;

//command struct
typedef struct {
    const char *name;
    t_console_cmd_handler *handler;
    const char *help;
    unsigned char is_job;
} t_cmd;

const t_cmd console_cmd_list[] =
{
    /*
     * INTERNAL
     */

    { "help", do_help, ":: lists all available console commands", 0 },
    { "quit", do_quit, ":: exit wifi-hal-cli", 0},

    /*
     * API CONSOLE COMMANDS
     */

    /* Init and Cleanup shall be called automatically */
    { "wifi_hal_set_country_code", wifi_hal_set_country_code, "command: wifi_hal_set_country_code countrycode[eg:US,CN...] ", 0 },
    { "wifi_hal_set_link_stats", wifi_hal_set_link_stats, "command: wifi_hal_set_link_stats aggressive_statistics_gathering[int] mpdu_size_threshold[int] ", 0 },
    { "wifi_hal_get_link_stats", wifi_hal_get_link_stats, "command: wifi_hal_get_link_stats [no need params]", 0 },
    { "wifi_hal_get_valid_channels", wifi_hal_get_valid_channels, "command: wifi_hal_get_valid_channels band[the band you choose,1-7]", 0 },
    { "wifi_hal_start_gscan", wifi_hal_start_gscan, "command: wifi_hal_start_gscan [no need params]", 0 },
    { "wifi_hal_stop_gscan", wifi_hal_stop_gscan, "command: wifi_hal_stop_gscan [no need params]", 0 },
    { "wifi_hal_get_cached_gscan_results", wifi_hal_get_cached_gscan_results, "command: wifi_hal_get_cached_gscan_results [no need params]", 0 },
    { "wifi_hal_get_gscan_capabilities", wifi_hal_get_gscan_capabilities, "command: wifi_hal_get_gscan_capabilities [no need params]", 0 },
    { "wifi_hal_set_bssid_hotlist", wifi_hal_set_bssid_hotlist, "command: wifi_hal_set_bssid_hotlist bssid1[mac_addr] low1[int] high1[int] bssid2[mac_addr] low2[int] high2[int] ... ", 0 },
    { "wifi_hal_reset_bssid_hotlist", wifi_hal_reset_bssid_hotlist, "command: wifi_hal_reset_bssid_hotlist [no need params]", 0 },
    { "wifi_hal_set_significant_change_handler", wifi_hal_set_significant_change_handler, "wifi_hal_set_significant_change_handler bssid1[mac_addr] low1[int] high1[int] bssid2[mac_addr] low2[int] high2[int] ... ", 0 },
    { "wifi_hal_reset_significant_change_handler", wifi_hal_reset_significant_change_handler, "command: wifi_hal_reset_significant_change_handler [no need params]", 0 },
    //{ "wifi_hal_set_ssid_white_list", wifi_hal_set_ssid_white_list, "command: wifi_hal_set_ssid_white_list ssid1[char *] ssid2[char *]... ", 0 },
    //{ "wifi_hal_set_gscan_roam_params", wifi_hal_set_gscan_roam_params, "command: wifi_hal_set_gscan_roam_params A_band_boost_threshold[int] A_band_penalty_threshold[int]", 0 },
    //{ "wifi_hal_set_bssid_preference", wifi_hal_set_bssid_preference, "command: wifi_hal_set_bssid_preference bssid1[mac_addr] rssi_modifier1[int] bssid2[mac_addr] rssi_modifier2[int] ", 0 },
    //{ "wifi_hal_set_bssid_blacklist", wifi_hal_set_bssid_blacklist, "command: wifi_hal_set_bssid_blacklist bssid1[mac_addr] bssid2[mac_addr] ... ", 0 },
    //{ "wifi_hal_enable_lazy_roam", wifi_hal_enable_lazy_roam, "command: wifi_hal_enable_lazy_roam eable[0 or 1] ", 0 },
    { "wifi_hal_clear_link_stats", wifi_hal_clear_link_stats, "wifi_hal_clear_link_stats [no need params]", 0 },
    { "wifi_hal_get_logger_feature_set", wifi_hal_get_logger_feature_set, "wifi_hal_get_logger_feature_set[no need params]", 0 },

    /* add here */

    /* last entry */
    {NULL, NULL, "", 0},
};

int init_wifi_hal_func_table(wifi_hal_fn *hal_fn) {
    if (hal_fn == NULL) {
        return -1;
    }
    return 0;
}

int wifi_hal_init() {
    bdt_log("wifi_hal_init");
    if (m_wifi_handle == NULL) {
        wifi_error res = init_wifi_vendor_hal_func_table(&hal_fn);
        if (res != WIFI_SUCCESS) {
            bdt_log("Can not initialize the vendor function pointer table");
            return -1;
        }
        res = hal_fn.wifi_initialize(&m_wifi_handle);
        if (res != WIFI_SUCCESS) {
            return -1;
        }
        get_wifi_handle_interface();
        return 0;
    } else {
        return 0;
    }
}

int get_wifi_handle_interface() {

    char *temp_name;
    int num = 0;
    temp_name = (char *)malloc(IFNAMSIZ);

    hal_fn.wifi_get_ifaces(m_wifi_handle, &num, &m_wifi_interface_handle);
    bdt_log("interface_num %d \n",num);
    for (int i = 0; i < num; i++) {
        memset(temp_name, 0, IFNAMSIZ);
        hal_fn.wifi_get_iface_name(m_wifi_interface_handle[i], temp_name, IFNAMSIZ);
        bdt_log("wlan0_name = %s, temp_name = %s, strcmp result = %d \n", wlan0_name, temp_name, strcmp(wlan0_name, temp_name));
        if (strcmp(wlan0_name, temp_name) == 0) {
            wlan0_interface_handle = m_wifi_interface_handle[i];
        }
    }
    bdt_log("wifi_handle_interface");
    free(temp_name);
    return 1;
}

void wifi_hal_set_country_code(char *p) {

    char param[10][80] = {0};  //store input params
    int param_num = 0;
    param_num = split(param, p, " ");
    /*for (int i = 0; i < param_num; i++) {
        puts(param[i]);
    }*/
    if (param_num < 1) {
        bdt_log("please input like this: wifi_hal_set_country_code countrycode \n");
        return;
    }
    //const char * country = "US";
    wifi_error result = hal_fn.wifi_set_country_code(wlan0_interface_handle, param[0]);
    bdt_log("wifi_hal_set_country_code result = %d", result);
    if (result < 0) {
        bdt_log("wifi_hal_set_country_code failed \n");
    }
}

void wifi_hal_set_link_stats(char *p) {

    wifi_link_layer_params params;
    char param[10][80] = {0};
    int param_num = 0;

    param_num = split(param, p, " ");
    /*for (int i = 0; i < param_num; i++) {
        puts(param[i]);
    }*/
    if (param_num < 2) {
        bdt_log("please input like this:wifi_hal_set_link_stats aggressive_statistics_gathering mpdu_size_threshold");
        return;
    }

    params.aggressive_statistics_gathering = strtoul(param[0], NULL, 10);
    bdt_log("params.aggressive_statistics_gathering = %u ", params.aggressive_statistics_gathering);
    params.mpdu_size_threshold = strtoul(param[1], NULL, 10);
    bdt_log("params.mpdu_size_threshold = %u ", params.mpdu_size_threshold);

    wifi_error result = hal_fn.wifi_set_link_stats(wlan0_interface_handle, params);
    bdt_log("wifi_hal_set_link_stats result = %d", result);
    if (result < 0) {
        bdt_log("wifi_hal set link stats failed \n");
    }
}

void wifi_hal_get_link_stats(char *p) {

    wifi_stats_result_handler handler;
    memset(&handler, 0, sizeof(handler));

    handler.on_link_stats_results = &onLinkStatsResults;
    id++;
    wifi_error result = hal_fn.wifi_get_link_stats(id, wlan0_interface_handle, handler);
    bdt_log("wifi_hal_get_link_stats result = %d ", result);
    if (result < 0) {
        bdt_log("wifi_hal get link stats failed \n");
        return;
    }
    bdt_log("wifi_hal_get_link_stats: \n radio_stat.on_time=%u \n radio_stat.tx_time=%u \n radio_stat.rx_time=%u \n radio_stat.on_time_scan=%u",
        radio_stat.on_time,radio_stat.tx_time,radio_stat.rx_time,radio_stat.on_time_scan);
    bdt_log(" link_stat.beacon_rx = %u \n link_stat.rssi_mgmt = %d \n link_stat.ac[WIFI_AC_BE].rx_mpdu = %u \n \
link_stat.ac[WIFI_AC_BK].rx_mpdu = %u \n link_stat.ac[WIFI_AC_VI].rx_mpdu = %u \n link_stat.ac[WIFI_AC_VO].rx_mpdu = %u \n link_stat.ac[WIFI_AC_BE].tx_mpdu = %u \n \
link_stat.ac[WIFI_AC_BK].tx_mpdu = %u \n link_stat.ac[WIFI_AC_VI].tx_mpdu = %u \n link_stat.ac[WIFI_AC_VO].tx_mpdu = %u \n link_stat.ac[WIFI_AC_BE].mpdu_lost = %u \n \
link_stat.ac[WIFI_AC_BK].mpdu_lost = %u \n link_stat.ac[WIFI_AC_VI].mpdu_lost = %u \n link_stat.ac[WIFI_AC_VO].mpdu_lost = %u \n link_stat.ac[WIFI_AC_BE].retries = %u \n \
link_stat.ac[WIFI_AC_BK].retries = %u \n link_stat.ac[WIFI_AC_VI].retries = %u \n link_stat.ac[WIFI_AC_VO].retries = %u \n",
link_stat.beacon_rx, link_stat.rssi_mgmt, link_stat.ac[WIFI_AC_BE].rx_mpdu, link_stat.ac[WIFI_AC_BK].rx_mpdu, link_stat.ac[WIFI_AC_VI].rx_mpdu,
link_stat.ac[WIFI_AC_VO].rx_mpdu, link_stat.ac[WIFI_AC_BE].tx_mpdu, link_stat.ac[WIFI_AC_BK].tx_mpdu, link_stat.ac[WIFI_AC_VI].tx_mpdu, link_stat.ac[WIFI_AC_VO].tx_mpdu,
link_stat.ac[WIFI_AC_BE].mpdu_lost, link_stat.ac[WIFI_AC_BK].mpdu_lost, link_stat.ac[WIFI_AC_VI].mpdu_lost, link_stat.ac[WIFI_AC_VO].mpdu_lost, link_stat.ac[WIFI_AC_BE].retries,
link_stat.ac[WIFI_AC_BK].retries, link_stat.ac[WIFI_AC_VI].retries, link_stat.ac[WIFI_AC_VO].retries);
}

void wifi_hal_clear_link_stats(char *p) {

    u32 stats_clear_req_mask = 0;
    u32 *stats_clear_rsp_mask = (u32 *)malloc(sizeof(u32));
    memset(stats_clear_rsp_mask, 0, sizeof(u32));
    *stats_clear_rsp_mask = 0;
    u8 stop_req = 'C';         //default value
    u8 stop_rsp[20] = "test";

    wifi_error result = hal_fn.wifi_clear_link_stats(wlan0_interface_handle, stats_clear_req_mask, stats_clear_rsp_mask, stop_req, stop_rsp);
    if (result < 0) {
        bdt_log("wifi_hal clear link stats failed \n");
    }
    free(stats_clear_rsp_mask);
}

void wifi_hal_get_logger_feature_set(char *nothing) {

    u32 set = 1234;
    bdt_log("%s before get logger value is %d\n", __func__, set);

    wifi_error result = hal_fn.wifi_get_logger_supported_feature_set(wlan0_interface_handle, &set);
    if (result < 0) {
        bdt_log("wifi_hal clear link stats failed \n");
    }
    bdt_log("%s after get logger value is %d\n", __func__, set);
}
void wifi_hal_get_valid_channels(char *p) {

    char param[10][80] = {0};
    int param_num = 0;
    wifi_channel channels[64];
    int num_channels = 0;
    int band = -1;
    static const int MaxChannels = 64;

    memset(channels, 0, sizeof(wifi_channel)*64);

    param_num = split(param, p, " ");
    /*for (int i = 0; i < param_num; i++) {
        puts(param[i]);
    }*/
    if(param_num < 1) {
        bdt_log("please input like this:wifi_hal_get_valid_channels band[the band you choose] \n");
        return;
    }
    char *temp2 = param[0];
    band = get_signed_int(&temp2, -1);

    bdt_log("wifi_hal_get_valid_channels  band=%d", band);
    wifi_error result = hal_fn.wifi_get_valid_channels(wlan0_interface_handle, band, MaxChannels, channels, &num_channels);
    bdt_log("wifi_hal_get_valid_channels result = %d", result);
    if (result < 0) {
        bdt_log("wifi_hal_get_valid_channels failed \n");
        return;
    }
    bdt_log("wifi_hal_get_valid_channels band = %d, num_channels=%d \n channels=", band, num_channels);
    for (int i=0; i < num_channels; i++) {
        bdt_log("channels[i] = %d ", channels[i]);
    }
}

int get_valid_channels_internal(int band, int &num_channels, wifi_channel *channels) {

    static const int MaxChannels = 64;
    bdt_log("wifi_hal_get_valid_channels  band = %d, num_channels = %d", band, num_channels);
    wifi_error result = hal_fn.wifi_get_valid_channels(wlan0_interface_handle, band, MaxChannels, channels, &num_channels);
    bdt_log("wifi_hal_get_valid_channels result = %d", result);
    if (result < 0) {
        bdt_log("wifi_hal_get_valid_channels failed \n");
        return 0;
    }
    bdt_log("wifi_hal_get_valid_channels band = %d, num_channels = %d \n channels = ", band, num_channels);
    for (int i = 0; i < num_channels; i++) {
        bdt_log("channels[i] = %d ", channels[i]);
    }
    return 1;
}


void wifi_hal_start_gscan(char *p) {

    wifi_scan_cmd_params *params;
    wifi_scan_result_handler handler;
    wifi_gscan_capabilities *capabilities;
    int is_correct = 0;
    int band = 1;
    wifi_channel channels[64];
    int num_channels = 0;

    params = (wifi_scan_cmd_params *)malloc(sizeof(wifi_scan_cmd_params));
    memset(params, 0, sizeof(wifi_scan_cmd_params));
    capabilities = (wifi_gscan_capabilities *)malloc(sizeof(wifi_gscan_capabilities));
    memset(capabilities, 0, sizeof(wifi_gscan_capabilities));
    memset(channels, 0, sizeof(wifi_channel)*64);

    params->base_period = 5;  //default value
    params->report_threshold_percent = 100;
    params->report_threshold_num_scans = 10;
    params->num_buckets = 1;
    params->buckets[0].bucket = 0;
    params->buckets[0].band = WIFI_BAND_BG;
    params->buckets[0].period = 5;
    params->buckets[0].report_events = REPORT_EVENTS_EACH_SCAN;
    params->buckets[0].max_period = 0;
    params->buckets[0].base = 0;
    params->buckets[0].step_count = 0;

    is_correct = get_gscan_capabilities_internal(capabilities);
    if(is_correct < 1) {
        bdt_log("In wifi_hal_start_gscan:wifi_hal get gscan capabilities failed \n");
        free(params);
        free(capabilities);
        return;
    }
    params->max_ap_per_scan = capabilities->max_ap_cache_per_scan;

    is_correct = get_valid_channels_internal(band, num_channels, channels);
    if (is_correct < 1) {
        bdt_log("In wifi_hal_start_gscan:wifi_hal get valid channels failed \n");
        free(params);
        free(capabilities);
        return;
    }
    params->buckets[0].num_channels = num_channels;
    for (int i = 0; i < num_channels; i++) {
        params->buckets[0].channels[i].channel = channels[i];
        params->buckets[0].channels[i].dwellTimeMs = 0;
        params->buckets[0].channels[i].passive = 0;
        bdt_log("channel = %d", channels[i]);
    }

    memset(&handler, 0, sizeof(handler));
    //handler.on_scan_results_available = &onScanResultsAvailable;
    handler.on_full_scan_result = &onFullScanResult;
    handler.on_scan_event = &onScanEvent;

    id++;
    wifi_error result = hal_fn.wifi_start_gscan(id, wlan0_interface_handle, *params, handler);
    bdt_log("wifi_hal_stop_gscan result = %d ", result);
    if (result < 0) {
        bdt_log("wifi_hal start gscan failed \n");
    }
    free(params);
    free(capabilities);
}
void wifi_hal_stop_gscan(char *p) {

    id++;
    wifi_error result = hal_fn.wifi_stop_gscan(id, wlan0_interface_handle);
    bdt_log("wifi_hal_stop_gscan result = %d", result);
    if (result < 0) {
        bdt_log("wifi_hal stop gscan failed \n");
    } else {
        id = 0;
    }
}

void wifi_hal_get_cached_gscan_results(char *p) {

    wifi_cached_scan_results scan_data[64];
    int num_scan_data = 64;
    byte b = true ? 0xFF : 0;

    wifi_error result = hal_fn.wifi_get_cached_gscan_results(wlan0_interface_handle, b, num_scan_data, scan_data, &num_scan_data);
    bdt_log("wifi_hal_get_cached_gscan_results result = %d", result);

    if (result < 0) {
        bdt_log("wifi_hal_get_cached_gscan_results failed \n");
        return;
    }
    bdt_log("wifi_hal_get_cached_gscan_results num_scan_data = %d", num_scan_data);
    for (int i = 0; i < num_scan_data; i++) {
        bdt_log("scan_data[%d]:    scan_id = %d    flags=%d num_results = %d \n wifi_scan_result:\n",i,scan_data[i].scan_id,scan_data[i].flags,scan_data[i].num_results);
        for (int j = 0; j < scan_data[i].num_results; j++) {
            bdt_log("wifi_timestamp = %lld \n ssid = %s \n bssid = %s \n channel = %d \n rssi = %d \n rtt = %lld \n \
                rtt_sd = %lld \n beacon_period = %u \n capability = %u \n ie_length = %u \n ie_data = %s \n",
                scan_data[i].results[j].ts, scan_data[i].results[j].ssid, scan_data[i].results[j].bssid, scan_data[i].results[j].channel,
                scan_data[i].results[j].rssi, scan_data[i].results[j].rtt, scan_data[i].results[j].rtt_sd, scan_data[i].results[j].beacon_period,
                scan_data[i].results[j].capability, scan_data[i].results[j].ie_length, scan_data[i].results[j].ie_data);
        }
    }
}

void wifi_hal_get_gscan_capabilities(char *p) {

    wifi_gscan_capabilities  *capabilities = (wifi_gscan_capabilities *)malloc(sizeof(wifi_gscan_capabilities));
    memset(capabilities, 0, sizeof(wifi_gscan_capabilities));

    wifi_error result = hal_fn.wifi_get_gscan_capabilities(wlan0_interface_handle, capabilities);
    bdt_log("wifi_hal_get_gscan_capabilities result = %d", result);

    if (result < 0) {
        bdt_log("wifi_hal_get_gscan_capabilities failed \n");
        free(capabilities);
        return;
    }
    bdt_log("wifi_hal_get_gscan_capabilities capabilities:\n max_scan_cache_size = %d \n max_scan_buckets = %d \n max_ap_cache_per_scan = %d \n \
max_rssi_sample_size = %d \n max_scan_reporting_threshold = %d \n max_hotlist_bssids = %d \n max_hotlist_ssids = %d \n \
max_significant_wifi_change_aps = %d \n max_bssid_history_entries = %d \n max_number_epno_networks = %d \n max_number_epno_networks_by_ssid = %d \n \
max_number_of_white_listed_ssid = %d",
capabilities->max_scan_cache_size,capabilities->max_scan_buckets,capabilities->max_ap_cache_per_scan,
capabilities->max_rssi_sample_size,capabilities->max_scan_reporting_threshold,capabilities->max_hotlist_bssids,capabilities->max_hotlist_ssids,
capabilities->max_significant_wifi_change_aps,capabilities->max_bssid_history_entries,capabilities->max_number_epno_networks,capabilities->max_number_epno_networks_by_ssid,
capabilities->max_number_of_white_listed_ssid);

    free(capabilities);
}

int get_gscan_capabilities_internal(wifi_gscan_capabilities *capabilities) {

    wifi_error result = hal_fn.wifi_get_gscan_capabilities(wlan0_interface_handle, capabilities);
    bdt_log("wifi_hal_get_gscan_capabilities result = %d", result);

    if (result < 0) {
        bdt_log("Int get_gscan_capabilities_internal wifi_hal_get_gscan_capabilities failed \n");
        return 0;
    }

    bdt_log("wifi_hal_get_gscan_capabilities capabilities:\n max_scan_cache_size = %d \n max_scan_buckets = %d \n max_ap_cache_per_scan = %d \n \
max_rssi_sample_size = %d \n max_scan_reporting_threshold = %d \n max_hotlist_bssids = %d \n max_hotlist_ssids = %d \n \
max_significant_wifi_change_aps = %d \n max_bssid_history_entries = %d \n max_number_epno_networks = %d \n max_number_epno_networks_by_ssid = %d \n \
max_number_of_white_listed_ssid = %d",
capabilities->max_scan_cache_size, capabilities->max_scan_buckets, capabilities->max_ap_cache_per_scan,
capabilities->max_rssi_sample_size, capabilities->max_scan_reporting_threshold, capabilities->max_hotlist_bssids, capabilities->max_hotlist_ssids,
capabilities->max_significant_wifi_change_aps, capabilities->max_bssid_history_entries, capabilities->max_number_epno_networks, capabilities->max_number_epno_networks_by_ssid,
capabilities->max_number_of_white_listed_ssid);

    return 1;
}


void wifi_hal_set_bssid_hotlist(char *p) {

    wifi_bssid_hotlist_params *params;

    char *temp;
    int j = 0;
    int param_num = 0;
    char param[20][80] = {0};

    params = (wifi_bssid_hotlist_params *)malloc(sizeof(wifi_bssid_hotlist_params));
    memset(params, 0, sizeof(wifi_bssid_hotlist_params));
    param_num = split(param, p, " ");
    if (param_num <= 0 || param_num%3 != 0) {
        bdt_log("please input like this:wifi_hal_set_bssid_hotlist bssid1 low1 high1 bssid2 low2 high2 ...\n");
        free(params);
        return;
    }

    params->num_bssid = param_num/3;
    for (int i = 0; i < param_num; i++) {
        puts(param[i]);
        if(i%3 == 0){
            memcpy(params->ap[j].bssid, param[i], strlen(param[i]));
        } else if (i%3 == 1) {
            temp = (char *)malloc(sizeof(char));
            temp = param[i];
            params->ap[j].low = get_signed_int(&temp,-128);
        } else {
            temp = (char *)malloc(sizeof(char));
            temp = param[i];
            params->ap[j].high= get_signed_int(&temp,0);
            j++;
        }
    }
    wifi_hotlist_ap_found_handler handler;
    memset(&handler, 0, sizeof(handler));
    handler.on_hotlist_ap_found = &onHotlistApFound;
    handler.on_hotlist_ap_lost  = &onHotlistApLost;

    id++;
    wifi_error result = hal_fn.wifi_set_bssid_hotlist(id, wlan0_interface_handle, *params, handler);
    bdt_log("wifi_hal_set_bssid_hotlist result = %d", result);

    if (result < 0) {
        bdt_log("wifi_hal set bssid hotlist failed \n");
    }
    free(params);
}

void wifi_hal_reset_bssid_hotlist(char *p) {

    id++;
    wifi_error result = hal_fn.wifi_reset_bssid_hotlist(id, wlan0_interface_handle);
    bdt_log("wifi_hal_reset_bssid_hotlist result = %d", result);

    if (result < 0) {
        bdt_log("wifi_hal reset bssid hotlist failed \n");
    }
}

void wifi_hal_set_significant_change_handler(char *p) {

    wifi_significant_change_params *params;
    wifi_significant_change_handler handler;

    params = (wifi_significant_change_params *)malloc(sizeof(wifi_significant_change_params));
    memset(params, 0, sizeof(wifi_significant_change_params));
    params->rssi_sample_size = 3;
    params->lost_ap_sample_size = 3;
    params->min_breaching = 3;
    char *temp;
    int param_num = 0;
    char param[30][80] = {0};
    int j = 0;
    param_num = split(param, p , " ");

    if (param_num <= 0 || param_num%3 != 0) {
        bdt_log("please input like this:wifi_hal_set_significant_change_handler bssid1 low1 high1 bssid2 low2 high2 ...\n");
        free(params);
        return;
    }
    params->num_bssid = param_num/3;
    for (int i = 0; i < param_num; i++) {
        puts(param[i]);
        if (i%3 == 0) {
            memcpy(params->ap[j].bssid, param[i], strlen(param[i]));
        } else if (i%3 == 1) {
            temp = (char *)malloc(sizeof(char));
            temp = param[i];
            params->ap[j].low = get_signed_int(&temp, -128);
        } else {
            temp = (char *)malloc(sizeof(char));
            temp = param[i];
            params->ap[j].high= get_signed_int(&temp, 0);
            j++;
        }
    }

    memset(&handler, 0, sizeof(handler));
    handler.on_significant_change = &onSignificantWifiChange;

    id++;
    wifi_error result = hal_fn.wifi_set_significant_change_handler(id, wlan0_interface_handle, *params, handler);
    bdt_log("wifi_hal_set_significant_change_handler result = %d \n", result);

    if (result < 0) {
        bdt_log("wifi_hal set significant change handler failed \n");
    }
    free(params);
}

void wifi_hal_reset_significant_change_handler(char *p) {

    id++;
    wifi_error result = hal_fn.wifi_reset_significant_change_handler(id, wlan0_interface_handle);
    bdt_log("wifi_hal_reset_significant_change_handler result = %d ", result);
    if (result < 0) {
        bdt_log("wifi_hal reset significant change handler failed \n");
    }
}

/*
void wifi_hal_set_ssid_white_list(char *p) {

    wifi_ssid *ssids;
    char param[10][80] = {0};
    int param_num = 0;
    param_num = split(param, p, " ");

    ssids = (wifi_ssid *)malloc(param_num * sizeof (wifi_ssid));
    memset(ssids, 0, param_num * sizeof (wifi_ssid));
    //for (int i = 0; i < param_num; i++) {
    //    puts(param[i]);
    //}
    if (param_num <= 0) {
        bdt_log("please input like this:wifi_hal_set_ssid_white_list ssid1 ssid2... \n");
        free(ssids);
        return;
    }
    for (int i=0; i < param_num; i++) {
        memcpy(ssids[i].ssid, param[i], strlen(param[i]));
        puts(ssids[i].ssid);
    }
    id++;
    wifi_error result = hal_fn.wifi_set_ssid_white_list(id, wlan0_interface_handle, param_num, ssids);
    bdt_log("wifi_hal_set_ssid_white_list result = %d", result);
    if (result < 0) {
        bdt_log("wifi_hal set ssid white list failed \n");
    }
    free(ssids);
}

void wifi_hal_set_gscan_roam_params(char *p) {

    wifi_roam_params * params;
    char param[10][80] = {0};
    int param_num = 0;
    params = (wifi_roam_params *)malloc(sizeof(wifi_roam_params));
    memset(params, 0, sizeof(wifi_roam_params));
    params->A_band_boost_factor = 5;
    params->A_band_penalty_factor = 2;
    params->A_band_max_boost = 65;
    params->lazy_roam_hysteresis = 25;
    params->alert_roam_rssi_trigger = -75;

    param_num = split(param, p, " ");
    //for (int i = 0; i < param_num; i++) {
    //    puts(param[i]);
    //}
    if (param_num < 2) {
        bdt_log("please input like this:wifi_hal_set_gscan_roam_params A_band_boost_threshold A_band_penalty_threshold \n");
        free(params);
        return;
    }
    char *temp=param[0];
    char *temp1=param[1];
    params->A_band_boost_threshold = get_signed_int(&temp, -1);
    params->A_band_penalty_threshold = get_signed_int(&temp1, -1);
    bdt_log("A_band_boost_threshold=%d \nA_band_penalty_threshold=%d", params->A_band_boost_threshold, params->A_band_penalty_threshold);

    id++;
    wifi_error result = hal_fn.wifi_set_gscan_roam_params(id, wlan0_interface_handle, params);
    bdt_log("wifi_hal_set_gscan_roam_params result = %d", result);
    if (result < 0) {
        bdt_log("wifi_hal set gscan roam params failed \n");
    }
    free(params);
}

void wifi_hal_set_bssid_preference(char *p) {

    char *temp;
    wifi_bssid_preference *prefs;
    int num_bssid = 0;
    int j=0;
    char param[10][80] = {0};

    num_bssid = split(param, p, " ");
    prefs = (wifi_bssid_preference *)malloc(num_bssid * sizeof (wifi_bssid_preference));
    memset(prefs, 0, num_bssid * sizeof (wifi_bssid_preference));
    if (num_bssid <= 0 || num_bssid%2 != 0) {
        bdt_log("please input like this:wifi_hal_set_gscan_roam_params bssid1 rssi_modifier1 bssid2 rssi_modifier2 ... \n");
        free(prefs);
        return;
    }
    bdt_log("num_bssid = %d", num_bssid/2);
    for (int i = 0; i < num_bssid; i++) {
        //puts(param[i]);
        if (i%2 == 0) {
            memcpy(prefs[j].bssid,param[i], strlen(param[i]));
        } else {
            temp = (char *)malloc(sizeof(char));
            temp = param[i];
            prefs[j].rssi_modifier = get_signed_int(&temp, -1);
            bdt_log("prefs[%d].rssi_modifier=%d", j, prefs[j].rssi_modifier);
            j++;
        }
    }
    int num = num_bssid/2;

    id++;
    wifi_error result = hal_fn.wifi_set_bssid_preference(id, wlan0_interface_handle, num, prefs);
    bdt_log("wifi_hal_set_bssid_preference result = %d", result);
    if (result < 0) {
        bdt_log("wifi_hal set bssid preference failed \n");
    }
    free(prefs);
}

void wifi_hal_set_bssid_blacklist(char *p) {

    wifi_bssid_params *params;
    params = (wifi_bssid_params *)malloc(sizeof(wifi_bssid_params));
    memset(params, 0, sizeof(wifi_bssid_params));
    char param[10][80] = {0};

    params->num_bssid= split(param, p, " ");
    if (params->num_bssid <= 0) {
        bdt_log("please input like this:wifi_hal_set_bssid_blacklist bssid1 bssid2 ...\n");
        free(params);
        return;
    }
    for (int i = 0; i < params->num_bssid; i++) {
        puts(param[i]);
        memcpy(params->bssids[i], param[i], strlen(param[i]));
    }
    id++;
    wifi_error result = hal_fn.wifi_set_bssid_blacklist(id, wlan0_interface_handle, *params);
    bdt_log("wifi_hal_set_bssid_blacklist result = %d", result);
    if (result < 0) {
        bdt_log("wifi_hal set bssid blacklist failed \n");
    }
    free(params);
}

void wifi_hal_enable_lazy_roam(char *p) {

    int param_num = 0;
    char *temp;
    char param[10][80] = {0};

    param_num = split(param, p, " ");
    if (param_num <= 0) {
        bdt_log("please input like this:wifi_hal_enable_lazy_roam eable(0 or 1)\n");
        return;
    }
    temp = param[0];
    int enable = get_signed_int(&temp,-1);
    bdt_log("enable = %d ", enable);
    id++;
    wifi_error result = hal_fn.wifi_enable_lazy_roam(id, wlan0_interface_handle, enable);
    bdt_log("wifi_hal_enable_lazy_roam result = %d", result);
    if (result < 0) {
        bdt_log("wifi_hal enable lazy roam failed \n");
    }
}
*/

void do_help(char UNUSED *p) {

    int i = 0;
    int max = 0;
    char line[128];
    int pos = 0;

    while (console_cmd_list[i].name != NULL) {
        pos = sprintf(line, "%s", (char*)console_cmd_list[i].name);
        printf("%s %s\n", (char*)line, (char*)console_cmd_list[i].help);
        i++;
    }
}

void do_quit(char UNUSED *p) {
    main_done = 1;
}

static void process_cmd(char *p, unsigned char is_job) {

    char cmd[64];
    int i = 0;
    char *p_saved = p;

    get_str(&p, cmd);
    /* table commands */
    while (console_cmd_list[i].name != NULL) {
        if (is_cmd(console_cmd_list[i].name)) {
            if (!is_job && console_cmd_list[i].is_job) {
                create_cmdjob(p_saved);
            } else {
                console_cmd_list[i].handler(p);
            }
            return;
        }
        i++;
    }
    bdt_log("Unknown command :'%s',please input 'help' to get command list.\n", p_saved);
}

int main (int UNUSED argc, char UNUSED *argv[]){
    int opt;
    char cmd[128];
    int args_processed = 0;
    int pid = -1;
    pthread_t th;
    int ret;

    bdt_log(":::::::::::::::::::::::::::::::::::::::::::::::::::");
    bdt_log(":: wifi_hal_cli starting ::\n");

    if (wifi_hal_init() < 0) {
        perror("HAL failed to initialize, exit\n");
        exit(0);
    }

    ret = pthread_create(&th, NULL, event_loop_thread, &m_wifi_handle);
    if (ret != 0) {
        bdt_log( "Create thread error!\n");
        return -1;
    }

    while (!main_done) {
        char line[128];

        /* command prompt */
        printf( ">" );
        fflush(stdout);

        fgets(line, 128, stdin);

        if (line[0] != '\0') {
            /* remove linefeed */
            line[strlen(line)-1] = 0;
            process_cmd(line, 0);
            memset(line, '\0', 128);
        }
    }

    bdt_log(":: wifi_hal_cli terminating \n");
    return 0;
}

void get_str (char **p, char *Buffer) {

    skip_blanks(p);
    while (**p != 0 && **p != ' ') {
        *Buffer = **p;
        (*p)++;
        Buffer++;
    }
    *Buffer = 0;
}

/*****************************************************************************
**   store params to dst
*****************************************************************************/
int split(char dst[][80], char* str, const char* spl) {

    skip_blanks(&str);
    int n = 0;
    char *result = NULL;
    result = strtok(str, spl);
    while(result != NULL) {
        strcpy(dst[n++], result);
        result = strtok(NULL, spl);
    }
    return n;
}

static int create_cmdjob(char *cmd) {

    pthread_t thread_id;
    char *job_cmd;

    job_cmd = (char*)malloc(strlen(cmd)+1); /* freed in job handler */
    strcpy(job_cmd, cmd);

    if (pthread_create(&thread_id, NULL,
                       (void* (*)(void*))cmdjob_handler, (void*)job_cmd)!=0)
        perror("pthread_create");

    return 0;
}

void skip_blanks(char **p) {
    while (**p == ' ')
        (*p)++;
}

static void cmdjob_handler(void *param) {

    char *job_cmd = (char*)param;

    bdt_log("cmdjob starting (%s)", job_cmd);
    process_cmd(job_cmd, 1);
    bdt_log("cmdjob terminating");

    free(job_cmd);
}

int get_signed_int(char **p, int DefaultValue) {
    int Value = 0;
    unsigned char  UseDefault;
    unsigned char  NegativeNum = 0;

    UseDefault = 1;
    skip_blanks(p);

    if ((**p) == '-') {
        NegativeNum = 1;
        (*p)++;
    }
    while (((**p)<= '9' && (**p)>= '0')) {
        Value = Value * 10 + (**p) - '0';
        UseDefault = 0;
        (*p)++;
    }

    if (UseDefault) {
        return DefaultValue;
    } else {
        return ((NegativeNum == 0)? Value : -Value);
    }
}

/*****************************************************************************
**   Logger API
*****************************************************************************/
void bdt_log(const char *fmt_str, ...) {
    static char buffer[1024];
    va_list ap;
    va_start(ap, fmt_str);
    vsnprintf(buffer, 1024, fmt_str, ap);
    va_end(ap);
    fprintf(stdout, "%s\n", buffer);
}

/*****************************************************************************
**   Handler for wifi_get_link_stats
*****************************************************************************/
void onLinkStatsResults(wifi_request_id id, wifi_iface_stat *iface_stat,
         int num_radios, wifi_radio_stat *radio_stats) {
    if (iface_stat != 0) {
        memcpy(&link_stat, iface_stat, sizeof(wifi_iface_stat));
    } else {
        memset(&link_stat, 0, sizeof(wifi_iface_stat));
    }

    if (num_radios > 0 && radio_stats != 0) {
        memcpy(&radio_stat, radio_stats, sizeof(wifi_radio_stat));
    } else {
        memset(&radio_stat, 0, sizeof(wifi_radio_stat));
    }
}

/*****************************************************************************
**   Handler for wifi_set_bssid_hotlist
*****************************************************************************/
void onHotlistApFound(wifi_request_id id,unsigned num_results, wifi_scan_result *results) {

    ALOGI("onHotlistApFound called num_results = %d \n", num_results);
    ALOGI("cout scan_result: \n");
    for (unsigned i = 0; i < num_results; i++) {
        ALOGI("results[%d]: wifi_timestamp=%lld, ssid=%s, bssid=%s, channel=%d, rssi=%d, \
rtt=%lld, rtt_sd=%lld, beacon_period=%u, capability=%u, ie_length=%u, ie_data=%s \n",
i, results[i].ts, results[i].ssid, results[i].bssid, results[i].channel, results[i].rssi,
results[i].rtt, results[i].rtt_sd, results[i].beacon_period, results[i].capability, results[i].ie_length, results[i].ie_data);
    }
}
void onHotlistApLost(wifi_request_id id,unsigned num_results, wifi_scan_result *results) {

    ALOGI("onHotlistApLost called num_results = %d \n", num_results);
    ALOGI("cout scan_result: \n");
    for (unsigned i = 0; i < num_results; i++) {
        ALOGI("results[%d]: wifi_timestamp=%lld, ssid=%s, bssid=%s, channel=%d, rssi=%d, \
rtt=%lld, rtt_sd=%lld, beacon_period=%u, capability=%u, ie_length=%u, ie_data=%s \n",
i, results[i].ts, results[i].ssid, results[i].bssid, results[i].channel, results[i].rssi,
results[i].rtt, results[i].rtt_sd, results[i].beacon_period, results[i].capability, results[i].ie_length, results[i].ie_data);
    }
}

/*****************************************************************************
**   Handler for wifi_set_significant_change_handler
*****************************************************************************/
void onSignificantWifiChange(wifi_request_id id,unsigned num_results, wifi_significant_change_result **results) {

    ALOGI("onSignificantWifiChange called num_results = %d \n", num_results);
    ALOGI("cout wifi_significant_change_result: \n");
    for (unsigned i = 0; i < num_results; i++) {
        wifi_significant_change_result &result = *(results[i]);
        ALOGI("result[%d]:\n bssid=%s\n channel=%d \n num_rssi=%d \n rssi[0]=%d \n", i, result.bssid, result.channel, result.num_rssi, result.rssi[0]);
    }
}

/*****************************************************************************
**   Handler for wifi_start_gscan
*****************************************************************************/
void onScanResultsAvailable(wifi_request_id id, unsigned num_results) {
    ALOGI("onScanResultsAvailable wifi_request_id = %d \n num_results=%u", id, num_results);
}
void onFullScanResult(wifi_request_id id, wifi_scan_result *result, unsigned buckets_scanned){

    bdt_log("onFullScanResult wifi_request_id = %d, buckets_scanned=%u \n", id, buckets_scanned);
    bdt_log("results[%d]: wifi_timestamp=%lld,ssid=%s,bssid=%s,channel=%d,rssi=%d, \
rtt=%lld,rtt_sd=%lld,beacon_period=%u,capability=%u,ie_length=%u,ie_data=%s \n",
id, result->ts, result->ssid, result->bssid, result->channel, result->rssi,
result->rtt, result->rtt_sd, result->beacon_period, result->capability, result->ie_length, result->ie_data);
}
void onScanEvent(wifi_request_id id, wifi_scan_event event) {
    ALOGI("onScanEvent id = %d \n event = %u \n", id, event);
}

void* event_loop_thread(void *arg) {
    bdt_log( "This is for event loop");
    hal_fn.wifi_event_loop(*(wifi_handle*)arg);
    return 0;
}


