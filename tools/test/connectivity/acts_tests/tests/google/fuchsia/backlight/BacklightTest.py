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

BRIGHTNESS_CHANGE_SLEEP_TIME_SECONDS = 2

ONE_QUARTER_BRIGHTNESS = 0.25
HALF_BRIGHTNESS = 0.5
THREE_QUARTERS_BRIGHTNESS = 0.75
FULL_BRIGHTNESS = 1.0


def float_near(a, b):
    return abs(a - b) < 0.001


class BacklightTest(BaseTestClass):
    def setup_class(self):
        super().setup_class()
        self.fd = self.fuchsia_devices[0]

    def setup_test(self):
        # Save the state and scale so that they can be restored after the test. Set a known initial
        # brightness so that the test effects are consistent.
        self.initial_state = self.fd.backlight_lib.getStateNormalized(
        )['result']
        self.initial_scale = self.fd.backlight_lib.getNormalizedBrightnessScale(
        )['result']

        self.fd.backlight_lib.setStateNormalized(True, HALF_BRIGHTNESS)

    def teardown_test(self):
        self.fd.backlight_lib.setNormalizedBrightnessScale(self.initial_scale)

        backlight_on = self.initial_state['backlight_on']
        brightness = self.initial_state['brightness']
        self.fd.backlight_lib.setStateNormalized(backlight_on,
                                                 brightness * HALF_BRIGHTNESS)
        self.fd.backlight_lib.setStateNormalized(backlight_on, brightness)

    def test_brightness(self):
        """Test SetStateNormalized and GetStateNormalized FIDL calls.

        The display brightness should decrease, increase, turn off, then turn back on again. This
        test will only run if the device config has a 'backlight_tests' entry.
        """
        asserts.skip_if('backlight_tests' not in self.user_params,
                        'backlight_tests not specified in the config')

        # Pause to make backlight brightness changes clearly visible.
        time.sleep(BRIGHTNESS_CHANGE_SLEEP_TIME_SECONDS)

        result = self.fd.backlight_lib.setStateNormalized(
            True, ONE_QUARTER_BRIGHTNESS)
        asserts.assert_true(result['error'] is None,
                            'SetStateNormalized failed')

        result = self.fd.backlight_lib.getStateNormalized()
        asserts.assert_true(result['error'] is None,
                            'GetStateNormalized failed')

        asserts.assert_true(result['result']['backlight_on'],
                            'Got unexpected device state')
        asserts.assert_true(
            float_near(result['result']['brightness'], ONE_QUARTER_BRIGHTNESS),
            'Got unexpected brightness value')

        time.sleep(BRIGHTNESS_CHANGE_SLEEP_TIME_SECONDS)

        # Brightness should increase
        result = self.fd.backlight_lib.setStateNormalized(
            True, THREE_QUARTERS_BRIGHTNESS)
        asserts.assert_true(result['error'] is None,
                            'SetStateNormalized failed')

        result = self.fd.backlight_lib.getStateNormalized()
        asserts.assert_true(result['error'] is None,
                            'GetStateNormalized failed')

        asserts.assert_true(result['result']['backlight_on'],
                            'Got unexpected device state')
        asserts.assert_true(
            float_near(result['result']['brightness'],
                       THREE_QUARTERS_BRIGHTNESS),
            'Got unexpected brightness value')

        time.sleep(BRIGHTNESS_CHANGE_SLEEP_TIME_SECONDS)

        # Backlight should turn off
        result = self.fd.backlight_lib.setStateNormalized(
            False, FULL_BRIGHTNESS)
        asserts.assert_true(result['error'] is None,
                            'SetStateNormalized failed')

        result = self.fd.backlight_lib.getStateNormalized()
        asserts.assert_true(result['error'] is None,
                            'GetStateNormalized failed')

        asserts.assert_false(result['result']['backlight_on'],
                             'Got unexpected device state')
        asserts.assert_true(
            float_near(result['result']['brightness'], FULL_BRIGHTNESS),
            'Got unexpected brightness value')

    def test_brightness_scale(self):
        """Test SetNormalizedBrightnessScale and SetNormalizedBrightnessScale FIDL calls.

        The display brightness should decrease then increase. This test will only run if the device
        config has a 'backlight_tests' entry.
        """
        asserts.skip_if('backlight_tests' not in self.user_params,
                        'backlight_tests not specified in the config')

        time.sleep(BRIGHTNESS_CHANGE_SLEEP_TIME_SECONDS)

        # Brightness should decrease
        result = self.fd.backlight_lib.setNormalizedBrightnessScale(
            self.initial_scale * ONE_QUARTER_BRIGHTNESS)
        asserts.assert_true(result['error'] is None,
                            'SetNormalizedBrightnessScale failed')

        result = self.fd.backlight_lib.getNormalizedBrightnessScale()
        asserts.assert_true(result['error'] is None,
                            'GetNormalizedBrightnessScale failed')

        asserts.assert_true(
            float_near(result['result'],
                       self.initial_scale * ONE_QUARTER_BRIGHTNESS),
            'Got unexpected brightness scale value')

        # Toggle the brightness to force an update. This works around a hardware limitation.
        self.fd.backlight_lib.setStateNormalized(True, ONE_QUARTER_BRIGHTNESS)
        self.fd.backlight_lib.setStateNormalized(True, HALF_BRIGHTNESS)

        time.sleep(BRIGHTNESS_CHANGE_SLEEP_TIME_SECONDS)

        # Brightness should increase
        result = self.fd.backlight_lib.setNormalizedBrightnessScale(
            self.initial_scale)
        asserts.assert_true(result['error'] is None,
                            'SetNormalizedBrightnessScale failed')

        result = self.fd.backlight_lib.getNormalizedBrightnessScale()
        asserts.assert_true(result['error'] is None,
                            'GetNormalizedBrightnessScale failed')

        asserts.assert_true(float_near(result['result'], self.initial_scale),
                            'Got unexpected brightness scale value')

        self.fd.backlight_lib.setStateNormalized(True, ONE_QUARTER_BRIGHTNESS)
        self.fd.backlight_lib.setStateNormalized(True, HALF_BRIGHTNESS)
