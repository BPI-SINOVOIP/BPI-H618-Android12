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
"""Utility functions for sensor_fusion hardware rig."""


import bisect
import codecs
import logging
import struct
import time
import unittest

import numpy as np
import scipy.spatial
import serial
from serial.tools import list_ports

# Constants for Rotation Rig
ARDUINO_ANGLE_MAX = 180.0  # degrees
ARDUINO_ANGLES = [0]*5 +list(range(0, 90, 3)) + [90]*5 +list(range(90, -1, -3))
ARDUINO_BAUDRATE = 9600
ARDUINO_CMD_LENGTH = 3
ARDUINO_CMD_TIME = 2.0 * ARDUINO_CMD_LENGTH / ARDUINO_BAUDRATE  # round trip
ARDUINO_MOVE_TIME = 0.06 - ARDUINO_CMD_TIME  # seconds
ARDUINO_PID = 0x0043
ARDUINO_START_BYTE = 255
ARDUINO_START_NUM_TRYS = 3
ARDUINO_TEST_CMD = (b'\x01', b'\x02', b'\x03')
ARDUINO_VALID_CH = ('1', '2', '3', '4', '5', '6')
ARDUINO_VIDS = (0x2341, 0x2a03)

CANAKIT_BAUDRATE = 115200
CANAKIT_CMD_TIME = 0.05  # seconds (found experimentally)
CANAKIT_DATA_DELIMITER = '\r\n'
CANAKIT_PID = 0xfc73
CANAKIT_SEND_TIMEOUT = 0.02  # seconds
CANAKIT_SET_CMD = 'REL'
CANAKIT_SLEEP_TIME = 2  # seconds (for full 90 degree rotation)
CANAKIT_VALID_CMD = ('ON', 'OFF')
CANAKIT_VALID_CH = ('1', '2', '3', '4')
CANAKIT_VID = 0x04d8

HS755HB_ANGLE_MAX = 202.0  # throw for rotation motor in degrees

_COARSE_FIT_RANGE = 20  # Range area around coarse fit to do optimization.
_CORR_TIME_OFFSET_MAX = 50  # ms max shift to try and match camera/gyro times.
_CORR_TIME_OFFSET_STEP = 0.5  # ms step for shifts.

# Unit translators
_MSEC_TO_NSEC = 1000000
_NSEC_TO_SEC = 1E-9
_SEC_TO_NSEC = int(1/_NSEC_TO_SEC)


def serial_port_def(name):
  """Determine the serial port and open.

  Args:
    name: string of device to locate (ie. 'Arduino', 'Canakit' or 'Default')
  Returns:
    serial port object
  """
  serial_port = None
  devices = list_ports.comports()
  for device in devices:
    if not (device.vid and device.pid):  # Not all comm ports have vid and pid
      continue
    if name.lower() == 'arduino':
      if (device.vid in ARDUINO_VIDS and device.pid == ARDUINO_PID):
        logging.debug('Arduino: %s', str(device))
        serial_port = device.device
        return serial.Serial(serial_port, ARDUINO_BAUDRATE, timeout=1)

    elif name.lower() in ('canakit', 'default'):
      if (device.vid == CANAKIT_VID and device.pid == CANAKIT_PID):
        logging.debug('Canakit: %s', str(device))
        serial_port = device.device
        return serial.Serial(serial_port, CANAKIT_BAUDRATE,
                             timeout=CANAKIT_SEND_TIMEOUT,
                             parity=serial.PARITY_EVEN,
                             stopbits=serial.STOPBITS_ONE,
                             bytesize=serial.EIGHTBITS)
  raise ValueError(f'{name} device not connected.')


def canakit_cmd_send(canakit_serial_port, cmd_str):
  """Wrapper for sending serial command to Canakit.

  Args:
    canakit_serial_port: port to write for canakit
    cmd_str: str; value to send to device.
  """
  try:
    logging.debug('writing port...')
    canakit_serial_port.write(CANAKIT_DATA_DELIMITER.encode())
    time.sleep(CANAKIT_CMD_TIME)  # This is critical for relay.
    canakit_serial_port.write(cmd_str.encode())

  except IOError:
    raise IOError(f'Port {CANAKIT_VID}:{CANAKIT_PID} is not open!')


