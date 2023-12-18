# Copyright 2019 The Android Open Source Project
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
"""Verifies per_frame_control."""


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

_AE_STATE_CONVERGED = 2
_AE_STATE_FLASH_REQUIRED = 4
_DELTA_GAIN_THRESH = 3  # >3% gain change --> luma change in same dir.
_DELTA_LUMA_THRESH = 3  # 3% frame-to-frame noise test_burst_sameness_manual.
_DELTA_NO_GAIN_THRESH = 1  # <1% gain change --> min luma change.
_NAME = os.path.splitext(os.path.basename(__file__))[0]
_NS_TO_MS = 1.0E-6
_NUM_CAPS = 1
_NUM_FRAMES = 30
_PATCH_H = 0.1  # Center 10%.
_PATCH_W = 0.1
_PATCH_X = 0.5 - _PATCH_W/2
_PATCH_Y = 0.5 - _PATCH_H/2
_RAW_NIBBLE_SIZE = 6  # Used to increase NUM_CAPS & decrease NUM_FRAMES for RAW.
_RAW_GR_CH = 1
_VALID_LUMA_MIN = 0.1
_VALID_LUMA_MAX = 0.9
_YUV_Y_CH = 0


def _check_delta_luma_vs_delta_gain(fmt, j, lumas, total_gains):
  """Determine if luma and gain move together for current frame."""
  delta_gain = total_gains[j] - total_gains[j-1]
  delta_luma = lumas[j] - lumas[j-1]
  delta_gain_rel = delta_gain / total_gains[j-1] * 100  # %
  delta_luma_rel = delta_luma / lumas[j-1] * 100  # %
  # luma and total_gain should change in same direction
  if abs(delta_gain_rel) > _DELTA_GAIN_THRESH:
    logging.debug('frame %d: %.2f%% delta gain, %.2f%% delta luma',
                  j, delta_gain_rel, delta_luma_rel)
    if delta_gain * delta_luma < 0.0:
      return (f"{fmt['format']}: frame {j}: gain {total_gains[j-1]:.1f} "
              f'-> {total_gains[j]:.1f} ({delta_gain_rel:.1f}%), '
              f'luma {lumas[j-1]} -> {lumas[j]} ({delta_luma_rel:.2f}%) '
              f'GAIN/LUMA OPPOSITE DIR')
  elif abs(delta_gain_rel) < _DELTA_NO_GAIN_THRESH:
    logging.debug('frame %d: <|%.1f%%| delta gain, %.2f%% delta luma', j,
                  _DELTA_NO_GAIN_THRESH, delta_luma_rel)
    if abs(delta_luma_rel) > _DELTA_LUMA_THRESH:
      return (f"{fmt['format']}: frame {j}: gain {total_gains[j-1]:.1f} "
              f'-> {total_gains[j]:.1f} ({delta_gain_rel:.1f}%), '
              f'luma {lumas[j-1]} -> {lumas[j]} ({delta_luma_rel:.2f}%), '
              f'<|{_DELTA_NO_GAIN_THRESH:.1f}%| GAIN, '
              f'>|{_DELTA_LUMA_THRESH:.1f}%| LUMA DELTA')
  else:
    logging.debug('frame %d: %.1f%% delta gain, %.2f%% delta luma',
                  j, delta_gain_rel, delta_luma_rel)
    return None


def _determine_test_formats(cam, props, raw_avlb, debug):
  """Determines the capture formats to test.

  Args:
    cam: Camera capture object.
    props: Camera properties dict.
    raw_avlb: Boolean for if RAW captures are available.
    debug: Boolean for whether in debug mode.
  Returns:
    fmts: List of formats.
  """
  largest_yuv = capture_request_utils.get_largest_yuv_format(props)
  match_ar = (largest_yuv['width'], largest_yuv['height'])
  fmt = capture_request_utils.get_smallest_yuv_format(
      props, match_ar=match_ar)
  if raw_avlb and debug:
    return (cam.CAP_RAW, fmt)
  else:
    return (fmt,)


