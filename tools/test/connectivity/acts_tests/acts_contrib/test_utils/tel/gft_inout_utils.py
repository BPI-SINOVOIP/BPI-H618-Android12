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


import json
import logging
import re
import os
import urllib.parse
import time
from acts.controllers import android_device

from acts_contrib.test_utils.tel.tel_test_utils import get_telephony_signal_strength
from acts_contrib.test_utils.tel.tel_test_utils import get_sim_state
from acts_contrib.test_utils.tel.tel_test_utils import get_service_state_by_adb
from acts_contrib.test_utils.tel.tel_test_utils import verify_internet_connection
from acts_contrib.test_utils.tel.tel_test_utils import verify_http_connection
from acts_contrib.test_utils.tel.tel_test_utils import start_youtube_video
from acts_contrib.test_utils.tel.tel_test_utils import hangup_call
from acts_contrib.test_utils.tel.tel_test_utils import initiate_call
from acts_contrib.test_utils.tel.tel_test_utils import wait_for_ringing_call
from acts_contrib.test_utils.tel.tel_test_utils import wait_and_answer_call
from acts_contrib.test_utils.tel.tel_test_utils import is_phone_in_call_active
from acts_contrib.test_utils.tel.tel_test_utils import get_screen_shot_log
from acts_contrib.test_utils.tel.tel_test_utils import get_screen_shot_logs
from acts_contrib.test_utils.tel.tel_test_utils import log_screen_shot
from acts_contrib.test_utils.tel.tel_test_utils import is_phone_in_call
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_iwlan
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_not_iwlan
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_volte
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_1x
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_2g
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_3g
from acts_contrib.test_utils.tel.tel_voice_utils import is_phone_in_call_csfb

from acts_contrib.test_utils.tel.tel_defines import DATA_STATE_CONNECTED
from acts_contrib.test_utils.tel.tel_defines import DATA_STATE_DISCONNECTED
from acts_contrib.test_utils.tel.tel_defines import SERVICE_STATE_EMERGENCY_ONLY
from acts_contrib.test_utils.tel.tel_defines import SERVICE_STATE_IN_SERVICE
from acts_contrib.test_utils.tel.tel_defines import SERVICE_STATE_UNKNOWN
from acts_contrib.test_utils.tel.tel_defines import SERVICE_STATE_OUT_OF_SERVICE
from acts_contrib.test_utils.tel.tel_defines import SERVICE_STATE_POWER_OFF
from acts_contrib.test_utils.tel.tel_defines import SIM_STATE_ABSENT
from acts_contrib.test_utils.tel.tel_defines import SIM_STATE_LOADED
from acts_contrib.test_utils.tel.tel_defines import SIM_STATE_NOT_READY
from acts_contrib.test_utils.tel.tel_defines import SIM_STATE_PIN_REQUIRED
from acts_contrib.test_utils.tel.tel_defines import SIM_STATE_READY
from acts_contrib.test_utils.tel.tel_defines import SIM_STATE_UNKNOWN

from acts_contrib.test_utils.tel.gft_inout_defines import VOICE_CALL
from acts_contrib.test_utils.tel.gft_inout_defines import VOLTE_CALL
from acts_contrib.test_utils.tel.gft_inout_defines import CSFB_CALL
from acts_contrib.test_utils.tel.gft_inout_defines import WFC_CALL



def check_no_service_time(ad, timeout=30):
    """ check device is no service or not

        Args:
            ad: android device
            timeout: timeout time for device back to service

        Returns:
            True if pass; False if fail.
    """
    for i in range (timeout):
        service_state = get_service_state_by_adb(ad.log,ad)
        if service_state != SERVICE_STATE_IN_SERVICE:
            ad.log.info("device becomes no/limited service in %s sec and service_state=%s" %(i+1, service_state))
            get_telephony_signal_strength(ad)
            return True
        time.sleep(1)
    get_telephony_signal_strength(ad)
    check_network_service(ad)
    ad.log.info("device does not become no/limited service in %s sec and service_state=%s" %(timeout, service_state))
    return False

def check_back_to_service_time(ad, timeout=30):
    """ check device is back to service or not

        Args:
            ad: android device
            timeout: timeout time for device back to service

        Returns:
            True if pass; False if fail.
    """
    for i in range (timeout):
        service_state = get_service_state_by_adb(ad.log,ad)
        if service_state == SERVICE_STATE_IN_SERVICE:
            if i==0:
                check_network_service(ad)
                ad.log.info("Skip check_back_to_service_time. Device is in-service and service_state=%s" %(service_state))
                return True
            else:
                ad.log.info("device is back to service in %s sec and service_state=%s" %(i+1, service_state))
                get_telephony_signal_strength(ad)
                return True
        time.sleep(1)
    get_telephony_signal_strength(ad)
    ad.log.info("device is not back in service in %s sec and service_state=%s" %(timeout, service_state))
    return False

