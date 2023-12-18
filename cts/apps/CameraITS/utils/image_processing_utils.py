# Copyright 2013 The Android Open Source Project
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
"""Image processing utility functions."""


import copy
import io
import logging
import math
import os
import random
import sys
import unittest

import numpy
from PIL import Image


import cv2
import capture_request_utils
import error_util

# The matrix is from JFIF spec
DEFAULT_YUV_TO_RGB_CCM = numpy.matrix([[1.000, 0.000, 1.402],
                                       [1.000, -0.344, -0.714],
                                       [1.000, 1.772, 0.000]])

DEFAULT_YUV_OFFSETS = numpy.array([0, 128, 128])
MAX_LUT_SIZE = 65536
DEFAULT_GAMMA_LUT = numpy.array([
    math.floor((MAX_LUT_SIZE-1) * math.pow(i/(MAX_LUT_SIZE-1), 1/2.2) + 0.5)
    for i in range(MAX_LUT_SIZE)])
NUM_TRIES = 2
NUM_FRAMES = 4
TEST_IMG_DIR = os.path.join(os.environ['CAMERA_ITS_TOP'], 'test_images')


# pylint: disable=unused-argument
def convert_capture_to_rgb_image(cap,
                                 ccm_yuv_to_rgb=DEFAULT_YUV_TO_RGB_CCM,
                                 yuv_off=DEFAULT_YUV_OFFSETS,
                                 props=None):
  """Convert a captured image object to a RGB image.

  Args:
     cap: A capture object as returned by its_session_utils.do_capture.
     ccm_yuv_to_rgb: (Optional) the 3x3 CCM to convert from YUV to RGB.
     yuv_off: (Optional) offsets to subtract from each of Y,U,V values.
     props: (Optional) camera properties object (of static values);
            required for processing raw images.

  Returns:
        RGB float-3 image array, with pixel values in [0.0, 1.0].
  """
  w = cap['width']
  h = cap['height']
  if cap['format'] == 'raw10':
    assert props is not None
    cap = unpack_raw10_capture(cap)

  if cap['format'] == 'raw12':
    assert props is not None
    cap = unpack_raw12_capture(cap)

  if cap['format'] == 'yuv':
    y = cap['data'][0: w * h]
    u = cap['data'][w * h: w * h * 5//4]
    v = cap['data'][w * h * 5//4: w * h * 6//4]
    return convert_yuv420_planar_to_rgb_image(y, u, v, w, h)
  elif cap['format'] == 'jpeg':
    return decompress_jpeg_to_rgb_image(cap['data'])
  elif cap['format'] == 'raw' or cap['format'] == 'rawStats':
    assert props is not None
    r, gr, gb, b = convert_capture_to_planes(cap, props)
    return convert_raw_to_rgb_image(r, gr, gb, b, props, cap['metadata'])
  elif cap['format'] == 'y8':
    y = cap['data'][0: w * h]
    return convert_y8_to_rgb_image(y, w, h)
  else:
    raise error_util.CameraItsError('Invalid format %s' % (cap['format']))


def unpack_raw10_capture(cap):
  """Unpack a raw-10 capture to a raw-16 capture.

  Args:
    cap: A raw-10 capture object.

  Returns:
    New capture object with raw-16 data.
  """
  # Data is packed as 4x10b pixels in 5 bytes, with the first 4 bytes holding
  # the MSBs of the pixels, and the 5th byte holding 4x2b LSBs.
  w, h = cap['width'], cap['height']
  if w % 4 != 0:
    raise error_util.CameraItsError('Invalid raw-10 buffer width')
  cap = copy.deepcopy(cap)
  cap['data'] = unpack_raw10_image(cap['data'].reshape(h, w * 5 // 4))
  cap['format'] = 'raw'
  return cap


def unpack_raw10_image(img):
  """Unpack a raw-10 image to a raw-16 image.

  Output image will have the 10 LSBs filled in each 16b word, and the 6 MSBs
  will be set to zero.

  Args:
    img: A raw-10 image, as a uint8 numpy array.

  Returns:
    Image as a uint16 numpy array, with all row padding stripped.
  """
  if img.shape[1] % 5 != 0:
    raise error_util.CameraItsError('Invalid raw-10 buffer width')
  w = img.shape[1] * 4 // 5
  h = img.shape[0]
  # Cut out the 4x8b MSBs and shift to bits [9:2] in 16b words.
  msbs = numpy.delete(img, numpy.s_[4::5], 1)
  msbs = msbs.astype(numpy.uint16)
  msbs = numpy.left_shift(msbs, 2)
  msbs = msbs.reshape(h, w)
  # Cut out the 4x2b LSBs and put each in bits [1:0] of their own 8b words.
  lsbs = img[::, 4::5].reshape(h, w // 4)
  lsbs = numpy.right_shift(
      numpy.packbits(numpy.unpackbits(lsbs).reshape(h, w // 4, 4, 2), 3), 6)
  # Pair the LSB bits group to 0th pixel instead of 3rd pixel
  lsbs = lsbs.reshape(h, w // 4, 4)[:, :, ::-1]
  lsbs = lsbs.reshape(h, w)
  # Fuse the MSBs and LSBs back together
  img16 = numpy.bitwise_or(msbs, lsbs).reshape(h, w)
  return img16


def unpack_raw12_capture(cap):
  """Unpack a raw-12 capture to a raw-16 capture.

  Args:
    cap: A raw-12 capture object.

  Returns:
     New capture object with raw-16 data.
  """
  # Data is packed as 4x10b pixels in 5 bytes, with the first 4 bytes holding
  # the MSBs of the pixels, and the 5th byte holding 4x2b LSBs.
  w, h = cap['width'], cap['height']
  if w % 2 != 0:
    raise error_util.CameraItsError('Invalid raw-12 buffer width')
  cap = copy.deepcopy(cap)
  cap['data'] = unpack_raw12_image(cap['data'].reshape(h, w * 3 // 2))
  cap['format'] = 'raw'
  return cap


def unpack_raw12_image(img):
  """Unpack a raw-12 image to a raw-16 image.

  Output image will have the 12 LSBs filled in each 16b word, and the 4 MSBs
  will be set to zero.

  Args:
   img: A raw-12 image, as a uint8 numpy array.

  Returns:
    Image as a uint16 numpy array, with all row padding stripped.
  """
  if img.shape[1] % 3 != 0:
    raise error_util.CameraItsError('Invalid raw-12 buffer width')
  w = img.shape[1] * 2 // 3
  h = img.shape[0]
  # Cut out the 2x8b MSBs and shift to bits [11:4] in 16b words.
  msbs = numpy.delete(img, numpy.s_[2::3], 1)
  msbs = msbs.astype(numpy.uint16)
  msbs = numpy.left_shift(msbs, 4)
  msbs = msbs.reshape(h, w)
  # Cut out the 2x4b LSBs and put each in bits [3:0] of their own 8b words.
  lsbs = img[::, 2::3].reshape(h, w // 2)
  lsbs = numpy.right_shift(
      numpy.packbits(numpy.unpackbits(lsbs).reshape(h, w // 2, 2, 4), 3), 4)
  # Pair the LSB bits group to pixel 0 instead of pixel 1
  lsbs = lsbs.reshape(h, w // 2, 2)[:, :, ::-1]
  lsbs = lsbs.reshape(h, w)
  # Fuse the MSBs and LSBs back together
  img16 = numpy.bitwise_or(msbs, lsbs).reshape(h, w)
  return img16


def convert_yuv420_planar_to_rgb_image(y_plane, u_plane, v_plane,
                                       w, h,
                                       ccm_yuv_to_rgb=DEFAULT_YUV_TO_RGB_CCM,
                                       yuv_off=DEFAULT_YUV_OFFSETS):
  """Convert a YUV420 8-bit planar image to an RGB image.

  Args:
    y_plane: The packed 8-bit Y plane.
    u_plane: The packed 8-bit U plane.
    v_plane: The packed 8-bit V plane.
    w: The width of the image.
    h: The height of the image.
    ccm_yuv_to_rgb: (Optional) the 3x3 CCM to convert from YUV to RGB.
    yuv_off: (Optional) offsets to subtract from each of Y,U,V values.

  Returns:
    RGB float-3 image array, with pixel values in [0.0, 1.0].
  """
  y = numpy.subtract(y_plane, yuv_off[0])
  u = numpy.subtract(u_plane, yuv_off[1]).view(numpy.int8)
  v = numpy.subtract(v_plane, yuv_off[2]).view(numpy.int8)
  u = u.reshape(h // 2, w // 2).repeat(2, axis=1).repeat(2, axis=0)
  v = v.reshape(h // 2, w // 2).repeat(2, axis=1).repeat(2, axis=0)
  yuv = numpy.dstack([y, u.reshape(w * h), v.reshape(w * h)])
  flt = numpy.empty([h, w, 3], dtype=numpy.float32)
  flt.reshape(w * h * 3)[:] = yuv.reshape(h * w * 3)[:]
  flt = numpy.dot(flt.reshape(w * h, 3), ccm_yuv_to_rgb.T).clip(0, 255)
  rgb = numpy.empty([h, w, 3], dtype=numpy.uint8)
  rgb.reshape(w * h * 3)[:] = flt.reshape(w * h * 3)[:]
  return rgb.astype(numpy.float32) / 255.0


def decompress_jpeg_to_rgb_image(jpeg_buffer):
  """Decompress a JPEG-compressed image, returning as an RGB image.

  Args:
    jpeg_buffer: The JPEG stream.

  Returns:
     A numpy array for the RGB image, with pixels in [0,1].
  """
  img = Image.open(io.BytesIO(jpeg_buffer))
  w = img.size[0]
  h = img.size[1]
  return numpy.array(img).reshape(h, w, 3) / 255.0


def convert_capture_to_planes(cap, props=None):
  """Convert a captured image object to separate image planes.

  Decompose an image into multiple images, corresponding to different planes.

  For YUV420 captures ("yuv"):
        Returns Y,U,V planes, where the Y plane is full-res and the U,V planes
        are each 1/2 x 1/2 of the full res.

    For Bayer captures ("raw", "raw10", "raw12", or "rawStats"):
        Returns planes in the order R,Gr,Gb,B, regardless of the Bayer pattern
        layout. For full-res raw images ("raw", "raw10", "raw12"), each plane
        is 1/2 x 1/2 of the full res. For "rawStats" images, the mean image
        is returned.

    For JPEG captures ("jpeg"):
        Returns R,G,B full-res planes.

  Args:
    cap: A capture object as returned by its_session_utils.do_capture.
    props: (Optional) camera properties object (of static values);
            required for processing raw images.

  Returns:
    A tuple of float numpy arrays (one per plane), consisting of pixel values
    in the range [0.0, 1.0].
  """
  w = cap['width']
  h = cap['height']
  if cap['format'] == 'raw10':
    assert props is not None
    cap = unpack_raw10_capture(cap)
  if cap['format'] == 'raw12':
    assert props is not None
    cap = unpack_raw12_capture(cap)
  if cap['format'] == 'yuv':
    y = cap['data'][0:w * h]
    u = cap['data'][w * h:w * h * 5 // 4]
    v = cap['data'][w * h * 5 // 4:w * h * 6 // 4]
    return ((y.astype(numpy.float32) / 255.0).reshape(h, w, 1),
            (u.astype(numpy.float32) / 255.0).reshape(h // 2, w // 2, 1),
            (v.astype(numpy.float32) / 255.0).reshape(h // 2, w // 2, 1))
  elif cap['format'] == 'jpeg':
    rgb = decompress_jpeg_to_rgb_image(cap['data']).reshape(w * h * 3)
    return (rgb[::3].reshape(h, w, 1), rgb[1::3].reshape(h, w, 1),
            rgb[2::3].reshape(h, w, 1))
  elif cap['format'] == 'raw':
    assert props is not None
    white_level = float(props['android.sensor.info.whiteLevel'])
    img = numpy.ndarray(
        shape=(h * w,), dtype='<u2', buffer=cap['data'][0:w * h * 2])
    img = img.astype(numpy.float32).reshape(h, w) / white_level
    # Crop the raw image to the active array region.
    if (props.get('android.sensor.info.preCorrectionActiveArraySize') is
        not None and
        props.get('android.sensor.info.pixelArraySize') is not None):
      # Note that the Rect class is defined such that the left,top values
      # are "inside" while the right,bottom values are "outside"; that is,
      # it's inclusive of the top,left sides only. So, the width is
      # computed as right-left, rather than right-left+1, etc.
      wfull = props['android.sensor.info.pixelArraySize']['width']
      hfull = props['android.sensor.info.pixelArraySize']['height']
      xcrop = props['android.sensor.info.preCorrectionActiveArraySize']['left']
      ycrop = props['android.sensor.info.preCorrectionActiveArraySize']['top']
      wcrop = props['android.sensor.info.preCorrectionActiveArraySize'][
          'right'] - xcrop
      hcrop = props['android.sensor.info.preCorrectionActiveArraySize'][
          'bottom'] - ycrop
      assert wfull >= wcrop >= 0
      assert hfull >= hcrop >= 0
      assert wfull - wcrop >= xcrop >= 0
      assert hfull - hcrop >= ycrop >= 0
      if w == wfull and h == hfull:
        # Crop needed; extract the center region.
        img = img[ycrop:ycrop + hcrop, xcrop:xcrop + wcrop]
        w = wcrop
        h = hcrop
      elif w == wcrop and h == hcrop:
        logging.debug('Image is already cropped.No cropping needed.')
        # pylint: disable=pointless-statement
        None
      else:
        raise error_util.CameraItsError('Invalid image size metadata')
    # Separate the image planes.
    imgs = [
        img[::2].reshape(w * h // 2)[::2].reshape(h // 2, w // 2, 1),
        img[::2].reshape(w * h // 2)[1::2].reshape(h // 2, w // 2, 1),
        img[1::2].reshape(w * h // 2)[::2].reshape(h // 2, w // 2, 1),
        img[1::2].reshape(w * h // 2)[1::2].reshape(h // 2, w // 2, 1)
    ]
    idxs = get_canonical_cfa_order(props)
    return [imgs[i] for i in idxs]
  elif cap['format'] == 'rawStats':
    assert props is not None
    white_level = float(props['android.sensor.info.whiteLevel'])
    # pylint: disable=unused-variable
    mean_image, var_image = unpack_rawstats_capture(cap)
    idxs = get_canonical_cfa_order(props)
    return [mean_image[:, :, i] / white_level for i in idxs]
  else:
    raise error_util.CameraItsError('Invalid format %s' % (cap['format']))


def convert_raw_to_rgb_image(r_plane, gr_plane, gb_plane, b_plane, props,
                             cap_res):
  """Convert a Bayer raw-16 image to an RGB image.

  Includes some extremely rudimentary demosaicking and color processing
  operations; the output of this function shouldn't be used for any image
  quality analysis.

  Args:
   r_plane:
   gr_plane:
   gb_plane:
   b_plane: Numpy arrays for each color plane
            in the Bayer image, with pixels in the [0.0, 1.0] range.
   props: Camera properties object.
   cap_res: Capture result (metadata) object.

  Returns:
    RGB float-3 image array, with pixel values in [0.0, 1.0]
  """
    # Values required for the RAW to RGB conversion.
  assert props is not None
  white_level = float(props['android.sensor.info.whiteLevel'])
  black_levels = props['android.sensor.blackLevelPattern']
  gains = cap_res['android.colorCorrection.gains']
  ccm = cap_res['android.colorCorrection.transform']

  # Reorder black levels and gains to R,Gr,Gb,B, to match the order
  # of the planes.
  black_levels = [get_black_level(i, props, cap_res) for i in range(4)]
  gains = get_gains_in_canonical_order(props, gains)

  # Convert CCM from rational to float, as numpy arrays.
  ccm = numpy.array(capture_request_utils.rational_to_float(ccm)).reshape(3, 3)

  # Need to scale the image back to the full [0,1] range after subtracting
  # the black level from each pixel.
  scale = white_level / (white_level - max(black_levels))

  # Three-channel black levels, normalized to [0,1] by white_level.
  black_levels = numpy.array(
      [b / white_level for b in [black_levels[i] for i in [0, 1, 3]]])

  # Three-channel gains.
  gains = numpy.array([gains[i] for i in [0, 1, 3]])

  h, w = r_plane.shape[:2]
  img = numpy.dstack([r_plane, (gr_plane + gb_plane) / 2.0, b_plane])
  img = (((img.reshape(h, w, 3) - black_levels) * scale) * gains).clip(0.0, 1.0)
  img = numpy.dot(img.reshape(w * h, 3), ccm.T).reshape(h, w, 3).clip(0.0, 1.0)
  return img


def convert_y8_to_rgb_image(y_plane, w, h):
  """Convert a Y 8-bit image to an RGB image.

  Args:
    y_plane: The packed 8-bit Y plane.
    w: The width of the image.
    h: The height of the image.

  Returns:
    RGB float-3 image array, with pixel values in [0.0, 1.0].
  """
  y3 = numpy.dstack([y_plane, y_plane, y_plane])
  rgb = numpy.empty([h, w, 3], dtype=numpy.uint8)
  rgb.reshape(w * h * 3)[:] = y3.reshape(w * h * 3)[:]
  return rgb.astype(numpy.float32) / 255.0


def write_image(img, fname, apply_gamma=False):
  """Save a float-3 numpy array image to a file.

  Supported formats: PNG, JPEG, and others; see PIL docs for more.

  Image can be 3-channel, which is interpreted as RGB, or can be 1-channel,
  which is greyscale.

  Can optionally specify that the image should be gamma-encoded prior to
  writing it out; this should be done if the image contains linear pixel
  values, to make the image look "normal".

  Args:
   img: Numpy image array data.
   fname: Path of file to save to; the extension specifies the format.
   apply_gamma: (Optional) apply gamma to the image prior to writing it.
  """
  if apply_gamma:
    img = apply_lut_to_image(img, DEFAULT_GAMMA_LUT)
  (h, w, chans) = img.shape
  if chans == 3:
    Image.fromarray((img * 255.0).astype(numpy.uint8), 'RGB').save(fname)
  elif chans == 1:
    img3 = (img * 255.0).astype(numpy.uint8).repeat(3).reshape(h, w, 3)
    Image.fromarray(img3, 'RGB').save(fname)
  else:
    raise error_util.CameraItsError('Unsupported image type')


def read_image(fname):
  """Read image function to match write_image() above."""
  return Image.open(fname)


def apply_lut_to_image(img, lut):
  """Applies a LUT to every pixel in a float image array.

  Internally converts to a 16b integer image, since the LUT can work with up
  to 16b->16b mappings (i.e. values in the range [0,65535]). The lut can also
  have fewer than 65536 entries, however it must be sized as a power of 2
  (and for smaller luts, the scale must match the bitdepth).

  For a 16b lut of 65536 entries, the operation performed is:

  lut[r * 65535] / 65535 -> r'
  lut[g * 65535] / 65535 -> g'
  lut[b * 65535] / 65535 -> b'

  For a 10b lut of 1024 entries, the operation becomes:

  lut[r * 1023] / 1023 -> r'
  lut[g * 1023] / 1023 -> g'
  lut[b * 1023] / 1023 -> b'

  Args:
    img: Numpy float image array, with pixel values in [0,1].
    lut: Numpy table encoding a LUT, mapping 16b integer values.

  Returns:
    Float image array after applying LUT to each pixel.
  """
  n = len(lut)
  if n <= 0 or n > MAX_LUT_SIZE or (n & (n - 1)) != 0:
    raise error_util.CameraItsError('Invalid arg LUT size: %d' % (n))
  m = float(n - 1)
  return (lut[(img * m).astype(numpy.uint16)] / m).astype(numpy.float32)


def get_gains_in_canonical_order(props, gains):
  """Reorders the gains tuple to the canonical R,Gr,Gb,B order.

  Args:
    props: Camera properties object.
    gains: List of 4 values, in R,G_even,G_odd,B order.

  Returns:
    List of gains values, in R,Gr,Gb,B order.
  """
  cfa_pat = props['android.sensor.info.colorFilterArrangement']
  if cfa_pat in [0, 1]:
    # RGGB or GRBG, so G_even is Gr
    return gains
  elif cfa_pat in [2, 3]:
    # GBRG or BGGR, so G_even is Gb
    return [gains[0], gains[2], gains[1], gains[3]]
  else:
    raise error_util.CameraItsError('Not supported')


def get_black_level(chan, props, cap_res=None):
  """Return the black level to use for a given capture.

  Uses a dynamic value from the capture result if available, else falls back
  to the static global value in the camera characteristics.

  Args:
    chan: The channel index, in canonical order (R, Gr, Gb, B).
    props: The camera properties object.
    cap_res: A capture result object.

  Returns:
    The black level value for the specified channel.
  """
  if (cap_res is not None and
      'android.sensor.dynamicBlackLevel' in cap_res and
      cap_res['android.sensor.dynamicBlackLevel'] is not None):
    black_levels = cap_res['android.sensor.dynamicBlackLevel']
  else:
    black_levels = props['android.sensor.blackLevelPattern']
  idxs = get_canonical_cfa_order(props)
  ordered_black_levels = [black_levels[i] for i in idxs]
  return ordered_black_levels[chan]


def get_canonical_cfa_order(props):
  """Returns a mapping to the standard order R,Gr,Gb,B.

  Returns a mapping from the Bayer 2x2 top-left grid in the CFA to the standard
  order R,Gr,Gb,B.

  Args:
    props: Camera properties object.

  Returns:
     List of 4 integers, corresponding to the positions in the 2x2 top-
     left Bayer grid of R,Gr,Gb,B, where the 2x2 grid is labeled as
     0,1,2,3 in row major order.
  """
    # Note that raw streams aren't croppable, so the cropRegion doesn't need
    # to be considered when determining the top-left pixel color.
  cfa_pat = props['android.sensor.info.colorFilterArrangement']
  if cfa_pat == 0:
    # RGGB
    return [0, 1, 2, 3]
  elif cfa_pat == 1:
    # GRBG
    return [1, 0, 3, 2]
  elif cfa_pat == 2:
    # GBRG
    return [2, 3, 0, 1]
  elif cfa_pat == 3:
    # BGGR
    return [3, 2, 1, 0]
  else:
    raise error_util.CameraItsError('Not supported')


def unpack_rawstats_capture(cap):
  """Unpack a rawStats capture to the mean and variance images.

  Args:
    cap: A capture object as returned by its_session_utils.do_capture.

  Returns:
    Tuple (mean_image var_image) of float-4 images, with non-normalized
    pixel values computed from the RAW16 images on the device
  """
  assert cap['format'] == 'rawStats'
  w = cap['width']
  h = cap['height']
  img = numpy.ndarray(shape=(2 * h * w * 4,), dtype='<f', buffer=cap['data'])
  analysis_image = img.reshape((2, h, w, 4))
  mean_image = analysis_image[0, :, :, :].reshape(h, w, 4)
  var_image = analysis_image[1, :, :, :].reshape(h, w, 4)
  return mean_image, var_image


def get_image_patch(img, xnorm, ynorm, wnorm, hnorm):
  """Get a patch (tile) of an image.

  Args:
   img: Numpy float image array, with pixel values in [0,1].
   xnorm:
   ynorm:
   wnorm:
   hnorm: Normalized (in [0,1]) coords for the tile.

  Returns:
     Numpy float image array of the patch.
  """
  hfull = img.shape[0]
  wfull = img.shape[1]
  xtile = int(math.ceil(xnorm * wfull))
  ytile = int(math.ceil(ynorm * hfull))
  wtile = int(math.floor(wnorm * wfull))
  htile = int(math.floor(hnorm * hfull))
  if len(img.shape) == 2:
    return img[ytile:ytile + htile, xtile:xtile + wtile].copy()
  else:
    return img[ytile:ytile + htile, xtile:xtile + wtile, :].copy()


def compute_image_means(img):
  """Calculate the mean of each color channel in the image.

  Args:
    img: Numpy float image array, with pixel values in [0,1].

  Returns:
     A list of mean values, one per color channel in the image.
  """
  means = []
  chans = img.shape[2]
  for i in range(chans):
    means.append(numpy.mean(img[:, :, i], dtype=numpy.float64))
  return means


def compute_image_variances(img):
  """Calculate the variance of each color channel in the image.

  Args:
    img: Numpy float image array, with pixel values in [0,1].

  Returns:
    A list of variance values, one per color channel in the image.
  """
  variances = []
  chans = img.shape[2]
  for i in range(chans):
    variances.append(numpy.var(img[:, :, i], dtype=numpy.float64))
  return variances


def compute_image_sharpness(img):
  """Calculate the sharpness of input image.

  Args:
    img: numpy float RGB/luma image array, with pixel values in [0,1].

  Returns:
    Sharpness estimation value based on the average of gradient magnitude.
    Larger value means the image is sharper.
  """
  chans = img.shape[2]
  assert chans == 1 or chans == 3
  if chans == 1:
    luma = img[:, :, 0]
  else:
    luma = convert_rgb_to_grayscale(img)
  gy, gx = numpy.gradient(luma)
  return numpy.average(numpy.sqrt(gy*gy + gx*gx))


def compute_image_max_gradients(img):
  """Calculate the maximum gradient of each color channel in the image.

  Args:
    img: Numpy float image array, with pixel values in [0,1].

  Returns:
    A list of gradient max values, one per color channel in the image.
  """
  grads = []
  chans = img.shape[2]
  for i in range(chans):
    grads.append(numpy.amax(numpy.gradient(img[:, :, i])))
  return grads


def compute_image_snrs(img):
  """Calculate the SNR (dB) of each color channel in the image.

  Args:
    img: Numpy float image array, with pixel values in [0,1].

  Returns:
    A list of SNR values in dB, one per color channel in the image.
  """
  means = compute_image_means(img)
  variances = compute_image_variances(img)
  std_devs = [math.sqrt(v) for v in variances]
  snrs = [20 * math.log10(m/s) for m, s in zip(means, std_devs)]
  return snrs


def convert_rgb_to_grayscale(img):
  """Convert and 3-D array RGB image to grayscale image.

  Args:
    img: numpy float RGB/luma image array, with pixel values in [0,1].

  Returns:
    2-D grayscale image
  """
  assert img.shape[2] == 3, 'Not an RGB image'
  return 0.299*img[:, :, 0] + 0.587*img[:, :, 1] + 0.114*img[:, :, 2]


def normalize_img(img):
  """Normalize the image values to between 0 and 1.

  Args:
    img: 2-D numpy array of image values
  Returns:
    Normalized image
  """
  return (img - numpy.amin(img))/(numpy.amax(img) - numpy.amin(img))


def rotate_img_per_argv(img):
  """Rotate an image 180 degrees if "rotate" is in argv.

  Args:
    img: 2-D numpy array of image values
  Returns:
    Rotated image
  """
  img_out = img
  if 'rotate180' in sys.argv:
    img_out = numpy.fliplr(numpy.flipud(img_out))
  return img_out


def chart_located_per_argv(chart_loc_arg):
  """Determine if chart already located outside of test.

  If chart info provided, return location and size. If not, return None.
  Args:
   chart_loc_arg: chart_loc arg value.

  Returns:
    chart_loc:  float converted xnorm,ynorm,wnorm,hnorm,scale from argv
    text.argv is of form 'chart_loc=0.45,0.45,0.1,0.1,1.0'
  """
  if chart_loc_arg:
    return map(float, chart_loc_arg)
  return None, None, None, None, None


def stationary_lens_cap(cam, req, fmt):
  """Take up to NUM_TRYS caps and save the 1st one with lens stationary.

  Args:
   cam: open device session
   req: capture request
   fmt: format for capture

  Returns:
    capture
  """
  tries = 0
  done = False
  reqs = [req] * NUM_FRAMES
  while not done:
    logging.debug('Waiting for lens to move to correct location.')
    cap = cam.do_capture(reqs, fmt)
    done = (cap[NUM_FRAMES - 1]['metadata']['android.lens.state'] == 0)
    logging.debug('status: %s', done)
    tries += 1
    if tries == NUM_TRIES:
      raise error_util.CameraItsError('Cannot settle lens after %d tries!' %
                                      tries)
  return cap[NUM_FRAMES - 1]


def compute_image_rms_difference(rgb_x, rgb_y):
  """Calculate the RMS difference between 2 RBG images.

  Args:
    rgb_x: image array
    rgb_y: image array

  Returns:
    rms_diff
  """
  len_rgb_x = len(rgb_x)
  assert len(rgb_y) == len_rgb_x, 'The images have different number of planes.'
  return math.sqrt(sum([pow(rgb_x[i] - rgb_y[i], 2.0)
                        for i in range(len_rgb_x)]) / len_rgb_x)


class ImageProcessingUtilsTest(unittest.TestCase):
  """Unit tests for this module."""
  _SQRT_2 = numpy.sqrt(2)
  _YUV_FULL_SCALE = 1023

  def test_unpack_raw10_image(self):
    """Unit test for unpack_raw10_image.

    RAW10 bit packing format
            bit 7   bit 6   bit 5   bit 4   bit 3   bit 2   bit 1   bit 0
    Byte 0: P0[9]   P0[8]   P0[7]   P0[6]   P0[5]   P0[4]   P0[3]   P0[2]
    Byte 1: P1[9]   P1[8]   P1[7]   P1[6]   P1[5]   P1[4]   P1[3]   P1[2]
    Byte 2: P2[9]   P2[8]   P2[7]   P2[6]   P2[5]   P2[4]   P2[3]   P2[2]
    Byte 3: P3[9]   P3[8]   P3[7]   P3[6]   P3[5]   P3[4]   P3[3]   P3[2]
    Byte 4: P3[1]   P3[0]   P2[1]   P2[0]   P1[1]   P1[0]   P0[1]   P0[0]
    """
    # Test using a random 4x4 10-bit image
    img_w, img_h = 4, 4
    check_list = random.sample(range(0, 1024), img_h*img_w)
    img_check = numpy.array(check_list).reshape(img_h, img_w)

    # Pack bits
    for row_start in range(0, len(check_list), img_w):
      msbs = []
      lsbs = ''
      for pixel in range(img_w):
        val = numpy.binary_repr(check_list[row_start+pixel], 10)
        msbs.append(int(val[:8], base=2))
        lsbs = val[8:] + lsbs
      packed = msbs
      packed.append(int(lsbs, base=2))
      chunk_raw10 = numpy.array(packed, dtype='uint8').reshape(1, 5)
      if row_start == 0:
        img_raw10 = chunk_raw10
      else:
        img_raw10 = numpy.vstack((img_raw10, chunk_raw10))

    # Unpack and check against original
    self.assertTrue(numpy.array_equal(unpack_raw10_image(img_raw10),
                                      img_check))

  def test_compute_image_sharpness(self):
    """Unit test for compute_img_sharpness.

    Tests by using PNG of ISO12233 chart and blurring intentionally.
    'sharpness' should drop off by sqrt(2) for 2x blur of image.

    We do one level of initial blur as PNG image is not perfect.
    """
    blur_levels = [2, 4, 8]
    chart_file = os.path.join(TEST_IMG_DIR, 'ISO12233.png')
    chart = cv2.imread(chart_file, cv2.IMREAD_ANYDEPTH)
    white_level = numpy.amax(chart).astype(float)
    sharpness = {}
    for blur in blur_levels:
      chart_blurred = cv2.blur(chart, (blur, blur))
      chart_blurred = chart_blurred[:, :, numpy.newaxis]
      sharpness[blur] = self._YUV_FULL_SCALE * compute_image_sharpness(
          chart_blurred / white_level)

    for i in range(len(blur_levels)-1):
      self.assertTrue(numpy.isclose(
          sharpness[blur_levels[i]]/sharpness[blur_levels[i+1]], self._SQRT_2,
          atol=0.1))

  def test_apply_lut_to_image(self):
    """Unit test for apply_lut_to_image.

    Test by using a canned set of values on a 1x1 pixel image.
    The look-up table should double the value of the index: lut[x] = x*2
    """
    ref_image = [0.1, 0.2, 0.3]
    lut_max = 65536
    lut = numpy.array([i*2 for i in range(lut_max)])
    x = numpy.array(ref_image).reshape(1, 1, 3)
    y = apply_lut_to_image(x, lut).reshape(3).tolist()
    y_ref = [i*2 for i in ref_image]
    self.assertTrue(numpy.allclose(y, y_ref, atol=1/lut_max))


if __name__ == '__main__':
  unittest.main()
