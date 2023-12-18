# Copyright 2015 The Android Open Source Project
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
"""Validate aspect ratio, crop and FoV vs format."""


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

_ANDROID11_API_LEVEL = 30
_CIRCLE_COLOR = 0  # [0: black, 255: white].
_CIRCLE_MIN_AREA = 0.01  # 1% of image size.
_FOV_PERCENT_RTOL = 0.15  # Relative tolerance on circle FoV % to expected.
_LARGE_SIZE = 2000  # Size of a large image (compared against max(w, h)).
_NAME = os.path.splitext(os.path.basename(__file__))[0]
_PREVIEW_SIZE = (1920, 1080)
_THRESH_AR_L = 0.02  # Aspect ratio test threshold of large images.
_THRESH_AR_S = 0.075  # Aspect ratio test threshold of mini images.
_THRESH_CROP_L = 0.02  # Crop test threshold of large images.
_THRESH_CROP_S = 0.075  # Crop test threshold of mini images.
_THRESH_MIN_PIXEL = 4  # Crop test allowed offset.

# Before API level 30, only resolutions with the following listed aspect ratio
# are checked. Device launched after API level 30 will need to pass the test
# for all advertised resolutions. Device launched before API level 30 just
# needs to pass the test for all resolutions within these aspect ratios.
_AR_CHECKED_PRE_API_30 = ('4:3', '16:9', '18:9')
_AR_DIFF_ATOL = 0.01


def _check_skip_conditions(first_api_level, props):
  """Check the skip conditions based on first API level."""
  if first_api_level < _ANDROID11_API_LEVEL:  # Original constraint.
    camera_properties_utils.skip_unless(camera_properties_utils.read_3a(props))
  else:  # Loosen from read_3a to enable LIMITED coverage.
    camera_properties_utils.skip_unless(
        camera_properties_utils.ae_lock(props) and
        camera_properties_utils.awb_lock(props))


def _check_basic_correctness(cap, fmt_iter, w_iter, h_iter):
  """Check the capture for basic correctness."""
  if cap['format'] != fmt_iter:
    raise AssertionError
  if cap['width'] != w_iter:
    raise AssertionError
  if cap['height'] != h_iter:
    raise AssertionError


def _create_format_list():
  """Create format list for multiple capture objects.

  Do multi-capture of 'iter' and 'cmpr'. Iterate through all the available
  sizes of 'iter', and only use the size specified for 'cmpr'.
  The 'cmpr' capture is only used so that we have multiple capture target
  instead of just one, which should help catching more potential issues.
  The test doesn't look into the output of 'cmpr' images at all.
  The 'iter_max' or 'cmpr_size' key defines the maximal size being iterated
  or selected for the 'iter' and 'cmpr' stream accordingly. None means no
  upper bound is specified.

  Args:
    None

  Returns:
    format_list
  """
  format_list = []
  format_list.append({'iter': 'yuv', 'iter_max': None,
                      'cmpr': 'yuv', 'cmpr_size': _PREVIEW_SIZE})
  format_list.append({'iter': 'yuv', 'iter_max': _PREVIEW_SIZE,
                      'cmpr': 'jpeg', 'cmpr_size': None})
  format_list.append({'iter': 'yuv', 'iter_max': _PREVIEW_SIZE,
                      'cmpr': 'raw', 'cmpr_size': None})
  format_list.append({'iter': 'jpeg', 'iter_max': None,
                      'cmpr': 'raw', 'cmpr_size': None})
  format_list.append({'iter': 'jpeg', 'iter_max': None,
                      'cmpr': 'yuv', 'cmpr_size': _PREVIEW_SIZE})
  return format_list


