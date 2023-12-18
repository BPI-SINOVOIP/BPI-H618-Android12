#!/usr/bin/env python3
#
#   Copyright 2016 - Google
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

import time
import random
import re
from queue import Empty

from acts.utils import adb_shell_ping
from acts.utils import rand_ascii_str
from acts.utils import disable_doze
from acts.utils import enable_doze
from acts_contrib.test_utils.bt.bt_test_utils import bluetooth_enabled_check
from acts_contrib.test_utils.bt.bt_test_utils import disable_bluetooth
from acts_contrib.test_utils.bt.bt_test_utils import pair_pri_to_sec
from acts_contrib.test_utils.tel.tel_subscription_utils import get_default_data_sub_id
from acts_contrib.test_utils.tel.tel_subscription_utils import get_outgoing_message_sub_id
from acts_contrib.test_utils.tel.tel_subscription_utils import get_outgoing_voice_sub_id
from acts_contrib.test_utils.tel.tel_subscription_utils import \
    get_subid_from_slot_index
from acts_contrib.test_utils.tel.tel_subscription_utils import set_subid_for_data
from acts_contrib.test_utils.tel.tel_defines import DIRECTION_MOBILE_ORIGINATED
from acts_contrib.test_utils.tel.tel_defines import EventNetworkCallback
from acts_contrib.test_utils.tel.tel_defines import GEN_5G
from acts_contrib.test_utils.tel.tel_defines import MAX_WAIT_TIME_FOR_STATE_CHANGE
from acts_contrib.test_utils.tel.tel_defines import MAX_WAIT_TIME_NW_SELECTION
from acts_contrib.test_utils.tel.tel_defines import MAX_WAIT_TIME_USER_PLANE_DATA
from acts_contrib.test_utils.tel.tel_defines import MAX_WAIT_TIME_WIFI_CONNECTION
from acts_contrib.test_utils.tel.tel_defines import NETWORK_SERVICE_DATA
from acts_contrib.test_utils.tel.tel_defines import NETWORK_SERVICE_VOICE
from acts_contrib.test_utils.tel.tel_defines import RAT_5G
from acts_contrib.test_utils.tel.tel_defines import RAT_UNKNOWN
from acts_contrib.test_utils.tel.tel_defines import WAIT_TIME_AFTER_REBOOT
from acts_contrib.test_utils.tel.tel_defines import WAIT_TIME_ANDROID_STATE_SETTLING
from acts_contrib.test_utils.tel.tel_defines import WAIT_TIME_BETWEEN_REG_AND_CALL
from acts_contrib.test_utils.tel.tel_defines import WAIT_TIME_BETWEEN_STATE_CHECK
from acts_contrib.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL_FOR_IMS
from acts_contrib.test_utils.tel.tel_defines import \
    WAIT_TIME_DATA_STATUS_CHANGE_DURING_WIFI_TETHERING
from acts_contrib.test_utils.tel.tel_defines import TETHERING_MODE_WIFI
from acts_contrib.test_utils.tel.tel_defines import MAX_WAIT_TIME_TETHERING_ENTITLEMENT_CHECK
from acts_contrib.test_utils.tel.tel_defines import WAIT_TIME_TETHERING_AFTER_REBOOT
from acts_contrib.test_utils.tel.tel_test_utils import call_setup_teardown
from acts_contrib.test_utils.tel.tel_test_utils import check_is_wifi_connected
from acts_contrib.test_utils.tel.tel_test_utils import ensure_network_generation
from acts_contrib.test_utils.tel.tel_test_utils import ensure_network_generation_for_subscription
from acts_contrib.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts_contrib.test_utils.tel.tel_test_utils import ensure_phone_idle
from acts_contrib.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts_contrib.test_utils.tel.tel_test_utils import get_mobile_data_usage
from acts_contrib.test_utils.tel.tel_test_utils import get_network_rat_for_subscription
from acts_contrib.test_utils.tel.tel_test_utils import get_service_state_by_adb
from acts_contrib.test_utils.tel.tel_test_utils import get_wifi_usage
from acts_contrib.test_utils.tel.tel_test_utils import hangup_call
from acts_contrib.test_utils.tel.tel_test_utils import is_droid_in_network_generation_for_subscription
from acts_contrib.test_utils.tel.tel_test_utils import is_ims_registered
from acts_contrib.test_utils.tel.tel_test_utils import multithread_func
from acts_contrib.test_utils.tel.tel_test_utils import rat_generation_from_rat
from acts_contrib.test_utils.tel.tel_test_utils import set_wifi_to_default
from acts_contrib.test_utils.tel.tel_test_utils import start_youtube_video
from acts_contrib.test_utils.tel.tel_test_utils import start_wifi_tethering
from acts_contrib.test_utils.tel.tel_test_utils import stop_wifi_tethering
from acts_contrib.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts_contrib.test_utils.tel.tel_test_utils import verify_http_connection
from acts_contrib.test_utils.tel.tel_test_utils import verify_incall_state
from acts_contrib.test_utils.tel.tel_test_utils import verify_internet_connection
from acts_contrib.test_utils.tel.tel_test_utils import wait_for_cell_data_connection
from acts_contrib.test_utils.tel.tel_test_utils import wait_for_data_attach_for_subscription
from acts_contrib.test_utils.tel.tel_test_utils import wait_for_network_service
from acts_contrib.test_utils.tel.tel_test_utils import wait_for_state
from acts_contrib.test_utils.tel.tel_test_utils import \
    wait_for_voice_attach_for_subscription
from acts_contrib.test_utils.tel.tel_test_utils import wait_for_wifi_data_connection
from acts_contrib.test_utils.tel.tel_test_utils import WIFI_CONFIG_APBAND_2G
from acts_contrib.test_utils.tel.tel_test_utils import WIFI_CONFIG_APBAND_5G
from acts_contrib.test_utils.tel.tel_test_utils import wifi_reset
from acts_contrib.test_utils.tel.tel_test_utils import wifi_toggle_state
from acts_contrib.test_utils.tel.tel_test_utils import active_file_download_task
from acts_contrib.test_utils.tel.tel_test_utils import run_multithread_func
from acts_contrib.test_utils.tel.tel_test_utils import ensure_phones_default_state
from acts_contrib.test_utils.tel.tel_test_utils import WIFI_SSID_KEY
from acts_contrib.test_utils.tel.tel_test_utils import is_phone_in_call_active
from acts_contrib.test_utils.tel.tel_5g_utils import is_current_network_5g_nsa
from acts_contrib.test_utils.tel.tel_5g_test_utils import check_current_network_5g
from acts_contrib.test_utils.tel.tel_5g_test_utils import provision_device_for_5g
from acts_contrib.test_utils.tel.tel_5g_test_utils import verify_5g_attach_for_both_devices
from acts_contrib.test_utils.tel.tel_voice_utils import phone_setup_voice_general
from acts_contrib.test_utils.tel.tel_voice_utils import phone_setup_iwlan
from acts_contrib.test_utils.tel.tel_voice_utils import two_phone_call_short_seq
from acts_contrib.test_utils.tel.tel_voice_utils import phone_idle_iwlan
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_iwlan


def wifi_tethering_cleanup(log, provider, client_list):
    """Clean up steps for WiFi Tethering.

    Make sure provider turn off tethering.
    Make sure clients reset WiFi and turn on cellular data.

    Args:
        log: log object.
        provider: android object provide WiFi tethering.
        client_list: a list of clients using tethered WiFi.

    Returns:
        True if no error happened. False otherwise.
    """
    for client in client_list:
        client.droid.telephonyToggleDataConnection(True)
        set_wifi_to_default(log, client)
    # If wifi tethering is enabled, disable it.
    if provider.droid.wifiIsApEnabled() and not stop_wifi_tethering(log, provider):
        provider.log.error("Provider stop WiFi tethering failed.")
        return False
    if provider.droid.wifiIsApEnabled():
        provider.log.error("Provider WiFi tethering is still enabled.")
        return False
    return True


