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
"""Image processing utilities using openCV."""


import logging
import math
import os
import unittest

import numpy


import cv2
import camera_properties_utils
import capture_request_utils
import image_processing_utils

ANGLE_CHECK_TOL = 1  # degrees
ANGLE_NUM_MIN = 10  # Minimum number of angles for find_angle() to be valid


TEST_IMG_DIR = os.path.join(os.environ['CAMERA_ITS_TOP'], 'test_images')
CHART_FILE = os.path.join(TEST_IMG_DIR, 'ISO12233.png')
CHART_HEIGHT = 13.5  # cm
CHART_DISTANCE_RFOV = 31.0  # cm
CHART_DISTANCE_WFOV = 22.0  # cm
CHART_SCALE_START = 0.65
CHART_SCALE_STOP = 1.35
CHART_SCALE_STEP = 0.025

CIRCLE_AR_ATOL = 0.1  # circle aspect ratio tolerance
CIRCLISH_ATOL = 0.10  # contour area vs ideal circle area & aspect ratio TOL
CIRCLISH_LOW_RES_ATOL = 0.15  # loosen for low res images
CIRCLE_MIN_PTS = 20
CIRCLE_RADIUS_NUMPTS_THRESH = 2  # contour num_pts/radius: empirically ~3x

CV2_RED = (255, 0, 0)  # color in cv2 to draw lines

FOV_THRESH_TELE25 = 25
FOV_THRESH_TELE40 = 40
FOV_THRESH_TELE = 60
FOV_THRESH_WFOV = 90

LOW_RES_IMG_THRESH = 320 * 240

RGB_GRAY_WEIGHTS = (0.299, 0.587, 0.114)  # RGB to Gray conversion matrix

SCALE_RFOV_IN_WFOV_BOX = 0.67
SCALE_TELE_IN_WFOV_BOX = 0.5
SCALE_TELE_IN_RFOV_BOX = 0.67
SCALE_TELE40_IN_RFOV_BOX = 0.5
SCALE_TELE25_IN_RFOV_BOX = 0.33

SQUARE_AREA_MIN_REL = 0.05  # Minimum size for square relative to image area
SQUARE_TOL = 0.1  # Square W vs H mismatch RTOL

VGA_HEIGHT = 480
VGA_WIDTH = 640


