# Copyright (c) 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from datetime import datetime
import logging
import time
import os

from autotest_lib.server import site_linux_system
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base
from autotest_lib.server.cros.network import wpa_mon
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes

class network_WiFi_RoamNatural(wifi_cell_test_base.WiFiCellTestBase):
    """Bring up two APs, connect, vary attenuation as if the device is moving
    between the two APs (i.e. the signal gets weaker on one and stronger on the
    other until the first one cannot be seen anymore). At some point before the
    first AP is torn down, the device should have roamed to the second AP. If it
    doesn't there will be an association failure, which we can then log and
    write to a file. Ideally, there would be no association failures and a roam
    every time we expected one. Realistically, RSSI can vary quite widely, and
    we can't expect to see a good roam signal on every scan even where there
    should be one.

    This test is used to sanity check that "normal" roaming behavior is not
    broken by any roaming algorithm changes. A couple failed associations is
    acceptable, but any more than that is a good indication that roaming has
    become too sticky."""

    version = 1

    MAX_CENTER = 100
    MIN_CENTER = 84
    MAX_ATTEN = 106
    ATTEN_STEP = 2

    def test_body(self, pair_num, ap_pair, logger, roam_stats, failure_stats):
        """
        Execute the test with the given APs and record stats.

        @param pair_num int: the nth time this function is called (for results
        logging purposes).
        @param ap_pair tuple of HostapConfig objects: the APs
        @param logger WpaMon object: used for event monitoring
        @roam_stats dict of tuple to int: used to log roam stats.
        @failure_stats int list with a single int element: used to log assoc
        failure stats
        """
        # Reset the attenuation here since it won't have been reset after
        # previous iterations of this function.
        self.context.attenuator.set_variable_attenuation(0)

        min_atten = self.context.attenuator.get_minimal_total_attenuation()
        ap_pair[0].ssid = None
        self.context.configure(ap_pair[0])
        ssid = self.context.router.get_ssid()
        self.context.assert_connect_wifi(
            xmlrpc_datatypes.AssociationParameters(ssid=ssid))
        ap_pair[0].ssid = ssid
        ap_pair[1].ssid = ssid
        self.context.configure(ap_pair[1], configure_pcap=True)
        self.context.client.wait_for_bss(self.context.pcap_host.get_hostapd_mac(0))
        skip_roam_log = open(
            os.path.join(self.resultsdir,
                         str(pair_num) + '_skip_roam.txt'), 'w')
        assoc_failure_log = open(
            os.path.join(self.resultsdir,
                         str(pair_num) + '_failure.txt'), 'w')
        for center in range(self.MIN_CENTER, self.MAX_CENTER,
                            2 * self.ATTEN_STEP):
            # The attenuation should [con,di]verge around center. We move
            # the attenuation out 2dBm at a time until self.MAX_ATTEN is hit
            # on one AP, at which point we tear that AP down to simulate it
            # disappearing from the DUT's view. This should trigger a deauth
            # if the DUT is still associated.
            max_offset = self.MAX_ATTEN - center
            for _ in range(2):
                ranges = [range(0, max_offset, self.ATTEN_STEP),
                          range(max_offset, 0, -self.ATTEN_STEP),
                          range(0, -max_offset, -self.ATTEN_STEP),
                          range(-max_offset, 0, self.ATTEN_STEP)]
                for r, _ in enumerate(ranges):
                    self.context.client.clear_supplicant_blacklist()
                    logger.start_event_capture()
                    for offset in ranges[r]:
                        ap1_atten = max(center + offset, min_atten)
                        ap2_atten = max(center - offset, min_atten)
                        self.context.attenuator.set_total_attenuation(
                            ap1_atten, ap_pair[0].frequency, 0)
                        self.context.attenuator.set_total_attenuation(
                            ap1_atten, ap_pair[0].frequency, 1)
                        self.context.attenuator.set_total_attenuation(
                            ap2_atten, ap_pair[1].frequency, 2)
                        self.context.attenuator.set_total_attenuation(
                            ap2_atten, ap_pair[1].frequency, 3)
                        time.sleep(2)
                    if r % 2 == 1:
                        # The APs' RSSIs should have converged. No reason to
                        # check for disconnects/roams here.
                        continue

                    if r == 0:
                        # First AP is no longer in view
                        self.context.router.deconfig()
                    elif r == 2:
                        # Second AP is no longer in view
                        self.context.pcap_host.deconfig()

                    dc_events = logger.wait_for_event(
                        wpa_mon.WpaMon.CTRL_EVENT_DISCONNECTED, timeout=5)
                    if dc_events:
                        # Association failure happened, check if this
                        # was because a roam was skipped.
                        skip_roams = logger.get_events(
                            wpa_mon.WpaMon.CTRL_EVENT_SKIP_ROAM, True)
                        if skip_roams:
                            # Skipped roam caused association failure, log this
                            # so we can re-examine the roam decision.
                            for roam in skip_roams:
                                logging.info(roam)
                                skip_roam_log.write(str(roam) + '\n')
                                freq_pair = (int(roam.cur_freq) / 1000,
                                             int(roam.sel_freq) / 1000)
                                roam_stats[freq_pair] += 1
                        else:
                            # Association failure happened for some other reason
                            # (likely because AP disappeared before scan
                            # results returned). Log the failure for the
                            # timestamp in case we'd like to take a closer look.
                            for event in dc_events:
                                dc = str(datetime.now()) + ' ' +  str(event)
                                logging.info(dc)
                                assoc_failure_log.write(dc + '\n')
                                failure_stats[0] += 1

                    # Reset the attenuation here. In some groamer cells, the
                    # attenuation for 5GHz channels is miscalibrated such that
                    # the RSSI is lower than expected. If we bring the AP back
                    # up while it's still maximally attenuated, it may not be
                    # visible to the DUT (the test was written deliberately so
                    # that it wouldn't happen even at full attenuation for
                    # properly calibrated cells, but this is apparently not
                    # always a good assumption).
                    self.context.attenuator.set_variable_attenuation(0)

                    self.context.configure(ap_pair[r / 2],
                                           configure_pcap=(r == 2))
                    host = self.context.router if r == 0 else \
                        self.context.pcap_host
                    self.context.client.wait_for_bss(host.get_hostapd_mac(0))

        skip_roam_log.close()
        assoc_failure_log.close()
        self.context.client.shill.disconnect(ssid)
        self.context.router.deconfig()
        self.context.pcap_host.deconfig()

    def output_roam_stats(self, roam_skip_stats, failure_stats):
        """Output roam stats."""
        for pair, skips in roam_skip_stats.items():
            logging.info('%d association failures caused by skipped roams ' \
                         'from %s GHz to %s GHz', skips, pair[0], pair[1])
            self.output_perf_value('roam_natural_%s_%s' % (pair[0], pair[1]),
                                   skips, units='roams skipped',
                                   higher_is_better=False)
        logging.info('%d association failures unrelated to skipped roams',
                     failure_stats)
        self.output_perf_value('roam_natural_assoc_failures', failure_stats,
                               units='assocation failures',
                               higher_is_better=False)

    def run_once(self):
        """Body of the test."""
        self.context.client.require_capabilities(
            [site_linux_system.LinuxSystem.CAPABILITY_SUPPLICANT_ROAMING])

        mode = hostap_config.HostapConfig.MODE_11N_PURE
        ap1 = hostap_config.HostapConfig(channel=1, mode=mode)
        ap2 = hostap_config.HostapConfig(channel=2, mode=mode)
        ap3 = hostap_config.HostapConfig(channel=36, mode=mode)
        ap_configs = [(ap1, ap2), (ap1, ap3)]

        # Dictionary of roams skipped keyed by a pair of ints representing the
        # current AP's frequency band and the target AP's frequency band.
        roam_stats = {(2, 2): 0,
                      (2, 5): 0,
                      (5, 2): 0}
        failure_stats = [0]

        with self.context.client._wpa_mon as logger:
            for pair_num, ap_pair in enumerate(ap_configs):
                self.test_body(pair_num, ap_pair, logger, roam_stats, failure_stats)

        # Log roam skips and assoc failures and output perf values
        self.output_roam_stats(roam_stats, failure_stats[0])
