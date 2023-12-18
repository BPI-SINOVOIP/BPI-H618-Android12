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
"""Verifies correct exposure control."""


import logging
import os.path
import matplotlib
from matplotlib import pylab

from mobly import test_runner
import numpy as np

import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils
import target_exposure_utils


NAME = os.path.splitext(os.path.basename(__file__))[0]
NUM_PTS_2X_GAIN = 3  # 3 points every 2x increase in gain
PATCH_H = 0.1  # center 10% patch params
PATCH_W = 0.1
PATCH_X = 0.45
PATCH_Y = 0.45
RAW_STATS_GRID = 9  # define 9x9 (11.11%) spacing grid for rawStats processing
RAW_STATS_XY = RAW_STATS_GRID//2  # define X, Y location for center rawStats
THRESH_MIN_LEVEL = 0.1
THRESH_MAX_LEVEL = 0.9
THRESH_MAX_LEVEL_DIFF = 0.045
THRESH_MAX_LEVEL_DIFF_WIDE_RANGE = 0.06
THRESH_MAX_OUTLIER_DIFF = 0.1
THRESH_ROUND_DOWN_GAIN = 0.1
THRESH_ROUND_DOWN_EXP = 0.03
THRESH_ROUND_DOWN_EXP0 = 1.00  # TOL at 0ms exp; theoretical limit @ 4-line exp
THRESH_EXP_KNEE = 6E6  # exposures less than knee have relaxed tol
WIDE_EXP_RANGE_THRESH = 64.0  # threshold for 'wide' range sensor


def plot_rgb_means(title, x, r, g, b, log_path):
  """Plot the RGB mean data.

  Args:
    title: string for figure title
    x: x values for plot, gain multiplier
    r: r plane means
    g: g plane means
    b: b plane menas
    log_path: path for saved files
  """
  pylab.figure(title)
  pylab.semilogx(x, r, 'ro-')
  pylab.semilogx(x, g, 'go-')
  pylab.semilogx(x, b, 'bo-')
  pylab.title(NAME + title)
  pylab.xlabel('Gain Multiplier')
  pylab.ylabel('Normalized RGB Plane Avg')
  pylab.minorticks_off()
  pylab.xticks(x[0::NUM_PTS_2X_GAIN], x[0::NUM_PTS_2X_GAIN])
  pylab.ylim([0, 1])
  plot_name = '%s_plot_means.png' % os.path.join(log_path, NAME)
  matplotlib.pyplot.savefig(plot_name)


def plot_raw_means(title, x, r, gr, gb, b, log_path):
  """Plot the RAW mean data.

  Args:
    title: string for figure title
    x: x values for plot, gain multiplier
    r: R plane means
    gr: Gr plane means
    gb: Gb plane means
    b: B plane menas
    log_path: path for saved files
  """
  pylab.figure(title)
  pylab.semilogx(x, r, 'ro-', label='R')
  pylab.semilogx(x, gr, 'go-', label='Gr')
  pylab.semilogx(x, gb, 'ko-', label='Gb')
  pylab.semilogx(x, b, 'bo-', label='B')
  pylab.title(NAME + title)
  pylab.xlabel('Gain Multiplier')
  pylab.ylabel('Normalized RAW Plane Avg')
  pylab.minorticks_off()
  pylab.xticks(x[0::NUM_PTS_2X_GAIN], x[0::NUM_PTS_2X_GAIN])
  pylab.ylim([0, 1])
  pylab.legend(numpoints=1)
  plot_name = '%s_plot_raw_means.png' % os.path.join(log_path, NAME)
  matplotlib.pyplot.savefig(plot_name)


