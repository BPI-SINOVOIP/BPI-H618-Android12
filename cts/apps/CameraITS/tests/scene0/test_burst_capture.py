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
"""Verify capture burst of full size images is fast enough to not timeout.
"""

import logging
import os

from mobly import test_runner

import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils

NAME = os.path.splitext(os.path.basename(__file__))[0]
NUM_TEST_FRAMES = 15


class BurstCaptureTest(its_base_test.ItsBaseTest):
  """Test capture a burst of full size images is fast enough and doesn't timeout.

  This test verifies that the entire capture pipeline can keep up the speed of
  fullsize capture + CPU read for at least some time.
  """

  def test_burst_capture(self):
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      camera_properties_utils.skip_unless(
          camera_properties_utils.backward_compatible(props))
      req = capture_request_utils.auto_capture_request()
      caps = cam.do_capture([req] * NUM_TEST_FRAMES)
      cap = caps[0]
      img = image_processing_utils.convert_capture_to_rgb_image(
          cap, props=props)
      name = os.path.join(self.log_path, NAME)
      img_name = '%s.jpg' % (name)
      logging.debug('Image Name: %s', img_name)
      image_processing_utils.write_image(img, img_name)


if __name__ == '__main__':
  test_runner.main()
