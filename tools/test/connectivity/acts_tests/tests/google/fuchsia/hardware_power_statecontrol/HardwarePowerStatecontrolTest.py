#!/usr/bin/env python3
#
# Copyright (C) 2020 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

from acts import signals
from acts.base_test import BaseTestClass
from acts import asserts
"""
This is a stress test for Fuchsia reboot tests.

Custom Params:
reboot_stress_test_iterations

    Example:
    "reboot_stress_test_iterations": 50

Setup:
One Fuchsia device
"""


class HardwarePowerStatecontrolStressTest(BaseTestClass):
    default_iterations = 100

    def setup_class(self):
        super().setup_class()
        self.dut = self.fuchsia_devices[0]
        self.default_iterations = self.user_params.get(
            "reboot_stress_test_iterations", self.default_iterations)

    def test_suspend_reboot(self):
        """Verify suspend reboot functionality

        Steps:
        1. Send a suspend reboot command to DUT.

        Expected Result:
        No errors in sending a reboot and device comes back online.

        Returns:
          signals.TestPass if no errors
          signals.TestFailure if there are any errors during the test.

        TAGS: Hardware
        Priority: 1
        """
        try:
            self.dut.reboot()
        except Exception as err:
            signals.TestFailure("Failed with err: {}".format(err))
        signals.TestPass("Pass")

    def test_suspend_reboot_stress(self):
        """Verify suspend reboot functionality N times.

        Steps:
        1. Send a suspend reboot command to DUT N times

        Expected Result:
        No errors in sending a reboot and device comes back online.

        Returns:
          signals.TestPass if no errors
          signals.TestFailure if there are any errors during the test.

        TAGS: Hardware
        Priority: 1
        """
        for i in range(self.default_iterations):
            try:
                self.dut.reboot()
            except Exception as err:
                signals.TestFailure("Failed with err: {}".format(err))
            self.log.info("Iteration {} successful".format(i + 1))

        signals.TestPass("Pass")
