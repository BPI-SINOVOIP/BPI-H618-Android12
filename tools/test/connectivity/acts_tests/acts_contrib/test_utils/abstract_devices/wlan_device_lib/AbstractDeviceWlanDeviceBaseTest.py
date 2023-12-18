#!/usr/bin/env python3
#
#   Copyright (C) 2020 The Android Open Source Project
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
from acts_contrib.test_utils.wifi.WifiBaseTest import WifiBaseTest


class AbstractDeviceWlanDeviceBaseTest(WifiBaseTest):
    def setup_class(self):
        super().setup_class()

    def on_fail(self, test_name, begin_time):
        try:
            self.dut.get_log(test_name, begin_time)
            if (not hasattr(self.dut.device, "take_bug_report_on_fail")
                    or self.dut.device.take_bug_report_on_fail):
                # Take a bug report if device does not have a take bug report flag,
                # or if the flag is true
                self.dut.take_bug_report(test_name, begin_time)
        except Exception:
            pass

        try:
            if self.dut.device.hard_reboot_on_fail:
                self.dut.hard_power_cycle(self.pdu_devices)
        except AttributeError:
            pass
