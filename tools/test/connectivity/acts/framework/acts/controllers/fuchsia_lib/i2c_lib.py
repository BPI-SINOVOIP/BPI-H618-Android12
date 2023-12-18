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


class FuchsiaI2cLib(BaseLib):
    def __init__(self, addr, tc, client_id):
        self.address = addr
        self.test_counter = tc
        self.client_id = client_id

    def transfer(self, device_idx, segments_is_write, write_segments_data,
                 read_segments_length):
        """Gets the fuchsia.input.report.DeviceDescriptor for a given device.

        Args:
          device_idx: the integer device index to use, e.g. 6 for /dev/class/i2c/006.
          segments_is_write: a list of bools specifying whether each segment is a read or a write.
          write_segments_data: a list of write segments, where each segment is a list of bytes to write.
          read_segments_length: a list of integers specifying the number of bytes in each read segment.

        Returns:
          The list of read segments received, or an error message if an error was encountered.
        """
        test_cmd = "i2c_facade.Transfer"
        test_args = {
            "device_idx": device_idx,
            "segments_is_write": segments_is_write,
            "write_segments_data": write_segments_data,
            "read_segments_length": read_segments_length
        }
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)