def wifi_tethering_setup_teardown(log,
                                  provider,
                                  client_list,
                                  ap_band=WIFI_CONFIG_APBAND_2G,
                                  check_interval=30,
                                  check_iteration=4,
                                  do_cleanup=True,
                                  ssid=None,
                                  password=None):
    """Test WiFi Tethering.

    Turn off WiFi on clients.
    Turn off data and reset WiFi on clients.
    Verify no Internet access on clients.
    Turn on WiFi tethering on provider.
    Clients connect to provider's WiFI.
    Verify Internet on provider and clients.
    Tear down WiFi tethering setup and clean up.

    Args:
        log: log object.
        provider: android object provide WiFi tethering.
        client_list: a list of clients using tethered WiFi.
        ap_band: setup WiFi tethering on 2G or 5G.
            This is optional, default value is WIFI_CONFIG_APBAND_2G
        check_interval: delay time between each around of Internet connection check.
            This is optional, default value is 30 (seconds).
        check_iteration: check Internet connection for how many times in total.
            This is optional, default value is 4 (4 times).
        do_cleanup: after WiFi tethering test, do clean up to tear down tethering
            setup or not. This is optional, default value is True.
        ssid: use this string as WiFi SSID to setup tethered WiFi network.
            This is optional. Default value is None.
            If it's None, a random string will be generated.
        password: use this string as WiFi password to setup tethered WiFi network.
            This is optional. Default value is None.
            If it's None, a random string will be generated.

    Returns:
        True if no error happened. False otherwise.
    """
    log.info("--->Start wifi_tethering_setup_teardown<---")
    log.info("Provider: {}".format(provider.serial))
    if not provider.droid.connectivityIsTetheringSupported():
        provider.log.error(
            "Provider does not support tethering. Stop tethering test.")
        return False

    if ssid is None:
        ssid = rand_ascii_str(10)
    if password is None:
        password = rand_ascii_str(8)

    # No password
    if password == "":
        password = None

    try:
        for client in client_list:
            log.info("Client: {}".format(client.serial))
            wifi_toggle_state(log, client, False)
            client.droid.telephonyToggleDataConnection(False)
        log.info("WiFI Tethering: Verify client have no Internet access.")
        for client in client_list:
            if not verify_internet_connection(
                    log, client, expected_state=False):
                client.log.error("Turn off Data on client fail")
                return False

        provider.log.info(
            "Provider turn on WiFi tethering. SSID: %s, password: %s", ssid,
            password)

        if not start_wifi_tethering(log, provider, ssid, password, ap_band):
            provider.log.error("Provider start WiFi tethering failed.")
            return False
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        provider.log.info("Provider check Internet connection.")
        if not verify_internet_connection(log, provider):
            return False
        for client in client_list:
            client.log.info(
                "Client connect to WiFi and verify AP band correct.")
            if not ensure_wifi_connected(log, client, ssid, password):
                client.log.error("Client connect to WiFi failed.")
                return False

            wifi_info = client.droid.wifiGetConnectionInfo()
            if ap_band == WIFI_CONFIG_APBAND_5G:
                if wifi_info["is_24ghz"]:
                    client.log.error("Expected 5g network. WiFi Info: %s",
                                     wifi_info)
                    return False
            else:
                if wifi_info["is_5ghz"]:
                    client.log.error("Expected 2g network. WiFi Info: %s",
                                     wifi_info)
                    return False

            client.log.info("Client check Internet connection.")
            if (not wait_for_wifi_data_connection(log, client, True)
                    or not verify_internet_connection(log, client)):
                client.log.error("No WiFi Data on client")
                return False

        if not tethering_check_internet_connection(
                log, provider, client_list, check_interval, check_iteration):
            return False

    finally:
        if (do_cleanup
                and (not wifi_tethering_cleanup(log, provider, client_list))):
            return False
    return True


def tethering_check_internet_connection(log, provider, client_list,
                                        check_interval, check_iteration):
    """During tethering test, check client(s) and provider Internet connection.

    Do the following for <check_iteration> times:
        Delay <check_interval> seconds.
        Check Tethering provider's Internet connection.
        Check each client's Internet connection.

    Args:
        log: log object.
        provider: android object provide WiFi tethering.
        client_list: a list of clients using tethered WiFi.
        check_interval: delay time between each around of Internet connection check.
        check_iteration: check Internet connection for how many times in total.

    Returns:
        True if no error happened. False otherwise.
    """
    for i in range(1, check_iteration + 1):
        result = True
        time.sleep(check_interval)
        provider.log.info(
            "Provider check Internet connection after %s seconds.",
            check_interval * i)
        if not verify_internet_connection(log, provider):
            result = False
            continue
        for client in client_list:
            client.log.info(
                "Client check Internet connection after %s seconds.",
                check_interval * i)
            if not verify_internet_connection(log, client):
                result = False
                break
        if result: return result
    return False


def wifi_cell_switching(log, ad, nw_gen, wifi_network_ssid=None, wifi_network_pass=None, sa_5g=False):
    """Test data connection network switching when phone on <nw_gen>.

    Ensure phone is on <nw_gen>
    Ensure WiFi can connect to live network,
    Airplane mode is off, data connection is on, WiFi is on.
    Turn off WiFi, verify data is on cell and browse to google.com is OK.
    Turn on WiFi, verify data is on WiFi and browse to google.com is OK.
    Turn off WiFi, verify data is on cell and browse to google.com is OK.

    Args:
        log: log object.
        ad: android object.
        wifi_network_ssid: ssid for live wifi network.
        wifi_network_pass: password for live wifi network.
        nw_gen: network generation the phone should be camped on.

    Returns:
        True if pass.
    """
    try:
        if nw_gen == GEN_5G:
            if not provision_device_for_5g(log, ad, sa_5g):
                return False
        elif nw_gen:
            if not ensure_network_generation_for_subscription(
                    log, ad, get_default_data_sub_id(ad), nw_gen,
                    MAX_WAIT_TIME_NW_SELECTION, NETWORK_SERVICE_DATA):
                ad.log.error("Device failed to register in %s", nw_gen)
                return False
        else:
            ad.log.debug("Skipping network generation since it is None")

        start_youtube_video(ad)
        # Ensure WiFi can connect to live network
        ad.log.info("Make sure phone can connect to live network by WIFI")
        if not ensure_wifi_connected(log, ad, wifi_network_ssid,
                                     wifi_network_pass):
            ad.log.error("WiFi connect fail.")
            return False
        log.info("Phone connected to WIFI.")

        log.info("Step1 Airplane Off, WiFi On, Data On.")
        toggle_airplane_mode(log, ad, False)
        wifi_toggle_state(log, ad, True)
        ad.droid.telephonyToggleDataConnection(True)
        if (not wait_for_wifi_data_connection(log, ad, True)
                or not verify_internet_connection(log, ad)):
            ad.log.error("Data is not on WiFi")
            return False

        log.info("Step2 WiFi is Off, Data is on Cell.")
        wifi_toggle_state(log, ad, False)
        if (not wait_for_cell_data_connection(log, ad, True)
                or not verify_internet_connection(log, ad)):
            ad.log.error("Data did not return to cell")
            return False

        log.info("Step3 WiFi is On, Data is on WiFi.")
        wifi_toggle_state(log, ad, True)
        if (not wait_for_wifi_data_connection(log, ad, True)
                or not verify_internet_connection(log, ad)):
            ad.log.error("Data did not return to WiFi")
            return False

        log.info("Step4 WiFi is Off, Data is on Cell.")
        wifi_toggle_state(log, ad, False)
        if (not wait_for_cell_data_connection(log, ad, True)
                or not verify_internet_connection(log, ad)):
            ad.log.error("Data did not return to cell")
            return False
        return True

    finally:
        ad.force_stop_apk("com.google.android.youtube")
        wifi_toggle_state(log, ad, False)


def airplane_mode_test(log, ad, wifi_ssid=None, retries=3):
    """ Test airplane mode basic on Phone and Live SIM.

    Test steps:
        1. Ensure airplane mode is disabled and multiple network services are
           available. Check WiFi and IMS registration status.
        2. Turn on airplane mode and ensure cellular data and internet
           connection are not available.
        3. Turn off airplane mode and then ensure multiple network services are
           available. Check if WiFi and IMS registration status keep the same.

    Args:
        log: log object.
        ad: android object.
        wifi_ssid: SSID of WiFi AP which ad should connect to.
        retries: times of retry

    Returns:
        True if pass; False if fail.
    """
    if not ensure_phones_idle(log, [ad]):
        log.error("Failed to return phones to idle.")
        return False

    try:
        log.info("Step1: disable airplane mode and ensure attach")

        if not toggle_airplane_mode(log, ad, False):
            ad.log.error("Failed to disable airplane mode,")
            return False

        wifi_connected = False
        if wifi_ssid:
            wifi_connected = check_is_wifi_connected(log, ad, wifi_ssid)

        ims_reg = is_ims_registered(log, ad)

        if not wait_for_network_service(
            log,
            ad,
            wifi_connected=wifi_connected,
            wifi_ssid=wifi_ssid,
            ims_reg=ims_reg):

            return False

        log.info("Step2: enable airplane mode and ensure detach")
        if not toggle_airplane_mode(log, ad, True):
            ad.log.error("Failed to enable Airplane Mode")
            return False

        if not wait_for_cell_data_connection(log, ad, False):
            ad.log.error("Failed to disable cell data connection")
            return False

        if not verify_internet_connection(log, ad, expected_state=False):
            ad.log.error("Data available in airplane mode.")
            return False

        log.info("Step3: disable airplane mode and ensure attach")
        if not toggle_airplane_mode(log, ad, False):
            ad.log.error("Failed to disable Airplane Mode")
            return False

        if not wait_for_network_service(
            log,
            ad,
            wifi_connected=wifi_connected,
            wifi_ssid=wifi_ssid,
            ims_reg=ims_reg):

            return False

        return True
    finally:
        toggle_airplane_mode(log, ad, False)


