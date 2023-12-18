# Copyright 2020 The Android Open Source Project
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
"""Verify that the android.control.afSceneChange asserted on scene change."""


import logging
import multiprocessing
import os.path
import time

from mobly import test_runner

import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils
import scene_change_utils

_BRIGHT_CHANGE_TOL = 0.2
_CONTINUOUS_PICTURE_MODE = 4
_CONVERGED_3A = (2, 2, 2)  # (AE, AF, AWB)
_DELAY_CAPTURE = 1.5  # Delay in first capture to sync events (sec).
_DELAY_DISPLAY = 3.0  # Time when display turns OFF (sec).
_FPS = 30  # Frames Per Second
_M_TO_CM = 100
_NAME = os.path.splitext(os.path.basename(__file__))[0]
_NSEC_TO_MSEC = 1E-6
_NUM_TRIES = 6
_NUM_FRAMES = 50
_PATCH_H = 0.1  # Center 10%.
_PATCH_W = 0.1
_PATCH_X = 0.5 - _PATCH_W/2
_PATCH_Y = 0.5 - _PATCH_H/2
_RGB_G_CH = 1
_SCENE_CHANGE_FLAG_TRUE = 1
_VALID_SCENE_CHANGE_VALS = (0, 1)
_VGA_W, _VGA_H = 640, 480


def find_3a_converged_frame(cap_data):
  converged_frame = -1
  for i, cap in enumerate(cap_data):
    if cap['3a_state'] == _CONVERGED_3A:
      converged_frame = i
      break
  logging.debug('Frame index where 3A converges: %d', converged_frame)
  return converged_frame


def determine_if_scene_changed(cap_data, converged_frame):
  """Determine if the scene has changed during captures.

  Args:
    cap_data: Camera capture object.
    converged_frame: Integer indicating when 3A converged.

  Returns:
    A 2-tuple of booleans where the first is for AF scene change flag asserted
    and the second is for whether brightness in images changed.
  """
  scene_change_flag = False
  bright_change_flag = False
  start_frame_brightness = cap_data[0]['avg']
  for i in range(converged_frame, len(cap_data)):
    if cap_data[i]['avg'] <= (
        start_frame_brightness * (1.0 - _BRIGHT_CHANGE_TOL)):
      bright_change_flag = True
    if cap_data[i]['flag'] == _SCENE_CHANGE_FLAG_TRUE:
      scene_change_flag = True
  return scene_change_flag, bright_change_flag


def toggle_screen(tablet, delay=1):
  """Sets the chart host screen display level .

  Args:
    tablet: Object for screen tablet.
    delay: Float value for time delay. Default is 1 second.
  """
  t0 = time.time()
  if delay >= 0:
    time.sleep(delay)
  else:
    raise ValueError(f'Screen toggle time shifted to {delay} w/o scene change. '
                     'Tablet does not appear to be toggling. Check setup.')
  tablet.adb.shell('input keyevent KEYCODE_POWER')
  t = time.time() - t0
  logging.debug('Toggling display at %.3f.', t)


