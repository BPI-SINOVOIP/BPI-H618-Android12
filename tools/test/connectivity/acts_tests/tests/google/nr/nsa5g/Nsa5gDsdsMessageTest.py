#!/usr/bin/env python3
#
#   Copyright 2021 - Google
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
from acts.test_decorators import test_tracker_info
from acts_contrib.test_utils.tel.loggers.protos.telephony_metric_pb2 import TelephonyVoiceTestResult
from acts_contrib.test_utils.tel.loggers.telephony_metric_logger import TelephonyMetricLogger
from acts_contrib.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts_contrib.test_utils.tel.tel_defines import MAX_WAIT_TIME_SMS_RECEIVE
from acts_contrib.test_utils.tel.tel_defines import WAIT_TIME_ANDROID_STATE_SETTLING
from acts_contrib.test_utils.tel.tel_defines import INVALID_SUB_ID
from acts_contrib.test_utils.tel.tel_defines import GEN_5G
from acts_contrib.test_utils.tel.tel_subscription_utils import get_incoming_voice_sub_id
from acts_contrib.test_utils.tel.tel_subscription_utils import get_subid_from_slot_index
from acts_contrib.test_utils.tel.tel_subscription_utils import get_outgoing_message_sub_id
from acts_contrib.test_utils.tel.tel_subscription_utils import get_default_data_sub_id
from acts_contrib.test_utils.tel.tel_subscription_utils import set_message_subid
from acts_contrib.test_utils.tel.tel_subscription_utils import set_subid_for_data
from acts_contrib.test_utils.tel.tel_subscription_utils import set_voice_sub_id
from acts_contrib.test_utils.tel.tel_subscription_utils import set_dds_on_slot_0
from acts_contrib.test_utils.tel.tel_subscription_utils import set_dds_on_slot_1
from acts_contrib.test_utils.tel.tel_subscription_utils import get_subid_on_same_network_of_host_ad
from acts_contrib.test_utils.tel.tel_test_utils import multithread_func
from acts_contrib.test_utils.tel.tel_test_utils import sms_send_receive_verify_for_subscription
from acts_contrib.test_utils.tel.tel_test_utils import sms_in_collision_send_receive_verify_for_subscription
from acts_contrib.test_utils.tel.tel_test_utils import sms_rx_power_off_multiple_send_receive_verify_for_subscription
from acts_contrib.test_utils.tel.tel_test_utils import voice_call_in_collision_with_mt_sms_msim
from acts_contrib.test_utils.tel.tel_test_utils import mms_send_receive_verify
from acts_contrib.test_utils.tel.tel_test_utils import verify_http_connection
from acts_contrib.test_utils.tel.tel_test_utils import log_messaging_screen_shot
from acts_contrib.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts_contrib.test_utils.tel.tel_test_utils import get_slot_index_from_subid
from acts_contrib.test_utils.tel.tel_voice_utils import phone_setup_voice_general_for_subscription
from acts_contrib.test_utils.tel.tel_voice_utils import phone_setup_on_rat
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_on_rat
from acts.utils import rand_ascii_str

CallResult = TelephonyVoiceTestResult.CallResult.Value

