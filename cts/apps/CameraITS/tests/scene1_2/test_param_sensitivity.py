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
"""Verifies android.sensor.sensitivity parameter is applied."""


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

COLORS = ['R', 'G', 'B']
NAME = os.path.splitext(os.path.basename(__file__))[0]
NUM_STEPS = 5
PATCH_H = 0.1  # center 10%
PATCH_W = 0.1
PATCH_X = 0.5 - PATCH_W/2
PATCH_Y = 0.5 - PATCH_H/2


class ParamSensitivityTest(its_base_test.ItsBaseTest):
  """Test that the android.sensor.sensitivity parameter is applied."""

  def test_param_sensitivity(self):
    logging.debug('Starting %s', NAME)
    sensitivities = None
    r_means = []
    g_means = []
    b_means = []

    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      log_path = self.log_path
      test_name_with_path = os.path.join(log_path, NAME)

      # check SKIP conditions
      camera_properties_utils.skip_unless(
          camera_properties_utils.compute_target_exposure(props))

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Initialize requests
      sync_latency = camera_properties_utils.sync_latency(props)
      largest_yuv = capture_request_utils.get_largest_yuv_format(props)
      match_ar = (largest_yuv['width'], largest_yuv['height'])
      fmt = capture_request_utils.get_smallest_yuv_format(
          props, match_ar=match_ar)

      expt, _ = target_exposure_utils.get_target_exposure_combos(
          log_path, cam)['midSensitivity']
      sens_range = props['android.sensor.info.sensitivityRange']
      sens_step = (sens_range[1] - sens_range[0]) / float(NUM_STEPS-1)
      sensitivities = [
          int(sens_range[0] + i * sens_step) for i in range(NUM_STEPS)]

      for s in sensitivities:
        logging.debug('Capturing with sensitivity: %d', s)
        req = capture_request_utils.manual_capture_request(s, expt)
        cap = its_session_utils.do_capture_with_latency(
            cam, req, sync_latency, fmt)
        img = image_processing_utils.convert_capture_to_rgb_image(cap)
        image_processing_utils.write_image(
            img, f'{test_name_with_path}_iso={s}.jpg')
        patch = image_processing_utils.get_image_patch(
            img, PATCH_X, PATCH_Y, PATCH_W, PATCH_H)
        rgb_means = image_processing_utils.compute_image_means(patch)
        r_means.append(rgb_means[0])
        g_means.append(rgb_means[1])
        b_means.append(rgb_means[2])

    logging.debug('R means: %s', str(r_means))
    logging.debug('G means: %s', str(g_means))
    logging.debug('B means: %s', str(b_means))

    # Draw plot
    pylab.figure(NAME)
    pylab.plot(sensitivities, r_means, '-ro')
    pylab.plot(sensitivities, g_means, '-go')
    pylab.plot(sensitivities, b_means, '-bo')
    pylab.ylim([0, 1])
    pylab.title(NAME)
    pylab.xlabel('Gain (ISO)')
    pylab.ylabel('RGB means')
    matplotlib.pyplot.savefig(f'{test_name_with_path}_plot_means.png')

    # Test for pass/fail: check that each shot is brighter than previous
    for i, means in enumerate([r_means, g_means, b_means]):
      for j in range(len(means)-1):
        if means[j+1] <= means[j]:
          raise AssertionError(f'{COLORS[i]} cap {j} means[j+1]: '
                               f'{means[j+1]:.3f}, means[j]: {means[j]:.3f}')

if __name__ == '__main__':
  test_runner.main()

