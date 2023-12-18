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
"""Verifies sensitivities on RAW images."""


import logging
import os.path
import matplotlib
from matplotlib import pylab
from mobly import test_runner

import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils
import opencv_processing_utils

GR_PLANE_IDX = 1  # GR plane index in RGGB data
IMG_STATS_GRID = 9  # Center 11.11%
NAME = os.path.splitext(os.path.basename(__file__))[0]
NUM_SENS_STEPS = 5
VAR_THRESH = 1.01  # Each shot must be 1% noisier than previous


def define_raw_stats_fmt(props):
  """Define format with active array width and height."""
  aaw = (props['android.sensor.info.preCorrectionActiveArraySize']['right'] -
         props['android.sensor.info.preCorrectionActiveArraySize']['left'])
  aah = (props['android.sensor.info.preCorrectionActiveArraySize']['bottom'] -
         props['android.sensor.info.preCorrectionActiveArraySize']['top'])
  logging.debug('Active array W,H: %d,%d', aaw, aah)
  return {'format': 'rawStats',
          'gridWidth': aaw // IMG_STATS_GRID,
          'gridHeight': aah // IMG_STATS_GRID}


class RawSensitivityTest(its_base_test.ItsBaseTest):
  """Capture a set of raw images with increasing gains and measure the noise."""

  def test_raw_sensitivity(self):
    logging.debug('Starting %s', NAME)
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      camera_properties_utils.skip_unless(
          camera_properties_utils.raw16(props) and
          camera_properties_utils.manual_sensor(props) and
          camera_properties_utils.read_3a(props) and
          camera_properties_utils.per_frame_control(props) and
          not camera_properties_utils.mono_camera(props))
      name_with_log_path = os.path.join(self.log_path, NAME)
      camera_fov = float(cam.calc_camera_fov(props))

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Expose for the scene with min sensitivity
      sens_min, _ = props['android.sensor.info.sensitivityRange']
      # Digital gains might not be visible on RAW data
      sens_max = props['android.sensor.maxAnalogSensitivity']
      sens_step = (sens_max - sens_min) // NUM_SENS_STEPS

      # Skip AF if TELE camera
      if camera_fov <= opencv_processing_utils.FOV_THRESH_TELE:
        s_ae, e_ae, _, _, _ = cam.do_3a(do_af=False, get_results=True)
        f_dist = 0
      else:
        s_ae, e_ae, _, _, f_dist = cam.do_3a(get_results=True)
      s_e_prod = s_ae * e_ae

      sensitivities = list(range(sens_min, sens_max, sens_step))
      variances = []
      for s in sensitivities:
        e = int(s_e_prod / float(s))
        req = capture_request_utils.manual_capture_request(s, e, f_dist)

        # Capture in rawStats to reduce test run time
        fmt = define_raw_stats_fmt(props)
        cap = cam.do_capture(req, fmt)

        if self.debug_mode:
          img = image_processing_utils.convert_capture_to_rgb_image(
              cap, props=props)
          image_processing_utils.write_image(
              img, f'{name_with_log_path}_{s}_{e}ns.jpg', True)

        # Measure variance
        _, var_image = image_processing_utils.unpack_rawstats_capture(cap)
        cfa_idxs = image_processing_utils.get_canonical_cfa_order(props)
        white_level = float(props['android.sensor.info.whiteLevel'])
        var = var_image[IMG_STATS_GRID//2, IMG_STATS_GRID//2,
                        cfa_idxs[GR_PLANE_IDX]]/white_level**2
        logging.debug('s=%d, e=%d, var=%e', s, e, var)
        variances.append(var)

      # Create plot
      pylab.figure(NAME)
      pylab.plot(sensitivities, variances, '-ro')
      pylab.xticks(sensitivities)
      pylab.xlabel('Sensitivities')
      pylab.ylabel('Image Center Patch Variance')
      pylab.ticklabel_format(axis='y', style='sci', scilimits=(-6, -6))
      pylab.title(NAME)
      matplotlib.pyplot.savefig(f'{name_with_log_path}_variances.png')

      # Test that each shot is noisier than previous
      for i in range(len(variances) - 1):
        if variances[i] >= variances[i+1]/VAR_THRESH:
          raise AssertionError(f'variances [i]: {variances[i]:5f}, [i+1]: '
                               f'{variances[i+1]:.5f}, THRESH: {VAR_THRESH}')

if __name__ == '__main__':
  test_runner.main()
