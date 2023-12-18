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
"""Verifies converted YUV images & device JPEG images look the same."""


import logging
import os.path
from mobly import test_runner

import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils
import target_exposure_utils

NAME = os.path.splitext(os.path.basename(__file__))[0]
PATCH_H = 0.1  # center 10%
PATCH_W = 0.1
PATCH_X = 0.5 - PATCH_W/2
PATCH_Y = 0.5 - PATCH_H/2
THRESHOLD_MAX_RMS_DIFF = 0.01


def compute_img_means_and_save(img, img_name, log_path):
  """Extract center patch, compute means, and save image.

  Args:
    img: image array
    img_name: text to identify image
    log_path: location to save image

  Returns:
    means of image patch
  """
  image_processing_utils.write_image(
      img, '%s_fmt=%s.jpg' % (os.path.join(log_path, NAME), img_name))
  patch = image_processing_utils.get_image_patch(
      img, PATCH_X, PATCH_Y, PATCH_W, PATCH_H)
  rgb_means = image_processing_utils.compute_image_means(patch)
  logging.debug('%s rgb_means: %s', img_name, str(rgb_means))
  return rgb_means


class JpegTest(its_base_test.ItsBaseTest):
  """Test that converted YUV images and device JPEG images look the same."""

  def test_jpeg(self):
    logging.debug('Starting %s', NAME)
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      log_path = self.log_path

      # Check SKIP conditions
      camera_properties_utils.skip_unless(
          camera_properties_utils.compute_target_exposure(props))
      sync_latency = camera_properties_utils.sync_latency(props)

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Initialize common request parameters
      e, s = target_exposure_utils.get_target_exposure_combos(
          log_path, cam)['midExposureTime']
      req = capture_request_utils.manual_capture_request(s, e, 0.0, True, props)

      # YUV
      size = capture_request_utils.get_available_output_sizes('yuv', props)[0]
      out_surface = {'width': size[0], 'height': size[1], 'format': 'yuv'}
      cap = its_session_utils.do_capture_with_latency(
          cam, req, sync_latency, out_surface)
      img = image_processing_utils.convert_capture_to_rgb_image(cap)
      rgb_means_yuv = compute_img_means_and_save(img, 'yuv', log_path)

      # JPEG
      size = capture_request_utils.get_available_output_sizes('jpg', props)[0]
      out_surface = {'width': size[0], 'height': size[1], 'format': 'jpg'}
      cap = its_session_utils.do_capture_with_latency(
          cam, req, sync_latency, out_surface)
      img = image_processing_utils.decompress_jpeg_to_rgb_image(cap['data'])
      rgb_means_jpg = compute_img_means_and_save(img, 'jpg', log_path)

      # Assert images are similar
      rms_diff = image_processing_utils.compute_image_rms_difference(
          rgb_means_yuv, rgb_means_jpg)
      logging.debug('RMS difference: %.3f', rms_diff)
      e_msg = 'RMS difference: %.3f, spec: %.2f' % (
          rms_diff, THRESHOLD_MAX_RMS_DIFF)
      assert rms_diff < THRESHOLD_MAX_RMS_DIFF, e_msg

if __name__ == '__main__':
  test_runner.main()

