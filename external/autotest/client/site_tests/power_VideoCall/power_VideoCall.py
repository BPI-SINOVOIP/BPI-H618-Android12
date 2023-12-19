# Copyright 2019 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging
import re
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.common_lib.cros import power_load_util
from autotest_lib.client.cros.input_playback import keyboard
from autotest_lib.client.cros.power import power_status
from autotest_lib.client.cros.power import power_test

class power_VideoCall(power_test.power_Test):
    """class for power_VideoCall test."""
    version = 1

    video_url = 'http://crospower.page.link/power_VideoCall'
    doc_url = 'http://doc.new'

    def initialize(self, seconds_period=20., pdash_note='',
                   force_discharge=False):
        """initialize method."""
        super(power_VideoCall, self).initialize(seconds_period=seconds_period,
                                                pdash_note=pdash_note,
                                                force_discharge=force_discharge)
        self._username = power_load_util.get_username()
        self._password = power_load_util.get_password()

    def run_once(self, duration=7200, preset=''):
        """run_once method.

        @param duration: time in seconds to display url and measure power.
        @param preset: preset of the camera record. Possible values are
                       'ultra' :  1080p30_vp9,
                       'high' :   720p30_vp9,
                       'medium' : 720p24_vp8,
                       'low' :    360p24_vp8
                       If not supplied, preset will be determined automatically.
        """

        if not preset:
            preset = self._get_camera_preset()

        extra_browser_args = self.get_extra_browser_args_for_camera_test()
        with keyboard.Keyboard() as keys,\
             chrome.Chrome(init_network_controller=True,
                           gaia_login=True,
                           username=self._username,
                           password=self._password,
                           extra_browser_args=extra_browser_args,
                           autotest_ext=True) as cr:

            # Move existing window to left half and open video page
            tab_left = cr.browser.tabs[0]
            tab_left.Activate()
            keys.press_key('alt+[')
            logging.info('Navigating left window to %s', self.video_url)
            tab_left.Navigate(self.video_url)
            tab_left.WaitForDocumentReadyStateToBeComplete()

            # We need to make sure that default camera preset was init properly
            # before changing preset or else MediaRecorder won't get torn down
            # properly. So capture the init time with the default preset and
            # then switch to appropriate preset later.
            video_init_time = power_status.VideoFpsLogger.time_until_ready(
                              tab_left, num_video=5)
            self.keyvals['video_init_time'] = video_init_time
            tab_left.EvaluateJavaScript('setPreset("%s")' % preset)

            # Wait for camera to init for the new preset.
            power_status.VideoFpsLogger.time_until_ready(tab_left, num_video=5)

            # Open Google Doc on right half
            logging.info('Navigating right window to %s', self.doc_url)
            cmd = 'chrome.windows.create({ url : "%s" });' % self.doc_url
            cr.autotest_ext.EvaluateJavaScript(cmd)
            tab_right = cr.browser.tabs[-1]
            tab_right.Activate()
            keys.press_key('alt+]')
            tab_right.WaitForDocumentReadyStateToBeComplete()
            time.sleep(5)

            self._vlog = power_status.VideoFpsLogger(tab_left,
                seconds_period=self._seconds_period,
                checkpoint_logger=self._checkpoint_logger)
            self._meas_logs.append(self._vlog)

            # Start typing number block
            self.start_measurements()
            while time.time() - self._start_time < duration:
                keys.press_key('number_block')
                self.status.refresh()
                if self.status.is_low_battery():
                    logging.info(
                        'Low battery, stop test early after %.0f minutes',
                        (time.time() - self._start_time) / 60)
                    break
            self.collect_keypress_latency(cr)

    def _get_camera_preset(self):
        """Return camera preset appropriate to hw spec.

        Preset will be determined using this logic.
        - Newer Intel Core U-series CPU with fan -> 'high'
        - AMD Ryzen CPU with fan -> 'high'
        - Above without fan -> 'medium'
        - High performance ARM -> 'medium'
        - Other Intel Core CPU -> 'medium'
        - AMD APU -> 'low'
        - Intel N-series CPU -> 'low'
        - Older ARM CPU -> 'low'
        - Other CPU -> 'low'
        """
        HIGH_IF_HAS_FAN_REGEX = r'''
            Intel[ ]Core[ ]i[357]-[6-9][0-9]{3}U|     # Intel Core i7-8650U
            Intel[ ]Core[ ]i[357]-1[0-9]{4}U|         # Intel Core i7-10510U
            AMD[ ]Ryzen[ ][357][ ][3-9][0-9]{3}C|     # AMD Ryzen 7 3700C
            Genuine[ ]Intel[ ]0000                    # Unrelease CPU
        '''
        MEDIUM_REGEX = r'''
            Intel[ ]Core[ ][im][357]-[0-9]{4,5}[UY]|  # Intel Core i5-8200Y
            Intel[ ]Core[ ][im][357]-[67]Y[0-9]{2}|   # Intel Core m7-6Y75
            Intel[ ]Pentium[ ][0-9]{4,5}[UY]|         # Intel Pentium 6405U
            Intel[ ]Celeron[ ][0-9]{4,5}[UY]|         # Intel Celeron 5205U
            qcom[ ]sc[0-9]{4}|                        # qcom sc7180
            mediatek[ ]mt819[0-9]                     # mediatek mt8192
        '''
        cpu_name = utils.get_cpu_name()

        if re.search(HIGH_IF_HAS_FAN_REGEX, cpu_name, re.VERBOSE):
            if power_status.has_fan():
                return 'high'
            return 'medium'

        if re.search(MEDIUM_REGEX, cpu_name, re.VERBOSE):
            return 'medium'

        return 'low'