def _tabulate_frame_data(metadata, luma, raw_cap, debug):
  """Puts relevant frame data into a dictionary."""
  ae_state = metadata['android.control.aeState']
  iso = metadata['android.sensor.sensitivity']
  isp_gain = metadata['android.control.postRawSensitivityBoost'] / 100
  exp_time = metadata['android.sensor.exposureTime'] * _NS_TO_MS
  total_gain = iso * exp_time
  if not raw_cap:
    total_gain *= isp_gain
  awb_state = metadata['android.control.awbState']
  frame = {
      'awb_gains': metadata['android.colorCorrection.gains'],
      'ccm': metadata['android.colorCorrection.transform'],
      'fd': metadata['android.lens.focusDistance'],
  }

  # Convert CCM from rational to float, as numpy arrays.
  awb_ccm = np.array(capture_request_utils.rational_to_float(
      frame['ccm'])).reshape(3, 3)

  logging.debug('AE: %d ISO: %d ISP_sen: %d exp: %4fms tot_gain: %f luma: %f',
                ae_state, iso, isp_gain, exp_time, total_gain, luma)
  logging.debug('fd: %f', frame['fd'])
  logging.debug('AWB state: %d, AWB gains: %s\n AWB matrix: %s', awb_state,
                str(frame['awb_gains']), str(awb_ccm))
  if debug:
    logging.debug('Tonemap curve: %s', str(metadata['android.tonemap.curve']))

  return frame, ae_state, total_gain


def _compute_frame_luma(cap, props, raw_cap):
  """Determines the luma for the center patch of the frame.

  RAW captures use GR plane, YUV captures use Y plane.

  Args:
    cap: Camera capture object.
    props: Camera properties dict.
    raw_cap: Boolean for capture is RAW or YUV.
  Returns:
    luma: Luma value for center patch of image.
  """
  if raw_cap:
    plane = image_processing_utils.convert_capture_to_planes(
        cap, props=props)[_RAW_GR_CH]
  else:
    plane = image_processing_utils.convert_capture_to_planes(cap)[_YUV_Y_CH]

  patch = image_processing_utils.get_image_patch(
      plane, _PATCH_X, _PATCH_Y, _PATCH_W, _PATCH_H)
  return image_processing_utils.compute_image_means(patch)[0]


def _plot_data(lumas, gains, fmt, log_path):
  """Plots lumas and gains data for this test.

  Args:
    lumas: List of luma data from captures.
    gains: List of gain data from captures.
    fmt: String to identify 'YUV' or 'RAW' plots.
    log_path: Location to store data.
  """
  norm_gains = [x / max(gains) * max(lumas) for x in gains]

  pylab.figure(fmt)
  pylab.plot(range(len(lumas)), lumas, '-g.', label='Center patch brightness')
  pylab.plot(range(len(gains)), norm_gains, '-r.',
             label='Metadata AE setting product')
  pylab.title(_NAME + ' ' + fmt)
  pylab.xlabel('frame index')

  # expand y axis for low delta results
  ymin = min(norm_gains + lumas)
  ymax = max(norm_gains + lumas)
  yavg = (ymax + ymin) / 2.0
  if ymax - ymin < 3 * _DELTA_LUMA_THRESH/100:
    ymin = round(yavg - 1.5 * _DELTA_LUMA_THRESH/100, 3)
    ymax = round(yavg + 1.5 * _DELTA_LUMA_THRESH/100, 3)
    pylab.ylim(ymin, ymax)
  pylab.legend()
  matplotlib.pyplot.savefig(
      '%s_plot_%s.png' % (os.path.join(log_path, _NAME), fmt))


def _is_awb_af_stable(cap_info, i):
  """Determines if Auto White Balance and Auto Focus are stable."""
  awb_gains_i_1 = cap_info[i-1]['awb_gains']
  awb_gains_i = cap_info[i]['awb_gains']

  return (np.allclose(awb_gains_i_1, awb_gains_i, rtol=0.01) and
          cap_info[i-1]['ccm'] == cap_info[i]['ccm'] and
          np.isclose(cap_info[i-1]['fd'], cap_info[i]['fd'], rtol=0.01))


