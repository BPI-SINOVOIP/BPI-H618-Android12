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

import re
import time

from acts import asserts
from acts import signals
from acts.test_decorators import test_tracker_info
from acts_contrib.test_utils.tel.loggers.protos.telephony_metric_pb2 import \
    TelephonyVoiceTestResult
from acts_contrib.test_utils.tel.loggers.telephony_metric_logger import \
    TelephonyMetricLogger
from acts_contrib.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts_contrib.test_utils.tel.tel_defines import CALL_CAPABILITY_MANAGE_CONFERENCE
from acts_contrib.test_utils.tel.tel_defines import CALL_PROPERTY_CONFERENCE
from acts_contrib.test_utils.tel.tel_defines import CALL_STATE_ACTIVE
from acts_contrib.test_utils.tel.tel_defines import CAPABILITY_CONFERENCE
from acts_contrib.test_utils.tel.tel_defines import WAIT_TIME_IN_CALL
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
from acts_contrib.test_utils.tel.tel_test_utils import call_setup_teardown
from acts_contrib.test_utils.tel.tel_test_utils import hangup_call
from acts_contrib.test_utils.tel.tel_test_utils import multithread_func
from acts_contrib.test_utils.tel.tel_test_utils import num_active_calls
from acts_contrib.test_utils.tel.tel_test_utils import verify_incall_state
from acts_contrib.test_utils.tel.tel_test_utils import get_capability_for_subscription
from acts_contrib.test_utils.tel.tel_test_utils import verify_http_connection
from acts_contrib.test_utils.tel.tel_test_utils import set_call_waiting
from acts_contrib.test_utils.tel.tel_test_utils import ensure_phones_idle
from acts_contrib.test_utils.tel.tel_test_utils import initiate_call
from acts_contrib.test_utils.tel.tel_test_utils import \
    wait_and_reject_call_for_subscription
from acts_contrib.test_utils.tel.tel_test_utils import erase_call_forwarding_by_mmi
from acts_contrib.test_utils.tel.tel_test_utils import get_operator_name
from acts_contrib.test_utils.tel.tel_voice_utils import get_cep_conference_call_id
from acts_contrib.test_utils.tel.tel_voice_utils import \
    three_phone_call_forwarding_short_seq
from acts_contrib.test_utils.tel.tel_voice_utils import \
    three_phone_call_waiting_short_seq
from acts_contrib.test_utils.tel.tel_voice_utils import swap_calls
from acts_contrib.test_utils.tel.tel_voice_utils import phone_setup_on_rat
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_on_rat

CallResult = TelephonyVoiceTestResult.CallResult.Value

