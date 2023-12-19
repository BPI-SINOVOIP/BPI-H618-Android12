/*
 * hidl interface for wpa_supplicant daemon
 * Copyright (c) 2004-2016, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2004-2016, Roshan Pius <rpius@google.com>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "hidl_manager.h"
#include "hidl_return_util.h"
#include "iface_config_utils.h"
#include "misc_utils.h"
#include "sta_iface.h"

extern "C"
{
#include "utils/eloop.h"
#include "gas_query.h"
#include "interworking.h"
#include "hs20_supplicant.h"
#include "wps_supplicant.h"
#include "dpp.h"
#include "dpp_supplicant.h"
#include "rsn_supp/wpa.h"
#include "rsn_supp/pmksa_cache.h"
}

namespace {
using ISupplicantStaNetworkV1_2 =
	android::hardware::wifi::supplicant::V1_2::ISupplicantStaNetwork;
using ISupplicantStaNetworkV1_3 =
	android::hardware::wifi::supplicant::V1_3::ISupplicantStaNetwork;
using android::hardware::wifi::V1_0::WifiChannelWidthInMhz;
using android::hardware::wifi::supplicant::V1_0::SupplicantStatus;
using android::hardware::wifi::supplicant::V1_0::SupplicantStatusCode;
using android::hardware::wifi::supplicant::V1_0::ISupplicantStaNetwork;
using android::hardware::wifi::supplicant::V1_3::WifiTechnology;
using android::hardware::wifi::supplicant::V1_4::ISupplicantStaIface;
using android::hardware::wifi::supplicant::V1_4::ConnectionCapabilities;
using android::hardware::wifi::supplicant::V1_4::LegacyMode;
using android::hardware::wifi::supplicant::V1_4::implementation::HidlManager;
using android::hardware::wifi::supplicant::V1_4::DppResponderBootstrapInfo;
using android::hardware::wifi::supplicant::V1_4::DppCurve;

constexpr uint32_t kMaxAnqpElems = 100;
constexpr char kGetMacAddress[] = "MACADDR";
constexpr char kStartRxFilter[] = "RXFILTER-START";
constexpr char kStopRxFilter[] = "RXFILTER-STOP";
constexpr char kAddRxFilter[] = "RXFILTER-ADD";
constexpr char kRemoveRxFilter[] = "RXFILTER-REMOVE";
constexpr char kSetBtCoexistenceMode[] = "BTCOEXMODE";
constexpr char kSetBtCoexistenceScanStart[] = "BTCOEXSCAN-START";
constexpr char kSetBtCoexistenceScanStop[] = "BTCOEXSCAN-STOP";
constexpr char kSetSupendModeEnabled[] = "SETSUSPENDMODE 1";
constexpr char kSetSupendModeDisabled[] = "SETSUSPENDMODE 0";
constexpr char kSetCountryCode[] = "COUNTRY";
constexpr uint32_t kExtRadioWorkDefaultTimeoutInSec = static_cast<uint32_t>(
    ISupplicantStaIface::ExtRadioWorkDefaults::TIMEOUT_IN_SECS);
constexpr char kExtRadioWorkNamePrefix[] = "ext:";

uint8_t convertHidlRxFilterTypeToInternal(
    ISupplicantStaIface::RxFilterType type)
{
	switch (type) {
	case ISupplicantStaIface::RxFilterType::V4_MULTICAST:
		return 2;
	case ISupplicantStaIface::RxFilterType::V6_MULTICAST:
		return 3;
	};
	WPA_ASSERT(false);
}

uint8_t convertHidlBtCoexModeToInternal(
    ISupplicantStaIface::BtCoexistenceMode mode)
{
	switch (mode) {
	case ISupplicantStaIface::BtCoexistenceMode::ENABLED:
		return 0;
	case ISupplicantStaIface::BtCoexistenceMode::DISABLED:
		return 1;
	case ISupplicantStaIface::BtCoexistenceMode::SENSE:
		return 2;
	};
	WPA_ASSERT(false);
}

SupplicantStatus doZeroArgDriverCommand(
    struct wpa_supplicant *wpa_s, const char *cmd)
{
	std::vector<char> cmd_vec(cmd, cmd + strlen(cmd) + 1);
	char driver_cmd_reply_buf[4096] = {};
	if (wpa_drv_driver_cmd(
		wpa_s, cmd_vec.data(), driver_cmd_reply_buf,
		sizeof(driver_cmd_reply_buf))) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus doOneArgDriverCommand(
    struct wpa_supplicant *wpa_s, const char *cmd, uint8_t arg)
{
	std::string cmd_str = std::string(cmd) + " " + std::to_string(arg);
	return doZeroArgDriverCommand(wpa_s, cmd_str.c_str());
}

SupplicantStatus doOneArgDriverCommand(
    struct wpa_supplicant *wpa_s, const char *cmd, const std::string &arg)
{
	std::string cmd_str = std::string(cmd) + " " + arg;
	return doZeroArgDriverCommand(wpa_s, cmd_str.c_str());
}

void endExtRadioWork(struct wpa_radio_work *work)
{
	auto *ework = static_cast<struct wpa_external_work *>(work->ctx);
	work->wpa_s->ext_work_in_progress = 0;
	radio_work_done(work);
	os_free(ework);
}

void extRadioWorkTimeoutCb(void *eloop_ctx, void *timeout_ctx)
{
	auto *work = static_cast<struct wpa_radio_work *>(eloop_ctx);
	auto *ework = static_cast<struct wpa_external_work *>(work->ctx);
	wpa_dbg(
	    work->wpa_s, MSG_DEBUG, "Timing out external radio work %u (%s)",
	    ework->id, work->type);

	HidlManager *hidl_manager = HidlManager::getInstance();
	WPA_ASSERT(hidl_manager);
	hidl_manager->notifyExtRadioWorkTimeout(work->wpa_s, ework->id);

	endExtRadioWork(work);
}

void startExtRadioWork(struct wpa_radio_work *work)
{
	auto *ework = static_cast<struct wpa_external_work *>(work->ctx);
	work->wpa_s->ext_work_in_progress = 1;
	if (!ework->timeout) {
		ework->timeout = kExtRadioWorkDefaultTimeoutInSec;
	}
	eloop_register_timeout(
	    ework->timeout, 0, extRadioWorkTimeoutCb, work, nullptr);
}

void extRadioWorkStartCb(struct wpa_radio_work *work, int deinit)
{
	// deinit==1 is invoked during interface removal. Since the HIDL
	// interface does not support interface addition/removal, we don't
	// need to handle this scenario.
	WPA_ASSERT(!deinit);

	auto *ework = static_cast<struct wpa_external_work *>(work->ctx);
	wpa_dbg(
	    work->wpa_s, MSG_DEBUG, "Starting external radio work %u (%s)",
	    ework->id, ework->type);

	HidlManager *hidl_manager = HidlManager::getInstance();
	WPA_ASSERT(hidl_manager);
	hidl_manager->notifyExtRadioWorkStart(work->wpa_s, ework->id);

	startExtRadioWork(work);
}

uint32_t convertWpaKeyMgmtCapabilitiesToHidl (
    struct wpa_supplicant *wpa_s, struct wpa_driver_capa *capa) {

	uint32_t mask = 0;
	/* Logic from ctrl_iface.c, NONE and IEEE8021X have no capability
	 * flags and always enabled.
	 */
	mask |=
	    (ISupplicantStaNetwork::KeyMgmtMask::NONE |
	     ISupplicantStaNetwork::KeyMgmtMask::IEEE8021X);

	if (capa->key_mgmt &
	    (WPA_DRIVER_CAPA_KEY_MGMT_WPA | WPA_DRIVER_CAPA_KEY_MGMT_WPA2)) {
		mask |= ISupplicantStaNetwork::KeyMgmtMask::WPA_EAP;
	}

	if (capa->key_mgmt & (WPA_DRIVER_CAPA_KEY_MGMT_WPA_PSK |
			     WPA_DRIVER_CAPA_KEY_MGMT_WPA2_PSK)) {
		mask |= ISupplicantStaNetwork::KeyMgmtMask::WPA_PSK;
	}
