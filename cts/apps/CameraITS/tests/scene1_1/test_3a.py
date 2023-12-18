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
"""Verifies 3A converges with gray chart scene."""


import logging
import os.path
from mobly import test_runner
import numpy as np

import its_base_test
import camera_properties_utils
import its_session_utils

AWB_GAINS_LENGTH = 4
AWB_XFORM_LENGTH = 9
NAME = os.path.splitext(os.path.basename(__file__))[0]


class ThreeATest(its_base_test.ItsBaseTest):
  """Test basic camera 3A behavior.

  To pass, 3A must converge. Check that returned 3A values are valid.
  """

  def test_3a(self):
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

      # Do 3A and evaluate outputs
      s, e, awb_gains, awb_xform, focus = cam.do_3a(
          get_results=True, mono_camera=mono_camera)
      logging.debug('AWB: gains %s, xform %s', str(awb_gains), str(awb_xform))
      logging.debug('AE: sensitivity %d, exposure %dns', s, e)
      logging.debug('AF: distance %.3f', focus)

      assert len(awb_gains) == AWB_GAINS_LENGTH
      for g in awb_gains:
        assert not np.isnan(g)
      assert len(awb_xform) == AWB_XFORM_LENGTH
      for x in awb_xform:
        assert not np.isnan(x)
      assert s > 0
      assert e > 0
      assert focus >= 0

if __name__ == '__main__':
  test_runner.main()
