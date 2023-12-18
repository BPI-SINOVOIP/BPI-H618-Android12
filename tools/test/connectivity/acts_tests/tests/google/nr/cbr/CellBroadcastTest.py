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
    Test Script for CellBroadcast Module Test
"""

import xml.etree.ElementTree as ET
import time

from acts import signals
from acts.test_decorators import test_tracker_info
from acts.utils import load_config
from acts_contrib.test_utils.tel.TelephonyBaseTest import TelephonyBaseTest
from acts_contrib.test_utils.tel.tel_defines import CARRIER_TEST_CONF_XML_PATH
from acts_contrib.test_utils.tel.tel_defines import UAE
from acts_contrib.test_utils.tel.tel_defines import JAPAN_KDDI
from acts_contrib.test_utils.tel.tel_defines import NEWZEALAND
from acts_contrib.test_utils.tel.tel_defines import HONGKONG
from acts_contrib.test_utils.tel.tel_defines import CHILE
from acts_contrib.test_utils.tel.tel_defines import PERU
from acts_contrib.test_utils.tel.tel_defines import KOREA
from acts_contrib.test_utils.tel.tel_defines import TAIWAN
from acts_contrib.test_utils.tel.tel_defines import CANADA
from acts_contrib.test_utils.tel.tel_defines import BRAZIL
from acts_contrib.test_utils.tel.tel_defines import COLUMBIA
from acts_contrib.test_utils.tel.tel_defines import EQUADOR
from acts_contrib.test_utils.tel.tel_defines import PUERTORICO
from acts_contrib.test_utils.tel.tel_defines import NETHERLANDS
from acts_contrib.test_utils.tel.tel_defines import ROMANIA
from acts_contrib.test_utils.tel.tel_defines import ESTONIA
from acts_contrib.test_utils.tel.tel_defines import LITHUANIA
from acts_contrib.test_utils.tel.tel_defines import LATVIA
from acts_contrib.test_utils.tel.tel_defines import GREECE
from acts_contrib.test_utils.tel.tel_defines import ITALY
from acts_contrib.test_utils.tel.tel_defines import SOUTHAFRICA
from acts_contrib.test_utils.tel.tel_defines import UK
from acts_contrib.test_utils.tel.tel_defines import ISRAEL
from acts_contrib.test_utils.tel.tel_defines import OMAN
from acts_contrib.test_utils.tel.tel_defines import JAPAN_SOFTBANK
from acts_contrib.test_utils.tel.tel_defines import SAUDIARABIA
from acts_contrib.test_utils.tel.tel_defines import MAIN_ACTIVITY
from acts_contrib.test_utils.tel.tel_defines import CBR_PACKAGE
from acts_contrib.test_utils.tel.tel_defines import CBR_ACTIVITY
from acts_contrib.test_utils.tel.tel_defines import CBR_TEST_APK
from acts_contrib.test_utils.tel.tel_defines import MCC_MNC
from acts_contrib.test_utils.tel.tel_defines import IMSI
from acts_contrib.test_utils.tel.tel_defines import PLMN_ADB_PROPERTY
from acts_contrib.test_utils.tel.tel_defines import WAIT_TIME_FOR_ALERTS_TO_POPULATE
from acts_contrib.test_utils.tel.tel_defines import WAIT_TIME_FOR_UI
from acts_contrib.test_utils.tel.tel_defines import SCROLL_DOWN
from acts_contrib.test_utils.tel.tel_test_utils import reboot_device
from acts_contrib.test_utils.tel.tel_test_utils import log_screen_shot
from acts_contrib.test_utils.tel.tel_test_utils import get_screen_shot_log
from acts_contrib.test_utils.net import ui_utils as uutils


class CellBroadcastTest(TelephonyBaseTest):
    def setup_class(self):
        super().setup_class()
        self.region_plmn_list_conf = self.user_params.get("region_plmn_list")
        self.emergency_alert_settings_conf = self.user_params.get("emergency_alert_settings")
        self.region_plmn_dict = load_config(self.region_plmn_list_conf)
        self.emergency_alert_settings_dict = load_config(self.emergency_alert_settings_conf)
        self._verify_cbr_test_apk_install(self.android_devices[0])

    def setup_test(self):
        TelephonyBaseTest.setup_test(self)
        self.number_of_devices = 1

    def teardown_class(self):
        TelephonyBaseTest.teardown_class(self)


    def _verify_cbr_test_apk_install(self, ad):
        if not ad.is_apk_installed(CBR_TEST_APK):
            cbrtestapk = self.user_params.get("cbrtestapk")
            ad.adb.install("%s" % cbrtestapk)
        else:
            ad.log.debug("%s apk already installed", CBR_TEST_APK)


    def _verify_device_in_specific_region(self, ad, region=None):
        mccmnc = self.region_plmn_dict[region][MCC_MNC]
        current_plmn = ad.adb.getprop(PLMN_ADB_PROPERTY)
        if current_plmn == mccmnc:
            ad.log.info("device in %s region", region.upper())
            return True
        else:
            ad.log.info("device not in %s region", region.upper())
            return False


    def _get_toggle_value(self, ad, alert_text=None):
        node = uutils.wait_and_get_xml_node(ad, timeout=30, text=alert_text)
        return node.parentNode.nextSibling.firstChild.attributes['checked'].value


    def _open_wea_settings_page(self, ad):
        ad.adb.shell("am start -a %s -n %s/%s" % (MAIN_ACTIVITY, CBR_PACKAGE, CBR_ACTIVITY))


    def _close_wea_settings_page(self, ad):
        pid = ad.adb.shell("pidof %s" % CBR_PACKAGE)
        ad.adb.shell("kill -9 %s" % pid, ignore_status=True)


    def _set_device_to_specific_region(self, ad, region=None):
        """
        Args:
            ad: AndroidDevice
            country: name of country
        """
        # fetch country codes
        mccmnc = self.region_plmn_dict[region][MCC_MNC]
        imsi = self.region_plmn_dict[region][IMSI]
        ad.log.info("setting device to %s with mccmnc %s imsi %s",
                    region.upper(), mccmnc, imsi)

        # update carrier xml file
        carrier_test_conf = self.user_params.get("carrier_test_conf")
        tree = ET.parse(carrier_test_conf)
        root = tree.getroot()
        root[1].attrib['value'] = mccmnc
        root[2].attrib['value'] = imsi
        tree.write(carrier_test_conf)

        # push carrier xml to device
        ad.adb.push("%s %s" % (carrier_test_conf, CARRIER_TEST_CONF_XML_PATH))

        # reboot device
        reboot_device(ad)
        time.sleep(WAIT_TIME_FOR_ALERTS_TO_POPULATE)

        # verify adb property
        if not self._verify_device_in_specific_region(ad, region):
            raise signals.TestSkip("unable to set device to %s region" % region.upper())
        return True


    def _verify_wea_default_settings(self, ad, region=None):
        result = True
        for key, value in self.emergency_alert_settings_dict[region].items():
            alert_text = key
            alert_value = value["default_value"]
            self._open_wea_settings_page(ad)
            # scroll till bottom
            if not uutils.has_element(ad, text=alert_text):
                for _ in range(3):
                    ad.adb.shell(SCROLL_DOWN)
                if not uutils.has_element(ad, text=alert_text):
                    ad.log.error("UI - %s missing", alert_text)
                    result = False
                    continue
            current_value = self._get_toggle_value(ad, alert_text)
            if current_value == alert_value:
                ad.log.info("UI - %s, default: %s",
                            alert_text, alert_value)
            else:
                ad.log.error("UI - %s, default: %s, expected: %s",
                             alert_text, current_value, alert_value)
                result = False
        return result


    def _verify_wea_toggle_settings(self, ad, region=None):
        result = True
        for key, value in self.emergency_alert_settings_dict[region].items():
            alert_text = key
            alert_toggle = value["toggle_avail"]
            if alert_toggle == "true":
                self._open_wea_settings_page(ad)
                if not uutils.has_element(ad, text=alert_text):
                    for _ in range(3):
                        ad.adb.shell(SCROLL_DOWN)
                    if not uutils.has_element(ad, text=alert_text):
                        ad.log.error("UI - %s missing", alert_text)
                        result = False
                        continue
                before_toggle = self._get_toggle_value(ad, alert_text)
                uutils.wait_and_click(ad, text=alert_text)
                after_toggle = self._get_toggle_value(ad, alert_text)
                if before_toggle == after_toggle:
                    for _ in range(3):
                        ad.adb.shell(SCROLL_DOWN)
                    uutils.wait_and_click(ad, text=alert_text)
                    after_toggle = self._get_toggle_value(ad, alert_text)
                    if before_toggle == after_toggle:
                        ad.log.error("UI - fail to toggle %s", alert_text)
                        result = False
                else:
                    uutils.wait_and_click(ad, text=alert_text)
                    reset_toggle = self._get_toggle_value(ad, alert_text)
                    if reset_toggle != before_toggle:
                        ad.log.error("UI - fail to reset toggle %s", alert_text)
                        result = False
                    else:
                        ad.log.info("UI - toggle verified for %s", alert_text)
        return result


    def _settings_test_flow(self, region):
        ad = self.android_devices[0]
        result = True
        self._set_device_to_specific_region(ad, region)
        time.sleep(WAIT_TIME_FOR_UI)
        if not self._verify_wea_default_settings(ad, region):
            result = False
        log_screen_shot(ad, "default_settings_for_%s" % region)
        self._close_wea_settings_page(ad)
        if not self._verify_wea_toggle_settings(ad, region):
            log_screen_shot(ad, "toggle_settings_for_%s" % region)
            result = False
        get_screen_shot_log(ad)
        self._close_wea_settings_page(ad)
        return result


    """ Tests Begin """


    @test_tracker_info(uuid="a4df03a7-2e44-4f8a-8d62-18435d92fc75")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_uae(self):
        """ Verifies Wireless Emergency Alert settings for UAE

        configures the device to UAE
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(UAE)


    @test_tracker_info(uuid="fd461335-21c0-470c-aca7-74c8ebb67711")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_japan_kddi(self):
        """ Verifies Wireless Emergency Alert settings for Japan (KDDI)

        configures the device to KDDI carrier on Japan
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(JAPAN_KDDI)


    @test_tracker_info(uuid="63806dbe3-3cce-4b03-b92c-18529f81b7c5")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_newzealand(self):
        """ Verifies Wireless Emergency Alert settings for NZ

        configures the device to NZ
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(NEWZEALAND)


    @test_tracker_info(uuid="426a295e-f64b-43f7-a0df-3959f07ff568")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_hongkong(self):
        """ Verifies Wireless Emergency Alert settings for HongKong

        configures the device to HongKong
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(HONGKONG)


    @test_tracker_info(uuid="d9e2dca2-4965-48d5-9d79-352c4ccf9e0f")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_chile(self):
        """ Verifies Wireless Emergency Alert settings for Chile

        configures the device to Chile
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(CHILE)


    @test_tracker_info(uuid="77cff297-fe3b-4b4c-b502-5324b4e91506")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_peru(self):
        """ Verifies Wireless Emergency Alert settings for Peru

        configures the device to Peru
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(PERU)


    @test_tracker_info(uuid="4c3c4e65-c624-4eba-9a81-263f4ee01e12")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_korea(self):
        """ Verifies Wireless Emergency Alert settings for Korea

        configures the device to Korea
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(KOREA)


    @test_tracker_info(uuid="fbaf258e-b596-4bfa-a20f-4b93fc4ccc4c")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_taiwan(self):
        """ Verifies Wireless Emergency Alert settings for Taiwan

        configures the device to Taiwan
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(TAIWAN)


    @test_tracker_info(uuid="3f8e4110-a7d3-4b3b-ac2b-36ea17cfc141")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_canada(self):
        """ Verifies Wireless Emergency Alert settings for Canada

        configures the device to Canada
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(CANADA)


    @test_tracker_info(uuid="fa0cd219-b0f2-4a38-8733-cd4212a954c5")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_brazil(self):
        """ Verifies Wireless Emergency Alert settings for Brazil

        configures the device to Brazil
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(BRAZIL)


    @test_tracker_info(uuid="581ecebe-9f68-4270-ab5d-182b1ee4e13b")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_columbia(self):
        """ Verifies Wireless Emergency Alert settings for Columbia

        configures the device to Columbia
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(COLUMBIA)


    @test_tracker_info(uuid="2ebfc05b-3512-4eff-9c09-5d8f49fe0b5e")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_equador(self):
        """ Verifies Wireless Emergency Alert settings for Equador

        configures the device to Equador
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(EQUADOR)


    @test_tracker_info(uuid="96628975-a23f-47f7-ab18-1aa7a7dc08b5")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_puertorico(self):
        """ Verifies Wireless Emergency Alert settings for Puertorico

        configures the device to Puertorico
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(PUERTORICO)


    @test_tracker_info(uuid="9f73f7ec-cb2a-45e5-8829-db14798dcdac")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_netherlands(self):
        """ Verifies Wireless Emergency Alert settings for Netherlands

        configures the device to Netherlands
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(NETHERLANDS)


    @test_tracker_info(uuid="b3caf3b4-3024-4431-9a7a-4982e20b178b")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_romania(self):
        """ Verifies Wireless Emergency Alert settings for Romania

        configures the device to Romania
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(ROMANIA)


    @test_tracker_info(uuid="081a5329-d23f-4df8-b472-d4f3ca5ee3c1")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_estonia(self):
        """ Verifies Wireless Emergency Alert settings for Estonia

        configures the device to Estonia
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(ESTONIA)


    @test_tracker_info(uuid="7e0d3b96-f11c-44d9-b3a3-9ce9e21bf37d")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_lithuania(self):
        """ Verifies Wireless Emergency Alert settings for Lithuania

        configures the device to Lithuania
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(LITHUANIA)


    @test_tracker_info(uuid="b40648a0-d04f-4c45-9051-76e64756ef00")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_latvia(self):
        """ Verifies Wireless Emergency Alert settings for Latvia

        configures the device to Latvia
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(LATVIA)


    @test_tracker_info(uuid="9488a6ef-2903-421d-adec-fd65df3aac60")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_greece(self):
        """ Verifies Wireless Emergency Alert settings for Greece

        configures the device to Greece
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(GREECE)


    @test_tracker_info(uuid="53cf276e-8617-45ce-b3f5-e8995b4be279")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_italy(self):
        """ Verifies Wireless Emergency Alert settings for Italy

        configures the device to Italy
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(ITALY)


    @test_tracker_info(uuid="a1a57aa8-c229-4f04-bc65-1f17688159a1")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_southafrica(self):
        """ Verifies Wireless Emergency Alert settings for SouthAfrica

        configures the device to SouthAfrica
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(SOUTHAFRICA)


    @test_tracker_info(uuid="a0ed231e-07e0-4dc8-a071-14ec7818e96f")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_uk(self):
        """ Verifies Wireless Emergency Alert settings for UK

        configures the device to UK
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(UK)


    @test_tracker_info(uuid="00c77647-0986-41f8-9202-cc0e2e51e278")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_israel(self):
        """ Verifies Wireless Emergency Alert settings for Israel

        configures the device to Israel
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(ISRAEL)


    @test_tracker_info(uuid="7f2ca9f5-31f6-4477-9383-5acd1ed2598f")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_oman(self):
        """ Verifies Wireless Emergency Alert settings for Oman

        configures the device to Oman
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(OMAN)


    @test_tracker_info(uuid="97525c27-3cba-4472-b00d-d5dabc5a2fe5")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_japan_softbank(self):
        """ Verifies Wireless Emergency Alert settings for Japan (Softbank)

        configures the device to Japan (Softbank)
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(JAPAN_SOFTBANK)


    @test_tracker_info(uuid="109494df-3ae2-4b77-ae52-fb0c22e654c8")
    @TelephonyBaseTest.tel_test_wrap
    def test_default_alert_settings_saudiarabia(self):
        """ Verifies Wireless Emergency Alert settings for SaudiArabia

        configures the device to SaudiArabia
        verifies alert names and its default values
        toggles the alert twice if available

        Returns:
            True if pass; False if fail and collects screenshot
        """
        return self._settings_test_flow(SAUDIARABIA)
