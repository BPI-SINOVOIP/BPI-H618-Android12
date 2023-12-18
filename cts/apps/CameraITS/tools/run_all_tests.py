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

import json
import logging
import os
import os.path
import subprocess
import sys
import tempfile
import time
import yaml

import capture_request_utils
import camera_properties_utils
import image_processing_utils
import its_session_utils

import numpy as np

YAML_FILE_DIR = os.environ['CAMERA_ITS_TOP']
CONFIG_FILE = os.path.join(YAML_FILE_DIR, 'config.yml')
TEST_KEY_TABLET = 'tablet'
TEST_KEY_SENSOR_FUSION = 'sensor_fusion'
LOAD_SCENE_DELAY = 1  # seconds
ACTIVITY_START_WAIT = 1.5  # seconds

NUM_TRIES = 2
RESULT_PASS = 'PASS'
RESULT_FAIL = 'FAIL'
RESULT_NOT_EXECUTED = 'NOT_EXECUTED'
RESULT_KEY = 'result'
SUMMARY_KEY = 'summary'
RESULT_VALUES = {RESULT_PASS, RESULT_FAIL, RESULT_NOT_EXECUTED}
ITS_TEST_ACTIVITY = 'com.android.cts.verifier/.camera.its.ItsTestActivity'
ACTION_ITS_RESULT = 'com.android.cts.verifier.camera.its.ACTION_ITS_RESULT'
EXTRA_VERSION = 'camera.its.extra.VERSION'
CURRENT_ITS_VERSION = '1.0'  # version number to sync with CtsVerifier
EXTRA_CAMERA_ID = 'camera.its.extra.CAMERA_ID'
EXTRA_RESULTS = 'camera.its.extra.RESULTS'
TIME_KEY_START = 'start'
TIME_KEY_END = 'end'
VALID_CONTROLLERS = ('arduino', 'canakit')
_INT_STR_DICT = {'11': '1_1', '12': '1_2'}  # recover replaced '_' in scene def

# All possible scenes
# Notes on scene names:
#   scene*_1/2/... are same scene split to load balance run times for scenes
#   scene*_a/b/... are similar scenes that share one or more tests
_ALL_SCENES = [
    'scene0', 'scene1_1', 'scene1_2', 'scene2_a', 'scene2_b', 'scene2_c',
    'scene2_d', 'scene2_e', 'scene3', 'scene4', 'scene5', 'scene6',
    'sensor_fusion', 'scene_change'
]

# Scenes that can be automated through tablet display
_AUTO_SCENES = [
    'scene0', 'scene1_1', 'scene1_2', 'scene2_a', 'scene2_b', 'scene2_c',
    'scene2_d', 'scene2_e', 'scene3', 'scene4', 'scene6', 'scene_change'
]

# Scenes that are logically grouped and can be called as group
_GROUPED_SCENES = {
        'scene1': ['scene1_1', 'scene1_2'],
        'scene2': ['scene2_a', 'scene2_b', 'scene2_c', 'scene2_d', 'scene2_e']
}

# Scenes that have to be run manually regardless of configuration
_MANUAL_SCENES = ['scene5']

# Scene requirements for manual testing.
_SCENE_REQ = {
    'scene0': None,
    'scene1_1': 'A grey card covering at least the middle 30% of the scene',
    'scene1_2': 'A grey card covering at least the middle 30% of the scene',
    'scene2_a': 'The picture with 3 faces in tests/scene2_a/scene2_a.png',
    'scene2_b': 'The picture with 3 faces in tests/scene2_b/scene2_b.png',
    'scene2_c': 'The picture with 3 faces in tests/scene2_c/scene2_c.png',
    'scene2_d': 'The picture with 3 faces in tests/scene2_d/scene2_d.png',
    'scene2_e': 'The picture with 3 faces in tests/scene2_e/scene2_e.png',
    'scene3': 'The ISO12233 chart',
    'scene4': 'A test chart of a circle covering at least the middle 50% of '
              'the scene. See tests/scene4/scene4.png',
    'scene5': 'Capture images with a diffuser attached to the camera. '
              'See CameraITS.pdf section 2.3.4 for more details',
    'scene6': 'A grid of black circles on a white background. '
              'See tests/scene6/scene6.png',
    'sensor_fusion': 'A checkerboard pattern for phone to rotate in front of '
                     'in tests/sensor_fusion/checkerboard.pdf\n'
                     'See tests/sensor_fusion/SensorFusion.pdf for detailed '
                     'instructions.\nNote that this test will be skipped '
                     'on devices not supporting REALTIME camera timestamp.',
    'scene_change': 'The picture with 3 faces in tests/scene2_e/scene2_e.png',
}


