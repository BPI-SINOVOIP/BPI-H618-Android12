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
    Test Script for 5G SMS scenarios
"""

import time
from acts.utils import rand_ascii_str
from acts.test_decorators import test_tracker_info
from acts_contrib.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts_contrib.test_utils.tel.tel_defines import WAIT_TIME_ANDROID_STATE_SETTLING
from acts_contrib.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts_contrib.test_utils.tel.tel_defines import WFC_MODE_CELLULAR_PREFERRED
from acts_contrib.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts_contrib.test_utils.tel.tel_test_utils import hangup_call
from acts_contrib.test_utils.tel.tel_test_utils import ensure_phone_default_state
from acts_contrib.test_utils.tel.tel_test_utils import multithread_func
from acts_contrib.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_iwlan
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_csfb
from acts_contrib.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts_contrib.test_utils.tel.tel_voice_utils import phone_setup_iwlan
from acts_contrib.test_utils.tel.tel_5g_test_utils import disable_apm_mode_both_devices
from acts_contrib.test_utils.tel.tel_5g_test_utils import provision_device_for_5g
from acts_contrib.test_utils.tel.tel_5g_test_utils import provision_both_devices_for_volte
from acts_contrib.test_utils.tel.tel_5g_test_utils import provision_both_devices_for_wfc_cell_pref
from acts_contrib.test_utils.tel.tel_5g_test_utils import provision_both_devices_for_wfc_wifi_pref
from acts_contrib.test_utils.tel.tel_5g_test_utils import verify_5g_attach_for_both_devices
from acts_contrib.test_utils.tel.tel_5g_test_utils import provision_both_devices_for_csfb
from acts_contrib.test_utils.tel.tel_5g_test_utils import provision_device_for_5g_nsa
from acts_contrib.test_utils.tel.tel_5g_utils import is_current_network_5g_nsa
from acts_contrib.test_utils.tel.tel_sms_utils import _sms_test_mo
from acts_contrib.test_utils.tel.tel_sms_utils import _sms_test_mt
from acts_contrib.test_utils.tel.tel_sms_utils import _long_sms_test_mo
from acts_contrib.test_utils.tel.tel_sms_utils import test_sms_mo_in_call


class Nsa5gSmsTest(TelephonyBaseTest):
    def setup_class(self):
        super().setup_class()

    def setup_test(self):
        TelephonyBaseTest.setup_test(self)

    def teardown_test(self):
        ensure_phones_idle(self.log, self.android_devices)


    """ Tests Begin """


    @test_tracker_info(uuid="4a64a262-7433-4a7f-b5c6-a36ff60aeaa2")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_mo_mt(self):
        """Test SMS between two phones in 5g NSA

        Provision devices in 5g NSA
        Send and Verify SMS from PhoneA to PhoneB
        Verify both devices are still on 5g NSA

        Returns:
            True if success.
            False if failed.
        """
        ads = self.android_devices
        if not provision_device_for_5g(self.log, ads):
            return False

        if not _sms_test_mo(self.log, ads):
            return False

        if not verify_5g_attach_for_both_devices(self.log, ads):
            return False

        self.log.info("PASS - SMS test over 5G NSA validated")
        return True


    @test_tracker_info(uuid="52b16764-0c9e-45c0-910f-a39d17c7cf7e")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_mo_general(self):
        """Test MO SMS for 1 phone in 5g NSA. The other phone in any network

        Provision PhoneA in 5g NSA
        Send and Verify SMS from PhoneA to PhoneB
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

        if not _sms_test_mo(self.log, ads):
            return False

        if not is_current_network_5g_nsa(ads[0]):
            return False

        self.log.info("PASS - MO SMS test over 5G NSA validated")
        return True


    @test_tracker_info(uuid="e9b2494a-0e40-449c-b877-1e4ddc78c536")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_mt_general(self):
        """Test MT SMS for 1 phone in 5g NSA. The other phone in any network

        Provision PhoneA in 5g NSA
        Send and Verify SMS from PhoneB to PhoneA
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

        if not _sms_test_mt(self.log, ads):
            return False

        if not is_current_network_5g_nsa(ads[0]):
            return False

        self.log.info("PASS - MT SMS test over 5G NSA validated")
        return True


    @test_tracker_info(uuid="2ce809d4-cbf6-4233-81ad-43f91107b201")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_mo_mt_volte(self):
        """Test SMS between two phones with VoLTE on 5G NSA

        Provision devices on VoLTE
        Provision devices in 5g NSA
        Send and Verify SMS from PhoneA to PhoneB
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

        if not _sms_test_mo(self.log, ads):
            return False

        if not hangup_call(self.log, ads[0]):
            ads[0].log.info("Failed to hang up call.!")
            return False

        if not verify_5g_attach_for_both_devices(self.log, ads):
            return False

        self.log.info("PASS - VoLTE SMS test over 5G NSA validated")
        return True


    @test_tracker_info(uuid="e51f3dbb-bb16-4400-b2be-f9422f511087")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_mo_volte(self):
        """Test MO SMS with VoLTE on 5G NSA. The other phone in any network

        Provision PhoneA on VoLTE
        Provision PhoneA in 5g NSA
        Send and Verify SMS from PhoneA to PhoneB
        Verify PhoneA is still on 5g NSA

        Returns:
            True if success.
            False if failed.
        """

        ads = self.android_devices
        if not phone_setup_volte(self.log, ads[0]):
            return False

        tasks = [(provision_device_for_5g, (self.log, ads[0])),
                 (ensure_phone_default_state, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            return False

        if not _sms_test_mo(self.log, ads):
            return False

        if not is_current_network_5g_nsa(ads[0]):
            return False

        self.log.info("PASS - MO VoLTE SMS test over 5G NSA validated")
        return True


    @test_tracker_info(uuid="5217d427-04a2-4b2b-9ed8-28951e71fc21")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_mt_volte(self):
        """Test MT SMS with VoLTE on 5G NSA. The other phone in any network

        Provision PhoneA on VoLTE
        Provision PhoneA in 5g NSA
        Send and Verify SMS from PhoneB to PhoneA
        Verify phoneA is still on 5g NSA

        Returns:
            True if success.
            False if failed.
        """

        ads = self.android_devices
        if not phone_setup_volte(self.log, ads[0]):
            return False

        tasks = [(provision_device_for_5g, (self.log, ads[0])),
                 (ensure_phone_default_state, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            return False

        if not _sms_test_mt(self.log, ads):
            return False

        if not is_current_network_5g_nsa(ads[0]):
            return False

        self.log.info("PASS - MT VoLTE SMS test over 5G NSA validated")
        return True


    @test_tracker_info(uuid="49bfb4b3-a6ec-45d4-ad96-09282fb07d1d")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_mo_mt_in_call_volte(self):
        """ Test MO SMS during a MO VoLTE call over 5G NSA.

        Provision devices on VoLTE
        Provision devices in 5g NSA
        Make a Voice call from PhoneA to PhoneB
        Send and Verify SMS from PhoneA to PhoneB
        Verify both devices are still on 5g NSA

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        if not provision_both_devices_for_volte(self.log, ads):
            return False
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        if not provision_device_for_5g(self.log, ads):
            return False

        if not test_sms_mo_in_call(self.log,
                                   ads,
                                   caller_func=is_phone_in_call_volte):
            return False

        if not verify_5g_attach_for_both_devices(self.log, ads):
            return False
        return True


    @test_tracker_info(uuid="3d5c8f60-1eaa-4f4a-b539-c529fa36db91")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_mo_in_call_volte(self):
        """ Test MO SMS during a MO VoLTE call over 5G NSA.

        Provision PhoneA on VoLTE
        Provision PhoneA in 5g NSA
        Make a Voice call from PhoneA to PhoneB
        Send and Verify SMS from PhoneA to PhoneB
        Verify phoneA is still on 5g NSA

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        if not phone_setup_volte(self.log, ads[0]):
            return False
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        tasks = [(provision_device_for_5g, (self.log, ads[0])),
                 (ensure_phone_default_state, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            return False

        if not test_sms_mo_in_call(self.log,
                                   ads,
                                   caller_func=is_phone_in_call_volte):
            return False

        if not is_current_network_5g_nsa(ads[0]):
            return False
        return True


    @test_tracker_info(uuid="c71813f3-bb04-4115-8519-e23046349689")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_mt_in_call_volte(self):
        """ Test MT SMS during a MT VoLTE call over 5G NSA.

        Provision PhoneA on VoLTE
        Provision PhoneA in 5g NSA
        Make a Voice call from PhoneB to PhoneA
        Send and Verify SMS from PhoneB to PhoneA
        Verify phoneA is still on 5g NSA

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices
        if not phone_setup_volte(self.log, ads[0]):
            return False
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        tasks = [(provision_device_for_5g, (self.log, ads[0])),
                 (ensure_phone_default_state, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            return False

        if not test_sms_mo_in_call(self.log,
                                   [ads[1], ads[0]],
                                   callee_func=is_phone_in_call_volte):
            return False

        if not is_current_network_5g_nsa(ads[0]):
            return False
        return True


    @test_tracker_info(uuid="1f914d5c-ac24-4794-9fcb-cb28e483d69a")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_mo_mt_iwlan(self):
        """ Test SMS text function between two phones,
        Phones in APM, WiFi connected, WFC Cell Preferred mode.

        Disable APM on both devices
        Provision devices in 5g NSA
        Provision devices for WFC Cell Pref with APM ON
        Send and Verify SMS from PhoneA to PhoneB

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

        if not _sms_test_mo(self.log, ads):
            return False

        self.log.info("PASS - iwlan sms test over 5g nsa validated")
        return True


    @test_tracker_info(uuid="7274be32-b9dd-4ce3-83d1-f32ab14ce05e")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_mo_mt_iwlan_apm_off(self):
        """ Test MO SMS, Phone in APM off, WiFi connected, WFC WiFi Preferred mode.

        Disable APM on both devices
        Provision devices in 5g NSA
        Provision devices for WFC Wifi Pref with APM OFF
        Send and Verify SMS from PhoneA to PhoneB
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

        if not _sms_test_mo(self.log, ads):
            self.log.error("failed to send receive sms over 5g nsa")
            return False
        self.log.info("PASS - iwlan sms test over 5g nsa validated")

        if not verify_5g_attach_for_both_devices(self.log, ads):
            return False
        return True


    @test_tracker_info(uuid="2d1787f2-d6fe-4b41-b389-2a8f817594e4")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_mo_mt_in_call_iwlan(self):
        """ Test MO SMS, Phone in APM, WiFi connected, WFC WiFi Preferred mode.

        Disable APM on both devices
        Provision devices in 5g NSA
        Provision devices for WFC Wifi Pref with APM ON
        Make a Voice call from PhoneA to PhoneB
        Send and Verify SMS from PhoneA to PhoneB

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

        return test_sms_mo_in_call(self.log,
                                   ads,
                                   caller_func=is_phone_in_call_iwlan)


    @test_tracker_info(uuid="784062e8-02a4-49ce-8fc1-5359ab40bbdd")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_long_message_mo_mt(self):
        """Test SMS basic function between two phone. Phones in nsa 5G network.

        Airplane mode is off.
        Send SMS from PhoneA to PhoneB.
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

        return _long_sms_test_mo(self.log, ads)


    @test_tracker_info(uuid="45dbd61a-6a90-473e-9cfa-03e2408d5f15")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_mo_mt_in_call_csfb(self):
        """ Test MO/MT SMS during a MO csfb call over 5G NSA.

        Disable APM on both devices
        Set up PhoneA/B are in CSFB mode.
        Provision PhoneA/B in 5g NSA.
        Make sure PhoneA/B is able to make/receive call.
        Call from PhoneA to PhoneB, accept on PhoneB, send SMS on PhoneA,
         receive SMS on PhoneB.

        Returns:
            True if pass; False if fail.
        """
        ads = self.android_devices

        if not disable_apm_mode_both_devices(self.log, ads):
            return False

        if not provision_both_devices_for_csfb(self.log, ads):
            return False
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        if not provision_device_for_5g_nsa(self.log, ads):
            return False

        return test_sms_mo_in_call(self.log,
                                   ads,
                                   caller_func=is_phone_in_call_csfb)


    @test_tracker_info(uuid="2d375f20-a785-42e0-b5a1-968d19bc693d")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_mo_iwlan(self):
        """ Test MO SMS for 1 phone in APM,
        WiFi connected, WFC Cell Preferred mode.

        Disable APM on PhoneA
        Provision PhoneA in 5g NSA
        Provision PhoneA for WFC Cell Pref with APM ON
        Send and Verify SMS from PhoneA to PhoneB

        Returns:
            True if pass; False if fail.
        """

        ads = self.android_devices
        if not toggle_airplane_mode(self.log, ads[0], False):
            return False

        tasks = [(provision_device_for_5g, (self.log, ads[0])),
                 (ensure_phone_default_state, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            return False

        if not phone_setup_iwlan(self.log,
                                ads[0],
                                True,
                                WFC_MODE_CELLULAR_PREFERRED,
                                self.wifi_network_ssid,
                                self.wifi_network_pass):
            return False
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        if not _sms_test_mo(self.log, ads):
            return False

        self.log.info("PASS - iwlan mo sms test over 5g nsa validated")
        return True


    @test_tracker_info(uuid="db8b2b5b-bf9e-4a99-9fdb-dbd028567705")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_mt_iwlan(self):
        """ Test MT SMS for 1 phone in APM,
        WiFi connected, WFC Cell Preferred mode.

        Disable APM on PhoneA
        Provision PhoneA in 5g NSA
        Provision PhoneA for WFC Cell Pref with APM ON
        Send and Verify SMS from PhoneB to PhoneA

        Returns:
            True if pass; False if fail.
        """

        ads = self.android_devices
        if not toggle_airplane_mode(self.log, ads[0], False):
            return False

        tasks = [(provision_device_for_5g, (self.log, ads[0])),
                 (ensure_phone_default_state, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            return False

        if not phone_setup_iwlan(self.log,
                                ads[0],
                                True,
                                WFC_MODE_CELLULAR_PREFERRED,
                                self.wifi_network_ssid,
                                self.wifi_network_pass):
            return False
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        if not _sms_test_mo(self.log, [ads[1], ads[0]]):
            return False

        self.log.info("PASS - iwlan mt sms test over 5g nsa validated")
        return True


    @test_tracker_info(uuid="5997a618-efee-478f-8fa9-6cf8ba9cfc58")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_mo_iwlan_apm_off(self):
        """ Test MO SMS for 1 Phone in APM off, WiFi connected,
        WFC WiFi Preferred mode.

        Disable APM on PhoneA
        Provision PhoneA in 5g NSA
        Provision PhoneA for WFC Wifi Pref with APM OFF
        Send and Verify SMS from PhoneA to PhoneB
        Verify 5g NSA attach for PhoneA

        Returns:
            True if pass; False if fail.
        """

        ads = self.android_devices
        if not toggle_airplane_mode(self.log, ads[0], False):
            return False

        tasks = [(provision_device_for_5g, (self.log, ads[0])),
                 (ensure_phone_default_state, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            return False

        if not phone_setup_iwlan(self.log,
                                ads[0],
                                False,
                                WFC_MODE_WIFI_PREFERRED,
                                self.wifi_network_ssid,
                                self.wifi_network_pass):
            return False
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        if not _sms_test_mo(self.log, ads):
            self.log.error("failed to send receive sms over 5g nsa")
            return False
        self.log.info("PASS - iwlan MO sms test over 5g nsa validated")

        if not is_current_network_5g_nsa(ads[0]):
            return False
        return True


    @test_tracker_info(uuid="352ca023-2cd1-4b08-877c-20c5d50cc265")
    @TelephonyBaseTest.tel_test_wrap
    def test_5g_nsa_sms_mt_iwlan_apm_off(self):
        """ Test MT SMS for 1 Phone in APM off, WiFi connected,
        WFC WiFi Preferred mode.

        Disable APM on PhoneA
        Provision PhoneA in 5g NSA
        Provision PhoneA for WFC Wifi Pref with APM OFF
        Send and Verify SMS from PhoneB to PhoneA
        Verify 5g NSA attach for PhoneA

        Returns:
            True if pass; False if fail.
        """

        ads = self.android_devices
        if not toggle_airplane_mode(self.log, ads[0], False):
            return False

        tasks = [(provision_device_for_5g, (self.log, ads[0])),
                 (ensure_phone_default_state, (self.log, ads[1]))]
        if not multithread_func(self.log, tasks):
            return False

        if not phone_setup_iwlan(self.log,
                                ads[0],
                                False,
                                WFC_MODE_WIFI_PREFERRED,
                                self.wifi_network_ssid,
                                self.wifi_network_pass):
            return False
        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)

        if not _sms_test_mt(self.log, ads):
            self.log.error("failed to send receive sms over 5g nsa")
            return False
        self.log.info("PASS - iwlan MT sms test over 5g nsa validated")

        if not is_current_network_5g_nsa(ads[0]):
            return False
        return True

    """ Tests End """
