# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest

class firmware_ECCharging(FirmwareTest):
    """
    Servo based EC charging control test.
    """
    version = 1

    # Flags set by battery
    BATT_FLAG_WANT_CHARGE = 0x1
    STATUS_FULLY_CHARGED = 0x20

    # Threshold of trickle charging current in mA
    TRICKLE_CHARGE_THRESHOLD = 100

    # The dict to cache the battery information
    BATTERY_INFO = {}

    def initialize(self, host, cmdline_args):
        super(firmware_ECCharging, self).initialize(host, cmdline_args)
        # Don't bother if there is no Chrome EC.
        if not self.check_ec_capability():
            raise error.TestNAError("Nothing needs to be tested on this device")
        # Only run in normal mode
        self.switcher.setup_mode('normal')
        self.ec.send_command("chan 0")


    def cleanup(self):
        try:
            self.ec.send_command("chan 0xffffffff")
        except Exception as e:
            logging.error("Caught exception: %s", str(e))
        super(firmware_ECCharging, self).cleanup()

    def _update_battery_info(self):
        """Get the battery info we care for this test."""
        # The battery parameters we care for this test. The order must match
        # the output of EC battery command.
        battery_params = ['V', 'V-desired', 'I', 'I-desired', 'Charge']
        regex_str_list = []

        for p in battery_params:
            if p == 'Charge':
                regex_str_list.append(p + ':\s+(\d+)\s+')
            else:
                regex_str_list.append(p + ':\s+0x[0-9a-f]*\s+=\s+([0-9-]+)\s+')

        battery_regex_match = self.ec.send_command_get_output('battery',
                                                              regex_str_list)
        for i in range(len(battery_params)):
            self.BATTERY_INFO[battery_params[i]] = int(
                    battery_regex_match[i][1])


    def _get_battery_desired_voltage(self):
        """Get battery desired voltage value."""
        if not self.BATTERY_INFO:
            self._update_battery_info()
        logging.info('Battery desired voltage = %d mV',
                     self.BATTERY_INFO['V-desired'])
        return self.BATTERY_INFO['V-desired']


    def _get_battery_desired_current(self):
        """Get battery desired current value."""
        if not self.BATTERY_INFO:
            self._update_battery_info()
        logging.info('Battery desired current = %d mA',
                     self.BATTERY_INFO['I-desired'])
        return self.BATTERY_INFO['I-desired']


    def _get_battery_actual_voltage(self):
        """Get the actual voltage from charger to battery."""
        if not self.BATTERY_INFO:
            self._update_battery_info()
        logging.info('Battery actual voltage = %d mV', self.BATTERY_INFO['V'])
        return self.BATTERY_INFO['V']


    def _get_battery_actual_current(self):
        """Get the actual current from charger to battery."""
        if not self.BATTERY_INFO:
            self._update_battery_info()
        logging.info('Battery actual current = %d mA', self.BATTERY_INFO['I'])
        return self.BATTERY_INFO['I']


    def _get_battery_charge(self):
        """Get battery charge state."""
        if not self.BATTERY_INFO:
            self._update_battery_info()
        logging.info("Battery charge = %d %%", self.BATTERY_INFO['Charge'])
        return self.BATTERY_INFO['Charge']


    def _get_charger_target_voltage(self):
        """Get target charging voltage set in charger."""
        voltage = int(self.ec.send_command_get_output("charger",
                ["V_batt:\s+(\d+)\s"])[0][1])
        logging.info("Charger target voltage = %d mV", voltage)
        return voltage


    def _get_charger_target_current(self):
        """Get target charging current set in charger."""
        current = int(self.ec.send_command_get_output("charger",
                ["I_batt:\s+(\d+)\s"])[0][1])
        logging.info("Charger target current = %d mA", current)
        return current


    def _get_trickle_charging(self):
        """Check if we are trickle charging battery."""
        return (self._get_battery_desired_current() <
                self.TRICKLE_CHARGE_THRESHOLD)


    def _check_target_value(self):
        """Check charger target values are correct.

        Raise:
          error.TestFail: Raised when check fails.
        """
        if (self._get_charger_target_voltage() >=
                1.05 * self._get_battery_desired_voltage()):
            raise error.TestFail("Charger target voltage is too high.")
        if (self._get_charger_target_current() >=
                1.05 * self._get_battery_desired_current()):
            raise error.TestFail("Charger target current is too high.")


    def _check_actual_value(self):
        """Check actual voltage/current values are correct.

        Raise:
          error.TestFail: Raised when check fails.
        """
        if (self._get_battery_actual_voltage() >=
                1.05 * self._get_charger_target_voltage()):
            raise error.TestFail("Battery actual voltage is too high.")
        if (self._get_battery_actual_current() >=
                1.05 * self._get_charger_target_current()):
            raise error.TestFail("Battery actual current is too high.")

    def _check_if_discharge_on_ac(self):
        """Check if DUT is performing discharge on AC"""
        match = self.ec.send_command_get_output("battery",
                ["Status:\s*(0x[0-9a-f]+)\s", "Param flags:\s*([0-9a-f]+)\s"])
        status = int(match[0][1], 16)
        params = int(match[1][1], 16)

        if (not (params & self.BATT_FLAG_WANT_CHARGE) and
                (status & self.STATUS_FULLY_CHARGED)):
            return True

        return False

    def run_once(self):
        """Execute the main body of the test.
        """
        if not self.check_ec_capability(['battery', 'charging']):
            raise error.TestNAError("Nothing needs to be tested on this device")
        if self._get_battery_charge() == 100:
            logging.info("Battery is full. Unable to test.")
            return
        if self._check_if_discharge_on_ac():
            logging.info("DUT is performing discharge on AC. Unable to test.")
            return
        if self._get_trickle_charging():
            logging.info("Trickling charging battery. Unable to test.")
            return
        if self._get_battery_actual_current() < 0:
            raise error.TestFail("This test must be run with AC power.")

        logging.info("Checking charger target values...")
        self._check_target_value()

        logging.info("Checking battery actual values...")
        self._check_actual_value()
