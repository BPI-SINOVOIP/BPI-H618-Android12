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
"""CameraITS test that the device will write/read correct exp/gain values.
"""

import logging
import os.path

from mobly import test_runner

import its_base_test
import camera_properties_utils
import capture_request_utils
import its_session_utils


NAME = os.path.basename(__file__).split('.')[0]
# Spec to be within 3% but not over for exposure in capture vs exposure request.
RTOL_EXP_GAIN = 0.97
TEST_EXP_RANGE = [6E6, 1E9]  # ns [6ms, 1s]


class ReadWriteTest(its_base_test.ItsBaseTest):
  """Test that the device will write/read correct exp/gain values.
  """

  def test_read_write(self):
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      camera_properties_utils.skip_unless(
          camera_properties_utils.manual_sensor(props) and
          camera_properties_utils.per_frame_control(props))

      valid_formats = ['yuv', 'jpg']
      if camera_properties_utils.raw16(props):
        valid_formats.insert(0, 'raw')
      # grab exp/gain ranges from camera
      sensor_exp_range = props['android.sensor.info.exposureTimeRange']
      sens_range = props['android.sensor.info.sensitivityRange']
      logging.debug('sensor exposure time range: %s', sensor_exp_range)
      logging.debug('sensor sensitivity range: %s', sens_range)

      # determine if exposure test range is within sensor reported range
      assert sensor_exp_range[0] != 0
      exp_range = []
      if sensor_exp_range[0] < TEST_EXP_RANGE[0]:
        exp_range.append(TEST_EXP_RANGE[0])
      else:
        exp_range.append(sensor_exp_range[0])
      if sensor_exp_range[1] > TEST_EXP_RANGE[1]:
        exp_range.append(TEST_EXP_RANGE[1])
      else:
        exp_range.append(sensor_exp_range[1])

      data = {}
      # build requests
      for fmt in valid_formats:
        logging.debug('format: %s', fmt)
        size = capture_request_utils.get_available_output_sizes(fmt, props)[-1]
        out_surface = {'width': size[0], 'height': size[1], 'format': fmt}
        # pylint: disable=protected-access
        if cam._hidden_physical_id:
          out_surface['physicalCamera'] = cam._hidden_physical_id
        reqs = []
        index_list = []
        for exp in exp_range:
          for sens in sens_range:
            reqs.append(capture_request_utils.manual_capture_request(sens, exp))
            index_list.append((fmt, exp, sens))
            logging.debug('exp_write: %d, sens_write: %d', exp, sens)

        # take shots
        caps = cam.do_capture(reqs, out_surface)

        # extract exp/sensitivity data
        for i, cap in enumerate(caps):
          exposure_read = cap['metadata']['android.sensor.exposureTime']
          sensitivity_read = cap['metadata']['android.sensor.sensitivity']
          data[index_list[i]] = (fmt, exposure_read, sensitivity_read)

      # check read/write match across all shots
      e_failed = []
      s_failed = []
      for fmt_write in valid_formats:
        for e_write in exp_range:
          for s_write in sens_range:
            fmt_read, e_read, s_read = data[(fmt_write, e_write, s_write)]
            if (e_write < e_read or e_read / float(e_write) <= RTOL_EXP_GAIN):
              e_failed.append({
                  'format': fmt_read,
                  'e_write': e_write,
                  'e_read': e_read,
                  's_write': s_write,
                  's_read': s_read
              })
            if (s_write < s_read or s_read / float(s_write) <= RTOL_EXP_GAIN):
              s_failed.append({
                  'format': fmt_read,
                  'e_write': e_write,
                  'e_read': e_read,
                  's_write': s_write,
                  's_read': s_read
              })

        # print results
        if e_failed:
          logging.debug('FAILs for exposure time')
          for fail in e_failed:
            logging.debug('format: %s, e_write: %d, e_read: %d, RTOL: %.2f, ',
                          fail['format'], fail['e_write'], fail['e_read'],
                          RTOL_EXP_GAIN)
            logging.debug('s_write: %d, s_read: %d, RTOL: %.2f',
                          fail['s_write'], fail['s_read'], RTOL_EXP_GAIN)
        if s_failed:
          logging.debug('FAILs for sensitivity(ISO)')
          for fail in s_failed:
            logging.debug('format: %s, s_write: %d, s_read: %d, RTOL: %.2f, ',
                          fail['format'], fail['s_write'], fail['s_read'],
                          RTOL_EXP_GAIN)
            logging.debug('e_write: %d, e_read: %d, RTOL: %.2f',
                          fail['e_write'], fail['e_read'], RTOL_EXP_GAIN)

        # assert PASS/FAIL
        assert not e_failed + s_failed


if __name__ == '__main__':
  test_runner.main()
