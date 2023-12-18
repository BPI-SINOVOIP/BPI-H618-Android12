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
"""CameraITS test to see sensitivity param applied properly in burst or not.
"""

from mobly import test_runner

import its_base_test
import camera_properties_utils
import capture_request_utils
import its_session_utils

NUM_STEPS = 3
ERROR_TOLERANCE = 0.96  # Allow ISO to be rounded down by 4%


class ParamSensitivityBurstTest(its_base_test.ItsBaseTest):
  """Test android.sensor.sensitivity parameter applied properly in burst.

  Inspects the output metadata only (not the image data).
  """

  def test_param_sensitivity_burst(self):
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      camera_properties_utils.skip_unless(
          camera_properties_utils.manual_sensor(props) and
          camera_properties_utils.per_frame_control(props))

      sens_range = props['android.sensor.info.sensitivityRange']
      sens_step = (sens_range[1] - sens_range[0]) // NUM_STEPS
      sens_list = range(sens_range[0], sens_range[1], sens_step)
      exp = min(props['android.sensor.info.exposureTimeRange'])
      assert exp != 0
      reqs = [
          capture_request_utils.manual_capture_request(s, exp)
          for s in sens_list
      ]
      _, fmt = capture_request_utils.get_fastest_manual_capture_settings(props)

      caps = cam.do_capture(reqs, fmt)
      for i, cap in enumerate(caps):
        s_req = sens_list[i]
        s_res = cap['metadata']['android.sensor.sensitivity']
        msg = 's_write: %d, s_read: %d, TOL: %.2f' % (s_req, s_res,
                                                      ERROR_TOLERANCE)
        assert s_req >= s_res, msg
        assert s_res / float(s_req) > ERROR_TOLERANCE, msg


if __name__ == '__main__':
  test_runner.main()
