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
"""Verifies sub-cameras have similar RGB values for gray patch."""


import logging
import os.path

from mobly import test_runner
import numpy as np

import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils

_NAME = os.path.splitext(os.path.basename(__file__))[0]
_PATCH_H = 0.0625  # 1/16 x 1/16 in center of image
_PATCH_W = 0.0625
_PATCH_X = 0.5 - _PATCH_W/2
_PATCH_Y = 0.5 - _PATCH_H/2
_THRESH_DIFF = 0.06
_THRESH_GAIN = 0.1
_THRESH_EXP = 0.05


class MultiCameraMatchTest(its_base_test.ItsBaseTest):
  """Test both cameras give similar RGB values for gray patch.

  This test uses android.lens.info.availableFocalLengths to determine
  subcameras. The test will take images of the gray chart for each cameras,
  crop the center patch, and compare the Y (of YUV) means of the two images.
  Y means must be within _THRESH_DIFF for the test to pass.

  Cameras that use android.control.zoomRatioRange will have only 1 focal
  length and will need separate test.
  """

  def test_multi_camera_match(self):
    logging.debug('Starting %s', _NAME)
    yuv_sizes = {}
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      log_path = self.log_path

      # check SKIP conditions
      camera_properties_utils.skip_unless(
          camera_properties_utils.per_frame_control(props) and
          camera_properties_utils.logical_multi_camera(props))

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      ids = camera_properties_utils.logical_multi_camera_physical_ids(props)
      for i in ids:
        physical_props = cam.get_camera_properties_by_id(i)
        camera_properties_utils.skip_unless(
            not camera_properties_utils.mono_camera(physical_props) and
            camera_properties_utils.backward_compatible(physical_props))
        yuv_sizes[i] = capture_request_utils.get_available_output_sizes(
            'yuv', physical_props)
        if i == ids[0]:  # get_available_output_sizes returns sorted list
          yuv_match_sizes = yuv_sizes[i]
        else:
          yuv_match_sizes = list(
              set(yuv_sizes[i]).intersection(yuv_match_sizes))

      # find matched size for captures
      yuv_match_sizes.sort()
      w = yuv_match_sizes[-1][0]
      h = yuv_match_sizes[-1][1]
      logging.debug('Matched YUV size: (%d, %d)', w, h)

      # do 3a and create requests
      cam.do_3a()
      reqs = []
      avail_fls = sorted(props['android.lens.info.availableFocalLengths'],
                         reverse=True)
      # SKIP test if only 1 focal length
      camera_properties_utils.skip_unless(len(avail_fls) > 1)

      for i, fl in enumerate(avail_fls):
        reqs.append(capture_request_utils.auto_capture_request())
        reqs[i]['android.lens.focalLength'] = fl
        if i > 0:
          # Calculate the active sensor region for a non-cropped image
          zoom = avail_fls[0] / fl
          aa = props['android.sensor.info.activeArraySize']
          aa_w, aa_h = aa['right'] - aa['left'], aa['bottom'] - aa['top']

          # Calculate a center crop region.
          assert zoom >= 1
          crop_w = aa_w // zoom
          crop_h = aa_h // zoom
          crop_region = {'left': aa_w // 2 - crop_w // 2,
                         'top': aa_h // 2 - crop_h // 2,
                         'right': aa_w // 2 + crop_w // 2,
                         'bottom': aa_h // 2 + crop_h // 2}
          reqs[i]['android.scaler.cropRegion'] = crop_region

      # capture YUVs
      y_means = {}
      e_msg = ''
      fmt = [{'format': 'yuv', 'width': w, 'height': h}]
      caps = cam.do_capture(reqs, fmt)
      for i, fl in enumerate(avail_fls):
        img = image_processing_utils.convert_capture_to_rgb_image(
            caps[i], props=props)
        image_processing_utils.write_image(img, '%s_yuv_fl=%s.jpg' % (
            os.path.join(log_path, _NAME), fl))
        y, _, _ = image_processing_utils.convert_capture_to_planes(
            caps[i], props=props)
        y_mean = image_processing_utils.compute_image_means(
            image_processing_utils.get_image_patch(
                y, _PATCH_X, _PATCH_Y, _PATCH_W, _PATCH_H))[0]
        msg = 'y[%s]: %.3f, ' % (fl, y_mean)
        logging.debug(msg)
        e_msg += msg
        y_means[fl] = y_mean

      # compare Y means
      e_msg += 'TOL=%.5f' % _THRESH_DIFF
      assert np.isclose(max(y_means.values()), min(y_means.values()),
                        rtol=_THRESH_DIFF), e_msg

if __name__ == '__main__':
  test_runner.main()
