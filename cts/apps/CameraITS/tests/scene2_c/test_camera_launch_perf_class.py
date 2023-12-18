# Copyright 2021 The Android Open Source Project
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
"""Verify camera startup is < 500ms for both front and back primary cameras.
"""

import logging

from mobly import test_runner

import camera_properties_utils
import its_base_test
import its_session_utils

CAMERA_LAUNCH_S_PERFORMANCE_CLASS_THRESHOLD = 500  # ms


class CameraLaunchSPerfClassTest(its_base_test.ItsBaseTest):
  """Test camera launch latency for S performance class as specified in CDD.

  [7.5/H-1-7] MUST have camera2 startup latency (open camera to first preview
  frame) < 500ms as measured by the CTS camera PerformanceTest under ITS
  lighting conditions (3000K) for both primary cameras.
  """

  def test_camera_launch(self):
    # Open camera with "with" semantics to check skip condition and load chart
    #
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id) as cam:

      camera_properties_utils.skip_unless(
          cam.is_performance_class_primary_camera())

      # Load chart for scene.
      props = cam.get_camera_properties()
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

    # Create an its session without opening the camera to test camera launch
    # latency
    cam = its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id)

    launch_ms = cam.measure_camera_launch_ms()
    if launch_ms >= CAMERA_LAUNCH_S_PERFORMANCE_CLASS_THRESHOLD:
      raise AssertionError(f'camera launch time: {launch_ms} ms, THRESH: '
                           f'{CAMERA_LAUNCH_S_PERFORMANCE_CLASS_THRESHOLD} ms')
    else:
      logging.debug('camera launch time: %.1f ms', launch_ms)

if __name__ == '__main__':
  test_runner.main()
