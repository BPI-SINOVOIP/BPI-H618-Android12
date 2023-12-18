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
"""Verifies android.flash.mode parameters is applied when set."""


import logging
import os.path
from mobly import test_runner

import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils
import target_exposure_utils

CHART_DISTANCE_NO_SCALING = 100  # 1 meter to disable scaling
FLASH_MODES = {'OFF': 0, 'SINGLE': 1, 'TORCH': 2}
FLASH_STATES = {'UNAVAIL': 0, 'CHARGING': 1, 'READY': 2, 'FIRED': 3,
                'PARTIAL': 4}
NAME = os.path.splitext(os.path.basename(__file__))[0]
PATCH_H = 0.25  # center 25%
PATCH_W = 0.25
PATCH_X = 0.5 - PATCH_W/2
PATCH_Y = 0.5 - PATCH_H/2
GRADIENT_DELTA = 0.1  # used for tablet setups (tablet screen aborbs energy)
Y_RELATIVE_DELTA_FLASH = 0.1  # 10%  # used for reflective chart setups
Y_RELATIVE_DELTA_TORCH = 0.05  # 5%  # used for reflective chart setups


class ParamFlashModeTest(its_base_test.ItsBaseTest):
  """Test that the android.flash.mode parameter is applied."""

  def test_param_flash_mode(self):
    logging.debug('Starting %s', NAME)
    logging.debug('FLASH_MODES[OFF]: %d, [SINGLE]: %d, [TORCH]: %d',
                  FLASH_MODES['OFF'], FLASH_MODES['SINGLE'],
                  FLASH_MODES['TORCH'])
    logging.debug(('FLASH_STATES[UNAVAIL]: %d, [CHARGING]: %d, [READY]: %d,'
                   '[FIRED] %d, [PARTIAL]: %d'), FLASH_STATES['UNAVAIL'],
                  FLASH_STATES['CHARGING'], FLASH_STATES['READY'],
                  FLASH_STATES['FIRED'], FLASH_STATES['PARTIAL'])

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
          camera_properties_utils.flash(props))

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, CHART_DISTANCE_NO_SCALING)

      modes = []
      states = []
      means = []
      grads = []

      # Manually set the exposure to be a little on the dark side, so that
      # it should be obvious whether the flash fired or not, and use a
      # linear tonemap.
      largest_yuv = capture_request_utils.get_largest_yuv_format(props)
      match_ar = (largest_yuv['width'], largest_yuv['height'])
      fmt = capture_request_utils.get_smallest_yuv_format(
          props, match_ar=match_ar)
      sync_latency = camera_properties_utils.sync_latency(props)

      e, s = target_exposure_utils.get_target_exposure_combos(
          log_path, cam)['midExposureTime']
      e /= 2  # darken image slightly
      req = capture_request_utils.manual_capture_request(s, e, 0.0, True, props)

      for f in FLASH_MODES.values():
        req['android.flash.mode'] = f
        cap = its_session_utils.do_capture_with_latency(
            cam, req, sync_latency, fmt)
        modes.append(cap['metadata']['android.flash.mode'])
        states.append(cap['metadata']['android.flash.state'])
        y, _, _ = image_processing_utils.convert_capture_to_planes(cap, props)
        image_processing_utils.write_image(
            y, '%s_%d.jpg' % (os.path.join(log_path, NAME), f))
        patch = image_processing_utils.get_image_patch(
            y, PATCH_X, PATCH_Y, PATCH_W, PATCH_H)
        image_processing_utils.write_image(
            patch, '%s_%d_patch.jpg' % (os.path.join(log_path, NAME), f))
        means.append(image_processing_utils.compute_image_means(patch)[0])
        grads.append(image_processing_utils.compute_image_max_gradients(
            patch)[0])

      # Assert state behavior
      logging.debug('Reported modes: %s', str(modes))
      logging.debug('Reported states: %s', str(states))
      assert modes == list(FLASH_MODES.values()), str(modes)

      e_msg = 'flash state reported[OFF]: %d' % states[FLASH_MODES['OFF']]
      assert states[FLASH_MODES['OFF']] not in [
          FLASH_STATES['FIRED'], FLASH_STATES['PARTIAL']], e_msg

      e_msg = 'flash state reported[SINGLE]: %d' % states[FLASH_MODES['SINGLE']]
      assert states[FLASH_MODES['SINGLE']] in [
          FLASH_STATES['FIRED'], FLASH_STATES['PARTIAL']], e_msg

      e_msg = 'flash state reported[TORCH]: %d' % states[FLASH_MODES['TORCH']]
      assert states[FLASH_MODES['TORCH']] in [
          FLASH_STATES['FIRED'], FLASH_STATES['PARTIAL']], e_msg

      # Assert image behavior: change between OFF & SINGLE
      logging.debug('Brightness means: %s', str(means))
      logging.debug('Max gradients: %s', str(grads))
      grad_delta = grads[FLASH_MODES['SINGLE']] - grads[FLASH_MODES['OFF']]
      mean_delta = ((means[FLASH_MODES['SINGLE']] - means[FLASH_MODES['OFF']]) /
                    means[FLASH_MODES['OFF']])
      e_msg = 'gradient SINGLE-OFF: %.3f, ATOL: %.3f' % (
          grad_delta, GRADIENT_DELTA)
      e_msg += ' mean SINGLE:OFF %.3f, ATOL: %.3f' % (
          mean_delta, Y_RELATIVE_DELTA_FLASH)
      assert (grad_delta > GRADIENT_DELTA or
              mean_delta > Y_RELATIVE_DELTA_FLASH), e_msg

      # Assert image behavior: change between OFF & TORCH
      grad_delta = grads[FLASH_MODES['TORCH']] - grads[FLASH_MODES['OFF']]
      mean_delta = ((means[FLASH_MODES['TORCH']] - means[FLASH_MODES['OFF']]) /
                    means[FLASH_MODES['OFF']])
      e_msg = 'gradient TORCH-OFF: %.3f, ATOL: %.3f' % (
          grad_delta, GRADIENT_DELTA)
      e_msg += ' mean TORCH:OFF %.3f, ATOL: %.3f' % (
          mean_delta, Y_RELATIVE_DELTA_TORCH)
      assert (grad_delta > GRADIENT_DELTA or
              mean_delta > Y_RELATIVE_DELTA_TORCH), e_msg

if __name__ == '__main__':
  test_runner.main()

