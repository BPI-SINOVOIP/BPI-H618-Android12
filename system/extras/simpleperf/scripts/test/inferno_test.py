#!/usr/bin/env python3
#
# Copyright (C) 2021 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import collections
import json
from typing import Any, Dict, List

from . test_utils import INFERNO_SCRIPT, TestBase, TestHelper


class TestInferno(TestBase):
    def get_report(self, options: List[str]) -> str:
        self.run_cmd([INFERNO_SCRIPT] + options)
        with open('report.html', 'r') as fh:
            return fh.read()

    def test_proguard_mapping_file(self):
        """ Test --proguard-mapping-file option. """
        testdata_file = TestHelper.testdata_path('perf_need_proguard_mapping.data')
        proguard_mapping_file = TestHelper.testdata_path('proguard_mapping.txt')
        original_methodname = 'androidx.fragment.app.FragmentActivity.startActivityForResult'
        # Can't show original method name without proguard mapping file.
        self.assertNotIn(original_methodname, self.get_report(
            ['--record_file', testdata_file, '-sc']))
        # Show original method name with proguard mapping file.
        self.assertIn(original_methodname, self.get_report(
            ['--record_file', testdata_file, '-sc', '--proguard-mapping-file', proguard_mapping_file]))
