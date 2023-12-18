# Copyright 2017 The Android Open Source Project
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
"""Verifies AE and AF can run independently."""


import logging
import os.path
from mobly import test_runner
import numpy as np

import its_base_test
import camera_properties_utils
import error_util
import its_session_utils

AWB_GAINS_LENGTH = 4
AWB_XFORM_LENGTH = 9
G_CHANNEL = 2
G_GAIN = 1.0
G_GAIN_TOL = 0.05
NAME = os.path.splitext(os.path.basename(__file__))[0]
THREE_A_STATES = {'AE': [True, False, True],
                  'AF': [False, True, True],
                  'FULL_3A': [True, True, True]}  # note no AWB solo


class SingleATest(its_base_test.ItsBaseTest):
  """Test basic camera 3A behavior with AE and AF run individually.

  To pass, 3A must converge. Check that returned 3A values are valid.
  """

  def test_ae_af(self):
    logging.debug('Starting %s', NAME)
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      camera_properties_utils.skip_unless(
          camera_properties_utils.read_3a(props))
      mono_camera = camera_properties_utils.mono_camera(props)

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Do AE/AF/3A and evaluate outputs
      for k, three_a_req in sorted(THREE_A_STATES.items()):
        logging.debug('Trying %s', k)
        try:
          s, e, awb_gains, awb_xform, fd = cam.do_3a(get_results=True,
                                                     do_ae=three_a_req[0],
                                                     do_af=three_a_req[1],
                                                     do_awb=three_a_req[2],
                                                     mono_camera=mono_camera)

        except error_util.CameraItsError:
          raise AssertionError(f'{k} did not converge.')

        logging.debug('AWB gains: %s, xform: %s', str(awb_gains),
                      str(awb_xform))
        if three_a_req[0]:  # can report None for AF only
          if not e:
            raise AssertionError('No valid exposure time returned for AE.')
          if not s:
            raise AssertionError('No valid sensitivity returned for AE.')
          logging.debug('AE sensitivity: %d, exposure: %dns', s, e)
        else:
          logging.debug('AE sensitivity: None, exposure: None')
        if three_a_req[1]:  # fd can report None for AE only
          logging.debug('AF fd: %.3f', fd)
        else:
          logging.debug('AF fd: None')
        # check AWB values
        if len(awb_gains) != AWB_GAINS_LENGTH:
          raise AssertionError(f'Problem with AWB gains: {awb_gains}')
        for g in awb_gains:
          if np.isnan(g):
            raise AssertionError('Gain in AWB is NaN.')
        if len(awb_xform) != AWB_XFORM_LENGTH:
          raise AssertionError(f'Problem with AWB transform: {awb_xform}')
        for x in awb_xform:
          if np.isnan(x):
            raise AssertionError('Value in AWB transform is NaN.')
        if not np.isclose(awb_gains[G_CHANNEL], G_GAIN, G_GAIN_TOL):
          raise AssertionError(
              f'AWB G channel mismatch. AWB(G): {awb_gains[G_CHANNEL]}, '
              f'REF: {G_GAIN}, TOL: {G_GAIN_TOL}')

        # check AE values
        if k == 'full_3a' or k == 'ae':
          if s < min(props['android.sensor.info.sensitivityRange']):
            raise AssertionError(f'ISO is < minimum! ISO: {s}')
          if e < min(props['android.sensor.info.exposureTimeRange']):
            raise AssertionError(f'Exposure is < minimum! exp: {e}')

        # check AF values
        if k == 'full_3a' or k == 'af':
          if fd < 0:
            raise AssertionError(f'Focal distance is < 0! fd: {fd}')

if __name__ == '__main__':
  test_runner.main()

