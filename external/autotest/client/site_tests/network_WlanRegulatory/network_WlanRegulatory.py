# Copyright (c) 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import interface
from autotest_lib.client.common_lib.cros.network import iw_runner


class network_WlanRegulatory(test.test):
    """
    Ensure the regulatory database is sane and that we can successfully switch
    domains using the "iw" userspace utility. We don't verify that the system
    truly respects the rules, but only that it does not reject them.
    Note that some drivers "self manage" their domain detection and so this
    test can't apply reliably. For those drivers that do not self-manage, we
    still ask for the phy-specific domain, to ensure that (if it's different
    than the global domain) it still follows along with the global domain.
    """
    version = 1
    # TODO(https://crbug.com/1000346): parse /lib/firmware/regulatory.db, once
    # CRDA goes away.
    REGULATORY_DATABASE = '/usr/lib/crda/regulatory.bin'

    def get_regulatory_domains(self):
        """Get the list or regulatory domains in the DUT's database."""
        return utils.system_output('regdbdump %s | grep country | '
                                   'sed -e s/^country.// -e s/:.*//' %
                                   self.REGULATORY_DATABASE).split()

    def assert_set_regulatory_domain(self, regdomain):
        """Set the system regulatory domain, then assert that it is correctly
        reflected for the wiphy.

        @param regdomain string 2-letter country code of the regulatory
            domain to set.

        """
        logging.info('Using iw to set regulatory domain to %s', regdomain)
        self._iw.set_regulatory_domain(regdomain)

        # The kernel handles the user hint asynchronously (either calling out
        # to udev/CRDA, or to the in-kernel database). Wait a bit.
        # TODO: poll instead, or watch for NL80211_CMD_REG_CHANGE.
        time.sleep(1)

        current_regdomain = self._iw.get_regulatory_domain(wiphy=self._wiphy)
        if current_regdomain != regdomain:
            raise error.TestFail('Expected iw to set regdomain %s but got %s' %
                                 (regdomain, current_regdomain))

    def run_once(self):
        """Test main loop"""
        self._iw = iw_runner.IwRunner()

        # If the driver "self manages" (NL80211_ATTR_WIPHY_SELF_MANAGED_REG)
        # its domain detection, we can't guarantee it will respect user-space
        # settings.
        if self._iw.is_regulatory_self_managed():
            raise error.TestNAError('Wiphy is self-managed')

        wlan_ifs = [nic for nic in interface.get_interfaces()
                    if nic.is_wifi_device()]
        if not wlan_ifs:
            raise error.TestFail('No WiFi device found')
        self._wiphy = wlan_ifs[0].wiphy_name

        # Stash the global domain; we can only 'set' the global domain, and we
        # want to restore it in the end if things go awry.
        self._initial_regdomain = self._iw.get_regulatory_domain()
        logging.info('Initial global regulatory domain is %s',
                     self._initial_regdomain)

        domain_list = self.get_regulatory_domains()
        if not domain_list:
            raise error.TestFail('Did not get a domain list from the database')

        for domain in domain_list:
            self.assert_set_regulatory_domain(domain)

    def cleanup(self):
        """Cleanup: restore device to original state."""
        if hasattr(self, '_initial_regdomain'):
            self.assert_set_regulatory_domain(self._initial_regdomain)
