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

import time
import random
import re

from queue import Empty
from acts.utils import rand_ascii_str
from acts_contrib.test_utils.tel.tel_defines import GEN_5G
from acts_contrib.test_utils.tel.tel_defines import NETWORK_MODE_NR_LTE_GSM_WCDMA
from acts_contrib.test_utils.tel.tel_defines import NETWORK_MODE_NR_ONLY
from acts_contrib.test_utils.tel.tel_defines import WFC_MODE_CELLULAR_PREFERRED
from acts_contrib.test_utils.tel.tel_defines import WFC_MODE_WIFI_PREFERRED
from acts_contrib.test_utils.tel.tel_test_utils import set_preferred_network_mode_pref
from acts_contrib.test_utils.tel.tel_test_utils import multithread_func
from acts_contrib.test_utils.tel.tel_test_utils import ensure_wifi_connected
from acts_contrib.test_utils.tel.tel_test_utils import toggle_airplane_mode
from acts_contrib.test_utils.tel.tel_voice_utils import phone_setup_volte
from acts_contrib.test_utils.tel.tel_voice_utils import phone_setup_iwlan
from acts_contrib.test_utils.tel.tel_voice_utils import phone_setup_csfb
from acts_contrib.test_utils.tel.tel_5g_utils import is_current_network_5g_nsa
from acts_contrib.test_utils.tel.tel_5g_utils import is_current_network_5g_sa


def provision_device_for_5g(log, ads, sa_5g=False, nsa_mmwave=False):
    """Provision Devices for 5G

    Args:
        log: Log object.
        ads: android device object(s).
        sa_5g: Check for provision on sa_5G or not
        nsa_mmwave: If true, check the band of NSA network is mmWave. Default is to check sub-6.

    Returns:
        True: Device(s) are provisioned on 5G
        False: Device(s) are not provisioned on 5G
    """
    if sa_5g:
        if not provision_device_for_5g_sa(log, ads):
            return False
    else:
        if not provision_device_for_5g_nsa(log, ads, nsa_mmwave=nsa_mmwave):
            return False
    return True


def provision_device_for_5g_nsa(log, ads, nsa_mmwave=False):
    """Provision Devices for 5G NSA

    Args:
        log: Log object.
        ads: android device object(s).
        nsa_mmwave: If true, check the band of NSA network is mmWave. Default is to check sub-6.

    Returns:
        True: Device(s) are provisioned on 5G NSA
        False: Device(s) are not provisioned on 5G NSA
    """
    if isinstance(ads, list):
        # Mode Pref
        tasks = [(set_preferred_mode_for_5g, [ad]) for ad in ads]
        if not multithread_func(log, tasks):
            log.error("failed to set preferred network mode on 5g")
            return False
        # Attach
        tasks = [(is_current_network_5g_nsa, [ad, nsa_mmwave]) for ad in ads]
        if not multithread_func(log, tasks):
            log.error("phone not on 5g nsa")
            return False
        return True
    else:
        # Mode Pref
        set_preferred_mode_for_5g(ads)

        # Attach nsa5g
        if not is_current_network_5g_nsa(ads, nsa_mmwave=nsa_mmwave):
            ads.log.error("Phone not attached on nsa 5g")
            return False
        return True


def provision_both_devices_for_volte(log, ads):
    # LTE attach and enable VoLTE on both phones
    tasks = [(phone_setup_volte, (log, ads[0])),
             (phone_setup_volte, (log, ads[1]))]
    if not multithread_func(log, tasks):
        log.error("phone failed to set up in volte")
        return False
    return True


def provision_both_devices_for_csfb(log, ads):
    tasks = [(phone_setup_csfb, (log, ads[0])),
             (phone_setup_csfb, (log, ads[1]))]
    if not multithread_func(log, tasks):
        log.error("Phone Failed to Set Up in csfb.")
        return False
    return True


def provision_both_devices_for_wfc_cell_pref(log,
                                             ads,
                                             wifi_ssid,
                                             wifi_pass,
                                             apm_mode=False):
    tasks = [(phone_setup_iwlan,
              (log, ads[0], apm_mode, WFC_MODE_CELLULAR_PREFERRED,
               wifi_ssid, wifi_pass)),
             (phone_setup_iwlan,
              (log, ads[1], apm_mode, WFC_MODE_CELLULAR_PREFERRED,
               wifi_ssid, wifi_pass))]
    if not multithread_func(log, tasks):
        log.error("failed to setup in wfc_cell_pref mode")
        return False
    return True


