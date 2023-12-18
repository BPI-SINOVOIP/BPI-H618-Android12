#!/usr/bin/env python3
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

import datetime
import http
import requests

import acts.controllers.fuchsia_lib.base_lib as base_lib

HW_PWR_STATE_CONTROL_TIMEOUT = 5


class FuchsiaHardwarePowerStatecontrolLib(base_lib.BaseLib):
    def __init__(self, addr, tc, client_id):
        self.address = addr
        self.test_counter = tc
        self.client_id = client_id

    def suspendReboot(self, timeout=HW_PWR_STATE_CONTROL_TIMEOUT):
        """Call Suspend Reboot.

        Returns:
            None if success.
        """
        test_cmd = "hardware_power_statecontrol_facade.SuspendReboot"
        test_args = {}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1
        try:
            response = self.send_command(test_id,
                                         test_cmd,
                                         test_args,
                                         response_timeout=timeout)
        except (ConnectionResetError, base_lib.DeviceOffline,
                requests.exceptions.ConnectionError,
                requests.exceptions.ReadTimeout):
            return
        return response

    def suspendRebootBootloader(self, timeout=HW_PWR_STATE_CONTROL_TIMEOUT):
        """Call Suspend Reboot Bootloader

        Returns:
            None if success.
        """
        test_cmd = "hardware_power_statecontrol_facade.SuspendRebootBootloader"
        test_args = {}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1
        try:
            response = self.send_command(test_id,
                                         test_cmd,
                                         test_args,
                                         response_timeout=timeout)
        except (requests.exceptions.ReadTimeout,
                http.client.RemoteDisconnected, base_lib.DeviceOffline):
            return
        return response

    def suspendPoweroff(self, timeout=HW_PWR_STATE_CONTROL_TIMEOUT):
        """Call Suspend Poweroff

        Returns:
            None if success.
        """
        test_cmd = "hardware_power_statecontrol_facade.SuspendPoweroff"
        test_args = {}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1
        try:
            response = self.send_command(test_id,
                                         test_cmd,
                                         test_args,
                                         response_timeout=timeout)
        except (requests.exceptions.ReadTimeout,
                http.client.RemoteDisconnected, base_lib.DeviceOffline):
            return
        return response

    def suspendMexec(self, timeout=HW_PWR_STATE_CONTROL_TIMEOUT):
        """Call Suspend Mexec

        Returns:
            None if success.
        """
        test_cmd = "hardware_power_statecontrol_facade.SuspendMexec"
        test_args = {}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1
        try:
            response = self.send_command(test_id,
                                         test_cmd,
                                         test_args,
                                         response_timeout=timeout)
        except (requests.exceptions.ReadTimeout,
                http.client.RemoteDisconnected, base_lib.DeviceOffline):
            return
        return response

    def suspendRam(self, timeout=HW_PWR_STATE_CONTROL_TIMEOUT):
        """Call Suspend Ram

        Returns:
            None if success.
        """
        test_cmd = "hardware_power_statecontrol_facade.SuspendRam"
        test_args = {}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1
        try:
            response = self.send_command(test_id,
                                         test_cmd,
                                         test_args,
                                         response_timeout=timeout)
        except (requests.exceptions.ReadTimeout,
                http.client.RemoteDisconnected, base_lib.DeviceOffline):
            return
        return response