def data_connectivity_single_bearer(log, ad, nw_gen=None, sa_5g=False):
    """Test data connection: single-bearer (no voice).

    Turn off airplane mode, enable Cellular Data.
    Ensure phone data generation is expected.
    Verify Internet.
    Disable Cellular Data, verify Internet is inaccessible.
    Enable Cellular Data, verify Internet.

    Args:
        log: log object.
        ad: android object.
        nw_gen: network generation the phone should on.

    Returns:
        True if success.
        False if failed.
    """
    ensure_phones_idle(log, [ad])
    wait_time = MAX_WAIT_TIME_NW_SELECTION
    if getattr(ad, 'roaming', False):
        wait_time = 2 * wait_time

    if nw_gen == GEN_5G:
        if not provision_device_for_5g(log, ad, sa_5g):
            return False
    elif nw_gen:
        if not ensure_network_generation_for_subscription(
                log, ad, get_default_data_sub_id(ad), nw_gen,
                MAX_WAIT_TIME_NW_SELECTION, NETWORK_SERVICE_DATA):
            ad.log.error("Device failed to connect to %s in %s seconds.", nw_gen,
                         wait_time)
            return False
    else:
        ad.log.debug("Skipping network generation since it is None")

    try:
        log.info("Step1 Airplane Off, Data On.")
        toggle_airplane_mode(log, ad, False)
        ad.droid.telephonyToggleDataConnection(True)
        if not wait_for_cell_data_connection(log, ad, True, timeout_value=wait_time):
            ad.log.error("Failed to enable data connection.")
            return False

        log.info("Step2 Verify internet")
        if not verify_internet_connection(log, ad, retries=3):
            ad.log.error("Data not available on cell.")
            return False

        log.info("Step3 Turn off data and verify not connected.")
        ad.droid.telephonyToggleDataConnection(False)
        if not wait_for_cell_data_connection(log, ad, False):
            ad.log.error("Step3 Failed to disable data connection.")
            return False

        if not verify_internet_connection(log, ad, expected_state=False):
            ad.log.error("Step3 Data still available when disabled.")
            return False

        log.info("Step4 Re-enable data.")
        ad.droid.telephonyToggleDataConnection(True)
        if not wait_for_cell_data_connection(log, ad, True, timeout_value=wait_time):
            ad.log.error("Step4 failed to re-enable data.")
            return False
        if not verify_internet_connection(log, ad, retries=3):
            ad.log.error("Data not available on cell.")
            return False

        if nw_gen == GEN_5G:
            if not check_current_network_5g(ad, sa_5g):
                return False
        else:
            if not is_droid_in_network_generation_for_subscription(
                    log, ad, get_default_data_sub_id(ad), nw_gen,
                    NETWORK_SERVICE_DATA):
                ad.log.error("Failed: droid is no longer on correct network")
                ad.log.info("Expected:%s, Current:%s", nw_gen,
                            rat_generation_from_rat(
                                get_network_rat_for_subscription(
                                    log, ad, get_default_data_sub_id(ad),
                                    NETWORK_SERVICE_DATA)))
                return False
        return True
    finally:
        ad.droid.telephonyToggleDataConnection(True)


def change_data_sim_and_verify_data(log, ad, sim_slot):
    """Change Data SIM and verify Data attach and Internet access

    Args:
        log: log object.
        ad: android device object.
        sim_slot: SIM slot index.

    Returns:
        Data SIM changed successfully, data attached and Internet access is OK.
    """
    sub_id = get_subid_from_slot_index(log, ad, sim_slot)
    ad.log.info("Change Data to subId: %s, SIM slot: %s", sub_id, sim_slot)
    set_subid_for_data(ad, sub_id)
    if not wait_for_data_attach_for_subscription(log, ad, sub_id,
                                                 MAX_WAIT_TIME_NW_SELECTION):
        ad.log.error("Failed to attach data on subId:%s", sub_id)
        return False
    if not verify_internet_connection(log, ad):
        ad.log.error("No Internet access after changing Data SIM.")
        return False
    return True


def browsing_test(log, ad, wifi_ssid=None, pass_threshold_in_mb = 1.0):
    """ Ramdomly browse 6 among 23 selected web sites. The idle time is used to
    simulate visit duration and normally distributed with the mean 35 seconds
    and std dev 15 seconds, which means 95% of visit duration will be between
    5 and 65 seconds. DUT will enter suspend mode when idle time is greater than
    35 seconds.

    Args:
        log: log object.
        ad: android object.
        pass_threshold_in_mb: minimum traffic of browsing 6 web sites in MB for
            test pass

    Returns:
        True if the total traffic of Chrome for browsing 6 web sites is greater
        than pass_threshold_in_mb. Otherwise False.
    """
    web_sites = [
        "http://tw.yahoo.com",
        "http://24h.pchome.com.tw",
        "http://www.mobile01.com",
        "https://www.android.com/phones/",
        "http://www.books.com.tw",
        "http://www.udn.com.tw",
        "http://news.baidu.com",
        "http://www.google.com",
        "http://www.cnn.com",
        "http://www.nytimes.com",
        "http://www.amazon.com",
        "http://www.wikipedia.com",
        "http://www.ebay.com",
        "http://www.youtube.com",
        "http://espn.go.com",
        "http://www.sueddeutsche.de",
        "http://www.bild.de",
        "http://www.welt.de",
        "http://www.lefigaro.fr",
        "http://www.accuweather.com",
        "https://www.flickr.com",
        "http://world.taobao.com",
        "http://www.theguardian.com",
        "http://www.abc.net.au",
        "http://www.gumtree.com.au",
        "http://www.commbank.com.au",
        "http://www.news.com.au",
        "http://rakuten.co.jp",
        "http://livedoor.jp",
        "http://yahoo.co.jp"]

    wifi_connected = False
    if wifi_ssid and check_is_wifi_connected(ad.log, ad, wifi_ssid):
        wifi_connected = True
        usage_level_at_start = get_wifi_usage(ad, apk="com.android.chrome")
    else:
        usage_level_at_start = get_mobile_data_usage(ad, apk="com.android.chrome")

    for web_site in random.sample(web_sites, 6):
        ad.log.info("Browsing %s..." % web_site)
        ad.adb.shell(
            "am start -a android.intent.action.VIEW -d %s --es "
            "com.android.browser.application_id com.android.browser" % web_site)

        idle_time = round(random.normalvariate(35, 15))
        if idle_time < 2:
            idle_time = 2
        elif idle_time > 90:
            idle_time = 90

        ad.log.info(
            "Idle time before browsing next web site: %s sec." % idle_time)

        if idle_time > 35:
            time.sleep(35)
            rest_idle_time = idle_time-35
            if rest_idle_time < 3:
                rest_idle_time = 3
            ad.log.info("Let device go to sleep for %s sec." % rest_idle_time)
            ad.droid.wakeLockRelease()
            ad.droid.goToSleepNow()
            time.sleep(rest_idle_time)
            ad.log.info("Wake up device.")
            ad.wakeup_screen()
            ad.adb.shell("input keyevent 82")
            time.sleep(3)
        else:
            time.sleep(idle_time)

    if wifi_connected:
        usage_level = get_wifi_usage(ad, apk="com.android.chrome")
    else:
        usage_level = get_mobile_data_usage(ad, apk="com.android.chrome")

    try:
        usage = round((usage_level - usage_level_at_start)/1024/1024, 2)
        if usage < pass_threshold_in_mb:
            ad.log.error(
                "Usage of browsing '%s MB' is smaller than %s " % (
                    usage, pass_threshold_in_mb))
            return False
        else:
            ad.log.info("Usage of browsing: %s MB" % usage)
            return True
    except Exception as e:
        ad.log.error(e)
        usage = "unknown"
        ad.log.info("Usage of browsing: %s MB" % usage)
        return False