class Nsa5gDsdsMessageTest(TelephonyBaseTest):
    def setup_class(self):
        TelephonyBaseTest.setup_class(self)
        self.message_lengths = (50, 160, 180)
        self.tel_logger = TelephonyMetricLogger.for_test_case()

    def teardown_test(self):
        ensure_phones_idle(self.log, self.android_devices)

    def _msim_message_test(
        self,
        ad_mo,
        ad_mt,
        mo_sub_id,
        mt_sub_id, msg="SMS",
        max_wait_time=MAX_WAIT_TIME_SMS_RECEIVE,
        expected_result=True):
        """Make MO/MT SMS/MMS at specific slot.

        Args:
            ad_mo: Android object of the device sending SMS/MMS
            ad_mt: Android object of the device receiving SMS/MMS
            mo_sub_id: Sub ID of MO device
            mt_sub_id: Sub ID of MT device
            max_wait_time: Max wait time before SMS/MMS is received.
            expected_result: True for successful sending/receiving and False on
                             the contrary

        Returns:
            True if the result matches expected_result and False on the
            contrary.
        """

        if msg == "SMS":
            for length in self.message_lengths:
                message_array = [rand_ascii_str(length)]
                if not sms_send_receive_verify_for_subscription(
                    self.log,
                    ad_mo,
                    ad_mt,
                    mo_sub_id,
                    mt_sub_id,
                    message_array,
                    max_wait_time):
                    ad_mo.log.warning(
                        "%s of length %s test failed", msg, length)
                    return False
                else:
                    ad_mo.log.info(
                        "%s of length %s test succeeded", msg, length)
            self.log.info("%s test of length %s characters succeeded.",
                msg, self.message_lengths)

        elif msg == "MMS":
            for length in self.message_lengths:
                message_array = [("Test Message", rand_ascii_str(length), None)]

                if not mms_send_receive_verify(
                    self.log,
                    ad_mo,
                    ad_mt,
                    message_array,
                    max_wait_time,
                    expected_result):
                    self.log.warning("%s of body length %s test failed",
                        msg, length)
                    return False
                else:
                    self.log.info(
                        "%s of body length %s test succeeded", msg, length)
            self.log.info("%s test of body lengths %s succeeded",
                          msg, self.message_lengths)
        return True

    def _test_msim_message(
            self,
            mo_slot,
            mt_slot,
            dds_slot,
            msg="SMS",
            mo_rat=["", ""],
            mt_rat=["", ""],
            direction="mo",
            expected_result=True):
        """Make MO/MT SMS/MMS at specific slot in specific RAT with DDS at
        specific slot.

        Test step:
        1. Get sub IDs of specific slots of both MO and MT devices.
        2. Switch DDS to specific slot.
        3. Check HTTP connection after DDS switch.
        4. Set up phones in desired RAT.
        5. Send SMS/MMS.

        Args:
            mo_slot: Slot sending MO SMS (0 or 1)
            mt_slot: Slot receiving MT SMS (0 or 1)
            dds_slot: Preferred data slot
            mo_rat: RAT for both slots of MO device
            mt_rat: RAT for both slots of MT device
            direction: "mo" or "mt"
            expected_result: True of False

        Returns:
            TestFailure if failed.
        """
        ads = self.android_devices

        if direction == "mo":
            ad_mo = ads[0]
            ad_mt = ads[1]
        else:
            ad_mo = ads[1]
            ad_mt = ads[0]

        if mo_slot is not None:
            mo_sub_id = get_subid_from_slot_index(self.log, ad_mo, mo_slot)
            if mo_sub_id == INVALID_SUB_ID:
                ad_mo.log.warning("Failed to get sub ID at slot %s.", mo_slot)
                return False
            mo_other_sub_id = get_subid_from_slot_index(
                self.log, ad_mo, 1-mo_slot)
            set_message_subid(ad_mo, mo_sub_id)
        else:
            _, mo_sub_id, _ = get_subid_on_same_network_of_host_ad(
                ads, type="sms")
            if mo_sub_id == INVALID_SUB_ID:
                ad_mo.log.warning("Failed to get sub ID at slot %s.", mo_slot)
                return False
            mo_slot = "auto"
            set_message_subid(ad_mo, mo_sub_id)
            if msg == "MMS":
                set_subid_for_data(ad_mo, mo_sub_id)
                ad_mo.droid.telephonyToggleDataConnection(True)
        ad_mo.log.info("Sub ID for outgoing %s at slot %s: %s", msg, mo_slot,
            get_outgoing_message_sub_id(ad_mo))

        if mt_slot is not None:
            mt_sub_id = get_subid_from_slot_index(self.log, ad_mt, mt_slot)
            if mt_sub_id == INVALID_SUB_ID:
                ad_mt.log.warning("Failed to get sub ID at slot %s.", mt_slot)
                return False
            mt_other_sub_id = get_subid_from_slot_index(
                self.log, ad_mt, 1-mt_slot)
            set_message_subid(ad_mt, mt_sub_id)
        else:
            _, mt_sub_id, _ = get_subid_on_same_network_of_host_ad(
                ads, type="sms")
            if mt_sub_id == INVALID_SUB_ID:
                ad_mt.log.warning("Failed to get sub ID at slot %s.", mt_slot)
                return False
            mt_slot = "auto"
            set_message_subid(ad_mt, mt_sub_id)
            if msg == "MMS":
                set_subid_for_data(ad_mt, mt_sub_id)
                ad_mt.droid.telephonyToggleDataConnection(True)
        ad_mt.log.info("Sub ID for incoming %s at slot %s: %s", msg, mt_slot,
                       get_outgoing_message_sub_id(ad_mt))

        self.log.info("Step 1: Switch DDS.")
        if dds_slot:
            if not set_dds_on_slot_1(ads[0]):
                self.log.warning(
                    "Failed to set DDS at eSIM on %s", ads[0].serial)
                return False
        else:
            if not set_dds_on_slot_0(ads[0]):
                self.log.warning(
                    "Failed to set DDS at pSIM on %s", ads[0].serial)
                return False

        self.log.info("Step 2: Check HTTP connection after DDS switch.")
        if not verify_http_connection(self.log,
           ads[0],
           url="https://www.google.com",
           retry=5,
           retry_interval=15,
           expected_state=True):

            self.log.error("Failed to verify http connection.")
            return False
        else:
            self.log.info("Verify http connection successfully.")

        mo_phone_setup_func_argv = (self.log, ad_mo, mo_sub_id)
        mt_phone_setup_func_argv = (self.log, ad_mt, mt_sub_id)

        if mo_slot in (0, 1):
            # set up the rat on mo side another slot which not to be test(primary device)
            phone_setup_on_rat(self.log, ad_mo, mo_rat[1-mo_slot], mo_other_sub_id)
            # get phone setup function and required argument of primary device
            if '5g' in mo_rat[mo_slot].lower():
                mo_phone_setup_func_argv = (self.log, ad_mo, mo_sub_id, GEN_5G)
            mo_phone_setup_func = phone_setup_on_rat(
                self.log,
                ad_mo,
                mo_rat[mo_slot],
                only_return_fn=True)
        else:
            # set up the rat and get phone setup function on mo side(non-primary device)
            phone_setup_on_rat(self.log, ad_mo, 'general', sub_id_type='sms')
            mo_phone_setup_func = phone_setup_voice_general_for_subscription

        if mt_slot in (0, 1):
            # set up the rat on mt side another slot which not to be test(primary device)
            phone_setup_on_rat(self.log, ad_mt, mt_rat[1-mt_slot], mt_other_sub_id)
            # get phone setup function and required argument of primary device
            if '5g' in mt_rat[mt_slot].lower():
                mt_phone_setup_func_argv = (self.log, ad_mt, mt_sub_id, GEN_5G)
            mt_phone_setup_func = phone_setup_on_rat(
                self.log,
                ad_mt,
                mt_rat[mt_slot],
                only_return_fn=True)
        else:
            # set up the rat and get phone setup function on mt side(non-primary device)
            phone_setup_on_rat(self.log, ad_mt, 'general', sub_id_type='sms')
            mt_phone_setup_func = phone_setup_voice_general_for_subscription

        self.log.info("Step 3: Set up phones in desired RAT.")
        tasks = [(mo_phone_setup_func, mo_phone_setup_func_argv),
                 (mt_phone_setup_func, mt_phone_setup_func_argv)]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            return False

        time.sleep(WAIT_TIME_ANDROID_STATE_SETTLING)
        self.log.info("Step 4: Send %s.", msg)

        if msg == "MMS":
            for ad, current_data_sub_id, current_msg_sub_id in [
                [ ads[0],
                  get_default_data_sub_id(ads[0]),
                  get_outgoing_message_sub_id(ads[0]) ],
                [ ads[1],
                  get_default_data_sub_id(ads[1]),
                  get_outgoing_message_sub_id(ads[1]) ]]:
                if current_data_sub_id != current_msg_sub_id:
                    ad.log.warning(
                        "Current data sub ID (%s) does not match message"
                        " sub ID (%s). MMS should NOT be sent.",
                        current_data_sub_id,
                        current_msg_sub_id)
                    expected_result = False

        result = self._msim_message_test(ad_mo, ad_mt, mo_sub_id, mt_sub_id,
            msg=msg, expected_result=expected_result)

        if not result:
            log_messaging_screen_shot(ad_mo, test_name="%s_tx" % msg)
            log_messaging_screen_shot(ad_mt, test_name="%s_rx" % msg)

        return result

    @test_tracker_info(uuid="183cda35-45aa-485d-b3d4-975d78f7d361")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_sms_mo_psim_volte_esim_nsa_5g_volte_dds_1(self):
        return self._test_msim_message(
            0, None, 1, mo_rat=["volte", "5g_volte"], msg="SMS", direction="mo")

    @test_tracker_info(uuid="d9cb69ce-c462-4fd4-b716-bfb1fd2ed86a")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_sms_mt_psim_volte_esim_nsa_5g_volte_dds_1(self):
        return self._test_msim_message(
            None, 0, 1, mt_rat=["volte", "5g_volte"], msg="SMS", direction="mt")

    @test_tracker_info(uuid="51d5e05d-66e7-4369-91e0-6cdc573d9a59")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_sms_mo_esim_nsa_5g_volte_psim_volte_dds_1(self):
        return self._test_msim_message(
            1, None, 1, mo_rat=["volte", "5g_volte"], msg="SMS", direction="mo")

    @test_tracker_info(uuid="38271a0f-2efb-4991-9f24-6da9f003ddd4")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_sms_mt_esim_nsa_5g_volte_psim_volte_dds_1(self):
        return self._test_msim_message(
            None, 1, 1, mt_rat=["volte", "5g_volte"], msg="SMS", direction="mt")

    @test_tracker_info(uuid="87759475-0208-4d9b-b5b9-814fdb97f09c")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_sms_mo_psim_nsa_5g_volte_esim_volte_dds_0(self):
        return self._test_msim_message(
            0, None, 0, mo_rat=["5g_volte", "volte"], msg="SMS", direction="mo")

    @test_tracker_info(uuid="2f14e81d-330f-4cdd-837c-1168185ffec4")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_sms_mt_psim_nsa_5g_volte_esim_volte_dds_0(self):
        return self._test_msim_message(
            None, 0, 0, mt_rat=["5g_volte", "volte"], msg="SMS", direction="mt")

    @test_tracker_info(uuid="9cc45474-1fca-4008-8499-87829d6516ea")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_sms_mo_esim_volte_psim_nsa_5g_volte_dds_0(self):
        return self._test_msim_message(
            1, None, 0, mo_rat=["5g_volte", "volte"], msg="SMS", direction="mo")

    @test_tracker_info(uuid="341786de-5b23-438a-a91b-97cf420ef5fd")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_sms_mt_esim_volte_psim_nsa_5g_volte_dds_0(self):
        return self._test_msim_message(
            None, 1, 0, mt_rat=["5g_volte", "volte"], msg="SMS", direction="mt")

