# Copyright 2018 The Android Open Source Project
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
"""CameraITS test for tonemap curve with sensor test pattern."""

import logging
import os

from mobly import test_runner
import numpy as np

import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils


NAME = os.path.basename(__file__).split('.')[0]
COLOR_BAR_PATTERN = 2  # Note scene0/test_test_patterns must PASS
COLOR_BARS = ['WHITE', 'YELLOW', 'CYAN', 'GREEN', 'MAGENTA', 'RED',
              'BLUE', 'BLACK']
N_BARS = len(COLOR_BARS)
COLOR_CHECKER = {'BLACK': [0, 0, 0], 'RED': [1, 0, 0], 'GREEN': [0, 1, 0],
                 'BLUE': [0, 0, 1], 'MAGENTA': [1, 0, 1], 'CYAN': [0, 1, 1],
                 'YELLOW': [1, 1, 0], 'WHITE': [1, 1, 1]}
DELTA = 0.0005  # crop on edge of color bars
RAW_TOL = 0.001  # 1 DN in [0:1] (1/(1023-64)
RGB_VAR_TOL = 0.0039  # 1/255
RGB_MEAN_TOL = 0.1
TONEMAP_MAX = 0.5
YUV_H = 480
YUV_W = 640
# Normalized co-ordinates for the color bar patch.
Y_NORM = 0.0
W_NORM = 1.0 / N_BARS - 2 * DELTA
H_NORM = 1.0

# Linear tonemap with maximum of 0.5
LINEAR_TONEMAP = sum([[i/63.0, i/126.0] for i in range(64)], [])


def get_x_norm(num):
  """Returns the normalized x co-ordinate for the title.

  Args:
   num: int; position on color in the color bar.

  Returns:
    normalized x co-ordinate.
  """
  return float(num) / N_BARS + DELTA


def check_raw_pattern(img_raw):
  """Checks for RAW capture matches color bar pattern.

  Args:
    img_raw: RAW image
  """
  logging.debug('Checking RAW/PATTERN match')
  color_match = []
  for n in range(N_BARS):
    x_norm = get_x_norm(n)
    raw_patch = image_processing_utils.get_image_patch(img_raw, x_norm, Y_NORM,
                                                       W_NORM, H_NORM)
    raw_means = image_processing_utils.compute_image_means(raw_patch)
    logging.debug('patch: %d, x_norm: %.3f, RAW means: %s',
                  n, x_norm, str(raw_means))
    for color in COLOR_BARS:
      if np.allclose(COLOR_CHECKER[color], raw_means, atol=RAW_TOL):
        color_match.append(color)
        logging.debug('%s match', color)
        break
      else:
        logging.debug('No match w/ %s: %s, ATOL: %.3f',
                      color, str(COLOR_CHECKER[color]), RAW_TOL)
  if set(color_match) != set(COLOR_BARS):
    raise AssertionError('RAW COLOR_BARS test pattern does not have all colors')


def check_yuv_vs_raw(img_raw, img_yuv):
  """Checks for YUV vs RAW match in 8 patches.

  Check for correct values and color consistency

  Args:
    img_raw: RAW image
    img_yuv: YUV image
  """
  logging.debug('Checking YUV/RAW match')
  color_match_errs = []
  color_variance_errs = []
  for n in range(N_BARS):
    x_norm = get_x_norm(n)
    logging.debug('x_norm: %.3f', x_norm)
    raw_patch = image_processing_utils.get_image_patch(img_raw, x_norm, Y_NORM,
                                                       W_NORM, H_NORM)
    yuv_patch = image_processing_utils.get_image_patch(img_yuv, x_norm, Y_NORM,
                                                       W_NORM, H_NORM)
    raw_means = np.array(image_processing_utils.compute_image_means(raw_patch))
    raw_vars = np.array(
        image_processing_utils.compute_image_variances(raw_patch))
    yuv_means = np.array(image_processing_utils.compute_image_means(yuv_patch))
    yuv_means /= TONEMAP_MAX  # Normalize to tonemap max
    yuv_vars = np.array(
        image_processing_utils.compute_image_variances(yuv_patch))
    if not np.allclose(raw_means, yuv_means, atol=RGB_MEAN_TOL):
      color_match_errs.append(
          'RAW: %s, RGB(norm): %s, ATOL: %.2f' %
          (str(raw_means), str(np.round(yuv_means, 3)), RGB_MEAN_TOL))
    if not np.allclose(raw_vars, yuv_vars, atol=RGB_VAR_TOL):
      color_variance_errs.append('RAW: %s, RGB: %s, ATOL: %.4f' %
                                 (str(raw_vars), str(yuv_vars), RGB_VAR_TOL))

  # Print all errors before assertion
  if color_match_errs:
    for err in color_match_errs:
      logging.debug(err)
    for err in color_variance_errs:
      logging.error(err)
    raise AssertionError('Color match errors. See test_log.DEBUG')
  if color_variance_errs:
    for err in color_variance_errs:
      logging.error(err)
    raise AssertionError('Color variance errors. See test_log.DEBUG')