def reboot_test(log, ad, wifi_ssid=None):
    """ Reboot test to verify the service availability after reboot.

    Test procedure:
    1. Check WiFi and IMS registration status.
    2. Reboot
    3. Wait WAIT_TIME_AFTER_REBOOT for reboot complete.
    4. Wait for multiple network services, including:
       - service state
       - network connection
       - wifi connection if connected before reboot
       - cellular data
       - internet connection
       - IMS registration if available before reboot
    5. Check if DSDS mode, data sub ID, voice sub ID and message sub ID still keep the same.
    6. Check if voice and data RAT keep the same.

    Args:
        log: log object.
        ad: android object.
        wifi_ssid: SSID of Wi-Fi AP for Wi-Fi connection.

    Returns:
        True if pass; False if fail.
    """
    try:
        data_subid = get_default_data_sub_id(ad)
        voice_subid = get_outgoing_voice_sub_id(ad)
        sms_subid = get_outgoing_message_sub_id(ad)

        data_rat_before_reboot = get_network_rat_for_subscription(
            log, ad, data_subid, NETWORK_SERVICE_DATA)
        voice_rat_before_reboot = get_network_rat_for_subscription(
            log, ad, voice_subid, NETWORK_SERVICE_VOICE)

        wifi_connected = False
        if wifi_ssid:
            wifi_connected = check_is_wifi_connected(log, ad, wifi_ssid)

        ims_reg = is_ims_registered(log, ad)

        ad.reboot()
        time.sleep(WAIT_TIME_AFTER_REBOOT)

        if not wait_for_network_service(
            log,
            ad,
            wifi_connected=wifi_connected,
            wifi_ssid=wifi_ssid,
            ims_reg=ims_reg):

            return False

        sim_mode = ad.droid.telephonyGetPhoneCount()
        if getattr(ad, 'dsds', False):
            if sim_mode == 1:
                ad.log.error("Phone is in single SIM mode after reboot.")
                return False
            elif sim_mode == 2:
                ad.log.info("Phone keeps being in dual SIM mode after reboot.")
        else:
            if sim_mode == 1:
                ad.log.info("Phone keeps being in single SIM mode after reboot.")
            elif sim_mode == 2:
                ad.log.error("Phone is in dual SIM mode after reboot.")
                return False

        data_subid_after_reboot = get_default_data_sub_id(ad)
        if data_subid_after_reboot != data_subid:
            ad.log.error(
                "Data sub ID changed! (Before reboot: %s; after reboot: %s)",
                data_subid, data_subid_after_reboot)
            return False
        else:
            ad.log.info("Data sub ID does not change after reboot.")

        voice_subid_after_reboot = get_outgoing_voice_sub_id(ad)
        if voice_subid_after_reboot != voice_subid:
            ad.log.error(
                "Voice sub ID changed! (Before reboot: %s; after reboot: %s)",
                voice_subid, voice_subid_after_reboot)
            return False
        else:
            ad.log.info("Voice sub ID does not change after reboot.")

        sms_subid_after_reboot = get_outgoing_message_sub_id(ad)
        if sms_subid_after_reboot != sms_subid:
            ad.log.error(
                "Message sub ID changed! (Before reboot: %s; after reboot: %s)",
                sms_subid, sms_subid_after_reboot)
            return False
        else:
            ad.log.info("Message sub ID does not change after reboot.")

        data_rat_after_reboot = get_network_rat_for_subscription(
            log, ad, data_subid_after_reboot, NETWORK_SERVICE_DATA)
        voice_rat_after_reboot = get_network_rat_for_subscription(
            log, ad, voice_subid_after_reboot, NETWORK_SERVICE_VOICE)

        if data_rat_after_reboot == data_rat_before_reboot:
            ad.log.info(
                "Data RAT (%s) does not change after reboot.",
                data_rat_after_reboot)
        else:
            ad.log.error(
                "Data RAT changed! (Before reboot: %s; after reboot: %s)",
                data_rat_before_reboot,
                data_rat_after_reboot)
            return False

        if voice_rat_after_reboot == voice_rat_before_reboot:
            ad.log.info(
                "Voice RAT (%s) does not change after reboot.",
                voice_rat_after_reboot)
        else:
            ad.log.error(
                "Voice RAT changed! (Before reboot: %s; after reboot: %s)",
                voice_rat_before_reboot,
                voice_rat_after_reboot)
            return False

    except Exception as e:
        ad.log.error(e)
        return False

    return True


def verify_for_network_callback(log, ad, event, apm_mode=False):
    """Verify network callback for Meteredness

    Args:
        ad: DUT to get the network callback for
        event: Network callback event

    Returns:
        True: if the expected network callback found, False if not
    """
    key = ad.droid.connectivityRegisterDefaultNetworkCallback()
    ad.droid.connectivityNetworkCallbackStartListeningForEvent(key, event)
    if apm_mode:
        ad.log.info("Turn on Airplane Mode")
        toggle_airplane_mode(ad.log, ad, True)
    curr_time = time.time()
    status = False
    while time.time() < curr_time + MAX_WAIT_TIME_USER_PLANE_DATA:
        try:
            nc_event = ad.ed.pop_event(EventNetworkCallback)
            ad.log.info("Received: %s" %
                        nc_event["data"]["networkCallbackEvent"])
            if nc_event["data"]["networkCallbackEvent"] == event:
                status = nc_event["data"]["metered"]
                ad.log.info("Current state of Meteredness is %s", status)
                break
        except Empty:
            pass

    ad.droid.connectivityNetworkCallbackStopListeningForEvent(key, event)
    ad.droid.connectivityUnregisterNetworkCallback(key)
    if apm_mode:
        ad.log.info("Turn off Airplane Mode")
        toggle_airplane_mode(ad.log, ad, False)
        time.sleep(WAIT_TIME_BETWEEN_STATE_CHECK)
    return status


def test_data_connectivity_multi_bearer(
        log,
        android_devices,
        nw_gen=None,
        simultaneous_voice_data=True,
        call_direction=DIRECTION_MOBILE_ORIGINATED,
        sa_5g=False):
    """Test data connection before call and in call.

    Turn off airplane mode, disable WiFi, enable Cellular Data.
    Make sure phone in <nw_gen>, verify Internet.
    Initiate a voice call.
    if simultaneous_voice_data is True, then:
        Verify Internet.
        Disable Cellular Data, verify Internet is inaccessible.
        Enable Cellular Data, verify Internet.
    if simultaneous_voice_data is False, then:
        Verify Internet is not available during voice call.
    Hangup Voice Call, verify Internet.

    Returns:
        True if success.
        False if failed.
    """

    class _LocalException(Exception):
        pass

    ad_list = [android_devices[0], android_devices[1]]
    ensure_phones_idle(log, ad_list)

    if nw_gen == GEN_5G:
        if not provision_device_for_5g(log, android_devices, sa_5g):
            return False
    elif nw_gen:
        if not ensure_network_generation_for_subscription(
                log, android_devices[0], android_devices[0]
                .droid.subscriptionGetDefaultDataSubId(), nw_gen,
                MAX_WAIT_TIME_NW_SELECTION, NETWORK_SERVICE_DATA):
            log.error("Device failed to reselect in {}s.".format(
                MAX_WAIT_TIME_NW_SELECTION))
            return False
    else:
        log.debug("Skipping network generation since it is None")

    if not wait_for_voice_attach_for_subscription(
            log, android_devices[0], android_devices[0]
            .droid.subscriptionGetDefaultVoiceSubId(),
            MAX_WAIT_TIME_NW_SELECTION):
        return False

    log.info("Step1 WiFi is Off, Data is on Cell.")
    toggle_airplane_mode(log, android_devices[0], False)
    wifi_toggle_state(log, android_devices[0], False)
    android_devices[0].droid.telephonyToggleDataConnection(True)
    if (not wait_for_cell_data_connection(log,
                                          android_devices[0], True)
            or not verify_internet_connection(log,
                                              android_devices[0])):
        log.error("Data not available on cell")
        return False

    log.info(
        "b/69431819, sending data to increase NW threshold limit")
    adb_shell_ping(
        android_devices[0], count=30, timeout=60, loss_tolerance=100)

    try:
        log.info("Step2 Initiate call and accept.")
        if call_direction == DIRECTION_MOBILE_ORIGINATED:
            ad_caller = android_devices[0]
            ad_callee = android_devices[1]
        else:
            ad_caller = android_devices[1]
            ad_callee = android_devices[0]
        if not call_setup_teardown(log, ad_caller, ad_callee, None,
                                   None, None):
            log.error(
                "Failed to Establish {} Voice Call".format(call_direction))
            return False
        if simultaneous_voice_data:
            log.info("Step3 Verify internet.")
            time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
            if not verify_internet_connection(
                    log, android_devices[0], retries=3):
                raise _LocalException("Internet Inaccessible when Enabled")

            log.info("Step4 Turn off data and verify not connected.")
            android_devices[0].droid.telephonyToggleDataConnection(
                False)
            if not wait_for_cell_data_connection(
                    log, android_devices[0], False):
                raise _LocalException("Failed to Disable Cellular Data")

            if not verify_internet_connection(log,
                                          android_devices[0], expected_state=False):
                raise _LocalException("Internet Accessible when Disabled")

            log.info("Step5 Re-enable data.")
            android_devices[0].droid.telephonyToggleDataConnection(
                True)
            if not wait_for_cell_data_connection(
                    log, android_devices[0], True):
                raise _LocalException("Failed to Re-Enable Cellular Data")
            if not verify_internet_connection(
                    log, android_devices[0], retries=3):
                raise _LocalException("Internet Inaccessible when Enabled")
        else:
            log.info("Step3 Verify no Internet and skip step 4-5.")
            if verify_internet_connection(
                    log, android_devices[0], retries=0):
                raise _LocalException("Internet Accessible.")

        log.info("Step6 Verify phones still in call and Hang up.")
        if not verify_incall_state(
                log,
            [android_devices[0], android_devices[1]], True):
            return False
        if not hangup_call(log, android_devices[0]):
            log.error("Failed to hang up call")
            return False
        if not verify_internet_connection(
                log, android_devices[0], retries=3):
            raise _LocalException("Internet Inaccessible when Enabled")

    except _LocalException as e:
        log.error(str(e))
        try:
            hangup_call(log, android_devices[0])
            android_devices[0].droid.telephonyToggleDataConnection(
                True)
        except Exception:
            pass
        return False

    return True