def provision_both_devices_for_wfc_wifi_pref(log,
                                             ads,
                                             wifi_ssid,
                                             wifi_pass,
                                             apm_mode=False):
    tasks = [(phone_setup_iwlan,
              (log, ads[0], apm_mode, WFC_MODE_WIFI_PREFERRED,
               wifi_ssid, wifi_pass)),
             (phone_setup_iwlan,
              (log, ads[1], apm_mode, WFC_MODE_WIFI_PREFERRED,
               wifi_ssid, wifi_pass))]
    if not multithread_func(log, tasks):
        log.error("failed to setup in wfc_wifi_pref mode")
        return False
    return True


def disable_apm_mode_both_devices(log, ads):
    # Turn off airplane mode
    log.info("Turn off apm mode on both devices")
    tasks = [(toggle_airplane_mode, (log, ads[0], False)),
             (toggle_airplane_mode, (log, ads[1], False))]
    if not multithread_func(log, tasks):
        log.error("Failed to turn off airplane mode")
        return False
    return True


def connect_both_devices_to_wifi(log,
                                 ads,
                                 wifi_ssid,
                                 wifi_pass):
    tasks = [(ensure_wifi_connected, (log, ad, wifi_ssid, wifi_pass))
             for ad in ads]
    if not multithread_func(log, tasks):
        log.error("phone failed to connect to wifi.")
        return False
    return True


def verify_5g_attach_for_both_devices(log, ads, sa_5g=False, nsa_mmwave=False):
    """Verify the network is attached

    Args:
        log: Log object.
        ads: android device object(s).
        sa_5g: Check for verify data network type is on 5G SA or not
        nsa_mmwave: If true, check the band of NSA network is mmWave. Default is to check sub-6.

    Returns:
        True: Device(s) are attached on 5G
        False: Device(s) are not attached on 5G NSA
    """
    if sa_5g:
        # Attach
        tasks = [(is_current_network_5g_sa, [ad]) for ad in ads]
        if not multithread_func(log, tasks):
            log.error("phone not on 5g sa")
            return False
        return True
    else:
        # Attach
        tasks = [(is_current_network_5g_nsa, [ad, nsa_mmwave]) for ad in ads]
        if not multithread_func(log, tasks):
            log.error("phone not on 5g nsa")
            return False
        return True


def set_preferred_mode_for_5g(ad, sub_id=None, mode=None):
    """Set Preferred Network Mode for 5G NSA
    Args:
        ad: Android device object.
        sub_id: Subscription ID.
        mode: 5G Network Mode Type
    """
    if sub_id is None:
        sub_id = ad.droid.subscriptionGetDefaultSubId()
    if mode is None:
        mode = NETWORK_MODE_NR_LTE_GSM_WCDMA
    return set_preferred_network_mode_pref(ad.log, ad, sub_id, mode)


def provision_device_for_5g_sa(log, ads):
    """Provision Devices for 5G SA

    Args:
        log: Log object.
        ads: android device object(s).

    Returns:
        True: Device(s) are provisioned on 5G SA
        False: Device(s) are not provisioned on 5G SA
    """
    if isinstance(ads, list):
        # Mode Pref
        tasks = [(set_preferred_mode_for_5g, [ad, None, NETWORK_MODE_NR_ONLY]) for ad in ads]
        if not multithread_func(log, tasks):
            log.error("failed to set preferred network mode on 5g SA")
            return False

        tasks = [(is_current_network_5g_sa, [ad]) for ad in ads]
        if not multithread_func(log, tasks):
            log.error("phone not on 5g SA")
            return False
        return True
    else:
        # Mode Pref
        set_preferred_mode_for_5g(ads, None, NETWORK_MODE_NR_ONLY)

        if not is_current_network_5g_sa(ads):
            ads.log.error("Phone not attached on SA 5g")
            return False
        return True


def check_current_network_5g(ad, timeout=30, sa_5g=False, nsa_mmwave=False):
    """Verifies data network type is on 5G

    Args:
        ad: android device object.
        timeout: max time to wait for event
        sa_5g: Check for verify data network type is on 5G SA or not
        nsa_mmwave: If true, check the band of NSA network is mmWave. Default is to check sub-6.

    Returns:
        True: if data is on 5g
        False: if data is not on 5g
    """
    if sa_5g:
        if not is_current_network_5g_sa(ad):
            return False
    else:
        if not is_current_network_5g_nsa(ad, nsa_mmwave=nsa_mmwave, timeout=timeout):
            return False
    return True