class AutoPerFrameControlTest(its_base_test.ItsBaseTest):
  """Tests PER_FRAME_CONTROL properties for auto capture requests.

  Takes a sequence of images with auto capture request.
  Determines if luma and gain settings move in same direction for large setting
  changes.
  Small settings changes should result in small changes in luma.
  Threshold for checking is DELTA_GAIN_THRESH. Theshold where not change is
  expected is DELTA_NO_GAIN_THRESH.

  While not included in this test, if camera debug is required:
    MANUAL_POSTPROCESSING capability is implied since
    camera_properties_utils.read_3a is valid for test.

    debug can also be performed with a defined tonemap curve:
      req['android.tonemap.mode'] = 0
      gamma = sum([[i/63.0,math.pow(i/63.0,1/2.2)] for i in xrange(64)],[])
      req['android.tonemap.curve'] = {'red': gamma, 'green': gamma,
                                      'blue': gamma}
  """

  def test_auto_per_frame_control(self):
    logging.debug('Starting %s', _NAME)
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      log_path = self.log_path

      # Check SKIP conditions.
      camera_properties_utils.skip_unless(
          camera_properties_utils.per_frame_control(props) and
          camera_properties_utils.read_3a(props))

      # Load chart for scene.
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      debug = self.debug_mode
      raw_avlb = camera_properties_utils.raw16(props)
      fmts = _determine_test_formats(cam, props, raw_avlb, debug)

      failed = []
      for i, fmt in enumerate(fmts):
        logging.debug('fmt: %s', str(fmt['format']))
        cam.do_3a()
        req = capture_request_utils.auto_capture_request()
        cap_info = {}
        ae_states = []
        lumas = []
        total_gains = []
        num_caps = _NUM_CAPS
        num_frames = _NUM_FRAMES
        raw_cap = i == 0 and raw_avlb and debug
        # Break up caps if RAW to reduce bandwidth requirements.
        if raw_cap:
          num_caps = _NUM_CAPS * _RAW_NIBBLE_SIZE
          num_frames = _NUM_FRAMES // _RAW_NIBBLE_SIZE

        # Capture frames and tabulate info.
        for j in range(num_caps):
          caps = cam.do_capture([req] * num_frames, fmt)
          for k, cap in enumerate(caps):
            idx = k + j * num_frames
            logging.debug('=========== frame %d ==========', idx)
            luma = _compute_frame_luma(cap, props, raw_cap)
            frame, ae_state, total_gain = _tabulate_frame_data(
                cap['metadata'], luma, raw_cap, debug)
            cap_info[idx] = frame
            ae_states.append(ae_state)
            lumas.append(luma)
            total_gains.append(total_gain)

             # Save image.
            img = image_processing_utils.convert_capture_to_rgb_image(
                cap, props=props)
            image_processing_utils.write_image(img, '%s_frame_%s_%d.jpg' % (
                os.path.join(log_path, _NAME), fmt['format'], idx))

        _plot_data(lumas, total_gains, fmt['format'], log_path)

        # Check correct behavior
        logging.debug('fmt: %s', str(fmt['format']))
        for j in range(1, num_caps * num_frames):
          if _is_awb_af_stable(cap_info, j):
            error_msg = _check_delta_luma_vs_delta_gain(
                fmt, j, lumas, total_gains)
            if error_msg:
              failed.append(error_msg)
          else:
            logging.debug('frame %d -> %d: AWB/AF changed', j-1, j)

        for j, luma in enumerate(lumas):
          if ((ae_states[j] == _AE_STATE_CONVERGED or
               ae_states[j] == _AE_STATE_FLASH_REQUIRED) and
              (_VALID_LUMA_MIN > luma or luma > _VALID_LUMA_MAX)):
            failed.append(
                f"{fmt['format']}: frame {j} AE converged luma {luma}. "
                f'Valid range: ({_VALID_LUMA_MIN}, {_VALID_LUMA_MAX})'
            )
      if failed:
        logging.error('Error summary')
        for fail in failed:
          logging.error('%s', fail)
        raise AssertionError

if __name__ == '__main__':
  test_runner.main()
