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
"""Verifies manual burst capture consistency."""


import logging
import os.path
from matplotlib import pylab
import matplotlib.pyplot
from mobly import test_runner
import numpy as np

import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils
import target_exposure_utils

API_LEVEL_30 = 30
BURST_LEN = 50
COLORS = ['R', 'G', 'B']
NAME = os.path.splitext(os.path.basename(__file__))[0]
NUM_BURSTS = 5
PATCH_H = 0.1  # center 10%
PATCH_W = 0.1
PATCH_X = 0.5 - PATCH_W/2
PATCH_Y = 0.5 - PATCH_H/2
SPREAD_THRESH = 0.03
SPREAD_THRESH_API_LEVEL_30 = 0.02

NUM_FRAMES = BURST_LEN * NUM_BURSTS


class BurstSamenessManualTest(its_base_test.ItsBaseTest):
  """Take long bursts of images and check that they're all identical.

  Assumes a static scene. Can be used to idenfity if there are sporadic
  frames that are processed differently or have artifacts. Uses manual
  capture settings.
  """

  def test_burst_sameness_manual(self):
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
          camera_properties_utils.per_frame_control(props))

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Capture at the smallest resolution
      _, fmt = capture_request_utils.get_fastest_manual_capture_settings(props)
      e, s = target_exposure_utils.get_target_exposure_combos(
          log_path, cam)['minSensitivity']
      req = capture_request_utils.manual_capture_request(s, e)
      w, h = fmt['width'], fmt['height']

      # Capture bursts of YUV shots.
      # Get the mean values of a center patch for each.
      # Also build a 4D array, imgs, which is an array of all RGB images.
      r_means = []
      g_means = []
      b_means = []
      imgs = np.empty([NUM_FRAMES, h, w, 3])
      for j in range(NUM_BURSTS):
        caps = cam.do_capture([req]*BURST_LEN, [fmt])
        for i, cap in enumerate(caps):
          n = j*BURST_LEN + i
          imgs[n] = image_processing_utils.convert_capture_to_rgb_image(cap)
          patch = image_processing_utils.get_image_patch(
              imgs[n], PATCH_X, PATCH_Y, PATCH_W, PATCH_H)
          means = image_processing_utils.compute_image_means(patch)
          r_means.append(means[0])
          g_means.append(means[1])
          b_means.append(means[2])

      # Save first frame for setup debug
      image_processing_utils.write_image(
          imgs[0], '%s_frame000.jpg' % os.path.join(log_path, NAME))

      # Save all frames if debug
      if self.debug_mode:
        logging.debug('Dumping all images')
        for i in range(1, NUM_FRAMES):
          image_processing_utils.write_image(
              imgs[i], '%s_frame%03d.jpg'%(os.path.join(log_path, NAME), i))

      # Plot RGB means vs frames
      frames = range(NUM_FRAMES)
      pylab.figure(NAME)
      pylab.title(NAME)
      pylab.plot(frames, r_means, '-ro')
      pylab.plot(frames, g_means, '-go')
      pylab.plot(frames, b_means, '-bo')
      pylab.ylim([0, 1])
      pylab.xlabel('frame number')
      pylab.ylabel('RGB avg [0, 1]')
      matplotlib.pyplot.savefig(
          '%s_plot_means.png' % os.path.join(log_path, NAME))

      # determine spread_thresh
      spread_thresh = SPREAD_THRESH
      if its_session_utils.get_first_api_level(self.dut.serial) >= API_LEVEL_30:
        spread_thresh = SPREAD_THRESH_API_LEVEL_30

      # PASS/FAIL based on center patch similarity.
      for plane, means in enumerate([r_means, g_means, b_means]):
        spread = max(means) - min(means)
        msg = '%s spread: %.5f, spread_thresh: %.2f' % (
            COLORS[plane], spread, spread_thresh)
        logging.debug('%s', msg)
        assert spread < spread_thresh, msg

if __name__ == '__main__':
  test_runner.main()