#ifdef CONFIG_SUITEB192
	if (capa->key_mgmt & WPA_DRIVER_CAPA_KEY_MGMT_SUITE_B_192) {
		mask |= ISupplicantStaNetworkV1_2::ISupplicantStaNetwork::KeyMgmtMask::SUITE_B_192;
	}
#endif /* CONFIG_SUITEB192 */
#ifdef CONFIG_OWE
	if (capa->key_mgmt & WPA_DRIVER_CAPA_KEY_MGMT_OWE) {
		mask |= ISupplicantStaNetworkV1_2::ISupplicantStaNetwork::KeyMgmtMask::OWE;
	}
#endif /* CONFIG_OWE */
#ifdef CONFIG_SAE
	if (wpa_s->drv_flags & WPA_DRIVER_FLAGS_SAE) {
		mask |= ISupplicantStaNetworkV1_2::ISupplicantStaNetwork::KeyMgmtMask::SAE;
	}
#endif /* CONFIG_SAE */
#ifdef CONFIG_DPP
	if (capa->key_mgmt & WPA_DRIVER_CAPA_KEY_MGMT_DPP) {
		mask |= ISupplicantStaNetworkV1_2::ISupplicantStaNetwork::KeyMgmtMask::DPP;
	}
#endif
#ifdef CONFIG_WAPI_INTERFACE
	mask |= ISupplicantStaNetworkV1_3::ISupplicantStaNetwork::KeyMgmtMask::WAPI_PSK;
	mask |= ISupplicantStaNetworkV1_3::ISupplicantStaNetwork::KeyMgmtMask::WAPI_CERT;
#endif /* CONFIG_WAPI_INTERFACE */
#ifdef CONFIG_FILS
	if (capa->key_mgmt & WPA_DRIVER_CAPA_KEY_MGMT_FILS_SHA256) {
		mask |= ISupplicantStaNetworkV1_3::ISupplicantStaNetwork::KeyMgmtMask::FILS_SHA256;
	}
	if (capa->key_mgmt & WPA_DRIVER_CAPA_KEY_MGMT_FILS_SHA384) {
		mask |= ISupplicantStaNetworkV1_3::ISupplicantStaNetwork::KeyMgmtMask::FILS_SHA384;
	}
#endif /* CONFIG_FILS */
	return mask;
}

const std::string getDppListenChannel(struct wpa_supplicant *wpa_s, int32_t *listen_channel)
{
	struct hostapd_hw_modes *mode;
	int chan44 = 0, chan149 = 0;
	*listen_channel = 0;

	/* Check if device support 2.4GHz band*/
	mode = get_mode(wpa_s->hw.modes, wpa_s->hw.num_modes,
			HOSTAPD_MODE_IEEE80211G, 0);
	if (mode) {
		*listen_channel = 6;
		return "81/6";
	}
	/* Check if device support 5GHz band */
	mode = get_mode(wpa_s->hw.modes, wpa_s->hw.num_modes,
			HOSTAPD_MODE_IEEE80211A, 0);
	if (mode) {
		for (int i = 0; i < mode->num_channels; i++) {
			struct hostapd_channel_data *chan = &mode->channels[i];

			if (chan->flag & (HOSTAPD_CHAN_DISABLED |
					  HOSTAPD_CHAN_RADAR))
				continue;
			if (chan->freq == 5220)
				chan44 = 1;
			if (chan->freq == 5745)
				chan149 = 1;
		}
		if (chan149) {
			*listen_channel = 149;
			return "124/149";
		} else if (chan44) {
			*listen_channel = 44;
			return "115/44";
		}
	}

	return "";
}

const std::string convertCurveTypeToName(DppCurve curve)
{
	switch (curve) {
	case DppCurve::PRIME256V1:
		return "prime256v1";
	case DppCurve::SECP384R1:
		return "secp384r1";
	case DppCurve::SECP521R1:
		return "secp521r1";
	case DppCurve::BRAINPOOLP256R1:
		return "brainpoolP256r1";
	case DppCurve::BRAINPOOLP384R1:
		return "brainpoolP384r1";
	case DppCurve::BRAINPOOLP512R1:
		return "brainpoolP512r1";
	}
	WPA_ASSERT(false);
}

}  // namespace

namespace android {
namespace hardware {
namespace wifi {
namespace supplicant {
namespace V1_4 {
namespace implementation {
using hidl_return_util::validateAndCall;
using V1_0::ISupplicantStaIfaceCallback;
using V1_0::SupplicantStatusCode;

StaIface::StaIface(struct wpa_global *wpa_global, const char ifname[])
    : wpa_global_(wpa_global), ifname_(ifname), is_valid_(true)
{}

void StaIface::invalidate() { is_valid_ = false; }
bool StaIface::isValid()
{
	return (is_valid_ && (retrieveIfacePtr() != nullptr));
}

Return<void> StaIface::getName(getName_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::getNameInternal, _hidl_cb);
}

Return<void> StaIface::getType(getType_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::getTypeInternal, _hidl_cb);
}

Return<void> StaIface::addNetwork(addNetwork_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::addNetworkInternal, _hidl_cb);
}

Return<void> StaIface::removeNetwork(
    SupplicantNetworkId id, removeNetwork_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::removeNetworkInternal, _hidl_cb, id);
}

Return<void> StaIface::filsHlpFlushRequest(filsHlpFlushRequest_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::filsHlpFlushRequestInternal, _hidl_cb);
}

Return<void> StaIface::filsHlpAddRequest(
    const hidl_array<uint8_t, 6> &dst_mac, const hidl_vec<uint8_t> &pkt,
    filsHlpAddRequest_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::filsHlpAddRequestInternal, _hidl_cb, dst_mac, pkt);
}

Return<void> StaIface::getNetwork(
    SupplicantNetworkId id, getNetwork_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::getNetworkInternal, _hidl_cb, id);
}

Return<void> StaIface::listNetworks(listNetworks_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::listNetworksInternal, _hidl_cb);
}

Return<void> StaIface::registerCallback(
    const sp<ISupplicantStaIfaceCallback> &callback,
    registerCallback_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::registerCallbackInternal, _hidl_cb, callback);
}

Return<void> StaIface::registerCallback_1_1(
    const sp<V1_1::ISupplicantStaIfaceCallback> &callback,
    registerCallback_cb _hidl_cb)
{
	sp<V1_0::ISupplicantStaIfaceCallback> callback_1_0 = callback;
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::registerCallbackInternal, _hidl_cb, callback_1_0);
}

Return<void> StaIface::registerCallback_1_2(
    const sp<V1_2::ISupplicantStaIfaceCallback> &callback,
    registerCallback_cb _hidl_cb)
{
	sp<V1_1::ISupplicantStaIfaceCallback> callback_1_1 = callback;
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::registerCallbackInternal, _hidl_cb, callback_1_1);
}

Return<void> StaIface::registerCallback_1_3(
    const sp<V1_3::ISupplicantStaIfaceCallback> &callback,
    registerCallback_cb _hidl_cb)
{
	sp<V1_3::ISupplicantStaIfaceCallback> callback_1_3 = callback;
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::registerCallbackInternal, _hidl_cb, callback_1_3);
}

