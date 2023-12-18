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

from acts.controllers.fuchsia_lib.base_lib import BaseLib


class FuchsiaGpioLib(BaseLib):
    def __init__(self, addr, tc, client_id):
        self.address = addr
        self.test_counter = tc
        self.client_id = client_id

    def configIn(self, pin, flags):
        """ Configures a GPIO for input.

        Args:
           pin: uint32, pin number of GPIO
           flags: uint32, flags to configure for GPIO

        Returns:
           None if success, prints error message if error.
        """
        test_cmd = "gpio_facade.ConfigIn"
        test_args = {"pin": pin, "flags": flags}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def configOut(self, pin, value):
        """ Configures a GPIO for output.

        Args:
           pin: uint32, pin number of GPIO
           value: uint8, initial value

        Returns:
           None if success, prints error message if error.
        """
        test_cmd = "gpio_facade.ConfigOut"
        test_args = {"pin": pin, "value": value}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def read(self, pin):
        """ Reads the current value of a GPIO (0 or 1).

        Args:
           pin: uint32, pin number of GPIO

        Returns:
           uint8. Current value of GPIO.
        """
        test_cmd = "gpio_facade.Read"
        test_args = {"pin": pin}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def write(self, pin, value):
        """ Sets the current value of the GPIO (any non-zero value maps to 1).

        Args:
           pin: uint32, pin number of GPIO
           value: uint8, value of GPIO

        Returns:
           None if success, prints error message if error.
        """
        test_cmd = "gpio_facade.Write"
        test_args = {"pin": pin, "value": value}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)