def test_tonemap_curve_impl(name, cam, props):
  """Test tonemap curve with sensor test pattern.

  Args:
   name: Path to save the captured image.
   cam: An open device session.
   props: Properties of cam.
  """

  avail_patterns = props['android.sensor.availableTestPatternModes']
  logging.debug('Available Patterns: %s', avail_patterns)
  sens_min, _ = props['android.sensor.info.sensitivityRange']
  min_exposure = min(props['android.sensor.info.exposureTimeRange'])

  # RAW image
  req_raw = capture_request_utils.manual_capture_request(
      int(sens_min), min_exposure)
  req_raw['android.sensor.testPatternMode'] = COLOR_BAR_PATTERN
  fmt_raw = {'format': 'raw'}
  cap_raw = cam.do_capture(req_raw, fmt_raw)
  img_raw = image_processing_utils.convert_capture_to_rgb_image(
      cap_raw, props=props)

  # Save RAW pattern
  image_processing_utils.write_image(
      img_raw, '%s_raw_%d.jpg' % (name, COLOR_BAR_PATTERN), True)
  check_raw_pattern(img_raw)

  # YUV image
  req_yuv = capture_request_utils.manual_capture_request(
      int(sens_min), min_exposure)
  req_yuv['android.sensor.testPatternMode'] = COLOR_BAR_PATTERN
  req_yuv['android.distortionCorrection.mode'] = 0
  req_yuv['android.tonemap.mode'] = 0
  req_yuv['android.tonemap.curve'] = {
      'red': LINEAR_TONEMAP,
      'green': LINEAR_TONEMAP,
      'blue': LINEAR_TONEMAP
  }
  fmt_yuv = {'format': 'yuv', 'width': YUV_W, 'height': YUV_H}
  cap_yuv = cam.do_capture(req_yuv, fmt_yuv)
  img_yuv = image_processing_utils.convert_capture_to_rgb_image(cap_yuv, True)

  # Save YUV pattern
  image_processing_utils.write_image(
      img_yuv, '%s_yuv_%d.jpg' % (name, COLOR_BAR_PATTERN), True)

  # Check pattern for correctness
  check_yuv_vs_raw(img_raw, img_yuv)


class TonemapCurveTest(its_base_test.ItsBaseTest):
  """Test conversion of test pattern from RAW to YUV with linear tonemap.

  Test makes use of android.sensor.testPatternMode 2 (COLOR_BARS).
  """

  def test_tonemap_curve(self):
    logging.debug('Starting %s', NAME)
    name = os.path.join(self.log_path, NAME)
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      camera_properties_utils.skip_unless(
          camera_properties_utils.raw16(props) and
          camera_properties_utils.manual_sensor(props) and
          camera_properties_utils.per_frame_control(props) and
          camera_properties_utils.manual_post_proc(props) and
          camera_properties_utils.color_bars_test_pattern(props))

      test_tonemap_curve_impl(name, cam, props)


if __name__ == '__main__':
  test_runner.main()
