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


class FuchsiaRamLib(BaseLib):
    def __init__(self, addr, tc, client_id):
        self.address = addr
        self.test_counter = tc
        self.client_id = client_id

    def measureBandwidth(self, cycles_to_measure, channels):
        """ Measures the DDR bandwidth on the specified channels.

        Args:
            cycles_to_measure: How many bus cycles to perform the measurement over.
            channels: An array of 8 uint64, specifying which ports to aggregate
                      for each channel.

        Returns:
            BandwidthInfo struct, prints an error message if error.
        """
        test_cmd = "ram_facade.MeasureBandwidth"
        test_args = {
            "values": {
                "cycles_to_measure": cycles_to_measure,
                "channels": channels
            }
        }
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)

    def getDdrWindowingResults(self):
        """ Retrieves the results from the DDR Windowing tool, which runs in
            the bootloader.

        Returns:
            The register value, prints an error message if error.
        """
        test_cmd = "ram_facade.GetDdrWindowingResults"
        test_args = {}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)
