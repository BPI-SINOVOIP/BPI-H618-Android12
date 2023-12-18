# Copyright 2013 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Verifies android.noiseReduction.mode parameters is applied when set."""


import logging
import os.path
import matplotlib
from matplotlib import pylab
from mobly import test_runner
import numpy as np

import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils
import target_exposure_utils

COLORS = ['R', 'G', 'B']
NAME = os.path.splitext(os.path.basename(__file__))[0]
NR_MODES = {'OFF': 0, 'FAST': 1, 'HQ': 2, 'MIN': 3, 'ZSL': 4}
NR_MODES_LIST = list(NR_MODES.values())
NUM_COLORS = len(COLORS)
NUM_FRAMES_PER_MODE = 4
PATCH_H = 0.1  # center 10%
PATCH_W = 0.1
PATCH_X = 0.5 - PATCH_W/2
PATCH_Y = 0.5 - PATCH_H/2
SNR_TOLERANCE = 3  # unit in dB


class ParamNoiseReductionTest(its_base_test.ItsBaseTest):
  """Test that the android.noiseReduction.mode param is applied when set.

  Capture images with the camera dimly lit.

  Capture images with low gain and noise redcution off, and use the
  variance of these captures as the baseline.

  Use high analog gain on remaining tests to ensure captured images are noisy.
  """

  def test_param_noise_reduction(self):
    logging.debug('Starting %s', NAME)
    logging.debug('NR_MODES: %s', str(NR_MODES))
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      log_path = self.log_path

      # check SKIP conditions
      camera_properties_utils.skip_unless(
          camera_properties_utils.compute_target_exposure(props) and
          camera_properties_utils.per_frame_control(props) and
          camera_properties_utils.noise_reduction_mode(props, 0))

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      snrs = [[], [], []]  # List of SNRs for R,G,B
      ref_snr = []  # Reference (baseline) SNR for each of R,G,B
      nr_modes_reported = []

      # NR mode 0 with low gain
      e, s = target_exposure_utils.get_target_exposure_combos(
          log_path, cam)['minSensitivity']
      req = capture_request_utils.manual_capture_request(s, e)
      req['android.noiseReduction.mode'] = 0
      cap = cam.do_capture(req)
      rgb_image = image_processing_utils.convert_capture_to_rgb_image(cap)
      image_processing_utils.write_image(
          rgb_image, '%s_low_gain.jpg' % os.path.join(log_path, NAME))
      rgb_patch = image_processing_utils.get_image_patch(
          rgb_image, PATCH_X, PATCH_Y, PATCH_W, PATCH_H)
      ref_snr = image_processing_utils.compute_image_snrs(rgb_patch)
      logging.debug('Ref SNRs: %s', str(ref_snr))

      e, s = target_exposure_utils.get_target_exposure_combos(
          log_path, cam)['maxSensitivity']
      for mode in NR_MODES_LIST:
        # Skip unavailable modes
        if not camera_properties_utils.noise_reduction_mode(props, mode):
          nr_modes_reported.append(mode)
          for channel in range(NUM_COLORS):
            snrs[channel].append(0)
          continue

        rgb_snr_list = []
        # Capture several images to account for per frame noise variations
        for n in range(NUM_FRAMES_PER_MODE):
          req = capture_request_utils.manual_capture_request(s, e)
          req['android.noiseReduction.mode'] = mode
          cap = cam.do_capture(req)
          rgb_image = image_processing_utils.convert_capture_to_rgb_image(cap)
          if n == 0:
            nr_modes_reported.append(
                cap['metadata']['android.noiseReduction.mode'])
            image_processing_utils.write_image(
                rgb_image, '%s_high_gain_nr=%d.jpg' % (
                    os.path.join(log_path, NAME), mode))
          rgb_patch = image_processing_utils.get_image_patch(
              rgb_image, PATCH_X, PATCH_Y, PATCH_W, PATCH_H)
          rgb_snrs = image_processing_utils.compute_image_snrs(rgb_patch)
          rgb_snr_list.append(rgb_snrs)

        r_snrs = [rgb[0] for rgb in rgb_snr_list]
        g_snrs = [rgb[1] for rgb in rgb_snr_list]
        b_snrs = [rgb[2] for rgb in rgb_snr_list]
        rgb_snrs = [np.mean(r_snrs), np.mean(g_snrs), np.mean(b_snrs)]
        logging.debug('NR mode %s SNRs', mode)
        logging.debug('R SNR: %.2f, Min: %.2f, Max: %.2f',
                      rgb_snrs[0], min(r_snrs), max(r_snrs))
        logging.debug('G SNR: %.2f, Min: %.2f, Max: %.2f',
                      rgb_snrs[1], min(g_snrs), max(g_snrs))
        logging.debug('B SNR: %.2f, Min: %.2f, Max: %.2f',
                      rgb_snrs[2], min(b_snrs), max(b_snrs))

        for chan in range(NUM_COLORS):
          snrs[chan].append(rgb_snrs[chan])

    # Draw plot
    pylab.figure(NAME)
    for j in range(NUM_COLORS):
      pylab.plot(NR_MODES_LIST, snrs[j], '-'+'rgb'[j]+'o')
    pylab.xlabel('Noise Reduction Mode')
    pylab.ylabel('SNR (dB)')
    pylab.xticks(NR_MODES_LIST)
    matplotlib.pyplot.savefig('%s_plot_SNRs.png' % os.path.join(log_path, NAME))

    assert nr_modes_reported == NR_MODES_LIST

    for j in range(NUM_COLORS):
      # Higher SNR is better
      # Verify OFF is not better than FAST
      e_msg = '%s OFF: %.3f, FAST: %.3f, TOL: %.3f' % (
          COLORS[j], snrs[j][NR_MODES['OFF']], snrs[j][NR_MODES['FAST']],
          SNR_TOLERANCE)
      assert (snrs[j][NR_MODES['OFF']] < snrs[j][NR_MODES['FAST']] +
              SNR_TOLERANCE), e_msg

      # Verify FAST is not better than HQ
      e_msg = '%s FAST: %.3f, HQ: %.3f, TOL: %.3f' % (
          COLORS[j], snrs[j][NR_MODES['FAST']], snrs[j][NR_MODES['HQ']],
          SNR_TOLERANCE)
      assert (snrs[j][NR_MODES['FAST']] < snrs[j][NR_MODES['HQ']] +
              SNR_TOLERANCE), e_msg

      # Verify HQ is better than OFF
      e_msg = '%s OFF: %.3f, HQ: %.3f' % (
          COLORS[j], snrs[j][NR_MODES['OFF']], snrs[j][NR_MODES['HQ']])
      assert snrs[j][NR_MODES['HQ']] > snrs[j][NR_MODES['OFF']], e_msg

      if camera_properties_utils.noise_reduction_mode(props, NR_MODES['MIN']):
        # Verify OFF is not better than MINIMAL
        e_msg = '%s OFF: %.3f, MIN: %.3f, TOL: %.3f' % (
            COLORS[j], snrs[j][NR_MODES['OFF']], snrs[j][NR_MODES['MIN']],
            SNR_TOLERANCE)
        assert (snrs[j][NR_MODES['OFF']] < snrs[j][NR_MODES['MIN']] +
                SNR_TOLERANCE), e_msg

        # Verify MINIMAL is not better than HQ
        e_msg = '%s MIN: %.3f, HQ: %.3f, TOL: %.3f' % (
            COLORS[j], snrs[j][NR_MODES['MIN']], snrs[j][NR_MODES['HQ']],
            SNR_TOLERANCE)
        assert (snrs[j][NR_MODES['MIN']] < snrs[j][NR_MODES['HQ']] +
                SNR_TOLERANCE), e_msg

        if camera_properties_utils.noise_reduction_mode(props, NR_MODES['ZSL']):
          # Verify ZSL is close to MINIMAL
          e_msg = '%s ZSL: %.3f, MIN: %.3f, TOL: %.3f' % (
              COLORS[j], snrs[j][NR_MODES['ZSL']], snrs[j][NR_MODES['MIN']],
              SNR_TOLERANCE)
          assert np.isclose(snrs[j][NR_MODES['ZSL']], snrs[j][NR_MODES['MIN']],
                            atol=SNR_TOLERANCE), e_msg
      elif camera_properties_utils.noise_reduction_mode(props, NR_MODES['ZSL']):
        # Verify ZSL is close to OFF
        e_msg = '%s OFF: %.3f, ZSL: %.3f, TOL: %.3f' % (
            COLORS[j], snrs[j][NR_MODES['OFF']], snrs[j][NR_MODES['ZSL']],
            SNR_TOLERANCE)
        assert np.isclose(snrs[j][NR_MODES['ZSL']], snrs[j][NR_MODES['OFF']],
                          atol=SNR_TOLERANCE), e_msg

if __name__ == '__main__':
  test_runner.main()