Return<void> StaIface::registerCallback_1_4(
    const sp<V1_4::ISupplicantStaIfaceCallback> &callback,
    registerCallback_1_4_cb _hidl_cb)
{
	sp<V1_4::ISupplicantStaIfaceCallback> callback_1_4 = callback;
	return validateAndCall(
	    this, V1_4::SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::registerCallbackInternal_1_4, _hidl_cb, callback_1_4);
}

Return<void> StaIface::reassociate(reassociate_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::reassociateInternal, _hidl_cb);
}

Return<void> StaIface::reconnect(reconnect_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::reconnectInternal, _hidl_cb);
}

Return<void> StaIface::disconnect(disconnect_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::disconnectInternal, _hidl_cb);
}

Return<void> StaIface::setPowerSave(bool enable, setPowerSave_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::setPowerSaveInternal, _hidl_cb, enable);
}

Return<void> StaIface::initiateTdlsDiscover(
    const hidl_array<uint8_t, 6> &mac_address, initiateTdlsDiscover_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::initiateTdlsDiscoverInternal, _hidl_cb, mac_address);
}

Return<void> StaIface::initiateTdlsSetup(
    const hidl_array<uint8_t, 6> &mac_address, initiateTdlsSetup_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::initiateTdlsSetupInternal, _hidl_cb, mac_address);
}

Return<void> StaIface::initiateTdlsTeardown(
    const hidl_array<uint8_t, 6> &mac_address, initiateTdlsTeardown_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::initiateTdlsTeardownInternal, _hidl_cb, mac_address);
}
Return<void> StaIface::initiateAnqpQuery(
    const hidl_array<uint8_t, 6> &mac_address,
    const hidl_vec<V1_0::ISupplicantStaIface::AnqpInfoId> &info_elements,
    const hidl_vec<ISupplicantStaIface::Hs20AnqpSubtypes> &sub_types,
    initiateAnqpQuery_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::initiateAnqpQueryInternal, _hidl_cb, mac_address,
	    info_elements, sub_types);
}

Return<void> StaIface::initiateVenueUrlAnqpQuery(
    const hidl_array<uint8_t, 6> &mac_address,
	initiateVenueUrlAnqpQuery_cb _hidl_cb)
{
	return validateAndCall(
	    this, V1_4::SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::initiateVenueUrlAnqpQueryInternal, _hidl_cb, mac_address);
}

Return<void> StaIface::initiateHs20IconQuery(
    const hidl_array<uint8_t, 6> &mac_address, const hidl_string &file_name,
    initiateHs20IconQuery_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::initiateHs20IconQueryInternal, _hidl_cb, mac_address,
	    file_name);
}

Return<void> StaIface::getMacAddress(getMacAddress_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::getMacAddressInternal, _hidl_cb);
}

Return<void> StaIface::startRxFilter(startRxFilter_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::startRxFilterInternal, _hidl_cb);
}

Return<void> StaIface::stopRxFilter(stopRxFilter_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::stopRxFilterInternal, _hidl_cb);
}

Return<void> StaIface::addRxFilter(
    ISupplicantStaIface::RxFilterType type, addRxFilter_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::addRxFilterInternal, _hidl_cb, type);
}

Return<void> StaIface::removeRxFilter(
    ISupplicantStaIface::RxFilterType type, removeRxFilter_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::removeRxFilterInternal, _hidl_cb, type);
}

Return<void> StaIface::setBtCoexistenceMode(
    ISupplicantStaIface::BtCoexistenceMode mode,
    setBtCoexistenceMode_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::setBtCoexistenceModeInternal, _hidl_cb, mode);
}

Return<void> StaIface::setBtCoexistenceScanModeEnabled(
    bool enable, setBtCoexistenceScanModeEnabled_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::setBtCoexistenceScanModeEnabledInternal, _hidl_cb,
	    enable);
}

Return<void> StaIface::setSuspendModeEnabled(
    bool enable, setSuspendModeEnabled_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::setSuspendModeEnabledInternal, _hidl_cb, enable);
}

Return<void> StaIface::setCountryCode(
    const hidl_array<int8_t, 2> &code, setCountryCode_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::setCountryCodeInternal, _hidl_cb, code);
}

Return<void> StaIface::startWpsRegistrar(
    const hidl_array<uint8_t, 6> &bssid, const hidl_string &pin,
    startWpsRegistrar_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::startWpsRegistrarInternal, _hidl_cb, bssid, pin);
}

Return<void> StaIface::startWpsPbc(
    const hidl_array<uint8_t, 6> &bssid, startWpsPbc_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::startWpsPbcInternal, _hidl_cb, bssid);
}

Return<void> StaIface::startWpsPinKeypad(
    const hidl_string &pin, startWpsPinKeypad_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::startWpsPinKeypadInternal, _hidl_cb, pin);
}

Return<void> StaIface::startWpsPinDisplay(
    const hidl_array<uint8_t, 6> &bssid, startWpsPinDisplay_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::startWpsPinDisplayInternal, _hidl_cb, bssid);
}

Return<void> StaIface::cancelWps(cancelWps_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::cancelWpsInternal, _hidl_cb);
}

Return<void> StaIface::setWpsDeviceName(
    const hidl_string &name, setWpsDeviceName_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::setWpsDeviceNameInternal, _hidl_cb, name);
}

Return<void> StaIface::setWpsDeviceType(
    const hidl_array<uint8_t, 8> &type, setWpsDeviceType_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::setWpsDeviceTypeInternal, _hidl_cb, type);
}

Return<void> StaIface::setWpsManufacturer(
    const hidl_string &manufacturer, setWpsManufacturer_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::setWpsManufacturerInternal, _hidl_cb, manufacturer);
}

Return<void> StaIface::setWpsModelName(
    const hidl_string &model_name, setWpsModelName_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::setWpsModelNameInternal, _hidl_cb, model_name);
}

Return<void> StaIface::setWpsModelNumber(
    const hidl_string &model_number, setWpsModelNumber_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::setWpsModelNumberInternal, _hidl_cb, model_number);
}

Return<void> StaIface::setWpsSerialNumber(
    const hidl_string &serial_number, setWpsSerialNumber_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::setWpsSerialNumberInternal, _hidl_cb, serial_number);
}

Return<void> StaIface::setWpsConfigMethods(
    uint16_t config_methods, setWpsConfigMethods_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::setWpsConfigMethodsInternal, _hidl_cb, config_methods);
}

Return<void> StaIface::setExternalSim(
    bool useExternalSim, setExternalSim_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::setExternalSimInternal, _hidl_cb, useExternalSim);
}

Return<void> StaIface::addExtRadioWork(
    const hidl_string &name, uint32_t freq_in_mhz, uint32_t timeout_in_sec,
    addExtRadioWork_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::addExtRadioWorkInternal, _hidl_cb, name, freq_in_mhz,
	    timeout_in_sec);
}

Return<void> StaIface::removeExtRadioWork(
    uint32_t id, removeExtRadioWork_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::removeExtRadioWorkInternal, _hidl_cb, id);
}

Return<void> StaIface::enableAutoReconnect(
    bool enable, enableAutoReconnect_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::enableAutoReconnectInternal, _hidl_cb, enable);
}

Return<void> StaIface::getKeyMgmtCapabilities(
    getKeyMgmtCapabilities_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaIface::getKeyMgmtCapabilitiesInternal, _hidl_cb);
}

Return<void> StaIface::addDppPeerUri(const hidl_string& uri,
		addDppPeerUri_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaIface::addDppPeerUriInternal, _hidl_cb, uri);
}

Return<void> StaIface::removeDppUri(uint32_t bootstrap_id,
		removeDppUri_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaIface::removeDppUriInternal, _hidl_cb, bootstrap_id);
}

