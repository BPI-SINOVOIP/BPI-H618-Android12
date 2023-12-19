# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base

class network_WiFi_BSSTMReq(wifi_cell_test_base.WiFiCellTestBase):
    """Tests a BSS Transition Management Request sent from the AP

    This test seeks to associate the DUT with an AP with a set of
    association parameters, create a second AP with a second set of
    parameters but the same SSID, and send a BSS Transition Management Request
    to the client. After that, the client will send a BSS Transition Management
    Response back to the first AP. We seek to observe that the DUT successfully
    connects to the second AP in a reasonable amount of time.
    """

    version = 1
    TIMEOUT_SECONDS = 15

    def _run_test(self, wait_for_scan=True):
        """Send BSS TM Requests and verify the behaviour.

        Setup an AP and make the DUT connect to that AP. Setup a second AP with
        the same SSID. Then send a BSS TM Request from the first AP to the
        second AP and verify that the DUT connects to te second AP.

        Args:
            wait_for_scan: when True, trigger a scan on the DUT and ensure that
                it has seen AP#2 before AP#1 sends a BSS TM Request.
        """
        router0_conf = hostap_config.HostapConfig(channel=1)
        router1_conf = hostap_config.HostapConfig(channel=48,
                mode=hostap_config.HostapConfig.MODE_11A)
        client_conf = xmlrpc_datatypes.AssociationParameters()

        # Capture the detection and connection to the second AP.
        self.context.capture_host.start_capture(router1_conf.frequency)

        # Configure the initial AP.
        self.context.configure(router0_conf)
        router_ssid = self.context.router.get_ssid()

        # Connect to the initial AP.
        client_conf.ssid = router_ssid
        self.context.assert_connect_wifi(client_conf)

        # Setup a second AP with the same SSID.
        router1_conf.ssid = router_ssid
        self.context.configure(router1_conf, multi_interface=True)

        # BSSID of the second AP. The DUT will have to roam to that second AP.
        bssid_roam = self.context.router.get_hostapd_mac(1)

        # Flush all scanned BSS from supplicant. Otherwise the DUT may have
        # noticed them in a scan before, which would change the behaviour of the
        # test.
        self.context.client.flush_bss(age=0)

        if wait_for_scan:
            # Wait for DUT to see the second AP.
            logging.info('Scanning to find BSS "%s"', bssid_roam)
            self.context.client.wait_for_bss(bssid_roam)

        # Send BSS Transition Management Request to client.
        reply = self.context.router.send_bss_tm_req(
            self.context.client.wifi_mac,
            [bssid_roam])
        if reply == 'OK':
            pass
        elif reply.startswith('Unknown command'):
            raise error.TestNAError('AP does not support BSS Transition '
                                    'Management')
        else:
            raise error.TestFail('Failed to send BSS TM Request: %s' % reply)

        # Expect that the DUT will re-connect to the new AP.
        # In some cases, the DUT may have roamed to the second AP on its own,
        # without waiting for the BSS TM Request. In those particular runs, the
        # test would not test the BSS TM feature, but the result would be
        # identical. And in most runs the roam will be caused by the BSS TM
        # Request anyway, so it's still a valid way to test the feature.
        # TODO: Enforce correct roaming behaviour when porting the test to Tast.
        if not self.context.client.wait_for_roam(
                bssid_roam, timeout_seconds=self.TIMEOUT_SECONDS):
            raise error.TestFail('Failed to roam after%s scanning.' %
                    ('' if wait_for_scan else ' not'))

        # Tear down.
        self.context.client.shill.disconnect(router_ssid)
        self.context.router.deconfig_aps()
        self.context.capture_host.stop_capture()

    def run_once(self):
        """Test body."""
        # Before sending the BSS TM Request, run a scan and make sure the DUT
        # has seen the second AP. In that case, the DUT will typically re-use
        # the result of the scan when receiving the request instead of probing
        # the second AP.
        self._run_test(wait_for_scan=True)

        # After setting up both APs, immediately send the BSS TM Request before
        # the DUT has scanned and noticed the second AP (at least in the
        # majority of test runs). Instead of relying on the result of a previous
        # scan, the DUT will probe for the second AP when receiving the
        # transition request.
        self._run_test(wait_for_scan=False)

    def cleanup(self):
        """Cleanup function."""
        super(network_WiFi_BSSTMReq, self).cleanup()
