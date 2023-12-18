#!/usr/bin/env python3
#
#   Copyright 2021 - The Android Open Source Project
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

import sys
import collections
import random
import time
import datetime
import os
import logging
import json
import subprocess
import math
import re

from acts import asserts
from acts.test_decorators import test_tracker_info

from acts.base_test import BaseTestClass
from acts_contrib.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts_contrib.test_utils.tel.GFTInOutBaseTest import GFTInOutBaseTest


from acts_contrib.test_utils.tel.tel_test_utils import set_wfc_mode
from acts_contrib.test_utils.tel.tel_test_utils import toggle_wfc

from acts_contrib.test_utils.tel.tel_test_utils import multithread_func
from acts_contrib.test_utils.tel.tel_test_utils import run_multithread_func
from acts_contrib.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts_contrib.test_utils.tel.tel_test_utils import get_screen_shot_log
from acts_contrib.test_utils.tel.tel_test_utils import get_screen_shot_logs
from acts_contrib.test_utils.tel.tel_test_utils import log_screen_shot
from acts_contrib.test_utils.tel.tel_test_utils import hangup_call

from acts_contrib.test_utils.tel.gft_inout_utils import check_no_service_time
from acts_contrib.test_utils.tel.gft_inout_utils import check_back_to_service_time
from acts_contrib.test_utils.tel.gft_inout_utils import mo_voice_call
from acts_contrib.test_utils.tel.gft_inout_utils import get_voice_call_type

from acts_contrib.test_utils.tel.tel_defines import WFC_MODE_WIFI_ONLY
from acts_contrib.test_utils.tel.tel_defines import WFC_MODE_CELLULAR_PREFERRED
from acts_contrib.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts_contrib.test_utils.tel.tel_defines import WFC_MODE_DISABLED

from acts_contrib.test_utils.tel.gft_inout_defines import VOICE_CALL
from acts_contrib.test_utils.tel.gft_inout_defines import VOLTE_CALL
from acts_contrib.test_utils.tel.gft_inout_defines import CSFB_CALL
from acts_contrib.test_utils.tel.gft_inout_defines import WFC_CALL
from acts_contrib.test_utils.tel.gft_inout_defines import NO_SERVICE_POWER_LEVEL
from acts_contrib.test_utils.tel.gft_inout_defines import IN_SERVICE_POWER_LEVEL
from acts_contrib.test_utils.tel.gft_inout_defines import NO_SERVICE_AREA
from acts_contrib.test_utils.tel.gft_inout_defines import IN_SERVICE_AREA
from acts_contrib.test_utils.tel.gft_inout_defines import WIFI_AREA
from acts_contrib.test_utils.tel.gft_inout_defines import NO_WIFI_AREA
from acts_contrib.test_utils.tel.gft_inout_defines import NO_SERVICE_TIME
from acts_contrib.test_utils.tel.gft_inout_defines import WAIT_FOR_SERVICE_TIME

WAIT_TIME_AT_NO_SERVICE_AREA = 300

class TelLabGFTVoWifiTest(GFTInOutBaseTest):
    def __init__(self, controllers):
        GFTInOutBaseTest.__init__(self, controllers)
        self.wifi_ssid = self.user_params.get('wifi_network_ssid')
        self.wifi_pw = self.user_params.get('wifi_network_pw')

    def setup_test(self):
        self.adjust_cellular_signal(IN_SERVICE_POWER_LEVEL)
        self.adjust_wifi_signal(IN_SERVICE_POWER_LEVEL)
        GFTInOutBaseTest.setup_test(self)
        tasks = [(toggle_wfc, (self.log, ad,True)) for ad in self.android_devices]
        if not multithread_func(self.log, tasks):
            msg = "device does not support WFC! Skip test"
            self.log.info(msg)
            asserts.skip(msg)
        for ad in self.android_devices:
            log_screen_shot(ad, self.test_name)

    @test_tracker_info(uuid="c0e74803-44ac-4a6b-be7e-2d1337ee4521")
    @TelephonyBaseTest.tel_test_wrap
    def test_wfc_in_out_wifi(self, loop=1, wfc_mode=WFC_MODE_WIFI_PREFERRED):
        """
            Enable Wi-Fi calling in Wi-Fi Preferred mode and connect to a
            valid Wi-Fi AP. Test VoWiFi call under WiFi and cellular area
            -> move to WiFi only area -> move to Cellular only area

            Args:
                loop: repeat this test cases for how many times
                wfc_mode: wfc mode

            Returns:
                True if pass; False if fail
        """
        test_result = True
        if 'wfc_cycle' in self.user_params:
            loop = self.user_params.get('wfc_cycle')

        for x in range (loop):
            self.log.info("%s loop: %s/%s" %(self.current_test_name, x+1, loop))
            self.log.info("Start test at cellular and wifi area")
            self.adjust_cellular_signal(IN_SERVICE_POWER_LEVEL)
            self.adjust_wifi_signal(IN_SERVICE_POWER_LEVEL)
            self.check_network()
            if self._enable_wifi_calling(wfc_mode):
                if not self._voice_call(self.android_devices, WFC_CALL, False):
                    self.log.info("VoWiFi call failure")
                    return False
                self.log.info("Move to no service area and wifi area")
                self.adjust_cellular_signal(NO_SERVICE_POWER_LEVEL)
                time.sleep(WAIT_TIME_AT_NO_SERVICE_AREA)
                # check call status
                for ad in self.android_devices:
                    get_voice_call_type(ad)
                self.log.info("Move back to service area and no wifi area")
                self.adjust_cellular_signal(IN_SERVICE_POWER_LEVEL)
                self.adjust_wifi_signal(NO_SERVICE_POWER_LEVEL)
            self.log.info("Verify device state after in-out service")
            tasks = [(check_back_to_service_time, (ad,)) for ad in self.android_devices]
            test_result = multithread_func(self.log, tasks)
            if test_result:
                test_result = self._voice_call(self.android_devices, VOICE_CALL)
            else:
                self.log.info("device is not back to service")
        return test_result

    def _enable_wifi_calling(self, wfc_mode):
        """ Enable Wi-Fi calling in Wi-Fi Preferred mode and connect to a
            valid Wi-Fi AP.

            Args:
                wfc_mode: wfc mode

            Returns:
                True if pass; False if fail.
        """
        self.log.info("Move in WiFi area and set WFC mode to %s" %(wfc_mode))
        self.adjust_wifi_signal(IN_SERVICE_POWER_LEVEL)
        time.sleep(10)
        tasks = [(set_wfc_mode, (self.log, ad, wfc_mode)) for ad in self.android_devices]
        if not multithread_func(self.log, tasks):
            self.log.error("fail to setup WFC mode %s"  %(wfc_mode))
            return False
        tasks = [(ensure_wifi_connected, (self.log, ad, self.wifi_ssid,
            self.wifi_pw)) for ad in self.android_devices]
        if not multithread_func(self.log, tasks):
            self.log.error("phone failed to connect to wifi.")
            return False
        return True

    def _voice_call(self, ads, call_type, end_call=True, talk_time=30):
        """ Enable Wi-Fi calling in Wi-Fi Preferred mode and connect to a
            valid Wi-Fi AP.

            Args:
                ads: android devices
                call_type: WFC call, VOLTE call. CSFB call, voice call
                end_call: hangup call after voice call flag
                talk_time: in call duration in sec

            Returns:
                True if pass; False if fail.
        """
        tasks = [(mo_voice_call, (self.log, ad, call_type, end_call, talk_time)) for ad in self.android_devices]
        return multithread_func(self.log, tasks)