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


class FuchsiaInputReportLib(BaseLib):
    def __init__(self, addr, tc, client_id):
        self.address = addr
        self.test_counter = tc
        self.client_id = client_id

    """Gets the fuchsia.input.report.DeviceDescriptor for a given device.

    Args:
      vendor_id: the uint32 vendor ID to match against available input devices, or None to match any
        vendor ID.
      product_id: the uint32 product ID to match against available input devices, or None to match
        any product ID.
      version: the uint32 version to match against available input devices, or None to match any
        version.

    Returns:
      The DeviceDescriptor object, or an error message if an error was encountered.
    """

    def getDescriptor(self, vendor_id=None, product_id=None, version=None):
        test_cmd = 'input_report_facade.GetDescriptor'
        test_args = {
            'vendor_id': vendor_id,
            'product_id': product_id,
            'version': version
        }
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1
        return self.send_command(test_id, test_cmd, test_args)

    """Gets the list of fuchsia.input.report.InputReports that were seen since the last call.

    Args:
      vendor_id: the uint32 vendor ID to match against available input devices, or None to match any
        vendor ID.
      product_id: the uint32 product ID to match against available input devices, or None to match
        any product ID.
      version: the uint32 version to match against available input devices, or None to match any
        version.

    Returns:
      A list of InputReports, or an error message if an error was encountered.
    """

    def getReports(self, vendor_id=None, product_id=None, version=None):
        test_cmd = 'input_report_facade.GetReports'
        test_args = {
            'vendor_id': vendor_id,
            'product_id': product_id,
            'version': version
        }
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1
        return self.send_command(test_id, test_cmd, test_args)
