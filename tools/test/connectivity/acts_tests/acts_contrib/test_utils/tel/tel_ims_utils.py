#!/usr/bin/env python3
#
#   Copyright 2021 - Google
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from acts_contrib.test_utils.tel.tel_subscription_utils import get_outgoing_voice_sub_id
from acts_contrib.test_utils.tel.tel_defines import CAPABILITY_VOLTE
from acts_contrib.test_utils.tel.tel_defines import CAPABILITY_WFC
from acts_contrib.test_utils.tel.tel_defines import CarrierConfigs
from acts_contrib.test_utils.tel.tel_defines import NETWORK_SERVICE_VOICE
from acts_contrib.test_utils.tel.tel_defines import RAT_LTE
from acts_contrib.test_utils.tel.tel_defines import RAT_NR
from acts_contrib.test_utils.tel.tel_defines import RAT_UNKNOWN
from acts_contrib.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts_contrib.test_utils.tel.tel_test_utils import call_setup_teardown
from acts_contrib.test_utils.tel.tel_test_utils import ensure_phone_subscription
from acts_contrib.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts_contrib.test_utils.tel.tel_test_utils import get_user_config_profile
from acts_contrib.test_utils.tel.tel_test_utils import set_wfc_mode
from acts_contrib.test_utils.tel.tel_test_utils import toggle_airplane_mode_by_adb
from acts_contrib.test_utils.tel.tel_test_utils import toggle_volte
from acts_contrib.test_utils.tel.tel_test_utils import toggle_wfc
from acts_contrib.test_utils.tel.tel_test_utils import wait_for_network_rat
from acts_contrib.test_utils.tel.tel_test_utils import wait_for_not_network_rat
from acts_contrib.test_utils.tel.tel_test_utils import wait_for_state
from acts_contrib.test_utils.tel.tel_test_utils import wait_for_voice_attach
from acts_contrib.test_utils.tel.tel_test_utils import wait_for_volte_enabled
from acts_contrib.test_utils.tel.tel_test_utils import wait_for_wfc_disabled
from acts_contrib.test_utils.tel.tel_test_utils import wait_for_wfc_enabled
from acts_contrib.test_utils.tel.tel_test_utils import wifi_toggle_state
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_iwlan
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_volte


def check_call(log, dut, dut_client):
    result = True
    if not call_setup_teardown(log, dut_client, dut,
                               dut):
        if not call_setup_teardown(log, dut_client,
                                   dut, dut):
            dut.log.error("MT call failed")
            result = False
    if not call_setup_teardown(log, dut, dut_client,
                               dut):
        dut.log.error("MO call failed")
        result = False
    return result


def check_call_in_wfc(log, dut, dut_client):
    result = True
    if not call_setup_teardown(log, dut_client, dut,
                               dut, None, is_phone_in_call_iwlan):
        if not call_setup_teardown(log, dut_client,
                                   dut, dut, None,
                                   is_phone_in_call_iwlan):
            dut.log.error("MT WFC call failed")
            result = False
    if not call_setup_teardown(log, dut, dut_client,
                               dut, is_phone_in_call_iwlan):
        dut.log.error("MO WFC call failed")
        result = False
    return result


def check_call_in_volte(log, dut, dut_client):
    result = True
    if not call_setup_teardown(log, dut_client, dut,
                               dut, None, is_phone_in_call_volte):
        if not call_setup_teardown(log, dut_client,
                                   dut, dut, None,
                                   is_phone_in_call_volte):
            dut.log.error("MT VoLTE call failed")
            result = False
    if not call_setup_teardown(log, dut, dut_client,
                               dut, is_phone_in_call_volte):
        dut.log.error("MO VoLTE call failed")
        result = False
    return result


