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
"""Verifies android.control.availableEffects that are supported."""


import logging
import os.path
from mobly import test_runner
import numpy as np

import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils

# android.control.availableEffects possible values
EFFECTS = {0: 'OFF',
           1: 'MONO',
           2: 'NEGATIVE',
           3: 'SOLARIZE',
           4: 'SEPIA',
           5: 'POSTERIZE',
           6: 'WHITEBOARD',
           7: 'BLACKBOARD',
           8: 'AQUA'}
MONO_UV_SPREAD_MAX = 2  # max spread for U & V channels [0:255] for mono image
NAME = os.path.splitext(os.path.basename(__file__))[0]
VGA_W, VGA_H = 640, 480
YUV_MAX = 255  # normalization number for YUV images [0:1] --> [0:255]
YUV_UV_SPREAD_ATOL = 10  # min spread for U & V channels [0:255] for color image
YUV_Y_SPREAD_ATOL = 50  # min spread for Y channel [0:255] for color image


class EffectsTest(its_base_test.ItsBaseTest):
  """Test effects.

  Test: capture frame for supported camera effects and check if generated
  correctly. Note we only check effects OFF and MONO currently. Other effects
  do not have standardized definitions, so they are not tested.
  However, the test saves images for all supported effects.
  """

  def test_effects(self):
    logging.debug('Starting %s', NAME)
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      mono_camera = camera_properties_utils.mono_camera(props)

      # Load chart for scene.
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Determine available effects and run test(s)
      effects = props['android.control.availableEffects']
      camera_properties_utils.skip_unless(effects != [0])
      cam.do_3a(mono_camera=mono_camera)
      logging.debug('Supported effects: %s', str(effects))
      failed = []
      for effect in effects:
        req = capture_request_utils.auto_capture_request()
        req['android.control.effectMode'] = effect
        fmt = {'format': 'yuv', 'width': VGA_W, 'height': VGA_H}
        cap = cam.do_capture(req, fmt)

        # Save image of each effect
        img = image_processing_utils.convert_capture_to_rgb_image(
            cap, props=props)
        img_name = '%s_%s.jpg' % (os.path.join(self.log_path,
                                               NAME), EFFECTS[effect])
        image_processing_utils.write_image(img, img_name)

        # Simple checks
        if effect == 0:
          logging.debug('Checking effects OFF...')
          y, u, v = image_processing_utils.convert_capture_to_planes(cap, props)
          y_min, y_max = np.amin(y)*YUV_MAX, np.amax(y)*YUV_MAX
          e_msg = 'Y_range: %.2f,%.2f THRESH: %d; ' % (
              y_min, y_max, YUV_Y_SPREAD_ATOL)
          if (y_max-y_min) < YUV_Y_SPREAD_ATOL:
            failed.append({'effect': EFFECTS[effect], 'error': e_msg})
          if not mono_camera:
            u_min, u_max = np.amin(u) * YUV_MAX, np.amax(u) * YUV_MAX
            v_min, v_max = np.amin(v) * YUV_MAX, np.amax(v) * YUV_MAX
            e_msg += 'U_range: %.2f,%.2f THRESH: %d; ' % (
                u_min, u_max, YUV_UV_SPREAD_ATOL)
            e_msg += 'V_range: %.2f,%.2f THRESH: %d' % (
                v_min, v_max, YUV_UV_SPREAD_ATOL)
            if ((u_max - u_min) < YUV_UV_SPREAD_ATOL or
                (v_max - v_min) < YUV_UV_SPREAD_ATOL):
              failed.append({'effect': EFFECTS[effect], 'error': e_msg})
        elif effect == 1:
          logging.debug('Checking MONO effect...')
          _, u, v = image_processing_utils.convert_capture_to_planes(cap, props)
          u_min, u_max = np.amin(u)*YUV_MAX, np.amax(u)*YUV_MAX
          v_min, v_max = np.amin(v)*YUV_MAX, np.amax(v)*YUV_MAX
          e_msg = 'U_range: %.2f,%.2f; V_range: %.2f,%.2f; TOL: %d' % (
              u_min, u_max, v_min, v_max, MONO_UV_SPREAD_MAX)
          if ((u_max - u_min) > MONO_UV_SPREAD_MAX or
              (v_max - v_min) > MONO_UV_SPREAD_MAX):
            failed.append({'effect': EFFECTS[effect], 'error': e_msg})
      if failed:
        logging.debug('Failed effects:')
        for fail in failed:
          logging.debug(' %s: %s', fail['effect'], fail['error'])
        raise AssertionError(f'{NAME} failed. See test_log.DEBUG for errors.')

if __name__ == '__main__':
  test_runner.main()
