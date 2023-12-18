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
    Test Script for 5G MMS scenarios
"""

import time

from acts.test_decorators import test_tracker_info
from acts_contrib.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts_contrib.test_utils.tel.tel_defines import WAIT_TIME_ANDROID_STATE_SETTLING
from acts_contrib.test_utils.tel.tel_defines import SMS_OVER_WIFI_PROVIDERS
from acts_contrib.test_utils.tel.tel_test_utils import call_setup_teardown
from acts_contrib.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts_contrib.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts_contrib.test_utils.tel.tel_test_utils import ensure_phone_default_state
from acts_contrib.test_utils.tel.tel_test_utils import multithread_func
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_iwlan
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_csfb
from acts_contrib.test_utils.tel.tel_5g_test_utils import connect_both_devices_to_wifi
from acts_contrib.test_utils.tel.tel_5g_test_utils import disable_apm_mode_both_devices
from acts_contrib.test_utils.tel.tel_5g_test_utils import provision_device_for_5g
from acts_contrib.test_utils.tel.tel_5g_test_utils import provision_both_devices_for_volte
from acts_contrib.test_utils.tel.tel_5g_test_utils import provision_both_devices_for_wfc_cell_pref
from acts_contrib.test_utils.tel.tel_5g_test_utils import provision_both_devices_for_wfc_wifi_pref
from acts_contrib.test_utils.tel.tel_5g_test_utils import verify_5g_attach_for_both_devices
from acts_contrib.test_utils.tel.tel_5g_test_utils import provision_both_devices_for_csfb
from acts_contrib.test_utils.tel.tel_5g_utils import is_current_network_5g_nsa
from acts_contrib.test_utils.tel.tel_mms_utils import _mms_test_mo
from acts_contrib.test_utils.tel.tel_mms_utils import _mms_test_mt
from acts_contrib.test_utils.tel.tel_mms_utils import _long_mms_test_mo
from acts_contrib.test_utils.tel.tel_mms_utils import test_mms_mo_in_call


class Nsa5gMmsTest(TelephonyBaseTest):
    def setup_class(self):
        super().setup_class()
        self.number_of_devices = 2
        self.message_lengths = (50, 160, 180)

        is_roaming = False
        for ad in self.android_devices:
            ad.sms_over_wifi = False
            # verizon supports sms over wifi. will add more carriers later
            for sub in ad.telephony["subscription"].values():
                if sub["operator"] in SMS_OVER_WIFI_PROVIDERS:
                    ad.sms_over_wifi = True
            if getattr(ad, 'roaming', False):
                is_roaming = True
        if is_roaming:
            # roaming device does not allow message of length 180
            self.message_lengths = (50, 160)

    def setup_test(self):
        TelephonyBaseTest.setup_test(self)

    def teardown_test(self):
        ensure_phones_idle(self.log, self.android_devices)


    """ Tests Begin """


    @test_tracker_info(uuid="bc484c2c-8086-42db-94cd-a1e4a35f35cf")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_mms_mo_mt(self):
        """Test MMS between two phones in 5g NSA

        Provision devices in 5g NSA
        Send and Verify MMS from PhoneA to PhoneB
        Verify both devices are still on 5g NSA

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not provision_device_for_5g(self.log, ads):
            return False

        if not _mms_test_mo(self.log, ads):
            return False

        if not verify_5g_attach_for_both_devices(self.log, ads):
            return False

        self.log.info("PASS - mms test over 5g nsa validated")
        return True


    @test_tracker_info(uuid="51d42104-cb87-4c9b-9a16-302e246a21dc")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_mms_mo_mt_volte(self):
        """Test MMS between two phones with VoLTE on 5G NSA

        Provision devices on VoLTE
        Provision devices in 5g NSA
        Send and Verify MMS from PhoneA to PhoneB
        Verify both devices are still on 5g NSA

        Returns:
            True if success.
            False if failed.
        """

        ads = self.android_devices
        if not provision_both_devices_for_volte(self.log, ads):
            return False

        if not provision_device_for_5g(self.log, ads):
            return False

        if not _mms_test_mo(self.log, ads):
            return False

        if not verify_5g_attach_for_both_devices(self.log, ads):
            return False

        self.log.info("PASS - volte mms test over 5g nsa validated")
        return True


    @test_tracker_info(uuid="97d6b071-aef2-40c1-8245-7be6c31870a6")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_mms_mo_mt_in_call_volte(self):
        """ Test MO MMS during a VoLTE call over 5G NSA.

        Provision devices on VoLTE
        Provision devices in 5g NSA
        Make a Voice call from PhoneA to PhoneB
        Send and Verify MMS from PhoneA to PhoneB
        Verify both devices are still on 5g NSA

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        if not provision_both_devices_for_volte(self.log, ads):
            return False

        if not provision_device_for_5g(self.log, ads):
            return False

        self.log.info("Begin Incall mms test.")
        if not call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                ad_hangup=None,
                verify_caller_func=is_phone_in_call_volte,
                verify_callee_func=None):
            return False

        if not _mms_test_mo(self.log, ads):
            return False

        if not verify_5g_attach_for_both_devices(self.log, ads):
            return False
        self.log.info("PASS - Incall volte mms test over 5g nsa validated")
        return True


    @test_tracker_info(uuid="bbb4b80c-fc1b-4377-b3c7-eeed642c5980")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_mms_mo_mt_iwlan(self):
        """ Test MMS text function between two phones,
        Phones in APM, WiFi connected, WFC Cell Preferred mode.

        Disable APM on both devices
        Provision devices in 5g NSA
        Provision devices for WFC Cell Pref with APM ON
        Send and Verify MMS from PhoneA to PhoneB

        Returns:
            True if pass; False if fail.
        """

        ads = self.android_devices
        if not disable_apm_mode_both_devices(self.log, ads):
            return False

        if not provision_device_for_5g(self.log, ads):
            return False

        if not provision_both_devices_for_wfc_cell_pref(self.log,
                                                        ads,
                                                        self.wifi_network_ssid,
                                                        self.wifi_network_pass,
                                                        apm_mode=True):
            return False
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        if not _mms_test_mo(self.log, ads):
            return False

        self.log.info("PASS - iwlan mms test over 5g nsa validated")
        return True


    @test_tracker_info(uuid="d36d95dc-0973-4711-bb08-c29ce23495e4")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_mms_mo_mt_iwlan_apm_off(self):
        """ Test MO MMS, Phone in APM off, WiFi connected, WFC WiFi Pref Mode

        Disable APM on both devices
        Provision devices in 5g NSA
        Provision devices for WFC Wifi Pref with APM OFF
        Send and Verify MMS from PhoneA to PhoneB
        Verify 5g NSA attach for both devices

        Returns:
            True if pass; False if fail.
        """

        ads = self.android_devices
        if not disable_apm_mode_both_devices(self.log, ads):
            return False

        if not provision_device_for_5g(self.log, ads):
            return False

        if not provision_both_devices_for_wfc_wifi_pref(self.log,
                                                        ads,
                                                        self.wifi_network_ssid,
                                                        self.wifi_network_pass,
                                                        apm_mode=False):
            return False
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        if not _mms_test_mo(self.log, ads):
            self.log.error("Failed to send receive sms over 5g nsa")
            return False
        self.log.info("PASS - iwlan mms test over 5g nsa validated")

        if not verify_5g_attach_for_both_devices(self.log, ads):
            return False
        return True


    @test_tracker_info(uuid="74ffb79e-f1e9-4087-a9d2-e07878e47869")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_mms_mo_mt_in_call_iwlan(self):
        """ Test MO MMS, Phone in APM, WiFi connected, WFC WiFi Pref mode

        Disable APM on both devices
        Provision devices in 5g NSA
        Provision devices for WFC Wifi Pref with APM ON
        Make a Voice call from PhoneA to PhoneB
        Send and Verify MMS from PhoneA to PhoneB

        Returns:
            True if pass; False if fail.
        """

        ads = self.android_devices

        if not disable_apm_mode_both_devices(self.log, ads):
            return False

        if not provision_device_for_5g(self.log, ads):
            return False

        if not provision_both_devices_for_wfc_wifi_pref(self.log,
                                                        ads,
                                                        self.wifi_network_ssid,
                                                        self.wifi_network_pass,
                                                        apm_mode=True):
            return False
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        self.log.info("Begin Incall mms test")
        if not call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                ad_hangup=None,
                verify_caller_func=is_phone_in_call_iwlan,
                verify_callee_func=None):
            return False

        return _mms_test_mo(self.log, ads)


    @test_tracker_info(uuid="68c8e0ca-bea4-45e4-92cf-19424ee47ca4")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_mms_mo_mt_in_call_volte_wifi(self):
        """ Test MMS during VoLTE call and WiFi connected

        Make sure PhoneA/B are in 5G NSA (with VoLTE).
        Make sure PhoneA/B are able to make/receive call.
        Connect PhoneA/B to Wifi.
        Call from PhoneA to PhoneB, accept on PhoneB, send MMS on PhoneA.
        Make sure PhoneA/B are in 5G NSA.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        if not provision_both_devices_for_volte(self.log, ads):
            return False

        if not provision_device_for_5g(self.log, ads):
            return False

        if not connect_both_devices_to_wifi(self.log,
                                            ads,
                                            self.wifi_network_ssid,
                                            self.wifi_network_pass):
            return False

        self.log.info("Begin In Call MMS Test.")
        if not call_setup_teardown(
                self.log,
                ads[0],
                ads[1],
                ad_hangup=None,
                verify_caller_func=is_phone_in_call_volte,
                verify_callee_func=None):
            return False

        if not _mms_test_mo(self.log, ads):
            return False

        if not verify_5g_attach_for_both_devices(self.log, ads):
            return False
        return True


    @test_tracker_info(uuid="8c795c3a-59d4-408c-9b99-5287e79ba00b")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_mms_long_message_mo_mt(self):
        """Test MMS basic function between two phone. Phones in nsa 5G network.

        Airplane mode is off. Phone in nsa 5G.
        Send MMS from PhoneA to PhoneB.
        Verify received message on PhoneB is correct.

        Returns:
            True if success.
            False if failed.
        """

        ads = self.android_devices

        if not disable_apm_mode_both_devices(self.log, ads):
            return False

        if not provision_device_for_5g(self.log, ads):
            return False

        return _long_mms_test_mo(self.log, ads)


    @test_tracker_info(uuid="e09b82ab-69a9-4eae-8cbe-b6f2cff993ad")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_mms_mo_wifi(self):
        """Test MMS basic function between two phone. Phones in nsa 5g network.

        Airplane mode is off. Phone in nsa 5G.
        Connect to Wifi.
        Send MMS from PhoneA to PhoneB.
        Verify received message on PhoneB is correct.

        Returns:
            True if success.
            False if failed.
        """

        ads = self.android_devices

        if not disable_apm_mode_both_devices(self.log, ads):
            return False

        if not provision_device_for_5g(self.log, ads):
            return False

        ensure_wifi_connected(self.log, ads[0], self.wifi_network_ssid,
                              self.wifi_network_pass)

        return _mms_test_mo(self.log, ads)


    @test_tracker_info(uuid="fedae24f-2577-4f84-9d76-53bbbe109d48")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_mms_mt_wifi(self):
        """Test MMS basic function between two phone. Phones in nsa 5g network.

        Airplane mode is off. Phone in nsa 5G.
        Connect to Wifi.
        Send MMS from PhoneB to PhoneA.
        Verify received message on PhoneA is correct.

        Returns:
            True if success.
            False if failed.
        """

        ads = self.android_devices

        if not disable_apm_mode_both_devices(self.log, ads):
            return False

        if not provision_device_for_5g(self.log, ads):
            return False

        ensure_wifi_connected(self.log, ads[0], self.wifi_network_ssid,
                              self.wifi_network_pass)

        return _mms_test_mt(self.log, ads)


    @test_tracker_info(uuid="156bf832-acc2-4729-a69d-b471cd5cfbde")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_mms_mo_mt_in_call_csfb_wifi(self):
        """ Test MO/MT MMS during a MO csfb call and devices connect to Wifi.

        Disable APM on both devices
        Set up PhoneA/PhoneB are in CSFB mode.
        Provision PhoneA/B in 5g NSA.
        Make sure PhoneA/B is able to make/receive call.
        Connect PhoneA/B to Wifi.
        Call from PhoneA to PhoneB, accept on PhoneB, send MMS on PhoneA,
         receive MMS on B.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        if not disable_apm_mode_both_devices(self.log, ads):
            return False

        if not provision_both_devices_for_csfb(self.log, ads):
            return False
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        if not provision_both_devices_for_5g(self.log, ads):
            return False

        if not connect_both_devices_to_wifi(self.log,
                                            ads,
                                            self.wifi_network_ssid,
                                            self.wifi_network_pass):
            return False
        if not test_mms_mo_in_call(self.log,
                                   ads,
                                   wifi=True,
                                   caller_func=is_phone_in_call_csfb):
            return False


    @test_tracker_info(uuid="88bd6658-30fa-41b1-b5d9-0f9dadd83219")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_mms_mo_general(self):
        """Test MO MMS for 1 phone in 5g NSA. The other phone in any network

        Provision PhoneA in 5g NSA
        Send and Verify MMS from PhoneA to PhoneB
        Verify phoneA is still on 5g NSA

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        tasks = [(provision_device_for_5g, (self.log, ads[0])),
                 (ensure_phone_default_state, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            return Fals

        if not _mms_test_mo(self.log, ads):
            return False

        if not is_current_network_5g_nsa(ads[0]):
            return False

        self.log.info("PASS - MO mms test over 5g nsa validated")
        return True


    @test_tracker_info(uuid="11f2e2c8-bb63-43fa-b279-e7bb32f80596")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_mms_mt_general(self):
        """Test MT MMS for 1 phone in 5g NSA. The other phone in any network

        Provision PhoneA in 5g NSA
        Send and Verify MMS from PhoneB to PhoneA
        Verify phoneA is still on 5g NSA

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        tasks = [(provision_device_for_5g, (self.log, ads[0])),
                 (ensure_phone_default_state, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            return False

        if not _mms_test_mt(self.log, ads):
            return False

        if not is_current_network_5g_nsa(ads[0]):
            return False

        self.log.info("PASS - MT mms test over 5g nsa validated")
        return True

    """ Tests End """
