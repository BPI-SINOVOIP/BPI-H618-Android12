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
"""Utility functions to form an ItsSession and perform various camera actions.
"""


import collections
import json
import logging
import math
import os
import socket
import subprocess
import sys
import time
import unicodedata
import unittest

import numpy

import camera_properties_utils
import capture_request_utils
import error_util
import image_processing_utils
import opencv_processing_utils

LOAD_SCENE_DELAY_SEC = 3
SUB_CAMERA_SEPARATOR = '.'
_VALIDATE_LIGHTING_PATCH_H = 0.05
_VALIDATE_LIGHTING_PATCH_W = 0.05
_VALIDATE_LIGHTING_REGIONS = {
    'top-left': (0, 0),
    'top-right': (0, 1-_VALIDATE_LIGHTING_PATCH_H),
    'bottom-left': (1-_VALIDATE_LIGHTING_PATCH_W, 0),
    'bottom-right': (1-_VALIDATE_LIGHTING_PATCH_W,
                     1-_VALIDATE_LIGHTING_PATCH_H),
}
_VALIDATE_LIGHTING_THRESH = 0.05  # Determined empirically from scene[1:6] tests


class ItsSession(object):
  """Controls a device over adb to run ITS scripts.

    The script importing this module (on the host machine) prepares JSON
    objects encoding CaptureRequests, specifying sets of parameters to use
    when capturing an image using the Camera2 APIs. This class encapsulates
    sending the requests to the device, monitoring the device's progress, and
    copying the resultant captures back to the host machine when done. TCP
    forwarded over adb is the transport mechanism used.

    The device must have CtsVerifier.apk installed.

    Attributes:
        sock: The open socket.
  """

  # Open a connection to localhost:<host_port>, forwarded to port 6000 on the
  # device. <host_port> is determined at run-time to support multiple
  # connected devices.
  IPADDR = '127.0.0.1'
  REMOTE_PORT = 6000
  BUFFER_SIZE = 4096

  # LOCK_PORT is used as a mutex lock to protect the list of forwarded ports
  # among all processes. The script assumes LOCK_PORT is available and will
  # try to use ports between CLIENT_PORT_START and
  # CLIENT_PORT_START+MAX_NUM_PORTS-1 on host for ITS sessions.
  CLIENT_PORT_START = 6000
  MAX_NUM_PORTS = 100
  LOCK_PORT = CLIENT_PORT_START + MAX_NUM_PORTS

  # Seconds timeout on each socket operation.
  SOCK_TIMEOUT = 20.0
  # Additional timeout in seconds when ITS service is doing more complicated
  # operations, for example: issuing warmup requests before actual capture.
  EXTRA_SOCK_TIMEOUT = 5.0

  PACKAGE = 'com.android.cts.verifier.camera.its'
  INTENT_START = 'com.android.cts.verifier.camera.its.START'

  # This string must be in sync with ItsService. Updated when interface
  # between script and ItsService is changed.
  ITS_SERVICE_VERSION = '1.0'

  SEC_TO_NSEC = 1000*1000*1000.0
  adb = 'adb -d'

  # Predefine camera props. Save props extracted from the function,
  # "get_camera_properties".
  props = None

  IMAGE_FORMAT_LIST_1 = [
      'jpegImage', 'rawImage', 'raw10Image', 'raw12Image', 'rawStatsImage',
      'dngImage', 'y8Image'
  ]

  IMAGE_FORMAT_LIST_2 = [
      'jpegImage', 'rawImage', 'raw10Image', 'raw12Image', 'rawStatsImage',
      'yuvImage'
  ]

  CAP_JPEG = {'format': 'jpeg'}
  CAP_RAW = {'format': 'raw'}
  CAP_YUV = {'format': 'yuv'}
  CAP_RAW_YUV = [{'format': 'raw'}, {'format': 'yuv'}]

  def __init_socket_port(self):
    """Initialize the socket port for the host to forward requests to the device.

    This method assumes localhost's LOCK_PORT is available and will try to
    use ports between CLIENT_PORT_START and CLIENT_PORT_START+MAX_NUM_PORTS-1
    """
    num_retries = 100
    retry_wait_time_sec = 0.05

    # Bind a socket to use as mutex lock
    socket_lock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    for i in range(num_retries):
      try:
        socket_lock.bind((ItsSession.IPADDR, ItsSession.LOCK_PORT))
        break
      except (socket.error, socket.timeout):
        if i == num_retries - 1:
          raise error_util.CameraItsError(self._device_id,
                                          'socket lock returns error')
        else:
          time.sleep(retry_wait_time_sec)

    # Check if a port is already assigned to the device.
    command = 'adb forward --list'
    proc = subprocess.Popen(command.split(), stdout=subprocess.PIPE)
    # pylint: disable=unused-variable
    output, error = proc.communicate()
    port = None
    used_ports = []
    for line  in output.decode('utf-8').split(os.linesep):
      # each line should be formatted as:
      # "<device_id> tcp:<host_port> tcp:<remote_port>"
      forward_info = line.split()
      if len(forward_info) >= 3 and len(
          forward_info[1]) > 4 and forward_info[1][:4] == 'tcp:' and len(
              forward_info[2]) > 4 and forward_info[2][:4] == 'tcp:':
        local_p = int(forward_info[1][4:])
        remote_p = int(forward_info[2][4:])
        if forward_info[
            0] == self._device_id and remote_p == ItsSession.REMOTE_PORT:
          port = local_p
          break
        else:
          used_ports.append(local_p)

      # Find the first available port if no port is assigned to the device.
    if port is None:
      for p in range(ItsSession.CLIENT_PORT_START,
                     ItsSession.CLIENT_PORT_START + ItsSession.MAX_NUM_PORTS):
        if self.check_port_availability(p, used_ports):
          port = p
          break

    if port is None:
      raise error_util.CameraItsError(self._device_id,
                                      ' cannot find an available ' + 'port')

    # Release the socket as mutex unlock
    socket_lock.close()

    # Connect to the socket
    self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.sock.connect((self.IPADDR, port))
    self.sock.settimeout(self.SOCK_TIMEOUT)

  def check_port_availability(self, check_port, used_ports):
    """Check if the port is available or not.

    Args:
      check_port: Port to check for availability
      used_ports: List of used ports

    Returns:
     True if the given port is available and can be assigned to the device.
    """
    if check_port not in used_ports:
      # Try to run "adb forward" with the port
      command = '%s forward tcp:%d tcp:%d' % \
                       (self.adb, check_port, self.REMOTE_PORT)
      proc = subprocess.Popen(
          command.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
      error = proc.communicate()[1]

      # Check if there is no error
      if error is None or error.find('error'.encode()) < 0:
        return True
      else:
        return False

  def __wait_for_service(self):
    """Wait for ItsService to be ready and reboot the device if needed.

    This also includes the optional reboot handling: if the user
    provides a "reboot" or "reboot=N" arg, then reboot the device,
    waiting for N seconds (default 30) before returning.
    """

    for s in sys.argv[1:]:
      if s[:6] == 'reboot':
        duration = 30
        if len(s) > 7 and s[6] == '=':
          duration = int(s[7:])
        logging.debug('Rebooting device')
        _run('%s reboot' % (self.adb))
        _run('%s wait-for-device' % (self.adb))
        time.sleep(duration)
        logging.debug('Reboot complete')

    # Flush logcat so following code won't be misled by previous
    # 'ItsService ready' log.
    _run('%s logcat -c' % (self.adb))
    time.sleep(1)

    _run('%s shell am force-stop --user 0 %s' % (self.adb, self.PACKAGE))
    _run(('%s shell am start-foreground-service --user 0 -t text/plain '
          '-a %s') % (self.adb, self.INTENT_START))

    # Wait until the socket is ready to accept a connection.
    proc = subprocess.Popen(
        self.adb.split() + ['logcat'], stdout=subprocess.PIPE)
    logcat = proc.stdout
    while True:
      line = logcat.readline().strip()
      if line.find(b'ItsService ready') >= 0:
        break
    proc.kill()
    proc.communicate()

  def __init__(self, device_id=None, camera_id=None, hidden_physical_id=None):
    self._camera_id = camera_id
    self._device_id = device_id
    self._hidden_physical_id = hidden_physical_id

    # Initialize device id and adb command.
    self.adb = 'adb -s ' + self._device_id
    self.__wait_for_service()
    self.__init_socket_port()

  def __enter__(self):
    self.__close_camera()
    self.__open_camera()
    return self

  def __exit__(self, exec_type, exec_value, exec_traceback):
    if hasattr(self, 'sock') and self.sock:
      self.__close_camera()
      self.sock.close()
    return False

  def override_with_hidden_physical_camera_props(self, props):
    """Check that it is a valid sub-camera backing the logical camera.

    If current session is for a hidden physical camera, check that it is a valid
    sub-camera backing the logical camera, override self.props, and return the
    characteristics of sub-camera. Otherwise, return "props" directly.

    Args:
     props: Camera properties object.

    Returns:
     The properties of the hidden physical camera if possible.
    """
    if self._hidden_physical_id:
      if not camera_properties_utils.logical_multi_camera(props):
        raise AssertionError(f'{self._camera_id} is not a logical multi-camera')
      physical_ids = camera_properties_utils.logical_multi_camera_physical_ids(
          props)
      if self._hidden_physical_id not in physical_ids:
        raise AssertionError(f'{self._hidden_physical_id} is not a hidden '
                             f'sub-camera of {self._camera_id}')
      props = self.get_camera_properties_by_id(self._hidden_physical_id)
      self.props = props
    return props

  def get_camera_properties(self):
    """Get the camera properties object for the device.

    Returns:
     The Python dictionary object for the CameraProperties object.
    """
    cmd = {}
    cmd['cmdName'] = 'getCameraProperties'
    self.sock.send(json.dumps(cmd).encode() + '\n'.encode())
    data, _ = self.__read_response_from_socket()
    if data['tag'] != 'cameraProperties':
      raise error_util.CameraItsError('Invalid command response')
    self.props = data['objValue']['cameraProperties']
    return data['objValue']['cameraProperties']

  def get_camera_properties_by_id(self, camera_id):
    """Get the camera properties object for device with camera_id.

    Args:
     camera_id: The ID string of the camera

    Returns:
     The Python dictionary object for the CameraProperties object. Empty
     if no such device exists.
    """
    cmd = {}
    cmd['cmdName'] = 'getCameraPropertiesById'
    cmd['cameraId'] = camera_id
    self.sock.send(json.dumps(cmd).encode() + '\n'.encode())
    data, _ = self.__read_response_from_socket()
    if data['tag'] != 'cameraProperties':
      raise error_util.CameraItsError('Invalid command response')
    return data['objValue']['cameraProperties']

  def __read_response_from_socket(self):
    """Reads a line (newline-terminated) string serialization of JSON object.

    Returns:
     Deserialized json obj.
    """
    chars = []
    while not chars or chars[-1] != '\n':
      ch = self.sock.recv(1).decode('utf-8')
      if not ch:
        # Socket was probably closed; otherwise don't get empty strings
        raise error_util.CameraItsError('Problem with socket on device side')
      chars.append(ch)
    line = ''.join(chars)
    jobj = json.loads(line)
    # Optionally read a binary buffer of a fixed size.
    buf = None
    if 'bufValueSize' in jobj:
      n = jobj['bufValueSize']
      buf = bytearray(n)
      view = memoryview(buf)
      while n > 0:
        nbytes = self.sock.recv_into(view, n)
        view = view[nbytes:]
        n -= nbytes
      buf = numpy.frombuffer(buf, dtype=numpy.uint8)
    return jobj, buf

  def __open_camera(self):
    """Get the camera ID to open if it is an argument as a single camera.

    This allows passing camera=# to individual tests at command line
    and camera=#,#,# or an no camera argv with tools/run_all_tests.py.
    In case the camera is a logical multi-camera, to run ITS on the
    hidden physical sub-camera, pass camera=[logical ID]:[physical ID]
    to an individual test at the command line, and same applies to multiple
    camera IDs for tools/run_all_tests.py: camera=#,#:#,#:#,#
    """
    if not self._camera_id:
      self._camera_id = 0
      for s in sys.argv[1:]:
        if s[:7] == 'camera=' and len(s) > 7:
          camera_ids = s[7:].split(',')
          camera_id_combos = parse_camera_ids(camera_ids)
          if len(camera_id_combos) == 1:
            self._camera_id = camera_id_combos[0].id
            self._hidden_physical_id = camera_id_combos[0].sub_id

    logging.debug('Opening camera: %s', self._camera_id)
    cmd = {'cmdName': 'open', 'cameraId': self._camera_id}
    self.sock.send(json.dumps(cmd).encode() + '\n'.encode())
    data, _ = self.__read_response_from_socket()
    if data['tag'] != 'cameraOpened':
      raise error_util.CameraItsError('Invalid command response')

  def __close_camera(self):
    cmd = {'cmdName': 'close'}
    self.sock.send(json.dumps(cmd).encode() + '\n'.encode())
    data, _ = self.__read_response_from_socket()
    if data['tag'] != 'cameraClosed':
      raise error_util.CameraItsError('Invalid command response')

  def get_sensors(self):
    """Get all sensors on the device.

    Returns:
       A Python dictionary that returns keys and booleans for each sensor.
    """
    cmd = {}
    cmd['cmdName'] = 'checkSensorExistence'
    self.sock.send(json.dumps(cmd).encode() + '\n'.encode())
    data, _ = self.__read_response_from_socket()
    if data['tag'] != 'sensorExistence':
      raise error_util.CameraItsError('Invalid response for command: %s' %
                                      cmd['cmdName'])
    return data['objValue']

  def start_sensor_events(self):
    """Start collecting sensor events on the device.

    See get_sensor_events for more info.

    Returns:
       Nothing.
    """
    cmd = {}
    cmd['cmdName'] = 'startSensorEvents'
    self.sock.send(json.dumps(cmd).encode() + '\n'.encode())
    data, _ = self.__read_response_from_socket()
    if data['tag'] != 'sensorEventsStarted':
      raise error_util.CameraItsError('Invalid response for command: %s' %
                                      cmd['cmdName'])

  def get_sensor_events(self):
    """Get a trace of all sensor events on the device.

        The trace starts when the start_sensor_events function is called. If
        the test runs for a long time after this call, then the device's
        internal memory can fill up. Calling get_sensor_events gets all events
        from the device, and then stops the device from collecting events and
        clears the internal buffer; to start again, the start_sensor_events
        call must be used again.

        Events from the accelerometer, compass, and gyro are returned; each
        has a timestamp and x,y,z values.

        Note that sensor events are only produced if the device isn't in its
        standby mode (i.e.) if the screen is on.

    Returns:
            A Python dictionary with three keys ("accel", "mag", "gyro") each
            of which maps to a list of objects containing "time","x","y","z"
            keys.
    """
    cmd = {}
    cmd['cmdName'] = 'getSensorEvents'
    self.sock.send(json.dumps(cmd).encode() + '\n'.encode())
    timeout = self.SOCK_TIMEOUT + self.EXTRA_SOCK_TIMEOUT
    self.sock.settimeout(timeout)
    data, _ = self.__read_response_from_socket()
    if data['tag'] != 'sensorEvents':
      raise error_util.CameraItsError('Invalid response for command: %s ' %
                                      cmd['cmdName'])
    self.sock.settimeout(self.SOCK_TIMEOUT)
    return data['objValue']

  def do_capture(self,
                 cap_request,
                 out_surfaces=None,
                 reprocess_format=None,
                 repeat_request=None):
    """Issue capture request(s), and read back the image(s) and metadata.

    The main top-level function for capturing one or more images using the
    device. Captures a single image if cap_request is a single object, and
    captures a burst if it is a list of objects.

    The optional repeat_request field can be used to assign a repeating
    request list ran in background for 3 seconds to warm up the capturing
    pipeline before start capturing. The repeat_requests will be ran on a
    640x480 YUV surface without sending any data back. The caller needs to
    make sure the stream configuration defined by out_surfaces and
    repeat_request are valid or do_capture may fail because device does not
    support such stream configuration.

    The out_surfaces field can specify the width(s), height(s), and
    format(s) of the captured image. The formats may be "yuv", "jpeg",
    "dng", "raw", "raw10", "raw12", "rawStats" or "y8". The default is a
    YUV420 frame ("yuv") corresponding to a full sensor frame.

    Optionally the out_surfaces field can specify physical camera id(s) if
    the current camera device is a logical multi-camera. The physical camera
    id must refer to a physical camera backing this logical camera device.

    Note that one or more surfaces can be specified, allowing a capture to
    request images back in multiple formats (e.g.) raw+yuv, raw+jpeg,
    yuv+jpeg, raw+yuv+jpeg. If the size is omitted for a surface, the
    default is the largest resolution available for the format of that
    surface. At most one output surface can be specified for a given format,
    and raw+dng, raw10+dng, and raw+raw10 are not supported as combinations.

    If reprocess_format is not None, for each request, an intermediate
    buffer of the given reprocess_format will be captured from camera and
    the intermediate buffer will be reprocessed to the output surfaces. The
    following settings will be turned off when capturing the intermediate
    buffer and will be applied when reprocessing the intermediate buffer.
    1. android.noiseReduction.mode
    2. android.edge.mode
    3. android.reprocess.effectiveExposureFactor

    Supported reprocess format are "yuv" and "private". Supported output
    surface formats when reprocessing is enabled are "yuv" and "jpeg".

    Example of a single capture request:

    {
     "android.sensor.exposureTime": 100*1000*1000,
     "android.sensor.sensitivity": 100
    }

    Example of a list of capture requests:
    [
     {
       "android.sensor.exposureTime": 100*1000*1000,
       "android.sensor.sensitivity": 100
     },
    {
      "android.sensor.exposureTime": 100*1000*1000,
       "android.sensor.sensitivity": 200
     }
    ]

    Example of output surface specifications:
    {
     "width": 640,
     "height": 480,
     "format": "yuv"
    }
    [
     {
       "format": "jpeg"
     },
     {
       "format": "raw"
     }
    ]

    The following variables defined in this class are shortcuts for
    specifying one or more formats where each output is the full size for
    that format; they can be used as values for the out_surfaces arguments:

    CAP_RAW
    CAP_DNG
    CAP_YUV
    CAP_JPEG
    CAP_RAW_YUV
    CAP_DNG_YUV
    CAP_RAW_JPEG
    CAP_DNG_JPEG
    CAP_YUV_JPEG
    CAP_RAW_YUV_JPEG
    CAP_DNG_YUV_JPEG

    If multiple formats are specified, then this function returns multiple
    capture objects, one for each requested format. If multiple formats and
    multiple captures (i.e. a burst) are specified, then this function
    returns multiple lists of capture objects. In both cases, the order of
    the returned objects matches the order of the requested formats in the
    out_surfaces parameter. For example:

    yuv_cap = do_capture(req1)
    yuv_cap = do_capture(req1,yuv_fmt)
    yuv_cap, raw_cap = do_capture(req1, [yuv_fmt,raw_fmt])
    yuv_caps = do_capture([req1,req2], yuv_fmt)
    yuv_caps, raw_caps = do_capture([req1,req2], [yuv_fmt,raw_fmt])

    The "rawStats" format processes the raw image and returns a new image
    of statistics from the raw image. The format takes additional keys,
    "gridWidth" and "gridHeight" which are size of grid cells in a 2D grid
    of the raw image. For each grid cell, the mean and variance of each raw
    channel is computed, and the do_capture call returns two 4-element float
    images of dimensions (rawWidth / gridWidth, rawHeight / gridHeight),
    concatenated back-to-back, where the first image contains the 4-channel
    means and the second contains the 4-channel variances. Note that only
    pixels in the active array crop region are used; pixels outside this
    region (for example optical black rows) are cropped out before the
    gridding and statistics computation is performed.

    For the rawStats format, if the gridWidth is not provided then the raw
    image width is used as the default, and similarly for gridHeight. With
    this, the following is an example of a output description that computes
    the mean and variance across each image row:
    {
      "gridHeight": 1,
      "format": "rawStats"
    }

    Args:
      cap_request: The Python dict/list specifying the capture(s), which will be
        converted to JSON and sent to the device.
      out_surfaces: (Optional) specifications of the output image formats and
        sizes to use for each capture.
      reprocess_format: (Optional) The reprocessing format. If not
        None,reprocessing will be enabled.
      repeat_request: Repeating request list.

    Returns:
      An object, list of objects, or list of lists of objects, where each
      object contains the following fields:
      * data: the image data as a numpy array of bytes.
      * width: the width of the captured image.
      * height: the height of the captured image.
      * format: image the format, in [
                        "yuv","jpeg","raw","raw10","raw12","rawStats","dng"].
      * metadata: the capture result object (Python dictionary).
    """
    cmd = {}
    if reprocess_format is not None:
      if repeat_request is not None:
        raise error_util.CameraItsError(
            'repeating request + reprocessing is not supported')
      cmd['cmdName'] = 'doReprocessCapture'
      cmd['reprocessFormat'] = reprocess_format
    else:
      cmd['cmdName'] = 'doCapture'

    if repeat_request is None:
      cmd['repeatRequests'] = []
    elif not isinstance(repeat_request, list):
      cmd['repeatRequests'] = [repeat_request]
    else:
      cmd['repeatRequests'] = repeat_request

    if not isinstance(cap_request, list):
      cmd['captureRequests'] = [cap_request]
    else:
      cmd['captureRequests'] = cap_request

    if out_surfaces is not None:
      if not isinstance(out_surfaces, list):
        cmd['outputSurfaces'] = [out_surfaces]
      else:
        cmd['outputSurfaces'] = out_surfaces
      formats = [
          c['format'] if 'format' in c else 'yuv' for c in cmd['outputSurfaces']
      ]
      formats = [s if s != 'jpg' else 'jpeg' for s in formats]
    else:
      max_yuv_size = capture_request_utils.get_available_output_sizes(
          'yuv', self.props)[0]
      formats = ['yuv']
      cmd['outputSurfaces'] = [{
          'format': 'yuv',
          'width': max_yuv_size[0],
          'height': max_yuv_size[1]
      }]

    ncap = len(cmd['captureRequests'])
    nsurf = 1 if out_surfaces is None else len(cmd['outputSurfaces'])

    cam_ids = []
    bufs = {}
    yuv_bufs = {}
    for i, s in enumerate(cmd['outputSurfaces']):
      if self._hidden_physical_id:
        s['physicalCamera'] = self._hidden_physical_id

      if 'physicalCamera' in s:
        cam_id = s['physicalCamera']
      else:
        cam_id = self._camera_id

      if cam_id not in cam_ids:
        cam_ids.append(cam_id)
        bufs[cam_id] = {
            'raw': [],
            'raw10': [],
            'raw12': [],
            'rawStats': [],
            'dng': [],
            'jpeg': [],
            'y8': []
        }

    for cam_id in cam_ids:
       # Only allow yuv output to multiple targets
      if cam_id == self._camera_id:
        yuv_surfaces = [
            s for s in cmd['outputSurfaces']
            if s['format'] == 'yuv' and 'physicalCamera' not in s
        ]
        formats_for_id = [
            s['format']
            for s in cmd['outputSurfaces']
            if 'physicalCamera' not in s
        ]
      else:
        yuv_surfaces = [
            s for s in cmd['outputSurfaces'] if s['format'] == 'yuv' and
            'physicalCamera' in s and s['physicalCamera'] == cam_id
        ]
        formats_for_id = [
            s['format']
            for s in cmd['outputSurfaces']
            if 'physicalCamera' in s and s['physicalCamera'] == cam_id
        ]

      n_yuv = len(yuv_surfaces)
      # Compute the buffer size of YUV targets
      yuv_maxsize_1d = 0
      for s in yuv_surfaces:
        if ('width' not in s and 'height' not in s):
          if self.props is None:
            raise error_util.CameraItsError('Camera props are unavailable')
          yuv_maxsize_2d = capture_request_utils.get_available_output_sizes(
              'yuv', self.props)[0]
          # YUV420 size = 1.5 bytes per pixel
          yuv_maxsize_1d = (yuv_maxsize_2d[0] * yuv_maxsize_2d[1] * 3) // 2
          break
      yuv_sizes = [
          (c['width'] * c['height'] * 3) // 2
          if 'width' in c and 'height' in c else yuv_maxsize_1d
          for c in yuv_surfaces
      ]
      # Currently we don't pass enough metadta from ItsService to distinguish
      # different yuv stream of same buffer size
      if len(yuv_sizes) != len(set(yuv_sizes)):
        raise error_util.CameraItsError(
            'ITS does not support yuv outputs of same buffer size')
      if len(formats_for_id) > len(set(formats_for_id)):
        if n_yuv != len(formats_for_id) - len(set(formats_for_id)) + 1:
          raise error_util.CameraItsError('Duplicate format requested')

      yuv_bufs[cam_id] = {size: [] for size in yuv_sizes}

    raw_formats = 0
    raw_formats += 1 if 'dng' in formats else 0
    raw_formats += 1 if 'raw' in formats else 0
    raw_formats += 1 if 'raw10' in formats else 0
    raw_formats += 1 if 'raw12' in formats else 0
    raw_formats += 1 if 'rawStats' in formats else 0
    if raw_formats > 1:
      raise error_util.CameraItsError('Different raw formats not supported')

    # Detect long exposure time and set timeout accordingly
    longest_exp_time = 0
    for req in cmd['captureRequests']:
      if 'android.sensor.exposureTime' in req and req[
          'android.sensor.exposureTime'] > longest_exp_time:
        longest_exp_time = req['android.sensor.exposureTime']

    extended_timeout = longest_exp_time // self.SEC_TO_NSEC + self.SOCK_TIMEOUT
    if repeat_request:
      extended_timeout += self.EXTRA_SOCK_TIMEOUT
    self.sock.settimeout(extended_timeout)

    logging.debug('Capturing %d frame%s with %d format%s [%s]', ncap,
                  's' if ncap > 1 else '', nsurf, 's' if nsurf > 1 else '',
                  ','.join(formats))
    self.sock.send(json.dumps(cmd).encode() + '\n'.encode())

    # Wait for ncap*nsurf images and ncap metadata responses.
    # Assume that captures come out in the same order as requested in
    # the burst, however individual images of different formats can come
    # out in any order for that capture.
    nbufs = 0
    mds = []
    physical_mds = []
    widths = None
    heights = None
    while nbufs < ncap * nsurf or len(mds) < ncap:
      json_obj, buf = self.__read_response_from_socket()
      if json_obj['tag'] in ItsSession.IMAGE_FORMAT_LIST_1 and buf is not None:
        fmt = json_obj['tag'][:-5]
        bufs[self._camera_id][fmt].append(buf)
        nbufs += 1
      elif json_obj['tag'] == 'yuvImage':
        buf_size = numpy.product(buf.shape)
        yuv_bufs[self._camera_id][buf_size].append(buf)
        nbufs += 1
      elif json_obj['tag'] == 'captureResults':
        mds.append(json_obj['objValue']['captureResult'])
        physical_mds.append(json_obj['objValue']['physicalResults'])
        outputs = json_obj['objValue']['outputs']
        widths = [out['width'] for out in outputs]
        heights = [out['height'] for out in outputs]
      else:
        tag_string = unicodedata.normalize('NFKD', json_obj['tag']).encode(
            'ascii', 'ignore')
        for x in ItsSession.IMAGE_FORMAT_LIST_2:
          x = bytes(x, encoding='utf-8')
          if tag_string.startswith(x):
            if x == b'yuvImage':
              physical_id = json_obj['tag'][len(x):]
              if physical_id in cam_ids:
                buf_size = numpy.product(buf.shape)
                yuv_bufs[physical_id][buf_size].append(buf)
                nbufs += 1
            else:
              physical_id = json_obj['tag'][len(x):]
              if physical_id in cam_ids:
                fmt = x[:-5].decode('UTF-8')
                bufs[physical_id][fmt].append(buf)
                nbufs += 1
    rets = []
    for j, fmt in enumerate(formats):
      objs = []
      if 'physicalCamera' in cmd['outputSurfaces'][j]:
        cam_id = cmd['outputSurfaces'][j]['physicalCamera']
      else:
        cam_id = self._camera_id

      for i in range(ncap):
        obj = {}
        obj['width'] = widths[j]
        obj['height'] = heights[j]
        obj['format'] = fmt
        if cam_id == self._camera_id:
          obj['metadata'] = mds[i]
        else:
          for physical_md in physical_mds[i]:
            if cam_id in physical_md:
              obj['metadata'] = physical_md[cam_id]
              break

        if fmt == 'yuv':
          buf_size = (widths[j] * heights[j] * 3) // 2
          obj['data'] = yuv_bufs[cam_id][buf_size][i]
        else:
          obj['data'] = bufs[cam_id][fmt][i]
        objs.append(obj)
      rets.append(objs if ncap > 1 else objs[0])
    self.sock.settimeout(self.SOCK_TIMEOUT)
    if len(rets) > 1 or (isinstance(rets[0], dict) and
                         isinstance(cap_request, list)):
      return rets
    else:
      return rets[0]

  def do_vibrate(self, pattern):
    """Cause the device to vibrate to a specific pattern.

    Args:
      pattern: Durations (ms) for which to turn on or off the vibrator.
      The first value indicates the number of milliseconds to wait
      before turning the vibrator on. The next value indicates the
      number of milliseconds for which to keep the vibrator on
      before turning it off. Subsequent values alternate between
      durations in milliseconds to turn the vibrator off or to turn
      the vibrator on.

    Returns:
      Nothing.
    """
    cmd = {}
    cmd['cmdName'] = 'doVibrate'
    cmd['pattern'] = pattern
    self.sock.send(json.dumps(cmd).encode() + '\n'.encode())
    data, _ = self.__read_response_from_socket()
    if data['tag'] != 'vibrationStarted':
      raise error_util.CameraItsError('Invalid response for command: %s' %
                                      cmd['cmdName'])

  def set_audio_restriction(self, mode):
    """Set the audio restriction mode for this camera device.

    Args:
     mode: int; the audio restriction mode. See CameraDevice.java for valid
     value.
    Returns:
     Nothing.
    """
    cmd = {}
    cmd['cmdName'] = 'setAudioRestriction'
    cmd['mode'] = mode
    self.sock.send(json.dumps(cmd).encode() + '\n'.encode())
    data, _ = self.__read_response_from_socket()
    if data['tag'] != 'audioRestrictionSet':
      raise error_util.CameraItsError('Invalid response for command: %s' %
                                      cmd['cmdName'])

  # pylint: disable=dangerous-default-value
  def do_3a(self,
            regions_ae=[[0, 0, 1, 1, 1]],
            regions_awb=[[0, 0, 1, 1, 1]],
            regions_af=[[0, 0, 1, 1, 1]],
            do_ae=True,
            do_awb=True,
            do_af=True,
            lock_ae=False,
            lock_awb=False,
            get_results=False,
            ev_comp=0,
            mono_camera=False):
    """Perform a 3A operation on the device.

    Triggers some or all of AE, AWB, and AF, and returns once they have
    converged. Uses the vendor 3A that is implemented inside the HAL.
    Note: do_awb is always enabled regardless of do_awb flag

    Throws an assertion if 3A fails to converge.

    Args:
      regions_ae: List of weighted AE regions.
      regions_awb: List of weighted AWB regions.
      regions_af: List of weighted AF regions.
      do_ae: Trigger AE and wait for it to converge.
      do_awb: Wait for AWB to converge.
      do_af: Trigger AF and wait for it to converge.
      lock_ae: Request AE lock after convergence, and wait for it.
      lock_awb: Request AWB lock after convergence, and wait for it.
      get_results: Return the 3A results from this function.
      ev_comp: An EV compensation value to use when running AE.
      mono_camera: Boolean for monochrome camera.

      Region format in args:
         Arguments are lists of weighted regions; each weighted region is a
         list of 5 values, [x, y, w, h, wgt], and each argument is a list of
         these 5-value lists. The coordinates are given as normalized
         rectangles (x, y, w, h) specifying the region. For example:
         [[0.0, 0.0, 1.0, 0.5, 5], [0.0, 0.5, 1.0, 0.5, 10]].
         Weights are non-negative integers.

    Returns:
      Five values are returned if get_results is true:
      * AE sensitivity; None if do_ae is False
      * AE exposure time; None if do_ae is False
      * AWB gains (list);
      * AWB transform (list);
      * AF focus position; None if do_af is false
      Otherwise, it returns five None values.
    """
    logging.debug('Running vendor 3A on device')
    cmd = {}
    cmd['cmdName'] = 'do3A'
    cmd['regions'] = {
        'ae': sum(regions_ae, []),
        'awb': sum(regions_awb, []),
        'af': sum(regions_af, [])
    }
    cmd['triggers'] = {'ae': do_ae, 'af': do_af}
    if lock_ae:
      cmd['aeLock'] = True
    if lock_awb:
      cmd['awbLock'] = True
    if ev_comp != 0:
      cmd['evComp'] = ev_comp
    if self._hidden_physical_id:
      cmd['physicalId'] = self._hidden_physical_id
    self.sock.send(json.dumps(cmd).encode() + '\n'.encode())

    # Wait for each specified 3A to converge.
    ae_sens = None
    ae_exp = None
    awb_gains = None
    awb_transform = None
    af_dist = None
    converged = False
    while True:
      data, _ = self.__read_response_from_socket()
      vals = data['strValue'].split()
      if data['tag'] == 'aeResult':
        if do_ae:
          ae_sens, ae_exp = [int(i) for i in vals]
      elif data['tag'] == 'afResult':
        if do_af:
          af_dist = float(vals[0])
      elif data['tag'] == 'awbResult':
        awb_gains = [float(f) for f in vals[:4]]
        awb_transform = [float(f) for f in vals[4:]]
      elif data['tag'] == '3aConverged':
        converged = True
      elif data['tag'] == '3aDone':
        break
      else:
        raise error_util.CameraItsError('Invalid command response')
    if converged and not get_results:
      return None, None, None, None, None
    if (do_ae and ae_sens is None or
        (not mono_camera and do_awb and awb_gains is None) or
        do_af and af_dist is None or not converged):
      raise error_util.CameraItsError('3A failed to converge')
    return ae_sens, ae_exp, awb_gains, awb_transform, af_dist

  def calc_camera_fov(self, props):
    """Determine the camera field of view from internal params.

    Args:
      props: Camera properties object.

    Returns:
      camera_fov: string; field of view for camera.
    """

    focal_ls = props['android.lens.info.availableFocalLengths']
    if len(focal_ls) > 1:
      logging.debug('Doing capture to determine logical camera focal length')
      cap = self.do_capture(capture_request_utils.auto_capture_request())
      focal_l = cap['metadata']['android.lens.focalLength']
    else:
      focal_l = focal_ls[0]

    sensor_size = props['android.sensor.info.physicalSize']
    diag = math.sqrt(sensor_size['height']**2 + sensor_size['width']**2)
    try:
      fov = str(round(2 * math.degrees(math.atan(diag / (2 * focal_l))), 2))
    except ValueError:
      fov = str(0)
    logging.debug('Calculated FoV: %s', fov)
    return fov

  def get_file_name_to_load(self, chart_distance, camera_fov, scene):
    """Get the image to load on the tablet depending on fov and chart_distance.

    Args:
     chart_distance: float; distance in cm from camera of displayed chart
     camera_fov: float; camera field of view.
     scene: String; Scene to be used in the test.

    Returns:
     file_name: file name to display on the tablet.

    """
    chart_scaling = opencv_processing_utils.calc_chart_scaling(
        chart_distance, camera_fov)
    if numpy.isclose(
        chart_scaling,
        opencv_processing_utils.SCALE_RFOV_IN_WFOV_BOX,
        atol=0.01):
      file_name = '%s_%sx_scaled.png' % (
          scene, str(opencv_processing_utils.SCALE_RFOV_IN_WFOV_BOX))
    elif numpy.isclose(
        chart_scaling,
        opencv_processing_utils.SCALE_TELE_IN_WFOV_BOX,
        atol=0.01):
      file_name = '%s_%sx_scaled.png' % (
          scene, str(opencv_processing_utils.SCALE_TELE_IN_WFOV_BOX))
    elif numpy.isclose(
        chart_scaling,
        opencv_processing_utils.SCALE_TELE25_IN_RFOV_BOX,
        atol=0.01):
      file_name = '%s_%sx_scaled.png' % (
          scene, str(opencv_processing_utils.SCALE_TELE25_IN_RFOV_BOX))
    elif numpy.isclose(
        chart_scaling,
        opencv_processing_utils.SCALE_TELE40_IN_RFOV_BOX,
        atol=0.01):
      file_name = '%s_%sx_scaled.png' % (
          scene, str(opencv_processing_utils.SCALE_TELE40_IN_RFOV_BOX))
    elif numpy.isclose(
        chart_scaling,
        opencv_processing_utils.SCALE_TELE_IN_RFOV_BOX,
        atol=0.01):
      file_name = '%s_%sx_scaled.png' % (
          scene, str(opencv_processing_utils.SCALE_TELE_IN_RFOV_BOX))
    else:
      file_name = '%s.png' % scene
    logging.debug('Scene to load: %s', file_name)
    return file_name

  def is_stream_combination_supported(self, out_surfaces):
    """Query whether out_surfaces combination is supported by the camera device.

    This function hooks up to the isSessionConfigurationSupported() camera API
    to query whether a particular stream combination is supported.

    Args:
      out_surfaces: dict; see do_capture() for specifications on out_surfaces

    Returns:
      Boolean
    """
    cmd = {}
    cmd['cmdName'] = 'isStreamCombinationSupported'

    if not isinstance(out_surfaces, list):
      cmd['outputSurfaces'] = [out_surfaces]
    else:
      cmd['outputSurfaces'] = out_surfaces
    formats = [c['format'] if 'format' in c else 'yuv'
               for c in cmd['outputSurfaces']]
    formats = [s if s != 'jpg' else 'jpeg' for s in formats]

    self.sock.send(json.dumps(cmd).encode() + '\n'.encode())

    data, _ = self.__read_response_from_socket()
    if data['tag'] != 'streamCombinationSupport':
      raise error_util.CameraItsError('Failed to query stream combination')

    return data['strValue'] == 'supportedCombination'

  def is_camera_privacy_mode_supported(self):
    """Query whether the mobile device supports camera privacy mode.

    This function checks whether the mobile device has FEATURE_CAMERA_TOGGLE
    feature support, which indicates the camera device can run in privacy mode.

    Returns:
      Boolean
    """
    cmd = {}
    cmd['cmdName'] = 'isCameraPrivacyModeSupported'
    self.sock.send(json.dumps(cmd).encode() + '\n'.encode())

    data, _ = self.__read_response_from_socket()
    if data['tag'] != 'cameraPrivacyModeSupport':
      raise error_util.CameraItsError('Failed to query camera privacy mode'
                                      ' support')
    return data['strValue'] == 'true'

  def is_performance_class_primary_camera(self):
    """Query whether the camera device is an R or S performance class primary camera.

    A primary rear/front facing camera is a camera device with the lowest
    camera Id for that facing.

    Returns:
      Boolean
    """
    cmd = {}
    cmd['cmdName'] = 'isPerformanceClassPrimaryCamera'
    cmd['cameraId'] = self._camera_id
    self.sock.send(json.dumps(cmd).encode() + '\n'.encode())

    data, _ = self.__read_response_from_socket()
    if data['tag'] != 'performanceClassPrimaryCamera':
      raise error_util.CameraItsError('Failed to query performance class '
                                      'primary camera')
    return data['strValue'] == 'true'

  def measure_camera_launch_ms(self):
    """Measure camera launch latency in millisecond, from open to first frame.

    Returns:
      Camera launch latency from camera open to receipt of first frame
    """
    cmd = {}
    cmd['cmdName'] = 'measureCameraLaunchMs'
    cmd['cameraId'] = self._camera_id
    self.sock.send(json.dumps(cmd).encode() + '\n'.encode())

    data, _ = self.__read_response_from_socket()
    if data['tag'] != 'cameraLaunchMs':
      raise error_util.CameraItsError('Failed to measure camera launch latency')
    return float(data['strValue'])

  def measure_camera_1080p_jpeg_capture_ms(self):
    """Measure camera 1080P jpeg capture latency in milliseconds.

    Returns:
      Camera jpeg capture latency in milliseconds
    """
    cmd = {}
    cmd['cmdName'] = 'measureCamera1080pJpegCaptureMs'
    cmd['cameraId'] = self._camera_id
    self.sock.send(json.dumps(cmd).encode() + '\n'.encode())

    data, _ = self.__read_response_from_socket()
    if data['tag'] != 'camera1080pJpegCaptureMs':
      raise error_util.CameraItsError(
          'Failed to measure camera 1080p jpeg capture latency')
    return float(data['strValue'])


def parse_camera_ids(ids):
  """Parse the string of camera IDs into array of CameraIdCombo tuples.

  Args:
   ids: List of camera ids.

  Returns:
   Array of CameraIdCombo
  """
  camera_id_combo = collections.namedtuple('CameraIdCombo', ['id', 'sub_id'])
  id_combos = []
  for one_id in ids:
    one_combo = one_id.split(SUB_CAMERA_SEPARATOR)
    if len(one_combo) == 1:
      id_combos.append(camera_id_combo(one_combo[0], None))
    elif len(one_combo) == 2:
      id_combos.append(camera_id_combo(one_combo[0], one_combo[1]))
    else:
      raise AssertionError('Camera id parameters must be either ID or '
                           f'ID{SUB_CAMERA_SEPARATOR}SUB_ID')
  return id_combos


def _run(cmd):
  """Replacement for os.system, with hiding of stdout+stderr messages.

  Args:
    cmd: Command to be executed in string format.
  """
  with open(os.devnull, 'wb') as devnull:
    subprocess.check_call(cmd.split(), stdout=devnull, stderr=subprocess.STDOUT)


def do_capture_with_latency(cam, req, sync_latency, fmt=None):
  """Helper function to take enough frames to allow sync latency.

  Args:
    cam: camera object
    req: request for camera
    sync_latency: integer number of frames
    fmt: format for the capture
  Returns:
    single capture with the unsettled frames discarded
  """
  caps = cam.do_capture([req]*(sync_latency+1), fmt)
  return caps[-1]


def load_scene(cam, props, scene, tablet, chart_distance):
  """Load the scene for the camera based on the FOV.

  Args:
    cam: camera object
    props: camera properties
    scene: scene to be loaded
    tablet: tablet to load scene on
    chart_distance: distance to tablet
  """
  if not tablet:
    logging.info('Manual run: no tablet to load scene on.')
    return
  # Calculate camera_fov which will determine the image to load on tablet.
  camera_fov = cam.calc_camera_fov(props)
  file_name = cam.get_file_name_to_load(chart_distance, camera_fov, scene)
  logging.debug('Displaying %s on the tablet', file_name)
  # Display the scene on the tablet depending on camera_fov
  tablet.adb.shell(
      'am start -a android.intent.action.VIEW -t image/png '
      f'-d file://mnt/sdcard/Download/{file_name}')
  time.sleep(LOAD_SCENE_DELAY_SEC)
  rfov_camera_in_rfov_box = (
      numpy.isclose(
          chart_distance,
          opencv_processing_utils.CHART_DISTANCE_RFOV, rtol=0.1) and
      opencv_processing_utils.FOV_THRESH_TELE <= float(camera_fov)
      <= opencv_processing_utils.FOV_THRESH_WFOV)
  wfov_camera_in_wfov_box = (
      numpy.isclose(
          chart_distance,
          opencv_processing_utils.CHART_DISTANCE_WFOV, rtol=0.1) and
      float(camera_fov) > opencv_processing_utils.FOV_THRESH_WFOV)
  if rfov_camera_in_rfov_box or wfov_camera_in_wfov_box:
    cam.do_3a()
    cap = cam.do_capture(
        capture_request_utils.auto_capture_request(), cam.CAP_YUV)
    y_plane, _, _ = image_processing_utils.convert_capture_to_planes(cap)
    validate_lighting(y_plane, scene)


def validate_lighting(y_plane, scene):
  """Validates the lighting level in scene corners based on empirical values.

  Args:
    y_plane: Y plane of YUV image
    scene: scene name
  Returns:
    boolean True if lighting validated, else raise AssertionError
  """
  logging.debug('Validating lighting levels.')

  # Test patches from each corner.
  for location, coordinates in _VALIDATE_LIGHTING_REGIONS.items():
    patch = image_processing_utils.get_image_patch(
        y_plane, coordinates[0], coordinates[1],
        _VALIDATE_LIGHTING_PATCH_W, _VALIDATE_LIGHTING_PATCH_H)
    y_mean = image_processing_utils.compute_image_means(patch)[0]
    logging.debug('%s corner Y mean: %.3f', location, y_mean)
    if y_mean > _VALIDATE_LIGHTING_THRESH:
      logging.debug('Lights ON in test rig.')
      return True
  image_processing_utils.write_image(y_plane, f'validate_lighting_{scene}.jpg')
  raise AssertionError('Lights OFF in test rig. Please turn ON and retry.')


def get_build_sdk_version(device_id):
  """Return the int build version of the device."""
  cmd = 'adb -s %s shell getprop ro.build.version.sdk' % device_id
  try:
    build_sdk_version = int(subprocess.check_output(cmd.split()).rstrip())
    logging.debug('Build SDK version: %d', build_sdk_version)
  except (subprocess.CalledProcessError, ValueError):
    raise AssertionError('No build_sdk_version.')
  return build_sdk_version


def get_first_api_level(device_id):
  """Return the int value for the first API level of the device."""
  cmd = 'adb -s %s shell getprop ro.product.first_api_level' % device_id
  try:
    first_api_level = int(subprocess.check_output(cmd.split()).rstrip())
    logging.debug('First API level: %d', first_api_level)
  except (subprocess.CalledProcessError, ValueError):
    logging.error('No first_api_level. Setting to build version.')
    first_api_level = get_build_sdk_version(device_id)
  return first_api_level


class ItsSessionUtilsTests(unittest.TestCase):
  """Run a suite of unit tests on this module."""

  _BRIGHTNESS_CHECKS = (0.0,
                        _VALIDATE_LIGHTING_THRESH-0.01,
                        _VALIDATE_LIGHTING_THRESH,
                        _VALIDATE_LIGHTING_THRESH+0.01,
                        1.0)
  _TEST_IMG_W = 640
  _TEST_IMG_H = 480

  def _generate_test_image(self, brightness):
    """Creates a Y plane array with pixel values of brightness.

    Args:
      brightness: float between [0.0, 1.0]

    Returns:
      Y plane array with elements of value brightness
    """
    test_image = numpy.zeros((self._TEST_IMG_W, self._TEST_IMG_H, 1),
                             dtype=float)
    test_image.fill(brightness)
    return test_image

  def test_validate_lighting(self):
    """Tests validate_lighting() works correctly."""
    # Run with different brightnesses to validate.
    for brightness in self._BRIGHTNESS_CHECKS:
      logging.debug('Testing validate_lighting with brightness %.1f',
                    brightness)
      test_image = self._generate_test_image(brightness)
      print(f'Testing brightness: {brightness}')
      if brightness <= _VALIDATE_LIGHTING_THRESH:
        self.assertRaises(
            AssertionError, validate_lighting, test_image, 'unittest')
      else:
        self.assertTrue(validate_lighting(test_image, 'unittest'),
                        f'image value {brightness} should PASS')


if __name__ == '__main__':
  unittest.main()
