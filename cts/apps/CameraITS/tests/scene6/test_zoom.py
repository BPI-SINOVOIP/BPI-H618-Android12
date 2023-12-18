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
"""Verify zoom ratio scales circle sizes correctly."""


import logging
import math
import os.path
from mobly import test_runner
import numpy as np

import cv2
import its_base_test
import camera_properties_utils
import capture_request_utils
import image_processing_utils
import its_session_utils
import opencv_processing_utils

CIRCLE_COLOR = 0  # [0: black, 255: white]
CIRCLE_TOL = 0.05  # contour area vs ideal circle area pi*((w+h)/4)**2
LINE_COLOR = (255, 0, 0)  # red
LINE_THICKNESS = 5
MIN_AREA_RATIO = 0.00015  # based on 2000/(4000x3000) pixels
MIN_CIRCLE_PTS = 25
MIN_FOCUS_DIST_TOL = 0.80  # allow charts a little closer than min
NAME = os.path.splitext(os.path.basename(__file__))[0]
NUM_STEPS = 10
OFFSET_RTOL = 0.15
RADIUS_RTOL = 0.10
RADIUS_RTOL_MIN_FD = 0.15
ZOOM_MAX_THRESH = 10.0
ZOOM_MIN_THRESH = 2.0


def get_test_tols_and_cap_size(cam, props, chart_distance, debug):
  """Determine the tolerance per camera based on test rig and camera params.

  Cameras are pre-filtered to only include supportable cameras.
  Supportable cameras are: YUV(RGB)

  Args:
    cam: camera object
    props: dict; physical camera properties dictionary
    chart_distance: float; distance to chart in cm
    debug: boolean; log additional data

  Returns:
    dict of TOLs with camera focal length as key
    largest common size across all cameras
  """
  ids = camera_properties_utils.logical_multi_camera_physical_ids(props)
  physical_props = {}
  physical_ids = []
  for i in ids:
    physical_props[i] = cam.get_camera_properties_by_id(i)
    # find YUV capable physical cameras
    if camera_properties_utils.backward_compatible(physical_props[i]):
      physical_ids.append(i)

  # find physical camera focal lengths that work well with rig
  chart_distance_m = abs(chart_distance)/100  # convert CM to M
  test_tols = {}
  test_yuv_sizes = []
  for i in physical_ids:
    min_fd = physical_props[i]['android.lens.info.minimumFocusDistance']
    focal_l = physical_props[i]['android.lens.info.availableFocalLengths'][0]
    logging.debug('cam[%s] min_fd: %.3f (diopters), fl: %.2f',
                  i, min_fd, focal_l)
    yuv_sizes = capture_request_utils.get_available_output_sizes(
        'yuv', physical_props[i])
    test_yuv_sizes.append(yuv_sizes)
    if debug:
      logging.debug('cam[%s] yuv sizes: %s', i, str(yuv_sizes))

    # determine if minimum focus distance is less than rig depth
    if (math.isclose(min_fd, 0.0, rel_tol=1E-6) or  # fixed focus
        1.0/min_fd < chart_distance_m*MIN_FOCUS_DIST_TOL):
      test_tols[focal_l] = RADIUS_RTOL
    else:
      test_tols[focal_l] = RADIUS_RTOL_MIN_FD
      logging.debug('loosening RTOL for cam[%s]: '
                    'min focus distance too large.', i)
  # find intersection of formats for max common format
  common_sizes = list(set.intersection(*[set(list) for list in test_yuv_sizes]))
  if debug:
    logging.debug('common_fmt: %s', max(common_sizes))

  return test_tols, max(common_sizes)


def distance(x, y):
  return math.sqrt(x**2 + y**2)


def circle_cropped(circle, size):
  """Determine if a circle is cropped by edge of img.

  Args:
    circle:  list [x, y, radius] of circle
    size:    tuple (x, y) of size of img

  Returns:
    Boolean True if selected circle is cropped
  """

  cropped = False
  circle_x, circle_y = circle[0], circle[1]
  circle_r = circle[2]
  x_min, x_max = circle_x - circle_r, circle_x + circle_r
  y_min, y_max = circle_y - circle_r, circle_y + circle_r
  if x_min < 0 or y_min < 0 or x_max > size[0] or y_max > size[1]:
    cropped = True
  return cropped


