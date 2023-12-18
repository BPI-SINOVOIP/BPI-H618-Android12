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
"""Verifies android.edge.mode param behavior for reprocessing reqs."""


import logging
import os
import matplotlib
from matplotlib import pylab
from mobly import test_runner
import numpy as np

import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils
import opencv_processing_utils

EDGE_MODES = {'OFF': 0, 'FAST': 1, 'HQ': 2, 'ZSL': 3}
EDGE_MODES_VALUES = list(EDGE_MODES.values())
NAME = os.path.splitext(os.path.basename(__file__))[0]
NUM_SAMPLES = 4
PLOT_COLORS = {'yuv': 'r', 'private': 'g', 'none': 'b'}
SHARPNESS_RTOL = 0.15


def check_edge_modes(sharpness):
  """Check that the sharpness for the different edge modes is correct."""
  logging.debug('Verify HQ is sharper than OFF')
  if sharpness[EDGE_MODES['HQ']] < sharpness[EDGE_MODES['OFF']]:
    raise AssertionError(f"HQ < OFF! HQ: {sharpness[EDGE_MODES['HQ']]:.5f}, "
                         f"OFF: {sharpness[EDGE_MODES['OFF']]:.5f}")

  logging.debug('Verify ZSL is similar to OFF')
  e_msg = 'ZSL: %.5f, OFF: %.5f, RTOL: %.2f' % (
      sharpness[EDGE_MODES['ZSL']], sharpness[EDGE_MODES['OFF']],
      SHARPNESS_RTOL)
  assert np.isclose(sharpness[EDGE_MODES['ZSL']], sharpness[EDGE_MODES['OFF']],
                    SHARPNESS_RTOL), e_msg

  logging.debug('Verify OFF is not sharper than FAST')
  e_msg = 'FAST: %.5f, OFF: %.5f, RTOL: %.2f' % (
      sharpness[EDGE_MODES['FAST']], sharpness[EDGE_MODES['OFF']],
      SHARPNESS_RTOL)
  assert (sharpness[EDGE_MODES['FAST']] >
          sharpness[EDGE_MODES['OFF']] * (1.0-SHARPNESS_RTOL)), e_msg

  logging.debug('Verify FAST is not sharper than HQ')
  e_msg = 'FAST: %.5f, HQ: %.5f, RTOL: %.2f' % (
      sharpness[EDGE_MODES['FAST']], sharpness[EDGE_MODES['HQ']],
      SHARPNESS_RTOL)
  assert (sharpness[EDGE_MODES['HQ']] >
          sharpness[EDGE_MODES['FAST']] * (1.0-SHARPNESS_RTOL)), e_msg


def do_capture_and_determine_sharpness(
    cam, edge_mode, sensitivity, exp, fd, out_surface, chart, log_path,
    reprocess_format=None):
  """Return sharpness of the output images and the capture result metadata.

   Processes a capture request with a given edge mode, sensitivity, exposure
   time, focus distance, output surface parameter, and reprocess format
   (None for a regular request.)

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
    chart: object containing chart information
    log_path: location to save files
    reprocess_format: (Optional) The reprocessing format. If not None,
                      reprocessing will be enabled.

  Returns:
    Object containing reported edge mode and the sharpness of the output
    image, keyed by the following strings:
        'edge_mode'
        'sharpness'
  """

  req = capture_request_utils.manual_capture_request(sensitivity, exp)
  req['android.lens.focusDistance'] = fd
  req['android.edge.mode'] = edge_mode
  if reprocess_format:
    req['android.reprocess.effectiveExposureFactor'] = 1.0

  sharpness_list = []
  caps = cam.do_capture([req]*NUM_SAMPLES, [out_surface], reprocess_format)
  for n in range(NUM_SAMPLES):
    y, _, _ = image_processing_utils.convert_capture_to_planes(caps[n])
    chart.img = image_processing_utils.normalize_img(
        image_processing_utils.get_image_patch(
            y, chart.xnorm, chart.ynorm, chart.wnorm, chart.hnorm))
    if n == 0:
      image_processing_utils.write_image(
          chart.img, '%s_reprocess_fmt_%s_edge=%d.jpg' % (
              os.path.join(log_path, NAME), reprocess_format, edge_mode))
      edge_mode_res = caps[n]['metadata']['android.edge.mode']
    sharpness_list.append(
        image_processing_utils.compute_image_sharpness(chart.img))
  logging.debug('Sharpness list for edge mode %d: %s',
                edge_mode, str(sharpness_list))
  return {'edge_mode': edge_mode_res, 'sharpness': np.mean(sharpness_list)}