def find_all_contours(img):
  cv2_version = cv2.__version__
  if cv2_version.startswith('3.'):  # OpenCV 3.x
    _, contours, _ = cv2.findContours(img, cv2.RETR_TREE,
                                      cv2.CHAIN_APPROX_SIMPLE)
  else:  # OpenCV 2.x and 4.x
    contours, _ = cv2.findContours(img, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
  return contours


def calc_chart_scaling(chart_distance, camera_fov):
  """Returns charts scaling factor.

  Args:
   chart_distance: float; distance in cm from camera of displayed chart
   camera_fov: float; camera field of view.

  Returns:
   chart_scaling: float; scaling factor for chart
  """
  chart_scaling = 1.0
  camera_fov = float(camera_fov)
  if (FOV_THRESH_TELE < camera_fov < FOV_THRESH_WFOV and
      numpy.isclose(chart_distance, CHART_DISTANCE_WFOV, rtol=0.1)):
    chart_scaling = SCALE_RFOV_IN_WFOV_BOX
  elif (camera_fov <= FOV_THRESH_TELE and
        numpy.isclose(chart_distance, CHART_DISTANCE_WFOV, rtol=0.1)):
    chart_scaling = SCALE_TELE_IN_WFOV_BOX
  elif (camera_fov <= FOV_THRESH_TELE25 and
        numpy.isclose(chart_distance, CHART_DISTANCE_RFOV, rtol=0.1)):
    chart_scaling = SCALE_TELE25_IN_RFOV_BOX
  elif (camera_fov <= FOV_THRESH_TELE40 and
        numpy.isclose(chart_distance, CHART_DISTANCE_RFOV, rtol=0.1)):
    chart_scaling = SCALE_TELE40_IN_RFOV_BOX
  elif (camera_fov <= FOV_THRESH_TELE and
        numpy.isclose(chart_distance, CHART_DISTANCE_RFOV, rtol=0.1)):
    chart_scaling = SCALE_TELE_IN_RFOV_BOX
  return chart_scaling


def scale_img(img, scale=1.0):
  """Scale image based on a real number scale factor."""
  dim = (int(img.shape[1] * scale), int(img.shape[0] * scale))
  return cv2.resize(img.copy(), dim, interpolation=cv2.INTER_AREA)


def gray_scale_img(img):
  """Return gray scale version of image."""
  if len(img.shape) == 2:
    img_gray = img.copy()
  elif len(img.shape) == 3:
    if img.shape[2] == 1:
      img_gray = img[:, :, 0].copy()
    else:
      img_gray = cv2.cvtColor(img, cv2.COLOR_RGB2GRAY)
  return img_gray


class Chart(object):
  """Definition for chart object.

  Defines PNG reference file, chart, size, distance and scaling range.
  """

  def __init__(
      self,
      cam,
      props,
      log_path,
      chart_loc=None,
      chart_file=None,
      height=None,
      distance=None,
      scale_start=None,
      scale_stop=None,
      scale_step=None):
    """Initial constructor for class.

    Args:
     cam: open ITS session
     props: camera properties object
     log_path: log path to store the captured images.
     chart_loc: chart locator arg.
     chart_file: str; absolute path to png file of chart
     height: float; height in cm of displayed chart
     distance: float; distance in cm from camera of displayed chart
     scale_start: float; start value for scaling for chart search
     scale_stop: float; stop value for scaling for chart search
     scale_step: float; step value for scaling for chart search
    """
    self._file = chart_file or CHART_FILE
    self._height = height or CHART_HEIGHT
    self._distance = distance or CHART_DISTANCE_RFOV
    self._scale_start = scale_start or CHART_SCALE_START
    self._scale_stop = scale_stop or CHART_SCALE_STOP
    self._scale_step = scale_step or CHART_SCALE_STEP
    self.xnorm, self.ynorm, self.wnorm, self.hnorm, self.scale = (
        image_processing_utils.chart_located_per_argv(chart_loc))
    if not self.xnorm:
      if camera_properties_utils.read_3a(props):
        self.locate(cam, props, log_path)
      else:
        logging.debug('Chart locator skipped.')
        self._set_scale_factors_to_one()

  def _set_scale_factors_to_one(self):
    """Set scale factors to 1.0 for skipped tests."""
    self.wnorm = 1.0
    self.hnorm = 1.0
    self.xnorm = 0.0
    self.ynorm = 0.0
    self.scale = 1.0

  def _calc_scale_factors(self, cam, props, fmt, s, e, fd, log_path):
    """Take an image with s, e, & fd to find the chart location.

    Args:
     cam: An open its session.
     props: Properties of cam
     fmt: Image format for the capture
     s: Sensitivity for the AF request as defined in
                            android.sensor.sensitivity
     e: Exposure time for the AF request as defined in
                            android.sensor.exposureTime
     fd: float; autofocus lens position
     log_path: log path to save the captured images.

    Returns:
      template: numpy array; chart template for locator
      img_3a: numpy array; RGB image for chart location
      scale_factor: float; scaling factor for chart search
    """
    req = capture_request_utils.manual_capture_request(s, e)
    req['android.lens.focusDistance'] = fd
    cap_chart = image_processing_utils.stationary_lens_cap(cam, req, fmt)
    img_3a = image_processing_utils.convert_capture_to_rgb_image(
        cap_chart, props)
    img_3a = image_processing_utils.rotate_img_per_argv(img_3a)
    af_scene_name = os.path.join(log_path, 'af_scene.jpg')
    image_processing_utils.write_image(img_3a, af_scene_name)
    template = cv2.imread(self._file, cv2.IMREAD_ANYDEPTH)
    focal_l = cap_chart['metadata']['android.lens.focalLength']
    pixel_pitch = (
        props['android.sensor.info.physicalSize']['height'] / img_3a.shape[0])
    logging.debug('Chart distance: %.2fcm', self._distance)
    logging.debug('Chart height: %.2fcm', self._height)
    logging.debug('Focal length: %.2fmm', focal_l)
    logging.debug('Pixel pitch: %.2fum', pixel_pitch * 1E3)
    logging.debug('Template height: %dpixels', template.shape[0])
    chart_pixel_h = self._height * focal_l / (self._distance * pixel_pitch)
    scale_factor = template.shape[0] / chart_pixel_h
    logging.debug('Chart/image scale factor = %.2f', scale_factor)
    return template, img_3a, scale_factor

  def locate(self, cam, props, log_path):
    """Find the chart in the image, and append location to chart object.

    Args:
      cam: Open its session.
      props: Camera properties object.
      log_path: log path to store the captured images.

    The values appended are:
    xnorm: float; [0, 1] left loc of chart in scene
    ynorm: float; [0, 1] top loc of chart in scene
    wnorm: float; [0, 1] width of chart in scene
    hnorm: float; [0, 1] height of chart in scene
    scale: float; scale factor to extract chart
    """
    if camera_properties_utils.read_3a(props):
      s, e, _, _, fd = cam.do_3a(get_results=True)
      fmt = {'format': 'yuv', 'width': VGA_WIDTH, 'height': VGA_HEIGHT}
      chart, scene, s_factor = self._calc_scale_factors(cam, props, fmt, s, e,
                                                        fd, log_path)
    else:
      logging.debug('Chart locator skipped.')
      self._set_scale_factors_to_one()
      return
    scale_start = self._scale_start * s_factor
    scale_stop = self._scale_stop * s_factor
    scale_step = self._scale_step * s_factor
    self.scale = s_factor
    max_match = []
    # check for normalized image
    if numpy.amax(scene) <= 1.0:
      scene = (scene * 255.0).astype(numpy.uint8)
    scene_gray = gray_scale_img(scene)
    logging.debug('Finding chart in scene...')
    for scale in numpy.arange(scale_start, scale_stop, scale_step):
      scene_scaled = scale_img(scene_gray, scale)
      if (scene_scaled.shape[0] < chart.shape[0] or
          scene_scaled.shape[1] < chart.shape[1]):
        continue
      result = cv2.matchTemplate(scene_scaled, chart, cv2.TM_CCOEFF)
      _, opt_val, _, top_left_scaled = cv2.minMaxLoc(result)
      logging.debug(' scale factor: %.3f, opt val: %.f', scale, opt_val)
      max_match.append((opt_val, top_left_scaled))

    # determine if optimization results are valid
    opt_values = [x[0] for x in max_match]
    if 2.0 * min(opt_values) > max(opt_values):
      estring = ('Warning: unable to find chart in scene!\n'
                 'Check camera distance and self-reported '
                 'pixel pitch, focal length and hyperfocal distance.')
      logging.warning(estring)
      self._set_scale_factors_to_one()
    else:
      if (max(opt_values) == opt_values[0] or
          max(opt_values) == opt_values[len(opt_values) - 1]):
        estring = ('Warning: Chart is at extreme range of locator.')
        logging.warning(estring)
      # find max and draw bbox
      match_index = max_match.index(max(max_match, key=lambda x: x[0]))
      self.scale = scale_start + scale_step * match_index
      logging.debug('Optimum scale factor: %.3f', self.scale)
      top_left_scaled = max_match[match_index][1]
      h, w = chart.shape
      bottom_right_scaled = (top_left_scaled[0] + w, top_left_scaled[1] + h)
      top_left = ((top_left_scaled[0] // self.scale),
                  (top_left_scaled[1] // self.scale))
      bottom_right = ((bottom_right_scaled[0] // self.scale),
                      (bottom_right_scaled[1] // self.scale))
      self.wnorm = ((bottom_right[0]) - top_left[0]) / scene.shape[1]
      self.hnorm = ((bottom_right[1]) - top_left[1]) / scene.shape[0]
      self.xnorm = (top_left[0]) / scene.shape[1]
      self.ynorm = (top_left[1]) / scene.shape[0]


def component_shape(contour):
  """Measure the shape of a connected component.

  Args:
    contour: return from cv2.findContours. A list of pixel coordinates of
    the contour.

  Returns:
    The most left, right, top, bottom pixel location, height, width, and
    the center pixel location of the contour.
  """
  shape = {'left': numpy.inf, 'right': 0, 'top': numpy.inf, 'bottom': 0,
           'width': 0, 'height': 0, 'ctx': 0, 'cty': 0}
  for pt in contour:
    if pt[0][0] < shape['left']:
      shape['left'] = pt[0][0]
    if pt[0][0] > shape['right']:
      shape['right'] = pt[0][0]
    if pt[0][1] < shape['top']:
      shape['top'] = pt[0][1]
    if pt[0][1] > shape['bottom']:
      shape['bottom'] = pt[0][1]
  shape['width'] = shape['right'] - shape['left'] + 1
  shape['height'] = shape['bottom'] - shape['top'] + 1
  shape['ctx'] = (shape['left'] + shape['right']) // 2
  shape['cty'] = (shape['top'] + shape['bottom']) // 2
  return shape


def find_circle(img, img_name, min_area, color):
  """Find the circle in the test image.

  Args:
    img: numpy image array in RGB, with pixel values in [0,255].
    img_name: string with image info of format and size.
    min_area: float of minimum area of circle to find
    color: int of [0 or 255] 0 is black, 255 is white

  Returns:
    circle = {'x', 'y', 'r', 'w', 'h', 'x_offset', 'y_offset'}
  """
  circle = {}
  img_size = img.shape
  if img_size[0]*img_size[1] >= LOW_RES_IMG_THRESH:
    circlish_atol = CIRCLISH_ATOL
  else:
    circlish_atol = CIRCLISH_LOW_RES_ATOL

  # convert to gray-scale image
  img_gray = numpy.dot(img[..., :3], RGB_GRAY_WEIGHTS)

  # otsu threshold to binarize the image
  _, img_bw = cv2.threshold(numpy.uint8(img_gray), 0, 255,
                            cv2.THRESH_BINARY + cv2.THRESH_OTSU)

  # find contours
  contours = find_all_contours(255-img_bw)

  # Check each contour and find the circle bigger than min_area
  num_circles = 0
  logging.debug('Initial number of contours: %d', len(contours))
  for contour in contours:
    area = cv2.contourArea(contour)
    num_pts = len(contour)
    if (area > img_size[0]*img_size[1]*min_area and
        num_pts >= CIRCLE_MIN_PTS):
      shape = component_shape(contour)
      radius = (shape['width'] + shape['height']) / 4
      colour = img_bw[shape['cty']][shape['ctx']]
      circlish = (math.pi * radius**2) / area
      aspect_ratio = shape['width'] / shape['height']
      logging.debug('Potential circle found. radius: %.2f, color: %d, '
                    'circlish: %.3f, ar: %.3f, pts: %d', radius, colour,
                    circlish, aspect_ratio, num_pts)
      if (colour == color and
          numpy.isclose(1.0, circlish, atol=circlish_atol) and
          numpy.isclose(1.0, aspect_ratio, atol=CIRCLE_AR_ATOL) and
          num_pts/radius >= CIRCLE_RADIUS_NUMPTS_THRESH):

        # Populate circle dictionary
        circle['x'] = shape['ctx']
        circle['y'] = shape['cty']
        circle['r'] = (shape['width'] + shape['height']) / 4
        circle['w'] = float(shape['width'])
        circle['h'] = float(shape['height'])
        circle['x_offset'] = (shape['ctx'] - img_size[1]//2) / circle['w']
        circle['y_offset'] = (shape['cty'] - img_size[0]//2) / circle['h']
        logging.debug('Num pts: %d', num_pts)
        logging.debug('Aspect ratio: %.3f', aspect_ratio)
        logging.debug('Circlish value: %.3f', circlish)
        logging.debug('Location: %.1f x %.1f', circle['x'], circle['y'])
        logging.debug('Radius: %.3f', circle['r'])
        logging.debug('Circle center position wrt to image center:%.3fx%.3f',
                      circle['x_offset'], circle['y_offset'])
        num_circles += 1
        # if more than one circle found, break
        if num_circles == 2:
          break

  if num_circles == 0:
    image_processing_utils.write_image(img/255, img_name, True)
    raise AssertionError('No black circle detected. '
                         'Please take pictures according to instructions.')

  if num_circles > 1:
    image_processing_utils.write_image(img/255, img_name, True)
    raise AssertionError('More than 1 black circle detected. '
                         'Background of scene may be too complex.')

  return circle


def append_circle_center_to_img(circle, img, img_name):
  """Append circle center and image center to image and save image.

  Draws line from circle center to image center and then labels end-points.
  Adjusts text positioning depending on circle center wrt image center.
  Moves text position left/right half of up/down movement for visual aesthetics.

  Args:
    circle: dict with circle location vals.
    img: numpy float image array in RGB, with pixel values in [0,255].
    img_name: string with image info of format and size.
  """
  line_width_scaling_factor = 500
  text_move_scaling_factor = 3
  img_size = img.shape
  img_center_x = img_size[1]//2
  img_center_y = img_size[0]//2

  # draw line from circle to image center
  line_width = int(max(1, max(img_size)//line_width_scaling_factor))
  font_size = line_width // 2
  move_text_dist = line_width * text_move_scaling_factor
  cv2.line(img, (circle['x'], circle['y']), (img_center_x, img_center_y),
           CV2_RED, line_width)

  # adjust text location
  move_text_right_circle = -1
  move_text_right_image = 2
  if circle['x'] > img_center_x:
    move_text_right_circle = 2
    move_text_right_image = -1

  move_text_down_circle = -1
  move_text_down_image = 4
  if circle['y'] > img_center_y:
    move_text_down_circle = 4
    move_text_down_image = -1

  # add circles to end points and label
  radius_pt = line_width * 2  # makes a dot 2x line width
  filled_pt = -1  # cv2 value for a filled circle
  # circle center
  cv2.circle(img, (circle['x'], circle['y']), radius_pt, CV2_RED, filled_pt)
  text_circle_x = move_text_dist * move_text_right_circle + circle['x']
  text_circle_y = move_text_dist * move_text_down_circle + circle['y']
  cv2.putText(img, 'circle center', (text_circle_x, text_circle_y),
              cv2.FONT_HERSHEY_SIMPLEX, font_size, CV2_RED, line_width)
  # image center
  cv2.circle(img, (img_center_x, img_center_y), radius_pt, CV2_RED, filled_pt)
  text_imgct_x = move_text_dist * move_text_right_image + img_center_x
  text_imgct_y = move_text_dist * move_text_down_image + img_center_y
  cv2.putText(img, 'image center', (text_imgct_x, text_imgct_y),
              cv2.FONT_HERSHEY_SIMPLEX, font_size, CV2_RED, line_width)
  image_processing_utils.write_image(img/255, img_name, True)  # [0, 1] values


def get_angle(input_img):
  """Computes anglular inclination of chessboard in input_img.

  Args:
    input_img (2D numpy.ndarray): Grayscale image stored as a 2D numpy array.
  Returns:
    Median angle of squares in degrees identified in the image.

  Angle estimation algorithm description:
    Input: 2D grayscale image of chessboard.
    Output: Angle of rotation of chessboard perpendicular to
            chessboard. Assumes chessboard and camera are parallel to
            each other.

    1) Use adaptive threshold to make image binary
    2) Find countours
    3) Filter out small contours
    4) Filter out all non-square contours
    5) Compute most common square shape.
        The assumption here is that the most common square instances are the
        chessboard squares. We've shown that with our current tuning, we can
        robustly identify the squares on the sensor fusion chessboard.
    6) Return median angle of most common square shape.

  USAGE NOTE: This function has been tuned to work for the chessboard used in
  the sensor_fusion tests. See images in test_images/rotated_chessboard/ for
  sample captures. If this function is used with other chessboards, it may not
  work as expected.
  """
  # Tuning parameters
  square_area_min = (float)(input_img.shape[1] * SQUARE_AREA_MIN_REL)

  # Creates copy of image to avoid modifying original.
  img = numpy.array(input_img, copy=True)

  # Scale pixel values from 0-1 to 0-255
  img *= 255
  img = img.astype(numpy.uint8)
  img_thresh = cv2.adaptiveThreshold(
      img, 255, cv2.ADAPTIVE_THRESH_MEAN_C, cv2.THRESH_BINARY, 201, 2)

  # Find all contours.
  contours = find_all_contours(img_thresh)

  # Filter contours to squares only.
  square_contours = []
  for contour in contours:
    rect = cv2.minAreaRect(contour)
    _, (width, height), angle = rect

    # Skip non-squares
    if not numpy.isclose(width, height, rtol=SQUARE_TOL):
      continue

    # Remove very small contours: usually just tiny dots due to noise.
    area = cv2.contourArea(contour)
    if area < square_area_min:
      continue

    square_contours.append(contour)

  areas = []
  for contour in square_contours:
    area = cv2.contourArea(contour)
    areas.append(area)

  median_area = numpy.median(areas)

  filtered_squares = []
  filtered_angles = []
  for square in square_contours:
    area = cv2.contourArea(square)
    if not numpy.isclose(area, median_area, rtol=SQUARE_TOL):
      continue

    filtered_squares.append(square)
    _, (width, height), angle = cv2.minAreaRect(square)
    filtered_angles.append(angle)

  if len(filtered_angles) < ANGLE_NUM_MIN:
    logging.debug(
        'A frame had too few angles to be processed. '
        'Num of angles: %d, MIN: %d', len(filtered_angles), ANGLE_NUM_MIN)
    return None

  return numpy.median(filtered_angles)


class Cv2ImageProcessingUtilsTests(unittest.TestCase):
  """Unit tests for this module."""

  def test_get_angle_identify_unrotated_chessboard_angle(self):
    normal_img_path = os.path.join(
        TEST_IMG_DIR, 'rotated_chessboards/normal.jpg')
    wide_img_path = os.path.join(
        TEST_IMG_DIR, 'rotated_chessboards/wide.jpg')
    normal_img = cv2.cvtColor(cv2.imread(normal_img_path), cv2.COLOR_BGR2GRAY)
    wide_img = cv2.cvtColor(cv2.imread(wide_img_path), cv2.COLOR_BGR2GRAY)
    normal_angle = get_angle(normal_img)
    wide_angle = get_angle(wide_img)
    e_msg = f'Angle: 0, Regular: {normal_angle}, Wide: {wide_angle}'
    self.assertEqual(get_angle(normal_img), 0, e_msg)
    self.assertEqual(get_angle(wide_img), 0, e_msg)

  def test_get_angle_identify_rotated_chessboard_angle(self):
    # Array of the image files and angles containing rotated chessboards.
    test_cases = [
        ('_15_ccw', 15),
        ('_30_ccw', 30),
        ('_45_ccw', 45),
        ('_60_ccw', 60),
        ('_75_ccw', 75),
        ('_90_ccw', 90)
    ]

    # For each rotated image pair (normal, wide), check angle against expected.
    for suffix, angle in test_cases:
      # Define image paths.
      normal_img_path = os.path.join(
          TEST_IMG_DIR, f'rotated_chessboards/normal{suffix}.jpg')
      wide_img_path = os.path.join(
          TEST_IMG_DIR, f'rotated_chessboards/wide{suffix}.jpg')

      # Load and color-convert images.
      normal_img = cv2.cvtColor(cv2.imread(normal_img_path), cv2.COLOR_BGR2GRAY)
      wide_img = cv2.cvtColor(cv2.imread(wide_img_path), cv2.COLOR_BGR2GRAY)

      # Assert angle as expected.
      normal_angle = get_angle(normal_img)
      wide_angle = get_angle(wide_img)
      e_msg = f'Angle: {angle}, Regular: {normal_angle}, Wide: {wide_angle}'
      self.assertTrue(
          numpy.isclose(abs(normal_angle), angle, ANGLE_CHECK_TOL), e_msg)
      self.assertTrue(
          numpy.isclose(abs(wide_angle), angle, ANGLE_CHECK_TOL), e_msg)


if __name__ == '__main__':
  unittest.main()