def canakit_set_relay_channel_state(canakit_port, ch, state):
  """Set Canakit relay channel and state.

  Waits CANAKIT_SLEEP_TIME for rotation to occur.

  Args:
    canakit_port: serial port object for the Canakit port.
    ch: string for channel number of relay to set. '1', '2', '3', or '4'
    state: string of either 'ON' or 'OFF'
  """
  logging.debug('Setting relay state %s', state)
  if ch in CANAKIT_VALID_CH and state in CANAKIT_VALID_CMD:
    canakit_cmd_send(canakit_port, CANAKIT_SET_CMD + ch + '.' + state + '\r\n')
    time.sleep(CANAKIT_SLEEP_TIME)
  else:
    logging.debug('Invalid ch (%s) or state (%s), no command sent.', ch, state)


def arduino_read_cmd(port):
  """Read back Arduino command from serial port."""
  cmd = []
  for _ in range(ARDUINO_CMD_LENGTH):
    cmd.append(port.read())
  return cmd


def arduino_send_cmd(port, cmd):
  """Send command to serial port."""
  for i in range(ARDUINO_CMD_LENGTH):
    port.write(cmd[i])


def arduino_loopback_cmd(port, cmd):
  """Send command to serial port."""
  arduino_send_cmd(port, cmd)
  time.sleep(ARDUINO_CMD_TIME)
  return arduino_read_cmd(port)


def establish_serial_comm(port):
  """Establish connection with serial port."""
  logging.debug('Establishing communication with %s', port.name)
  trys = 1
  hex_test = convert_to_hex(ARDUINO_TEST_CMD)
  logging.debug(' test tx: %s %s %s', hex_test[0], hex_test[1], hex_test[2])
  while trys <= ARDUINO_START_NUM_TRYS:
    cmd_read = arduino_loopback_cmd(port, ARDUINO_TEST_CMD)
    hex_read = convert_to_hex(cmd_read)
    logging.debug(' test rx: %s %s %s', hex_read[0], hex_read[1], hex_read[2])
    if cmd_read != list(ARDUINO_TEST_CMD):
      trys += 1
    else:
      logging.debug(' Arduino comm established after %d try(s)', trys)
      break


def convert_to_hex(cmd):
  return [('%0.2x' % int(codecs.encode(x, 'hex_codec'), 16) if x else '--')
          for x in cmd]


def arduino_rotate_servo_to_angle(ch, angle, serial_port, delay=0):
  """Rotate servo to the specified angle.

  Args:
    ch: str; servo to rotate in ARDUINO_VALID_CH
    angle: int; servo angle to move to
    serial_port: object; serial port
    delay: int; time in seconds
  """
  if angle < 0 or angle > ARDUINO_ANGLE_MAX:
    logging.debug('Angle must be between 0 and %d.', ARDUINO_ANGLE_MAX)
    angle = 0
    if angle > ARDUINO_ANGLE_MAX:
      angle = ARDUINO_ANGLE_MAX

  cmd = [struct.pack('B', i) for i in [ARDUINO_START_BYTE, int(ch), angle]]
  arduino_send_cmd(serial_port, cmd)
  time.sleep(delay)


def arduino_rotate_servo(ch, serial_port):
  """Rotate servo between 0 --> 90 --> 0.

  Args:
    ch: str; servo to rotate
    serial_port: object; serial port
  """
  for angle in ARDUINO_ANGLES:
    angle_norm = int(round(angle*ARDUINO_ANGLE_MAX/HS755HB_ANGLE_MAX, 0))
    arduino_rotate_servo_to_angle(
        ch, angle_norm, serial_port, ARDUINO_MOVE_TIME)


def rotation_rig(rotate_cntl, rotate_ch, num_rotations):
  """Rotate the phone n times using rotate_cntl and rotate_ch defined.

  rotate_ch is hard wired and must be determined from physical setup.

  First initialize the port and send a test string defined by ARDUINO_TEST_CMD
  to establish communications. Then rotate servo motor to origin position.

  Args:
    rotate_cntl: str to identify as 'arduino' or 'canakit' controller.
    rotate_ch: str to identify rotation channel number.
    num_rotations: int number of rotations.
  """

  logging.debug('Controller: %s, ch: %s', rotate_cntl, rotate_ch)
  if rotate_cntl.lower() == 'arduino':
    # identify port
    arduino_serial_port = serial_port_def('Arduino')

    # send test cmd to Arduino until cmd returns properly
    establish_serial_comm(arduino_serial_port)

    # initialize servo at origin
    logging.debug('Moving servo to origin')
    arduino_rotate_servo_to_angle(rotate_ch, 0, arduino_serial_port, 1)

  elif rotate_cntl.lower() == 'canakit':
    canakit_serial_port = serial_port_def('Canakit')

  # rotate phone
  logging.debug('Rotating phone %dx', num_rotations)
  for _ in range(num_rotations):
    if rotate_cntl == 'arduino':
      arduino_rotate_servo(rotate_ch, arduino_serial_port)
    else:
      canakit_set_relay_channel_state(canakit_serial_port, rotate_ch, 'ON')
      canakit_set_relay_channel_state(canakit_serial_port, rotate_ch, 'OFF')
  logging.debug('Finished rotations')


