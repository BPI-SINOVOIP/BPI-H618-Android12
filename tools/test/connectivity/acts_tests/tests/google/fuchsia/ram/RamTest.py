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

import os
import time
import uuid

from acts import asserts, signals
from acts.base_test import BaseTestClass
from acts.libs.proc.job import Error
from acts_contrib.test_utils.tel.tel_test_utils import setup_droid_properties

# Some number bigger than the minimum 512k:
MEMORY_CYCLES_TO_MEASURE = 1024 * 1024
# Taken from the commandline tool.
#TODO(48254): Convert driver to fill in these values automatically.
MEMORY_CHANNELS = [1, 2, 0x600110, 0x1F0000, 0, 0, 0, 0]


class RamTest(BaseTestClass):
    def setup_class(self):
        super().setup_class()
        self.fd = self.fuchsia_devices[0]

    def test_bandwidth(self):
        """Test MeasureBandwidth FIDL calls.

        There should be no noticable effect. This test will only run
        if the device config has a 'ram_tests' entry.
        """
        asserts.skip_if('ram_tests' not in self.user_params,
                        'ram_tests not specified in the config')

        result = self.fd.ram_lib.measureBandwidth(MEMORY_CYCLES_TO_MEASURE,
                                                  MEMORY_CHANNELS)
        asserts.assert_true(
            result['error'] is None,
            "MeasureBandwidth failed with: {}".format(result['error']))

    def test_get_ddr_results(self):
        """Test GetDdrWindowingResults FIDL calls.

        There should be no noticable effect. This test will only run
        if the device config has a 'ram_tests' entry.
        """
        asserts.skip_if('ram_tests' not in self.user_params,
                        'ram_tests not specified in the config')

        result = self.fd.ram_lib.getDdrWindowingResults()
        asserts.assert_true(
            result['error'] is None,
            "GetDdrWindowingResults failed with: {}".format(result['error']))