def _print_failed_test_results(failed_ar, failed_fov, failed_crop,
                               first_api_level, level_3):
  """Print failed test results."""
  if failed_ar:
    logging.error('Aspect ratio test summary')
    logging.error('Images failed in the aspect ratio test:')
    logging.error('Aspect ratio value: width / height')
    for fa in failed_ar:
      logging.error('%s', fa)

  if failed_fov:
    logging.error('FoV test summary')
    logging.error('Images failed in the FoV test:')
    for fov in failed_fov:
      logging.error('%s', str(fov))

  if failed_crop:
    logging.error('Crop test summary')
    logging.error('Images failed in the crop test:')
    logging.error('Circle center (H x V) relative to the image center.')
    for fc in failed_crop:
      logging.error('%s', fc)
  if failed_ar:
    raise RuntimeError
  if failed_fov:
    raise RuntimeError
  if first_api_level > _ANDROID11_API_LEVEL:
    if failed_crop:  # failed_crop = [] if run_crop_test = False.
      raise RuntimeError
  else:
    if failed_crop and level_3:
      raise RuntimeError


def _is_checked_aspect_ratio(first_api_level, w, h):
  """Determine if format aspect ratio is a checked on based of first_API."""
  if first_api_level >= _ANDROID11_API_LEVEL:
    return True

  for ar_check in _AR_CHECKED_PRE_API_30:
    match_ar_list = [float(x) for x in ar_check.split(':')]
    match_ar = match_ar_list[0] / match_ar_list[1]
    if np.isclose(float(w) / h, match_ar, atol=_AR_DIFF_ATOL):
      return True

  return False


def _calc_expected_circle_image_ratio(ref_fov, img_w, img_h):
  """Determine the circle image area ratio in percentage for a given image size.

  Cropping happens either horizontally or vertically. In both cases crop results
  in the visble area reduced by a ratio r (r < 1) and the circle will in turn
  occupy ref_pct/r (percent) on the target image size.

  Args:
    ref_fov: dict with {fmt, % coverage, w, h, circle_w, circle_h}
    img_w: the image width
    img_h: the image height

  Returns:
    chk_percent: the expected circle image area ratio in percentage
  """
  ar_ref = ref_fov['w'] / ref_fov['h']
  ar_target = img_w / img_h

  r = ar_ref / ar_target
  if r < 1.0:
    r = 1.0 / r
  return ref_fov['percent'] * r


def _find_raw_fov_reference(cam, req, props, log_path):
  """Determine the circle coverage of the image in RAW reference image.

  Captures a full-frame RAW and uses its aspect ratio and circle center
  location as ground truth for the other jpeg or yuv images.

  The intrinsics and distortion coefficients are meant for full-sized RAW,
  so convert_capture_to_rgb_image returns a 2x downsampled version, so resizes
  RGB back to full size.

  If the device supports lens distortion correction, applies the coefficients on
  the RAW image so it can be compared to YUV/JPEG outputs which are subject
  to the same correction via ISP.

  Finds circle size and location for reference values in calculations for other
  formats.

  Args:
    cam: camera object
    req: camera request
    props: camera properties
    log_path: location to save data

  Returns:
    ref_fov: dict with [fmt, % coverage, w, h, circle_w, circle_h]
    cc_ct_gt: circle center position relative to the center of image.
    aspect_ratio_gt: aspect ratio of the detected circle in float.
  """
  logging.debug('Creating references for fov_coverage from RAW')
  out_surface = {'format': 'raw'}
  cap_raw = cam.do_capture(req, out_surface)
  logging.debug('Captured RAW %dx%d', cap_raw['width'], cap_raw['height'])
  img_raw = image_processing_utils.convert_capture_to_rgb_image(
      cap_raw, props=props)
  # Resize back up to full scale.
  img_raw = cv2.resize(img_raw, (0, 0), fx=2.0, fy=2.0)

  if (camera_properties_utils.distortion_correction(props) and
      camera_properties_utils.intrinsic_calibration(props)):
    logging.debug('Applying intrinsic calibration and distortion params')
    fd = float(cap_raw['metadata']['android.lens.focalLength'])
    k = camera_properties_utils.get_intrinsic_calibration(props, True, fd)
    opencv_dist = camera_properties_utils.get_distortion_matrix(props)
    img_raw = cv2.undistort(img_raw, k, opencv_dist)

  # Get image size.
  size_raw = img_raw.shape
  w_raw = size_raw[1]
  h_raw = size_raw[0]
  img_name = '%s_%s_w%d_h%d.png' % (
      os.path.join(log_path, _NAME), 'raw', w_raw, h_raw)
  image_processing_utils.write_image(img_raw, img_name, True)

  # Find circle.
  img_raw *= 255  # cv2 needs images between [0,255].
  circle_raw = opencv_processing_utils.find_circle(
      img_raw, img_name, _CIRCLE_MIN_AREA, _CIRCLE_COLOR)
  opencv_processing_utils.append_circle_center_to_img(circle_raw, img_raw,
                                                      img_name)

  # Determine final return values.
  aspect_ratio_gt = circle_raw['w'] / circle_raw['h']
  cc_ct_gt = {'hori': circle_raw['x_offset'], 'vert': circle_raw['y_offset']}
  raw_fov_percent = _calc_circle_image_ratio(circle_raw['r'], w_raw, h_raw)
  ref_fov = {}
  ref_fov['fmt'] = 'RAW'
  ref_fov['percent'] = raw_fov_percent
  ref_fov['w'] = w_raw
  ref_fov['h'] = h_raw
  ref_fov['circle_w'] = circle_raw['w']
  ref_fov['circle_h'] = circle_raw['h']
  logging.debug('Using RAW reference: %s', str(ref_fov))
  return ref_fov, cc_ct_gt, aspect_ratio_gt


