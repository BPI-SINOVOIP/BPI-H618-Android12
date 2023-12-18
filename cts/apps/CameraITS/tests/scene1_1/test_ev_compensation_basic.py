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
"""Verifies EV compensation is applied."""


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

LOCKED = 3
LUMA_LOCKED_TOL = 0.05
NAME = os.path.splitext(os.path.basename(__file__))[0]
NUM_UNSATURATED_EVS = 3
PATCH_H = 0.1  # center 10%
PATCH_W = 0.1
PATCH_X = 0.5 - PATCH_W/2
PATCH_Y = 0.5 - PATCH_H/2
THRESH_CONVERGE_FOR_EV = 8  # AE must converge within this num
YUV_FULL_SCALE = 255.0
YUV_SAT_MIN = 250.0
YUV_SAT_TOL = 3.0


def create_request_with_ev(ev):
  req = capture_request_utils.auto_capture_request()
  req['android.control.aeExposureCompensation'] = ev
  req['android.control.aeLock'] = True
  return req


def extract_luma_from_capture(cap):
  """Extract luma from capture."""
  y = image_processing_utils.convert_capture_to_planes(cap)[0]
  patch = image_processing_utils.get_image_patch(
      y, PATCH_X, PATCH_Y, PATCH_W, PATCH_H)
  luma = image_processing_utils.compute_image_means(patch)[0]
  return luma


class EvCompensationBasicTest(its_base_test.ItsBaseTest):
  """Tests that EV compensation is applied."""

  def test_ev_compensation_basic(self):
    logging.debug('Starting %s', NAME)
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      log_path = self.log_path
      debug = self.debug_mode
      test_name_w_path = os.path.join(log_path, NAME)

      # check SKIP conditions
      camera_properties_utils.skip_unless(
          camera_properties_utils.ev_compensation(props) and
          camera_properties_utils.ae_lock(props))

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Create ev compensation changes
      ev_per_step = capture_request_utils.rational_to_float(
          props['android.control.aeCompensationStep'])
      steps_per_ev = int(1.0 / ev_per_step)
      evs = range(-2 * steps_per_ev, 2 * steps_per_ev + 1, steps_per_ev)

      # Converge 3A, and lock AE once converged. skip AF trigger as
      # dark/bright scene could make AF convergence fail and this test
      # doesn't care the image sharpness.
      mono_camera = camera_properties_utils.mono_camera(props)
      cam.do_3a(ev_comp=0, lock_ae=True, do_af=False, mono_camera=mono_camera)

      # Do captures and extract information
      largest_yuv = capture_request_utils.get_largest_yuv_format(props)
      match_ar = (largest_yuv['width'], largest_yuv['height'])
      fmt = capture_request_utils.get_smallest_yuv_format(
          props, match_ar=match_ar)
      lumas = []
      for ev in evs:
        # Capture a single shot with the same EV comp and locked AE.
        req = create_request_with_ev(ev)
        caps = cam.do_capture([req]*THRESH_CONVERGE_FOR_EV, fmt)
        luma_locked = []
        for i, cap in enumerate(caps):
          if debug:
            img = image_processing_utils.convert_capture_to_rgb_image(
                cap, props)
            image_processing_utils.write_image(
                img, f'{test_name_w_path}_ev{ev}_frame{i}.jpg')
          if cap['metadata']['android.control.aeState'] == LOCKED:
            luma = extract_luma_from_capture(cap)
            luma_locked.append(luma)
            if i == THRESH_CONVERGE_FOR_EV-1:
              lumas.append(luma)
              if not math.isclose(min(luma_locked), max(luma_locked),
                                  rel_tol=LUMA_LOCKED_TOL):
                raise AssertionError(f'AE locked lumas: {luma_locked}, '
                                     f'RTOL: {LUMA_LOCKED_TOL}')
      logging.debug('lumas in AE locked captures: %s', str(lumas))
      if caps[THRESH_CONVERGE_FOR_EV-1]['metadata'][
          'android.control.aeState'] != LOCKED:
        raise AssertionError(f'No AE lock by {THRESH_CONVERGE_FOR_EV} frame.')

    # Create plot
    pylab.figure(NAME)
    pylab.plot(evs, lumas, '-ro')
    pylab.title(NAME)
    pylab.xlabel('EV Compensation')
    pylab.ylabel('Mean Luma (Normalized)')
    matplotlib.pyplot.savefig(f'{test_name_w_path}_plot_means.png')

    # Trim extra saturated images
    while (lumas[-2] >= YUV_SAT_MIN/YUV_FULL_SCALE and
           lumas[-1] >= YUV_SAT_MIN/YUV_FULL_SCALE and
           len(lumas) > 2):
      lumas.pop(-1)
      logging.debug('Removed saturated image.')

    # Only allow positive EVs to give saturated image
    if len(lumas) < NUM_UNSATURATED_EVS:
      raise AssertionError(
          f'>{NUM_UNSATURATED_EVS-1} unsaturated images needed.')
    min_luma_diffs = min(np.diff(lumas))
    logging.debug('Min of luma value difference between adjacent ev comp: %.3f',
                  min_luma_diffs)

    # Assert unsaturated lumas increasing with increasing ev comp.
    if min_luma_diffs <= 0:
      raise AssertionError('Lumas not increasing with ev comp! '
                           f'EVs: {list(evs)}, lumas: {lumas}')


if __name__ == '__main__':
  test_runner.main()
