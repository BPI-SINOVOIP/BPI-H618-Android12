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
"""Verifies faces are detected and landmarks in bounding boxes."""


import logging
import os.path
from mobly import test_runner

import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils

NAME = os.path.splitext(os.path.basename(__file__))[0]
NUM_TEST_FRAMES = 20
FD_MODE_OFF = 0
FD_MODE_SIMPLE = 1
FD_MODE_FULL = 2
W, H = 640, 480


def check_face_bounding_box(rect, aa_w, aa_h):
  """Check that face bounding box is within the active array area."""
  rect_t = rect['top']
  rect_b = rect['bottom']
  rect_l = rect['left']
  rect_r = rect['right']
  if rect_t > rect_b:
    raise AssertionError(f'Face top > bottom! t: {rect_t}, b: {rect_b}')
  if rect_l > rect_r:
    raise AssertionError(f'Face left > right! l: {rect_l}, r: {rect_r}')

  if not 0 <= rect_l <= aa_w:
    raise AssertionError(f'Face l: {rect_l} outside of active W: 0,{aa_w}')
  if not 0 <= rect_r <= aa_w:
    raise AssertionError(f'Face r: {rect_r} outside of active W: 0,{aa_w}')
  if not 0 <= rect_t <= aa_h:
    raise AssertionError(f'Face t: {rect_t} outside active H: 0,{aa_h}')
  if not 0 <= rect_b <= aa_h:
    raise AssertionError(f'Face b: {rect_b} outside active H: 0,{aa_h}')


def check_face_landmarks(face):
  """Check that face landmarks fall within face bounding box."""
  l, r = face['bounds']['left'], face['bounds']['right']
  t, b = face['bounds']['top'], face['bounds']['bottom']
  l_eye_x, l_eye_y = face['leftEye']['x'], face['leftEye']['y']
  r_eye_x, r_eye_y = face['rightEye']['x'], face['rightEye']['y']
  mouth_x, mouth_y = face['mouth']['x'], face['mouth']['y']
  if not l <= l_eye_x <= r:
    raise AssertionError(f'Face l: {l}, r: {r}, left eye x: {l_eye_x}')
  if not t <= l_eye_y <= b:
    raise AssertionError(f'Face t: {t}, b: {b}, left eye y: {l_eye_y}')
  if not l <= r_eye_x <= r:
    raise AssertionError(f'Face l: {l}, r: {r}, right eye x: {r_eye_x}')
  if not t <= r_eye_y <= b:
    raise AssertionError(f'Face t: {t}, b: {b}, right eye y: {r_eye_y}')
  if not l <= mouth_x <= r:
    raise AssertionError(f'Face l: {l}, r: {r}, mouth x: {mouth_x}')
  if not t <= mouth_y <= b:
    raise AssertionError(f'Face t: {t}, b: {b}, mouth y: {mouth_y}')


class FacesTest(its_base_test.ItsBaseTest):
  """Tests face detection algorithms.

  Allows NUM_TEST_FRAMES for face detection algorithm to find all faces.
  Tests OFF, SIMPLE, and FULL modes if available.
    OFF --> no faces should be found.
    SIMPLE --> face(s) should be found, but no landmarks.
    FULL --> face(s) should be found and face landmarks reported.
  """

  def test_faces(self):
    logging.debug('Starting %s', NAME)
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)

      # Load chart for scene.
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      camera_properties_utils.skip_unless(
          camera_properties_utils.face_detect(props))
      mono_camera = camera_properties_utils.mono_camera(props)
      fd_modes = props['android.statistics.info.availableFaceDetectModes']
      a = props['android.sensor.info.activeArraySize']
      aw, ah = a['right'] - a['left'], a['bottom'] - a['top']
      if camera_properties_utils.read_3a(props):
        gain, exp, _, _, focus = cam.do_3a(
            get_results=True, mono_camera=mono_camera)
        logging.debug('iso = %d', gain)
        logging.debug('exp = %.2fms', (exp * 1.0E-6))
        if focus == 0.0:
          logging.debug('fd = infinity')
        else:
          logging.debug('fd = %.2fcm', (1.0E2 / focus))
      for fd_mode in fd_modes:
        if not FD_MODE_OFF <= fd_mode <= FD_MODE_FULL:
          raise AssertionError(f'fd_mode undefined: {fd_mode}')
        req = capture_request_utils.auto_capture_request()
        req['android.statistics.faceDetectMode'] = fd_mode
        fmt = {'format': 'yuv', 'width': W, 'height': H}
        caps = cam.do_capture([req] * NUM_TEST_FRAMES, fmt)
        for i, cap in enumerate(caps):
          fd_mode_md = cap['metadata']['android.statistics.faceDetectMode']
          if fd_mode_md != fd_mode:
            raise AssertionError('Metadata does not match request! '
                                 f'Request: {fd_mode} metadata: {fd_mode_md}.')
          faces = cap['metadata']['android.statistics.faces']

          # 0 faces should be returned for OFF mode
          if fd_mode == FD_MODE_OFF:
            if faces:
              raise AssertionError('Faces found in OFF mode.')
            continue
          # Save last frame.
          if i == NUM_TEST_FRAMES - 1:
            img = image_processing_utils.convert_capture_to_rgb_image(
                cap, props=props)
            img = image_processing_utils.rotate_img_per_argv(img)
            img_name = '%s_fd_mode_%s.jpg' % (os.path.join(self.log_path,
                                                           NAME), fd_mode)
            image_processing_utils.write_image(img, img_name)
            if not faces:
              raise AssertionError(f'No face detected in mode {fd_mode}.')
          if not faces:
            continue

          logging.debug('Frame %d face metadata:', i)
          logging.debug('Faces: %s', faces)

          face_scores = [face['score'] for face in faces]
          face_rectangles = [face['bounds'] for face in faces]
          for score in face_scores:
            if not 1 <= score <= 100:
              raise AssertionError(f'Face score not valid! score: {score}.')
          # Face bounds should be within active array.
          for j, rect in enumerate(face_rectangles):
            logging.debug('Checking face rectangle %d', j)
            check_face_bounding_box(rect, aw, ah)

          # Face ID should be -1 for SIMPLE and unique for FULL
          if fd_mode == FD_MODE_SIMPLE:
            for face in faces:
              if 'leftEye' in face or 'rightEye' in face:
                raise AssertionError('Eyes not supported in FD_MODE_SIMPLE.')
              if 'mouth' in face:
                raise AssertionError('Mouth not supported in FD_MODE_SIMPLE.')
              if face['id'] != -1:
                raise AssertionError('face_id should be -1 in FD_MODE_SIMPLE.')
          elif fd_mode == FD_MODE_FULL:
            face_ids = [face['id'] for face in faces]
            if len(face_ids) != len(set(face_ids)):
              raise AssertionError('Same face detected more than 1x.')
            # Face landmarks should be within face bounds
            for k, face in enumerate(faces):
              logging.debug('Checking landmarks in face %d: %s', k, str(face))
              check_face_landmarks(face)

if __name__ == '__main__':
  test_runner.main()
