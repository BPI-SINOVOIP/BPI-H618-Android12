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
"""Verifies YUV & JPEG image captures have similar brightness."""


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

NAME = os.path.splitext(os.path.basename(__file__))[0]
PATCH_H = 0.1  # center 10%
PATCH_W = 0.1
PATCH_X = 0.5 - PATCH_W/2
PATCH_Y = 0.5 - PATCH_H/2
THRESHOLD_MAX_RMS_DIFF = 0.03


def do_capture_and_extract_rgb_means(req, cam, size, img_type, log_path, debug):
  """Do capture and extra rgb_means of center patch.

  Args:
    req: capture request
    cam: camera object
    size: [width, height]
    img_type: string of 'yuv' or 'jpeg'
    log_path: location for saving image
    debug: boolean to flag saving captured images

  Returns:
    center patch RGB means
  """
  out_surface = {'width': size[0], 'height': size[1], 'format': img_type}
  cap = cam.do_capture(req, out_surface)
  if img_type == 'jpg':
    assert cap['format'] == 'jpeg'
    img = image_processing_utils.decompress_jpeg_to_rgb_image(cap['data'])
  else:
    assert cap['format'] == img_type
    img = image_processing_utils.convert_capture_to_rgb_image(cap)
  assert cap['width'] == size[0]
  assert cap['height'] == size[1]

  if debug:
    image_processing_utils.write_image(img, '%s_%s_w%d_h%d.jpg'%(
        os.path.join(log_path, NAME), img_type, size[0], size[1]))
  if img_type == 'jpg':
    assert img.shape[0] == size[1]
    assert img.shape[1] == size[0]
    assert img.shape[2] == 3
  patch = image_processing_utils.get_image_patch(
      img, PATCH_X, PATCH_Y, PATCH_W, PATCH_H)
  rgb = image_processing_utils.compute_image_means(patch)
  logging.debug('Captured %s %dx%d rgb = %s',
                img_type, cap['width'], cap['height'], str(rgb))
  return rgb


class YuvJpegAllTest(its_base_test.ItsBaseTest):
  """Test reported sizes & fmts for YUV & JPEG caps return similar images."""

  def test_yuv_jpeg_all(self):
    logging.debug('Starting %s', NAME)
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      camera_properties_utils.skip_unless(
          camera_properties_utils.compute_target_exposure(props) and
          camera_properties_utils.per_frame_control(props))
      log_path = self.log_path
      debug = self.debug_mode

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Use a manual request with a linear tonemap so that the YUV and JPEG
      # should look the same (once converted by the image_processing_utils).
      e, s = target_exposure_utils.get_target_exposure_combos(
          log_path, cam)['midExposureTime']
      req = capture_request_utils.manual_capture_request(s, e, 0.0, True, props)

      rgbs = []
      for size in capture_request_utils.get_available_output_sizes(
          'yuv', props):
        rgbs.append(do_capture_and_extract_rgb_means(
            req, cam, size, 'yuv', log_path, debug))

      for size in capture_request_utils.get_available_output_sizes(
          'jpg', props):
        rgbs.append(do_capture_and_extract_rgb_means(
            req, cam, size, 'jpg', log_path, debug))

      # Plot means vs format
      pylab.figure(NAME)
      pylab.title(NAME)
      pylab.plot(range(len(rgbs)), [r[0] for r in rgbs], '-ro')
      pylab.plot(range(len(rgbs)), [g[1] for g in rgbs], '-go')
      pylab.plot(range(len(rgbs)), [b[2] for b in rgbs], '-bo')
      pylab.ylim([0, 1])
      pylab.xlabel('format number')
      pylab.ylabel('RGB avg [0, 1]')
      matplotlib.pyplot.savefig(
          '%s_plot_means.png' % os.path.join(log_path, NAME))

      # Assert all captured images are similar in RBG space
      max_diff = 0
      for rgb_i in rgbs[1:]:
        rms_diff = image_processing_utils.compute_image_rms_difference(
            rgbs[0], rgb_i)  # use first capture as reference
        max_diff = max(max_diff, rms_diff)
      msg = 'Max RMS difference: %.4f' % max_diff
      logging.debug('%s', msg)
      e_msg = msg + ' spec: %.3f' % THRESHOLD_MAX_RMS_DIFF
      assert max_diff < THRESHOLD_MAX_RMS_DIFF, e_msg

if __name__ == '__main__':
  test_runner.main()
