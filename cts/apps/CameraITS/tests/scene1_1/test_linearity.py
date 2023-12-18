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
"""Verifies linear behavior in exposure/gain space."""


import logging
import math
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

NAME = os.path.splitext(os.path.basename(__file__))[0]
NUM_STEPS = 6
PATCH_H = 0.1  # center 10% patch params
PATCH_W = 0.1
PATCH_X = 0.5 - PATCH_W/2
PATCH_Y = 0.5 - PATCH_H/2
RESIDUAL_THRESH = 0.0003  # sample error of ~2/255 in np.arange(0, 0.5, 0.1)
VGA_W, VGA_H = 640, 480

# HAL3.2 spec requires curves up to 64 control points in length be supported
L = 63
GAMMA_LUT = np.array(
    sum([[i/L, math.pow(i/L, 1/2.2)] for i in range(L+1)], []))
INV_GAMMA_LUT = np.array(
    sum([[i/L, math.pow(i/L, 2.2)] for i in range(L+1)], []))


class LinearityTest(its_base_test.ItsBaseTest):
  """Test that device processing can be inverted to linear pixels.

  Captures a sequence of shots with the device pointed at a uniform
  target. Attempts to invert all the ISP processing to get back to
  linear R,G,B pixel data.
  """

  def test_linearity(self):
    logging.debug('Starting %s', NAME)
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      camera_properties_utils.skip_unless(
          camera_properties_utils.compute_target_exposure(props))
      sync_latency = camera_properties_utils.sync_latency(props)

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Determine sensitivities to test over
      e_mid, s_mid = target_exposure_utils.get_target_exposure_combos(
          self.log_path, cam)['midSensitivity']
      sens_range = props['android.sensor.info.sensitivityRange']
      sensitivities = [s_mid*x/NUM_STEPS for x in range(1, NUM_STEPS)]
      sensitivities = [s for s in sensitivities
                       if s > sens_range[0] and s < sens_range[1]]

      # Initialize capture request
      req = capture_request_utils.manual_capture_request(0, e_mid)
      req['android.blackLevel.lock'] = True
      req['android.tonemap.mode'] = 0
      req['android.tonemap.curve'] = {'red': GAMMA_LUT.tolist(),
                                      'green': GAMMA_LUT.tolist(),
                                      'blue': GAMMA_LUT.tolist()}
      # Do captures and calculate center patch RGB means
      r_means = []
      g_means = []
      b_means = []
      fmt = {'format': 'yuv', 'width': VGA_W, 'height': VGA_H}
      for sens in sensitivities:
        req['android.sensor.sensitivity'] = sens
        cap = its_session_utils.do_capture_with_latency(
            cam, req, sync_latency, fmt)
        img = image_processing_utils.convert_capture_to_rgb_image(cap)
        img_name = '%s_sens=%.04d.jpg' % (
            os.path.join(self.log_path, NAME), sens)
        image_processing_utils.write_image(img, img_name)
        img = image_processing_utils.apply_lut_to_image(
            img, INV_GAMMA_LUT[1::2] * L)
        patch = image_processing_utils.get_image_patch(
            img, PATCH_X, PATCH_Y, PATCH_W, PATCH_H)
        rgb_means = image_processing_utils.compute_image_means(patch)
        r_means.append(rgb_means[0])
        g_means.append(rgb_means[1])
        b_means.append(rgb_means[2])

      # Plot means
      pylab.figure(NAME)
      pylab.plot(sensitivities, r_means, '-ro')
      pylab.plot(sensitivities, g_means, '-go')
      pylab.plot(sensitivities, b_means, '-bo')
      pylab.title(NAME)
      pylab.xlim([sens_range[0], sens_range[1]/2])
      pylab.ylim([0, 1])
      pylab.xlabel('sensitivity(ISO)')
      pylab.ylabel('RGB avg [0, 1]')
      matplotlib.pyplot.savefig(
          '%s_plot_means.png' % os.path.join(self.log_path, NAME))

      # Assert plot curves are linear w/ + slope by examining polyfit residual
      for means in [r_means, g_means, b_means]:
        line, residuals, _, _, _ = np.polyfit(
            range(len(sensitivities)), means, 1, full=True)
        logging.debug('Line: m=%f, b=%f, resid=%f',
                      line[0], line[1], residuals[0])
        msg = 'residual: %.5f, THRESH: %.4f' % (residuals[0], RESIDUAL_THRESH)
        assert residuals[0] < RESIDUAL_THRESH, msg
        assert line[0] > 0, 'slope %.6f less than 0!' % line[0]

if __name__ == '__main__':
  test_runner.main()