Return<void> StaIface::startDppConfiguratorInitiator(uint32_t peer_bootstrap_id,
		uint32_t own_bootstrap_id, const hidl_string& ssid,
		const hidl_string& password, const hidl_string& psk,
		DppNetRole net_role, DppAkm security_akm,
		startDppConfiguratorInitiator_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaIface::startDppConfiguratorInitiatorInternal, _hidl_cb, peer_bootstrap_id,
		own_bootstrap_id, ssid, password, psk, net_role, security_akm);
}

Return<void> StaIface::startDppEnrolleeInitiator(uint32_t peer_bootstrap_id,
		uint32_t own_bootstrap_id, startDppConfiguratorInitiator_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaIface::startDppEnrolleeInitiatorInternal, _hidl_cb, peer_bootstrap_id,
		own_bootstrap_id);
}

Return<void> StaIface::stopDppInitiator(stopDppInitiator_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaIface::stopDppInitiatorInternal, _hidl_cb);
}

Return<void> StaIface::generateDppBootstrapInfoForResponder(
		const hidl_array<uint8_t, 6> &mac_address, const hidl_string& device_info,
		DppCurve curve, generateDppBootstrapInfoForResponder_cb _hidl_cb)
{
	return validateAndCall(
	    this, V1_4::SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::generateDppBootstrapInfoForResponderInternal, _hidl_cb, mac_address,
	    device_info, curve);
}

Return<void> StaIface::startDppEnrolleeResponder(
		uint32_t listen_channel, startDppEnrolleeResponder_cb _hidl_cb)
{
	return validateAndCall(
	    this, V1_4::SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::startDppEnrolleeResponderInternal, _hidl_cb, listen_channel);
}

Return<void> StaIface::stopDppResponder(uint32_t own_bootstrap_id, stopDppResponder_cb _hidl_cb)
{
	return validateAndCall(
	    this, V1_4::SupplicantStatusCode::FAILURE_IFACE_INVALID,
	    &StaIface::stopDppResponderInternal, _hidl_cb, own_bootstrap_id);
}

Return<void> StaIface::getWpaDriverCapabilities(
		getWpaDriverCapabilities_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_UNKNOWN,
	    &StaIface::getWpaDriverCapabilitiesInternal, _hidl_cb);
}

Return<void> StaIface::getWpaDriverCapabilities_1_4(
		getWpaDriverCapabilities_1_4_cb _hidl_cb)
{
	return validateAndCall(
	    this, V1_4::SupplicantStatusCode::FAILURE_UNKNOWN,
	    &StaIface::getWpaDriverCapabilitiesInternal_1_4, _hidl_cb);
}

Return<void> StaIface::setMboCellularDataStatus(bool available,
		setMboCellularDataStatus_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_UNKNOWN,
	    &StaIface::setMboCellularDataStatusInternal, _hidl_cb, available);
}

Return<void> StaIface::getKeyMgmtCapabilities_1_3(
    getKeyMgmtCapabilities_1_3_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_NETWORK_INVALID,
	    &StaIface::getKeyMgmtCapabilitiesInternal_1_3, _hidl_cb);
}

std::pair<SupplicantStatus, std::string> StaIface::getNameInternal()
{
	return {{SupplicantStatusCode::SUCCESS, ""}, ifname_};
}

std::pair<SupplicantStatus, IfaceType> StaIface::getTypeInternal()
{
	return {{SupplicantStatusCode::SUCCESS, ""}, IfaceType::STA};
}

SupplicantStatus StaIface::filsHlpFlushRequestInternal()
{
#ifdef CONFIG_FILS
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();

	wpas_flush_fils_hlp_req(wpa_s);
	return {SupplicantStatusCode::SUCCESS, ""};
#else /* CONFIG_FILS */
	return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
#endif /* CONFIG_FILS */
}

SupplicantStatus StaIface::filsHlpAddRequestInternal(
    const std::array<uint8_t, 6> &dst_mac, const std::vector<uint8_t> &pkt)
{
#ifdef CONFIG_FILS
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	struct fils_hlp_req *req;

	if (!pkt.size())
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};


	req = (struct fils_hlp_req *)os_zalloc(sizeof(*req));
	if (!req)
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};

	os_memcpy(req->dst, dst_mac.data(), ETH_ALEN);

	req->pkt = wpabuf_alloc_copy(pkt.data(), pkt.size());
	if (!req->pkt) {
		os_free(req);
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}

	dl_list_add_tail(&wpa_s->fils_hlp_req, &req->list);
	return {SupplicantStatusCode::SUCCESS, ""};
#else /* CONFIG_FILS */
	return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
#endif /* CONFIG_FILS */
}

std::pair<SupplicantStatus, sp<ISupplicantNetwork>>
StaIface::addNetworkInternal()
{
	android::sp<V1_3::ISupplicantStaNetwork> network;
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	struct wpa_ssid *ssid = wpa_supplicant_add_network(wpa_s);
	if (!ssid) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, network};
	}
	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager ||
	    hidl_manager->getStaNetworkHidlObjectByIfnameAndNetworkId(
		wpa_s->ifname, ssid->id, &network)) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, network};
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, network};
}

Return<void> StaIface::getConnectionCapabilities(
    getConnectionCapabilities_cb _hidl_cb)
{
	return validateAndCall(
	    this, SupplicantStatusCode::FAILURE_UNKNOWN,
	    &StaIface::getConnectionCapabilitiesInternal, _hidl_cb);
}

Return<void> StaIface::getConnectionCapabilities_1_4(
    getConnectionCapabilities_1_4_cb _hidl_cb)
{
	return validateAndCall(
	    this, V1_4::SupplicantStatusCode::FAILURE_UNKNOWN,
	    &StaIface::getConnectionCapabilitiesInternal_1_4, _hidl_cb);
}

SupplicantStatus StaIface::removeNetworkInternal(SupplicantNetworkId id)
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	int result = wpa_supplicant_remove_network(wpa_s, id);
	if (result == -1) {
		return {SupplicantStatusCode::FAILURE_NETWORK_UNKNOWN, ""};
	}
	if (result != 0) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

std::pair<SupplicantStatus, sp<ISupplicantNetwork>>
StaIface::getNetworkInternal(SupplicantNetworkId id)
{
	android::sp<V1_3::ISupplicantStaNetwork> network;
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	struct wpa_ssid *ssid = wpa_config_get_network(wpa_s->conf, id);
	if (!ssid) {
		return {{SupplicantStatusCode::FAILURE_NETWORK_UNKNOWN, ""},
			network};
	}
	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager ||
	    hidl_manager->getStaNetworkHidlObjectByIfnameAndNetworkId(
		wpa_s->ifname, ssid->id, &network)) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, network};
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, network};
}

std::pair<SupplicantStatus, std::vector<SupplicantNetworkId>>
StaIface::listNetworksInternal()
{
	std::vector<SupplicantNetworkId> network_ids;
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	for (struct wpa_ssid *wpa_ssid = wpa_s->conf->ssid; wpa_ssid;
	     wpa_ssid = wpa_ssid->next) {
		network_ids.emplace_back(wpa_ssid->id);
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, std::move(network_ids)};
}

SupplicantStatus StaIface::registerCallbackInternal(
    const sp<ISupplicantStaIfaceCallback> &callback)
{
	return {SupplicantStatusCode::FAILURE_UNKNOWN, "deprecated"};
}

