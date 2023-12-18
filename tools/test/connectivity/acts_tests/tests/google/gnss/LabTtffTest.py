#!/usr/bin/env python3
#
#   Copyright 2020 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the 'License');
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an 'AS IS' BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import os
import time
import glob
import errno

from acts import utils
from acts import asserts
from acts import signals
from acts import base_test
from pandas import DataFrame
from collections import namedtuple
from acts.controllers.spectracom_lib import gsg6
from acts.test_utils.gnss import dut_log_test_utils as diaglog
from acts_contrib.test_utils.gnss import gnss_test_utils as gutils
from acts_contrib.test_utils.gnss import gnss_testlog_utils as glogutils

DEVICE_GPSLOG_FOLDER = '/sdcard/Android/data/com.android.gpstool/files/'
GPS_PKG_NAME = 'com.android.gpstool'

class LabTtffTest(base_test.BaseTestClass):

    """ LAB TTFF Tests"""
    GTW_GPSTOOL_APP = 'gtw_gpstool_apk'
    SPECTRACOM_IP_KEY = 'spectracom_ip'
    SPECTRACOM_PORT_KEY = 'spectracom_port'
    SPECTRACOM_FILES_KEY = 'spectracom_files'
    SPECTRACOM_POWER_KEY = 'spectracom_power_level'
    SPIRENT_IP_KEY = 'spirent_ip'
    SPIRENT_SCENARIO = 'sprient_scenario'
    CUSTOM_FILES_KEY = 'custom_files'
    CSTTFF_CRITERIA = 'cs_criteria'
    HSTTFF_CRITERIA = 'hs_criteria'
    WSTTFF_CRITERIA = 'ws_criteria'
    CSTTFF_PECRITERIA = 'cs_ttff_pecriteria'
    HSTTFF_PECRITERIA = 'hs_ttff_pecriteria'
    WSTTFF_PECRITERIA = 'ws_ttff_pecriteria'
    TTFF_ITERATION = 'ttff_iteration'
    SIMULATOR_LOCATION = 'simulator_location'
    DIAG_OPTION = 'diag_option'

    def __init__(self, controllers):
        """ Initializes class attributes. """

        super().__init__(controllers)

        self.dut = None
        self.spectracom = None

    def setup_class(self):
        super().setup_class()

        req_params = [
            self.SPECTRACOM_IP_KEY, self.SPECTRACOM_PORT_KEY,
            self.SPECTRACOM_FILES_KEY, self.SPECTRACOM_POWER_KEY,
            self.CSTTFF_CRITERIA, self.HSTTFF_CRITERIA,
            self.WSTTFF_CRITERIA, self.TTFF_ITERATION,
            self.SIMULATOR_LOCATION, self.DIAG_OPTION
        ]

        for param in req_params:
            if param not in self.user_params:
                self.log.error('Required parameter {} is missing in config '
                               'file.'.format(param))
                raise signals.TestAbortClass(
                    'Required parameter {} is missing in config '
                               'file.'.format(param)) 
        self.dut = self.android_devices[0]
        self.spectracom_ip = self.user_params[self.SPECTRACOM_IP_KEY]
        self.spectracom_port = self.user_params[self.SPECTRACOM_PORT_KEY]
        self.spectracom_file = self.user_params[self.SPECTRACOM_FILES_KEY]
        self.spectracom_power = self.user_params[self.SPECTRACOM_POWER_KEY]
        self.gtw_gpstool_app = self.user_params[self.GTW_GPSTOOL_APP]
        custom_files = self.user_params.get(self.CUSTOM_FILES_KEY, [])
        self.cs_ttff_criteria = self.user_params.get(self.CSTTFF_CRITERIA, [])
        self.hs_ttff_criteria = self.user_params.get(self.HSTTFF_CRITERIA, [])
        self.ws_ttff_criteria = self.user_params.get(self.WSTTFF_CRITERIA, [])
        self.cs_ttff_pecriteria = self.user_params.get(
            self.CSTTFF_PECRITERIA, [])
        self.hs_ttff_pecriteria = self.user_params.get(
            self.HSTTFF_PECRITERIA, [])
        self.ws_ttff_pecriteria = self.user_params.get(
            self.WSTTFF_PECRITERIA, [])
        self.ttff_iteration = self.user_params.get(self.TTFF_ITERATION, [])
        self.simulator_location = self.user_params.get(
            self.SIMULATOR_LOCATION, [])
	self.diag_option = self.user_params.get(self.DIAG_OPTION, [])

        test_type = namedtuple('Type', ['command', 'criteria'])
        self.test_types = {
            'cs': test_type('Cold Start', self.cs_ttff_criteria),
            'ws': test_type('Warm Start', self.ws_ttff_criteria),
            'hs': test_type('Hot Start', self.hs_ttff_criteria)
        }

        # Unpack the rockbottom script or fail class setup if it can't be found
        for file in custom_files:
            if 'rockbottom_' + self.dut.model in file:
                self.rockbottom_script = file
                break
        else:
            raise signals.TestAbortClass(
                'Required rockbottom script is missing.')

    def setup_test(self):

	self.clear_gps_log()
        self.spectracom = gsg6.GSG6(self.spectracom_ip, self.spectracom_port)

        self.spectracom.stop_scenario()
        time.sleep(10)
        self.spectracom.close()

        self.dut_rockbottom()
        utils.set_location_service(self.dut, True)
        gutils.reinstall_package_apk(self.dut, GPS_PKG_NAME,
                                     self.gtw_gpstool_app)
        self.spectracom = gsg6.GSG6(self.spectracom_ip, self.spectracom_port)
        self.spectracom.connect()

    def dut_rockbottom(self):
        """
        Set the dut to rockbottom state

        """
        # The rockbottom script might include a device reboot, so it is
        # necessary to stop SL4A during its execution.
        self.dut.stop_services()
        self.log.info('Executing rockbottom script for ' + self.dut.model)
        os.chmod(self.rockbottom_script, 0o777)
        os.system('{} {}'.format(self.rockbottom_script, self.dut.serial))
        # Make sure the DUT is in root mode after coming back
        self.dut.root_adb()
        # Restart SL4A
        self.dut.start_services()

    def teardown_class(self):
        """ Executed after completing all selected test cases."""
        self.clear_gps_log()
        if self.spectracom:
            self.spectracom.stop_scenario()
            time.sleep(10)
            self.spectracom.close()

    def start_and_set_spectracom_power(self):
        """
        Start spectracom secnario and set power level.

        """

        self.spectracom.start_scenario(self.spectracom_file)
        time.sleep(25)
        self.spectracom.set_power(self.spectracom_power)

    def get_and_verify_ttff(self, mode):
        """Retrieve ttff with designate mode.

            Args:
                mode: A string for identify gnss test mode.
        """
        if mode not in self.test_types:
            raise signals.TestError('Unrecognized mode %s' % mode)
        test_type = self.test_types.get(mode)

        gutils.process_gnss_by_gtw_gpstool(self.dut,
                                           self.test_types['cs'].criteria)
        begin_time = gutils.get_current_epoch_time()
        gutils.start_ttff_by_gtw_gpstool(
            self.dut, ttff_mode=mode,
            iteration=self.ttff_iteration, aid_data=True)
        ttff_data = gutils.process_ttff_by_gtw_gpstool(self.dut, begin_time,
                                                       self.simulator_location)

        gps_log_path = os.path.join(self.log_path, 'GPSLogs')
        self.dut.adb.pull("{} {}".format(DEVICE_GPSLOG_FOLDER, gps_log_path))

        gps_api_log = glob.glob(gps_log_path + '/GPS_API_*.txt')
        ttff_loop_log = glob.glob(gps_log_path + '/GPS_{}_*.txt'.
                                  format(mode.upper()))

        if not gps_api_log and ttff_loop_log:
            raise FileNotFoundError(errno.ENOENT, os.strerror(errno.ENOENT),
                                    gps_log_path)

        df = DataFrame(glogutils.parse_gpstool_ttfflog_to_df(gps_api_log[0]))

        ttff_dict = {}
        for i in ttff_data:
            d = ttff_data[i]._asdict()
            ttff_dict[i] = dict(d)

        ttff_time =[]
        ttff_pe = []
        for i, k in ttff_dict.items():
            ttff_time.append(ttff_dict[i]['ttff_time'])
            ttff_pe.append(ttff_dict[i]['ttff_pe'])
        df['ttff_time'] = ttff_time
        df['ttff_pe'] = ttff_pe
        df.to_json(gps_log_path + '/gps_log.json', orient='table')
        result = gutils.check_ttff_data(
            self.dut,
            ttff_data,
            ttff_mode=test_type.command,
            criteria=test_type.criteria)
        if not result:
            raise signals.TestFailure('%s TTFF fails to reach '
                                      'designated criteria'
                                      % test_type.command)
        return ttff_data

    def verify_pe(self, mode):
        """
        Verify ttff Position Error with designate mode.

        Args:
             mode: A string for identify gnss test mode.
        """

        ffpe_type = namedtuple('Type', ['command', 'pecriteria'])
        ffpe_types = {
            'cs': ffpe_type('Cold Start', self.cs_ttff_pecriteria),
            'ws': ffpe_type('Warm Start', self.ws_ttff_pecriteria),
            'hs': ffpe_type('Hot Start', self.hs_ttff_pecriteria)
        }

        if mode not in self.test_types:
            raise signals.TestError('Unrecognized mode %s' % mode)
        test_type = self.test_types.get(mode)

        ttff_data = self.get_and_verify_ttff(mode)
        result = gutils.check_ttff_pe(
            self.dut,
            ttff_data,
            ttff_mode=test_type.command,
            pecriteria=test_type.pecriteria
        )
        if not result:
            raise signals.TestFailure('%s TTFF fails to reach '
                                      'designated criteria'
                                      % test_type.command)
        return ttff_data

    def clear_gps_log(self):
        """
        Delete the existing GPS GTW Log from DUT.

        """
        self.dut.adb.shell("rm -rf {}".format(DEVICE_GPSLOG_FOLDER))

    def test_gnss_cold_ttff_ffpe(self):

        self.start_and_set_spectracom_power()
        if self.diag_option is "QCOM":
                diaglog.start_diagmdlog_background(self.dut, maskfile=self.maskfile)
        else:
                #start_tbdlog() yet to add for Broadcom
                pass
	self.verify_pe('cs')
        diaglog.stop_background_diagmdlog(self.dut, self.qxdm_log_path, keep_logs=False)

    def test_gnss_warm_ttff_ffpe(self):

        self.start_and_set_spectracom_power()
	if self.diag_option is "QCOM":
	        diaglog.start_diagmdlog_background(self.dut, maskfile=self.maskfile)
	else:
		#start_tbdlog() yet to add for Broadcom
		pass
        self.verify_pe('ws')
        diaglog.stop_background_diagmdlog(self.dut, self.qxdm_log_path, keep_logs=False)

    def test_gnss_hot_ttff_ffpe(self):

        self.start_and_set_spectracom_power()
        if self.diag_option is "QCOM":
                diaglog.start_diagmdlog_background(self.dut, maskfile=self.maskfile)
        else:
                #start_tbdlog() yet to add for Broadcom
                pass
        self.verify_pe('hs')
        diaglog.stop_background_diagmdlog(self.dut, self.qxdm_log_path, keep_logs=False)

