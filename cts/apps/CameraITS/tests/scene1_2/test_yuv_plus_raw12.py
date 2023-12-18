# Copyright 2015 The Android Open Source Project
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
"""Verifies RAW12 and YUV images are similar."""


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
THRESHOLD_MAX_RMS_DIFF = 0.035


class YuvPlusRaw12Test(its_base_test.ItsBaseTest):
  """Test capturing a single frame as both RAW12 and YUV outputs."""

  def test_yuv_plus_raw12(self):
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
          camera_properties_utils.compute_target_exposure(props) and
          camera_properties_utils.raw12(props) and
          camera_properties_utils.per_frame_control(props) and
          not camera_properties_utils.mono_camera(props))

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Use a manual request with a linear tonemap so that the YUV and RAW
      # look the same (once converted by the image_processing_utils module).
      e, s = target_exposure_utils.get_target_exposure_combos(
          log_path, cam)['midExposureTime']
      req = capture_request_utils.manual_capture_request(
          s, e, 0.0, True, props)

      mode = req['android.shading.mode']
      logging.debug('shading mode: %d', mode)

      max_raw12_size = capture_request_utils.get_available_output_sizes(
          'raw12', props)[0]
      w, h = capture_request_utils.get_available_output_sizes(
          'yuv', props, MAX_IMG_SIZE, max_raw12_size)[0]
      out_surfaces = [{'format': 'raw12'},
                      {'format': 'yuv', 'width': w, 'height': h}]
      cap_raw, cap_yuv = cam.do_capture(req, out_surfaces)

      img = image_processing_utils.convert_capture_to_rgb_image(cap_yuv)
      image_processing_utils.write_image(img, '%s_shading=%d_yuv.jpg' % (
          os.path.join(log_path, NAME), mode), True)
      patch = image_processing_utils.get_image_patch(
          img, PATCH_X, PATCH_Y, PATCH_W, PATCH_H)
      rgb_means_yuv = image_processing_utils.compute_image_means(patch)

      # RAW shots are 1/2 x 1/2 smaller after conversion to RGB, but patch
      # cropping is relative.
      img = image_processing_utils.convert_capture_to_rgb_image(
          cap_raw, props=props)
      image_processing_utils.write_image(img, '%s_shading=%d_raw12.jpg' % (
          os.path.join(log_path, NAME), mode), True)
      patch = image_processing_utils.get_image_patch(
          img, PATCH_X, PATCH_Y, PATCH_W, PATCH_H)
      rgb_means_raw = image_processing_utils.compute_image_means(patch)

      rms_diff = image_processing_utils.compute_image_rms_difference(
          rgb_means_yuv, rgb_means_raw)
      msg = f'RMS diff: {rms_diff:.4f}'
      logging.debug('%s', msg)
      if rms_diff >= THRESHOLD_MAX_RMS_DIFF:
        raise AssertionError(msg + f', spec: {THRESHOLD_MAX_RMS_DIFF}')

if __name__ == '__main__':
  test_runner.main()