def get_gyro_rotations(gyro_events, cam_times):
  """Get the rotation values of the gyro.

  Integrates the gyro data between each camera frame to compute an angular
  displacement.

  Args:
    gyro_events: List of gyro event objects.
    cam_times: Array of N camera times, one for each frame.

  Returns:
    Array of N-1 gyro rotation measurements (rads/s).
  """
  gyro_times = np.array([e['time'] for e in gyro_events])
  all_gyro_rots = np.array([e['z'] for e in gyro_events])
  gyro_rots = []
  if gyro_times[0] > cam_times[0] or gyro_times[-1] < cam_times[-1]:
    raise AssertionError('Gyro times do not bound camera times! '
                         f'gyro: {gyro_times[0]:.0f} -> {gyro_times[-1]:.0f} '
                         f'cam: {cam_times[0]} -> {cam_times[-1]} (ns).')
  # Integrate the gyro data between each pair of camera frame times.
  for i_cam in range(len(cam_times)-1):
    # Get the window of gyro samples within the current pair of frames.
    # Note: bisect always picks first gyro index after camera time.
    t_cam0 = cam_times[i_cam]
    t_cam1 = cam_times[i_cam+1]
    i_gyro_window0 = bisect.bisect(gyro_times, t_cam0)
    i_gyro_window1 = bisect.bisect(gyro_times, t_cam1)
    gyro_sum = 0

    # Integrate samples within the window.
    for i_gyro in range(i_gyro_window0, i_gyro_window1):
      gyro_val = all_gyro_rots[i_gyro+1]
      t_gyro0 = gyro_times[i_gyro]
      t_gyro1 = gyro_times[i_gyro+1]
      t_gyro_delta = (t_gyro1 - t_gyro0) * _NSEC_TO_SEC
      gyro_sum += gyro_val * t_gyro_delta

    # Handle the fractional intervals at the sides of the window.
    for side, i_gyro in enumerate([i_gyro_window0-1, i_gyro_window1]):
      gyro_val = all_gyro_rots[i_gyro+1]
      t_gyro0 = gyro_times[i_gyro]
      t_gyro1 = gyro_times[i_gyro+1]
      t_gyro_delta = (t_gyro1 - t_gyro0) * _NSEC_TO_SEC
      if side == 0:
        f = (t_cam0 - t_gyro0) / (t_gyro1 - t_gyro0)
        frac_correction = gyro_val * t_gyro_delta * (1.0 - f)
        gyro_sum += frac_correction
      else:
        f = (t_cam1 - t_gyro0) / (t_gyro1 - t_gyro0)
        frac_correction = gyro_val * t_gyro_delta * f
        gyro_sum += frac_correction

    gyro_rots.append(gyro_sum)
  gyro_rots = np.array(gyro_rots)
  return gyro_rots