class TelLiveGFTDSDSSupplementaryServiceTest(TelephonyBaseTest):
    def setup_class(self):
        TelephonyBaseTest.setup_class(self)
        self.message_lengths = (50, 160, 180)
        self.tel_logger = TelephonyMetricLogger.for_test_case()
        self.erase_call_forwarding(self.log, self.android_devices[0])
        if not get_capability_for_subscription(
            self.android_devices[0],
            CAPABILITY_CONFERENCE,
            get_outgoing_voice_sub_id(self.android_devices[0])):
            self.android_devices[0].log.error(
                "Conference call is not supported, abort test.")
            raise signals.TestAbortClass(
                "Conference call is not supported, abort test.")

    def teardown_test(self):
        ensure_phones_idle(self.log, self.android_devices)
        self.erase_call_forwarding(self.log, self.android_devices[0])
        set_call_waiting(self.log, self.android_devices[0], enable=1)

    def _hangup_call(self, ad, device_description='Device'):
        if not hangup_call(self.log, ad):
            ad.log.error("Failed to hang up on %s", device_description)
            return False
        return True

    def erase_call_forwarding(self, log, ad):
        slot0_sub_id = get_subid_from_slot_index(log, ad, 0)
        slot1_sub_id = get_subid_from_slot_index(log, ad, 1)
        current_voice_sub_id = get_incoming_voice_sub_id(ad)
        for sub_id in (slot0_sub_id, slot1_sub_id):
            set_voice_sub_id(ad, sub_id)
            get_operator_name(log, ad, sub_id)
            erase_call_forwarding_by_mmi(log, ad)
        set_voice_sub_id(ad, current_voice_sub_id)

    def _three_phone_call_mo_add_mt(
        self,
        ads,
        phone_setups,
        verify_funcs,
        reject_once=False):
        """Use 3 phones to make MO call and MT call.

        Call from PhoneA to PhoneB, accept on PhoneB.
        Call from PhoneC to PhoneA, accept on PhoneA.

        Args:
            ads: list of ad object.
                The list should have three objects.
            phone_setups: list of phone setup functions.
                The list should have three objects.
            verify_funcs: list of phone call verify functions.
                The list should have three objects.

        Returns:
            If success, return 'call_AB' id in PhoneA.
            if fail, return None.
        """

        class _CallException(Exception):
            pass

        try:
            verify_func_a, verify_func_b, verify_func_c = verify_funcs
            tasks = []
            for ad, setup_func in zip(ads, phone_setups):
                if setup_func is not None:
                    tasks.append((setup_func, (self.log, ad, get_incoming_voice_sub_id(ad))))
            if tasks != [] and not multithread_func(self.log, tasks):
                self.log.error("Phone Failed to Set Up Properly.")
                raise _CallException("Setup failed.")
            for ad in ads:
                ad.droid.telecomCallClearCallList()
                if num_active_calls(self.log, ad) != 0:
                    ad.log.error("Phone Call List is not empty.")
                    raise _CallException("Clear call list failed.")

            self.log.info("Step1: Call From PhoneA to PhoneB.")
            if not call_setup_teardown(
                    self.log,
                    ads[0],
                    ads[1],
                    ad_hangup=None,
                    verify_caller_func=verify_func_a,
                    verify_callee_func=verify_func_b):
                raise _CallException("PhoneA call PhoneB failed.")

            calls = ads[0].droid.telecomCallGetCallIds()
            ads[0].log.info("Calls in PhoneA %s", calls)
            if num_active_calls(self.log, ads[0]) != 1:
                raise _CallException("Call list verify failed.")
            call_ab_id = calls[0]

            self.log.info("Step2: Call From PhoneC to PhoneA.")
            if reject_once:
                self.log.info("Step2-1: Reject incoming call once.")
                if not initiate_call(
                    self.log,
                    ads[2],
                    ads[0].telephony['subscription'][get_incoming_voice_sub_id(
                        ads[0])]['phone_num']):
                    ads[2].log.error("Initiate call failed.")
                    raise _CallException("Failed to initiate call.")

                if not wait_and_reject_call_for_subscription(
                        self.log,
                        ads[0],
                        get_incoming_voice_sub_id(ads[0]),
                        incoming_number= \
                            ads[2].telephony['subscription'][
                                get_incoming_voice_sub_id(
                                    ads[2])]['phone_num']):
                    ads[0].log.error("Reject call fail.")
                    raise _CallException("Failed to reject call.")

                self._hangup_call(ads[2], "PhoneC")
                time.sleep(15)

            if not call_setup_teardown(
                    self.log,
                    ads[2],
                    ads[0],
                    ad_hangup=None,
                    verify_caller_func=verify_func_c,
                    verify_callee_func=verify_func_a):
                raise _CallException("PhoneA call PhoneC failed.")
            if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]],
                                       True):
                raise _CallException("Not All phones are in-call.")

        except Exception as e:
            self.log.error(e)
            setattr(ads[0], "exception", e)
            return None

        return call_ab_id

    def _test_ims_conference_merge_drop_second_call_from_participant(
            self, call_ab_id, call_ac_id):
        """Test conference merge and drop in IMS (VoLTE or WiFi Calling) call.
        (supporting both cases of CEP enabled and disabled).

        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneB.
        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneC.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """
        ads = self.android_devices

        call_conf_id = self._merge_ims_conference_call(call_ab_id, call_ac_id)
        if call_conf_id is None:
            return False

        self.log.info("Step5: End call on PhoneC and verify call continues.")
        if not self._hangup_call(ads[2], "PhoneC"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            return False
        if not verify_incall_state(self.log, [ads[2]], False):
            return False

        self.log.info("Step6: End call on PhoneB and verify PhoneA end.")
        if not self._hangup_call(ads[1], "PhoneB"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    def _merge_ims_conference_call(self, call_ab_id, call_ac_id):
        """Merge IMS conference call for both cases of CEP enabled and disabled.

        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneB.
        PhoneA in IMS (VoLTE or WiFi Calling) call with PhoneC.
        Merge calls to conference on PhoneA.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            call_id for conference
        """
        ads = self.android_devices
        self.log.info("Step4: Merge to Conf Call and verify Conf Call.")
        ads[0].droid.telecomCallJoinCallsInConf(call_ab_id, call_ac_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)

        call_conf_id = None
        if num_active_calls(self.log, ads[0]) != 1:
            ads[0].log.info("Total number of call ids is not 1.")
            call_conf_id = get_cep_conference_call_id(ads[0])
            if call_conf_id is not None:
                self.log.info("New conference call id is found. CEP enabled.")
                calls.remove(call_conf_id)
                if (set(ads[0].droid.telecomCallGetCallChildren(
                    call_conf_id)) != set(calls)):
                    ads[0].log.error(
                        "Children list %s for conference call is not correct.",
                        ads[0].droid.telecomCallGetCallChildren(call_conf_id))
                    return None

                if (CALL_PROPERTY_CONFERENCE not in ads[0]
                        .droid.telecomCallGetProperties(call_conf_id)):
                    ads[0].log.error(
                        "Conf call id % properties wrong: %s", call_conf_id,
                        ads[0].droid.telecomCallGetProperties(call_conf_id))
                    return None

                if (CALL_CAPABILITY_MANAGE_CONFERENCE not in ads[0]
                        .droid.telecomCallGetCapabilities(call_conf_id)):
                    ads[0].log.error(
                        "Conf call id %s capabilities wrong: %s", call_conf_id,
                        ads[0].droid.telecomCallGetCapabilities(call_conf_id))
                    return None

                if (call_ab_id in calls) or (call_ac_id in calls):
                    self.log.error("Previous call ids should not in new call"
                    " list after merge.")
                    return None
        else:
            for call_id in calls:
                if call_id != call_ab_id and call_id != call_ac_id:
                    call_conf_id = call_id
                    self.log.info("CEP not enabled.")

        if not call_conf_id:
            self.log.error("Merge call fail, no new conference call id.")
            raise signals.TestFailure(
                "Calls were not merged. Failed to merge calls.",
                extras={"fail_reason": "Calls were not merged."
                    " Failed to merge calls."})
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        if ads[0].droid.telecomCallGetCallState(
                call_conf_id) != CALL_STATE_ACTIVE:
            ads[0].log.error(
                "Call_ID: %s, state: %s, expected: STATE_ACTIVE", call_conf_id,
                ads[0].droid.telecomCallGetCallState(call_conf_id))
            return None

        return call_conf_id

    def _test_wcdma_conference_merge_drop(self, call_ab_id, call_ac_id):
        """Test conference merge and drop in WCDMA/CSFB_WCDMA call.

        PhoneA in WCDMA (or CSFB_WCDMA) call with PhoneB.
        PhoneA in WCDMA (or CSFB_WCDMA) call with PhoneC.
        Merge calls to conference on PhoneA.
        Hangup on PhoneC, check call continues between AB.
        Hangup on PhoneB, check A ends.

        Args:
            call_ab_id: call id for call_AB on PhoneA.
            call_ac_id: call id for call_AC on PhoneA.

        Returns:
            True if succeed;
            False if failed.
        """
        ads = self.android_devices

        self.log.info("Step4: Merge to Conf Call and verify Conf Call.")
        ads[0].droid.telecomCallJoinCallsInConf(call_ab_id, call_ac_id)
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        num_calls = num_active_calls(self.log, ads[0])
        if num_calls != 3:
            ads[0].log.error("Total number of call ids is not 3.")
            if num_calls == 2:
                if call_ab_id in calls and call_ac_id in calls:
                    ads[0].log.error("Calls were not merged."
                        " Failed to merge calls.")
                    raise signals.TestFailure(
                        "Calls were not merged. Failed to merge calls.",
                        extras={"fail_reason": "Calls were not merged."
                            " Failed to merge calls."})
            return False
        call_conf_id = None
        for call_id in calls:
            if call_id != call_ab_id and call_id != call_ac_id:
                call_conf_id = call_id
        if not call_conf_id:
            self.log.error("Merge call fail, no new conference call id.")
            return False
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], True):
            return False

        if ads[0].droid.telecomCallGetCallState(
                call_conf_id) != CALL_STATE_ACTIVE:
            ads[0].log.error(
                "Call_id: %s, state: %s, expected: STATE_ACTIVE", call_conf_id,
                ads[0].droid.telecomCallGetCallState(call_conf_id))
            return False

        self.log.info("Step5: End call on PhoneC and verify call continues.")
        if not self._hangup_call(ads[2], "PhoneC"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 1:
            return False
        if not verify_incall_state(self.log, [ads[0], ads[1]], True):
            return False
        if not verify_incall_state(self.log, [ads[2]], False):
            return False

        self.log.info("Step6: End call on PhoneB and verify PhoneA end.")
        if not self._hangup_call(ads[1], "PhoneB"):
            return False
        time.sleep(WAIT_TIME_IN_CALL)
        if not verify_incall_state(self.log, [ads[0], ads[1], ads[2]], False):
            return False
        return True

    def _test_msim_call_forwarding(
            self,
            caller_slot,
            callee_slot,
            forwarded_callee_slot,
            dds_slot,
            caller_rat=["", ""],
            callee_rat=["", ""],
            forwarded_callee_rat=["", ""],
            call_forwarding_type="unconditional"):
        """Make MO voice call to the primary device at specific slot in specific
        RAT with DDS at specific slot, and then forwarded to 3rd device with
        specific call forwarding type.

        Test step:
        1. Get sub IDs of specific slots of both MO and MT devices.
        2. Switch DDS to specific slot.
        3. Check HTTP connection after DDS switch.
        4. Set up phones in desired RAT.
        5. Register and enable call forwarding with specifc type.
        5. Make voice call to the primary device and wait for being forwarded
           to 3rd device.

        Args:
            caller_slot: Slot of 2nd device making MO call (0 or 1)
            callee_slot: Slot of primary device receiving and forwarding MT call
                         (0 or 1)
            forwarded_callee_slot: Slot of 3rd device receiving forwarded call.
            dds_slot: Preferred data slot
            caller_rat: RAT for both slots of the 2nd device
            callee_rat: RAT for both slots of the primary device
            forwarded_callee_rat: RAT for both slots of the 3rd device
            call_forwarding_type:
                "unconditional"
                "busy"
                "not_answered"
                "not_reachable"

        Returns:
            True or False
        """
        ads = self.android_devices

        ad_caller = ads[1]
        ad_callee = ads[0]
        ad_forwarded_callee = ads[2]

        if callee_slot is not None:
            callee_sub_id = get_subid_from_slot_index(
                self.log, ad_callee, callee_slot)
            if callee_sub_id == INVALID_SUB_ID:
                ad_callee.log.warning(
                    "Failed to get sub ID at slot %s.", callee_slot)
                return False
            callee_other_sub_id = get_subid_from_slot_index(
                self.log, ad_callee, 1-callee_slot)
            set_voice_sub_id(ad_callee, callee_sub_id)
        else:
            callee_sub_id, _, _ = get_subid_on_same_network_of_host_ad(ads)
            if callee_sub_id == INVALID_SUB_ID:
                ad_callee.log.warning(
                    "Failed to get sub ID at slot %s.", callee_slot)
                return False
            callee_slot = "auto"
            set_voice_sub_id(ad_callee, callee_sub_id)
        ad_callee.log.info(
            "Sub ID for incoming call at slot %s: %s",
            callee_slot, get_incoming_voice_sub_id(ad_callee))

        if caller_slot is not None:
            caller_sub_id = get_subid_from_slot_index(
                self.log, ad_caller, caller_slot)
            if caller_sub_id == INVALID_SUB_ID:
                ad_caller.log.warning(
                    "Failed to get sub ID at slot %s.", caller_slot)
                return False
            caller_other_sub_id = get_subid_from_slot_index(
                self.log, ad_caller, 1-caller_slot)
            set_voice_sub_id(ad_caller, caller_sub_id)
        else:
            _, caller_sub_id, _ = get_subid_on_same_network_of_host_ad(ads)
            if caller_sub_id == INVALID_SUB_ID:
                ad_caller.log.warning(
                    "Failed to get sub ID at slot %s.", caller_slot)
                return False
            caller_slot = "auto"
            set_voice_sub_id(ad_caller, caller_sub_id)
        ad_caller.log.info(
            "Sub ID for outgoing call at slot %s: %s",
            caller_slot, get_outgoing_voice_sub_id(ad_caller))

        if forwarded_callee_slot is not None:
            forwarded_callee_sub_id = get_subid_from_slot_index(
                self.log, ad_forwarded_callee, forwarded_callee_slot)
            if forwarded_callee_sub_id == INVALID_SUB_ID:
                ad_forwarded_callee.log.warning(
                    "Failed to get sub ID at slot %s.", forwarded_callee_slot)
                return False
            forwarded_callee_other_sub_id = get_subid_from_slot_index(
                self.log, ad_forwarded_callee, 1-forwarded_callee_slot)
            set_voice_sub_id(
                ad_forwarded_callee, forwarded_callee_sub_id)
        else:
            _, _, forwarded_callee_sub_id = \
                get_subid_on_same_network_of_host_ad(ads)
            if forwarded_callee_sub_id == INVALID_SUB_ID:
                ad_forwarded_callee.log.warning(
                    "Failed to get sub ID at slot %s.", forwarded_callee_slot)
                return False
            forwarded_callee_slot = "auto"
            set_voice_sub_id(
                ad_forwarded_callee, forwarded_callee_sub_id)
        ad_forwarded_callee.log.info(
            "Sub ID for incoming call at slot %s: %s",
            forwarded_callee_slot,
            get_incoming_voice_sub_id(ad_forwarded_callee))

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

        if caller_slot == 1:
            phone_setup_on_rat(
                self.log,
                ad_caller,
                caller_rat[0],
                caller_other_sub_id)

        elif caller_slot == 0:
            phone_setup_on_rat(
                self.log,
                ad_caller,
                caller_rat[1],
                caller_other_sub_id)
        else:
            phone_setup_on_rat(
                self.log,
                ad_caller,
                'general')

        if callee_slot == 1:
            phone_setup_on_rat(
                self.log,
                ad_callee,
                callee_rat[0],
                callee_other_sub_id)

        elif callee_slot == 0:
            phone_setup_on_rat(
                self.log,
                ad_callee,
                callee_rat[1],
                callee_other_sub_id)
        else:
            phone_setup_on_rat(
                self.log,
                ad_callee,
                'general')

        if forwarded_callee_slot == 1:
            phone_setup_on_rat(
                self.log,
                ad_forwarded_callee,
                forwarded_callee_rat[0],
                forwarded_callee_other_sub_id)

        elif forwarded_callee_slot == 0:
            phone_setup_on_rat(
                self.log,
                ad_forwarded_callee,
                forwarded_callee_rat[1],
                forwarded_callee_other_sub_id)
        else:
            phone_setup_on_rat(
                self.log,
                ad_forwarded_callee,
                'general')

        if caller_slot == 0 or caller_slot == 1:
            caller_phone_setup_func = phone_setup_on_rat(
                self.log, ad_caller, caller_rat[caller_slot], only_return_fn=True)
        else:
            caller_phone_setup_func = phone_setup_on_rat(
                self.log, ad_caller, 'general', only_return_fn=True)

        callee_phone_setup_func = phone_setup_on_rat(
            self.log, ad_callee, callee_rat[callee_slot], only_return_fn=True)

        if forwarded_callee_slot == 0 or forwarded_callee_slot == 1:
            forwarded_callee_phone_setup_func = phone_setup_on_rat(
                self.log,
                ad_forwarded_callee,
                forwarded_callee_rat[forwarded_callee_slot],
                only_return_fn=True)
        else:
            forwarded_callee_phone_setup_func = phone_setup_on_rat(
                self.log,
                ad_forwarded_callee,
                'general',
                only_return_fn=True)

        self.log.info("Step 3: Set up phones in desired RAT.")
        tasks = [(caller_phone_setup_func, (self.log, ad_caller, caller_sub_id)),
                 (callee_phone_setup_func, (self.log, ad_callee, callee_sub_id)),
                 (forwarded_callee_phone_setup_func,
                 (self.log, ad_forwarded_callee, forwarded_callee_sub_id))]
        if not multithread_func(self.log, tasks):
            self.log.error("Phone Failed to Set Up Properly.")
            self.tel_logger.set_result(CallResult("CALL_SETUP_FAILURE"))
            raise signals.TestFailure("Failed",
                extras={"fail_reason": "Phone Failed to Set Up Properly."})

        is_callee_in_call = is_phone_in_call_on_rat(
            self.log, ad_callee, callee_rat[callee_slot], only_return_fn=True)

        is_call_waiting = re.search(
            "call_waiting (True (\d)|False)", call_forwarding_type, re.I)
        if is_call_waiting:
            if is_call_waiting.group(1) == "False":
                call_waiting = False
                scenario = None
            else:
                call_waiting = True
                scenario = int(is_call_waiting.group(2))

            self.log.info(
                "Step 4: Make voice call with call waiting enabled = %s.",
                call_waiting)
            result = three_phone_call_waiting_short_seq(
                self.log,
                ads[0],
                None,
                is_callee_in_call,
                ads[1],
                ads[2],
                call_waiting=call_waiting, scenario=scenario)
        else:
            self.log.info(
                "Step 4: Make voice call with call forwarding %s.",
                call_forwarding_type)
            result = three_phone_call_forwarding_short_seq(
                self.log,
                ads[0],
                None,
                is_callee_in_call,
                ads[1],
                ads[2],
                call_forwarding_type=call_forwarding_type)

        if not result:
            if is_call_waiting:
                pass
            else:
                self.log.error(
                    "Failed to make MO call from %s slot %s to %s slot %s"
                    " and forward to %s slot %s",
                    ad_caller.serial,
                    caller_slot,
                    ad_callee.serial,
                    callee_slot,
                    ad_forwarded_callee.serial,
                    forwarded_callee_slot)

        return result

    def _test_msim_call_voice_conf(
            self,
            host_slot,
            p1_slot,
            p2_slot,
            dds_slot,
            host_rat=["volte", "volte"],
            p1_rat="",
            p2_rat="",
            merge=True,
            disable_cw=False):
        """Make a voice conference call at specific slot in specific RAT with
        DDS at specific slot.

        Test step:
        1. Get sub IDs of specific slots of both MO and MT devices.
        2. Switch DDS to specific slot.
        3. Check HTTP connection after DDS switch.
        4. Set up phones in desired RAT and make 3-way voice call.
        5. Swap calls.
        6. Merge calls.

        Args:
            host_slot: Slot on the primary device to host the comference call.
            0 or 1 (0 for pSIM or 1 for eSIM)
            p1_slot: Slot on the participant device for the call
            p2_slot: Slot on another participant device for the call
            dds_slot: Preferred data slot
            host_rat: RAT for both slots of the primary device
            p1_rat: RAT for both slots of the participant device
            p2_rat: RAT for both slots of another participant device
            merge: True for merging 2 calls into the conference call. False for
            not merging 2 separated call.
            disable_cw: True for disabling call waiting and False on the
            contrary.

        Returns:
            True of False
        """
        ads = self.android_devices
        ad_host = ads[0]
        ad_p1 = ads[1]
        ad_p2 = ads[2]

        if host_slot is not None:
            host_sub_id = get_subid_from_slot_index(
                self.log, ad_host, host_slot)
            if host_sub_id == INVALID_SUB_ID:
                ad_host.log.warning("Failed to get sub ID at slot.", host_slot)
                return False
            host_other_sub_id = get_subid_from_slot_index(
                self.log, ad_host, 1-host_slot)
            set_voice_sub_id(ad_host, host_sub_id)
        else:
            host_sub_id, _, _ = get_subid_on_same_network_of_host_ad(ads)
            if host_sub_id == INVALID_SUB_ID:
                ad_host.log.warning("Failed to get sub ID at slot.", host_slot)
                return False
            host_slot = "auto"
            set_voice_sub_id(ad_host, host_sub_id)

        ad_host.log.info("Sub ID for outgoing call at slot %s: %s",
            host_slot, get_outgoing_voice_sub_id(ad_host))

        if p1_slot is not None:
            p1_sub_id = get_subid_from_slot_index(self.log, ad_p1, p1_slot)
            if p1_sub_id == INVALID_SUB_ID:
                ad_p1.log.warning("Failed to get sub ID at slot %s.", p1_slot)
                return False
            set_voice_sub_id(ad_p1, p1_sub_id)
        else:
            _, p1_sub_id, _ = get_subid_on_same_network_of_host_ad(ads)
            if p1_sub_id == INVALID_SUB_ID:
                ad_p1.log.warning("Failed to get sub ID at slot %s.", p1_slot)
                return False
            p1_slot = "auto"
            set_voice_sub_id(ad_p1, p1_sub_id)
        ad_p1.log.info("Sub ID for incoming call at slot %s: %s",
            p1_slot, get_incoming_voice_sub_id(ad_p1))

        if p2_slot is not None:
            p2_sub_id = get_subid_from_slot_index(self.log, ad_p2, p2_slot)
            if p2_sub_id == INVALID_SUB_ID:
                ad_p2.log.warning("Failed to get sub ID at slot %s.", p2_slot)
                return False
            set_voice_sub_id(ad_p2, p2_sub_id)
        else:
            _, _, p2_sub_id = get_subid_on_same_network_of_host_ad(ads)
            if p2_sub_id == INVALID_SUB_ID:
                ad_p2.log.warning("Failed to get sub ID at slot %s.", p2_slot)
                return False
            p2_slot = "auto"
            set_voice_sub_id(ad_p2, p2_sub_id)
        ad_p2.log.info("Sub ID for incoming call at slot %s: %s",
            p2_slot, get_incoming_voice_sub_id(ad_p2))

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

        if disable_cw:
            if not set_call_waiting(self.log, ad_host, enable=0):
                return False
        else:
            if not set_call_waiting(self.log, ad_host, enable=1):
                return False

        if host_slot == 1:
            phone_setup_on_rat(
                self.log,
                ad_host,
                host_rat[0],
                host_other_sub_id)

        elif host_slot == 0:
            phone_setup_on_rat(
                self.log,
                ad_host,
                host_rat[1],
                host_other_sub_id)

        host_phone_setup_func = phone_setup_on_rat(
            self.log, ad_host, host_rat[host_slot], only_return_fn=True)

        is_host_in_call = is_phone_in_call_on_rat(
            self.log, ad_host, host_rat[host_slot], only_return_fn=True)

        if p1_rat:
            p1_phone_setup_func = phone_setup_on_rat(
                self.log, ad_p1, p1_rat, only_return_fn=True)
            is_p1_in_call = is_phone_in_call_on_rat(
                self.log, ad_p1, p1_rat, only_return_fn=True)
        else:
            p1_phone_setup_func = phone_setup_on_rat(
                self.log, ad_p1, 'general', only_return_fn=True)
            is_p1_in_call = is_phone_in_call_on_rat(
                self.log, ad_p1, 'general', only_return_fn=True)

        if p2_rat:
            p2_phone_setup_func = phone_setup_on_rat(
                self.log, ad_p2, p2_rat, only_return_fn=True)
            is_p2_in_call = is_phone_in_call_on_rat(
                self.log, ad_p2, p2_rat, only_return_fn=True)
        else:
            p2_phone_setup_func = phone_setup_on_rat(
                self.log, ad_p2, 'general', only_return_fn=True)
            is_p2_in_call = is_phone_in_call_on_rat(
                self.log, ad_p2, 'general', only_return_fn=True)

        self.log.info("Step 3: Set up phone in desired RAT and make 3-way"
            " voice call.")
        call_ab_id = self._three_phone_call_mo_add_mt(
            [ad_host, ad_p1, ad_p2],
            [host_phone_setup_func, p1_phone_setup_func, p2_phone_setup_func], [
                is_host_in_call, is_p1_in_call,
                is_p2_in_call
            ])

        if call_ab_id is None:
            if disable_cw:
                set_call_waiting(self.log, ad_host, enable=1)
                if str(getattr(ad_host, "exception", None)) == \
                    "PhoneA call PhoneC failed.":
                    ads[0].log.info("PhoneA failed to call PhoneC due to call"
                        " waiting being disabled.")
                    delattr(ad_host, "exception")
                    return True
            self.log.error("Failed to get call_ab_id")
            return False
        else:
            if disable_cw:
                return False

        calls = ads[0].droid.telecomCallGetCallIds()
        ads[0].log.info("Calls in PhoneA %s", calls)
        if num_active_calls(self.log, ads[0]) != 2:
            return False
        if calls[0] == call_ab_id:
            call_ac_id = calls[1]
        else:
            call_ac_id = calls[0]

        if call_ac_id is None:
            self.log.error("Failed to get call_ac_id")
            return False

        num_swaps = 2
        self.log.info("Step 4: Begin Swap x%s test.", num_swaps)
        if not swap_calls(self.log, ads, call_ab_id, call_ac_id,
                          num_swaps):
            self.log.error("Swap test failed.")
            return False

        if not merge:
            result = True
            if not self._hangup_call(ads[1], "PhoneB"):
                result =  False
            if not self._hangup_call(ads[2], "PhoneC"):
                result =  False
            return result
        else:
            self.log.info("Step 5: Merge calls.")
            if host_rat[host_slot] == "volte":
                return self._test_ims_conference_merge_drop_second_call_from_participant(
                    call_ab_id, call_ac_id)
            else:
                return self._test_wcdma_conference_merge_drop(
                    call_ab_id, call_ac_id)

    @test_tracker_info(uuid="ccaeff83-4b8c-488a-8c7f-6bb019528bf8")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_call_forwarding_unconditional_volte_psim_dds_slot_0(self):
        return self._test_msim_call_forwarding(
            None,
            0,
            None,
            0,
            callee_rat=["volte", "volte"],
            call_forwarding_type="unconditional")

    @test_tracker_info(uuid="a132bfa6-d545-4970-9a39-55aea7477f8c")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_call_forwarding_unconditional_volte_psim_dds_slot_1(self):
        return self._test_msim_call_forwarding(
            None,
            0,
            None,
            1,
            callee_rat=["volte", "volte"],
            call_forwarding_type="unconditional")

    @test_tracker_info(uuid="71a4db8a-d20f-4fcb-ac5f-5fe6b9fa36f5")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_call_forwarding_unconditional_volte_esim_dds_slot_0(self):
        return self._test_msim_call_forwarding(
            None,
            1,
            None,
            0,
            callee_rat=["volte", "volte"],
            call_forwarding_type="unconditional")

    @test_tracker_info(uuid="50b064e7-4bf6-4bb3-aed1-e4d78b0b6195")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_call_forwarding_unconditional_volte_esim_dds_slot_1(self):
        return self._test_msim_call_forwarding(
            None,
            1,
            None,
            1,
            callee_rat=["volte", "volte"],
            call_forwarding_type="unconditional")



    @test_tracker_info(uuid="b1cfe07f-f4bf-49c4-95f1-f0973f32940e")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_call_forwarding_unconditional_volte_csfb_psim_dds_slot_0(self):
        return self._test_msim_call_forwarding(
            None,
            0,
            None,
            0,
            callee_rat=["volte", "csfb"],
            call_forwarding_type="unconditional")

    @test_tracker_info(uuid="668bd2c6-beee-4c38-a9e5-8b0cc5937c28")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_call_forwarding_unconditional_volte_csfb_psim_dds_slot_1(self):
        return self._test_msim_call_forwarding(
            None,
            0,
            None,
            1,
            callee_rat=["volte", "csfb"],
            call_forwarding_type="unconditional")

    @test_tracker_info(uuid="d69e86f3-f279-4cc8-8c1f-8a9dce0acfdf")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_call_forwarding_unconditional_volte_csfb_esim_dds_slot_0(self):
        return self._test_msim_call_forwarding(
            None,
            1,
            None,
            0,
            callee_rat=["volte", "csfb"],
            call_forwarding_type="unconditional")

    @test_tracker_info(uuid="6156c374-7b07-473b-84f7-45de633f9681")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_call_forwarding_unconditional_volte_csfb_esim_dds_slot_1(self):
        return self._test_msim_call_forwarding(
            None,
            1,
            None,
            1,
            callee_rat=["volte", "csfb"],
            call_forwarding_type="unconditional")



    @test_tracker_info(uuid="29e36a21-9c94-418b-8628-e601e56fb168")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_call_forwarding_unconditional_csfb_volte_psim_dds_slot_0(self):
        return self._test_msim_call_forwarding(
            None,
            0,
            None,
            0,
            callee_rat=["csfb", "volte"],
            call_forwarding_type="unconditional")

    @test_tracker_info(uuid="36ebf549-e64e-4093-bebf-c9ca56289477")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_call_forwarding_unconditional_csfb_volte_psim_dds_slot_1(self):
        return self._test_msim_call_forwarding(
            None,
            0,
            None,
            1,
            callee_rat=["csfb", "volte"],
            call_forwarding_type="unconditional")

    @test_tracker_info(uuid="cfb973d7-aa3b-4e59-9f00-501e42c99947")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_call_forwarding_unconditional_csfb_volte_esim_dds_slot_0(self):
        return self._test_msim_call_forwarding(
            None,
            1,
            None,
            0,
            callee_rat=["csfb", "volte"],
            call_forwarding_type="unconditional")

    @test_tracker_info(uuid="a347c3db-e128-4deb-9009-c8b8e8145f67")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_call_forwarding_unconditional_csfb_volte_esim_dds_slot_1(self):
        return self._test_msim_call_forwarding(
            None,
            1,
            None,
            1,
            callee_rat=["csfb", "volte"],
            call_forwarding_type="unconditional")



    @test_tracker_info(uuid="7040e929-eb1d-4dc6-a404-2c185dc8a0a0")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_call_forwarding_unconditional_csfb_psim_dds_slot_0(self):
        return self._test_msim_call_forwarding(
            None,
            0,
            None,
            0,
            callee_rat=["csfb", "csfb"],
            call_forwarding_type="unconditional")

    @test_tracker_info(uuid="b88a2ce3-74c7-41df-8114-71b6c3d0b050")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_call_forwarding_unconditional_csfb_psim_dds_slot_1(self):
        return self._test_msim_call_forwarding(
            None,
            0,
            None,
            1,
            callee_rat=["csfb", "csfb"],
            call_forwarding_type="unconditional")

    @test_tracker_info(uuid="0ffd2391-ec5a-4a48-b0a8-fceba0c922d3")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_call_forwarding_unconditional_csfb_esim_dds_slot_0(self):
        return self._test_msim_call_forwarding(
            None,
            1,
            None,
            0,
            callee_rat=["csfb", "csfb"],
            call_forwarding_type="unconditional")

    @test_tracker_info(uuid="44937439-2d0a-4aea-bb4d-263e5ed634b4")
    @TelephonyBaseTest.tel_test_wrap
    def test_msim_call_forwarding_unconditional_csfb_esim_dds_slot_1(self):
        return self._test_msim_call_forwarding(
            None,
            1,
            None,
            1,
            callee_rat=["csfb", "csfb"],
            call_forwarding_type="unconditional")



    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="73ac948b-5260-44f1-a0a6-e4a410cb3283")
    def test_msim_voice_conf_call_host_volte_psim_dds_slot_0(self):
        return self._test_msim_call_voice_conf(
            0, None, None, 0, host_rat=["volte", "volte"])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="75d7fb2c-aa62-4b4f-9e70-8f6b1647f816")
    def test_msim_voice_conf_call_host_volte_psim_dds_slot_1(self):
        return self._test_msim_call_voice_conf(
            0, None, None, 1, host_rat=["volte", "volte"])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="2343369e-0240-4adc-bc01-7c08f9327737")
    def test_msim_voice_conf_call_host_volte_esim_dds_slot_0(self):
        return self._test_msim_call_voice_conf(
            1, None, None, 0, host_rat=["volte", "volte"])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="3a28e621-1d47-432c-a7e8-20d2d9f82588")
    def test_msim_voice_conf_call_host_volte_esim_dds_slot_1(self):
        return self._test_msim_call_voice_conf(
            1, None, None, 1, host_rat=["volte", "volte"])



    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="378f24cf-bb96-45e1-8150-02f08d7417b6")
    def test_msim_voice_conf_call_host_volte_csfb_psim_dds_slot_0(self):
        return self._test_msim_call_voice_conf(
            0, None, None, 0, host_rat=["volte", "csfb"])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="e3fdf5ec-eafe-4825-acd3-5d4ff03df1d2")
    def test_msim_voice_conf_call_host_volte_csfb_psim_dds_slot_1(self):
        return self._test_msim_call_voice_conf(
            0, None, None, 1, host_rat=["volte", "csfb"])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="221da988-e8c7-43e5-ae3a-414e8f01e872")
    def test_msim_voice_conf_call_host_volte_csfb_esim_dds_slot_0(self):
        return self._test_msim_call_voice_conf(
            1, None, None, 0, host_rat=["volte", "csfb"])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="ea5f0254-59b8-4f63-8a4a-6f0ecb55ddbf")
    def test_msim_voice_conf_call_host_volte_csfb_esim_dds_slot_1(self):
        return self._test_msim_call_voice_conf(
            1, None, None, 1, host_rat=["volte", "csfb"])



    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="90abbc8a-d492-45f9-9919-fae7e44c877a")
    def test_msim_voice_conf_call_host_csfb_volte_psim_dds_slot_0(self):
        return self._test_msim_call_voice_conf(
            0, None, None, 0, host_rat=["csfb", "volte"])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="da98268a-a94a-4fc7-8fb9-8e8573baed50")
    def test_msim_voice_conf_call_host_csfb_volte_psim_dds_slot_1(self):
        return self._test_msim_call_voice_conf(
            0, None, None, 1, host_rat=["csfb", "volte"])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="df46bcf5-48a3-466f-ba37-9519f5a671cf")
    def test_msim_voice_conf_call_host_csfb_volte_esim_dds_slot_0(self):
        return self._test_msim_call_voice_conf(
            1, None, None, 0, host_rat=["csfb", "volte"])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="f0c82ae0-c659-45e3-9a00-419e2da55739")
    def test_msim_voice_conf_call_host_csfb_volte_esim_dds_slot_1(self):
        return self._test_msim_call_voice_conf(
            1, None, None, 1, host_rat=["csfb", "volte"])



    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="4831c07a-9a38-4ccd-8fa0-beaf52a2751e")
    def test_msim_voice_conf_call_host_csfb_psim_dds_slot_0(self):
        return self._test_msim_call_voice_conf(
            0, None, None, 0, host_rat=["csfb", "csfb"])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="79cbf768-88ea-4d03-b798-2097789ee456")
    def test_msim_voice_conf_call_host_csfb_psim_dds_slot_1(self):
        return self._test_msim_call_voice_conf(
            0, None, None, 1, host_rat=["csfb", "csfb"])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="68b0a15f-62e4-419d-948a-d74d763a736c")
    def test_msim_voice_conf_call_host_csfb_esim_dds_slot_0(self):
        return self._test_msim_call_voice_conf(
            1, None, None, 0, host_rat=["csfb", "csfb"])

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="a93af289-98a8-4d4b-bdbd-54478f273fea")
    def test_msim_voice_conf_call_host_csfb_esim_dds_slot_1(self):
        return self._test_msim_call_voice_conf(
            1, None, None, 1, host_rat=["csfb", "csfb"])



    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="43e450c8-8a0b-4dfc-8c59-d0865c4c6399")
    def test_msim_call_waiting_volte_psim_dds_slot_0(self):
        result = True
        if not self._test_msim_call_voice_conf(
            0,
            None,
            None,
            0,
            host_rat=["volte", "volte"],
            merge=False, disable_cw=False):
        	result = False
        if not self._test_msim_call_voice_conf(
            0,
            None,
            None,
            0,
            host_rat=["volte", "volte"],
            merge=False,
            disable_cw=True):
        	result = False
        return result

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="7d05525e-8fcf-4630-9248-22803a14209d")
    def test_msim_call_waiting_volte_psim_dds_slot_1(self):
        result = True
        if not self._test_msim_call_voice_conf(
            0,
            None,
            None,
            1,
            host_rat=["volte", "volte"],
            merge=False,
            disable_cw=False):
            result = False
        if not self._test_msim_call_voice_conf(
            0,
            None,
            None,
            1,
            host_rat=["volte", "volte"],
            merge=False,
            disable_cw=True):
            result = False
        return result

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="caec880c-948a-4fcd-b57e-e64fd3048b08")
    def test_msim_call_waiting_volte_esim_dds_slot_0(self):
        result = True
        if not self._test_msim_call_voice_conf(
            1,
            None,
            None,
            0,
            host_rat=["volte", "volte"],
            merge=False,
            disable_cw=False):
            result = False
        if not self._test_msim_call_voice_conf(
            1,
            None,
            None,
            0,
            host_rat=["volte", "volte"],
            merge=False,
            disable_cw=True):
            result = False
        return result

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="72ec685d-6c36-40cd-81fd-dd97e32b1e48")
    def test_msim_call_waiting_volte_esim_dds_slot_1(self):
        result = True
        if not self._test_msim_call_voice_conf(
            1,
            None,
            None,
            1,
            host_rat=["volte", "volte"],
            merge=False,
            disable_cw=False):
            result = False
        if not self._test_msim_call_voice_conf(
            1,
            None,
            None,
            1,
            host_rat=["volte", "volte"],
            merge=False,
            disable_cw=True):
            result = False
        return result



    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="3cef5c80-b15f-45fa-8376-5252e61d7849")
    def test_msim_call_waiting_volte_csfb_psim_dds_slot_0(self):
        result = True
        if not self._test_msim_call_voice_conf(
            0,
            None,
            None,
            0,
            host_rat=["volte", "csfb"],
            merge=False,
            disable_cw=False):
            result = False
        if not self._test_msim_call_voice_conf(
            0,
            None,
            None,
            0,
            host_rat=["volte", "csfb"],
            merge=False,
            disable_cw=True):
            result = False
        return result

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="5da5c799-5349-4cf3-b683-c7372aadfdfa")
    def test_msim_call_waiting_volte_csfb_psim_dds_slot_1(self):
        result = True
        if not self._test_msim_call_voice_conf(
            0,
            None,
            None,
            1,
            host_rat=["volte", "csfb"],
            merge=False,
            disable_cw=False):
            result = False
        if not self._test_msim_call_voice_conf(
            0,
            None,
            None,
            1,
            host_rat=["volte", "csfb"],
            merge=False,
            disable_cw=True):
            result = False
        return result

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="30c06bb3-a62f-4dba-90c2-1b00c515034a")
    def test_msim_call_waiting_volte_csfb_esim_dds_slot_0(self):
        result = True
        if not self._test_msim_call_voice_conf(
            1,
            None,
            None,
            0,
            host_rat=["volte", "csfb"],
            merge=False,
            disable_cw=False):
            result = False
        if not self._test_msim_call_voice_conf(
            1,
            None,
            None,
            0,
            host_rat=["volte", "csfb"],
            merge=False,
            disable_cw=True):
            result = False
        return result

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="d2b0fdb1-5ea6-4958-a34f-6f701801e3c9")
    def test_msim_call_waiting_volte_csfb_esim_dds_slot_1(self):
        result = True
        if not self._test_msim_call_voice_conf(
            1,
            None,
            None,
            1,
            host_rat=["volte", "csfb"],
            merge=False,
            disable_cw=False):
            result = False
        if not self._test_msim_call_voice_conf(
            1,
            None,
            None,
            1,
            host_rat=["volte", "csfb"],
            merge=False,
            disable_cw=True):
            result = False
        return result



    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="b239d4be-9a36-4791-84df-ecebae645c84")
    def test_msim_call_waiting_csfb_volte_psim_dds_slot_0(self):
        result = True
        if not self._test_msim_call_voice_conf(
            0,
            None,
            None,
            0,
            host_rat=["csfb", "volte"],
            merge=False,
            disable_cw=False):
            result = False
        if not self._test_msim_call_voice_conf(
            0,
            None,
            None,
            0,
            host_rat=["csfb", "volte"],
            merge=False,
            disable_cw=True):
            result = False
        return result

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="51a368e6-83d8-46af-8a85-56aaed787f9f")
    def test_msim_call_waiting_csfb_volte_psim_dds_slot_1(self):
        result = True
        if not self._test_msim_call_voice_conf(
            0,
            None,
            None,
            1,
            host_rat=["csfb", "volte"],
            merge=False,
            disable_cw=False):
            result = False
        if not self._test_msim_call_voice_conf(
            0,
            None,
            None,
            1,
            host_rat=["csfb", "volte"],
            merge=False,
            disable_cw=True):
            result = False
        return result

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="73646014-1ead-4bd9-bd8f-2c21da3d596a")
    def test_msim_call_waiting_csfb_volte_esim_dds_slot_0(self):
        result = True
        if not self._test_msim_call_voice_conf(
            1,
            None,
            None,
            0,
            host_rat=["csfb", "volte"],
            merge=False,
            disable_cw=False):
            result = False
        if not self._test_msim_call_voice_conf(
            1,
            None,
            None,
            0,
            host_rat=["csfb", "volte"],
            merge=False,
            disable_cw=True):
            result = False
        return result

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="0d520b78-20b8-4be7-833a-40179114cbce")
    def test_msim_call_waiting_csfb_volte_esim_dds_slot_1(self):
        result = True
        if not self._test_msim_call_voice_conf(
            1,
            None,
            None,
            1,
            host_rat=["csfb", "volte"],
            merge=False,
            disable_cw=False):
            result = False
        if not self._test_msim_call_voice_conf(
            1,
            None,
            None,
            1,
            host_rat=["csfb", "volte"],
            merge=False,
            disable_cw=True):
            result = False
        return result

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="0544abec-7a59-4de0-be45-0b9b9d706b17")
    def test_msim_call_waiting_csfb_psim_dds_slot_0(self):
        result = True
        if not self._test_msim_call_voice_conf(
            0,
            None,
            None,
            0,
            host_rat=["csfb", "csfb"],
            merge=False,
            disable_cw=False):
            result = False
        if not self._test_msim_call_voice_conf(
            0,
            None,
            None,
            0,
            host_rat=["csfb", "csfb"],
            merge=False,
            disable_cw=True):
            result = False
        return result

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="4329319b-0503-4c51-8792-2f36090b8071")
    def test_msim_call_waiting_csfb_psim_dds_slot_1(self):
        result = True
        if not self._test_msim_call_voice_conf(
            0,
            None,
            None,
            1,
            host_rat=["csfb", "csfb"],
            merge=False,
            disable_cw=False):
            result = False
        if not self._test_msim_call_voice_conf(
            0,
            None,
            None,
            1,
            host_rat=["csfb", "csfb"],
            merge=False,
            disable_cw=True):
            result = False
        return result

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="d612ce5c-b4cd-490c-bc6c-7f67c25264aa")
    def test_msim_call_waiting_csfb_esim_dds_slot_0(self):
        result = True
        if not self._test_msim_call_voice_conf(
            1,
            None,
            None,
            0,
            host_rat=["csfb", "csfb"],
            merge=False,
            disable_cw=False):
            result = False
        if not self._test_msim_call_voice_conf(
            1,
            None,
            None,
            0,
            host_rat=["csfb", "csfb"],
            merge=False,
            disable_cw=True):
            result = False
        return result

    @TelephonyBaseTest.tel_test_wrap
    @test_tracker_info(uuid="fb4869da-a346-4275-a742-d2c653bfc39a")
    def test_msim_call_waiting_csfb_esim_dds_slot_1(self):
        result = True
        if not self._test_msim_call_voice_conf(
            1,
            None,
            None,
            1,
            host_rat=["csfb", "csfb"],
            merge=False,
            disable_cw=False):
            result = False
        if not self._test_msim_call_voice_conf(
            1,
            None,
            None,
            1,
            host_rat=["csfb", "csfb"],
            merge=False,
            disable_cw=True):
            result = False
        return result