SUB_CAMERA_TESTS = {
    'scene0': [
        'test_burst_capture',
        'test_jitter',
        'test_metadata',
        'test_read_write',
        'test_sensor_events',
        'test_solid_color_test_pattern',
        'test_unified_timestamps',
    ],
    'scene1_1': [
        'test_burst_sameness_manual',
        'test_dng_noise_model',
        'test_exposure',
        'test_linearity',
    ],
    'scene1_2': [
        'test_raw_exposure',
        'test_raw_sensitivity',
        'test_yuv_plus_raw',
    ],
    'scene2_a': [
        'test_faces',
        'test_num_faces',
    ],
    'scene4': [
        'test_aspect_ratio_and_crop',
    ],
    'sensor_fusion': [
        'test_sensor_fusion',
    ],
}

_DST_SCENE_DIR = '/mnt/sdcard/Download/'
MOBLY_TEST_SUMMARY_TXT_FILE = 'test_mobly_summary.txt'


def run(cmd):
  """Replaces os.system call, while hiding stdout+stderr messages."""
  with open(os.devnull, 'wb') as devnull:
    subprocess.check_call(cmd.split(), stdout=devnull, stderr=subprocess.STDOUT)


def report_result(device_id, camera_id, results):
  """Sends a pass/fail result to the device, via an intent.

  Args:
   device_id: The ID string of the device to report the results to.
   camera_id: The ID string of the camera for which to report pass/fail.
   results: a dictionary contains all ITS scenes as key and result/summary of
            current ITS run. See test_report_result unit test for an example.
  """
  adb = f'adb -s {device_id}'

  # Start ItsTestActivity to receive test results
  cmd = f'{adb} shell am start {ITS_TEST_ACTIVITY} --activity-brought-to-front'
  run(cmd)
  time.sleep(ACTIVITY_START_WAIT)

  # Validate/process results argument
  for scene in results:
    if RESULT_KEY not in results[scene]:
      raise ValueError(f'ITS result not found for {scene}')
    if results[scene][RESULT_KEY] not in RESULT_VALUES:
      raise ValueError(f'Unknown ITS result for {scene}: {results[RESULT_KEY]}')
    if SUMMARY_KEY in results[scene]:
      device_summary_path = f'/sdcard/its_camera{camera_id}_{scene}.txt'
      run('%s push %s %s' %
          (adb, results[scene][SUMMARY_KEY], device_summary_path))
      results[scene][SUMMARY_KEY] = device_summary_path

  json_results = json.dumps(results)
  cmd = (f"{adb} shell am broadcast -a {ACTION_ITS_RESULT} --es {EXTRA_VERSION}"
         f" {CURRENT_ITS_VERSION} --es {EXTRA_CAMERA_ID} {camera_id} --es "
         f"{EXTRA_RESULTS} \'{json_results}\'")
  if len(cmd) > 8000:
    logging.info('ITS command string might be too long! len:%s', len(cmd))
  run(cmd)


