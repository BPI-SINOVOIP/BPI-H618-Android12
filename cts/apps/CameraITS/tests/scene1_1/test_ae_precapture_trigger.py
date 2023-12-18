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
"""Verifies AE state machine when using precapture trigger."""


import logging
import os
from mobly import test_runner

import its_base_test
import camera_properties_utils
import capture_request_utils
import its_session_utils
import target_exposure_utils

AE_INACTIVE = 0
AE_SEARCHING = 1
AE_CONVERGED = 2
AE_LOCKED = 3  # not used in this test
AE_FLASHREQUIRED = 4  # not used in this test
AE_PRECAPTURE = 5
FRAMES_AE_DISABLED = 5
FRAMES_PER_ITERATION = 8
ITERATIONS_TO_CONVERGE = 5
NAME = os.path.splitext(os.path.basename(__file__))[0]
START_AE_PRECAP_TRIG = 1
STOP_AE_PRECAP_TRIG = 0


class AePrecaptureTest(its_base_test.ItsBaseTest):
  """Test the AE state machine when using the precapture trigger.
  """

  def test_ae_precapture(self):
    logging.debug('Starting %s', NAME)
    logging.debug('AE_INACTIVE: %d', AE_INACTIVE)
    logging.debug('AE_SEARCHING: %d', AE_SEARCHING)
    logging.debug('AE_CONVERGED: %d', AE_CONVERGED)
    logging.debug('AE_PRECAPTURE: %d', AE_PRECAPTURE)

    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)

      # Check SKIP conditions
      camera_properties_utils.skip_unless(
          camera_properties_utils.compute_target_exposure(props) and
          camera_properties_utils.per_frame_control(props))

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      _, fmt = capture_request_utils.get_fastest_manual_capture_settings(props)

      # Capture 5 manual requests with AE disabled and the last request
      # has an AE precapture trigger (which should be ignored since AE is
      # disabled).
      logging.debug('Manual captures')
      manual_reqs = []
      e, s = target_exposure_utils.get_target_exposure_combos(
          self.log_path, cam)['midExposureTime']
      manual_req = capture_request_utils.manual_capture_request(s, e)
      manual_req['android.control.aeMode'] = AE_INACTIVE
      manual_reqs += [manual_req] * (FRAMES_AE_DISABLED-1)
      precap_req = capture_request_utils.manual_capture_request(s, e)
      precap_req['android.control.aeMode'] = AE_INACTIVE
      precap_req['android.control.aePrecaptureTrigger'] = START_AE_PRECAP_TRIG
      manual_reqs.append(precap_req)
      caps = cam.do_capture(manual_reqs, fmt)
      for i, cap in enumerate(caps):
        state = cap['metadata']['android.control.aeState']
        msg = 'AE state after manual request %d: %d' % (i, state)
        logging.debug('%s', msg)
        e_msg = msg + ' AE_INACTIVE: %d' % AE_INACTIVE
        assert state == AE_INACTIVE, e_msg

      # Capture auto request and verify the AE state: no trigger.
      logging.debug('Auto capture')
      auto_req = capture_request_utils.auto_capture_request()
      auto_req['android.control.aeMode'] = AE_SEARCHING
      cap = cam.do_capture(auto_req, fmt)
      state = cap['metadata']['android.control.aeState']
      msg = 'AE state after auto request: %d' % state
      logging.debug('%s', msg)
      e_msg = msg + ' AE_SEARCHING: %d, AE_CONVERGED: %d' % (
          AE_SEARCHING, AE_CONVERGED)
      assert state in [AE_SEARCHING, AE_CONVERGED], e_msg

      # Capture auto request with a precapture trigger.
      logging.debug('Auto capture with precapture trigger')
      auto_req['android.control.aePrecaptureTrigger'] = START_AE_PRECAP_TRIG
      cap = cam.do_capture(auto_req, fmt)
      state = cap['metadata']['android.control.aeState']
      msg = 'AE state after auto request with precapture trigger: %d' % state
      logging.debug('%s', msg)
      e_msg = msg + ' AE_SEARCHING: %d, AE_CONVERGED: %d, AE_PRECAPTURE: %d' % (
          AE_SEARCHING, AE_CONVERGED, AE_PRECAPTURE)
      assert state in [AE_SEARCHING, AE_CONVERGED, AE_PRECAPTURE], e_msg

      # Capture some more auto requests, and AE should converge.
      logging.debug('Additional auto captures')
      auto_req['android.control.aePrecaptureTrigger'] = STOP_AE_PRECAP_TRIG
      for _ in range(ITERATIONS_TO_CONVERGE):
        caps = cam.do_capture([auto_req] * FRAMES_PER_ITERATION, fmt)
        state = caps[-1]['metadata']['android.control.aeState']
        msg = 'AE state after auto request: %d' % state
        logging.debug('%s', msg)
        if state == AE_CONVERGED:
          return
      e_msg = msg + ' AE_CONVERGED: %d' % AE_CONVERGED
      assert state == AE_CONVERGED, e_msg

if __name__ == '__main__':
  test_runner.main()