def check_line_fit(chan, mults, values, thresh_max_level_diff):
  """Find line fit and check values.

  Check for linearity. Verify sample pixel mean values are close to each
  other. Also ensure that the images aren't clamped to 0 or 1
  (which would also make them look like flat lines).

  Args:
    chan: integer number to define RGB or RAW channel
    mults: list of multiplication values for gain*m, exp/m
    values: mean values for chan
    thresh_max_level_diff: threshold for max difference
  """

  m, b = np.polyfit(mults, values, 1).tolist()
  min_val = min(values)
  max_val = max(values)
  max_diff = max_val - min_val
  logging.debug('Channel %d line fit (y = mx+b): m = %f, b = %f', chan, m, b)
  logging.debug('Channel min %f max %f diff %f', min_val, max_val, max_diff)
  if max_diff >= thresh_max_level_diff:
    raise AssertionError(f'max_diff: {max_diff:.4f}, '
                         f'THRESH: {thresh_max_level_diff:.3f}')
  if not THRESH_MAX_LEVEL > b > THRESH_MIN_LEVEL:
    raise AssertionError(f'b: {b:.2f}, THRESH_MIN: {THRESH_MIN_LEVEL}, '
                         f'THRESH_MAX: {THRESH_MAX_LEVEL}')
  for v in values:
    if not THRESH_MAX_LEVEL > v > THRESH_MIN_LEVEL:
      raise AssertionError(f'v: {v:.2f}, THRESH_MIN: {THRESH_MIN_LEVEL}, '
                           f'THRESH_MAX: {THRESH_MAX_LEVEL}')

    if abs(v - b) >= THRESH_MAX_OUTLIER_DIFF:
      raise AssertionError(f'v: {v:.2f}, b: {b:.2f}, '
                           f'THRESH_DIFF: {THRESH_MAX_OUTLIER_DIFF}')


def get_raw_active_array_size(props):
  """Return the active array w, h from props."""
  aaw = (props['android.sensor.info.preCorrectionActiveArraySize']['right'] -
         props['android.sensor.info.preCorrectionActiveArraySize']['left'])
  aah = (props['android.sensor.info.preCorrectionActiveArraySize']['bottom'] -
         props['android.sensor.info.preCorrectionActiveArraySize']['top'])
  return aaw, aah