def load_scenes_on_tablet(scene, tablet_id):
  """Copies scenes onto the tablet before running the tests.

  Args:
    scene: Name of the scene to copy image files.
    tablet_id: adb id of tablet
  """
  logging.info('Copying files to tablet: %s', tablet_id)
  scene_dir = os.listdir(
      os.path.join(os.environ['CAMERA_ITS_TOP'], 'tests', scene))
  for file_name in scene_dir:
    if file_name.endswith('.png'):
      src_scene_file = os.path.join(os.environ['CAMERA_ITS_TOP'], 'tests',
                                    scene, file_name)
      cmd = f'adb -s {tablet_id} push {src_scene_file} {_DST_SCENE_DIR}'
      subprocess.Popen(cmd.split())
  time.sleep(LOAD_SCENE_DELAY)
  logging.info('Finished copying files to tablet.')


def check_manual_scenes(device_id, camera_id, scene, out_path):
  """Halt run to change scenes.

  Args:
    device_id: id of device
    camera_id: id of camera
    scene: Name of the scene to copy image files.
    out_path: output file location
  """
  with its_session_utils.ItsSession(
      device_id=device_id,
      camera_id=camera_id) as cam:
    props = cam.get_camera_properties()
    props = cam.override_with_hidden_physical_camera_props(props)

    while True:
      input(f'\n Press <ENTER> after positioning camera {camera_id} with '
            f'{scene}.\n The scene setup should be: \n  {_SCENE_REQ[scene]}\n')
      # Converge 3A prior to capture
      if scene == 'scene5':
        cam.do_3a(do_af=False, lock_ae=camera_properties_utils.ae_lock(props),
                  lock_awb=camera_properties_utils.awb_lock(props))
      else:
        cam.do_3a()
      req, fmt = capture_request_utils.get_fastest_auto_capture_settings(props)
      logging.info('Capturing an image to check the test scene')
      cap = cam.do_capture(req, fmt)
      img = image_processing_utils.convert_capture_to_rgb_image(cap)
      img_name = os.path.join(out_path, f'test_{scene}.jpg')
      logging.info('Please check scene setup in %s', img_name)
      image_processing_utils.write_image(img, img_name)
      choice = input('Is the image okay for ITS {scene}? (Y/N)').lower()
      if choice == 'y':
        break


def get_config_file_contents():
  """Read the config file contents from a YML file.

  Args:
    None

  Returns:
    config_file_contents: a dict read from config.yml
  """
  with open(CONFIG_FILE) as file:
    config_file_contents = yaml.load(file, yaml.FullLoader)
  return config_file_contents


def get_test_params(config_file_contents):
  """Reads the config file parameters.

  Args:
    config_file_contents: dict read from config.yml file

  Returns:
    dict of test parameters
  """
  test_params = None
  for _, j in config_file_contents.items():
    for datadict in j:
      test_params = datadict.get('TestParams')
  return test_params


def get_device_serial_number(device, config_file_contents):
  """Returns the serial number of the device with label from the config file.

  The config file contains TestBeds dictionary which contains Controllers and
  Android Device dicts.The two devices used by the test per box are listed
  here labels dut and tablet. Parse through the nested TestBeds dict to get
  the Android device details.

  Args:
    device: String device label as specified in config file.dut/tablet
    config_file_contents: dict read from config.yml file
  """

  for _, j in config_file_contents.items():
    for datadict in j:
      android_device_contents = datadict.get('Controllers')
      for device_dict in android_device_contents.get('AndroidDevice'):
        for _, label in device_dict.items():
          if label == 'tablet':
            tablet_device_id = device_dict.get('serial')
          if label == 'dut':
            dut_device_id = device_dict.get('serial')
  if device == 'tablet':
    return tablet_device_id
  else:
    return dut_device_id


def get_updated_yml_file(yml_file_contents):
  """Create a new yml file and write the testbed contents in it.

  This testbed file is per box and contains all the parameters and
  device id used by the mobly tests.

  Args:
   yml_file_contents: Data to write in yml file.

  Returns:
    Updated yml file contents.
  """
  os.chmod(YAML_FILE_DIR, 0o755)
  _, new_yaml_file = tempfile.mkstemp(
      suffix='.yml', prefix='config_', dir=YAML_FILE_DIR)
  with open(new_yaml_file, 'w') as f:
    yaml.dump(yml_file_contents, stream=f, default_flow_style=False)
  new_yaml_file_name = os.path.basename(new_yaml_file)
  return new_yaml_file_name


