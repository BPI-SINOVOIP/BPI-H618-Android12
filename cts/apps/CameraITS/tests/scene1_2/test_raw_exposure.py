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
"""Verifies exposure times on RAW images."""


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

BLK_LVL_RTOL = 0.1
BURST_LEN = 10  # break captures into burst of BURST_LEN requests
COLORS = ['R', 'Gr', 'Gb', 'B']
EXP_LONG_THRESH = 1E6  # 1ms
EXP_MULT_SHORT = pow(2, 1.0/3)  # Test 3 steps per 2x exposure
EXP_MULT_LONG = pow(10, 1.0/3)  # Test 3 steps per 10x exposure
IMG_DELTA_THRESH = 0.99  # Each shot must be > 0.99*previous
IMG_SAT_RTOL = 0.01  # 1%
IMG_STATS_GRID = 9  # find used to find the center 11.11%
NAME = os.path.splitext(os.path.basename(__file__))[0]
NS_TO_MS_FACTOR = 1.0E-6
NUM_COLORS = len(COLORS)
NUM_ISO_STEPS = 5


def create_test_exposure_list(e_min, e_max):
  """Create the list of exposure values to test."""
  e_list = []
  mult = 1.0
  while e_min*mult < e_max:
    e_list.append(int(e_min*mult))
    if e_min*mult < EXP_LONG_THRESH:
      mult *= EXP_MULT_SHORT
    else:
      mult *= EXP_MULT_LONG
  if e_list[-1] < e_max*IMG_DELTA_THRESH:
    e_list.append(int(e_max))
  return e_list


def define_raw_stats_fmt(props):
  """Define format with active array width and height."""
  aax = props['android.sensor.info.preCorrectionActiveArraySize']['left']
  aay = props['android.sensor.info.preCorrectionActiveArraySize']['top']
  aaw = props['android.sensor.info.preCorrectionActiveArraySize']['right']-aax
  aah = props['android.sensor.info.preCorrectionActiveArraySize']['bottom']-aay
  return {'format': 'rawStats',
          'gridWidth': aaw // IMG_STATS_GRID,
          'gridHeight': aah // IMG_STATS_GRID}


def create_plot(exps, means, sens, log_path):
  """Create plots R, Gr, Gb, B vs exposures.

  Args:
    exps: array of exposure times in ms
    means: array of means for RAW captures
    sens: int value for ISO gain
    log_path: path to write plot file
  Returns:
    None
  """
  # means[0] is black level value
  r = [m[0] for m in means[1:]]
  gr = [m[1] for m in means[1:]]
  gb = [m[2] for m in means[1:]]
  b = [m[3] for m in means[1:]]
  pylab.figure('%s_%s' % (NAME, sens))
  pylab.plot(exps, r, 'r.-')
  pylab.plot(exps, b, 'b.-')
  pylab.plot(exps, gr, 'g.-')
  pylab.plot(exps, gb, 'k.-')
  pylab.xscale('log')
  pylab.yscale('log')
  pylab.title('%s ISO=%d' % (NAME, sens))
  pylab.xlabel('Exposure time (ms)')
  pylab.ylabel('Center patch pixel mean')
  matplotlib.pyplot.savefig(
      '%s_s=%d.png' % (os.path.join(log_path, NAME), sens))
  pylab.clf()


def assert_increasing_means(means, exps, sens, black_levels, white_level):
  """Assert that each image increases unless over/undersaturated.

  Args:
    means: COLORS means for set of images
    exps: exposure times in ms
    sens: ISO gain value
    black_levels: COLORS black_level values
    white_level: full scale value
  Returns:
    None
  """
  allow_under_saturated = True
  for i in range(1, len(means)):
    prev_mean = means[i-1]
    mean = means[i]

    if np.isclose(max(mean), white_level, rtol=IMG_SAT_RTOL):
      logging.debug('Saturated: white_level %f, max_mean %f',
                    white_level, max(mean))
      break

    if allow_under_saturated and np.allclose(
        mean, black_levels, rtol=BLK_LVL_RTOL):
      # All channel means are close to black level
      continue

    allow_under_saturated = False
    # Check pixel means are increasing (with small tolerance)
    for ch, color in enumerate(COLORS):
      e_msg = 'ISO=%d, %s, exp %3fms mean: %.2f, %s mean: %.2f, TOL=%.f%%' % (
          sens, color, exps[i-1], mean[ch],
          'black level' if i == 1 else 'exp_time %.3fms'%exps[i-2],
          prev_mean[ch], IMG_DELTA_THRESH*100)
      if mean[ch] <= prev_mean[ch] * IMG_DELTA_THRESH:
        raise AssertionError(e_msg)


class RawExposureTest(its_base_test.ItsBaseTest):
  """Capture RAW images with increasing exp time and measure pixel values."""

  def test_raw_exposure(self):
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
          camera_properties_utils.per_frame_control(props) and
          not camera_properties_utils.mono_camera(props))
      log_path = self.log_path

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Create list of exposures
      e_min, e_max = props['android.sensor.info.exposureTimeRange']
      e_test = create_test_exposure_list(e_min, e_max)
      e_test_ms = [e*NS_TO_MS_FACTOR for e in e_test]

      # Capture with rawStats to reduce capture times
      fmt = define_raw_stats_fmt(props)

      # Create sensitivity range from min to max analog sensitivity
      sens_min, _ = props['android.sensor.info.sensitivityRange']
      sens_max = props['android.sensor.maxAnalogSensitivity']
      sens_step = (sens_max - sens_min) // NUM_ISO_STEPS
      white_level = float(props['android.sensor.info.whiteLevel'])
      black_levels = [image_processing_utils.get_black_level(
          i, props) for i in range(NUM_COLORS)]

      # Do captures with exposure list over sensitivity range
      for s in range(sens_min, sens_max, sens_step):
        # Break caps into bursts and do captures
        burst_len = BURST_LEN
        caps = []
        reqs = [capture_request_utils.manual_capture_request(
            s, e, 0) for e in e_test]
        # Eliminate burst len==1. Error because returns [[]], not [{}, ...]
        while len(reqs) % burst_len == 1:
          burst_len -= 1
        # Break caps into bursts
        for i in range(len(reqs) // burst_len):
          caps += cam.do_capture(reqs[i*burst_len:(i+1)*burst_len], fmt)
        last_n = len(reqs) % burst_len
        if last_n:
          caps += cam.do_capture(reqs[-last_n:], fmt)

        # Extract means for each capture
        means = []
        means.append(black_levels)
        for i, cap in enumerate(caps):
          mean_image, _ = image_processing_utils.unpack_rawstats_capture(cap)
          mean = mean_image[IMG_STATS_GRID // 2, IMG_STATS_GRID // 2]
          logging.debug('ISO=%d, exp_time=%.3fms, mean=%s',
                        s, (e_test[i] * NS_TO_MS_FACTOR), str(mean))
          means.append(mean)

        # Create plot
        create_plot(e_test_ms, means, s, log_path)

        # Each shot mean should be brighter (except under/overexposed scene)
        assert_increasing_means(means, e_test_ms, s, black_levels, white_level)

if __name__ == '__main__':
  test_runner.main()
