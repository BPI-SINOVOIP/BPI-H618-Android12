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
"""Utility functions for scene change test."""


import logging
import unittest

_DARK_SCENE_THRESH = 0.2
_FPS = 30  # Frames Per Second
_FRAME_SHIFT_SMALL = 5  # Num of frames to shift if scene or brightness change.
_FRAME_SHIFT_LARGE = 30  # Num of frames to shift if no change in capture.
SCENE_CHANGE_FAIL_CODE = -1001
SCENE_CHANGE_PASS_CODE = 1001


def calc_timing_adjustment(converged, scene_change_flag,
                           bright_change_flag, bright_final):
  """Calculate timing adjustment based on converged frame and flags.

  Args:
    converged: Boolean on whether 3A converged or not.
    scene_change_flag: Boolean for if afSceneChanged triggered.
    bright_change_flag: Boolean for if image brightness changes.
    bright_final: Float for average value of center patch of final frame.
  Returns:
    scene_change_timing_shift: Timing shift in frames.

  Does timing adjustment based on input values from captured frames.
    Truth table for 3A frame, Change flag, Bright flag, Last frame brightness
      3, C, B, L
      1, 1, 1, X --> PASS: 3A settled, scene and brightness change
      1, 1, 0, X --> FAIL: 3A settled, scene change, but no brightness change
      1, 0, 1, X --> shift FRAME_SHIFT_SMALL earlier
      1, 0, 0, 1 --> shift FRAME_SHIFT_LARGE earlier
      1, 0, 0, 0 --> shift FRAME_SHIFT_LARGE later
      0, X, 1, X --> shift FRAME_SHIFT_SMALL later
      0, X, 0, X --> FAIL: Check results of scene2/test_continuous_picture.
    Note: these values have been found empirically for 4 different phone
          models and 8 cameras. It is possible they may need to be tweaked as
          more phone models become available.
  """
  if converged:  # 3A converges
    if scene_change_flag:
      if bright_change_flag:  # scene_change_flag & brightness change --> PASS
        logging.debug('Scene & brightness change: PASS.')
        return SCENE_CHANGE_PASS_CODE
      else:  # scene_change_flag & no brightness change --> FAIL
        scene_change_frame_shift = SCENE_CHANGE_FAIL_CODE
        logging.error('Scene change, but no brightness change.')
    else:  # No scene change flag: shift timing
      if bright_change_flag:
        scene_change_frame_shift = -1 * _FRAME_SHIFT_SMALL
        logging.debug('No scene change flag, but brightness change.')
      else:
        logging.debug('No scene change flag, no brightness change.')
        if bright_final < _DARK_SCENE_THRESH:
          scene_change_frame_shift = _FRAME_SHIFT_LARGE
          logging.debug('Scene dark entire capture.')
        else:
          scene_change_frame_shift = -1 * _FRAME_SHIFT_LARGE
          logging.debug('Scene light entire capture.')
  else:  # 3A does not converge.
    if bright_change_flag:
      scene_change_frame_shift = _FRAME_SHIFT_SMALL
      logging.debug('3A does not converge, but brightness changes.')
    else:
      scene_change_frame_shift = SCENE_CHANGE_FAIL_CODE
      logging.error('3A does not converge, and brightness does not change.')
  if scene_change_frame_shift >= 0:
    logging.debug('Shift +%d frames.', scene_change_frame_shift)
  else:
    logging.debug('Shift %d frames.', scene_change_frame_shift)
  return scene_change_frame_shift


class ItsSessionUtilsTests(unittest.TestCase):
  """Unit tests for this module."""

  def test_calc_timing_adjustment_shift(self):
    results = {}
    expected_results = {'1111': SCENE_CHANGE_PASS_CODE,
                        '1110': SCENE_CHANGE_PASS_CODE,
                        '1101': SCENE_CHANGE_FAIL_CODE,
                        '1100': SCENE_CHANGE_FAIL_CODE,
                        '1011': -1*_FRAME_SHIFT_SMALL,
                        '1010': -1*_FRAME_SHIFT_SMALL,
                        '1001': -1*_FRAME_SHIFT_LARGE,
                        '1000': _FRAME_SHIFT_LARGE,
                        '0111': _FRAME_SHIFT_SMALL,
                        '0110': _FRAME_SHIFT_SMALL,
                        '0101': SCENE_CHANGE_FAIL_CODE,
                        '0100': SCENE_CHANGE_FAIL_CODE,
                        '0011': _FRAME_SHIFT_SMALL,
                        '0010': _FRAME_SHIFT_SMALL,
                        '0001': SCENE_CHANGE_FAIL_CODE,
                        '0000': SCENE_CHANGE_FAIL_CODE,
                        }
    converged_list = [1, 0]
    scene_change_flag_list = [1, 0]
    bright_change_flag_list = [1, 0]
    bright_final_list = [1, 0]
    for converged in converged_list:
      for scene_flag in scene_change_flag_list:
        for bright_flag in bright_change_flag_list:
          for bright_final in bright_final_list:
            key = f'{converged}{scene_flag}{bright_flag}{bright_final}'
            results[key] = calc_timing_adjustment(converged, scene_flag,
                                                  bright_flag, bright_final)
    self.assertEqual(results, expected_results)


if __name__ == '__main__':
  unittest.main()