class ReprocessEdgeEnhancementTest(its_base_test.ItsBaseTest):
  """Test android.edge.mode param applied when set for reprocessing requests.

  Capture non-reprocess images for each edge mode and calculate their
  sharpness as a baseline.

  Capture reprocessed images for each supported reprocess format and edge_mode
  mode. Calculate the sharpness of reprocessed images and compare them against
  the sharpess of non-reprocess images.
  """

  def test_reprocess_edge_enhancement(self):
    logging.debug('Starting %s', NAME)
    logging.debug('Edge modes: %s', str(EDGE_MODES))
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      chart_loc_arg = self.chart_loc_arg
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      log_path = self.log_path

      # Check skip conditions
      camera_properties_utils.skip_unless(
          camera_properties_utils.read_3a(props) and
          camera_properties_utils.per_frame_control(props) and
          camera_properties_utils.edge_mode(props, 0) and
          (camera_properties_utils.yuv_reprocess(props) or
           camera_properties_utils.private_reprocess(props)))

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Initialize chart class and locate chart in scene
      chart = opencv_processing_utils.Chart(
          cam, props, self.log_path, chart_loc=chart_loc_arg)

      # If reprocessing is supported, ZSL edge mode must be avaiable.
      assert camera_properties_utils.edge_mode(
          props, EDGE_MODES['ZSL']), 'ZSL android.edge.mode not available!'

      reprocess_formats = []
      if camera_properties_utils.yuv_reprocess(props):
        reprocess_formats.append('yuv')
      if camera_properties_utils.private_reprocess(props):
        reprocess_formats.append('private')

      size = capture_request_utils.get_available_output_sizes('jpg', props)[0]
      logging.debug('image W: %d, H: %d', size[0], size[1])
      out_surface = {'width': size[0], 'height': size[1], 'format': 'jpg'}

      # Get proper sensitivity, exposure time, and focus distance.
      mono_camera = camera_properties_utils.mono_camera(props)
      s, e, _, _, fd = cam.do_3a(get_results=True, mono_camera=mono_camera)

      # Initialize plot
      pylab.figure('reprocess_result')
      pylab.title(NAME)
      pylab.xlabel('Edge Enhance Mode')
      pylab.ylabel('Sharpness')
      pylab.xticks(EDGE_MODES_VALUES)

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
            cam, edge_mode, s, e, fd, out_surface, chart, log_path)
        edge_mode_reported_regular.append(ret['edge_mode'])
        sharpness_regular.append(ret['sharpness'])

      pylab.plot(EDGE_MODES_VALUES, sharpness_regular,
                 '-'+PLOT_COLORS['none']+'o', label='None')
      logging.debug('Sharpness for edge modes with regular request: %s',
                    str(sharpness_regular))

      # Get sharpness for each edge mode and reprocess format
      sharpnesses_reprocess = []
      edge_mode_reported_reprocess = []

      for reprocess_format in reprocess_formats:
        # List of sharpness
        sharpnesses = []
        edge_mode_reported = []
        for edge_mode in range(4):
          # Skip unavailable modes
          if not camera_properties_utils.edge_mode(props, edge_mode):
            edge_mode_reported.append(edge_mode)
            sharpnesses.append(0)
            continue

          ret = do_capture_and_determine_sharpness(
              cam, edge_mode, s, e, fd, out_surface, chart, log_path,
              reprocess_format)
          edge_mode_reported.append(ret['edge_mode'])
          sharpnesses.append(ret['sharpness'])

        sharpnesses_reprocess.append(sharpnesses)
        edge_mode_reported_reprocess.append(edge_mode_reported)

        # Add to plot and log results
        pylab.plot(EDGE_MODES_VALUES, sharpnesses,
                   '-'+PLOT_COLORS[reprocess_format]+'o',
                   label=reprocess_format)
        logging.debug('Sharpness for edge modes w/ %s reprocess fmt: %s',
                      reprocess_format, str(sharpnesses))
      # Finalize plot
      pylab.legend(numpoints=1, fancybox=True)
      matplotlib.pyplot.savefig('%s_plot.png' %
                                os.path.join(log_path, NAME))
      logging.debug('Check regular requests')
      check_edge_modes(sharpness_regular)

      for reprocess_format in range(len(reprocess_formats)):
        logging.debug('Check reprocess format: %s', reprocess_format)
        check_edge_modes(sharpnesses_reprocess[reprocess_format])

        hq_div_off_reprocess = (
            sharpnesses_reprocess[reprocess_format][EDGE_MODES['HQ']] /
            sharpnesses_reprocess[reprocess_format][EDGE_MODES['OFF']])
        hq_div_off_regular = (
            sharpness_regular[EDGE_MODES['HQ']] /
            sharpness_regular[EDGE_MODES['OFF']])
        e_msg = 'HQ/OFF_reprocess: %.4f, HQ/OFF_reg: %.4f, RTOL: %.2f' % (
            hq_div_off_reprocess, hq_div_off_regular, SHARPNESS_RTOL)
        logging.debug('Verify reprocess HQ ~= reg HQ relative to OFF')
        assert np.isclose(hq_div_off_reprocess, hq_div_off_regular,
                          SHARPNESS_RTOL), e_msg

if __name__ == '__main__':
  test_runner.main()

