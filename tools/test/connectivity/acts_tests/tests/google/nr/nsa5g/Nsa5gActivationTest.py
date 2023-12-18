#!/usr/bin/env python3.4
#
#   Copyright 2020 - Google
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
"""
    Test Script for 5G Activation scenarios
"""

import time

from acts.test_decorators import test_tracker_info
from acts_contrib.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts_contrib.test_utils.tel.tel_defines import GEN_4G
from acts_contrib.test_utils.tel.tel_defines import NETWORK_SERVICE_DATA
from acts_contrib.test_utils.tel.tel_defines import NETWORK_MODE_WCDMA_ONLY
from acts_contrib.test_utils.tel.tel_defines import WAIT_TIME_ANDROID_STATE_SETTLING
from acts_contrib.test_utils.tel.tel_test_utils import reboot_device
from acts_contrib.test_utils.tel.tel_test_utils import get_current_override_network_type
from acts_contrib.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts_contrib.test_utils.tel.tel_test_utils import wait_for_network_generation
from acts_contrib.test_utils.tel.tel_test_utils import wifi_toggle_state
from acts_contrib.test_utils.tel.tel_test_utils import set_preferred_network_mode_pref
from acts_contrib.test_utils.tel.tel_5g_utils import is_current_network_5g_nsa
from acts_contrib.test_utils.tel.tel_5g_test_utils import set_preferred_mode_for_5g


class Nsa5gActivationTest(TelephonyBaseTest):
    def setup_class(self):
        super().setup_class()

    def setup_test(self):
        TelephonyBaseTest.setup_test(self)
        self.number_of_devices = 1

    def teardown_class(self):
        TelephonyBaseTest.teardown_class(self)


    """ Tests Begin """


    @test_tracker_info(uuid="b10b3779-b535-4cac-a12f-2fb6daed55a5")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_activation_from_apm(self):
        """ Verifies 5G NSA activation from Airplane Mode

        Toggle Airplane mode on and off
        Ensure phone attach, data on, LTE attach
        Wait for 120 secs for ENDC attach
        Verify is data network type is NR_NSA

        Returns:
            True if pass; False if fail.
        """
        ad = self.android_devices[0]
        wifi_toggle_state(ad.log, ad, False)
        set_preferred_mode_for_5g(ad)
        for iteration in range(3):
            ad.log.info("Attempt %d", iteration + 1)
            # APM toggle
            toggle_airplane_mode(ad.log, ad, True)
            toggle_airplane_mode(ad.log, ad, False)
            # LTE attach
            if not wait_for_network_generation(
                    ad.log, ad, GEN_4G, voice_or_data=NETWORK_SERVICE_DATA):
                ad.log.error("Fail to ensure initial data in 4G")
            # 5G attach
            ad.log.info("Waiting for 5g NSA attach for 60 secs")
            if is_current_network_5g_nsa(ad, timeout=60):
                ad.log.info("Success! attached on 5g NSA")
                return True
            else:
                ad.log.error("Failure - expected NR_NSA, current %s",
                             get_current_override_network_type(ad))
            time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        ad.log.info("nsa5g attach test FAIL for all 3 iterations")
        return False


    @test_tracker_info(uuid="d4f5f0c5-cc58-4531-96dd-32eed9121b95")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_activation_from_reboot(self):
        """ Verifies 5G NSA activation from Reboot

        Reboot device
        Ensure phone attach, data on, LTE attach
        Wait for 120 secs for ENDC attach
        Verify is data network type is NR_NSA

        Returns:
            True if pass; False if fail.
        """
        ad = self.android_devices[0]
        wifi_toggle_state(ad.log, ad, False)
        toggle_airplane_mode(ad.log, ad, False)
        set_preferred_mode_for_5g(ad)
        for iteration in range(3):
            ad.log.info("Attempt %d", iteration + 1)
            # Reboot phone
            reboot_device(ad)
            # LTE attach
            if not wait_for_network_generation(
                    ad.log, ad, GEN_4G, voice_or_data=NETWORK_SERVICE_DATA):
                ad.log.error("Fail to ensure initial data in 4G")
            # 5G attach
            ad.log.info("Waiting for 5g NSA attach for 60 secs")
            if is_current_network_5g_nsa(ad, timeout=60):
                ad.log.info("Success! attached on 5g NSA")
                return True
            else:
                ad.log.error("Failure - expected NR_NSA, current %s",
                             get_current_override_network_type(ad))
            time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        ad.log.info("nsa5g reboot test FAIL for all 3 iterations")
        return False


    @test_tracker_info(uuid="1ceda4b5-4a6a-43fa-8976-67cbfb7eab5b")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_activation_from_3g(self):
        """ Verifies 5G NSA activation from 3G Mode Pref

        Change Mode to 3G and wait for 15 secs
        Change Mode back to 5G
        Ensure phone attach, data on, LTE attach
        Wait for 120 secs for ENDC attach
        Verify is data network type is NR_NSA

        Returns:
            True if pass; False if fail.
        """
        ad = self.android_devices[0]
        sub_id = ad.droid.subscriptionGetDefaultSubId()
        wifi_toggle_state(ad.log, ad, False)
        toggle_airplane_mode(ad.log, ad, False)
        for iteration in range(3):
            ad.log.info("Attempt %d", iteration + 1)
            # Set mode pref to 3G
            set_preferred_network_mode_pref(ad.log, ad, sub_id,
                                            NETWORK_MODE_WCDMA_ONLY)
            time.sleep(15)
            # Set mode pref to 5G
            set_preferred_mode_for_5g(ad)
            # LTE attach
            if not wait_for_network_generation(
                    ad.log, ad, GEN_4G, voice_or_data=NETWORK_SERVICE_DATA):
                ad.log.error("Fail to ensure initial data in 4G")
            # 5G attach
            ad.log.info("Waiting for 5g NSA attach for 60 secs")
            if is_current_network_5g_nsa(ad, timeout=60):
                ad.log.info("Success! attached on 5g NSA")
                return True
            else:
                ad.log.error("Failure - expected NR_NSA, current %s",
                             get_current_override_network_type(ad))
            time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        ad.log.info("nsa5g mode pref from 3G test FAIL for all 3 iterations")
        return False

    """ Tests End """
