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


class RgbValue:
    # use __dict__ to send in json
    def __init__(self, r, g, b):
        self.red = r
        self.green = g
        self.blue = b


class FuchsiaLightLib(BaseLib):
    def __init__(self, addr, tc, client_id):
        self.address = addr
        self.test_counter = tc
        self.client_id = client_id

    def getNumLights(self):
        """ Gets the total number of physical lights on the Fuchsia device
        under test.

        Returns:
            Integer (number of lights), prints an error message if error.
        """
        test_cmd = "light_facade.GetNumLights"
        test_args = {}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def getNumLightGroups(self):
        """ Gets the total number of light groups on the Fuchsia device
        under test.

        Returns:
            Integer (number of light groups), prints an error message if error.
        """
        test_cmd = "light_facade.GetNumLightGroups"
        test_args = {}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def getInfo(self, index):
        """ Gets info (name and capability) for a single light on the Fuchsia device
        under test.

        Args:
            index: uint32, index of the light defined by board, must be less than
                   value returned by getNumLights

        Returns:
            The Info object, prints an error message if error.
        """
        test_cmd = "light_facade.GetInfo"
        test_args = {"index": index}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def getCurrentSimpleValue(self, index):
        """ Gets the current simple value for a single light on the Fuchsia device
        under test.

        Args:
            index: uint32, index of the light defined by board, must be less than
                   value returned by getNumLights

        Returns:
            Boolean. True if the light is ON, False if the light is OFF.
            If the capability 'SIMPLE' is not supported by this light, fails and prints
            error message.
            Use GetInfo to check if light supports this operation.
        """
        test_cmd = "light_facade.GetCurrentSimpleValue"
        test_args = {"index": index}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def setSimpleValue(self, index, value):
        """ Sets the simple value for a single light on the Fuchsia device
        under test.

        Args:
            index: uint32, index of the light defined by board, must be less than
                   value returned by getNumLights
            value: boolean, True for ON, False for OFF

        Returns:
            None if success, prints error message if error (e.g. capability 'SIMPLE'
            is not supported by the light).
        """
        test_cmd = "light_facade.SetSimpleValue"
        test_args = {"index": index, "value": value}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def getCurrentBrightnessValue(self, index):
        """ Gets the current brightness value for a single light on the Fuchsia device
        under test.

        Args:
            index: uint32, index of the light defined by board, must be less than
                   value returned by getNumLights

        Returns:
            uint8. A value 0-255 inclusive indicating the brightness of the light.
            If the capability 'BRIGHTNESS' is not supported by this light, fails and prints
            error message.
            Use GetInfo to check if light supports this operation.
        """
        test_cmd = "light_facade.GetCurrentBrightnessValue"
        test_args = {"index": index}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def setBrightnessValue(self, index, value):
        """ Sets the brightness value for a single light on the Fuchsia device
        under test.

        Args:
            index: uint32, index of the light defined by board, must be less than
                   value returned by getNumLights
            value: uint8, 0-255 inclusive indicating the brightness of the light

        Returns:
            None if success, prints error message if error (e.g. capability
            'BRIGHTNESS' is not supported by the light).
        """
        test_cmd = "light_facade.SetBrightnessValue"
        test_args = {"index": index, "value": value}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def getCurrentRgbValue(self, index):
        """ Gets the current RGB value for a single light on the Fuchsia device
        under test.

        Args:
            index: uint32, index of the light defined by board, must be less than
                   value returned by getNumLights

        Returns:
            The RGB object indicating the RGB value of the light.
            If the capability 'RGB' is not supported by this light, fails and prints
            error message.
            Use GetInfo to check if light supports this operation.
        """
        test_cmd = "light_facade.GetCurrentRgbValue"
        test_args = {"index": index}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def setRgbValue(self, index, value):
        """ Sets the RGB value for a single light on the Fuchsia device
        under test.

        Args:
            index: uint32, index of the light defined by board, must be less than
                   value returned by getNumLights
            value: an RGB object

        Returns:
            None if success, prints error message if error (e.g. capability 'RGB'
            is not supported by the light).
        """
        test_cmd = "light_facade.SetRgbValue"
        test_args = {"index": index, "value": value}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def getGroupInfo(self, index):
        """ Gets group info (name, number of lights, and capability) for a light
        group on the Fuchsia device under test.

        Args:
            index: uint32, index of the light group defined by board, must be less
                   than value returned by getNumLightGroups

        Returns:
            The GroupInfo object, prints an error message if error.
        """
        test_cmd = "light_facade.GetGroupInfo"
        test_args = {"index": index}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def getGroupCurrentSimpleValue(self, index):
        """ Gets the current simple values for the light group on the Fuchsia device
        under test.

        Args:
            index: uint32, index of the light group defined by board, must be less than
                   value returned by getNumLightGroups

        Returns:
            Vector of booleans, each indicating whether or not the light is on.
            If the capability 'SIMPLE' is not supported by the light group, fails and prints
            error message.
            Use GetGroupInfo to check if light supports this operation.
        """
        test_cmd = "light_facade.GetGroupCurrentSimpleValue"
        test_args = {"index": index}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def setGroupSimpleValue(self, index, values):
        """ Sets the simple value for a light group on the Fuchsia device
        under test.

        Args:
            index: uint32, index of the light defined by board, must be less than
                   value returned by getNumLights
            values: a vector of booleans, each has True for ON, False for OFF

        Returns:
            None if success, prints error message if error (e.g. capability 'SIMPLE'
            is not supported by the light group).
        """
        test_cmd = "light_facade.SetGroupSimpleValue"
        test_args = {"index": index, "values": values}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def getGroupCurrentBrightnessValue(self, index):
        """ Gets the current brightness values for the light group on the Fuchsia device
        under test.

        Args:
            index: uint32, index of the light group defined by board, must be less than
                   value returned by getNumLightGroups

        Returns:
            Vector of uint8, each indicating the brightness of the light.
            If the capability 'SIMPLE' is not supported by the light group, fails and prints
            error message.
            Use GetGroupInfo to check if light supports this operation.
        """
        test_cmd = "light_facade.GetGroupCurrentBrightnessValue"
        test_args = {"index": index}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def setGroupBrightnessValue(self, index, values):
        """ Sets the brightness value for a light group on the Fuchsia device
        under test.

        Args:
            index: uint32, index of the light defined by board, must be less than
                   value returned by getNumLights
            values: a vector of uint8s, each indicating the brightness of a light.

        Returns:
            None if success, prints error message if error (e.g. capability 'BRIGHTNESS'
            is not supported by the light group).
        """
        test_cmd = "light_facade.SetGroupBrightnessValue"
        test_args = {"index": index, "values": values}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def getGroupCurrentRgbValue(self, index):
        """ Gets the current RGB values for the light group on the Fuchsia device
        under test.

        Args:
            index: uint32, index of the light group defined by board, must be less than
                   value returned by getNumLightGroups

        Returns:
            Vector of RGB objects, each indicating the RGB value of the light.
            If the capability 'RGB' is not supported by the light group, fails and prints
            error message.
            Use GetGroupInfo to check if light supports this operation.
        """
        test_cmd = "light_facade.GetGroupCurrentRgbValue"
        test_args = {"index": index}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def setGroupRgbValue(self, index, values):
        """ Sets the RGB value for a light group on the Fuchsia device
        under test.

        Args:
            index: uint32, index of the light defined by board, must be less than
                   value returned by getNumLights
            values: a vector of RGB objects, each indicating the RGB value of a light.

        Returns:
            None if success, prints error message if error (e.g. capability 'RGB'
            is not supported by the light group).
        """
        test_cmd = "light_facade.SetGroupRgbValue"
        test_args = {"index": index, "values": values}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)
