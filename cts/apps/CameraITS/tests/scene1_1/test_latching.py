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
"""Verifies settings latch on the correct frame."""


import logging
import os.path
import matplotlib
from matplotlib import pylab
from mobly import test_runner

import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils
import target_exposure_utils

EXP_GAIN_FACTOR = 2
NAME = os.path.splitext(os.path.basename(__file__))[0]
PATCH_H = 0.1  # center 10%
PATCH_W = 0.1
PATCH_X = 0.5 - PATCH_W/2
PATCH_Y = 0.5 - PATCH_H/2
REQ_PATTERN = ['base', 'base', 'iso', 'iso', 'base', 'base', 'exp',
               'base', 'iso', 'base', 'exp', 'base', 'exp', 'exp']
PATTERN_CHECK = [False if r == 'base' else True for r in REQ_PATTERN]


class LatchingTest(its_base_test.ItsBaseTest):
  """Test that settings latch on the right frame.

  Takes a sequence of 14 shots using back-to-back requests, varying the capture
  request gain and exp parameters between shots. Check images that come back
  have the properties.

  Pattern is described in EXP_GAIN_HIGH_PATTERN where False is NOM, True is High
  """

  def test_latching(self):
    logging.debug('Starting %s', NAME)
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      log_path = self.log_path

      # check SKIP conditions
      camera_properties_utils.skip_unless(
          camera_properties_utils.full_or_better(props))

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Create requests, do captures and extract means for each image
      _, fmt = capture_request_utils.get_fastest_manual_capture_settings(props)
      e, s = target_exposure_utils.get_target_exposure_combos(
          log_path, cam)['midExposureTime']

      e /= EXP_GAIN_FACTOR
      r_means = []
      g_means = []
      b_means = []
      reqs = []
      base_req = capture_request_utils.manual_capture_request(
          s, e, 0.0, True, props)
      iso_mult_req = capture_request_utils.manual_capture_request(
          s * EXP_GAIN_FACTOR, e, 0.0, True, props)
      exp_mult_req = capture_request_utils.manual_capture_request(
          s, e * EXP_GAIN_FACTOR, 0.0, True, props)
      for req_type in REQ_PATTERN:
        if req_type == 'base':
          reqs.append(base_req)
        elif req_type == 'exp':
          reqs.append(exp_mult_req)
        elif req_type == 'iso':
          reqs.append(iso_mult_req)
        else:
          assert 0, 'Incorrect capture request!'

      caps = cam.do_capture(reqs, fmt)
      for i, cap in enumerate(caps):
        img = image_processing_utils.convert_capture_to_rgb_image(cap)
        image_processing_utils.write_image(img, '%s_i=%02d.jpg' % (
            os.path.join(log_path, NAME), i))
        patch = image_processing_utils.get_image_patch(
            img, PATCH_X, PATCH_Y, PATCH_W, PATCH_H)
        rgb_means = image_processing_utils.compute_image_means(patch)
        r_means.append(rgb_means[0])
        g_means.append(rgb_means[1])
        b_means.append(rgb_means[2])
      logging.debug('G means: %s', str(g_means))

      # Plot results
      idxs = range(len(r_means))
      pylab.figure(NAME)
      pylab.plot(idxs, r_means, '-ro')
      pylab.plot(idxs, g_means, '-go')
      pylab.plot(idxs, b_means, '-bo')
      pylab.ylim([0, 1])
      pylab.title(NAME)
      pylab.xlabel('capture')
      pylab.ylabel('RGB means')
      matplotlib.pyplot.savefig('%s_plot_means.png' % os.path.join(
          log_path, NAME))

      # check G mean pattern for correctness
      g_avg_for_caps = sum(g_means) / len(g_means)
      g_high = [g / g_avg_for_caps > 1 for g in g_means]
      assert g_high == PATTERN_CHECK, 'G means: %s, TEMPLATE: %s' % (
          str(g_means), str(REQ_PATTERN))

if __name__ == '__main__':
  test_runner.main()

