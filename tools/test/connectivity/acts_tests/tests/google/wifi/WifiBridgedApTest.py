#!/usr/bin/env python3
#
#   Copyright 2021 - The Android Open Source Project
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


import logging
import time
from acts import asserts
from acts import signals
from acts.test_decorators import test_tracker_info
import acts_contrib.test_utils.wifi.wifi_test_utils as wutils
from acts_contrib.test_utils.wifi import wifi_constants
from acts_contrib.test_utils.wifi.WifiBaseTest import WifiBaseTest
from acts.controllers.ap_lib.hostapd_constants import BAND_2G
from acts.controllers.ap_lib.hostapd_constants import BAND_5G


WifiEnums = wutils.WifiEnums
BRIDGED_AP_LAUNCH_INTERVAL_5_SECONDS = 5
BRIDGED_AP_SHUTDOWN_INTERVAL_5_MINUTES = 300
BRIDGED_AP_SHUTDOWN_INTERVAL_5_SECONDS = 5
SOFT_AP_SHUTDOWN_INTERVAL_10_MINUTES = 600
INTERVAL_9_MINUTES = 540
INTERVAL_2_MINUTES = 120
INTERVAL_1_MINUTES = 60


class WifiBridgedApTest(WifiBaseTest):
    """WiFi BridgedAp test class.

    Test Bed Requirement:
        * Android device x 1 with BridgedAp supported.
        * Android devices x2 as clients, at least Android 10.
        * OpenWrt AP x 1.
    """

    def setup_class(self):
        super().setup_class()

        if len(self.android_devices) == 3:
            self.dut = self.android_devices[0]
            self.client1 = self.android_devices[1]
            self.client2 = self.android_devices[2]
        else:
            raise signals.TestAbortClass("WifiBridgedApTest requires 3 DUTs")

        if not self.dut.droid.wifiIsBridgedApConcurrencySupported():
            raise signals.TestAbortClass("Legacy phone is not supported")

        for ad in self.android_devices:
            wutils.wifi_test_device_init(ad)

        req_params = ["dbs_supported_models"]
        opt_param = ["cnss_diag_file", "pixel_models"]

        self.unpack_userparams(
            req_param_names=req_params, opt_param_names=opt_param)

    def setup_test(self):
        super().setup_test()
        for ad in self.android_devices:
            wutils.reset_wifi(ad)
        wutils.wifi_toggle_state(self.dut, False)
        wutils.wifi_toggle_state(self.client1, True)
        wutils.wifi_toggle_state(self.client2, True)

    def teardown_test(self):
        super().teardown_test()
        if self.dut.droid.wifiIsApEnabled():
            wutils.stop_wifi_tethering(self.dut)
        for ad in self.android_devices:
            wutils.reset_wifi(ad)
            wutils.set_wifi_country_code(
                ad, wutils.WifiEnums.CountryCode.US)

    def teardown_class(self):
        super().teardown_class()
        for ad in self.android_devices:
            wutils.reset_wifi(ad)
        if "AccessPoint" in self.user_params:
            del self.user_params["reference_networks"]
            del self.user_params["open_network"]

    def set_country_code_and_verify(self, ad, country_code):
        wutils.set_wifi_country_code(ad, country_code)
        # Wi-Fi ON and OFF to make sure country code take effect.
        wutils.wifi_toggle_state(ad, True)
        wutils.wifi_toggle_state(ad, False)

        country = ad.droid.wifiGetCountryCode()
        asserts.assert_true(country == country_code,
                            "country code {} is not set".format(country_code))
        ad.log.info("code code set to : {}".format(country))

    def verify_clients_support_wpa3_sae(self, *args):
        """Check if clients support WPA3 SAE.

        Args:
            args: arbitrary number of clients. e.g., self.dut1, self.dut2, ...
        """
        duts = args
        for dut in duts:
            asserts.skip_if(not dut.droid.wifiIsWpa3SaeSupported(),
                            "All DUTs support WPA3 SAE is required")

    def verify_band_of_bridged_ap_instance(self, ad, infos, bands):
        """Check bands enabled as expected.
           This function won't be called directly.

        Args:
            infos: SoftAp info list.
            bands: A list of bands.
                   e,g,. [WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                          WifiEnums.WIFI_CONFIG_SOFTAP_BAND_5G]
        """
        asserts.assert_true(len(infos) == len(bands),
                            "length of infos and bands not matched")
        if len(bands) == 1 and (bands[0] == WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G):
            asserts.assert_true(infos[0][wifi_constants.
                                SOFTAP_INFO_FREQUENCY_CALLBACK_KEY]
                                in WifiEnums.softap_band_frequencies[bands[0]],
                                "This should be a %s instance", bands[0])
        if len(bands) == 2 and (bands[0] == WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G):
            asserts.assert_true((infos[0][wifi_constants.
                                SOFTAP_INFO_FREQUENCY_CALLBACK_KEY]
                                in WifiEnums.ALL_2G_FREQUENCIES and
                                infos[1][wifi_constants.
                                SOFTAP_INFO_FREQUENCY_CALLBACK_KEY]
                                in WifiEnums.ALL_5G_FREQUENCIES) or
                                (infos[0][wifi_constants.
                                 SOFTAP_INFO_FREQUENCY_CALLBACK_KEY]
                                 in WifiEnums.ALL_5G_FREQUENCIES and
                                 infos[1][wifi_constants.
                                 SOFTAP_INFO_FREQUENCY_CALLBACK_KEY]
                                 in WifiEnums.ALL_2G_FREQUENCIES),
                                "There should only be 2G and 5G instances")

    def verify_softap_freq_equals_to_ap_freq(self, ad, infos):
        """Verify if instance frequency equal to AP frequency.
           This function won't be called directly.
        Args:
            infos: SoftAp info list.
        """
        wlan0_freq = wutils.get_wlan0_link(ad)["freq"]
        softap_freqs = []
        for i in range(len(infos)):
            softap_freqs.append(infos[i][WifiEnums.frequency_key])
        ad.log.info("softap_freqs : {}".format(softap_freqs))
        asserts.assert_true(int(wlan0_freq) in softap_freqs,
                            "AP freq != SoftAp freq")
        ad.log.info("softap freq == AP freq")

    def verify_number_band_freq_of_bridged_ap(self, ad, bands,
                                              freq_equal=False):
        """Verify every aspect of info list from BridgedAp.

        Args:
            bands: A list of bands,
                   e,g,. [WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                          WifiEnums.WIFI_CONFIG_SOFTAP_BAND_5G]
            freq_equal: True if need to check SoftAp freq equals to STA freq.
        """
        callbackId = ad.droid.registerSoftApCallback()
        infos = wutils.get_current_softap_infos(ad, callbackId, True)
        self.verify_band_of_bridged_ap_instance(ad, infos, bands)
        if freq_equal:
            self.verify_softap_freq_equals_to_ap_freq(ad, infos)
        ad.droid.unregisterSoftApCallback(callbackId)

    def verify_expected_number_of_softap_clients(self, ad, number):
        """Verify the number of softap clients.

        Args:
            number: expect number of client connect to SoftAp.
        """
        callbackId = self.dut.droid.registerSoftApCallback()
        wutils.wait_for_expected_number_of_softap_clients(self.dut,
                                                          callbackId, number)
        ad.log.info("{} clients connect to soft ap".format(number))
        self.dut.droid.unregisterSoftApCallback(callbackId)

    def wait_interval(self, interval):
        """print different messages with different intervals.

        Args:
            interval: different interval for different situation.
        """
        if interval == BRIDGED_AP_LAUNCH_INTERVAL_5_SECONDS:
            logging.info("Wait {} seconds for BridgedAp launch"
                         .format(interval))
            time.sleep(interval)
        elif interval == BRIDGED_AP_SHUTDOWN_INTERVAL_5_MINUTES:
            logging.info("Wait {} minutes for BridgedAp shutdown"
                         .format(interval/60))
            time.sleep(interval)
        elif interval == SOFT_AP_SHUTDOWN_INTERVAL_10_MINUTES:
            logging.info("Wait {} minutes for SoftAp shutdown"
                         .format(interval/60))
            time.sleep(interval)
        elif interval == INTERVAL_9_MINUTES:
            logging.info("Wait {} minutes".format(interval/60))
            time.sleep(interval)
        elif interval == INTERVAL_2_MINUTES:
            logging.info("Wait {} minutes".format(interval/60))
            time.sleep(interval)
        elif interval == INTERVAL_1_MINUTES:
            logging.info("Wait {} minutes".format(interval/60))
            time.sleep(interval)

    def two_clients_connect_to_wifi_network(self, dut1, dut2, config):
        """Connect two clients to different BridgedAp instances.
           This function will be called only when BridgedAp ON.

        Args:
            config: Wi-Fi config, e.g., {"SSID": "xxx", "password": "xxx"}
        Steps:
            Backup config.
            Register SoftAp Callback.
            Get SoftAp Infos.
            Get BSSIDs from Infos.
            Connect two clients to different BridgedAp instances.
            Restore config.
        """
        # Make sure 2 instances enabled, and get BSSIDs from BridgedAp Infos.
        callbackId = self.dut.droid.registerSoftApCallback()
        infos = wutils.get_current_softap_infos(self.dut, callbackId, True)
        self.dut.droid.unregisterSoftApCallback(callbackId)

        if len(infos) == 0:
            raise signals.TestFailure("No BridgedAp instance")
        elif len(infos) == 1:
            raise signals.TestFailure(
                "Only one BridgedAp instance, should be two")
        else:
            bssid_5g = infos[0][wifi_constants.SOFTAP_INFO_BSSID_CALLBACK_KEY]
            bssid_2g = infos[1][wifi_constants.SOFTAP_INFO_BSSID_CALLBACK_KEY]

        # Two configs for BridgedAp 2G and 5G instances.
        config_5g = config.copy()
        config_2g = config.copy()
        config_5g[WifiEnums.BSSID_KEY] = bssid_5g
        config_2g[WifiEnums.BSSID_KEY] = bssid_2g

        # Connect two clients to BridgedAp.
        wutils.connect_to_wifi_network(dut1, config_5g,
                                       check_connectivity=False)
        wutils.connect_to_wifi_network(dut2, config_2g,
                                       check_connectivity=False)

        # Verify if Clients connect to the expected BridgedAp instances.
        client1_bssid = wutils.get_wlan0_link(
            self.client1)[wifi_constants.SOFTAP_INFO_BSSID_CALLBACK_KEY]
        client2_bssid = wutils.get_wlan0_link(
            self.client2)[wifi_constants.SOFTAP_INFO_BSSID_CALLBACK_KEY]
        asserts.assert_true(client1_bssid == bssid_5g,
                            "Client1 does not connect to the 5G instance")
        asserts.assert_true(client2_bssid == bssid_2g,
                            "Client2 does not connect to the 2G instance")

    # Tests

    @test_tracker_info(uuid="6f776b4a-b080-4b52-a330-52aa641b18f2")
    def test_two_clients_ping_on_bridged_ap_band_2_and_5_with_wpa3_in_country_us(self):
        """Test clients on different instances can ping each other.

        Steps:
            Backup config.
            Make sure clients support WPA3 SAE.
            Make sure DUT is able to enable BridgedAp.
            Enable BridgedAp with bridged configuration.
            RegisterSoftApCallback.
            Check the bridged AP enabled succeed.
            Force client#1 connect to 5G.
            Force client#2 connect to 2.4G.
            Trigger client#1 and client#2 each other.
            Restore config.
        """
        # Backup config
        original_softap_config = self.dut.droid.wifiGetApConfiguration()

        # Make sure clients support WPA3 SAE.
        self.verify_clients_support_wpa3_sae(self.client1, self.client2)
        # Make sure DUT is able to enable BridgedAp.
        is_supported = wutils.check_available_channels_in_bands_2_5(
            self.dut, wutils.WifiEnums.CountryCode.US)
        asserts.skip_if(not is_supported, "BridgedAp is not supported in {}"
                        .format(wutils.WifiEnums.CountryCode.US))

        # Enable BridgedAp
        config = wutils.create_softap_config()
        config[WifiEnums.SECURITY] = WifiEnums.SoftApSecurityType.WPA3_SAE
        wutils.save_wifi_soft_ap_config(
            self.dut, config,
            bands=[WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                   WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G_5G])
        wutils.start_wifi_tethering_saved_config(self.dut)
        # Wait 5 seconds for BridgedAp launch.
        self.wait_interval(BRIDGED_AP_LAUNCH_INTERVAL_5_SECONDS)

        self.two_clients_connect_to_wifi_network(self.client1, self.client2,
                                                 config)
        # Trigger client#1 and client#2 ping each other.
        wutils.validate_ping_between_two_clients(self.client1, self.client2)

        # Restore config
        wutils.save_wifi_soft_ap_config(self.dut, original_softap_config)

    @test_tracker_info(uuid="0325ee58-ed8e-489e-9dee-55740406f896")
    def test_bridged_ap_5g_2g_and_sta_5g_dfs(self):
        """Test if auto fallback to one single 2G AP mode when
         BridgedAP enabled and STA connect to a 5G DFS channel.

        Steps:
            Backup config.
            DUT enable BridgedAp.
            DUT connect to a 5G DFS channel Wi-Fi network.
            Verify 5G instance is shutdown.
            Restore config.
        """
        # Backup config
        original_softap_config = self.dut.droid.wifiGetApConfiguration()

        # Make sure DUT is able to enable BridgedAp.
        is_supported = wutils.check_available_channels_in_bands_2_5(
            self.dut, wutils.WifiEnums.CountryCode.US)
        asserts.skip_if(not is_supported, "BridgedAp is not supported in {}"
                        .format(wutils.WifiEnums.CountryCode.US))

        # Enable BridgedAp
        config = wutils.create_softap_config()
        config[WifiEnums.SECURITY] = WifiEnums.SoftApSecurityType.WPA3_SAE
        wutils.save_wifi_soft_ap_config(
            self.dut, config,
            bands=[WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                   WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G_5G])
        wutils.start_wifi_tethering_saved_config(self.dut)
        # Wait 5 seconds for BridgedAp launch.
        self.wait_interval(BRIDGED_AP_LAUNCH_INTERVAL_5_SECONDS)
        self.verify_number_band_freq_of_bridged_ap(
            self.dut, [WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                       WifiEnums.WIFI_CONFIG_SOFTAP_BAND_5G], False)

        # STA connect to a 5G DFS Channel.
        wutils.wifi_toggle_state(self.dut, True)
        if "OpenWrtAP" in self.user_params:
            self.configure_openwrt_ap_and_start(wpa_network=True,
                                                channel_2g=6,
                                                channel_5g=132)
            wutils.connect_to_wifi_network(self.dut,
                                           self.wpa_networks[0][BAND_5G])

        self.verify_number_band_freq_of_bridged_ap(
            self.dut, [WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G], False)

        # Restore config
        wutils.save_wifi_soft_ap_config(self.dut, original_softap_config)

    @test_tracker_info(uuid="9a5d4ca9-67fc-412c-8114-01c43c34a76d")
    def test_bridged_ap_5g_2g_and_sta_5g_non_dfs(self):
        """Test 5G scc when BridgedAp enabled and 5G STA.

        Steps:
            Backup config.
            DUT enable BridgedAp.
            DUT connect to a 5G non-DFS channel Wi-Fi network.
            Verify STA frequency equals to 5G BridgedAp frequency.
            Restore config.
        """
        # Backup config
        original_softap_config = self.dut.droid.wifiGetApConfiguration()

        # Make sure DUT is able to enable BridgedAp.
        is_supported = wutils.check_available_channels_in_bands_2_5(
            self.dut, wutils.WifiEnums.CountryCode.US)
        asserts.skip_if(not is_supported, "BridgedAp is not supported in {}"
                        .format(wutils.WifiEnums.CountryCode.US))

        # Enable BridgedAp
        config = wutils.create_softap_config()
        config[WifiEnums.SECURITY] = WifiEnums.SoftApSecurityType.WPA3_SAE
        wutils.save_wifi_soft_ap_config(
            self.dut, config,
            bands=[WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                   WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G_5G])
        wutils.start_wifi_tethering_saved_config(self.dut)
        # Wait 5 seconds for BridgedAp launch.
        self.wait_interval(BRIDGED_AP_LAUNCH_INTERVAL_5_SECONDS)

        # STA connect to a 5G Non-DFS Channel.
        wutils.wifi_toggle_state(self.dut, True)
        if "OpenWrtAP" in self.user_params:
            self.configure_openwrt_ap_and_start(wpa_network=True,
                                                channel_2g=6,
                                                channel_5g=36)
            wutils.connect_to_wifi_network(self.dut,
                                           self.wpa_networks[0][BAND_5G])

        self.verify_number_band_freq_of_bridged_ap(
            self.dut, [WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                       WifiEnums.WIFI_CONFIG_SOFTAP_BAND_5G], True)

        # Restore config
        wutils.save_wifi_soft_ap_config(self.dut, original_softap_config)

    @test_tracker_info(uuid="e26e8a72-3e21-47b3-8fd8-4c4db5fb2573")
    def test_bridged_ap_5g_2g_and_sta_2g(self):
        """ Test 2G SCC when BridgedAp enable and 2G STA.

        Steps:
            Backup config.
            DUT enable BridgedAp.
            STA connect to a 2G Wi-Fi network.
            Verify STA frequency equals to 2G BridgedAp frequency.
            Restore config.
        """
        # Backup config
        original_softap_config = self.dut.droid.wifiGetApConfiguration()

        # Make sure DUT is able to enable BridgedAp.
        is_supported = wutils.check_available_channels_in_bands_2_5(
            self.dut, wutils.WifiEnums.CountryCode.US)
        asserts.skip_if(not is_supported, "BridgedAp is not supported in {}"
                        .format(wutils.WifiEnums.CountryCode.US))

        # Enable BridgedAp
        config = wutils.create_softap_config()
        config[WifiEnums.SECURITY] = WifiEnums.SoftApSecurityType.WPA3_SAE
        wutils.save_wifi_soft_ap_config(
            self.dut, config,
            bands=[WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                   WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G_5G])
        wutils.start_wifi_tethering_saved_config(self.dut)
        # Wait 5 seconds for BridgedAp launch.
        self.wait_interval(BRIDGED_AP_LAUNCH_INTERVAL_5_SECONDS)

        # STA connect to a 2G Channel.
        wutils.wifi_toggle_state(self.dut, True)
        if "OpenWrtAP" in self.user_params:
            self.configure_openwrt_ap_and_start(wpa_network=True,
                                                channel_2g=6,
                                                channel_5g=36)
            wutils.connect_to_wifi_network(self.dut,
                                           self.wpa_networks[0][BAND_2G])

        self.verify_number_band_freq_of_bridged_ap(
            self.dut, [WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                       WifiEnums.WIFI_CONFIG_SOFTAP_BAND_5G], True)

        # Restore config
        wutils.save_wifi_soft_ap_config(self.dut, original_softap_config)

    @test_tracker_info(uuid="b25f710a-2e53-456e-8280-dcdd37badd6c")
    def test_sta_5g_dfs_and_bridged_ap_5g_2g(self):
        """Test auto fallback to Single AP mode
           when STA connect to a DFS channel.

        Steps:
            Backup config.
            DUT connect to a 5G DFS channel Wi-Fi network.
            DUT enable BridgedAp.
            Verify 5G instance is shutdown and only 2G instance enabled.
            Restore config.
        """
        # Backup config
        original_softap_config = self.dut.droid.wifiGetApConfiguration()

        # Make sure DUT is able to enable BridgedAp.
        is_supported = wutils.check_available_channels_in_bands_2_5(
            self.dut, wutils.WifiEnums.CountryCode.US)
        asserts.skip_if(not is_supported, "BridgedAp is not supported in {}"
                        .format(wutils.WifiEnums.CountryCode.US))

        # STA connect to a 5G DFS Channel.
        wutils.wifi_toggle_state(self.dut, True)
        if "OpenWrtAP" in self.user_params:
            self.configure_openwrt_ap_and_start(wpa_network=True,
                                                channel_2g=6,
                                                channel_5g=132)
            wutils.connect_to_wifi_network(self.dut,
                                           self.wpa_networks[0][BAND_5G])

        # Enable BridgedAp
        config = wutils.create_softap_config()
        config[WifiEnums.SECURITY] = WifiEnums.SoftApSecurityType.WPA3_SAE
        wutils.save_wifi_soft_ap_config(
            self.dut, config,
            bands=[WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                   WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G_5G])
        wutils.start_wifi_tethering_saved_config(self.dut)
        # Wait 5 seconds for BridgedAp launch.
        self.wait_interval(BRIDGED_AP_LAUNCH_INTERVAL_5_SECONDS)

        # # Verify only 2G instance enabled.
        self.verify_number_band_freq_of_bridged_ap(
            self.dut, [WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G], False)

        # Restore config
        wutils.save_wifi_soft_ap_config(self.dut, original_softap_config)

    @test_tracker_info(uuid="6acaed3f-616f-4a03-a329-702e6cc537bd")
    def test_sta_5g_non_dfs_and_bridged_ap_5g_2g(self):
        """Test 5G scc when 5G STA and BridgedAp enabled.

        Steps:
            Backup config.
            DUT connect to a 5G non-DFS channel Wi-Fi network.
            DUT enable BridgedAp.
            Verify STA frequency equals to 5G BridgedAp frequency.
            Restore config.
        """
        # Backup config
        original_softap_config = self.dut.droid.wifiGetApConfiguration()

        # Make sure DUT is able to enable BridgedAp.
        is_supported = wutils.check_available_channels_in_bands_2_5(
            self.dut, wutils.WifiEnums.CountryCode.US)
        asserts.skip_if(not is_supported, "BridgedAp is not supported in {}"
                        .format(wutils.WifiEnums.CountryCode.US))

        # STA connect to a 5G non-DFS Channel.
        wutils.wifi_toggle_state(self.dut, True)
        if "OpenWrtAP" in self.user_params:
            self.configure_openwrt_ap_and_start(wpa_network=True,
                                                channel_2g=6,
                                                channel_5g=36)
            wutils.connect_to_wifi_network(self.dut,
                                           self.wpa_networks[0][BAND_5G])

        # Enable BridgedAp
        config = wutils.create_softap_config()
        config[WifiEnums.SECURITY] = WifiEnums.SoftApSecurityType.WPA3_SAE
        wutils.save_wifi_soft_ap_config(
            self.dut, config,
            bands=[WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                   WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G_5G])
        wutils.start_wifi_tethering_saved_config(self.dut)
        # Wait 5 seconds for BridgedAp launch.
        self.wait_interval(BRIDGED_AP_LAUNCH_INTERVAL_5_SECONDS)

        # Verify STA frequency equals to 5G BridgedAp frequency.
        self.verify_number_band_freq_of_bridged_ap(
            self.dut, [WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                       WifiEnums.WIFI_CONFIG_SOFTAP_BAND_5G], True)

        # Restore config
        wutils.save_wifi_soft_ap_config(self.dut, original_softap_config)

    @test_tracker_info(uuid="e24e7f2d-f1d5-4dc7-aa6a-487677864d1d")
    def test_sta_2g_and_bridged_ap_5g_2g(self):
        """ Test 2G SCC when 2G STA and BridgedAp enable.

        Steps:
            Backup config.
            DUT connect to a 2G Wi-Fi network.
            DUT enable BridgedAp.
            Verify STA frequency equals to 2G BridgedAp frequency.
            Restore config.
        """
        # Backup config
        original_softap_config = self.dut.droid.wifiGetApConfiguration()

        # Make sure DUT is able to enable BridgedAp.
        is_supported = wutils.check_available_channels_in_bands_2_5(
            self.dut, wutils.WifiEnums.CountryCode.US)
        asserts.skip_if(not is_supported, "BridgedAp is not supported in {}"
                        .format(wutils.WifiEnums.CountryCode.US))

        # STA connect to a 2G Channel.
        wutils.wifi_toggle_state(self.dut, True)
        if "OpenWrtAP" in self.user_params:
            self.configure_openwrt_ap_and_start(wpa_network=True,
                                                channel_2g=6,
                                                channel_5g=36)
            wutils.connect_to_wifi_network(self.dut,
                                           self.wpa_networks[0][BAND_2G])
        # Enable BridgedAp
        config = wutils.create_softap_config()
        config[WifiEnums.SECURITY] = WifiEnums.SoftApSecurityType.WPA3_SAE
        wutils.save_wifi_soft_ap_config(
            self.dut, config,
            bands=[WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                   WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G_5G])
        wutils.start_wifi_tethering_saved_config(self.dut)
        # Wait 5 seconds for BridgedAp launch.
        self.wait_interval(BRIDGED_AP_LAUNCH_INTERVAL_5_SECONDS)

        self.verify_number_band_freq_of_bridged_ap(
            self.dut, [WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                       WifiEnums.WIFI_CONFIG_SOFTAP_BAND_5G], True)

        # Restore config
        wutils.save_wifi_soft_ap_config(self.dut, original_softap_config)

    @test_tracker_info(uuid="927d564d-2ac4-4e6f-bb36-270efd519e0b")
    def test_bridged_ap_5g_2g_shutdown_5g_2g_no_client(self):
        """Test the BridgeAp shutdown mechanism with no client connect to it.

        Steps:
            Backup config.
            DUT turns ON BridgedAp.
            Verify no client connect to the BridgedAp.
            Wait for 5 minutes.
            Verify that 5G BridgedAp instance is shutdown.
            Maks sure there is still no client connect to the BridgedAp.
            Wait for 5 minutes.
            Verify that 2G BridgedAp instance is shutdown.
            Restore config.
        """
        # Backup config
        original_softap_config = self.dut.droid.wifiGetApConfiguration()

        # Make sure DUT is able to enable BridgedAp.
        is_supported = wutils.check_available_channels_in_bands_2_5(
            self.dut, wutils.WifiEnums.CountryCode.US)
        asserts.skip_if(not is_supported, "BridgedAp is not supported in {}"
                        .format(wutils.WifiEnums.CountryCode.US))

        # Enable BridgedAp
        config = wutils.create_softap_config()
        config[WifiEnums.SECURITY] = WifiEnums.SoftApSecurityType.WPA3_SAE
        wutils.save_wifi_soft_ap_config(
            self.dut, config,
            bands=[WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                   WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G_5G])
        wutils.start_wifi_tethering_saved_config(self.dut)
        # Wait 5 seconds for BridgedAp launch.
        self.wait_interval(BRIDGED_AP_LAUNCH_INTERVAL_5_SECONDS)

        # No client connection, wait 5 minutes, verify 5G is shutdown.
        self.verify_expected_number_of_softap_clients(self.dut, 0)
        self.wait_interval(BRIDGED_AP_SHUTDOWN_INTERVAL_5_MINUTES)
        self.verify_number_band_freq_of_bridged_ap(
            self.dut, [WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G], False)
        # No client connection, wait 5 minutes, verify 2G is shutdown.
        self.verify_expected_number_of_softap_clients(self.dut, 0)
        self.wait_interval(BRIDGED_AP_SHUTDOWN_INTERVAL_5_MINUTES)

        self.verify_number_band_freq_of_bridged_ap(self.dut, [], False)

        # Restore config
        wutils.save_wifi_soft_ap_config(self.dut, original_softap_config)

    @test_tracker_info(uuid="4c5b210c-412f-40e4-84b6-2a12dbffa017")
    def test_bridged_ap_5g_2g_shutdown_5g_auto_shutdown_2g(self):
        """Test the BridgeAp shutdown mechanism with no client connect to it.

        Steps:
            Backup config.
            DUT turns ON BridgedAp.
            Verify no client connect to the BridgedAp.
            Wait for 5 minutes.
            Verify that 5G BridgedAp instance is shutdown.
            A client connect to the 2G BridgedAp.
            The client disconnect from 2G BridgedAp.
            Wait for 10 minutes.
            Verify 2G BridgedAp is shutdown, no BridgedAp instance enabled.
            Restore config.
        """
        # Backup config
        original_softap_config = self.dut.droid.wifiGetApConfiguration()

        # Make sure DUT is able to enable BridgedAp.
        is_supported = wutils.check_available_channels_in_bands_2_5(
            self.dut, wutils.WifiEnums.CountryCode.US)
        asserts.skip_if(not is_supported, "BridgedAp is not supported in {}"
                        .format(wutils.WifiEnums.CountryCode.US))

        # Enable BridgedAp
        config = wutils.create_softap_config()
        config[WifiEnums.SECURITY] = WifiEnums.SoftApSecurityType.WPA3_SAE
        wutils.save_wifi_soft_ap_config(
            self.dut, config,
            bands=[WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                   WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G_5G])
        wutils.start_wifi_tethering_saved_config(self.dut)
        self.wait_interval(BRIDGED_AP_LAUNCH_INTERVAL_5_SECONDS)
        self.verify_number_band_freq_of_bridged_ap(
            self.dut, [WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                       WifiEnums.WIFI_CONFIG_SOFTAP_BAND_5G], False)

        # Verify no connection to BridgedAp, wait for 5 mins, 5G shutdown.
        self.verify_expected_number_of_softap_clients(self.dut, 0)
        self.wait_interval(BRIDGED_AP_SHUTDOWN_INTERVAL_5_MINUTES)
        self.verify_number_band_freq_of_bridged_ap(
            self.dut, [WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G], False)

        # A client connect to 2G BridgedAp instance.
        callbackId = self.dut.droid.registerSoftApCallback()
        infos = wutils.get_current_softap_infos(self.dut, callbackId, True)
        self.dut.droid.unregisterSoftApCallback(callbackId)
        bssid_2g = infos[0][wifi_constants.SOFTAP_INFO_BSSID_CALLBACK_KEY]
        config_2g = config.copy()
        config_2g[WifiEnums.BSSID_KEY] = bssid_2g
        wutils.connect_to_wifi_network(self.client1, config_2g,
                                       check_connectivity=False)
        self.wait_interval(BRIDGED_AP_SHUTDOWN_INTERVAL_5_SECONDS)

        # Client disconnect From 2G BridgedAp instance.
        is_disconnect = self.client1.droid.wifiDisconnect()
        self.client1.log.info("Disconnected from SoftAp")
        if not is_disconnect:
            raise signals.TestFailure("Wi-Fi is not disconnected as expect")
        wutils.reset_wifi(self.client1)

        # Verify 2G instance is still enabled.
        self.wait_interval(INTERVAL_9_MINUTES)
        self.verify_number_band_freq_of_bridged_ap(
            self.dut, [WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G], False)

        # Verify all BridgedAp instances are shutdown after 10 minutes.
        self.wait_interval(INTERVAL_1_MINUTES)
        self.verify_number_band_freq_of_bridged_ap(self.dut, [], False)

        # Restore config
        wutils.save_wifi_soft_ap_config(self.dut, original_softap_config)

    @test_tracker_info(uuid="2b1b4579-d610-4983-83f4-5fc34b3cdd6b")
    def test_bridged_ap_5g_2g_shutdown_5g_auto_shutdown_2g_after_10_mins(self):
        """Test the BridgeAp shutdown mechanism with no client connect to it.

        Steps:
            Backup config.
            DUT turns ON BridgedAp.
            Verify no client connect to the BridgedAp.
            Wait for 5 minutes.
            Verify that 5G BridgedAp instance is shutdown.
            A client connect to the 2G BridgedAp at 7th minutes.
            The client disconnect from 2G BridgedAp at 9th minutes.
            Wait for 10 minutes.
            Verify 2G BridgedAp is shutdown, no BridgedAp instance enabled.
            Restore config.
        """
        # Backup config
        original_softap_config = self.dut.droid.wifiGetApConfiguration()

        # Make sure DUT is able to enable BridgedAp.
        is_supported = wutils.check_available_channels_in_bands_2_5(
            self.dut, wutils.WifiEnums.CountryCode.US)
        asserts.skip_if(not is_supported, "BridgedAp is not supported in {}"
                        .format(wutils.WifiEnums.CountryCode.US))

        # Enable BridgedAp
        config = wutils.create_softap_config()
        config[WifiEnums.SECURITY] = WifiEnums.SoftApSecurityType.WPA3_SAE
        wutils.save_wifi_soft_ap_config(
            self.dut, config,
            bands=[WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                   WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G_5G],
            bridged_opportunistic_shutdown_enabled=True,
            shutdown_timeout_enable=True)
        wutils.start_wifi_tethering_saved_config(self.dut)
        self.wait_interval(BRIDGED_AP_LAUNCH_INTERVAL_5_SECONDS)

        self.verify_number_band_freq_of_bridged_ap(
            self.dut, [WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                       WifiEnums.WIFI_CONFIG_SOFTAP_BAND_5G], False)
        self.verify_expected_number_of_softap_clients(self.dut, 0)
        self.wait_interval(BRIDGED_AP_SHUTDOWN_INTERVAL_5_MINUTES)
        self.verify_number_band_freq_of_bridged_ap(
            self.dut, [WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G], False)

        # Client connects to 2G instance at 7th mins.
        self.wait_interval(INTERVAL_2_MINUTES)
        callbackId = self.dut.droid.registerSoftApCallback()
        infos = wutils.get_current_softap_infos(self.dut, callbackId, True)
        self.dut.droid.unregisterSoftApCallback(callbackId)
        bssid_2g = infos[0][wifi_constants.SOFTAP_INFO_BSSID_CALLBACK_KEY]
        config_2g = config.copy()
        config_2g[WifiEnums.BSSID_KEY] = bssid_2g
        wutils.connect_to_wifi_network(self.client1, config_2g,
                                       check_connectivity=False)
        self.wait_interval(BRIDGED_AP_SHUTDOWN_INTERVAL_5_SECONDS)

        # Client disconnect From 2G BridgedAp instance at 9th mins.
        self.wait_interval(INTERVAL_2_MINUTES)
        is_disconnect = self.client1.droid.wifiDisconnect()
        self.client1.log.info("Disconnected from SoftAp")
        if not is_disconnect:
            raise signals.TestFailure("Wi-Fi is not disconnected as expect")
        wutils.reset_wifi(self.client1)

        # Make sure 2G instance is still enabled.
        self.wait_interval(INTERVAL_9_MINUTES)
        self.verify_number_band_freq_of_bridged_ap(
            self.dut, [WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G], False)

        self.wait_interval(INTERVAL_1_MINUTES)
        self.verify_number_band_freq_of_bridged_ap(self.dut, [], False)

        # Restore config
        wutils.save_wifi_soft_ap_config(self.dut, original_softap_config)

    @test_tracker_info(uuid="1785a487-dd95-4c17-852d-3c9b7b7dd4c3")
    def test_bridged_ap_5g_2g_fallback_2g_country_jp(self):
        """Test BridgedAp fallback to Single AP mode with JP country code.

        Steps:
            Backup config.
            Set DUT country code to "JP".
            DUT turns ON BridgedAp.
            Verify only 2G BridgedAp instance is enabled.
            Restore config.
        """
        # Backup config
        original_softap_config = self.dut.droid.wifiGetApConfiguration()

        # Set country code to JP and enable BridgedAp
        self.set_country_code_and_verify(self.dut, WifiEnums.CountryCode.JAPAN)
        config = wutils.create_softap_config()
        config[WifiEnums.SECURITY] = WifiEnums.SoftApSecurityType.WPA3_SAE
        wutils.save_wifi_soft_ap_config(
            self.dut, config,
            bands=[WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G,
                   WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G_5G])
        wutils.start_wifi_tethering_saved_config(self.dut)
        self.wait_interval(BRIDGED_AP_LAUNCH_INTERVAL_5_SECONDS)

        # Verify only 2G BridgedAp instance enabled.
        self.verify_number_band_freq_of_bridged_ap(
            self.dut, [WifiEnums.WIFI_CONFIG_SOFTAP_BAND_2G], False)

        # Restore config
        wutils.save_wifi_soft_ap_config(self.dut, original_softap_config)