def get_best_alignment_offset(cam_times, cam_rots, gyro_events):
  """Find the best offset to align the camera and gyro motion traces.

  This function integrates the shifted gyro data between camera samples
  for a range of candidate shift values, and returns the shift that
  result in the best correlation.

  Uses a correlation distance metric between the curves, where a smaller
  value means that the curves are better-correlated.

  Fits a curve to the correlation distance data to measure the minima more
  accurately, by looking at the correlation distances within a range of
  +/- 10ms from the measured best score; note that this will use fewer
  than the full +/- 10 range for the curve fit if the measured score
  (which is used as the center of the fit) is within 10ms of the edge of
  the +/- 50ms candidate range.

  Args:
    cam_times: Array of N camera times, one for each frame.
    cam_rots: Array of N-1 camera rotation displacements (rad).
    gyro_events: List of gyro event objects.

  Returns:
    Best alignment offset(ms), fit coefficients, candidates, and distances.
  """
  # Measure the correlation distance over defined shift
  shift_candidates = np.arange(-_CORR_TIME_OFFSET_MAX,
                               _CORR_TIME_OFFSET_MAX+_CORR_TIME_OFFSET_STEP,
                               _CORR_TIME_OFFSET_STEP).tolist()
  spatial_distances = []
  for shift in shift_candidates:
    shifted_cam_times = cam_times + shift*_MSEC_TO_NSEC
    gyro_rots = get_gyro_rotations(gyro_events, shifted_cam_times)
    spatial_distance = scipy.spatial.distance.correlation(cam_rots, gyro_rots)
    logging.debug('shift %.1fms spatial distance: %.5f', shift,
                  spatial_distance)
    spatial_distances.append(spatial_distance)

  best_corr_dist = min(spatial_distances)
  coarse_best_shift = shift_candidates[spatial_distances.index(best_corr_dist)]
  logging.debug('Best shift without fitting is %.4f ms', coarse_best_shift)

  # Fit a 2nd order polynomial around coarse_best_shift to extract best fit
  i = spatial_distances.index(best_corr_dist)
  i_poly_fit_min = i - _COARSE_FIT_RANGE
  i_poly_fit_max = i + _COARSE_FIT_RANGE + 1
  shift_candidates = shift_candidates[i_poly_fit_min:i_poly_fit_max]
  spatial_distances = spatial_distances[i_poly_fit_min:i_poly_fit_max]
  fit_coeffs = np.polyfit(shift_candidates, spatial_distances, 2)  # ax^2+bx+c
  exact_best_shift = -fit_coeffs[1]/(2*fit_coeffs[0])
  if abs(coarse_best_shift - exact_best_shift) > 2.0:
    raise AssertionError(
        f'Test failed. Bad fit to time-shift curve. Coarse best shift: '
        f'{coarse_best_shift}, Exact best shift: {exact_best_shift}.')
  if fit_coeffs[0] <= 0 or fit_coeffs[2] <= 0:
    raise AssertionError(
        f'Coefficients are < 0: a: {fit_coeffs[0]}, c: {fit_coeffs[2]}.')

  return exact_best_shift, fit_coeffs, shift_candidates, spatial_distances