def _find_jpeg_fov_reference(cam, req, props, log_path):
  """Determine the circle coverage of the image in JPEG reference image.

  Similar to _find_raw_fov_reference() and used when RAW is not available.

  Args:
    cam: camera object
    req: camera request
    props: camera properties
    log_path: location to save data

  Returns:
    ref_fov: dict with [fmt, % coverage, w, h, circle_w, circle_h]
    cc_ct_gt: circle center position relative to the center of image.
  """
  ref_fov = {}
  fmt = capture_request_utils.get_largest_jpeg_format(props)
  # Capture and determine circle area in image.
  cap = cam.do_capture(req, fmt)
  w = cap['width']
  h = cap['height']

  img = image_processing_utils.convert_capture_to_rgb_image(cap, props)
  img *= 255  # cv2 works with [0,255] images.
  logging.debug('Captured JPEG %dx%d', w, h)
  img_name = '%s_jpeg_w%d_h%d.png' % (os.path.join(log_path, _NAME), w, h)
  circle_jpg = opencv_processing_utils.find_circle(
      img, img_name, _CIRCLE_MIN_AREA, _CIRCLE_COLOR)
  opencv_processing_utils.append_circle_center_to_img(circle_jpg, img,
                                                      img_name)

  # Determine final return values.
  cc_ct_gt = {'hori': circle_jpg['x_offset'], 'vert': circle_jpg['y_offset']}
  fov_percent = _calc_circle_image_ratio(circle_jpg['r'], w, h)
  ref_fov = {}
  ref_fov['fmt'] = 'JPEG'
  ref_fov['percent'] = fov_percent
  ref_fov['w'] = w
  ref_fov['h'] = h
  ref_fov['circle_w'] = circle_jpg['w']
  ref_fov['circle_h'] = circle_jpg['h']
  logging.debug('Using JPEG reference: %s', str(ref_fov))
  return ref_fov, cc_ct_gt


def _calc_circle_image_ratio(radius, img_w, img_h):
  """Calculate the percent of area the input circle covers in input image.

  Args:
    radius: radius of circle
    img_w: int width of image
    img_h: int height of image
  Returns:
    fov_percent: float % of image covered by circle
  """
  return 100 * math.pi * math.pow(radius, 2) / (img_w * img_h)


def _check_fov(circle, ref_fov, w, h, first_api_level):
  """Check the FoV for correct size."""
  fov_percent = _calc_circle_image_ratio(circle['r'], w, h)
  chk_percent = _calc_expected_circle_image_ratio(ref_fov, w, h)
  chk_enabled = _is_checked_aspect_ratio(first_api_level, w, h)
  if chk_enabled and not np.isclose(fov_percent, chk_percent,
                                    rtol=_FOV_PERCENT_RTOL):
    e_msg = 'FoV %%: %.2f, Ref FoV %%: %.2f, ' % (fov_percent, chk_percent)
    e_msg += 'TOL=%.f%%, img: %dx%d, ref: %dx%d' % (
        _FOV_PERCENT_RTOL*100, w, h, ref_fov['w'], ref_fov['h'])
    return e_msg