def enable_external_storage(device_id):
  """Override apk mode to allow write to external storage.

  Args:
    device_id: Serial number of the device.

  """
  cmd = (f'adb -s {device_id} shell appops '
         'set com.android.cts.verifier MANAGE_EXTERNAL_STORAGE allow')
  run(cmd)


def main():
  """Run all the Camera ITS automated tests.

    Script should be run from the top-level CameraITS directory.

    Command line arguments:
        camera:  the camera(s) to be tested. Use comma to separate multiple
                 camera Ids. Ex: "camera=0,1" or "camera=1"
        scenes:  the test scene(s) to be executed. Use comma to separate
                 multiple scenes. Ex: "scenes=scene0,scene1_1" or
                 "scenes=0,1_1,sensor_fusion" (sceneX can be abbreviated by X
                 where X is scene name minus 'scene')
  """
  logging.basicConfig(level=logging.INFO)
  # Make output directories to hold the generated files.
  topdir = tempfile.mkdtemp(prefix='CameraITS_')
  subprocess.call(['chmod', 'g+rx', topdir])
  logging.info('Saving output files to: %s', topdir)

  scenes = []
  camera_id_combos = []
  # Override camera & scenes with cmd line values if available
  for s in list(sys.argv[1:]):
    if 'scenes=' in s:
      scenes = s.split('=')[1].split(',')
    elif 'camera=' in s:
      camera_id_combos = s.split('=')[1].split(',')

  # Read config file and extract relevant TestBed
  config_file_contents = get_config_file_contents()
  for i in config_file_contents['TestBeds']:
    if scenes == ['sensor_fusion']:
      if TEST_KEY_SENSOR_FUSION not in i['Name'].lower():
        config_file_contents['TestBeds'].remove(i)
    else:
      if TEST_KEY_SENSOR_FUSION in i['Name'].lower():
        config_file_contents['TestBeds'].remove(i)

  # Get test parameters from config file
  test_params_content = get_test_params(config_file_contents)
  if not camera_id_combos:
    camera_id_combos = str(test_params_content['camera']).split(',')
  if not scenes:
    scenes = str(test_params_content['scene']).split(',')
    scenes = [_INT_STR_DICT.get(n, n) for n in scenes]  # recover '1_1' & '1_2'

  device_id = get_device_serial_number('dut', config_file_contents)
  # Enable external storage on DUT to send summary report to CtsVerifier.apk
  enable_external_storage(device_id)

  config_file_test_key = config_file_contents['TestBeds'][0]['Name'].lower()
  if TEST_KEY_TABLET in config_file_test_key:
    tablet_id = get_device_serial_number('tablet', config_file_contents)
  else:
    tablet_id = None

  testing_sensor_fusion_with_controller = False
  if TEST_KEY_SENSOR_FUSION in config_file_test_key:
    if test_params_content['rotator_cntl'].lower() in VALID_CONTROLLERS:
      testing_sensor_fusion_with_controller = True

  # Prepend 'scene' if not specified at cmd line
  for i, s in enumerate(scenes):
    if (not s.startswith('scene') and
        not s.startswith(('sensor_fusion', '<scene-name>'))):
      scenes[i] = f'scene{s}'

  # Expand GROUPED_SCENES and remove any duplicates
  scenes = [_GROUPED_SCENES[s] if s in _GROUPED_SCENES else s for s in scenes]
  scenes = np.hstack(scenes).tolist()
  scenes = sorted(set(scenes), key=scenes.index)

  logging.info('Running ITS on device: %s, camera(s): %s, scene(s): %s',
               device_id, camera_id_combos, scenes)

  # Determine if manual run
  if tablet_id is not None and not set(scenes).intersection(_MANUAL_SCENES):
    auto_scene_switch = True
  else:
    auto_scene_switch = False
    logging.info('Manual testing: no tablet defined or testing scene5.')

  for camera_id in camera_id_combos:
    test_params_content['camera'] = camera_id
    results = {}

    # Run through all scenes if user does not supply one and config file doesn't
    # have specific scene name listed.
    if its_session_utils.SUB_CAMERA_SEPARATOR in camera_id:
      possible_scenes = list(SUB_CAMERA_TESTS.keys())
      if auto_scene_switch:
        possible_scenes.remove('sensor_fusion')
    else:
      possible_scenes = _AUTO_SCENES if auto_scene_switch else _ALL_SCENES

    if '<scene-name>' in scenes:
      per_camera_scenes = possible_scenes
    else:
      # Validate user input scene names
      per_camera_scenes = []
      for s in scenes:
        if s in possible_scenes:
          per_camera_scenes.append(s)
      if not per_camera_scenes:
        raise ValueError('No valid scene specified for this camera.')

    logging.info('camera: %s, scene(s): %s', camera_id, per_camera_scenes)
    for s in _ALL_SCENES:
      results[s] = {RESULT_KEY: RESULT_NOT_EXECUTED}
    # A subdir in topdir will be created for each camera_id. All scene test
    # output logs for each camera id will be stored in this subdir.
    # This output log path is a mobly param : LogPath
    cam_id_string = 'cam_id_%s' % (
        camera_id.replace(its_session_utils.SUB_CAMERA_SEPARATOR, '_'))
    mobly_output_logs_path = os.path.join(topdir, cam_id_string)
    os.mkdir(mobly_output_logs_path)
    tot_pass = 0
    for s in per_camera_scenes:
      test_params_content['scene'] = s
      results[s]['TEST_STATUS'] = []

      # unit is millisecond for execution time record in CtsVerifier
      scene_start_time = int(round(time.time() * 1000))
      scene_test_summary = f'Cam{camera_id} {s}' + '\n'
      mobly_scene_output_logs_path = os.path.join(mobly_output_logs_path, s)

      if auto_scene_switch:
        # Copy scene images onto the tablet
        if s not in ['scene0']:
          load_scenes_on_tablet(s, tablet_id)
      else:
        # Check manual scenes for correctness
        if s not in ['scene0'] and not testing_sensor_fusion_with_controller:
          check_manual_scenes(device_id, camera_id, s, mobly_output_logs_path)

      scene_test_list = []
      config_file_contents['TestBeds'][0]['TestParams'] = test_params_content
      # Add the MoblyParams to config.yml file with the path to store camera_id
      # test results. This is a separate dict other than TestBeds.
      mobly_params_dict = {
          'MoblyParams': {
              'LogPath': mobly_scene_output_logs_path
          }
      }
      config_file_contents.update(mobly_params_dict)
      logging.debug('Final config file contents: %s', config_file_contents)
      new_yml_file_name = get_updated_yml_file(config_file_contents)
      logging.info('Using %s as temporary config yml file', new_yml_file_name)
      if camera_id.rfind(its_session_utils.SUB_CAMERA_SEPARATOR) == -1:
        scene_dir = os.listdir(
            os.path.join(os.environ['CAMERA_ITS_TOP'], 'tests', s))
        for file_name in scene_dir:
          if file_name.endswith('.py') and 'test' in file_name:
            scene_test_list.append(file_name)
      else:  # sub-camera
        if SUB_CAMERA_TESTS.get(s):
          scene_test_list = [f'{test}.py' for test in SUB_CAMERA_TESTS[s]]
        else:
          scene_test_list = []
      scene_test_list.sort()

      # Run tests for scene
      logging.info('Running tests for %s with camera %s', s, camera_id)
      num_pass = 0
      num_skip = 0
      num_not_mandated_fail = 0
      num_fail = 0
      for test in scene_test_list:
        # Handle repeated test
        if 'tests/' in test:
          cmd = [
              'python3',
              os.path.join(os.environ['CAMERA_ITS_TOP'], test), '-c',
              '%s' % new_yml_file_name
          ]
        else:
          cmd = [
              'python3',
              os.path.join(os.environ['CAMERA_ITS_TOP'], 'tests', s, test),
              '-c',
              '%s' % new_yml_file_name
          ]
        for num_try in range(NUM_TRIES):
          # pylint: disable=subprocess-run-check
          with open(MOBLY_TEST_SUMMARY_TXT_FILE, 'w') as fp:
            output = subprocess.run(cmd, stdout=fp)
          # pylint: enable=subprocess-run-check

          # Parse mobly logs to determine SKIP, NOT_YET_MANDATED, and
          # socket FAILs.
          with open(MOBLY_TEST_SUMMARY_TXT_FILE, 'r') as file:
            test_code = output.returncode
            test_failed = False
            test_skipped = False
            test_not_yet_mandated = False
            line = file.read()
            if 'Test skipped' in line:
              return_string = 'SKIP '
              num_skip += 1
              test_skipped = True
              break

            if 'Not yet mandated test' in line:
              return_string = 'FAIL*'
              num_not_mandated_fail += 1
              test_not_yet_mandated = True
              break

            if test_code == 0 and not test_skipped:
              return_string = 'PASS '
              num_pass += 1
              break

            if test_code == 1 and not test_not_yet_mandated:
              return_string = 'FAIL '
              if 'Problem with socket' in line and num_try != NUM_TRIES-1:
                logging.info('Retry %s/%s', s, test)
              else:
                num_fail += 1
                test_failed = True
                break
            os.remove(MOBLY_TEST_SUMMARY_TXT_FILE)
        logging.info('%s %s/%s', return_string, s, test)
        test_name = test.split('/')[-1].split('.')[0]
        results[s]['TEST_STATUS'].append({'test':test_name,'status':return_string.strip()})
        msg_short = '%s %s' % (return_string, test)
        scene_test_summary += msg_short + '\n'

      # unit is millisecond for execution time record in CtsVerifier
      scene_end_time = int(round(time.time() * 1000))
      skip_string = ''
      tot_tests = len(scene_test_list)
      if num_skip > 0:
        skipstr = f",{num_skip} test{'s' if num_skip > 1 else ''} skipped"
      test_result = '%d / %d tests passed (%.1f%%)%s' % (
          num_pass + num_not_mandated_fail, len(scene_test_list) - num_skip,
          100.0 * float(num_pass + num_not_mandated_fail) /
          (len(scene_test_list) - num_skip)
          if len(scene_test_list) != num_skip else 100.0, skip_string)
      logging.info(test_result)
      if num_not_mandated_fail > 0:
        logging.info('(*) %s not_yet_mandated tests failed',
                     num_not_mandated_fail)

      tot_pass += num_pass
      logging.info('scene tests: %s, Total tests passed: %s', tot_tests,
                   tot_pass)
      if tot_tests > 0:
        logging.info('%s compatibility score: %.f/100\n',
                     s, 100 * num_pass / tot_tests)
        scene_test_summary_path = os.path.join(mobly_scene_output_logs_path,
                                               'scene_test_summary.txt')
        with open(scene_test_summary_path, 'w') as f:
          f.write(scene_test_summary)
        results[s][RESULT_KEY] = (RESULT_PASS if num_fail == 0 else RESULT_FAIL)
        results[s][SUMMARY_KEY] = scene_test_summary_path
        results[s][TIME_KEY_START] = scene_start_time
        results[s][TIME_KEY_END] = scene_end_time
      else:
        logging.info('%s compatibility score: 0/100\n')

      # Delete temporary yml file after scene run.
      new_yaml_file_path = os.path.join(YAML_FILE_DIR, new_yml_file_name)
      os.remove(new_yaml_file_path)

    # Log results per camera
    logging.info('Reporting camera %s ITS results to CtsVerifier', camera_id)
    report_result(device_id, camera_id, results)

  logging.info('Test execution completed.')

  # Power down tablet
  cmd = f'adb -s {tablet_id} shell input keyevent KEYCODE_POWER'
  subprocess.Popen(cmd.split())

if __name__ == '__main__':
  main()
