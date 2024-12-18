#
#   Copyright 2016 - The Android Open Source Project
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

from acts.test_decorators import test_tracker_info
from acts_contrib.test_utils.wifi import wifi_test_utils as wutils
from acts_contrib.test_utils.wifi.WifiBaseTest import WifiBaseTest

WifiEnums = wutils.WifiEnums

# EAP Macros
EAP = WifiEnums.Eap
EapPhase2 = WifiEnums.EapPhase2

# Enterprise Config Macros
Ent = WifiEnums.Enterprise


class WifiEnterpriseRoamingTest(WifiBaseTest):

    def __init__(self, configs):
        super().__init__(configs)
        self.enable_packet_log = True

    def setup_class(self):
        super().setup_class()

        self.dut = self.android_devices[0]
        req_params = (
            "roaming_attn",
            # Expected time within which roaming should finish, in seconds.
            "roam_interval",
            "ca_cert",
            "client_cert",
            "client_key",
            "eap_identity",
            "eap_password",
            "device_password",
            "wifi6_models",
            "radius_conf_2g",
            "radius_conf_5g")
        self.unpack_userparams(req_params)
        if "AccessPoint" in self.user_params:
            self.legacy_configure_ap_and_start(
                mirror_ap=True,
                ent_network=True,
                ap_count=2,
                radius_conf_2g=self.radius_conf_2g,
                radius_conf_5g=self.radius_conf_5g,)
        elif "OpenWrtAP" in self.user_params:
            self.configure_openwrt_ap_and_start(
                mirror_ap=True,
                ent_network=True,
                ap_count=2,
                radius_conf_2g=self.radius_conf_2g,
                radius_conf_5g=self.radius_conf_5g,)
        self.ent_network_2g_a = self.ent_networks[0]["2g"]
        self.ent_network_2g_b = self.ent_networks[1]["2g"]
        self.ent_roaming_ssid = self.ent_network_2g_a[WifiEnums.SSID_KEY]
        if "AccessPoint" in self.user_params:
            self.bssid_a = self.ent_network_2g_a[WifiEnums.BSSID_KEY.lower()]
            self.bssid_b = self.ent_network_2g_b[WifiEnums.BSSID_KEY.lower()]
        elif "OpenWrtAP" in self.user_params:
            self.bssid_a = self.bssid_map[0]["2g"][self.ent_roaming_ssid]
            self.bssid_b = self.bssid_map[1]["2g"][self.ent_roaming_ssid]

        self.config_peap = {
            Ent.EAP: int(EAP.PEAP),
            Ent.CA_CERT: self.ca_cert,
            Ent.IDENTITY: self.eap_identity,
            Ent.PASSWORD: self.eap_password,
            Ent.PHASE2: int(EapPhase2.MSCHAPV2),
            WifiEnums.SSID_KEY: self.ent_roaming_ssid
        }
        self.config_tls = {
            Ent.EAP: int(EAP.TLS),
            Ent.CA_CERT: self.ca_cert,
            WifiEnums.SSID_KEY: self.ent_roaming_ssid,
            Ent.CLIENT_CERT: self.client_cert,
            Ent.PRIVATE_KEY_ID: self.client_key,
            Ent.IDENTITY: self.eap_identity,
        }
        self.config_ttls = {
            Ent.EAP: int(EAP.TTLS),
            Ent.CA_CERT: self.ca_cert,
            Ent.IDENTITY: self.eap_identity,
            Ent.PASSWORD: self.eap_password,
            Ent.PHASE2: int(EapPhase2.MSCHAPV2),
            WifiEnums.SSID_KEY: self.ent_roaming_ssid
        }
        self.config_sim = {
            Ent.EAP: int(EAP.SIM),
            WifiEnums.SSID_KEY: self.ent_roaming_ssid,
        }
        self.attn_a = self.attenuators[0]
        self.attn_b = self.attenuators[2]
        if "OpenWrtAP" in self.user_params:
            self.attn_b = self.attenuators[1]
        # Set screen lock password so ConfigStore is unlocked.
        self.dut.droid.setDevicePassword(self.device_password)
        wutils.set_attns(self.attenuators, "default")

    def teardown_class(self):
        wutils.reset_wifi(self.dut)
        self.dut.droid.disableDevicePassword(self.device_password)
        self.dut.ed.clear_all_events()
        wutils.set_attns(self.attenuators, "default")

    def setup_test(self):
        super().setup_test()
        self.dut.droid.wifiStartTrackingStateChange()
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()
        wutils.reset_wifi(self.dut)
        self.dut.ed.clear_all_events()

    def teardown_test(self):
        super().teardown_test()
        self.dut.droid.wakeLockRelease()
        self.dut.droid.goToSleepNow()
        self.dut.droid.wifiStopTrackingStateChange()
        wutils.set_attns(self.attenuators, "default")

    def trigger_roaming_and_validate(self, attn_val_name, expected_con):
        """Sets attenuators to trigger roaming and validate the DUT connected
        to the BSSID expected.

        Args:
            attn_val_name: Name of the attenuation value pair to use.
            expected_con: The expected info of the network to we expect the DUT
                to roam to.
        """
        wutils.set_attns_steps(
            self.attenuators, attn_val_name, self.roaming_attn)
        self.log.info("Wait %ss for roaming to finish.", self.roam_interval)
        try:
            self.dut.droid.wakeLockAcquireBright()
            self.dut.droid.wakeUpNow()
            wutils.verify_wifi_connection_info(self.dut, expected_con)
            expected_bssid = expected_con[WifiEnums.BSSID_KEY]
            self.log.info("Roamed to %s successfully", expected_bssid)
        finally:
            self.dut.droid.wifiLockRelease()
            self.dut.droid.goToSleepNow()

    def roaming_between_a_and_b_logic(self, config):
        """Test roaming between two enterprise APs.

        Steps:
        1. Make bssid_a visible, bssid_b not visible.
        2. Connect to ent_roaming_ssid. Expect DUT to connect to bssid_a.
        3. Make bssid_a not visible, bssid_b visible.
        4. Expect DUT to roam to bssid_b.
        5. Make bssid_a visible, bssid_b not visible.
        6. Expect DUT to roam back to bssid_a.
        """
        expected_con_to_a = {
            WifiEnums.SSID_KEY: self.ent_roaming_ssid,
            WifiEnums.BSSID_KEY: self.bssid_a,
        }
        expected_con_to_b = {
            WifiEnums.SSID_KEY: self.ent_roaming_ssid,
            WifiEnums.BSSID_KEY: self.bssid_b,
        }
        wutils.set_attns_steps(
            self.attenuators, "AP1_on_AP2_off", self.roaming_attn)
        wutils.connect_to_wifi_network(self.dut, config)
        wutils.verify_11ax_wifi_connection(
            self.dut, self.wifi6_models, "wifi6_ap" in self.user_params)
        wutils.verify_wifi_connection_info(self.dut, expected_con_to_a)
        self.log.info("Roaming from %s to %s", self.bssid_a, self.bssid_b)
        self.trigger_roaming_and_validate("AP1_off_AP2_on", expected_con_to_b)
        wutils.verify_11ax_wifi_connection(
            self.dut, self.wifi6_models, "wifi6_ap" in self.user_params)
        self.log.info("Roaming from %s to %s", self.bssid_b, self.bssid_a)
        self.trigger_roaming_and_validate("AP1_on_AP2_off", expected_con_to_a)
        wutils.verify_11ax_wifi_connection(
            self.dut, self.wifi6_models, "wifi6_ap" in self.user_params)

    """ Tests Begin """

    @test_tracker_info(uuid="b15e4b3f-841d-428d-87ac-272f29f06e14")
    def test_roaming_with_config_tls(self):
        self.roaming_between_a_and_b_logic(self.config_tls)

    @test_tracker_info(uuid="d349cfec-b4af-48b2-88b7-744f5de25d43")
    def test_roaming_with_config_ttls_none(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.NONE.value
        self.roaming_between_a_and_b_logic(config)

    @test_tracker_info(uuid="89b8161c-754e-4138-831d-5fe40f521ce4")
    def test_roaming_with_config_ttls_pap(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.PAP.value
        self.roaming_between_a_and_b_logic(config)

    @test_tracker_info(uuid="d4925470-924b-4d03-8437-83e26b5f2df3")
    def test_roaming_with_config_ttls_mschap(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAP.value
        self.roaming_between_a_and_b_logic(config)

    @test_tracker_info(uuid="206b1327-dd9c-4742-8717-e7bf2a04eed6")
    def test_roaming_with_config_ttls_mschapv2(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAPV2.value
        self.roaming_between_a_and_b_logic(config)

    @test_tracker_info(uuid="c2c0168b-2933-4954-af62-fb41f42dc45a")
    def test_roaming_with_config_ttls_gtc(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.GTC.value
        self.roaming_between_a_and_b_logic(config)

    @test_tracker_info(uuid="481c4102-8f5b-4fcd-95cc-5e3285f47985")
    def test_roaming_with_config_peap_mschapv2(self):
        config = dict(self.config_peap)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAPV2.value
        self.roaming_between_a_and_b_logic(config)

    @test_tracker_info(uuid="404155d4-33a7-42b3-b369-5f2d63d19f16")
    def test_roaming_with_config_peap_gtc(self):
        config = dict(self.config_peap)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.GTC.value
        self.roaming_between_a_and_b_logic(config)

    """ Tests End """