class SensorFusionUtilsTests(unittest.TestCase):
  """Run a suite of unit tests on this module."""

  _CAM_FRAME_TIME = 30 * _MSEC_TO_NSEC  # Similar to 30FPS
  _CAM_ROT_AMPLITUDE = 0.04  # Empirical number for rotation per frame (rads/s).

  def _generate_pwl_waveform(self, pts, step, amplitude):
    """Helper function to generate piece wise linear waveform."""
    pwl_waveform = []
    for t in range(pts[0], pts[1], step):
      pwl_waveform.append(0)
    for t in range(pts[1], pts[2], step):
      pwl_waveform.append((t-pts[1])/(pts[2]-pts[1])*amplitude)
    for t in range(pts[2], pts[3], step):
      pwl_waveform.append(amplitude)
    for t in range(pts[3], pts[4], step):
      pwl_waveform.append((pts[4]-t)/(pts[4]-pts[3])*amplitude)
    for t in range(pts[4], pts[5], step):
      pwl_waveform.append(0)
    for t in range(pts[5], pts[6], step):
      pwl_waveform.append((-1*(t-pts[5])/(pts[6]-pts[5]))*amplitude)
    for t in range(pts[6], pts[7], step):
      pwl_waveform.append(-1*amplitude)
    for t in range(pts[7], pts[8], step):
      pwl_waveform.append((t-pts[8])/(pts[8]-pts[7])*amplitude)
    for t in range(pts[8], pts[9], step):
      pwl_waveform.append(0)
    return pwl_waveform

  def _generate_test_waveforms(self, gyro_sampling_rate, t_offset=0):
    """Define ideal camera/gryo behavior.

    Args:
      gyro_sampling_rate: Value in samples/sec.
      t_offset: Value in ns for gyro/camera timing offset.

    Returns:
      cam_times: numpy array of camera times N values long.
      cam_rots: numpy array of camera rotations N-1 values long.
      gyro_events: list of dicts of gyro events N*gyro_sampling_rate/30 long.

    Round trip for motor is ~2 seconds (~60 frames)
            1111111111111111
           i                i
          i                  i
         i                    i
     0000                      0000                      0000
                                   i                    i
                                    i                  i
                                     i                i
                                      -1-1-1-1-1-1-1-1
    t_0 t_1 t_2           t_3 t_4 t_5 t_6           t_7 t_8 t_9

    Note gyro waveform must extend +/- _CORR_TIME_OFFSET_MAX to enable shifting
    of camera waveform to find best correlation.

    """

    t_ramp = 4 * self._CAM_FRAME_TIME
    pts = {}
    pts[0] = 3 * self._CAM_FRAME_TIME
    pts[1] = pts[0] + 3 * self._CAM_FRAME_TIME
    pts[2] = pts[1] + t_ramp
    pts[3] = pts[2] + 32 * self._CAM_FRAME_TIME
    pts[4] = pts[3] + t_ramp
    pts[5] = pts[4] + 4 * self._CAM_FRAME_TIME
    pts[6] = pts[5] + t_ramp
    pts[7] = pts[6] + 32 * self._CAM_FRAME_TIME
    pts[8] = pts[7] + t_ramp
    pts[9] = pts[8] + 4 * self._CAM_FRAME_TIME
    cam_times = np.array(range(pts[0], pts[9], self._CAM_FRAME_TIME))
    cam_rots = self._generate_pwl_waveform(
        pts, self._CAM_FRAME_TIME, self._CAM_ROT_AMPLITUDE)
    cam_rots.pop()  # rots is N-1 for N length times.
    cam_rots = np.array(cam_rots)

    # Generate gyro waveform.
    gyro_step = int(round(_SEC_TO_NSEC/gyro_sampling_rate, 0))
    gyro_pts = {k: v+t_offset+self._CAM_FRAME_TIME//2 for k, v in pts.items()}
    gyro_pts[0] = 0  # adjust end pts to bound camera
    gyro_pts[9] += self._CAM_FRAME_TIME*2  # adjust end pt to bound camera
    gyro_rot_amplitude = (
        self._CAM_ROT_AMPLITUDE / self._CAM_FRAME_TIME * _SEC_TO_NSEC)
    gyro_rots = self._generate_pwl_waveform(
        gyro_pts, gyro_step, gyro_rot_amplitude)

    # Create gyro events list of dicts.
    gyro_events = []
    for i, t in enumerate(range(gyro_pts[0], gyro_pts[9], gyro_step)):
      gyro_events.append({'time': t, 'z': gyro_rots[i]})

    return cam_times, cam_rots, gyro_events

  def test_get_gyro_rotations(self):
    """Tests that gyro rotations are masked properly by camera rotations.

    Note that waveform ideal waveform generation only works properly with
    integer multiples of frame rate.
    """
    # Run with different sampling rates to validate.
    for gyro_sampling_rate in [200, 1000]:  # 6x, 30x frame rate
      cam_times, cam_rots, gyro_events = self._generate_test_waveforms(
          gyro_sampling_rate)
      gyro_rots = get_gyro_rotations(gyro_events, cam_times)
      e_msg = f'gyro sampling rate = {gyro_sampling_rate}\n'
      e_msg += f'cam_times = {list(cam_times)}\n'
      e_msg += f'cam_rots = {list(cam_rots)}\n'
      e_msg += f'gyro_rots = {list(gyro_rots)}'

      self.assertTrue(np.allclose(
          gyro_rots, cam_rots, atol=self._CAM_ROT_AMPLITUDE*0.10), e_msg)

  def test_get_best_alignment_offset(self):
    """Unittest for alignment offset check."""

    gyro_sampling_rate = 5000
    for t_offset_ms in [0, 1]:  # Run with different offsets to validate.
      t_offset = int(t_offset_ms * _MSEC_TO_NSEC)
      cam_times, cam_rots, gyro_events = self._generate_test_waveforms(
          gyro_sampling_rate, t_offset)

      best_fit_offset, coeffs, x, y = get_best_alignment_offset(
          cam_times, cam_rots, gyro_events)
      e_msg = f'best: {best_fit_offset} ms\n'
      e_msg += f'coeffs: {coeffs}\n'
      e_msg += f'x: {x}\n'
      e_msg += f'y: {y}'
      self.assertTrue(np.isclose(t_offset_ms, best_fit_offset, atol=0.1), e_msg)


if __name__ == '__main__':
  unittest.main()