def _check_ar(circle, ar_gt, w, h, fmt_iter, fmt_cmpr):
  """Check the aspect ratio of the circle.

  size is the larger of w or h.
  if size >= LARGE_SIZE: use THRESH_AR_L
  elif size == 0 (extreme case): THRESH_AR_S
  elif 0 < image size < LARGE_SIZE: scale between THRESH_AR_S & THRESH_AR_L

  Args:
    circle: dict with circle parameters
    ar_gt: aspect ratio ground truth to compare against
    w: width of image
    h: height of image
    fmt_iter: format of primary capture
    fmt_cmpr: format of secondary capture

  Returns:
    error string if check fails
  """
  thresh_ar = max(_THRESH_AR_L, _THRESH_AR_S +
                  max(w, h) * (_THRESH_AR_L-_THRESH_AR_S) / _LARGE_SIZE)
  ar = circle['w'] / circle['h']
  if not np.isclose(ar, ar_gt, atol=thresh_ar):
    e_msg = (f'{fmt_iter} with {fmt_cmpr} {w}x{h}: aspect_ratio {ar:.3f}, '
             f'thresh {thresh_ar:.3f}')
    return e_msg


def _check_crop(circle, cc_gt, w, h, fmt_iter, fmt_cmpr, crop_thresh_factor):
  """Check cropping.

  if size >= LARGE_SIZE: use thresh_crop_l
  elif size == 0 (extreme case): thresh_crop_s
  elif 0 < size < LARGE_SIZE: scale between thresh_crop_s & thresh_crop_l
  Also allow at least THRESH_MIN_PIXEL to prevent threshold being too tight
  for very small circle.

  Args:
    circle: dict of circle values
    cc_gt: circle center {'hori', 'vert'}  ground truth (ref'd to img center)
    w: width of image
    h: height of image
    fmt_iter: format of primary capture
    fmt_cmpr: format of secondary capture
    crop_thresh_factor: scaling factor for crop thresholds

  Returns:
    error string if check fails
  """
  thresh_crop_l = _THRESH_CROP_L * crop_thresh_factor
  thresh_crop_s = _THRESH_CROP_S * crop_thresh_factor
  thresh_crop_hori = max(
      [thresh_crop_l,
       thresh_crop_s + w * (thresh_crop_l - thresh_crop_s) / _LARGE_SIZE,
       _THRESH_MIN_PIXEL / circle['w']])
  thresh_crop_vert = max(
      [thresh_crop_l,
       thresh_crop_s + h * (thresh_crop_l - thresh_crop_s) / _LARGE_SIZE,
       _THRESH_MIN_PIXEL / circle['h']])

  if (not np.isclose(circle['x_offset'], cc_gt['hori'],
                     atol=thresh_crop_hori) or
      not np.isclose(circle['y_offset'], cc_gt['vert'],
                     atol=thresh_crop_vert)):
    valid_x_range = (cc_gt['hori'] - thresh_crop_hori,
                     cc_gt['hori'] + thresh_crop_hori)
    valid_y_range = (cc_gt['vert'] - thresh_crop_vert,
                     cc_gt['vert'] + thresh_crop_vert)
    e_msg = (f'{fmt_iter} with {fmt_cmpr} {w}x{h} '
             f"offset X {circle['x_offset']:.3f}, Y {circle['y_offset']:.3f}, "
             f'valid X range: {valid_x_range[0]:.3f} ~ {valid_x_range[1]:.3f}, '
             f'valid Y range: {valid_y_range[0]:.3f} ~ {valid_y_range[1]:.3f}')
    return e_msg


