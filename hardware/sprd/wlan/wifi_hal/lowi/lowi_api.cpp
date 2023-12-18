#include "lowi_api.h"

#define LOG_TAG "WifiLOWIHAL"
#include <log/log.h>

/*
 * NOTE:
 * extern "C" should be needed as wifi hal fetchLowiCbTableAndCapabilities
 * will call dlsym() to search specific function name.
 * C++ compiler will modify the function name by adding someting,
 * which will make that function search failed.
 */

extern "C" {
lowi_wifi_handle m_lowi_wifi_handle;
lowi_wifi_interface_handle *m_lowi_wifi_interface_handle;
lowi_wifi_interface_handle m_lowi_wlan0_interface_handle;
lowi_cb_table_t lowi_api_table;


static void* event_loop_thread(void *arg) {
	ALOGI("starting event loop");
	lowi_wifi_event_loop(*(lowi_wifi_handle*)arg);
	return 0;
}


int init()
{
	wifi_error ret;
	int pret, i;
	int num = 0;
	pthread_t th = 0;
	char *ifname;

	ALOGI("initializing...");
	ifname = (char *)malloc(IFNAMSIZ);
	if (!ifname) {
		ALOGE("init: malloc failed!");
		return -2;
	}

	ret = lowi_wifi_initialize(&m_lowi_wifi_handle);
	if (ret != WIFI_SUCCESS) {
		ALOGE("init: wifi_initialize failed!");
		free(ifname);
		return -3;
	}

	lowi_wifi_get_ifaces(m_lowi_wifi_handle, &num, &m_lowi_wifi_interface_handle);

	for (i = 0; i < num; i++) {
		memset(ifname, 0, IFNAMSIZ);
		lowi_wifi_get_iface_name(m_lowi_wifi_interface_handle[i], ifname, IFNAMSIZ);
		ALOGI("lowi_ifname = %s, ifname= %s, strcmp result = %d \n",
			LOWI_IFNAME, ifname, strcmp(LOWI_IFNAME, ifname));
		if (!strcmp(LOWI_IFNAME, ifname)) {
			m_lowi_wlan0_interface_handle = m_lowi_wifi_interface_handle[i];
			ALOGI("%s: got LOWI interface handle", __func__);
		}
	}
	ALOGI("%s: starting run event loop", __func__);

	pret = pthread_create(&th, NULL, event_loop_thread, &m_lowi_wifi_handle);
	if (pret != 0) {
		ALOGI( "Create thread error!\n");
		free(ifname);
		return -1;
	}

	ALOGI("%s: inited", __func__);
	free(ifname);
	return 0;
}


void lowi_wifi_cleaned_up(lowi_wifi_handle handle)
{
	ALOGI("%s cleaned up", __func__);
	return;
}


int destroy()
{
	ALOGI("lowi_api_table destroy");
	lowi_wifi_cleanup(m_lowi_wifi_handle, lowi_wifi_cleaned_up);
	return 0;
}


int get_rtt_capabilities(wifi_interface_handle iface, wifi_rtt_capabilities *capabilities)
{
	wifi_error ret = WIFI_SUCCESS;

	ret = lowi_wifi_get_rtt_capabilities(m_lowi_wlan0_interface_handle, capabilities);
	if (ret < 0) {
		ALOGE("%s failed: ret: %d", __func__, ret);
		return WIFI_ERROR_UNKNOWN;
	}
	ALOGI("%s Got cap: sied 0x%x, ftm: 0x%x, lci: 0x%x, lcr: 0x%x, preamble: 0x%x, bw: 0x%x "
		"responder: 0x%x, mc_version: 0x%x", __func__,
		capabilities->rtt_one_sided_supported,
		capabilities->rtt_ftm_supported,
		capabilities->lci_support,
		capabilities->lcr_support,
		capabilities->preamble_support,
		capabilities->bw_support,
		capabilities->responder_supported,
		capabilities->mc_version);

	return WIFI_SUCCESS;
}


int rtt_range_request(u32 request_id, wifi_interface_handle iface,
			u32 num_rtt_config,
			wifi_rtt_config rtt_config[],
			wifi_rtt_event_handler handler)
{
	wifi_error ret = WIFI_SUCCESS;	

	ret = lowi_wifi_rtt_range_request(request_id, m_lowi_wlan0_interface_handle,
				num_rtt_config, rtt_config, handler);
	if (ret < 0) {
		ALOGE("%s failed: ret: %d", __func__, ret);
		return WIFI_ERROR_UNKNOWN ;
	}
	ALOGI("%s success!", __func__);
	return WIFI_SUCCESS;
}


int rtt_range_cancel(u32 request_id, u32 num_devices, mac_addr addr[])
{
	wifi_error ret = WIFI_SUCCESS;	

	ret = lowi_wifi_rtt_range_cancel(request_id,  m_lowi_wlan0_interface_handle,
        			num_devices, addr);
	if (ret < 0) {
		ALOGE("%s failed: ret: %d", __func__, ret);
		return WIFI_ERROR_UNKNOWN ;
	}
	ALOGI("%s success!", __func__);
	return WIFI_SUCCESS;
}


wifi_error rtt_get_responder_info(wifi_interface_handle iface,
				wifi_rtt_responder *responder_info)
{
	wifi_error ret = WIFI_SUCCESS;	
	ret = lowi_wifi_rtt_get_responder_info(m_lowi_wlan0_interface_handle,
			responder_info);
	if (ret < 0) {
		ALOGE("%s failed: ret: %d", __func__, ret);
		return WIFI_ERROR_UNKNOWN ;
	}
	ALOGI("%s success!", __func__);
	return WIFI_SUCCESS;
}


wifi_error enable_responder(wifi_request_id id,
			wifi_interface_handle iface,
			wifi_channel_info channel_hint,
			unsigned max_duration_seconds,
			wifi_rtt_responder *responder_info)
{
	wifi_error ret = WIFI_SUCCESS;
	
	ret = lowi_wifi_enable_responder(id, m_lowi_wlan0_interface_handle,
                                channel_hint, max_duration_seconds,
                                responder_info);
	if (ret < 0) {
		ALOGE("%s failed: ret: %d", __func__, ret);
		return WIFI_ERROR_UNKNOWN ;
	}
	ALOGI("%s success!", __func__);
	return WIFI_SUCCESS;
}


wifi_error disable_responder(wifi_request_id id, wifi_interface_handle iface)
{
	wifi_error ret = WIFI_SUCCESS;

	ret = lowi_wifi_disable_responder(id, m_lowi_wlan0_interface_handle);

	if (ret < 0) {
		ALOGE("%s failed: ret: %d", __func__, ret);
		return WIFI_ERROR_UNKNOWN ;
	}
	ALOGI("%s success!", __func__);
	return WIFI_SUCCESS;
}


wifi_error rtt_set_lci(wifi_request_id id,
			wifi_interface_handle iface,
			wifi_lci_information *lci)
{
	wifi_error ret = WIFI_SUCCESS;

	ret = lowi_wifi_set_lci(id, m_lowi_wlan0_interface_handle, lci);

	if (ret < 0) {
		ALOGE("%s failed: ret: %d", __func__, ret);
		return WIFI_ERROR_UNKNOWN ;
	}
	ALOGI("%s success!", __func__);
	return WIFI_SUCCESS;
}


wifi_error rtt_set_lcr(wifi_request_id id,
			wifi_interface_handle iface,
			wifi_lcr_information *lcr)
{
	wifi_error ret = WIFI_SUCCESS;

	ret = lowi_wifi_set_lcr(id, m_lowi_wlan0_interface_handle, lcr);

	if (ret < 0) {
		ALOGE("%s failed: ret: %d", __func__, ret);
		return WIFI_ERROR_UNKNOWN ;
	}
	ALOGI("%s success!", __func__);
	return WIFI_SUCCESS;
}


int get_lowi_capabilities(u32 *capabilities)
{
	if (!capabilities) {
		ALOGE("%s: invalid caps", __func__);
		return WIFI_ERROR_INVALID_ARGS;
	}
	ALOGI("%s: only support RTT caps for now", __func__);
	*capabilities |= (ONE_SIDED_RANGING_SUPPORTED | DUAL_SIDED_RANGING_SUPPORED);
	return 0;
}


/*
 * dlsym(handle, "name"):
 * must have exact name match with extern "C"
 */
lowi_cb_table_t *lowi_wifihal_get_cb_table()
{
	lowi_cb_table_t *func_table = &lowi_api_table;

	ALOGI("lowi_api_table addr: 0x%p and initialized", func_table);
	memset(func_table, 0, sizeof(lowi_cb_table_t));

	func_table->init = init;
	func_table->destroy = destroy;
	func_table->get_rtt_capabilities = get_rtt_capabilities;
	func_table->rtt_range_request = rtt_range_request;
	func_table->rtt_range_cancel = rtt_range_cancel;
	/* func_table->get_lowi_versioin = get_lowi_version; */
	func_table->get_lowi_capabilities = get_lowi_capabilities;
	func_table->rtt_get_responder_info = rtt_get_responder_info;
	func_table->enable_responder = enable_responder;
	func_table->disable_responder = disable_responder;
	func_table->rtt_set_lci = rtt_set_lci;
	func_table->rtt_set_lcr = rtt_set_lcr;

	return func_table;
}
} /* extern "C" */
