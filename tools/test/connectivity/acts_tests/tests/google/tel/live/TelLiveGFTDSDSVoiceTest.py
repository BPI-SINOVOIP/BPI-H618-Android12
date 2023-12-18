#!/usr/bin/env python3
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

from acts import asserts
from acts import signals
from acts.test_decorators import test_tracker_info
from acts_contrib.test_utils.tel.loggers.protos.telephony_metric_pb2 import \
    TelephonyVoiceTestResult
from acts_contrib.test_utils.tel.loggers.telephony_metric_logger import \
    TelephonyMetricLogger
from acts_contrib.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts_contrib.test_utils.tel.tel_defines import CAPABILITY_CONFERENCE
from acts_contrib.test_utils.tel.tel_defines import INVALID_SUB_ID
from acts_contrib.test_utils.tel.tel_subscription_utils import \
    get_incoming_voice_sub_id
from acts_contrib.test_utils.tel.tel_subscription_utils import \
    get_outgoing_voice_sub_id
from acts_contrib.test_utils.tel.tel_subscription_utils import get_subid_from_slot_index
from acts_contrib.test_utils.tel.tel_subscription_utils import set_voice_sub_id
from acts_contrib.test_utils.tel.tel_subscription_utils import set_dds_on_slot_0
from acts_contrib.test_utils.tel.tel_subscription_utils import set_dds_on_slot_1
from acts_contrib.test_utils.tel.tel_subscription_utils import \
    get_subid_on_same_network_of_host_ad
from acts_contrib.test_utils.tel.tel_test_utils import hangup_call
from acts_contrib.test_utils.tel.tel_test_utils import multithread_func
from acts_contrib.test_utils.tel.tel_test_utils import get_capability_for_subscription
from acts_contrib.test_utils.tel.tel_test_utils import verify_http_connection
from acts_contrib.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts_contrib.test_utils.tel.tel_test_utils import get_slot_index_from_subid
from acts_contrib.test_utils.tel.tel_test_utils import get_operator_name
from acts_contrib.test_utils.tel.tel_voice_utils import \
    phone_setup_voice_general_for_subscription
from acts_contrib.test_utils.tel.tel_voice_utils import phone_setup_on_rat
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_on_rat
from acts_contrib.test_utils.tel.tel_voice_utils import two_phone_call_msim_for_slot

CallResult = TelephonyVoiceTestResult.CallResult.Value

