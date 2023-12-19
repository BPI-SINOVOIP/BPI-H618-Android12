# Copyright 2019 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Server side bluetooth tests about sending bluetooth HID reports."""

from __future__ import absolute_import

import logging
import time

import common
from autotest_lib.server.cros.bluetooth import bluetooth_adapter_tests


class BluetoothAdapterHIDReportTests(
        bluetooth_adapter_tests.BluetoothAdapterTests):
    """Server side bluetooth tests about sending bluetooth HID reports.

    This test tries to send HID reports to a DUT and verifies if the DUT
    could receive the reports correctly. For the time being, only bluetooth
    mouse events are tested. Bluetooth keyboard events will be supported
    later.
    """

    HID_TEST_SLEEP_SECS = 5

    def run_mouse_tests(self, device):
        """Run all bluetooth mouse reports tests.

        @param device: the bluetooth HID device.

        """
        self.test_mouse_left_click(device)
        self.test_mouse_right_click(device)
        self.test_mouse_move_in_x(device, 80)
        self.test_mouse_move_in_y(device, -50)
        self.test_mouse_move_in_xy(device, -60, 100)
        self.test_mouse_scroll_down(device, 70)
        self.test_mouse_scroll_up(device, 40)
        self.test_mouse_click_and_drag(device, 90, 30)


    def run_keyboard_tests(self, device):
        """Run all bluetooth mouse reports tests.

        @param device: the bluetooth HID device.

        """

        self.test_keyboard_input_from_trace(device, "simple_text")


    def run_battery_reporting_tests(self, device):
        """Run battery reporting tests.

        @param device: the Bluetooth device.

        """

        self.test_battery_reporting(device)

    def run_hid_reports_test(self, device,
                             check_connected_method=lambda device: True,
                             suspend_resume=False, reboot=False):
        """Running Bluetooth HID reports tests."""
        logging.info("run hid reports test")
        # Reset the adapter and set it pairable.
        self.test_reset_on_adapter()
        self.test_pairable()

        # Let the adapter pair, and connect to the target device.
        self.test_discover_device(device.address)
        self.test_pairing(device.address, device.pin, trusted=True)
        self.test_connection_by_adapter(device.address)

        # Run hid test to make sure profile is connected
        check_connected_method(device)

        if suspend_resume:
            self.suspend_resume()

            time.sleep(self.HID_TEST_SLEEP_SECS)
            self.test_device_is_paired(device.address)


            # check if peripheral is connected after suspend resume, reconnect
            # if it isn't
            if not self.ignore_failure(check_connected_method, device):
                logging.info("device not connected after suspend_resume")
                self.test_connection_by_device(device)
            else:
                logging.info("device remains connected after suspend_resume")

            time.sleep(self.HID_TEST_SLEEP_SECS)
            check_connected_method(device)

            time.sleep(self.HID_TEST_SLEEP_SECS)
            self.test_device_name(device.address, device.name)

        if reboot:
            self.reboot()

            time.sleep(self.HID_TEST_SLEEP_SECS)
            # TODO(b/173146480) - Power on the adapter for now until this bug
            # is resolved.
            self.test_power_on_adapter()

            self.test_device_is_paired(device.address)

            time.sleep(self.HID_TEST_SLEEP_SECS)
            self.test_connection_by_device(device)

            time.sleep(self.HID_TEST_SLEEP_SECS)
            self.test_device_name(device.address, device.name)

        # Run HID test after suspend/reboot as well.
        if suspend_resume or reboot:
            check_connected_method(device)

        # Disconnect the device, and remove the pairing.
        self.test_disconnection_by_adapter(device.address)
        self.test_remove_pairing(device.address)