class AspectRatioAndCropTest(its_base_test.ItsBaseTest):
  """Test aspect ratio/field of view/cropping for each tested fmt combinations.

  This test checks for:
    1. Aspect ratio: images are not stretched
    2. Crop: center of images is not shifted
    3. FOV: images cropped to keep maximum possible FOV with only 1 dimension
       (horizontal or veritical) cropped.

  Aspect ratio and FOV test runs on level3, full and limited devices.
  Crop test only runs on level3 and full devices.

  The test chart is a black circle inside a black square. When raw capture is
  available, set the height vs. width ratio of the circle in the full-frame
  raw as ground truth. In an ideal setup such ratio should be very close to
  1.0, but here we just use the value derived from full resolution RAW as
  ground truth to account for the possibility that the chart is not well
  positioned to be precisely parallel to image sensor plane.
  The test then compares the ground truth ratio with the same ratio measured
  on images captued using different stream combinations of varying formats
  ('jpeg' and 'yuv') and resolutions.
  If raw capture is unavailable, a full resolution JPEG image is used to setup
  ground truth. In this case, the ground truth aspect ratio is defined as 1.0
  and it is the tester's responsibility to make sure the test chart is
  properly positioned so the detected circles indeed have aspect ratio close
  to 1.0 assuming no bugs causing image stretched.

  The aspect ratio test checks the aspect ratio of the detected circle and
  it will fail if the aspect ratio differs too much from the ground truth
  aspect ratio mentioned above.

  The FOV test examines the ratio between the detected circle area and the
  image size. When the aspect ratio of the test image is the same as the
  ground truth image, the ratio should be very close to the ground truth
  value. When the aspect ratio is different, the difference is factored in
  per the expectation of the Camera2 API specification, which mandates the
  FOV reduction from full sensor area must only occur in one dimension:
  horizontally or vertically, and never both. For example, let's say a sensor
  has a 16:10 full sensor FOV. For all 16:10 output images there should be no
  FOV reduction on them. For 16:9 output images the FOV should be vertically
  cropped by 9/10. For 4:3 output images the FOV should be cropped
  horizontally instead and the ratio (r) can be calculated as follows:
      (16 * r) / 10 = 4 / 3 => r = 40 / 48 = 0.8333
  Say the circle is covering x percent of the 16:10 sensor on the full 16:10
  FOV, and assume the circle in the center will never be cut in any output
  sizes (this can be achieved by picking the right size and position of the
  test circle), the from above cropping expectation we can derive on a 16:9
  output image the circle will cover (x / 0.9) percent of the 16:9 image; on
  a 4:3 output image the circle will cover (x / 0.8333) percent of the 4:3
  image.

  The crop test checks that the center of any output image remains aligned
  with center of sensor's active area, no matter what kind of cropping or
  scaling is applied. The test verifies that by checking the relative vector
  from the image center to the center of detected circle remains unchanged.
  The relative part is normalized by the detected circle size to account for
  scaling effect.
  """

  def test_aspect_ratio_and_crop(self):
    logging.debug('Starting %s', _NAME)
    failed_ar = []  # Streams failed the aspect ratio test.
    failed_crop = []  # Streams failed the crop test.
    failed_fov = []  # Streams that fail FoV test.
    format_list = _create_format_list()

    with its_session_utils.ItsSession(
        device_id=self.dut.serial,
        camera_id=self.camera_id,
        hidden_physical_id=self.hidden_physical_id) as cam:
      props = cam.get_camera_properties()
      fls_logical = props['android.lens.info.availableFocalLengths']
      logging.debug('logical available focal lengths: %s', str(fls_logical))
      props = cam.override_with_hidden_physical_camera_props(props)
      fls_physical = props['android.lens.info.availableFocalLengths']
      logging.debug('physical available focal lengths: %s', str(fls_physical))
      log_path = self.log_path

      # Check SKIP conditions.
      first_api_level = its_session_utils.get_first_api_level(self.dut.serial)
      _check_skip_conditions(first_api_level, props)

      # Load chart for scene.
      its_session_utils.load_scene(
          cam, props, self.scene, self.tablet, self.chart_distance)

      # Determine camera capabilities.
      full_or_better = camera_properties_utils.full_or_better(props)
      level3 = camera_properties_utils.level3(props)
      raw_avlb = camera_properties_utils.raw16(props)
      debug = self.debug_mode

      # Converge 3A.
      cam.do_3a()
      req = capture_request_utils.auto_capture_request()

      # If raw is available and main camera, use it as ground truth.
      if raw_avlb and (fls_physical == fls_logical):
        ref_fov, cc_ct_gt, aspect_ratio_gt = _find_raw_fov_reference(
            cam, req, props, log_path)
      else:
        aspect_ratio_gt = 1.0  # Ground truth circle width/height ratio.
        ref_fov, cc_ct_gt = _find_jpeg_fov_reference(cam, req, props, log_path)

      run_crop_test = full_or_better and raw_avlb
      if run_crop_test:
        # Normalize the circle size to 1/4 of the image size, so that
        # circle size won't affect the crop test result
        crop_thresh_factor = ((min(ref_fov['w'], ref_fov['h']) / 4.0) /
                              max(ref_fov['circle_w'], ref_fov['circle_h']))
      else:
        logging.debug('Crop test skipped')

      # Take pictures of each settings with all the image sizes available.
      for fmt in format_list:
        fmt_iter = fmt['iter']
        fmt_cmpr = fmt['cmpr']
        # Get the size of 'cmpr'.
        sizes = capture_request_utils.get_available_output_sizes(
            fmt_cmpr, props, fmt['cmpr_size'])
        if not sizes:  # Device might not support RAW.
          continue
        w_cmpr, h_cmpr = sizes[0][0], sizes[0][1]
        for size_iter in capture_request_utils.get_available_output_sizes(
            fmt_iter, props, fmt['iter_max']):
          w_iter, h_iter = size_iter[0], size_iter[1]
          # Skip same format/size combination: ITS doesn't handle that properly.
          if w_iter*h_iter == w_cmpr*h_cmpr and fmt_iter == fmt_cmpr:
            continue
          out_surface = [{'width': w_iter, 'height': h_iter,
                          'format': fmt_iter}]
          out_surface.append({'width': w_cmpr, 'height': h_cmpr,
                              'format': fmt_cmpr})

          cap = cam.do_capture(req, out_surface)[0]
          _check_basic_correctness(cap, fmt_iter, w_iter, h_iter)
          logging.debug('Captured %s with %s %dx%d. Compared size: %dx%d',
                        fmt_iter, fmt_cmpr, w_iter, h_iter, w_cmpr, h_cmpr)
          img = image_processing_utils.convert_capture_to_rgb_image(cap)
          img *= 255  # cv2 uses [0, 255].
          img_name = '%s_%s_with_%s_w%d_h%d.png' % (
              os.path.join(log_path, _NAME), fmt_iter, fmt_cmpr, w_iter, h_iter)
          circle = opencv_processing_utils.find_circle(
              img, img_name, _CIRCLE_MIN_AREA, _CIRCLE_COLOR)
          if debug:
            opencv_processing_utils.append_circle_center_to_img(circle, img,
                                                                img_name)

          # Check pass/fail for fov coverage for all fmts in AR_CHECKED
          img /= 255  # image_processing_utils uses [0, 1].
          fov_chk_msg = _check_fov(circle, ref_fov, w_iter, h_iter,
                                   first_api_level)
          if fov_chk_msg:
            failed_fov.append(fov_chk_msg)
            image_processing_utils.write_image(img, img_name, True)

          # Check pass/fail for aspect ratio.
          ar_chk_msg = _check_ar(
              circle, aspect_ratio_gt, w_iter, h_iter, fmt_iter, fmt_cmpr)
          if ar_chk_msg:
            failed_ar.append(ar_chk_msg)
            image_processing_utils.write_image(img, img_name, True)

          # Check pass/fail for crop.
          if run_crop_test:
            crop_chk_msg = _check_crop(circle, cc_ct_gt, w_iter, h_iter,
                                       fmt_iter, fmt_cmpr, crop_thresh_factor)
            if crop_chk_msg:
              failed_crop.append(crop_chk_msg)
              image_processing_utils.write_image(img, img_name, True)

      # Print any failed test results.
      _print_failed_test_results(failed_ar, failed_fov, failed_crop,
                                 first_api_level, level3)

if __name__ == '__main__':
  test_runner.main()