class TelLiveGFTDSDSVoiceTest(TelephonyBaseTest):
    def setup_class(self):
        TelephonyBaseTest.setup_class(self)
        self.tel_logger = TelephonyMetricLogger.for_test_case()

    def teardown_test(self):
        ensure_phones_idle(self.log, self.android_devices)

    def _hangup_call(self, ad, device_description='Device'):
        if not hangup_call(self.log, ad):
            ad.log.error("Failed to hang up on %s", device_description)
            return False
        return True

    def _test_msim_call_voice(
            self,
            mo_slot,
            mt_slot,
            dds_slot,
            mo_rat=["", ""],
            mt_rat=["", ""],
            call_direction="mo"):
        """Make MO/MT voice call at specific slot in specific RAT with DDS at
        specific slot.

        Test step:
        1. Get sub IDs of specific slots of both MO and MT devices.
        2. Switch DDS to specific slot.
        3. Check HTTP connection after DDS switch.
        4. Set up phones in desired RAT.
        5. Make voice call.

        Args:
            mo_slot: Slot making MO call (0 or 1)
            mt_slot: Slot receiving MT call (0 or 1)
            dds_slot: Preferred data slot
            mo_rat: RAT for both slots of MO device
            mt_rat: RAT for both slots of MT device
            call_direction: "mo" or "mt"

        Returns:
            TestFailure if failed.
        """
        ads = self.android_devices

        if call_direction == "mo":
            ad_mo = ads[0]
            ad_mt = ads[1]
        else:
            ad_mo = ads[1]
            ad_mt = ads[0]

        if mo_slot is not None:
            mo_sub_id = get_subid_from_slot_index(self.log, ad_mo, mo_slot)
            if mo_sub_id == INVALID_SUB_ID:
                ad_mo.log.warning("Failed to get sub ID ar slot %s.", mo_slot)
                return False
            mo_other_sub_id = get_subid_from_slot_index(
                self.log, ad_mo, 1-mo_slot)
            set_voice_sub_id(ad_mo, mo_sub_id)
        else:
            _, mo_sub_id, _ = get_subid_on_same_network_of_host_ad(ads)
            if mo_sub_id == INVALID_SUB_ID:
                ad_mo.log.warning("Failed to get sub ID ar slot %s.", mo_slot)
                return False
            mo_slot = "auto"
            set_voice_sub_id(ad_mo, mo_sub_id)
        ad_mo.log.info("Sub ID for outgoing call at slot %s: %s",
            mo_slot, get_outgoing_voice_sub_id(ad_mo))

        if mt_slot is not None:
            mt_sub_id = get_subid_from_slot_index(self.log, ad_mt, mt_slot)
            if mt_sub_id == INVALID_SUB_ID:
                ad_mt.log.warning("Failed to get sub ID at slot %s.", mt_slot)
                return False
            mt_other_sub_id = get_subid_from_slot_index(
                self.log, ad_mt, 1-mt_slot)
            set_voice_sub_id(ad_mt, mt_sub_id)
        else:
            _, mt_sub_id, _ = get_subid_on_same_network_of_host_ad(ads)
            if mt_sub_id == INVALID_SUB_ID:
                ad_mt.log.warning("Failed to get sub ID at slot %s.", mt_slot)
                return False
            mt_slot = "auto"
            set_voice_sub_id(ad_mt, mt_sub_id)
        ad_mt.log.info("Sub ID for incoming call at slot %s: %s", mt_slot,
            get_incoming_voice_sub_id(ad_mt))

        self.log.info("Step 1: Switch DDS.")
        if dds_slot:
            if not set_dds_on_slot_1(ads[0]):
                ads[0].log.warning("Failed to set DDS at eSIM.")
                return False
        else:
            if not set_dds_on_slot_0(ads[0]):
                ads[0].log.warning("Failed to set DDS at pSIM.")
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

        if mo_slot == 0 or mo_slot == 1:
            phone_setup_on_rat(self.log, ad_mo, mo_rat[1-mo_slot], mo_other_sub_id)
            mo_phone_setup_func = phone_setup_on_rat(
                self.log,
                ad_mo,
                mo_rat[mo_slot],
                only_return_fn=True)
            is_mo_in_call = is_phone_in_call_on_rat(
                self.log, ad_mo, mo_rat[mo_slot], only_return_fn=True)
        else:
            phone_setup_on_rat(self.log, ad_mo, 'general')
            mo_phone_setup_func = phone_setup_voice_general_for_subscription
            is_mo_in_call = is_phone_in_call_on_rat(
                self.log, ad_mo, 'general', only_return_fn=True)

        if mt_slot == 0 or mt_slot == 1:
            phone_setup_on_rat(self.log, ad_mt, mt_rat[1-mt_slot], mt_other_sub_id)
            mt_phone_setup_func = phone_setup_on_rat(
                self.log,
                ad_mt,
                mt_rat[mt_slot],
                only_return_fn=True)
            is_mt_in_call = is_phone_in_call_on_rat(
                self.log, ad_mt, mt_rat[mt_slot], only_return_fn=True)
        else:
            phone_setup_on_rat(self.log, ad_mt, 'general')
            mt_phone_setup_func = phone_setup_voice_general_for_subscription
            is_mt_in_call = is_phone_in_call_on_rat(
                self.log, ad_mt, 'general', only_return_fn=True)

        self.log.info("Step 3: Set up phones in desired RAT.")
        tasks = [(mo_phone_setup_func, (self.log, ad_mo, mo_sub_id)),
                 (mt_phone_setup_func, (self.log, ad_mt, mt_sub_id))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            self.tel_logger.set_result(CallResult("CALL_SETUP_FAILURE"))
            raise signals.TestFailure("Failed",
                extras={"fail_reason": "Phone Failed to Set Up Properly."})

        self.log.info("Step 4: Make voice call.")
        result = two_phone_call_msim_for_slot(
            self.log,
            ad_mo,
            get_slot_index_from_subid(self.log, ad_mo, mo_sub_id),
            None,
            is_mo_in_call,
            ad_mt,
            get_slot_index_from_subid(self.log, ad_mt, mt_sub_id),
            None,
            is_mt_in_call)
        self.tel_logger.set_result(result.result_value)

        if not result:
            self.log.error(
                "Failed to make MO call from %s slot %s to %s slot %s",
                    ad_mo.serial, mo_slot, ad_mt.serial, mt_slot)
            raise signals.TestFailure("Failed",
                extras={"fail_reason": str(result.result_value)})


    @test_tracker_info(uuid="e252aa07-c377-4e12-8f06-ed1dc8f2b6a6")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_volte_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            0, None, 0, mo_rat=["volte", "volte"], call_direction="mo")

    @test_tracker_info(uuid="7631b805-48b6-4b91-99a3-eef392e5b0fc")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_volte_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            0, None, 1, mo_rat=["volte", "volte"], call_direction="mo")

    @test_tracker_info(uuid="4771e517-08cf-4169-afe7-fe3e41f05c45")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_volte_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 0, 0, mt_rat=["volte", "volte"], call_direction="mt")

    @test_tracker_info(uuid="e8f914df-cada-4187-ab53-734624c9c941")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_volte_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 0, 1, mt_rat=["volte", "volte"], call_direction="mt")

    @test_tracker_info(uuid="967a665a-9614-4fe4-b293-e20b66637802")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_volte_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            1, None, 0, mo_rat=["volte", "volte"], call_direction="mo")

    @test_tracker_info(uuid="901c7fa3-039f-4888-90eb-82af587fa8dd")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_volte_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            1, None, 1, mo_rat=["volte", "volte"], call_direction="mo")

    @test_tracker_info(uuid="a78f2808-a6c6-4483-b7f5-ad1ec925dd52")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_volte_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 1, 0, mt_rat=["volte", "volte"], call_direction="mt")

    @test_tracker_info(uuid="f6994dbd-c5a0-42c7-a43d-67227f5dfb88")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_volte_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 1, 1, mt_rat=["volte", "volte"], call_direction="mt")

    @test_tracker_info(uuid="0786d7d3-d272-4233-83dd-0667e844094d")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_volte_csfb_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            0, None, 0, mo_rat=["volte", "csfb"], call_direction="mo")

    @test_tracker_info(uuid="b9dfd46c-752c-4424-83b1-b5749a7018af")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_volte_csfb_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            0, None, 1, mo_rat=["volte", "csfb"], call_direction="mo")

    @test_tracker_info(uuid="8bc57654-a5d9-4c82-b11a-62e76ece9b43")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_volte_csfb_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 0, 0, mt_rat=["volte", "csfb"], call_direction="mt")

    @test_tracker_info(uuid="dbe44bf1-4638-4490-a06f-406205681ca5")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_volte_csfb_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 0, 1, mt_rat=["volte", "csfb"], call_direction="mt")

    @test_tracker_info(uuid="ffd82db7-eaaa-4f96-9e3b-e0e15d054e62")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_volte_csfb_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            1, None, 0, mo_rat=["volte", "csfb"], call_direction="mo")

    @test_tracker_info(uuid="f7f3f28b-eecf-42e5-ba28-168a38337c80")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_volte_csfb_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            1, None, 1, mo_rat=["volte", "csfb"], call_direction="mo")

    @test_tracker_info(uuid="eb6ae70a-3251-4642-8268-b91b593cecfd")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_volte_csfb_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 1, 0, mt_rat=["volte", "csfb"], call_direction="mt")

    @test_tracker_info(uuid="1d927140-34d2-4fc7-8fe4-b23a303fd190")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_volte_csfb_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 1, 1, mt_rat=["volte", "csfb"], call_direction="mt")

    @test_tracker_info(uuid="f15f6696-6e11-414b-8e28-9c16793b66b0")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_csfb_volte_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            0, None, 0, mo_rat=["csfb", "volte"], call_direction="mo")

    @test_tracker_info(uuid="ca99d987-0bdb-4034-892f-cc0b1d22f381")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_csfb_volte_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            0, None, 1, mo_rat=["csfb", "volte"], call_direction="mo")

    @test_tracker_info(uuid="692bd3d0-05be-4597-afab-2f837a3f9bd4")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_csfb_volte_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 0, 0, mt_rat=["csfb", "volte"], call_direction="mt")

    @test_tracker_info(uuid="87a5fae2-f32c-4b4d-8028-d065b582b117")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_csfb_volte_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 0, 1, mt_rat=["csfb", "volte"], call_direction="mt")

    @test_tracker_info(uuid="f6375034-5ecb-4872-bab2-cf9529f20fda")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_csfb_volte_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            1, None, 0, mo_rat=["csfb", "volte"], call_direction="mo")

    @test_tracker_info(uuid="6185bc28-1703-4ca2-a617-171d81adfe9a")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_csfb_volte_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            1, None, 1, mo_rat=["csfb", "volte"], call_direction="mo")

    @test_tracker_info(uuid="06bad228-27af-47b4-9b74-aacba81f9da7")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_csfb_volte_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 1, 0, mt_rat=["csfb", "volte"], call_direction="mt")

    @test_tracker_info(uuid="5a5f2178-2ac6-4d21-bf6f-b9d8455365f1")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_csfb_volte_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 1, 1, mt_rat=["csfb", "volte"], call_direction="mt")

    @test_tracker_info(uuid="216f8569-8120-43c4-a9c5-da3081d168db")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_volte_3g_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            0, None, 0, mo_rat=["volte", "3g"], call_direction="mo")

    @test_tracker_info(uuid="8d15524a-f7f9-4321-a962-b455bfdf4ec9")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_volte_3g_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            0, None, 1, mo_rat=["volte", "3g"], call_direction="mo")

    @test_tracker_info(uuid="c6aa5975-9ea6-4367-a59e-a248fde2c8be")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_volte_3g_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 0, 0, mt_rat=["volte", "3g"], call_direction="mt")

    @test_tracker_info(uuid="a99a54e0-46ea-4d35-a3c1-d825c546cc21")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_volte_3g_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 0, 1, mt_rat=["volte", "3g"], call_direction="mt")

    @test_tracker_info(uuid="6d128732-8a8e-488b-bb38-fbb764d228dd")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_volte_3g_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            1, None, 0, mo_rat=["volte", "3g"], call_direction="mo")

    @test_tracker_info(uuid="29517d00-5edf-4617-9d29-226d56426abf")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_volte_3g_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            1, None, 1, mo_rat=["volte", "3g"], call_direction="mo")

    @test_tracker_info(uuid="d18ec79e-3bc3-4c7e-89fd-d03519b2e2a6")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_volte_3g_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 1, 0, mt_rat=["volte", "3g"], call_direction="mt")

    @test_tracker_info(uuid="6442b85a-b116-4987-b6d5-2d2b9bac7fd5")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_volte_3g_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 1, 1, mt_rat=["volte", "3g"], call_direction="mt")

    @test_tracker_info(uuid="82e6f955-5156-4ad3-885d-d1d5ff0526cb")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_3g_volte_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            0, None, 0, mo_rat=["3g", "volte"], call_direction="mo")

    @test_tracker_info(uuid="ffdafbac-026d-4d7d-a1dc-f639c01db818")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_3g_volte_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            0, None, 1, mo_rat=["3g", "volte"], call_direction="mo")

    @test_tracker_info(uuid="b18dc6a7-e4a1-4409-a4aa-4e4add2fee13")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_3g_volte_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 0, 0, mt_rat=["3g", "volte"], call_direction="mt")

    @test_tracker_info(uuid="ea6fc855-31b8-4680-b306-51228277e0d3")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_3g_volte_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 0, 1, mt_rat=["3g", "volte"], call_direction="mt")

    @test_tracker_info(uuid="fdf9f4ea-a6f6-4434-a912-13711bb33a72")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_3g_volte_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            1, None, 0, mo_rat=["3g", "volte"], call_direction="mo")

    @test_tracker_info(uuid="deb8e2f6-e097-451e-9f19-aadaf1820fea")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_3g_volte_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            1, None, 1, mo_rat=["3g", "volte"], call_direction="mo")

    @test_tracker_info(uuid="6a636c5d-5da9-4916-9751-435ab39aaa00")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_3g_volte_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 1, 0, mt_rat=["3g", "volte"], call_direction="mt")

    @test_tracker_info(uuid="0b9d9d5c-e5e7-4c9d-ab8a-0658fadbf450")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_3g_volte_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 1, 1, mt_rat=["3g", "volte"], call_direction="mt")

    @test_tracker_info(uuid="fce99df9-8931-4a34-9285-121145fb9b2f")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_csfb_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            0, None, 0, mo_rat=["csfb", "csfb"], call_direction="mo")

    @test_tracker_info(uuid="81d9a087-e494-40e4-a0fb-7e4ef62e566c")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_csfb_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            0, None, 1, mo_rat=["csfb", "csfb"], call_direction="mo")

    @test_tracker_info(uuid="d4520000-9cd1-4aff-9862-bfb6832d51ce")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_csfb_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 0, 0, mt_rat=["csfb", "csfb"], call_direction="mt")

    @test_tracker_info(uuid="0eaf1e67-6aec-4a39-8f06-4e49009400e0")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_csfb_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 0, 1, mt_rat=["csfb", "csfb"], call_direction="mt")

    @test_tracker_info(uuid="0e6bc15a-e510-4b56-826c-96e0add6b20a")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_csfb_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            1, None, 0, mo_rat=["csfb", "csfb"], call_direction="mo")

    @test_tracker_info(uuid="ecead288-424d-4579-bf6a-10a1a78600d5")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_csfb_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            1, None, 1, mo_rat=["csfb", "csfb"], call_direction="mo")

    @test_tracker_info(uuid="3a76076d-808e-45f8-b99c-95b82f2f07de")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_csfb_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 1, 0, mt_rat=["csfb", "csfb"], call_direction="mt")

    @test_tracker_info(uuid="af638c75-c0e1-4ac1-83f5-bc0e6cdd913c")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_csfb_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 1, 1, mt_rat=["csfb", "csfb"], call_direction="mt")

    @test_tracker_info(uuid="0bf59f38-ddbc-4a88-bc8a-d6985e7d7567")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_csfb_3g_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            0, None, 0, mo_rat=["csfb", "3g"], call_direction="mo")

    @test_tracker_info(uuid="59f5a14a-c7e5-4bca-82dd-cb90b9a7a0e1")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_csfb_3g_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            0, None, 1, mo_rat=["csfb", "3g"], call_direction="mo")

    @test_tracker_info(uuid="79fc4d6f-0915-4717-80ae-db656cf3a82c")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_csfb_3g_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 0, 0, mt_rat=["csfb", "3g"], call_direction="mt")

    @test_tracker_info(uuid="b4927ebb-ae36-4ca7-a0b7-ea011b271122")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_csfb_3g_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 0, 1, mt_rat=["csfb", "3g"], call_direction="mt")

    @test_tracker_info(uuid="620be8d5-40b7-45f2-abfd-f788c8ce1977")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_csfb_3g_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            1, None, 0, mo_rat=["csfb", "3g"], call_direction="mo")

    @test_tracker_info(uuid="e277e0db-2dfb-4cfc-8d13-7c699f397b9b")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_csfb_3g_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            1, None, 1, mo_rat=["csfb", "3g"], call_direction="mo")

    @test_tracker_info(uuid="f7822fca-a22d-4989-bca8-506e9652cee1")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_csfb_3g_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 1, 0, mt_rat=["csfb", "3g"], call_direction="mt")

    @test_tracker_info(uuid="60e5f5cd-a2e1-4a6a-b76b-25a8ce2b037d")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_csfb_3g_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 1, 1, mt_rat=["csfb", "3g"], call_direction="mt")

    @test_tracker_info(uuid="ef4b5c61-e9c9-4a29-8ff1-f9920ec9f4dd")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_3g_csfb_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            0, None, 0, mo_rat=["3g", "csfb"], call_direction="mo")

    @test_tracker_info(uuid="0e82934d-391d-46af-9609-f59522140ea9")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_3g_csfb_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            0, None, 1, mo_rat=["3g", "csfb"], call_direction="mo")

    @test_tracker_info(uuid="dab9ea2d-5370-4438-b0ee-67c68ebda024")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_3g_csfb_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 0, 0, mt_rat=["3g", "csfb"], call_direction="mt")

    @test_tracker_info(uuid="3ba5816e-11fe-4a39-968d-2e9853e8f47a")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_3g_csfb_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 0, 1, mt_rat=["3g", "csfb"], call_direction="mt")

    @test_tracker_info(uuid="e8246c60-031d-4362-94c6-ad0882511d21")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_3g_csfb_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            1, None, 0, mo_rat=["3g", "csfb"], call_direction="mo")

    @test_tracker_info(uuid="db66ecb2-b09d-44be-991b-8025c6fab26a")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_3g_csfb_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            1, None, 1, mo_rat=["3g", "csfb"], call_direction="mo")

    @test_tracker_info(uuid="008e990c-94e4-4adc-abaa-e328d84079a5")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_3g_csfb_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 1, 0, mt_rat=["3g", "csfb"], call_direction="mt")

    @test_tracker_info(uuid="0a87cbb1-96d9-4eed-b92c-745432dc4ba4")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_3g_csfb_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 1, 1, mt_rat=["3g", "csfb"], call_direction="mt")

    @test_tracker_info(uuid="5620c3c8-e847-42c1-ae4e-b3370a0b6f98")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_3g_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            0, None, 0, mo_rat=["3g", "3g"], call_direction="mo")

    @test_tracker_info(uuid="a4415a1e-cd91-4a74-8f49-6c8ea428fe8f")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_3g_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            0, None, 1, mo_rat=["3g", "3g"], call_direction="mo")

    @test_tracker_info(uuid="35a73981-15d7-491f-bade-42642cabbf76")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_3g_psim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 0, 0, mt_rat=["3g", "3g"], call_direction="mt")

    @test_tracker_info(uuid="e38de6bd-8f6b-4a95-8c0f-e685abc3e7ef")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_3g_psim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 0, 1, mt_rat=["3g", "3g"], call_direction="mt")

    @test_tracker_info(uuid="1c86a1cb-5bd6-404a-a38f-4619a4b641a2")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_3g_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            1, None, 0, mo_rat=["3g", "3g"], call_direction="mo")

    @test_tracker_info(uuid="665736ff-206f-4c02-ae81-26f2e25d5988")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mo_3g_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            1, None, 1, mo_rat=["3g", "3g"], call_direction="mo")

    @test_tracker_info(uuid="5e5c8f33-60e5-44be-bf69-56c7715ead41")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_3g_esim_dds_slot_0(self):
        return self._test_msim_call_voice(
            None, 1, 0, mt_rat=["3g", "3g"], call_direction="mt")

    @test_tracker_info(uuid="2250b4d5-7b34-45cb-8ec2-300f4a4fbc2b")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_voice_call_mt_3g_esim_dds_slot_1(self):
        return self._test_msim_call_voice(
            None, 1, 1, mt_rat=["3g", "3g"], call_direction="mt")