def capture_frames(cam, delay, burst, log_path):
  """Capture NUM_FRAMES frames and log metadata.

  3A state information:
    AE_STATES: {0: INACTIVE, 1: SEARCHING, 2: CONVERGED, 3: LOCKED,
                4: FLASH_REQ, 5: PRECAPTURE}
    AF_STATES: {0: INACTIVE, 1: PASSIVE_SCAN, 2: PASSIVE_FOCUSED,
                3: ACTIVE_SCAN, 4: FOCUS_LOCKED, 5: NOT_FOCUSED_LOCKED,
                6: PASSIVE_UNFOCUSED}
    AWB_STATES: {0: INACTIVE, 1: SEARCHING, 2: CONVERGED, 3: LOCKED}

  Args:
    cam: Camera object.
    delay: Float value for time delay in seconds.
    burst: Integer number of burst index.
    log_path: String location to save images.
  Returns:
    cap_data_list. List of dicts for each capture.
  """
  cap_data_list = []
  req = capture_request_utils.auto_capture_request()
  req['android.control.afMode'] = _CONTINUOUS_PICTURE_MODE
  fmt = {'format': 'yuv', 'width': _VGA_W, 'height': _VGA_H}
  t0 = time.time()
  time.sleep(delay)
  logging.debug('cap event start: %.6f', time.time() - t0)
  caps = cam.do_capture([req]*_NUM_FRAMES, fmt)
  logging.debug('cap event stop: %.6f', time.time() - t0)

  # Extract frame metadata.
  for i, cap in enumerate(caps):
    cap_data = {}
    exp = cap['metadata']['android.sensor.exposureTime'] * _NSEC_TO_MSEC
    iso = cap['metadata']['android.sensor.sensitivity']
    focal_length = cap['metadata']['android.lens.focalLength']
    ae_state = cap['metadata']['android.control.aeState']
    af_state = cap['metadata']['android.control.afState']
    awb_state = cap['metadata']['android.control.awbState']
    if focal_length:
      fl_str = str(round(_M_TO_CM/focal_length, 2)) + 'cm'
    else:
      fl_str = 'infinity'
    flag = cap['metadata']['android.control.afSceneChange']
    if flag not in _VALID_SCENE_CHANGE_VALS:
      raise AssertionError(f'afSceneChange not a valid value: {flag}.')
    img = image_processing_utils.convert_capture_to_rgb_image(cap)
    image_processing_utils.write_image(
        img, '%s_%d_%d.jpg' % (os.path.join(log_path, _NAME), burst, i))
    patch = image_processing_utils.get_image_patch(
        img, _PATCH_X, _PATCH_Y, _PATCH_W, _PATCH_H)
    green_avg = image_processing_utils.compute_image_means(patch)[_RGB_G_CH]
    logging.debug(
        '%d, iso: %d, exp: %.2fms, fd: %s, avg: %.3f, 3A: [%d,%d,%d], flag: %d',
        i, iso, exp, fl_str, green_avg, ae_state, af_state, awb_state, flag)
    cap_data['3a_state'] = (ae_state, af_state, awb_state)
    cap_data['avg'] = green_avg
    cap_data['flag'] = flag
    cap_data_list.append(cap_data)
  return cap_data_list


class SceneChangeTest(its_base_test.ItsBaseTest):
  """Tests that AF scene change detected metadata changes for scene change.

  Confirm android.control.afSceneChangeDetected is asserted when scene changes.

  Does continuous capture with face scene during scene change. With no scene
  change, behavior should be similar to scene2_b/test_continuous_picture.
  Scene change is modeled with scene tablet powered down during continuous
  capture. If tablet does not exist, scene change can be modeled with hand wave
  in front of camera.

  Depending on scene brightness changes and scene change flag assertions during
  test, adjust tablet timing to move scene change to appropriate timing for
  test.
  """

  def test_scene_change(self):
    logging.debug('Starting %s', _NAME)
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      log_path = self.log_path
      tablet = self.tablet

      # Check SKIP conditions.
      camera_properties_utils.skip_unless(
          camera_properties_utils.continuous_picture(props) and
          camera_properties_utils.af_scene_change(props))

      # Load chart for scene.
      its_session_utils.load_scene(
          cam, props, self.scene, tablet, self.chart_distance)

      # Do captures with scene change.
      tablet_level = int(self.tablet_screen_brightness)
      logging.debug('Tablet brightness: %d', tablet_level)
      scene_change_delay = _DELAY_DISPLAY
      cam.do_3a()  # Do 3A up front to settle camera.
      for burst in range(_NUM_TRIES):
        logging.debug('burst number: %d', burst)
        # Create scene change by turning off chart display & capture frames
        if tablet:
          multiprocessing.Process(name='p1', target=toggle_screen,
                                  args=(tablet, scene_change_delay,)).start()
        else:
          print('Wave hand in front of camera to create scene change.')
        cap_data = capture_frames(cam, _DELAY_CAPTURE, burst, log_path)

        # Find frame where 3A converges and final brightness.
        converged_frame = find_3a_converged_frame(cap_data)
        converged_flag = True if converged_frame != -1 else False
        bright_final = cap_data[_NUM_FRAMES - 1]['avg']

        # Determine if scene changed.
        scene_change_flag, bright_change_flag = determine_if_scene_changed(
            cap_data, converged_frame)

        # Adjust timing based on captured frames and scene change flags.
        timing_adjustment = scene_change_utils.calc_timing_adjustment(
            converged_flag, scene_change_flag, bright_change_flag, bright_final)
        if timing_adjustment == scene_change_utils.SCENE_CHANGE_PASS_CODE:
          break
        elif timing_adjustment == scene_change_utils.SCENE_CHANGE_FAIL_CODE:
          raise AssertionError('Test fails. Check logging.error.')
        else:
          if burst == _NUM_TRIES-1:  # FAIL out after NUM_TRIES.
            raise AssertionError(f'No scene change in {_NUM_TRIES}x tries.')
          else:
            scene_change_delay += timing_adjustment / _FPS

        if tablet:
          logging.debug('Turning screen back ON.')
          toggle_screen(tablet)


if __name__ == '__main__':
  test_runner.main()