def change_ims_setting(log,
                       ad,
                       dut_client,
                       wifi_network_ssid,
                       wifi_network_pass,
                       subid,
                       dut_capabilities,
                       airplane_mode,
                       wifi_enabled,
                       volte_enabled,
                       wfc_enabled,
                       nw_gen=RAT_LTE,
                       wfc_mode=None):
    result = True
    ad.log.info(
        "Setting APM %s, WIFI %s, VoLTE %s, WFC %s, WFC mode %s",
        airplane_mode, wifi_enabled, volte_enabled, wfc_enabled, wfc_mode)

    toggle_airplane_mode_by_adb(log, ad, airplane_mode)
    if wifi_enabled:
        if not ensure_wifi_connected(log, ad,
                                     wifi_network_ssid,
                                     wifi_network_pass,
                                     apm=airplane_mode):
            ad.log.error("Fail to connected to WiFi")
            result = False
    else:
        if not wifi_toggle_state(log, ad, False):
            ad.log.error("Failed to turn off WiFi.")
            result = False
    toggle_volte(log, ad, volte_enabled)
    toggle_wfc(log, ad, wfc_enabled)
    if wfc_mode:
        set_wfc_mode(log, ad, wfc_mode)
    wfc_mode = ad.droid.imsGetWfcMode()
    if wifi_enabled or not airplane_mode:
        if not ensure_phone_subscription(log, ad):
            ad.log.error("Failed to find valid subscription")
            result = False
    if airplane_mode:
        if (CAPABILITY_WFC in dut_capabilities) and (wifi_enabled
                                                          and wfc_enabled):
            if not wait_for_wfc_enabled(log, ad):
                result = False
            elif not check_call_in_wfc(log, ad, dut_client):
                result = False
        else:
            if not wait_for_state(
                    ad.droid.telephonyGetCurrentVoiceNetworkType,
                    RAT_UNKNOWN):
                ad.log.error(
                    "Voice RAT is %s not UNKNOWN",
                    ad.droid.telephonyGetCurrentVoiceNetworkType())
                result = False
            else:
                ad.log.info("Voice RAT is in UNKKNOWN")
    else:
        if (wifi_enabled and wfc_enabled) and (
                wfc_mode == WFC_MODE_WIFI_PREFERRED) and (
                    CAPABILITY_WFC in dut_capabilities):
            if not wait_for_wfc_enabled(log, ad):
                result = False
            if not wait_for_state(
                    ad.droid.telephonyGetCurrentVoiceNetworkType,
                    RAT_UNKNOWN):
                ad.log.error(
                    "Voice RAT is %s, not UNKNOWN",
                    ad.droid.telephonyGetCurrentVoiceNetworkType())
            if not check_call_in_wfc(log, ad, dut_client):
                result = False
        else:
            if not wait_for_wfc_disabled(log, ad):
               ad.log.error("WFC is not disabled")
               result = False
            if volte_enabled and CAPABILITY_VOLTE in dut_capabilities:
               if not wait_for_volte_enabled(log, ad):
                    result = False
               if not check_call_in_volte(log, ad, dut_client):
                    result = False
            else:
                if not wait_for_not_network_rat(
                        log,
                        ad,
                        nw_gen,
                        voice_or_data=NETWORK_SERVICE_VOICE):
                    ad.log.error(
                        "Voice RAT is %s",
                        ad.droid.telephonyGetCurrentVoiceNetworkType(
                        ))
                    result = False
                if not wait_for_voice_attach(log, ad):
                    result = False
                if not check_call(log, ad, dut_client):
                    result = False
    user_config_profile = get_user_config_profile(ad)
    ad.log.info("user_config_profile: %s ",
                      sorted(user_config_profile.items()))
    return result


def verify_default_ims_setting(log,
                       ad,
                       dut_client,
                       carrier_configs,
                       default_wfc_enabled,
                       default_volte,
                       wfc_mode=None):
    result = True
    airplane_mode = ad.droid.connectivityCheckAirplaneMode()
    default_wfc_mode = carrier_configs.get(
        CarrierConfigs.DEFAULT_WFC_IMS_MODE_INT, wfc_mode)
    if default_wfc_enabled:
        wait_for_wfc_enabled(log, ad)
    else:
        wait_for_wfc_disabled(log, ad)
        if airplane_mode:
            wait_for_network_rat(
                log,
                ad,
                RAT_UNKNOWN,
                voice_or_data=NETWORK_SERVICE_VOICE)
        else:
            if default_volte:
                wait_for_volte_enabled(log, ad)
            else:
                wait_for_not_network_rat(
                    log,
                    ad,
                    RAT_UNKNOWN,
                    voice_or_data=NETWORK_SERVICE_VOICE)

    if not ensure_phone_subscription(log, ad):
        ad.log.error("Failed to find valid subscription")
        result = False
    user_config_profile = get_user_config_profile(ad)
    ad.log.info("user_config_profile = %s ",
                      sorted(user_config_profile.items()))
    if user_config_profile["VoLTE Enabled"] != default_volte:
        ad.log.error("VoLTE mode is not %s", default_volte)
        result = False
    else:
        ad.log.info("VoLTE mode is %s as expected",
                          default_volte)
    if user_config_profile["WFC Enabled"] != default_wfc_enabled:
        ad.log.error("WFC enabled is not %s", default_wfc_enabled)
    if user_config_profile["WFC Enabled"]:
        if user_config_profile["WFC Mode"] != default_wfc_mode:
            ad.log.error(
                "WFC mode is not %s after IMS factory reset",
                default_wfc_mode)
            result = False
        else:
            ad.log.info("WFC mode is %s as expected",
                              default_wfc_mode)
    if default_wfc_enabled and \
        default_wfc_mode == WFC_MODE_WIFI_PREFERRED:
        if not check_call_in_wfc(log, ad, dut_client):
            result = False
    elif not airplane_mode:
        if default_volte:
            if not check_call_in_volte(log, ad, dut_client):
                result = False
        else:
            if not check_call(log, ad, dut_client):
                result = False
    if result == False:
        user_config_profile = get_user_config_profile(ad)
        ad.log.info("user_config_profile = %s ",
                          sorted(user_config_profile.items()))
    return result