V1_4::SupplicantStatus StaIface::registerCallbackInternal_1_4(
    const sp<V1_4::ISupplicantStaIfaceCallback> &callback)
{
	HidlManager *hidl_manager = HidlManager::getInstance();
	if (!hidl_manager ||
	    hidl_manager->addStaIfaceCallbackHidlObject(ifname_, callback)) {
		return {V1_4::SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {V1_4::SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaIface::reassociateInternal()
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	if (wpa_s->wpa_state == WPA_INTERFACE_DISABLED) {
		return {SupplicantStatusCode::FAILURE_IFACE_DISABLED, ""};
	}
	wpas_request_connection(wpa_s);
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaIface::reconnectInternal()
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	if (wpa_s->wpa_state == WPA_INTERFACE_DISABLED) {
		return {SupplicantStatusCode::FAILURE_IFACE_DISABLED, ""};
	}
	if (!wpa_s->disconnected) {
		return {SupplicantStatusCode::FAILURE_IFACE_NOT_DISCONNECTED,
			""};
	}
	wpas_request_connection(wpa_s);
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaIface::disconnectInternal()
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	if (wpa_s->wpa_state == WPA_INTERFACE_DISABLED) {
		return {SupplicantStatusCode::FAILURE_IFACE_DISABLED, ""};
	}
	wpas_request_disconnection(wpa_s);
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaIface::setPowerSaveInternal(bool enable)
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	if (wpa_s->wpa_state == WPA_INTERFACE_DISABLED) {
		return {SupplicantStatusCode::FAILURE_IFACE_DISABLED, ""};
	}
	if (wpa_drv_set_p2p_powersave(wpa_s, enable, -1, -1)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaIface::initiateTdlsDiscoverInternal(
    const std::array<uint8_t, 6> &mac_address)
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	int ret;
	const u8 *peer = mac_address.data();
	if (wpa_tdls_is_external_setup(wpa_s->wpa)) {
		ret = wpa_tdls_send_discovery_request(wpa_s->wpa, peer);
	} else {
		ret = wpa_drv_tdls_oper(wpa_s, TDLS_DISCOVERY_REQ, peer);
	}
	if (ret) {
		wpa_printf(MSG_INFO, "StaIface: TDLS discover failed: %d", ret);
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaIface::initiateTdlsSetupInternal(
    const std::array<uint8_t, 6> &mac_address)
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	int ret;
	const u8 *peer = mac_address.data();
	if (wpa_tdls_is_external_setup(wpa_s->wpa) &&
	    !(wpa_s->conf->tdls_external_control)) {
		wpa_tdls_remove(wpa_s->wpa, peer);
		ret = wpa_tdls_start(wpa_s->wpa, peer);
	} else {
		ret = wpa_drv_tdls_oper(wpa_s, TDLS_SETUP, peer);
	}
	if (ret) {
		wpa_printf(MSG_INFO, "StaIface: TDLS setup failed: %d", ret);
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaIface::initiateTdlsTeardownInternal(
    const std::array<uint8_t, 6> &mac_address)
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	int ret;
	const u8 *peer = mac_address.data();
	if (wpa_tdls_is_external_setup(wpa_s->wpa) &&
	    !(wpa_s->conf->tdls_external_control)) {
		ret = wpa_tdls_teardown_link(
		    wpa_s->wpa, peer, WLAN_REASON_TDLS_TEARDOWN_UNSPECIFIED);
	} else {
		ret = wpa_drv_tdls_oper(wpa_s, TDLS_TEARDOWN, peer);
	}
	if (ret) {
		wpa_printf(MSG_INFO, "StaIface: TDLS teardown failed: %d", ret);
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaIface::initiateAnqpQueryInternal(
    const std::array<uint8_t, 6> &mac_address,
    const std::vector<ISupplicantStaIface::AnqpInfoId> &info_elements,
    const std::vector<ISupplicantStaIface::Hs20AnqpSubtypes> &sub_types)
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	if (info_elements.size() > kMaxAnqpElems) {
		return {SupplicantStatusCode::FAILURE_ARGS_INVALID, ""};
	}
	uint16_t info_elems_buf[kMaxAnqpElems];
	uint32_t num_info_elems = 0;
	for (const auto &info_element : info_elements) {
		info_elems_buf[num_info_elems++] =
		    static_cast<std::underlying_type<
			ISupplicantStaIface::AnqpInfoId>::type>(info_element);
	}
	uint32_t sub_types_bitmask = 0;
	for (const auto &type : sub_types) {
		sub_types_bitmask |= BIT(
		    static_cast<std::underlying_type<
			ISupplicantStaIface::Hs20AnqpSubtypes>::type>(type));
	}

	if (anqp_send_req(
		wpa_s, mac_address.data(), 0, info_elems_buf, num_info_elems,
		sub_types_bitmask, 0)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

V1_4::SupplicantStatus StaIface::initiateVenueUrlAnqpQueryInternal(
    const std::array<uint8_t, 6> &mac_address)
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	uint16_t info_elems_buf[1] = {ANQP_VENUE_URL};

	if (anqp_send_req(
		wpa_s, mac_address.data(), 0, info_elems_buf, 1, 0, 0)) {
		return {V1_4::SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {V1_4::SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaIface::initiateHs20IconQueryInternal(
    const std::array<uint8_t, 6> &mac_address, const std::string &file_name)
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	wpa_s->fetch_osu_icon_in_progress = 0;
	if (hs20_anqp_send_req(
		wpa_s, mac_address.data(), BIT(HS20_STYPE_ICON_REQUEST),
		reinterpret_cast<const uint8_t *>(file_name.c_str()),
		file_name.size(), true)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

std::pair<SupplicantStatus, std::array<uint8_t, 6>>
StaIface::getMacAddressInternal()
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	std::vector<char> cmd(
	    kGetMacAddress, kGetMacAddress + sizeof(kGetMacAddress));
	char driver_cmd_reply_buf[4096] = {};
	int ret = wpa_drv_driver_cmd(
	    wpa_s, cmd.data(), driver_cmd_reply_buf,
	    sizeof(driver_cmd_reply_buf));
	// Reply is of the format: "Macaddr = XX:XX:XX:XX:XX:XX"
	std::string reply_str = driver_cmd_reply_buf;
	if (ret < 0 || reply_str.empty() ||
	    reply_str.find("=") == std::string::npos) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	// Remove all whitespace first and then split using the delimiter "=".
	reply_str.erase(
	    remove_if(reply_str.begin(), reply_str.end(), isspace),
	    reply_str.end());
	std::string mac_addr_str =
	    reply_str.substr(reply_str.find("=") + 1, reply_str.size());
	std::array<uint8_t, 6> mac_addr;
	if (hwaddr_aton(mac_addr_str.c_str(), mac_addr.data())) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, {}};
	}
	return {{SupplicantStatusCode::SUCCESS, ""}, mac_addr};
}

SupplicantStatus StaIface::startRxFilterInternal()
{
	return doZeroArgDriverCommand(retrieveIfacePtr(), kStartRxFilter);
}

SupplicantStatus StaIface::stopRxFilterInternal()
{
	return doZeroArgDriverCommand(retrieveIfacePtr(), kStopRxFilter);
}

SupplicantStatus StaIface::addRxFilterInternal(
    ISupplicantStaIface::RxFilterType type)
{
	return doOneArgDriverCommand(
	    retrieveIfacePtr(), kAddRxFilter,
	    convertHidlRxFilterTypeToInternal(type));
}

SupplicantStatus StaIface::removeRxFilterInternal(
    ISupplicantStaIface::RxFilterType type)
{
	return doOneArgDriverCommand(
	    retrieveIfacePtr(), kRemoveRxFilter,
	    convertHidlRxFilterTypeToInternal(type));
}

SupplicantStatus StaIface::setBtCoexistenceModeInternal(
    ISupplicantStaIface::BtCoexistenceMode mode)
{
	return doOneArgDriverCommand(
	    retrieveIfacePtr(), kSetBtCoexistenceMode,
	    convertHidlBtCoexModeToInternal(mode));
}

SupplicantStatus StaIface::setBtCoexistenceScanModeEnabledInternal(bool enable)
{
	const char *cmd;
	if (enable) {
		cmd = kSetBtCoexistenceScanStart;
	} else {
		cmd = kSetBtCoexistenceScanStop;
	}
	return doZeroArgDriverCommand(retrieveIfacePtr(), cmd);
}

SupplicantStatus StaIface::setSuspendModeEnabledInternal(bool enable)
{
	const char *cmd;
	if (enable) {
		cmd = kSetSupendModeEnabled;
	} else {
		cmd = kSetSupendModeDisabled;
	}
	return doZeroArgDriverCommand(retrieveIfacePtr(), cmd);
}

SupplicantStatus StaIface::setCountryCodeInternal(
    const std::array<int8_t, 2> &code)
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	SupplicantStatus status = doOneArgDriverCommand(
	    wpa_s, kSetCountryCode,
	    std::string(std::begin(code), std::end(code)));
	if (status.code != SupplicantStatusCode::SUCCESS) {
		return status;
	}
	struct p2p_data *p2p = wpa_s->global->p2p;
	if (p2p) {
		char country[3];
		country[0] = code[0];
		country[1] = code[1];
		country[2] = 0x04;
		p2p_set_country(p2p, country);
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaIface::startWpsRegistrarInternal(
    const std::array<uint8_t, 6> &bssid, const std::string &pin)
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	if (wpas_wps_start_reg(wpa_s, bssid.data(), pin.c_str(), nullptr)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaIface::startWpsPbcInternal(
    const std::array<uint8_t, 6> &bssid)
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	const uint8_t *bssid_addr =
	    is_zero_ether_addr(bssid.data()) ? nullptr : bssid.data();
	if (wpas_wps_start_pbc(wpa_s, bssid_addr, 0, 0)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaIface::startWpsPinKeypadInternal(const std::string &pin)
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	if (wpas_wps_start_pin(
		wpa_s, nullptr, pin.c_str(), 0, DEV_PW_DEFAULT)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

std::pair<SupplicantStatus, std::string> StaIface::startWpsPinDisplayInternal(
    const std::array<uint8_t, 6> &bssid)
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	const uint8_t *bssid_addr =
	    is_zero_ether_addr(bssid.data()) ? nullptr : bssid.data();
	int pin =
	    wpas_wps_start_pin(wpa_s, bssid_addr, nullptr, 0, DEV_PW_DEFAULT);
	if (pin < 0) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, ""};
	}
	return {{SupplicantStatusCode::SUCCESS, ""},
		misc_utils::convertWpsPinToString(pin)};
}

SupplicantStatus StaIface::cancelWpsInternal()
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	if (wpas_wps_cancel(wpa_s)) {
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}
	return {SupplicantStatusCode::SUCCESS, ""};
}

SupplicantStatus StaIface::setWpsDeviceNameInternal(const std::string &name)
{
	return iface_config_utils::setWpsDeviceName(retrieveIfacePtr(), name);
}

SupplicantStatus StaIface::setWpsDeviceTypeInternal(
    const std::array<uint8_t, 8> &type)
{
	return iface_config_utils::setWpsDeviceType(retrieveIfacePtr(), type);
}

SupplicantStatus StaIface::setWpsManufacturerInternal(
    const std::string &manufacturer)
{
	return iface_config_utils::setWpsManufacturer(
	    retrieveIfacePtr(), manufacturer);
}

SupplicantStatus StaIface::setWpsModelNameInternal(
    const std::string &model_name)
{
	return iface_config_utils::setWpsModelName(
	    retrieveIfacePtr(), model_name);
}

SupplicantStatus StaIface::setWpsModelNumberInternal(
    const std::string &model_number)
{
	return iface_config_utils::setWpsModelNumber(
	    retrieveIfacePtr(), model_number);
}

SupplicantStatus StaIface::setWpsSerialNumberInternal(
    const std::string &serial_number)
{
	return iface_config_utils::setWpsSerialNumber(
	    retrieveIfacePtr(), serial_number);
}

SupplicantStatus StaIface::setWpsConfigMethodsInternal(uint16_t config_methods)
{
	return iface_config_utils::setWpsConfigMethods(
	    retrieveIfacePtr(), config_methods);
}

SupplicantStatus StaIface::setExternalSimInternal(bool useExternalSim)
{
	return iface_config_utils::setExternalSim(
	    retrieveIfacePtr(), useExternalSim);
}

std::pair<SupplicantStatus, uint32_t> StaIface::addExtRadioWorkInternal(
    const std::string &name, uint32_t freq_in_mhz, uint32_t timeout_in_sec)
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	auto *ework = static_cast<struct wpa_external_work *>(
	    os_zalloc(sizeof(struct wpa_external_work)));
	if (!ework) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""},
			UINT32_MAX};
	}

	std::string radio_work_name = kExtRadioWorkNamePrefix + name;
	os_strlcpy(ework->type, radio_work_name.c_str(), sizeof(ework->type));
	ework->timeout = timeout_in_sec;
	wpa_s->ext_work_id++;
	if (wpa_s->ext_work_id == 0) {
		wpa_s->ext_work_id++;
	}
	ework->id = wpa_s->ext_work_id;

	if (radio_add_work(
		wpa_s, freq_in_mhz, ework->type, 0, extRadioWorkStartCb,
		ework)) {
		os_free(ework);
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""},
			UINT32_MAX};
	}
	return {SupplicantStatus{SupplicantStatusCode::SUCCESS, ""}, ework->id};
}

SupplicantStatus StaIface::removeExtRadioWorkInternal(uint32_t id)
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	struct wpa_radio_work *work;
	dl_list_for_each(work, &wpa_s->radio->work, struct wpa_radio_work, list)
	{
		if (os_strncmp(
			work->type, kExtRadioWorkNamePrefix,
			sizeof(kExtRadioWorkNamePrefix)) != 0)
			continue;

		auto *ework =
		    static_cast<struct wpa_external_work *>(work->ctx);
		if (ework->id != id)
			continue;

		wpa_dbg(
		    wpa_s, MSG_DEBUG, "Completed external radio work %u (%s)",
		    ework->id, ework->type);
		eloop_cancel_timeout(extRadioWorkTimeoutCb, work, NULL);
		endExtRadioWork(work);

		return {SupplicantStatusCode::SUCCESS, ""};
	}
	return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
}

SupplicantStatus StaIface::enableAutoReconnectInternal(bool enable)
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	wpa_s->auto_reconnect_disabled = enable ? 0 : 1;
	return {SupplicantStatusCode::SUCCESS, ""};
}

std::pair<SupplicantStatus, uint32_t>
StaIface::getKeyMgmtCapabilitiesInternal()
{
	return {{SupplicantStatusCode::FAILURE_UNKNOWN, "deprecated"}, 0};
}

std::pair<SupplicantStatus, uint32_t>
StaIface::addDppPeerUriInternal(const std::string& uri)
{
#ifdef CONFIG_DPP
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	int32_t id;

	id = wpas_dpp_qr_code(wpa_s, uri.c_str());

	if (id > 0) {
		return {{SupplicantStatusCode::SUCCESS, ""}, id};
	}
#endif
	return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, -1};
}

SupplicantStatus StaIface::removeDppUriInternal(uint32_t bootstrap_id)
{
#ifdef CONFIG_DPP
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	std::string bootstrap_id_str;

	if (bootstrap_id == 0) {
		bootstrap_id_str = "*";
	}
	else {
		bootstrap_id_str = std::to_string(bootstrap_id);
	}

	if (dpp_bootstrap_remove(wpa_s->dpp, bootstrap_id_str.c_str()) >= 0) {
		return {SupplicantStatusCode::SUCCESS, ""};
	}
#endif
	return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
}

SupplicantStatus StaIface::startDppConfiguratorInitiatorInternal(
		uint32_t peer_bootstrap_id,	uint32_t own_bootstrap_id,
		const std::string& ssid, const std::string& password,
		const std::string& psk, DppNetRole net_role, DppAkm security_akm)
{
#ifdef CONFIG_DPP
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	std::string cmd = "";

	if (net_role != DppNetRole::AP &&
			net_role != DppNetRole::STA) {
		wpa_printf(MSG_ERROR,
			   "DPP: Error: Invalid network role specified: %d", net_role);
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}

	cmd += " peer=" + std::to_string(peer_bootstrap_id);
	cmd += (own_bootstrap_id > 0) ?
			" own=" + std::to_string(own_bootstrap_id) : "";

	/* Check for supported AKMs */
	if (security_akm != DppAkm::PSK && security_akm != DppAkm::SAE &&
			security_akm != DppAkm::PSK_SAE) {
		wpa_printf(MSG_ERROR, "DPP: Error: invalid AKM specified: %d",
				security_akm);
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}

	/* SAE AKM requires SSID and password to be initialized */
	if ((security_akm == DppAkm::SAE ||
			security_akm == DppAkm::PSK_SAE) &&
			(ssid.empty() || password.empty())) {
		wpa_printf(MSG_ERROR, "DPP: Error: Password or SSID not specified");
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	} else if (security_akm == DppAkm::PSK ||
			security_akm == DppAkm::PSK_SAE) {
		/* PSK AKM requires SSID and password/psk to be initialized */
		if (ssid.empty()) {
			wpa_printf(MSG_ERROR, "DPP: Error: SSID not specified");
			return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
		}
		if (password.empty() && psk.empty()) {
			wpa_printf(MSG_ERROR, "DPP: Error: Password or PSK not specified");
			return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
		}
	}

	cmd += " role=configurator";
	cmd += (ssid.empty()) ? "" : " ssid=" + ssid;

	if (!psk.empty()) {
		cmd += " psk=" + psk;
	} else {
		cmd += (password.empty()) ? "" : " pass=" + password;
	}

	std::string role = "";
	if (net_role == DppNetRole::AP) {
		role = "ap-";
	}
	else {
		role = "sta-";
	}

	switch (security_akm) {
	case DppAkm::PSK:
		role += "psk";
		break;

	case DppAkm::SAE:
		role += "sae";
		break;

	case DppAkm::PSK_SAE:
		role += "psk-sae";
		break;

	default:
		wpa_printf(MSG_ERROR,
			   "DPP: Invalid or unsupported security AKM specified: %d", security_akm);
		return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
	}

	cmd += " conf=";
	cmd += role;

	if (net_role == DppNetRole::STA) {
		/* DPP R2 connection status request */
		cmd += " conn_status=1";
	}

	wpa_printf(MSG_DEBUG,
		   "DPP initiator command: %s", cmd.c_str());

	if (wpas_dpp_auth_init(wpa_s, cmd.c_str()) == 0) {
		return {SupplicantStatusCode::SUCCESS, ""};
	}
#endif
	return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
}

SupplicantStatus StaIface::startDppEnrolleeInitiatorInternal(uint32_t peer_bootstrap_id,
			uint32_t own_bootstrap_id) {
#ifdef CONFIG_DPP
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	std::string cmd = "";

	/* Report received configuration to HIDL and create an internal profile */
	wpa_s->conf->dpp_config_processing = 1;

	cmd += " peer=" + std::to_string(peer_bootstrap_id);
	cmd += (own_bootstrap_id > 0) ?
			" own=" + std::to_string(own_bootstrap_id) : "";

	cmd += " role=enrollee";

	wpa_printf(MSG_DEBUG,
		   "DPP initiator command: %s", cmd.c_str());

	if (wpas_dpp_auth_init(wpa_s, cmd.c_str()) == 0) {
		return {SupplicantStatusCode::SUCCESS, ""};
	}
#endif
	return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
}
SupplicantStatus StaIface::stopDppInitiatorInternal()
{
#ifdef CONFIG_DPP
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();

	wpas_dpp_stop(wpa_s);
	return {SupplicantStatusCode::SUCCESS, ""};
#else
	return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
#endif
}

std::pair<V1_4::SupplicantStatus, DppResponderBootstrapInfo>
StaIface::generateDppBootstrapInfoForResponderInternal(const std::array<uint8_t, 6> &mac_address,
		const std::string& device_info, DppCurve curve)
{
#ifdef CONFIG_DPP
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	std::string cmd = "type=qrcode";
	int32_t id;
	int32_t listen_channel = 0;
	struct DppResponderBootstrapInfo bootstrap_info;
	const char *uri;
	std::string listen_channel_str;
	std::string mac_addr_str;
	char buf[3] = {0};

	cmd += (device_info.empty()) ? "" : " info=" + device_info;

	listen_channel_str = getDppListenChannel(wpa_s, &listen_channel);
	if (listen_channel == 0) {
		wpa_printf(MSG_ERROR, "StaIface: Failed to derive DPP listen channel");
		return {{V1_4::SupplicantStatusCode::FAILURE_UNKNOWN, ""}, bootstrap_info};
	}
	cmd += " chan=" + listen_channel_str;

	cmd += " mac=";
	for (int i = 0;i < 6;i++) {
		snprintf(buf, sizeof(buf), "%02x", mac_address[i]);
		mac_addr_str.append(buf);
	}
	cmd += mac_addr_str;

	cmd += " curve=" + convertCurveTypeToName(curve);

	id = dpp_bootstrap_gen(wpa_s->dpp, cmd.c_str());
	wpa_printf(MSG_DEBUG,
		   "DPP generate bootstrap QR code command: %s id: %d", cmd.c_str(), id);
	if (id > 0) {
		uri = dpp_bootstrap_get_uri(wpa_s->dpp, id);
		if (uri) {
			wpa_printf(MSG_DEBUG, "DPP Bootstrap info: id: %d "
				   "listen_channel: %d uri: %s", id, listen_channel, uri);
			bootstrap_info.bootstrapId = id;
			bootstrap_info.listenChannel = listen_channel;
			bootstrap_info.uri = uri;
			return {{V1_4::SupplicantStatusCode::SUCCESS, ""}, bootstrap_info};
		}
	}
	return {{V1_4::SupplicantStatusCode::FAILURE_UNKNOWN, ""}, bootstrap_info};
#else
	return {{V1_4::SupplicantStatusCode::FAILURE_UNSUPPORTED, ""}, bootstrap_info};
#endif
}

V1_4::SupplicantStatus StaIface::startDppEnrolleeResponderInternal(uint32_t listen_channel)
{
#ifdef CONFIG_DPP
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	std::string cmd = "";
	uint32_t freq = (listen_channel <= 14 ? 2407 : 5000) + listen_channel * 5;

	/* Report received configuration to HIDL and create an internal profile */
	wpa_s->conf->dpp_config_processing = 1;

	cmd += std::to_string(freq);
	cmd += " role=enrollee netrole=sta";

	wpa_printf(MSG_DEBUG,
		   "DPP Enrollee Responder command: %s", cmd.c_str());

	if (wpas_dpp_listen(wpa_s, cmd.c_str()) == 0) {
		return {V1_4::SupplicantStatusCode::SUCCESS, ""};
	}
	return {V1_4::SupplicantStatusCode::FAILURE_UNKNOWN, ""};
#else
	return {V1_4::SupplicantStatusCode::FAILURE_UNSUPPORTED, ""};
#endif
}

V1_4::SupplicantStatus StaIface::stopDppResponderInternal(uint32_t own_bootstrap_id)
{
#ifdef CONFIG_DPP
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	std::string bootstrap_id_str;

	if (own_bootstrap_id == 0) {
		bootstrap_id_str = "*";
	}
	else {
		bootstrap_id_str = std::to_string(own_bootstrap_id);
	}

	wpa_printf(MSG_DEBUG, "DPP Stop DPP Responder id: %d ", own_bootstrap_id);
	wpas_dpp_stop(wpa_s);
	wpas_dpp_listen_stop(wpa_s);

	if (dpp_bootstrap_remove(wpa_s->dpp, bootstrap_id_str.c_str()) < 0) {
		wpa_printf(MSG_ERROR, "StaIface: dpp_bootstrap_remove failed");
	}

	return {V1_4::SupplicantStatusCode::SUCCESS, ""};
#else
	return {V1_4::SupplicantStatusCode::FAILURE_UNSUPPORTED, ""};
#endif
}

std::pair<SupplicantStatus, android::hardware::wifi::supplicant::V1_3::ConnectionCapabilities>
StaIface::getConnectionCapabilitiesInternal()
{
  struct android::hardware::wifi::supplicant::V1_3::ConnectionCapabilities capa;
	return {{SupplicantStatusCode::FAILURE_UNKNOWN, "deprecated"}, capa};
}

std::pair<V1_4::SupplicantStatus, ConnectionCapabilities>
StaIface::getConnectionCapabilitiesInternal_1_4()
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	struct ConnectionCapabilities capa;

	if (wpa_s->connection_set) {
		capa.legacyMode = LegacyMode::UNKNOWN;
		if (wpa_s->connection_he) {
			capa.V1_3.technology = WifiTechnology::HE;
		} else if (wpa_s->connection_vht) {
			capa.V1_3.technology = WifiTechnology::VHT;
		} else if (wpa_s->connection_ht) {
			capa.V1_3.technology = WifiTechnology::HT;
		} else {
			capa.V1_3.technology = WifiTechnology::LEGACY;
			if (wpas_freq_to_band(wpa_s->assoc_freq) == BAND_2_4_GHZ) {
				capa.legacyMode = (wpa_s->connection_11b_only) ? LegacyMode::B_MODE
						: LegacyMode::G_MODE; 
			} else {
				capa.legacyMode = LegacyMode::A_MODE;
			}
		}
		switch (wpa_s->connection_channel_bandwidth) {
		case CHAN_WIDTH_20:
			capa.V1_3.channelBandwidth = WifiChannelWidthInMhz::WIDTH_20;
			break;
		case CHAN_WIDTH_40:
			capa.V1_3.channelBandwidth = WifiChannelWidthInMhz::WIDTH_40;
			break;
		case CHAN_WIDTH_80:
			capa.V1_3.channelBandwidth = WifiChannelWidthInMhz::WIDTH_80;
			break;
		case CHAN_WIDTH_160:
			capa.V1_3.channelBandwidth = WifiChannelWidthInMhz::WIDTH_160;
			break;
		case CHAN_WIDTH_80P80:
			capa.V1_3.channelBandwidth = WifiChannelWidthInMhz::WIDTH_80P80;
			break;
		default:
			capa.V1_3.channelBandwidth = WifiChannelWidthInMhz::WIDTH_20;
			break;
		}
		capa.V1_3.maxNumberRxSpatialStreams = wpa_s->connection_max_nss_rx;
		capa.V1_3.maxNumberTxSpatialStreams = wpa_s->connection_max_nss_tx;
	} else {
		capa.V1_3.technology = WifiTechnology::UNKNOWN;
		capa.V1_3.channelBandwidth = WifiChannelWidthInMhz::WIDTH_20;
		capa.V1_3.maxNumberTxSpatialStreams = 1;
		capa.V1_3.maxNumberRxSpatialStreams = 1;
		capa.legacyMode = LegacyMode::UNKNOWN;
	}
	return {{V1_4::SupplicantStatusCode::SUCCESS, ""}, capa};
}

