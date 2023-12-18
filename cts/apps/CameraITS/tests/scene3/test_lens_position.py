# Copyright 2016 The Android Open Source Project
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
"""Verifies android.lens.focusDistance for lens moving and stationary."""


import logging
import os
from mobly import test_runner
import numpy as np

import its_base_test
import camera_properties_utils
import capture_request_utils
import error_util
import image_processing_utils
import its_session_utils
import opencv_processing_utils

FRAME_ATOL_MS = 10  # ms
LENS_MOVING_STATE = 1
NAME = os.path.splitext(os.path.basename(__file__))[0]
NSEC_TO_MSEC = 1.0E-6
NUM_TRYS = 2
NUM_STEPS = 6
POSITION_RTOL = 0.1
SHARPNESS_RTOL = 0.1
VGA_W, VGA_H = 640, 480


def assert_static_frames_behavior(d_stat):
  """Assert locations/sharpness are correct in static frames."""
  logging.debug('Asserting static lens locations/sharpness are similar')
  for i in range(len(d_stat) // 2):
    j = 2 * NUM_STEPS - 1 - i
    rw_msg = 'fd_write: %.3f, fd_read: %.3f, RTOL: %.2f' % (
        d_stat[i]['fd'], d_stat[i]['loc'], POSITION_RTOL)
    fr_msg = 'loc_fwd[%d]: %.3f, loc_rev[%d]: %.3f, RTOL: %.2f' % (
        i, d_stat[i]['loc'], j, d_stat[j]['loc'], POSITION_RTOL)
    s_msg = 'sharpness_fwd: %.3f, sharpness_rev: %.3f, RTOL: %.2f' % (
        d_stat[i]['sharpness'], d_stat[j]['sharpness'], SHARPNESS_RTOL)
    assert np.isclose(d_stat[i]['loc'], d_stat[i]['fd'],
                      rtol=POSITION_RTOL), rw_msg
    assert np.isclose(d_stat[i]['loc'], d_stat[j]['loc'],
                      rtol=POSITION_RTOL), fr_msg
    assert np.isclose(d_stat[i]['sharpness'], d_stat[j]['sharpness'],
                      rtol=SHARPNESS_RTOL), s_msg


def assert_moving_frames_behavior(d_move, d_stat):
  """Assert locations/sharpness are correct for consecutive moving frames."""
  logging.debug('Asserting moving frames are consecutive')
  times = [v['timestamp'] for v in d_move.values()]
  diffs = np.gradient(times)
  assert np.isclose(np.amin(diffs), np.amax(diffs),
                    atol=FRAME_ATOL_MS), 'ATOL(ms): %.1f' % FRAME_ATOL_MS

  logging.debug('Asserting moving lens locations/sharpness are similar')
  for i in range(len(d_move)):
    e_msg = 'static: %.3f, moving: %.3f, RTOL: %.2f' % (
        d_stat[i]['loc'], d_move[i]['loc'], POSITION_RTOL)
    assert np.isclose(d_stat[i]['loc'], d_move[i]['loc'],
                      rtol=POSITION_RTOL), e_msg
    if d_move[i]['lens_moving'] and i > 0:
      e_msg = '%d sharpness[stat]: %.2f ' % (i-1, d_stat[i-1]['sharpness'])
      e_msg += '%d sharpness[stat]: %.2f, [move]: %.2f, RTOL: %.1f' % (
          i, d_stat[i]['sharpness'], d_move[i]['sharpness'], SHARPNESS_RTOL)
      if d_stat[i]['sharpness'] > d_stat[i-1]['sharpness']:
        assert (d_stat[i]['sharpness'] * (1.0 + SHARPNESS_RTOL) >
                d_move[i]['sharpness'] > d_stat[i-1]['sharpness'] *
                (1.0 - SHARPNESS_RTOL)), e_msg
      else:
        assert (d_stat[i-1]['sharpness'] * (1.0 + SHARPNESS_RTOL) >
                d_move[i]['sharpness'] > d_stat[i]['sharpness'] *
                (1.0 - SHARPNESS_RTOL)), e_msg
    elif not d_move[i]['lens_moving']:
      e_msg = '%d sharpness[stat]: %.2f, [move]: %.2f, RTOL: %.1f' % (
          i, d_stat[i]['sharpness'], d_move[i]['sharpness'], SHARPNESS_RTOL)
      assert np.isclose(d_stat[i]['sharpness'], d_move[i]['sharpness'],
                        rtol=SHARPNESS_RTOL), e_msg
    else:
      raise error_util.Error('Lens is moving at frame 0!')


def take_caps_and_return_data(cam, props, fmt, sens, exp, chart, log_path):
  """Return fd, sharpness, lens state of the output images.

  Args:
    cam: An open device session
    props: Properties of cam
    fmt: Dict for capture format
    sens: Sensitivity for 3A request as defined in android.sensor.sensitivity
    exp: Exposure time for 3A request as defined in android.sensor.exposureTime
    chart: Object with chart properties
    log_path: Location to save images

  Returns:
    Dictionary of results for different focal distance captures with static
    lens positions and moving lens positions: d_static, d_moving
  """

  # initialize variables and take data sets
  data_static = {}
  data_moving = {}
  white_level = int(props['android.sensor.info.whiteLevel'])
  min_fd = props['android.lens.info.minimumFocusDistance']
  hyperfocal = props['android.lens.info.hyperfocalDistance']
  # create forward + back list of focal distances
  fds_f = np.arange(hyperfocal, min_fd, (min_fd-hyperfocal)/(NUM_STEPS-1))
  fds_f = np.append(fds_f, min_fd)
  fds_fb = list(fds_f) + list(reversed(fds_f))

  # take static data set
  for i, fd in enumerate(fds_fb):
    req = capture_request_utils.manual_capture_request(sens, exp)
    req['android.lens.focusDistance'] = fd
    cap = image_processing_utils.stationary_lens_cap(cam, req, fmt)
    data = {'fd': fds_fb[i]}
    data['loc'] = cap['metadata']['android.lens.focusDistance']
    y, _, _ = image_processing_utils.convert_capture_to_planes(cap, props)
    chart.img = image_processing_utils.normalize_img(
        image_processing_utils.get_image_patch(y, chart.xnorm, chart.ynorm,
                                               chart.wnorm, chart.hnorm))
    image_processing_utils.write_image(chart.img, '%s_stat_i=%d_chart.jpg' % (
        os.path.join(log_path, NAME), i))
    data['sharpness'] = white_level*image_processing_utils.compute_image_sharpness(
        chart.img)
    data_static[i] = data

  # take moving data set
  reqs = []
  for i, fd in enumerate(fds_f):
    reqs.append(capture_request_utils.manual_capture_request(sens, exp))
    reqs[i]['android.lens.focusDistance'] = fd
  caps = cam.do_capture(reqs, fmt)
  for i, cap in enumerate(caps):
    data = {'fd': fds_f[i]}
    data['loc'] = cap['metadata']['android.lens.focusDistance']
    data['lens_moving'] = (
        cap['metadata']['android.lens.state'] == LENS_MOVING_STATE)
    timestamp = cap['metadata']['android.sensor.timestamp'] * NSEC_TO_MSEC
    if i == 0:
      timestamp_init = timestamp
    timestamp -= timestamp_init
    data['timestamp'] = timestamp
    y, _, _ = image_processing_utils.convert_capture_to_planes(cap, props)
    y = image_processing_utils.rotate_img_per_argv(y)
    chart.img = image_processing_utils.normalize_img(
        image_processing_utils.get_image_patch(
            y, chart.xnorm, chart.ynorm, chart.wnorm, chart.hnorm))
    image_processing_utils.write_image(chart.img, '%s_move_i=%d_chart.jpg' % (
        os.path.join(log_path, NAME), i))
    data['sharpness'] = (
        white_level * image_processing_utils.compute_image_sharpness(chart.img))
    data_moving[i] = data
  return data_static, data_moving


class LensPositionReportingTest(its_base_test.ItsBaseTest):
  """Test if focus position is properly reported for moving lenses."""

  def test_lens_position_reporting(self):
    logging.debug('Starting %s', NAME)
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
          not camera_properties_utils.fixed_focus(props) and
          camera_properties_utils.read_3a(props) and
          camera_properties_utils.lens_calibrated(props))

      # Calculate camera_fov and load scaled image on tablet.
      its_session_utils.load_scene(cam, props, self.scene, self.tablet,
                                   self.chart_distance)

      # Initialize chart class and locate chart in scene
      chart = opencv_processing_utils.Chart(
          cam, props, self.log_path, chart_loc=chart_loc_arg)

      # Initialize capture format
      fmt = {'format': 'yuv', 'width': VGA_W, 'height': VGA_H}

      # Get proper sensitivity and exposure time with 3A
      mono_camera = camera_properties_utils.mono_camera(props)
      s, e, _, _, _ = cam.do_3a(get_results=True, mono_camera=mono_camera)

      # Take caps and get sharpness for each focal distance
      d_stat, d_move = take_caps_and_return_data(
          cam, props, fmt, s, e, chart, log_path)

      # Summarize info for log file and easier debug
      logging.debug('Lens stationary')
      for k in sorted(d_stat):
        logging.debug(
            'i: %d\tfd: %.3f\tlens location (diopters): %.3f \t'
            'sharpness: %.1f', k, d_stat[k]['fd'], d_stat[k]['loc'],
            d_stat[k]['sharpness'])
      logging.debug('Lens moving')
      for k in sorted(d_move):
        logging.debug(
            'i: %d\tfd: %.3f\tlens location (diopters): %.3f \t'
            'sharpness: %.1f  \tlens_moving: %r \t'
            'timestamp: %.1fms', k, d_move[k]['fd'], d_move[k]['loc'],
            d_move[k]['sharpness'], d_move[k]['lens_moving'],
            d_move[k]['timestamp'])

      # assert reported location/sharpness is correct in static frames
      assert_static_frames_behavior(d_stat)

      # assert reported location/sharpness is correct in moving frames
      assert_moving_frames_behavior(d_move, d_stat)


if __name__ == '__main__':
  test_runner.main()
