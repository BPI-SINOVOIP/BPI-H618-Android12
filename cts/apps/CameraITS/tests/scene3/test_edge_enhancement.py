# Copyright 2015 The Android Open Source Project
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
"""Verifies android.edge.mode works properly."""


import logging
import os
from mobly import test_runner
import numpy as np

import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils
import opencv_processing_utils

EDGE_MODES = {'OFF': 0, 'FAST': 1, 'HQ': 2, 'ZSL': 3}
NAME = os.path.splitext(os.path.basename(__file__))[0]
NUM_SAMPLES = 4
SHARPNESS_RTOL = 0.1


def do_capture_and_determine_sharpness(
    cam, edge_mode, sensitivity, exp, fd, out_surface, chart, log_path):
  """Return sharpness of the output image and the capture result metadata.

     Processes a capture request with a given edge mode, sensitivity, exposure
     time, focus distance, output surface parameter.

  Args:
    cam: An open device session.
    edge_mode: Edge mode for the request as defined in android.edge.mode
    sensitivity: Sensitivity for the request as defined in
                 android.sensor.sensitivity
    exp: Exposure time for the request as defined in
         android.sensor.exposureTime.
    fd: Focus distance for the request as defined in
        android.lens.focusDistance
    out_surface: Specifications of the output image format and size.
    chart: object that contains chart information
    log_path: path to write result images

  Returns:
    Object containing reported edge mode and the sharpness of the output
    image, keyed by the following strings:
        edge_mode
        sharpness
  """

  req = capture_request_utils.manual_capture_request(sensitivity, exp)
  req['android.lens.focusDistance'] = fd
  req['android.edge.mode'] = edge_mode

  sharpness_list = []
  for n in range(NUM_SAMPLES):
    cap = cam.do_capture(req, out_surface, repeat_request=req)
    y, _, _ = image_processing_utils.convert_capture_to_planes(cap)
    chart.img = image_processing_utils.normalize_img(
        image_processing_utils.get_image_patch(
            y, chart.xnorm, chart.ynorm, chart.wnorm, chart.hnorm))
    if n == 0:
      image_processing_utils.write_image(
          chart.img, '%s_edge=%d.jpg' % (
              os.path.join(log_path, NAME), edge_mode))
      edge_mode_res = cap['metadata']['android.edge.mode']
    sharpness_list.append(
        image_processing_utils.compute_image_sharpness(chart.img))

  return {'edge_mode': edge_mode_res, 'sharpness': np.mean(sharpness_list)}


class EdgeEnhancementTest(its_base_test.ItsBaseTest):
  """Test that the android.edge.mode param is applied correctly.

  Capture non-reprocess images for each edge mode and calculate their
  sharpness as a baseline.
  """

  def test_edge_enhancement(self):
    logging.debug('Starting %s', NAME)
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      chart_loc_arg = self.chart_loc_arg
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)

      # Check skip conditions
      camera_properties_utils.skip_unless(
          camera_properties_utils.read_3a(props) and
          camera_properties_utils.per_frame_control(props) and
          camera_properties_utils.edge_mode(props, 0))

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Initialize chart class and locate chart in scene
      chart = opencv_processing_utils.Chart(
          cam, props, self.log_path, chart_loc=chart_loc_arg)

      # Define format
      fmt = 'yuv'
      size = capture_request_utils.get_available_output_sizes(fmt, props)[0]
      out_surface = {'width': size[0], 'height': size[1], 'format': fmt}

      # Get proper sensitivity, exposure time, and focus distance.
      mono_camera = camera_properties_utils.mono_camera(props)
      s, e, _, _, fd = cam.do_3a(get_results=True, mono_camera=mono_camera)

      # Get the sharpness for each edge mode for regular requests
      sharpness_regular = []
      edge_mode_reported_regular = []
      for edge_mode in EDGE_MODES.values():
        # Skip unavailable modes
        if not camera_properties_utils.edge_mode(props, edge_mode):
          edge_mode_reported_regular.append(edge_mode)
          sharpness_regular.append(0)
          continue

        ret = do_capture_and_determine_sharpness(
            cam, edge_mode, s, e, fd, out_surface, chart, self.log_path)
        edge_mode_reported_regular.append(ret['edge_mode'])
        sharpness_regular.append(ret['sharpness'])

      logging.debug('Reported edge modes: %s', edge_mode_reported_regular)
      logging.debug('Sharpness with EE mode [0,1,2,3]: %s',
                    str(sharpness_regular))

      logging.debug('Verify HQ is sharper than OFF')
      e_msg = 'HQ: %.3f, OFF: %.3f' % (sharpness_regular[EDGE_MODES['HQ']],
                                       sharpness_regular[EDGE_MODES['OFF']])
      assert (sharpness_regular[EDGE_MODES['HQ']] >
              sharpness_regular[EDGE_MODES['OFF']]), e_msg

      logging.debug('Verify OFF is not sharper than FAST')
      e_msg = 'FAST: %.3f, OFF: %.3f, RTOL: %.2f' % (
          sharpness_regular[EDGE_MODES['FAST']],
          sharpness_regular[EDGE_MODES['OFF']], SHARPNESS_RTOL)
      assert (sharpness_regular[EDGE_MODES['FAST']] >
              sharpness_regular[EDGE_MODES['OFF']]*(1.0-SHARPNESS_RTOL)), e_msg

      logging.debug('Verify FAST is not sharper than HQ')
      e_msg = 'HQ: %.3f, FAST: %.3f, RTOL: %.2f' % (
          sharpness_regular[EDGE_MODES['HQ']],
          sharpness_regular[EDGE_MODES['FAST']], SHARPNESS_RTOL)
      assert (sharpness_regular[EDGE_MODES['HQ']] >
              sharpness_regular[EDGE_MODES['FAST']]*(1.0-SHARPNESS_RTOL)), e_msg

if __name__ == '__main__':
  test_runner.main()
