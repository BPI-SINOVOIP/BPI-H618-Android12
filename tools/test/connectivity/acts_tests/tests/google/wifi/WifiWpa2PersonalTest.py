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


import acts_contrib.test_utils.wifi.wifi_test_utils as wutils
from acts_contrib.test_utils.wifi.WifiBaseTest import WifiBaseTest
from acts.controllers.ap_lib import hostapd_constants
from acts.controllers.openwrt_lib.openwrt_constants import OpenWrtWifiSecurity
from acts.test_decorators import test_tracker_info
from acts import asserts


WifiEnums = wutils.WifiEnums


class WifiWpa2PersonalTest(WifiBaseTest):
  """ Wi-Fi WPA2 test

      Test Bed Requirement:
        * One Android device
        * One OpenWrt Wi-Fi AP.
  """
  def __init__(self, configs):
    super().__init__(configs)
    self.enable_packet_log = True

  def setup_class(self):
    super().setup_class()
    self.dut = self.android_devices[0]
    opt_params = ["pixel_models", "cnss_diag_file"]
    self.unpack_userparams(opt_params)

  def setup_test(self):
    super().setup_test()
    for ad in self.android_devices:
      ad.droid.wakeLockAcquireBright()
      ad.droid.wakeUpNow()
      wutils.wifi_toggle_state(ad, True)
    wutils.reset_wifi(self.dut)

  def teardown_test(self):
    super().teardown_test()
    for ad in self.android_devices:
      ad.droid.wakeLockRelease()
      ad.droid.goToSleepNow()
    wutils.reset_wifi(self.dut)

  def start_openwrt(self, channel_2g=None, channel_5g=None):
    """Enable one OpenWrt to generate a Wi-Fi network.

      Args:
        channel_2g: Optional; An integer to represent a Wi-Fi 2g channel.
          The default value is 6 if it's not given.
        channel_5g: Optional; An integer to represent a Wi-Fi 5g channel.
          The default value is 36 if it's not given.
  """
    if not channel_2g:
      channel_2g = hostapd_constants.AP_DEFAULT_CHANNEL_2G
    if not channel_5g:
      channel_5g = hostapd_constants.AP_DEFAULT_CHANNEL_5G
    if "OpenWrtAP" in self.user_params:
      self.openwrt = self.access_points[0]
      self.configure_openwrt_ap_and_start(wpa_network=True,
                                          channel_2g=channel_2g,
                                          channel_5g=channel_5g)
      self.wpa2_psk_2g = self.wpa_networks[0]["2g"]
      self.wpa2_psk_5g = self.wpa_networks[0]["5g"]

  def verify_wpa_network_encryption(self, encryption):
    result = wutils.get_wlan0_link(self.dut)
    if encryption == 'psk2+ccmp':
      asserts.assert_true(
          result['pairwise_cipher'] == 'CCMP' and
          result['group_cipher'] == 'CCMP' and
          result['key_mgmt'] == "WPA2-PSK",
          'DUT does not connect to {} encryption network'.format(encryption))
    elif encryption == 'psk2+tkip':
      asserts.assert_true(
          result['pairwise_cipher'] == 'TKIP' and
          result['group_cipher'] == 'TKIP' and
          result['key_mgmt'] == "WPA2-PSK",
          'DUT does not connect to {} encryption network'.format(encryption))
    elif encryption == 'psk2+tkip+ccmp':
      asserts.assert_true(
          result['pairwise_cipher'] == 'CCMP' and
          result['group_cipher'] == 'TKIP' and
          result['key_mgmt'] == "WPA2-PSK",
          'DUT does not connect to {} encryption network'.format(encryption))

  """ Tests"""

  @test_tracker_info(uuid="d1f984c9-d85f-4b0d-8d64-2e8d6ce74c48")
  def test_connect_to_wpa2_psk_ccmp_2g(self):
    """Generate a Wi-Fi network.
       Change AP's security type to "WPA2" and cipher to "CCMP".
       Connect to 2g network.
    """
    self.start_openwrt()
    self.openwrt.log.info("Enable WPA2-PSK CCMP on OpenWrt AP")
    self.openwrt.set_wpa_encryption(OpenWrtWifiSecurity.WPA2_PSK_CCMP)
    wutils.connect_to_wifi_network(self.dut, self.wpa2_psk_2g)
    self.verify_wpa_network_encryption(OpenWrtWifiSecurity.WPA2_PSK_CCMP)

  @test_tracker_info(uuid="0f9631e8-04a9-4b9c-8225-ab30b4d1173b")
  def test_connect_to_wpa2_psk_ccmp_5g(self):
    """Generate a Wi-Fi network.
       Change AP's security type to "WPA2" and cipher to "CCMP".
       Connect to 5g network.
    """
    self.start_openwrt()
    self.openwrt.log.info("Enable WPA2-PSK CCMP on OpenWrt AP")
    self.openwrt.set_wpa_encryption(OpenWrtWifiSecurity.WPA2_PSK_CCMP)
    wutils.connect_to_wifi_network(self.dut, self.wpa2_psk_5g)
    self.verify_wpa_network_encryption(OpenWrtWifiSecurity.WPA2_PSK_CCMP)

  @test_tracker_info(uuid="e6eb3932-10cc-476f-a5d7-936e2631afc1")
  def test_connect_to_wpa2_psk_tkip_2g(self):
    """Generate a Wi-Fi network.
       Change AP's security type to "WPA2" and cipher to "TKIP".
       Connect to 2g network.
    """
    self.start_openwrt()
    self.openwrt.log.info("Enable WPA2-PSK TKIP on OpenWrt AP")
    self.openwrt.set_wpa_encryption(OpenWrtWifiSecurity.WPA2_PSK_TKIP)
    wutils.connect_to_wifi_network(self.dut, self.wpa2_psk_2g)
    self.verify_wpa_network_encryption(OpenWrtWifiSecurity.WPA2_PSK_TKIP)

  @test_tracker_info(uuid="59ba3cd4-dbc5-44f9-9290-48ae468a51da")
  def test_connect_to_wpa2_psk_tkip_5g(self):
    """Generate a Wi-Fi network.
       Change AP's security type to "WPA2" and cipher to "TKIP".
       Connect to 5g network.
    """
    self.start_openwrt()
    self.openwrt.log.info("Enable WPA2-PSK TKIP on OpenWrt AP")
    self.openwrt.set_wpa_encryption(OpenWrtWifiSecurity.WPA2_PSK_TKIP)
    wutils.connect_to_wifi_network(self.dut, self.wpa2_psk_5g)
    self.verify_wpa_network_encryption(OpenWrtWifiSecurity.WPA2_PSK_TKIP)

  @test_tracker_info(uuid="a06be3db-d653-4549-95f3-87bbeb0db813")
  def test_connect_to_wpa2_psk_tkip_and_ccmp_2g(self):
    """Generate a Wi-Fi network.
       Change AP's security type to "WPA2" and cipher to "CCMP and TKIP".
       Connect to 2g network.
    """
    self.start_openwrt()
    self.openwrt.log.info("Enable WPA2-PSK CCMP and TKIP on OpenWrt AP")
    self.openwrt.set_wpa_encryption(OpenWrtWifiSecurity.WPA2_PSK_TKIP_AND_CCMP)
    wutils.connect_to_wifi_network(self.dut, self.wpa2_psk_2g)
    self.verify_wpa_network_encryption(
        OpenWrtWifiSecurity.WPA2_PSK_TKIP_AND_CCMP)

  @test_tracker_info(uuid="ac9b9581-0b32-42b4-8e76-de702c837b86")
  def test_connect_to_wpa2_psk_tkip_and_ccmp_5g(self):
    """Generate a Wi-Fi network.
       Change AP's security type to "WPA2" and cipher to "CCMP and TKIP".
       Connect to 5g network.
    """
    self.start_openwrt()
    self.openwrt.log.info("Enable WPA2-PSK CCMP and TKIP on OpenWrt AP")
    self.openwrt.set_wpa_encryption(OpenWrtWifiSecurity.WPA2_PSK_TKIP_AND_CCMP)
    wutils.connect_to_wifi_network(self.dut, self.wpa2_psk_5g)
    self.verify_wpa_network_encryption(
        OpenWrtWifiSecurity.WPA2_PSK_TKIP_AND_CCMP)