def test_internet_connection(log, provider, clients,
                              client_status=True,
                              provider_status=True):
    client_retry = 10 if client_status else 1
    for client in clients:
        if not verify_internet_connection(
                log,
                client,
                retries=client_retry,
                expected_state=client_status):
            client.log.error("client internet connection state is not %s",
                             client_status)
            return False
        else:
            client.log.info("client internet connection state is %s",
                            client_status)
    if not verify_internet_connection(
            log, provider, retries=3,
            expected_state=provider_status):
        provider.log.error(
            "provider internet connection is not %s" % provider_status)
        return False
    else:
        provider.log.info(
            "provider internet connection is %s" % provider_status)
    return True


def test_setup_tethering(log, provider, clients, network_generation=None, sa_5g=False):
    """Pre setup steps for WiFi tethering test.

    Ensure all ads are idle.
    Ensure tethering provider:
        turn off APM, turn off WiFI, turn on Data.
        have Internet connection, no active ongoing WiFi tethering.

    Returns:
        True if success.
        False if failed.
    """

    ensure_phone_idle(log, provider)
    ensure_phones_idle(log, clients)
    wifi_toggle_state(log, provider, False)
    if network_generation == RAT_5G:
        if not provision_device_for_5g(log, provider, sa_5g):
            return False
    elif network_generation:
        if not ensure_network_generation(
                log, provider, network_generation,
                MAX_WAIT_TIME_NW_SELECTION, NETWORK_SERVICE_DATA):
            provider.log.error("Provider failed to connect to %s.",
                                    network_generation)
            return False
    else:
        log.debug("Skipping network generation since it is None")

    provider.log.info(
        "Set provider Airplane Off, Wifi Off, Bluetooth Off, Data On.")
    toggle_airplane_mode(log, provider, False)
    provider.droid.telephonyToggleDataConnection(True)
    provider.log.info("Provider disable wifi")
    wifi_toggle_state(log, provider, False)

    # Turn off active SoftAP if any.
    if provider.droid.wifiIsApEnabled():
        provider.log.info("Disable provider wifi tethering")
        stop_wifi_tethering(log, provider)
    provider.log.info("Provider disable bluetooth")
    disable_bluetooth(provider.droid)
    time.sleep(10)

    for ad in clients:
        ad.log.info(
            "Set client Airplane Off, Wifi Off, Bluetooth Off, Data Off.")
        toggle_airplane_mode(log, ad, False)
        ad.log.info("Client disable data")
        ad.droid.telephonyToggleDataConnection(False)
        ad.log.info("Client disable bluetooth")
        disable_bluetooth(ad.droid)
        ad.log.info("Client disable wifi")
        wifi_toggle_state(log, ad, False)

    if not wait_for_cell_data_connection(log, provider, True):
        provider.log.error(
            "Provider failed to enable data connection.")
        return False

    time.sleep(10)
    log.info("Verify internet")
    if not test_internet_connection(log, provider, clients,
            client_status=False, provider_status=True):
        log.error("Internet connection check failed before tethering")
        return False

    return True


def enable_bluetooth_tethering_connection(log, provider, clients):
    for ad in [provider] + clients:
        if not bluetooth_enabled_check(ad):
            ad.log.info("Bluetooth is not enabled")
            return False
        else:
            ad.log.info("Bluetooth is enabled")
    time.sleep(5)
    provider.log.info("Provider enabling bluetooth tethering")
    try:
        provider.droid.bluetoothPanSetBluetoothTethering(True)
    except Exception as e:
        provider.log.warning(
            "Failed to enable provider Bluetooth tethering with %s", e)
        provider.droid.bluetoothPanSetBluetoothTethering(True)

    if wait_for_state(provider.droid.bluetoothPanIsTetheringOn, True):
        provider.log.info("Provider Bluetooth tethering is enabled.")
    else:
        provider.log.error(
            "Failed to enable provider Bluetooth tethering.")
        provider.log.error("bluetoothPanIsTetheringOn = %s",
                           provider.droid.bluetoothPanIsTetheringOn())
        return False
    for client in clients:
        if not (pair_pri_to_sec(provider, client)):
            client.log.error("Client failed to pair with provider")
            return False
        else:
            client.log.info("Client paired with provider")

    time.sleep(5)
    for client in clients:
        client.droid.bluetoothConnectBonded(provider.droid.bluetoothGetLocalAddress())

    time.sleep(20)
    return True


def verify_bluetooth_tethering_connection(log, provider, clients,
                                           change_rat=None,
                                           toggle_data=False,
                                           toggle_tethering=False,
                                           voice_call=False,
                                           toggle_bluetooth=True):
    """Setups up a bluetooth tethering connection between two android devices.

    Returns:
        True if PAN connection and verification is successful,
        false if unsuccessful.
    """


    if not enable_bluetooth_tethering_connection(log, provider, clients):
        return False

    if not test_internet_connection(log, provider, clients):
        log.error("Internet connection check failed")
        return False
    if voice_call:
        log.info("====== Voice call test =====")
        for caller, callee in [(provider, clients[0]),
                               (clients[0], provider)]:
            if not call_setup_teardown(
                    log, caller, callee, ad_hangup=None):
                log.error("Setup Call Failed.")
                hangup_call(log, caller)
                return False
            log.info("Verify data.")
            if not verify_internet_connection(
                    log, clients[0], retries=1):
                clients[0].log.warning(
                    "client internet connection state is not on")
            else:
                clients[0].log.info(
                    "client internet connection state is on")
            hangup_call(log, caller)
            if not verify_internet_connection(
                    log, clients[0], retries=1):
                clients[0].log.warning(
                    "client internet connection state is not on")
                return False
            else:
                clients[0].log.info(
                    "client internet connection state is on")
    if toggle_tethering:
        log.info("====== Toggling provider bluetooth tethering =====")
        provider.log.info("Disable bluetooth tethering")
        provider.droid.bluetoothPanSetBluetoothTethering(False)
        if not test_internet_connection(log, provider, clients, False, True):
            log.error(
                "Internet connection check failed after disable tethering")
            return False
        provider.log.info("Enable bluetooth tethering")
        if not enable_bluetooth_tethering_connection(log,
                provider, clients):
            provider.log.error(
                "Fail to re-enable bluetooth tethering")
            return False
        if not test_internet_connection(log, provider, clients, True, True):
            log.error(
                "Internet connection check failed after enable tethering")
            return False
    if toggle_bluetooth:
        log.info("====== Toggling provider bluetooth =====")
        provider.log.info("Disable provider bluetooth")
        disable_bluetooth(provider.droid)
        time.sleep(10)
        if not test_internet_connection(log, provider, clients, False, True):
            log.error(
                "Internet connection check failed after disable bluetooth")
            return False
        if not enable_bluetooth_tethering_connection(log,
                provider, clients):
            provider.log.error(
                "Fail to re-enable bluetooth tethering")
            return False
        if not test_internet_connection(log, provider, clients, True, True):
            log.error(
                "Internet connection check failed after enable bluetooth")
            return False
    if toggle_data:
        log.info("===== Toggling provider data connection =====")
        provider.log.info("Disable provider data connection")
        provider.droid.telephonyToggleDataConnection(False)
        time.sleep(10)
        if not test_internet_connection(log, provider, clients, False, False):
            return False
        provider.log.info("Enable provider data connection")
        provider.droid.telephonyToggleDataConnection(True)
        if not wait_for_cell_data_connection(log, provider,
                                             True):
            provider.log.error(
                "Provider failed to enable data connection.")
            return False
        if not test_internet_connection(log, provider, clients, True, True):
            log.error(
                "Internet connection check failed after enable data")
            return False
    if change_rat:
        log.info("===== Change provider RAT to %s =====", change_rat)
        if not ensure_network_generation(
                log,
                provider,
                change_rat,
                voice_or_data=NETWORK_SERVICE_DATA,
                toggle_apm_after_setting=False):
            provider.log.error("Provider failed to reselect to %s.",
                                    change_rat)
            return False
        if not test_internet_connection(log, provider, clients, True, True):
            log.error(
                "Internet connection check failed after RAT change to %s",
                change_rat)
            return False
    return True


