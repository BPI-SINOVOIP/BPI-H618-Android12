# Copyright 2014 The Android Open Source Project
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
"""Verifies JPEG and YUV images are similar."""


import logging
import os.path
from mobly import test_runner

import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils
import target_exposure_utils

MAX_IMG_SIZE = (1920, 1080)
NAME = os.path.splitext(os.path.basename(__file__))[0]
PATCH_H = 0.1  # center 10%
PATCH_W = 0.1
PATCH_X = 0.5 - PATCH_W/2
PATCH_Y = 0.5 - PATCH_H/2
THRESHOLD_MAX_RMS_DIFF = 0.01


def compute_means_and_save(cap, img_name, log_path):
  """Compute the RGB means of a capture.

  Args:
    cap: 'YUV' or 'JPEG' capture
    img_name: text for saved image name
    log_path: path for saved image location
  Returns:
    RGB means
  """
  img = image_processing_utils.convert_capture_to_rgb_image(cap, True)
  image_processing_utils.write_image(
      img, '%s_%s.jpg' % (os.path.join(log_path, NAME), img_name))
  patch = image_processing_utils.get_image_patch(
      img, PATCH_X, PATCH_Y, PATCH_W, PATCH_H)
  rgb_means = image_processing_utils.compute_image_means(patch)
  logging.debug('%s rbg_means: %s', img_name, rgb_means)
  return rgb_means


class YuvPlusJpegTest(its_base_test.ItsBaseTest):
  """Test capturing a single frame as both YUV and JPEG outputs."""

  def test_yuv_plus_jpeg(self):
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
          camera_properties_utils.compute_target_exposure(props))

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Create requests
      max_jpeg_size = capture_request_utils.get_available_output_sizes(
          'jpeg', props)[0]
      w, h = capture_request_utils.get_available_output_sizes(
          'yuv', props, MAX_IMG_SIZE, max_jpeg_size)[0]
      fmt_yuv = {'format': 'yuv', 'width': w, 'height': h}
      fmt_jpg = {'format': 'jpeg'}

      # Use a manual request with a linear tonemap so that the YUV and JPEG
      # should look the same (once converted by the image_processing_utils).
      e, s = target_exposure_utils.get_target_exposure_combos(
          log_path, cam)['midExposureTime']
      req = capture_request_utils.manual_capture_request(s, e, 0.0, True, props)

      cap_yuv, cap_jpg = cam.do_capture(req, [fmt_yuv, fmt_jpg])
      rgb_means_yuv = compute_means_and_save(cap_yuv, 'yuv', log_path)
      rgb_means_jpg = compute_means_and_save(cap_jpg, 'jpg', log_path)

      rms_diff = image_processing_utils.compute_image_rms_difference(
          rgb_means_yuv, rgb_means_jpg)
      msg = f'RMS diff: {rms_diff:.4f}'
      logging.debug('%s', msg)
      if rms_diff >= THRESHOLD_MAX_RMS_DIFF:
        raise AssertionError(msg + f', spec: {THRESHOLD_MAX_RMS_DIFF}')

if __name__ == '__main__':
  test_runner.main()
