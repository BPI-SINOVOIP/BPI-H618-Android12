# Copyright 2016 The Android Open Source Project
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
"""Verifies image is not flipped or mirrored."""


import logging
import os

from mobly import test_runner
import numpy as np


import cv2
import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils
import opencv_processing_utils

CHART_ORIENTATIONS = ['nominal', 'flip', 'mirror', 'rotate']
NAME = os.path.splitext(os.path.basename(__file__))[0]
PATCH_H = 0.5  # center 50%
PATCH_W = 0.5
PATCH_X = 0.5 - PATCH_W/2
PATCH_Y = 0.5 - PATCH_H/2
VGA_W, VGA_H = 640, 480


def test_flip_mirror_impl(cam, props, fmt, chart, debug, log_path):

  """Return if image is flipped or mirrored.

  Args:
   cam : An open its session.
   props : Properties of cam.
   fmt : dict,Capture format.
   chart: Object with chart properties.
   debug: boolean,whether to run test in debug mode or not.
   log_path: log_path to save the captured image.

  Returns:
    boolean: True if flipped, False if not
  """

  # determine if monochrome camera
  mono_camera = camera_properties_utils.mono_camera(props)

  # get a local copy of the chart template
  template = cv2.imread(opencv_processing_utils.CHART_FILE, cv2.IMREAD_ANYDEPTH)

  # take img, crop chart, scale and prep for cv2 template match
  cam.do_3a()
  req = capture_request_utils.auto_capture_request()
  cap = cam.do_capture(req, fmt)
  y, _, _ = image_processing_utils.convert_capture_to_planes(cap, props)
  y = image_processing_utils.rotate_img_per_argv(y)
  patch = image_processing_utils.get_image_patch(y, chart.xnorm, chart.ynorm,
                                                 chart.wnorm, chart.hnorm)
  patch = 255 * opencv_processing_utils.gray_scale_img(patch)
  patch = opencv_processing_utils.scale_img(
      patch.astype(np.uint8), chart.scale)

  # check image has content
  if np.max(patch)-np.min(patch) < 255/8:
    raise AssertionError('Image patch has no content! Check setup.')

  # save full images if in debug
  if debug:
    image_processing_utils.write_image(
        template[:, :, np.newaxis] / 255.0,
        '%s_template.jpg' % os.path.join(log_path, NAME))

  # save patch
  image_processing_utils.write_image(
      patch[:, :, np.newaxis] / 255.0,
      '%s_scene_patch.jpg' % os.path.join(log_path, NAME))

  # crop center areas and strip off any extra rows/columns
  template = image_processing_utils.get_image_patch(
      template, PATCH_X, PATCH_Y, PATCH_W, PATCH_H)
  patch = image_processing_utils.get_image_patch(
      patch, PATCH_X, PATCH_Y, PATCH_W, PATCH_H)
  patch = patch[0:min(patch.shape[0], template.shape[0]),
                0:min(patch.shape[1], template.shape[1])]
  comp_chart = patch

  # determine optimum orientation
  opts = []
  for orientation in CHART_ORIENTATIONS:
    if orientation == 'flip':
      comp_chart = np.flipud(patch)
    elif orientation == 'mirror':
      comp_chart = np.fliplr(patch)
    elif orientation == 'rotate':
      comp_chart = np.flipud(np.fliplr(patch))
    correlation = cv2.matchTemplate(comp_chart, template, cv2.TM_CCOEFF)
    _, opt_val, _, _ = cv2.minMaxLoc(correlation)
    if debug:
      cv2.imwrite('%s_%s.jpg' % (os.path.join(log_path, NAME), orientation),
                  comp_chart)
    logging.debug('%s correlation value: %d', orientation, opt_val)
    opts.append(opt_val)

  # determine if 'nominal' or 'rotated' is best orientation
  if not (opts[0] == max(opts) or opts[3] == max(opts)):
    raise AssertionError(
        f'Optimum orientation is {CHART_ORIENTATIONS[np.argmax(opts)]}')
  # print warning if rotated
  if opts[3] == max(opts):
    logging.warning('Image is rotated 180 degrees. Tablet might be rotated.')


class FlipMirrorTest(its_base_test.ItsBaseTest):
  """Test to verify if the image is flipped or mirrored."""

  def test_flip_mirror(self):
    """Test if image is properly oriented."""

    logging.debug('Starting %s', NAME)

    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      debug = self.debug_mode
      chart_loc_arg = self.chart_loc_arg

      # load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # initialize chart class and locate chart in scene
      chart = opencv_processing_utils.Chart(
          cam, props, self.log_path, chart_loc=chart_loc_arg)
      fmt = {'format': 'yuv', 'width': VGA_W, 'height': VGA_H}

      # test that image is not flipped, mirrored, or rotated
      test_flip_mirror_impl(cam, props, fmt, chart, debug, self.log_path)


if __name__ == '__main__':
  test_runner.main()