std::pair<SupplicantStatus, uint32_t>
StaIface::getWpaDriverCapabilitiesInternal()
{
	return {{SupplicantStatusCode::FAILURE_UNKNOWN, "deprecated"}, 0};
}

std::pair<V1_4::SupplicantStatus, uint32_t>
StaIface::getWpaDriverCapabilitiesInternal_1_4()
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	uint32_t mask = 0;

#ifdef CONFIG_MBO
	/* MBO has no capability flags. It's mainly legacy 802.11v BSS
	 * transition + Cellular steering. 11v is a default feature in
	 * supplicant. And cellular steering is handled in framework.
	 */
	mask |= V1_3::WpaDriverCapabilitiesMask::MBO;
	if (wpa_s->enable_oce & OCE_STA) {
		mask |= V1_3::WpaDriverCapabilitiesMask::OCE;
	}
#endif
#ifdef CONFIG_SAE_PK
	mask |= V1_4::WpaDriverCapabilitiesMask::SAE_PK;
#endif
	mask |= V1_4::WpaDriverCapabilitiesMask::WFD_R2;

	wpa_printf(MSG_DEBUG, "Driver capability mask: 0x%x", mask);

	return {{V1_4::SupplicantStatusCode::SUCCESS, ""}, mask};
}

SupplicantStatus StaIface::setMboCellularDataStatusInternal(bool available)
{
#ifdef CONFIG_MBO
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	enum mbo_cellular_capa mbo_cell_capa;

	if (available) {
		mbo_cell_capa = MBO_CELL_CAPA_AVAILABLE;
	} else {
		mbo_cell_capa = MBO_CELL_CAPA_NOT_AVAILABLE;
	}

#ifdef ENABLE_PRIV_CMD_UPDATE_MBO_CELL_STATUS
	char mbo_cmd[32];
	char buf[32];

	os_snprintf(mbo_cmd, sizeof(mbo_cmd), "%s %d", "MBO CELL_DATA_CAP", mbo_cell_capa);
	if (wpa_drv_driver_cmd(wpa_s, mbo_cmd, buf, sizeof(buf)) < 0) {
		wpa_printf(MSG_ERROR, "MBO CELL_DATA_CAP cmd failed CAP:%d", mbo_cell_capa);
	}
#else
	wpas_mbo_update_cell_capa(wpa_s, mbo_cell_capa);
#endif

	return {SupplicantStatusCode::SUCCESS, ""};
#else
	return {SupplicantStatusCode::FAILURE_UNKNOWN, ""};
#endif
}

std::pair<SupplicantStatus, uint32_t>
StaIface::getKeyMgmtCapabilitiesInternal_1_3()
{
	struct wpa_supplicant *wpa_s = retrieveIfacePtr();
	struct wpa_driver_capa capa;
	uint32_t mask = 0;

	/* Get capabilities from driver and populate the key management mask */
	if (wpa_drv_get_capa(wpa_s, &capa) < 0) {
		return {{SupplicantStatusCode::FAILURE_UNKNOWN, ""}, mask};
	}

	return {{SupplicantStatusCode::SUCCESS, ""},
	    convertWpaKeyMgmtCapabilitiesToHidl(wpa_s, &capa)};
}

/**
 * Retrieve the underlying |wpa_supplicant| struct
 * pointer for this iface.
 * If the underlying iface is removed, then all RPC method calls on this object
 * will return failure.
 */
wpa_supplicant *StaIface::retrieveIfacePtr()
{
	return wpa_supplicant_get_iface(wpa_global_, ifname_.c_str());
}
}  // namespace implementation
}  // namespace V1_4
}  // namespace supplicant
}  // namespace wifi
}  // namespace hardware
}  // namespace android
