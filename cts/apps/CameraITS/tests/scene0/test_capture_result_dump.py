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
"""CameraITS test that a capture result is returned from a manual capture.
"""

import pprint
from mobly import test_runner

import its_base_test
import camera_properties_utils
import capture_request_utils
import its_session_utils


class CaptureResultDumpTest(its_base_test.ItsBaseTest):
  """Test that a capture result is returned from a manual capture and dump it.
  """

  def test_capture_result_dump(self):
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      # Arbitrary capture request exposure values; image content is not
      # important for this test, only the metadata.
      props = cam.get_camera_properties()
      camera_properties_utils.skip_unless(
          camera_properties_utils.manual_sensor(props))

      req, fmt = capture_request_utils.get_fastest_manual_capture_settings(
          props)
      cap = cam.do_capture(req, fmt)
      pprint.pprint(cap['metadata'])

      # No pass/fail check; test passes if it completes.

if __name__ == '__main__':
  test_runner.main()
