#
#   Copyright 2020 - The Android Open Source Project
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


from acts import base_test
from acts.test_decorators import test_tracker_info
from acts_contrib.test_utils.net import connectivity_const
from acts_contrib.test_utils.net import net_test_utils as nutils
from acts_contrib.test_utils.wifi import wifi_test_utils as wutils

VPN_CONST = connectivity_const.VpnProfile
VPN_TYPE = connectivity_const.VpnProfileType
VPN_PARAMS = connectivity_const.VpnReqParams


class IKEv2VpnOverWifiTest(base_test.BaseTestClass):
  """IKEv2 VPN tests."""

  def setup_class(self):
    self.dut = self.android_devices[0]

    required_params = dir(VPN_PARAMS)
    required_params = [x for x in required_params if not x.startswith("__")]
    self.unpack_userparams(req_param_names=required_params)
    self.vpn_params = {
        "vpn_username": self.vpn_username,
        "vpn_password": self.vpn_password,
        "psk_secret": self.psk_secret,
        "client_pkcs_file_name": self.client_pkcs_file_name,
        "cert_path_vpnserver": self.cert_path_vpnserver,
        "cert_password": self.cert_password,
        "vpn_identity": self.vpn_identity,
    }

    wutils.wifi_test_device_init(self.dut)
    wutils.connect_to_wifi_network(self.dut, self.wifi_network)

  def teardown_class(self):
    wutils.reset_wifi(self.dut)

  def on_fail(self, test_name, begin_time):
    self.dut.take_bug_report(test_name, begin_time)

  ### Helper methods ###
  def _test_ikev2_vpn(self, vpn, hostname=None):
    """Verify IKEv2 VPN connection.

    Args:
      vpn: type of VPN.
      hostname: hostname or IP address of the server.
    """
    server_addr = self.vpn_server_addresses[vpn.name][0]
    self.vpn_params["server_addr"] = server_addr
    if not hostname:
      hostname = server_addr
    vpn_addr = self.vpn_verify_addresses[vpn.name][0]
    vpn_profile = nutils.generate_ikev2_vpn_profile(
        self.dut, self.vpn_params, vpn, hostname, self.log_path)
    nutils.legacy_vpn_connection_test_logic(self.dut, vpn_profile, vpn_addr)

  ### Test cases ###

  @test_tracker_info(uuid="4991755c-321d-4e9a-ada9-fc821a35bb5b")
  def test_ikev2_psk_vpn_wifi(self):
    self._test_ikev2_vpn(VPN_TYPE.IKEV2_IPSEC_PSK)

  @test_tracker_info(uuid="04d88575-7b96-4746-bff8-a1d6841e202e")
  def test_ikev2_mschapv2_vpn_wifi(self):
    self._test_ikev2_vpn(VPN_TYPE.IKEV2_IPSEC_USER_PASS)

  @test_tracker_info(uuid="e65f8a3e-f807-4493-822e-377dd6fa89cd")
  def test_ikev2_rsa_vpn_wifi(self):
    self._test_ikev2_vpn(VPN_TYPE.IKEV2_IPSEC_RSA)

  @test_tracker_info(uuid="bdd8a967-8dac-4e48-87b7-2ce9f7d32158")
  def test_ikev2_psk_vpn_wifi_with_hostname(self):
    self._test_ikev2_vpn(VPN_TYPE.IKEV2_IPSEC_PSK, self.vpn_server_hostname)

  @test_tracker_info(uuid="19692520-c123-4b42-8549-08dda9c4873e")
  def test_ikev2_mschapv2_vpn_wifi_with_hostname(self):
    self._test_ikev2_vpn(VPN_TYPE.IKEV2_IPSEC_USER_PASS,
                         self.vpn_server_hostname)

  @test_tracker_info(uuid="bdaaf6e3-6671-4533-baba-2951009c7d69")
  def test_ikev2_rsa_vpn_wifi_with_hostname(self):
    self._test_ikev2_vpn(VPN_TYPE.IKEV2_IPSEC_RSA, self.vpn_server_hostname)
