# Copyright 2020 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import logging
import os
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import constants as cros_constants
from autotest_lib.client.cros.audio import audio_helper
from autotest_lib.client.cros.audio import audio_test_data
from autotest_lib.client.cros.audio import check_quality
from autotest_lib.client.cros.audio import cmd_utils
from autotest_lib.client.cros.audio import cras_utils
from autotest_lib.client.cros.audio import sox_utils
from autotest_lib.client.cros.multimedia import audio_facade_native


CheckQualityArgsClass = collections.namedtuple(
        'args_type', ['filename', 'rate', 'channel', 'bit_width'])


class audio_AudioInputGain(audio_helper.cras_rms_test):
    """Verifies input capture gain of chrome.audio API."""
    version = 1

    ALOOP_CRAS_NODE_TYPE = 'ALSA_LOOPBACK'
    ALOOP_MODULE_NAME = 'snd-aloop'
    CAPTURE_DURATION = 1
    # 25: -20 dB
    # 75: 10 dB
    # Expected gain: 10 * sqrt(10)
    LOW_GAIN = 25
    HIGH_GAIN = 75
    EXPECTED_GAIN = 31.62
    FREQ_TOLERANCE = 1
    SECOND_PEAK_RATIO_TOLERANCE = 0.05
    GAIN_TOLERANCE = 10

    def run_once(self):
        """Entry point of this test."""
        def cras_playback_record(gain_level):
            """Do capture record at CRAS level.

            @param gain_level: The input gain level.

            @returns: A string for the recorded file path.

            """
            # Sine raw file lasts 5 seconds
            raw_path = os.path.join(self.bindir, '5SEC.raw')
            raw_file = audio_test_data.GenerateAudioTestData(
                    path=raw_path,
                    duration_secs=5,
                    frequencies=[440, 440],
                    volume_scale=0.05)

            recorded_file = os.path.join(self.resultsdir,
                                         'cras_recorded_%d.raw' % gain_level)

            # Note: we've found that a couple of seconds after Chrome is up,
            #       there may be a ~30-second-long output stream sourced from
            #       "What's New In Your Chromebook", and it plays no sound.
            #       Just ignore it and continue testing.
            p = cmd_utils.popen(cras_utils.playback_cmd(raw_file.path))
            try:
                cras_utils.capture(recorded_file,
                                   duration=self.CAPTURE_DURATION)
                # Make sure the audio is still playing.
                if p.poll() != None:
                    raise error.TestError('playback stopped')
            finally:
                cmd_utils.kill_or_log_returncode(p)
                raw_file.delete()
            return recorded_file

        # Check CRAS server is alive. If not, restart it and wait a second to
        # get server ready.
        if utils.get_service_pid('cras') == 0:
            logging.debug('CRAS server is down. Restart it.')
            utils.start_service('cras', ignore_status=True)
            time.sleep(1)

        utils.load_module(self.ALOOP_MODULE_NAME)

        try:
            with chrome.Chrome(
                    extension_paths=[cros_constants.AUDIO_TEST_EXTENSION],
                    autotest_ext=True) as cr:
                audio_facade = audio_facade_native.AudioFacadeNative(cr)
                audio_facade.set_chrome_active_node_type(
                        self.ALOOP_CRAS_NODE_TYPE, self.ALOOP_CRAS_NODE_TYPE)

                rms_value = []
                for gain in [self.LOW_GAIN, self.HIGH_GAIN]:
                    logging.debug('Start testing loopback with gain %d.', gain)
                    audio_facade.set_chrome_active_input_gain(gain)
                    recorded_file = cras_playback_record(gain)
                    args = CheckQualityArgsClass(filename=recorded_file,
                                                 rate=48000,
                                                 channel=1,
                                                 bit_width=16)
                    raw_data, rate = check_quality.read_audio_file(args)
                    checker = check_quality.QualityChecker(raw_data, rate)
                    # The highest frequency recorded would be near 24 Khz
                    # as the max sample rate is 48000 in our tests.
                    # So let's set ignore_high_freq to be 48000.
                    checker.do_spectral_analysis(ignore_high_freq=48000,
                                                 check_quality=False,
                                                 quality_params=None)
                    spectra = checker._spectrals
                    primary_freq = float(spectra[0][0][0])
                    if abs(primary_freq - 440.0) > self.FREQ_TOLERANCE:
                        raise error.TestFail(
                                'Primary freq is beyond the expectation: '
                                'got %.2f, expected 440.00, tolerance %f' %
                                        (primary_freq, self.FREQ_TOLERANCE))

                    if len(spectra[0]) > 1:
                        peak_ratio = (float(spectra[0][1][1]) /
                                float(spectra[0][0][1]))
                        if peak_ratio > self.SECOND_PEAK_RATIO_TOLERANCE:
                            raise error.TestFail(
                                    'The second peak is not negligible: '
                                    'f %.2f, peak_ratio %f (tolerance %f)' %
                                            (float(spectra[0][1][0]),
                                             peak_ratio,
                                             self.SECOND_PEAK_RATIO_TOLERANCE))

                    sox_stat = sox_utils.get_stat(input=recorded_file,
                                                  channels=1,
                                                  bits=16,
                                                  rate=48000)
                    rms_value.append(float(sox_stat.rms))
                    logging.debug('signal RMS from sox = %f', rms_value[-1])

                gain = rms_value[1] / rms_value[0]
                if abs(gain - self.EXPECTED_GAIN) > self.GAIN_TOLERANCE:
                    raise error.TestFail(
                        'Gain is beyond the expectation: '
                        'got %.2f, expected %.2f, tolerance %f' %
                                (gain, self.EXPECTED_GAIN, self.GAIN_TOLERANCE))
        finally:
            utils.stop_service('cras', ignore_status=True)
            utils.unload_module(self.ALOOP_MODULE_NAME)
            utils.start_service('cras', ignore_status=True)