def test_tethering_wifi_and_voice_call(log, provider, clients,
                                        provider_data_rat,
                                        provider_setup_func,
                                        provider_in_call_check_func,
                                        sa_5g=False):

    if not test_setup_tethering(log, provider, clients, provider_data_rat):
        log.error("Verify 4G Internet access failed.")
        return False

    tasks = [(provider_setup_func, (log, provider)),
             (phone_setup_voice_general, (log, clients[0]))]
    if not multithread_func(log, tasks):
        log.error("Phone Failed to Set Up VoLTE.")
        return False

    if provider_setup_func == RAT_5G:
        if not provision_device_for_5g(log, provider, sa_5g):
            return False
    try:
        log.info("1. Setup WiFi Tethering.")
        if not wifi_tethering_setup_teardown(
                log,
                provider, [clients[0]],
                ap_band=WIFI_CONFIG_APBAND_2G,
                check_interval=10,
                check_iteration=2,
                do_cleanup=False):
            log.error("WiFi Tethering failed.")
            return False
        log.info("2. Make outgoing call.")
        if not call_setup_teardown(
                log,
                provider,
                clients[0],
                ad_hangup=None,
                verify_caller_func=provider_in_call_check_func):
            log.error("Setup Call Failed.")
            return False
        log.info("3. Verify data.")
        if not verify_internet_connection(log, provider):
            provider.log.error("Provider have no Internet access.")
        if not verify_internet_connection(log, clients[0]):
            clients[0].log.error("Client have no Internet access.")
        hangup_call(log, provider)
        time.sleep(WAIT_TIME_BETWEEN_REG_AND_CALL)

        log.info("4. Make incoming call.")
        if not call_setup_teardown(
                log,
                clients[0],
                provider,
                ad_hangup=None,
                verify_callee_func=provider_in_call_check_func):
            log.error("Setup Call Failed.")
            return False
        log.info("5. Verify data.")
        if not verify_internet_connection(log, provider):
            provider.log.error("Provider have no Internet access.")
        if not verify_internet_connection(log, clients[0]):
            clients[0].log.error("Client have no Internet access.")
        hangup_call(log, provider)

    finally:
        if not wifi_tethering_cleanup(log, provider, clients):
            return False
    return True


def test_wifi_connect_disconnect(log, ad, wifi_network_ssid=None, wifi_network_pass=None):
    """Perform multiple connects and disconnects from WiFi and verify that
        data switches between WiFi and Cell.

    Steps:
    1. Reset Wifi on DUT
    2. Connect DUT to a WiFi AP
    3. Repeat steps 1-2, alternately disconnecting and disabling wifi

    Expected Results:
    1. Verify Data on Cell
    2. Verify Data on Wifi

    Returns:
        True if success.
        False if failed.
    """

    wifi_toggles = [
        True, False, True, False, False, True, False, False, False, False,
        True, False, False, False, False, False, False, False, False
    ]

    for toggle in wifi_toggles:

        wifi_reset(log, ad, toggle)

        if not wait_for_cell_data_connection(
                log, ad, True, MAX_WAIT_TIME_WIFI_CONNECTION):
            log.error("Failed wifi connection, aborting!")
            return False

        if not verify_internet_connection(log, ad):
            log.error("Failed to get user-plane traffic, aborting!")
            return False

        if toggle:
            wifi_toggle_state(log, ad, True)

        ensure_wifi_connected(log, ad, wifi_network_ssid,
                     wifi_network_pass)

        if not wait_for_wifi_data_connection(
                log, ad, True, MAX_WAIT_TIME_WIFI_CONNECTION):
            log.error("Failed wifi connection, aborting!")
            return False

        if not verify_http_connection(
                log, ad, 'http://www.google.com', 100, .1):
            log.error("Failed to get user-plane traffic, aborting!")
            return False
    return True


def test_call_setup_in_active_data_transfer(
        log,
        ads,
        nw_gen=None,
        call_direction=DIRECTION_MOBILE_ORIGINATED,
        allow_data_transfer_interruption=False,
        sa_5g=False):
    """Test call can be established during active data connection.

    Turn off airplane mode, disable WiFi, enable Cellular Data.
    Make sure phone in <nw_gen>.
    Starting downloading file from Internet.
    Initiate a voice call. Verify call can be established.
    Hangup Voice Call, verify file is downloaded successfully.
    Note: file download will be suspended when call is initiated if voice
          is using voice channel and voice channel and data channel are
          on different RATs.
    Args:
        log: log object.
        ads: list of android objects, this list should have two ad.
        nw_gen: network generation.
        call_direction: MO(DIRECTION_MOBILE_ORIGINATED) or MT(DIRECTION_MOBILE_TERMINATED) call.
        allow_data_transfer_interruption: if allow to interrupt data transfer.

    Returns:
        True if success.
        False if failed.
    """

    def _call_setup_teardown(log, ad_caller, ad_callee, ad_hangup,
                             caller_verifier, callee_verifier,
                             wait_time_in_call):
        # wait time for active data transfer
        time.sleep(5)
        return call_setup_teardown(log, ad_caller, ad_callee, ad_hangup,
                                   caller_verifier, callee_verifier,
                                   wait_time_in_call)

    if nw_gen == GEN_5G:
        if not provision_device_for_5g(log, ads[0], sa_5g):
            return False
    elif nw_gen:
        if not ensure_network_generation(log, ads[0], nw_gen,
                                         MAX_WAIT_TIME_NW_SELECTION,
                                         NETWORK_SERVICE_DATA):
            ads[0].log.error("Device failed to reselect in %s.",
                             MAX_WAIT_TIME_NW_SELECTION)
            return False

        ads[0].droid.telephonyToggleDataConnection(True)
        if not wait_for_cell_data_connection(log, ads[0], True):
            ads[0].log.error("Data connection is not on cell")
            return False
    else:
        ads[0].log.debug("Skipping network generation since it is None")

    if not verify_internet_connection(log, ads[0]):
        ads[0].log.error("Internet connection is not available")
        return False

    if call_direction == DIRECTION_MOBILE_ORIGINATED:
        ad_caller = ads[0]
        ad_callee = ads[1]
    else:
        ad_caller = ads[1]
        ad_callee = ads[0]
    ad_download = ads[0]

    start_youtube_video(ad_download)
    call_task = (_call_setup_teardown, (log, ad_caller, ad_callee,
                                        ad_caller, None, None, 30))
    download_task = active_file_download_task(log, ad_download)
    results = run_multithread_func(log, [download_task, call_task])
    if wait_for_state(ad_download.droid.audioIsMusicActive, True, 15, 1):
        ad_download.log.info("After call hangup, audio is back to music")
    else:
        ad_download.log.warning(
            "After call hang up, audio is not back to music")
    ad_download.force_stop_apk("com.google.android.youtube")
    if not results[1]:
        log.error("Call setup failed in active data transfer.")
    if results[0]:
        ad_download.log.info("Data transfer succeeded.")
    elif not allow_data_transfer_interruption:
        ad_download.log.error(
            "Data transfer failed with parallel phone call.")
        return False
    else:
        ad_download.log.info("Retry data connection after call hung up")
        if not verify_internet_connection(log, ad_download):
            ad_download.log.error("Internet connection is not available")
            return False
    # Disable airplane mode if test under apm on.
    toggle_airplane_mode(log, ads[0], False)
    if nw_gen == GEN_5G and not check_current_network_5g(ads[0], sa_5g):
        ads[0].log.error("Phone not attached on 5G after call.")
        return False
    return True


def test_call_setup_in_active_youtube_video(
        log,
        ads,
        nw_gen=None,
        call_direction=DIRECTION_MOBILE_ORIGINATED,
        allow_data_transfer_interruption=False,
        sa_5g=False):
    """Test call can be established during active data connection.

    Turn off airplane mode, disable WiFi, enable Cellular Data.
    Make sure phone in <nw_gen>.
    Starting playing youtube video.
    Initiate a voice call. Verify call can be established.
    Args:
        log: log object.
        ads: list of android objects, this list should have two ad.
        nw_gen: network generation.
        call_direction: MO(DIRECTION_MOBILE_ORIGINATED) or MT(DIRECTION_MOBILE_TERMINATED) call.
        allow_data_transfer_interruption: if allow to interrupt data transfer.

    Returns:
        True if success.
        False if failed.
    """
    if nw_gen == GEN_5G:
        if not provision_device_for_5g(log, ads[0], sa_5g):
            return False
    elif nw_gen:
        if not ensure_network_generation(log, ads[0], nw_gen,
                                         MAX_WAIT_TIME_NW_SELECTION,
                                         NETWORK_SERVICE_DATA):
            ads[0].log.error("Device failed to reselect in %s.",
                             MAX_WAIT_TIME_NW_SELECTION)
            return False
        ads[0].droid.telephonyToggleDataConnection(True)
        if not wait_for_cell_data_connection(log, ads[0], True):
            ads[0].log.error("Data connection is not on cell")
            return False
    else:
        ensure_phones_default_state(log, ads)

    if not verify_internet_connection(log, ads[0]):
        ads[0].log.error("Internet connection is not available")
        return False

    if call_direction == DIRECTION_MOBILE_ORIGINATED:
        ad_caller = ads[0]
        ad_callee = ads[1]
    else:
        ad_caller = ads[1]
        ad_callee = ads[0]
    ad_download = ads[0]

    if not start_youtube_video(ad_download):
        ad_download.log.warning("Fail to bring up youtube video")

    if not call_setup_teardown(log, ad_caller, ad_callee, ad_caller,
                               None, None, 30):
        ad_download.log.error("Call setup failed in active youtube video")
        result = False
    else:
        ad_download.log.info("Call setup succeed in active youtube video")
        result = True

    if wait_for_state(ad_download.droid.audioIsMusicActive, True, 15, 1):
        ad_download.log.info("After call hangup, audio is back to music")
    else:
        ad_download.log.warning(
                "After call hang up, audio is not back to music")
    ad_download.force_stop_apk("com.google.android.youtube")
    # Disable airplane mode if test under apm on.
    toggle_airplane_mode(log, ads[0], False)
    if nw_gen == GEN_5G and not check_current_network_5g(ads[0], sa_5g):
        ads[0].log.error("Phone not attached on 5G after call.")
        result = False
    return result


