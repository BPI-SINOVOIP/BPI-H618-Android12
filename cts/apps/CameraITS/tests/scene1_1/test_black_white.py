# Copyright 2019 The Android Open Source Project
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
"""Verifies camera will produce full black & full white images."""


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

CH_FULL_SCALE = 255
CH_THRESH_BLACK = 6
CH_THRESH_WHITE = CH_FULL_SCALE - 6
CH_TOL_WHITE = 2
COLOR_PLANES = ['R', 'G', 'B']
NAME = os.path.splitext(os.path.basename(__file__))[0]
PATCH_H = 0.1
PATCH_W = 0.1
PATCH_X = 0.45
PATCH_Y = 0.45
VGA_WIDTH, VGA_HEIGHT = 640, 480


def do_img_capture(cam, s, e, fmt, latency, cap_name, log_path):
  """Do the image captures with the defined parameters.

  Args:
    cam: its_session open for camera
    s: sensitivity for request
    e: exposure in ns for request
    fmt: format of request
    latency: number of frames for sync latency of request
    cap_name: string to define the capture
    log_path: path for plot directory

  Returns:
    means values of center patch from capture
  """

  req = capture_request_utils.manual_capture_request(s, e)
  cap = its_session_utils.do_capture_with_latency(cam, req, latency, fmt)
  img = image_processing_utils.convert_capture_to_rgb_image(cap)
  image_processing_utils.write_image(
      img, '%s_%s.jpg' % (os.path.join(log_path, NAME), cap_name))
  patch = image_processing_utils.get_image_patch(
      img, PATCH_X, PATCH_Y, PATCH_W, PATCH_H)
  means = image_processing_utils.compute_image_means(patch)
  means = [m * CH_FULL_SCALE for m in means]
  logging.debug('%s pixel means: %s', cap_name, str(means))
  r_exp = cap['metadata']['android.sensor.exposureTime']
  r_iso = cap['metadata']['android.sensor.sensitivity']
  logging.debug('%s shot write values: sens = %d, exp time = %.4fms',
                cap_name, s, (e / 1000000.0))
  logging.debug('%s shot read values: sens = %d, exp time = %.4fms',
                cap_name, r_iso, (r_exp / 1000000.0))
  return means


class BlackWhiteTest(its_base_test.ItsBaseTest):
  """Test that device will prodoce full black + white images.
  """

  def test_black_white(self):
    r_means = []
    g_means = []
    b_means = []

    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)

      # Check SKIP conditions
      camera_properties_utils.skip_unless(
          camera_properties_utils.manual_sensor(props))

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Initialize params for requests
      latency = camera_properties_utils.sync_latency(props)
      fmt = {'format': 'yuv', 'width': VGA_WIDTH, 'height': VGA_HEIGHT}
      expt_range = props['android.sensor.info.exposureTimeRange']
      sens_range = props['android.sensor.info.sensitivityRange']
      log_path = self.log_path

      # Take shot with very low ISO and exp time: expect it to be black
      s = sens_range[0]
      e = expt_range[0]
      black_means = do_img_capture(cam, s, e, fmt, latency, 'black', log_path)
      r_means.append(black_means[0])
      g_means.append(black_means[1])
      b_means.append(black_means[2])

      # Take shot with very high ISO and exp time: expect it to be white.
      s = sens_range[1]
      e = expt_range[1]
      white_means = do_img_capture(cam, s, e, fmt, latency, 'white', log_path)
      r_means.append(white_means[0])
      g_means.append(white_means[1])
      b_means.append(white_means[2])

      # Draw plot
      pylab.title('test_black_white')
      pylab.plot([0, 1], r_means, '-ro')
      pylab.plot([0, 1], g_means, '-go')
      pylab.plot([0, 1], b_means, '-bo')
      pylab.xlabel('Capture Number')
      pylab.ylabel('Output Values [0:255]')
      pylab.ylim([0, 255])
      matplotlib.pyplot.savefig('%s_plot_means.png' % (
          os.path.join(log_path, NAME)))

      # Assert blacks below CH_THRESH_BLACK
      for ch, mean in enumerate(black_means):
        e_msg = '%s black: %.1f, THRESH: %.f' % (
            COLOR_PLANES[ch], mean, CH_THRESH_BLACK)
        assert mean < CH_THRESH_BLACK, e_msg

      # Assert whites above CH_THRESH_WHITE
      for ch, mean in enumerate(white_means):
        e_msg = '%s white: %.1f, THRESH: %.f' % (
            COLOR_PLANES[ch], mean, CH_THRESH_WHITE)
        assert mean > CH_THRESH_WHITE, e_msg

      # Assert channels saturate evenly (was test_channel_saturation)
      e_msg = 'ch saturation not equal! RGB: %s, ATOL: %.f' % (
          str(white_means), CH_TOL_WHITE)
      assert np.isclose(
          np.amin(white_means), np.amax(white_means), atol=CH_TOL_WHITE), e_msg

if __name__ == '__main__':
  test_runner.main()