def check_network_service(ad):
    """ check network service

        Args:
            ad: android device

        Returns:
            True if ad is in service; False if ad is not in service.
    """
    network_type_voice = ad.droid.telephonyGetCurrentVoiceNetworkType()
    network_type_data = ad.droid.telephonyGetCurrentDataNetworkType()
    service_state = get_service_state_by_adb(ad.log,ad)
    sim_state = ad.droid.telephonyGetSimState()
    ad.log.info("sim_state=%s" %(sim_state))
    ad.log.info("networkType_voice=%s" %(network_type_voice))
    ad.log.info("networkType_data=%s" %(network_type_data))
    ad.log.info("service_state=%s" %(service_state))
    if service_state == SERVICE_STATE_OUT_OF_SERVICE:
        return False
    return True

def mo_voice_call(log, ad, call_type, end_call=True, talk_time=15):
    """ MO voice call and check call type.
        End call if necessary.

        Args:
            log: log
            ad: android device
            call_type: WFC call, VOLTE call. CSFB call, voice call
            end_call: hangup call after voice call flag
            talk_time: in call duration in sec

        Returns:
            True if pass; False if fail.
    """
    callee_number = ad.mt_phone_number
    ad.log.info("MO voice call")
    if is_phone_in_call(log, ad):
        ad.log.info("%s is in call. hangup_call before initiate call" %(callee_number))
        hangup_call(log, ad)
        time.sleep(1)

    if initiate_call(log, ad, callee_number):
        time.sleep(5)
        check_voice_call_type(ad,call_type)
        get_voice_call_type(ad)
    else:
        ad.log.error("initiate_call fail")
        return False

    time.sleep(talk_time)
    if end_call:
        if is_phone_in_call(log, ad):
            ad.log.info("end voice call")
            if not hangup_call(log, ad):
                ad.log.error("end call fail")
                return False
        else:
            #Call drop is unexpected
            ad.log.error("%s Unexpected call drop" %(call_type))
            return False
        ad.log.info("%s successful" %(call_type))
    return True


def check_voice_call_type(ad, call_type):
    """ check current voice call type

        Args:
            ad: android device
            call_type: WFC call, VOLTE call. CSFB call, voice call

        Returns:
            True if pass; False if fail.
    """
    if is_phone_in_call(ad.log, ad):
        log_screen_shot(ad, "expected_call_type_%s" %call_type)
        if call_type == CSFB_CALL:
            if not is_phone_in_call_csfb(ad.log, ad):
                ad.log.error("current call is not %s" %(call_type))
                return False
            else:
                ad.log.info("current call is CSFB %s" %(call_type))
        elif call_type == WFC_CALL:
            if not is_phone_in_call_iwlan(ad.log, ad):
                ad.log.error("current call is not %s" %(call_type))
                return False
            else:
                ad.log.info("current call is VoWiFi %s" %(call_type))
        elif call_type == VOLTE_CALL:
            if not is_phone_in_call_volte(ad.log, ad):
                ad.log.error("current call is not %s" %(call_type))
                return False
            else:
                ad.log.info(" current call is VOLTE %s" %(call_type))
    else:
        ad.log.error("device is not in call")
        return False
    return True


def get_voice_call_type(ad):
    """ get current voice call type

        Args:
            ad: android device

        Returns:
            call type
    """
    if is_phone_in_call(ad.log, ad):
        if is_phone_in_call_csfb(ad.log, ad):
            ad.log.info("current call is CSFB")
            return CSFB_CALL
        elif is_phone_in_call_iwlan(ad.log, ad):
            ad.log.info("current call is VoWiFi")
            return WFC_CALL
        elif is_phone_in_call_volte(ad.log, ad):
            ad.log.info("current call is VOLTE")
            return VOLTE_CALL
    else:
        ad.log.error("device is not in call")
    return "UNKNOWN"

def verify_data_connection(ad):
    data_state = ad.droid.telephonyGetDataConnectionState()
    if data_state != DATA_STATE_CONNECTED:
        ad.log.error("data is not connected. data_state=%s" %(data_state))
        return False
    else:
        #Verify internet connection by ping test and http connection
        if not verify_internet_connection(ad.log, ad, retries=3):
            data_state = ad.droid.telephonyGetDataConnectionState()
            ad.log.error("verify_internet_connection fail. data_state=%s" %(data_state))
            return False
        ad.log.info("verify_data_connection pass")
        return True