class ExposureTest(its_base_test.ItsBaseTest):
  """Test that a constant exposure is seen as ISO and exposure time vary.

  Take a series of shots that have ISO and exposure time chosen to balance
  each other; result should be the same brightness, but over the sequence
  the images should get noisier.
  """

  def test_exposure(self):
    mults = []
    r_means = []
    g_means = []
    b_means = []
    raw_r_means = []
    raw_gr_means = []
    raw_gb_means = []
    raw_b_means = []
    thresh_max_level_diff = THRESH_MAX_LEVEL_DIFF

    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)

      # Check SKIP conditions
      camera_properties_utils.skip_unless(
          camera_properties_utils.compute_target_exposure(props))

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Initialize params for requests
      debug = self.debug_mode
      raw_avlb = (camera_properties_utils.raw16(props) and
                  camera_properties_utils.manual_sensor(props))
      sync_latency = camera_properties_utils.sync_latency(props)
      logging.debug('sync latency: %d frames', sync_latency)
      largest_yuv = capture_request_utils.get_largest_yuv_format(props)
      match_ar = (largest_yuv['width'], largest_yuv['height'])
      fmt = capture_request_utils.get_smallest_yuv_format(
          props, match_ar=match_ar)
      e, s = target_exposure_utils.get_target_exposure_combos(
          self.log_path, cam)['minSensitivity']
      s_e_product = s*e
      expt_range = props['android.sensor.info.exposureTimeRange']
      sens_range = props['android.sensor.info.sensitivityRange']
      m = 1.0

      # Do captures with a range of exposures, but constant s*e
      while s*m < sens_range[1] and e/m > expt_range[0]:
        mults.append(m)
        s_req = round(s * m)
        e_req = s_e_product // s_req
        logging.debug('Testing s: %d, e: %dns', s_req, e_req)
        req = capture_request_utils.manual_capture_request(
            s_req, e_req, 0.0, True, props)
        cap = its_session_utils.do_capture_with_latency(
            cam, req, sync_latency, fmt)
        s_res = cap['metadata']['android.sensor.sensitivity']
        e_res = cap['metadata']['android.sensor.exposureTime']
        # determine exposure tolerance based on exposure time
        if e_req >= THRESH_EXP_KNEE:
          thresh_round_down_exp = THRESH_ROUND_DOWN_EXP
        else:
          thresh_round_down_exp = (
              THRESH_ROUND_DOWN_EXP +
              (THRESH_ROUND_DOWN_EXP0 - THRESH_ROUND_DOWN_EXP) *
              (THRESH_EXP_KNEE - e_req) / THRESH_EXP_KNEE)
        if not 0 <= s_req - s_res < s_req * THRESH_ROUND_DOWN_GAIN:
          raise AssertionError(f's_req: {s_req}, s_res: {s_res}, '
                               f'TOL=-{THRESH_ROUND_DOWN_GAIN*100}%')
        if not 0 <= e_req - e_res < e_req * thresh_round_down_exp:
          raise AssertionError(f'e_req: {e_req}ns, e_res: {e_res}ns, '
                               f'TOL=-{thresh_round_down_exp*100}%')
        s_e_product_res = s_res * e_res
        req_res_ratio = s_e_product / s_e_product_res
        logging.debug('Capture result s: %d, e: %dns', s_res, e_res)
        img = image_processing_utils.convert_capture_to_rgb_image(cap)
        image_processing_utils.write_image(
            img, '%s_mult=%3.2f.jpg' % (os.path.join(self.log_path, NAME), m))
        patch = image_processing_utils.get_image_patch(
            img, PATCH_X, PATCH_Y, PATCH_W, PATCH_H)
        rgb_means = image_processing_utils.compute_image_means(patch)
        # Adjust for the difference between request and result
        r_means.append(rgb_means[0] * req_res_ratio)
        g_means.append(rgb_means[1] * req_res_ratio)
        b_means.append(rgb_means[2] * req_res_ratio)

        # Do with RAW_STATS space if debug
        if raw_avlb and debug:
          aaw, aah = get_raw_active_array_size(props)
          fmt_raw = {'format': 'rawStats',
                     'gridWidth': aaw//RAW_STATS_GRID,
                     'gridHeight': aah//RAW_STATS_GRID}
          raw_cap = its_session_utils.do_capture_with_latency(
              cam, req, sync_latency, fmt_raw)
          r, gr, gb, b = image_processing_utils.convert_capture_to_planes(
              raw_cap, props)
          raw_r_means.append(r[RAW_STATS_XY, RAW_STATS_XY] * req_res_ratio)
          raw_gr_means.append(gr[RAW_STATS_XY, RAW_STATS_XY] * req_res_ratio)
          raw_gb_means.append(gb[RAW_STATS_XY, RAW_STATS_XY] * req_res_ratio)
          raw_b_means.append(b[RAW_STATS_XY, RAW_STATS_XY] * req_res_ratio)

          # Test number of points per 2x gain
        m *= pow(2, 1.0/NUM_PTS_2X_GAIN)

      # Loosen threshold for devices with wider exposure range
      if m >= WIDE_EXP_RANGE_THRESH:
        thresh_max_level_diff = THRESH_MAX_LEVEL_DIFF_WIDE_RANGE

    # Draw plots and check data
    if raw_avlb and debug:
      plot_raw_means('RAW data', mults, raw_r_means, raw_gr_means, raw_gb_means,
                     raw_b_means, self.log_path)
      for ch, _ in enumerate(['r', 'gr', 'gb', 'b']):
        values = [raw_r_means, raw_gr_means, raw_gb_means, raw_b_means][ch]
        check_line_fit(ch, mults, values, thresh_max_level_diff)

    plot_rgb_means('RGB data', mults, r_means, g_means, b_means, self.log_path)
    for ch, _ in enumerate(['r', 'g', 'b']):
      values = [r_means, g_means, b_means][ch]
      check_line_fit(ch, mults, values, thresh_max_level_diff)

if __name__ == '__main__':
  test_runner.main()
