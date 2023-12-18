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

from acts.controllers.fuchsia_lib.base_lib import BaseLib


class FuchsiaBacklightLib(BaseLib):
    def __init__(self, addr, tc, client_id):
        self.address = addr
        self.test_counter = tc
        self.client_id = client_id

    def getStateNormalized(self):
        """Gets the backlight state and normalized brightness.

        Returns:
          The backlight state as a bool and the normalized brightness as a float in [0.0, 1.0], or
          an error if the call failed.
        """
        test_cmd = 'backlight_facade.GetStateNormalized'
        test_args = {}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def setStateNormalized(self, backlight_on, brightness):
        """Sets the backlight state and normalized brightness.

        Args:
          backlight_on: A bool indicating whether or not the backlight is on.
          brightness: A float in [0.0, 1.0] representing the backlight brightness.

        Returns:
          None or an error if the call failed.
        """
        test_cmd = 'backlight_facade.SetStateNormalized'
        test_args = {'backlight_on': backlight_on, 'brightness': brightness}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def getNormalizedBrightnessScale(self):
        """Gets the normalized brightness scale.

        Returns:
          The normalized brightness scale as a float in [0.0, 1.0], or an error if the call failed.
        """
        test_cmd = 'backlight_facade.GetNormalizedBrightnessScale'
        test_args = {}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def setNormalizedBrightnessScale(self, scale):
        """Sets the normalized brightness scale.

        Args:
          scale: The normalized brightness scale to set as a float in [0.0, 1.0].

        Returns:
          None or an error if the call failed.
        """
        test_cmd = 'backlight_facade.SetNormalizedBrightnessScale'
        test_args = scale
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)