def call_epdg_to_epdg_wfc(log,
                          ads,
                          apm_mode,
                          wfc_mode,
                          wifi_ssid,
                          wifi_pwd,
                          nw_gen=None,
                          sa_5g=False):
    """ Test epdg<->epdg call functionality.

    Make Sure PhoneA is set to make epdg call.
    Make Sure PhoneB is set to make epdg call.
    Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneA.
    Call from PhoneA to PhoneB, accept on PhoneB, hang up on PhoneB.

    Args:
        log: log object.
        ads: list of android objects, this list should have two ad.
        apm_mode: phones' airplane mode.
            if True, phones are in airplane mode during test.
            if False, phones are not in airplane mode during test.
        wfc_mode: phones' wfc mode.
            Valid mode includes: WFC_MODE_WIFI_ONLY, WFC_MODE_CELLULAR_PREFERRED,
            WFC_MODE_WIFI_PREFERRED, WFC_MODE_DISABLED.
        wifi_ssid: WiFi ssid to connect during test.
        wifi_pwd: WiFi password.
        nw_gen: network generation.

    Returns:
        True if pass; False if fail.
    """
    DEFAULT_PING_DURATION = 120

    if nw_gen == GEN_5G:
        # Turn off apm first before setting network preferred mode to 5G NSA.
        log.info("Turn off APM mode before starting testing.")
        tasks = [(toggle_airplane_mode, (log, ads[0], False)),
                 (toggle_airplane_mode, (log, ads[1], False))]
        if not multithread_func(log, tasks):
            log.error("Failed to turn off airplane mode")
            return False
        if not provision_device_for_5g(log, ad, sa_5g):
            return False

    tasks = [(phone_setup_iwlan, (log, ads[0], apm_mode, wfc_mode,
                                  wifi_ssid, wifi_pwd)),
             (phone_setup_iwlan, (log, ads[1], apm_mode, wfc_mode,
                                  wifi_ssid, wifi_pwd))]
    if not multithread_func(log, tasks):
        log.error("Phone Failed to Set Up Properly.")
        return False

    ad_ping = ads[0]

    call_task = (two_phone_call_short_seq,
                 (log, ads[0], phone_idle_iwlan,
                  is_phone_in_call_iwlan, ads[1], phone_idle_iwlan,
                  is_phone_in_call_iwlan, None, WAIT_TIME_IN_CALL_FOR_IMS))
    ping_task = (adb_shell_ping, (ad_ping, DEFAULT_PING_DURATION))

    results = run_multithread_func(log, [ping_task, call_task])

    time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

    if nw_gen == GEN_5G and not verify_5g_attach_for_both_devices(log, ads, sa_5g):
        log.error("Phone not attached on 5G after epdg call.")
        return False

    if not results[1]:
        log.error("Call setup failed in active ICMP transfer.")
    if results[0]:
        log.info("ICMP transfer succeeded with parallel phone call.")
    else:
        log.error("ICMP transfer failed with parallel phone call.")
    return all(results)


def verify_toggle_apm_tethering_internet_connection(log, provider, clients, ssid):
    """ Verify internet connection by toggling apm during wifi tethering.
    Args:
        log: log object.
        provider: android object provide WiFi tethering.
        clients: a list of clients using tethered WiFi.
        ssid: use this string as WiFi SSID to setup tethered WiFi network.

    """
    if not provider.droid.wifiIsApEnabled():
        log.error("Provider WiFi tethering stopped.")
        return False

    log.info(
        "Provider turn on APM, verify no wifi/data on Client.")

    if not toggle_airplane_mode(log, provider, True):
        log.error("Provider turn on APM failed.")
        return False
    time.sleep(WAIT_TIME_DATA_STATUS_CHANGE_DURING_WIFI_TETHERING)

    if provider.droid.wifiIsApEnabled():
        provider.log.error("Provider WiFi tethering not stopped.")
        return False

    if not verify_internet_connection(log, clients[0], expected_state=False):
        clients[0].log.error(
            "Client should not have Internet connection.")
        return False

    wifi_info = clients[0].droid.wifiGetConnectionInfo()
    clients[0].log.info("WiFi Info: {}".format(wifi_info))
    if wifi_info[WIFI_SSID_KEY] == ssid:
        clients[0].log.error(
            "WiFi error. WiFi should not be connected.".format(
                wifi_info))
        return False

    log.info("Provider turn off APM.")
    if not toggle_airplane_mode(log, provider, False):
        provider.log.error("Provider turn on APM failed.")
        return False
    time.sleep(WAIT_TIME_DATA_STATUS_CHANGE_DURING_WIFI_TETHERING)
    if provider.droid.wifiIsApEnabled():
        provider.log.error(
            "Provider WiFi tethering should not on.")
        return False
    if not verify_internet_connection(log, provider):
        provider.log.error(
            "Provider should have Internet connection.")
        return False
    return True


def verify_tethering_entitlement_check(log, provider):
    """Tethering Entitlement Check Test

    Get tethering entitlement check result.
    Args:
        log: log object.
        provider: android object provide WiFi tethering.

    Returns:
        True if entitlement check returns True.
    """
    if (not wait_for_cell_data_connection(log, provider, True)
            or not verify_internet_connection(log, provider)):
        log.error("Failed cell data call for entitlement check.")
        return False

    result = provider.droid.carrierConfigIsTetheringModeAllowed(
        TETHERING_MODE_WIFI, MAX_WAIT_TIME_TETHERING_ENTITLEMENT_CHECK)
    provider.log.info("Tethering entitlement check result: %s",
                      result)
    return result


def test_wifi_tethering(log, provider,
                        clients,
                        clients_tethering,
                        nw_gen,
                        ap_band=WIFI_CONFIG_APBAND_2G,
                        check_interval=30,
                        check_iteration=4,
                        do_cleanup=True,
                        ssid=None,
                        password=None,
                        pre_teardown_func=None):
    """WiFi Tethering test
    Args:
        log: log object.
        provider: android object provide WiFi tethering.
        clients: a list of clients are valid for tethered WiFi.
        clients_tethering: a list of clients using tethered WiFi.
        nw_gen: network generation.
        ap_band: setup WiFi tethering on 2G or 5G.
            This is optional, default value is WIFI_CONFIG_APBAND_2G
        check_interval: delay time between each around of Internet connection check.
            This is optional, default value is 30 (seconds).
        check_iteration: check Internet connection for how many times in total.
            This is optional, default value is 4 (4 times).
        do_cleanup: after WiFi tethering test, do clean up to tear down tethering
            setup or not. This is optional, default value is True.
        ssid: use this string as WiFi SSID to setup tethered WiFi network.
            This is optional. Default value is None.
            If it's None, a random string will be generated.
        password: use this string as WiFi password to setup tethered WiFi network.
            This is optional. Default value is None.
            If it's None, a random string will be generated.
        pre_teardown_func: execute custom actions between tethering setup adn teardown.

    """
    if not test_setup_tethering(log, provider, clients, nw_gen):
        log.error("Verify %s Internet access failed.", nw_gen)
        return False

    if pre_teardown_func:
        if not pre_teardown_func():
            return False

    return wifi_tethering_setup_teardown(
        log,
        provider,
        clients_tethering,
        ap_band=ap_band,
        check_interval=check_interval,
        check_iteration=check_iteration,
        do_cleanup=do_cleanup,
        ssid=ssid,
        password=password)


def run_stress_test(log,
                    stress_test_number,
                    precondition_func=None,
                    test_case_func=None):
    """Run stress test of a test case.

    Args:
        log: log object.
        stress_test_number: The number of times the test case is run.
        precondition_func: A function performing set up before running test case
        test_case_func: A test case function.

    Returns:
        True stress pass rate is higher than MINIMUM_SUCCESS_RATE.
        False otherwise.

    """
    MINIMUM_SUCCESS_RATE = .95
    success_count = 0
    fail_count = 0
    for i in range(1, stress_test_number + 1):
        if precondition_func:
            precondition_func()
        if test_case_func:
            result = test_case_func()
        if result:
            success_count += 1
            result_str = "Succeeded"
        else:
            fail_count += 1
            result_str = "Failed"
        log.info("Iteration {} {}. Current: {} / {} passed.".format(
            i, result_str, success_count, stress_test_number))

    log.info("Final Count - Success: {}, Failure: {} - {}%".format(
        success_count, fail_count,
        str(100 * success_count / (success_count + fail_count))))
    if success_count / (
            success_count + fail_count) >= MINIMUM_SUCCESS_RATE:
        return True
    else:
        return False