def find_center_circle(img, img_name, color, min_area, debug):
  """Find the circle closest to the center of the image.

  Finds all contours in the image. Rejects those too small and not enough
  points to qualify as a circle. The remaining contours must have center
  point of color=color and are sorted based on distance from the center
  of the image. The contour closest to the center of the image is returned.

  Note: hierarchy is not used as the hierarchy for black circles changes
  as the zoom level changes.

  Args:
    img:       numpy img array with pixel values in [0,255].
    img_name:  str file name for saved image
    color:     int 0 --> black, 255 --> white
    min_area:  int minimum area of circles to screen out
    debug:     bool to save extra data

  Returns:
    circle:    [center_x, center_y, radius]
  """

  # gray scale & otsu threshold to binarize the image
  gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
  _, img_bw = cv2.threshold(
      np.uint8(gray), 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)

  # use OpenCV to find contours (connected components)
  _, contours, _ = cv2.findContours(255 - img_bw, cv2.RETR_TREE,
                                    cv2.CHAIN_APPROX_SIMPLE)

  # check contours and find the best circle candidates
  circles = []
  img_ctr = [gray.shape[1] // 2, gray.shape[0] // 2]
  for contour in contours:
    area = cv2.contourArea(contour)
    if area > min_area and len(contour) >= MIN_CIRCLE_PTS:
      shape = opencv_processing_utils.component_shape(contour)
      radius = (shape['width'] + shape['height']) / 4
      colour = img_bw[shape['cty']][shape['ctx']]
      circlish = round((math.pi * radius**2) / area, 4)
      if colour == color and (1 - CIRCLE_TOL <= circlish <= 1 + CIRCLE_TOL):
        circles.append([shape['ctx'], shape['cty'], radius, circlish, area])

  if not circles:
    raise AssertionError('No circle was detected. Please take pictures '
                         'according to instructions carefully!')

  if debug:
    logging.debug('circles [x, y, r, pi*r**2/area, area]: %s', str(circles))

  # find circle closest to center
  circles.sort(key=lambda x: distance(x[0] - img_ctr[0], x[1] - img_ctr[1]))
  circle = circles[0]

  # mark image center
  size = gray.shape
  m_x, m_y = size[1] // 2, size[0] // 2
  marker_size = LINE_THICKNESS * 10
  cv2.drawMarker(img, (m_x, m_y), LINE_COLOR, markerType=cv2.MARKER_CROSS,
                 markerSize=marker_size, thickness=LINE_THICKNESS)

  # add circle to saved image
  center_i = (int(round(circle[0], 0)), int(round(circle[1], 0)))
  radius_i = int(round(circle[2], 0))
  cv2.circle(img, center_i, radius_i, LINE_COLOR, LINE_THICKNESS)
  image_processing_utils.write_image(img / 255.0, img_name)

  return [circle[0], circle[1], circle[2]]


class ZoomTest(its_base_test.ItsBaseTest):
  """Test the camera zoom behavior.
  """

  def test_zoom(self):
    test_data = {}
    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      props = cam.override_with_hidden_physical_camera_props(props)
      camera_properties_utils.skip_unless(
          camera_properties_utils.zoom_ratio_range(props))

      # Load chart for scene
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      z_range = props['android.control.zoomRatioRange']
      logging.debug('testing zoomRatioRange: %s', str(z_range))
      debug = self.debug_mode

      z_min, z_max = float(z_range[0]), float(z_range[1])
      camera_properties_utils.skip_unless(z_max >= z_min * ZOOM_MIN_THRESH)
      z_list = np.arange(z_min, z_max, float(z_max - z_min) / (NUM_STEPS - 1))
      z_list = np.append(z_list, z_max)

      # set TOLs based on camera and test rig params
      if camera_properties_utils.logical_multi_camera(props):
        test_tols, size = get_test_tols_and_cap_size(
            cam, props, self.chart_distance, debug)
      else:
        fl = props['android.lens.info.availableFocalLengths'][0]
        test_tols = {fl: RADIUS_RTOL}
        yuv_size = capture_request_utils.get_largest_yuv_format(props)
        size = [yuv_size['width'], yuv_size['height']]
      logging.debug('capture size: %s', str(size))
      logging.debug('test TOLs: %s', str(test_tols))

      # do captures over zoom range and find circles with cv2
      logging.debug('cv2_version: %s', cv2.__version__)
      cam.do_3a()
      req = capture_request_utils.auto_capture_request()
      for i, z in enumerate(z_list):
        logging.debug('zoom ratio: %.2f', z)
        req['android.control.zoomRatio'] = z
        cap = cam.do_capture(
            req, {'format': 'yuv', 'width': size[0], 'height': size[1]})
        img = image_processing_utils.convert_capture_to_rgb_image(
            cap, props=props)
        img_name = '%s_%s.jpg' % (os.path.join(self.log_path,
                                               NAME), round(z, 2))
        image_processing_utils.write_image(img, img_name)

        # determine radius tolerance of capture
        cap_fl = cap['metadata']['android.lens.focalLength']
        radius_tol = test_tols[cap_fl]

        # convert to [0, 255] images with unsigned integer
        img *= 255
        img = img.astype(np.uint8)

        # Find the center circle in img
        circle = find_center_circle(
            img, img_name, CIRCLE_COLOR,
            min_area=MIN_AREA_RATIO * size[0] * size[1] * z * z,
            debug=debug)
        if circle_cropped(circle, size):
          logging.debug('zoom %.2f is too large! Skip further captures', z)
          break
        test_data[i] = {'z': z, 'circle': circle, 'r_tol': radius_tol,
                        'fl': cap_fl}

    # assert some range is tested before circles get too big
    zoom_max_thresh = ZOOM_MAX_THRESH
    if z_max < ZOOM_MAX_THRESH:
      zoom_max_thresh = z_max
    test_data_max_z = test_data[max(test_data.keys())]['z']
    logging.debug('zoom data max: %.2f', test_data_max_z)
    if test_data_max_z < zoom_max_thresh:
      raise AssertionError(f'Max zoom ratio tested: {test_data_max_z:.4f}, '
                           f'range advertised min: {z_min}, max: {z_max} '
                           f'THRESH: {zoom_max_thresh}')

    # initialize relative size w/ zoom[0] for diff zoom ratio checks
    radius_0 = float(test_data[0]['circle'][2])
    z_0 = float(test_data[0]['z'])

    for i, data in test_data.items():
      logging.debug('Zoom: %.2f, fl: %.2f', data['z'], data['fl'])
      offset_abs = [(data['circle'][0] - size[0] // 2),
                    (data['circle'][1] - size[1] // 2)]
      logging.debug('Circle r: %.1f, center offset x, y: %d, %d',
                    data['circle'][2], offset_abs[0], offset_abs[1])
      z_ratio = data['z'] / z_0

      # check relative size against zoom[0]
      radius_ratio = data['circle'][2] / radius_0
      logging.debug('r ratio req: %.3f, measured: %.3f', z_ratio, radius_ratio)
      if not math.isclose(z_ratio, radius_ratio, rel_tol=data['r_tol']):
        raise AssertionError(f'zoom: {z_ratio:.2f}, radius ratio: '
                             f"{radius_ratio:.2f}, RTOL: {data['r_tol']}")

      # check relative offset against init vals w/ no focal length change
      if i == 0 or test_data[i-1]['fl'] != data['fl']:  # set init values
        z_init = float(data['z'])
        offset_init = [data['circle'][0] - size[0]//2,
                       data['circle'][1] - size[1]//2]
      else:  # check
        z_ratio = data['z'] / z_init
        offset_rel = (distance(offset_abs[0], offset_abs[1]) / z_ratio /
                      distance(offset_init[0], offset_init[1]))
        logging.debug('offset_rel: %.3f', offset_rel)
        if not math.isclose(offset_rel, 1.0, rel_tol=OFFSET_RTOL):
          raise AssertionError(f"zoom: {data['z']:.2f}, offset(rel): "
                               f'{offset_rel:.4f}, RTOL: {OFFSET_RTOL}')

if __name__ == '__main__':
  test_runner.main()
