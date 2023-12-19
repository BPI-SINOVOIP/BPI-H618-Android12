# Copyright (c) 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.server import site_linux_system
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base
from autotest_lib.server.cros.network import wpa_mon
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes

class network_WiFi_RoamDiagnostics(wifi_cell_test_base.WiFiCellTestBase):
    """Bring an AP up, connect to it, set the attenuation, and vary a second AP
    around the same RSSI as the first AP. Perform a scan after every change in
    attenuation and observe when the device roams between APs. Record all roam
    events in a file for analysis.

    The purpose of this diagnostic is to capture the stickiness of the device's
    roam algorithm. For example, the stickier the algorithm, the more skewed
    toward higher RSSI differentials (between current and target AP) the
    distribution of roams in the output files will be. This is not necessarily
    a good thing as it's important for a device to be able to move between APs
    when it needs to. Therefore, we use network_WiFi_RoamNatural in conjunction
    with this test to ensure that normal roam behavior is not broken."""

    version = 1
    MAX_ATTEN = 96
    MIN_ATTEN = 56
    ATTEN_STEP = 4
    ATTEN_RANGE = 12
    ROAM_BUCKETS = 7

    def test_body(self, pair_num, ap_pair, logger, roam_stats):
        """
        Execute the test on the given APs.

        @param pair_num int: the nth time this function is called (for logging
        purpose).
        @param ap_pair tuple of HostapConfig objects: the APs.
        @param logger WpaMon object: used for event monitoring.
        """
        self.context.configure(ap_pair[0])
        ssid = self.context.router.get_ssid()
        bgscan_none = xmlrpc_datatypes.BgscanConfiguration(
            method=xmlrpc_datatypes.BgscanConfiguration.SCAN_METHOD_NONE)
        assoc_params = xmlrpc_datatypes.AssociationParameters(
                ssid=ssid,
                bgscan_config=bgscan_none)
        self.context.assert_connect_wifi(assoc_params)
        ap_pair[1].ssid = ssid
        self.context.configure(ap_pair[1], configure_pcap=True)

        roam_log = open(
            os.path.join(self.resultsdir, str(pair_num) + "_roam.txt"), 'w')
        for atten0 in range(self.MIN_ATTEN, self.MAX_ATTEN, self.ATTEN_STEP):
            self.context.attenuator.set_total_attenuation(
                atten0, ap_pair[0].frequency, 0)
            self.context.attenuator.set_total_attenuation(
                atten0, ap_pair[0].frequency, 1)

            # Vary the RSSI of the second AP around that of the first AP.
            min_atten = max(
                atten0 - self.ATTEN_RANGE,
                self.context.attenuator.get_minimal_total_attenuation())
            max_atten = atten0 + self.ATTEN_RANGE
            for _ in range(2):
                for atten1 in range(max_atten, min_atten, -self.ATTEN_STEP) + \
                              range(min_atten, max_atten, self.ATTEN_STEP):
                    self.context.attenuator.set_total_attenuation(
                        atten1, ap_pair[1].frequency, 2)
                    self.context.attenuator.set_total_attenuation(
                        atten1, ap_pair[1].frequency, 3)

                    scan_success = False
                    logger.start_event_capture()
                    for i in range(2):
                        # Explicitly ask shill to perform a scan. This
                        # should induce a roam if the RSSI difference is
                        # large enough.
                        self.context.client.shill.request_scan()
                        if logger.wait_for_event(wpa_mon.WpaMon.\
                                                 CTRL_EVENT_SCAN_RESULTS):
                            scan_success = True
                            break
                        logging.info("Scan failed %d time(s)", i + 1)
                    if not scan_success:
                        logging.error("Unable to get scan results")
                        continue

                    # Wait for roam.
                    roams = logger.wait_for_event(
                        wpa_mon.WpaMon.CTRL_EVENT_DO_ROAM, timeout=5)
                    for roam in roams:
                        logging.info(roam)
                        roam_log.write(str(roam) + '\n')
                        freq_pair = (int(roam.cur_freq) / 1000,
                                     int(roam.sel_freq) / 1000)
                        diff = max(min(int(roam.sel_level) - \
                                       int(roam.cur_level),
                                       (self.ROAM_BUCKETS - 1) * 2), 0)
                        roam_stats[freq_pair][diff / 2] += 1

        roam_log.close()
        self.context.client.shill.disconnect(ssid)
        self.context.router.deconfig()
        self.context.pcap_host.deconfig()

    def output_roam_stats(self, roam_stats):
        """Output roam stats."""
        for pair, stats in roam_stats.items():
            total = sum(stats)
            logging.info('Roams from %d GHz to %d GHz',
                         pair[0], pair[1])
            self.output_perf_value('roam_diagnostics_%d_%d' % \
                                   (pair[0], pair[1]),
                                   stats, units='roams',
                                   higher_is_better=False)
            for i, roams in enumerate(stats):
                logging.info('%d roams out of %d with diff >= %d', roams,
                             total, i * 2)

    def run_once(self):
        """Body of the test."""
        self.context.client.require_capabilities(
            [site_linux_system.LinuxSystem.CAPABILITY_SUPPLICANT_ROAMING])

        mode = hostap_config.HostapConfig.MODE_11N_PURE
        ap1 = hostap_config.HostapConfig(channel=1, mode=mode)
        ap2 = hostap_config.HostapConfig(channel=2, mode=mode)
        ap3 = hostap_config.HostapConfig(channel=36, mode=mode)
        ap_configs = [(ap1, ap2), (ap1, ap3)]

        # roam_stats records the number of roams that occurred in each roam
        # bucket for each frequency type (2.4 GHz to 2.4 GHz, 2.4 GHz to 5 GHz,
        # and 5 GHz to 2.4 GHz; we don't test 5 GHz to 5 GHz because the logic
        # should be the same as 2.4 GHz to 2.4 GHz). Each bucket holds a 2 dBm
        # range of RSSI differences at which a roam occurred. For example,
        # roam_stats[(2, 5)][5] represents the number of roams from 2.4 GHz to
        # 5GHz that happened at an RSSI difference of 8-9 dBm.
        roam_stats = {(2, 2): [0] * self.ROAM_BUCKETS,
                      (2, 5): [0] * self.ROAM_BUCKETS,
                      (5, 2): [0] * self.ROAM_BUCKETS}

        with self.context.client._wpa_mon as logger:
            for pair_num, ap_pair in enumerate(ap_configs):
                self.test_body(pair_num, ap_pair, logger, roam_stats)

        # Log roam distribution and output perf values
        self.output_roam_stats(roam_stats)