def test_start_wifi_tethering_connect_teardown(log,
                                               ad_host,
                                               ad_client,
                                               ssid,
                                               password):
    """Private test util for WiFi Tethering.

    1. Host start WiFi tethering.
    2. Client connect to tethered WiFi.
    3. Host tear down WiFi tethering.

    Args:
        log: log object.
        ad_host: android device object for host
        ad_client: android device object for client
        ssid: WiFi tethering ssid
        password: WiFi tethering password

    Returns:
        True if pass, otherwise False.
    """
    result = True
    # Turn off active SoftAP if any.
    if ad_host.droid.wifiIsApEnabled():
        stop_wifi_tethering(log, ad_host)

    time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
    if not start_wifi_tethering(log, ad_host, ssid, password,
                                WIFI_CONFIG_APBAND_2G):
        log.error("Start WiFi tethering failed.")
        result = False
    time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
    if not ensure_wifi_connected(log, ad_client, ssid, password):
        log.error("Client connect to WiFi failed.")
        result = False
    if not wifi_reset(log, ad_client):
        log.error("Reset client WiFi failed. {}".format(
            ad_client.serial))
        result = False
    if not stop_wifi_tethering(log, ad_host):
        log.error("Stop WiFi tethering failed.")
        result = False
    return result


def verify_wifi_tethering_when_reboot(log,
                                      provider,
                                      post_reboot_func=None):
    """ Verify wifi tethering when reboot.

    Verify wifi tethering is enabled before reboot and disabled
    after reboot, and execute additional custom actions verification
    of wifi tethering.

    Args:
        log: log object.
        provider: android object provide WiFi tethering.
        post_reboot_func: execute custom actions after reboot.
    Returns:
        True if pass, otherwise False.

    """
    if not provider.droid.wifiIsApEnabled():
        log.error("Provider WiFi tethering stopped.")
        return False

    provider.log.info("Reboot provider")
    provider.reboot()
    time.sleep(
        WAIT_TIME_AFTER_REBOOT + WAIT_TIME_TETHERING_AFTER_REBOOT)

    log.info("After reboot check if tethering stopped.")
    if provider.droid.wifiIsApEnabled():
        log.error("Provider WiFi tethering did NOT stopped.")
        return False
    if post_reboot_func:
        post_reboot_func()
    return True


def setup_device_internet_connection(log,
                                     device,
                                     network_ssid,
                                     network_password):
    """Setup wifi network for device and verify internet connection.
    Args:
        log: log object.
        device: android device object.
        network_ssid: wifi network ssid.
        network_password: wifi network password.
    Returns:
        True if pass, otherwise False.

    """
    log.info("Make sure DUT can connect to live network by WIFI")
    if ((not ensure_wifi_connected(log,
                                   device,
                                   network_ssid,
                                   network_password))
            or (not verify_internet_connection(log, device))):
        log.error("WiFi connect fail.")
        return False
    return True


def wait_and_verify_device_internet_connection(log,
                                               device):
    """Wait for device connecting to wifi network and verify internet connection.
    Args:
        log: log object.
        device: Android device object.
    Returns:
        True if pass, otherwise False.

    """
    log.info("Make sure WiFi can connect automatically.")
    if (not wait_for_wifi_data_connection(log, device, True) or
            not verify_internet_connection(log, device)):
        log.error("Data did not return to WiFi")
        return False
    return True


def setup_device_internet_connection_then_reboot(log,
                                                 device,
                                                 network_ssid,
                                                 network_password):
    """Setup wifi network for device and verify internet connection, then reboot.
    Args:
        log: log object.
        device: android device object.
        network_ssid: wifi network ssid.
        network_password: wifi network password.
    Returns:
        True if pass, otherwise False.

    """
    if not setup_device_internet_connection(log,
                                            device,
                                            network_ssid,
                                            network_password):
        return False

    device.log.info("Reboot!")
    device.reboot()
    time.sleep(WAIT_TIME_AFTER_REBOOT)
    time.sleep(WAIT_TIME_TETHERING_AFTER_REBOOT)
    return True


def verify_internet_connection_in_doze_mode(log,
                                            provider,
                                            client):
    """Set provider in doze mode and verify client's internet connection.
    Args:
        log: log object.
        provider: android device object for provider.
        client: android device object for client.
    Returns:
        True if pass, otherwise False.

    """
    try:
        if not provider.droid.wifiIsApEnabled():
            provider.log.error("Provider WiFi tethering stopped.")
            return False
        provider.log.info("Turn off screen on provider")
        provider.droid.goToSleepNow()
        time.sleep(60)
        if not verify_internet_connection(log, client):
            client.log.error("Client have no Internet access.")
            return False

        provider.log.info("Enable doze mode on provider")
        if not enable_doze(provider):
            provider.log.error("Failed to enable doze mode.")
            return False
        time.sleep(60)
        if not verify_internet_connection(log, client):
            client.log.error("Client have no Internet access.")
            return False
    finally:
        log.info("Disable doze mode.")
        if not disable_doze(provider):
            log.error("Failed to disable doze mode.")
            return False
    return True


def test_wifi_cell_switching_in_call(log,
                                     ads,
                                     network_ssid,
                                     network_password,
                                     new_gen=None):
    """Test data connection network switching during voice call when phone on <nw_gen>
    Args:
        log: log object.
        ads: android device objects.
        network_ssid: wifi network ssid.
        network_password: wifi network password.
        new_gen: network generation.
    Returns:
        True if pass, otherwise False.

    """
    result = True
    if not call_setup_teardown(log, ads[0], ads[1], None, None, None,
                               5):
        log.error("Call setup failed")
        return False
    else:
        log.info("Call setup succeed")

    if not wifi_cell_switching(log,
                               ads[0],
                               new_gen,
                               network_ssid,
                               network_password):
        ads[0].log.error("Failed to do WIFI and Cell switch in call")
        result = False

    if not is_phone_in_call_active(ads[0]):
        return False
    else:
        if not ads[0].droid.telecomCallGetAudioState():
            ads[0].log.error("Audio is not on call")
            result = False
        else:
            ads[0].log.info("Audio is on call")
        hangup_call(log, ads[0])
        return result


def verify_toggle_data_during_wifi_tethering(log,
                                             provider,
                                             clients,
                                             new_gen=None):
    """Verify toggle Data network during WiFi Tethering.
    Args:
        log: log object.
        provider: android device object for provider.
        clients: android device objects for clients.
        new_gen: network generation.
    Returns:
        True if pass, otherwise False.

    """
    try:
        ssid = rand_ascii_str(10)
        if not test_wifi_tethering(log,
                                   provider,
                                   clients,
                                   [clients[0]],
                                   new_gen,
                                   WIFI_CONFIG_APBAND_2G,
                                   check_interval=10,
                                   check_iteration=2,
                                   do_cleanup=False,
                                   ssid=ssid):
            log.error("WiFi Tethering failed.")
            return False
        if not provider.droid.wifiIsApEnabled():
            provider.log.error("Provider WiFi tethering stopped.")
            return False

        provider.log.info(
            "Disable Data on Provider, verify no data on Client.")
        provider.droid.telephonyToggleDataConnection(False)
        time.sleep(WAIT_TIME_DATA_STATUS_CHANGE_DURING_WIFI_TETHERING)
        if not verify_internet_connection(log, provider, expected_state=False):
            provider.log.error("Disable data on provider failed.")
            return False
        if not provider.droid.wifiIsApEnabled():
            provider.log.error("Provider WiFi tethering stopped.")
            return False
        if not check_is_wifi_connected(log, clients[0], ssid):
            clients[0].log.error("Client WiFi is not connected")
            return False

        log.info(
            "Enable Data on Provider, verify data available on Client.")
        provider.droid.telephonyToggleDataConnection(True)
        if not wait_for_cell_data_connection(log, provider,
                                             True):
            provider.log.error(
                "Provider failed to enable data connection.")
            return False
        if not verify_internet_connection(log, provider):
            provider.log.error(
                "Provider internet connection check failed.")
            return False
        if not provider.droid.wifiIsApEnabled():
            provider.log.error("Provider WiFi tethering stopped.")
            return False

        if not check_is_wifi_connected(log, clients[0], ssid) or (not verify_internet_connection(log, clients[0])):
            clients[0].log.error("Client wifi connection check failed!")
            return False

    finally:
        if not wifi_tethering_cleanup(log,
                                      provider,
                                      clients):
            return False
    return True
