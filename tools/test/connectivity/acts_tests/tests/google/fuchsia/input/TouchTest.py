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

import time

from acts import asserts
from acts import signals
from acts.base_test import BaseTestClass
from acts.libs.proc.job import Error

TEST_TIME_SECONDS = 5
TEST_POLL_TIME_SECONDS = 0.1


class TouchTest(BaseTestClass):
    def setup_class(self):
        super().setup_class()
        self.fd = self.fuchsia_devices[0]

    def test_touch_reports(self):
        """Prints touch events for the next 5 seconds.

        This test requires a 'touch_test_params' object to be specified in the
        device config. touch_test_params will be used by the facade to find the
        touch device, and may include 'vendor_id' and 'product_id' values.
        """
        asserts.skip_if('touch_tests_params' not in self.user_params,
                        'touch_tests_params not specified in the config')

        # Make a call to the facade to get it to establish a connection to the
        # touch device. This ensures that the first second of touch events don't
        # get missed.
        result = self.fd.input_report_lib.getDescriptor(
            **self.user_params['touch_tests_params'])

        self.log.info('Printing touch events for the next %d seconds...' %
                      TEST_TIME_SECONDS)

        end_time = time.time() + TEST_TIME_SECONDS
        while time.time() < end_time:
            time.sleep(TEST_POLL_TIME_SECONDS)

            result = self.fd.input_report_lib.getReports(
                **self.user_params['touch_tests_params'])
            asserts.assert_true(result['error'] is None,
                                'GetReports failed: %s' % result['error'])

            for event in result['result']:
                contacts = event['touch']['contacts']
                touch = ', '.join([
                    '(%d, %d)' % (t['position_x'], t['position_y'])
                    for t in event['touch']['contacts']
                ]) or '(none)'
                self.log.info('touch event @ %d: %s' %
                              (event['event_time'], touch))
