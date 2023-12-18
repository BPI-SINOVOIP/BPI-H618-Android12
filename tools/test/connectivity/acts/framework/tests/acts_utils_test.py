#!/usr/bin/env python3
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import logging
import time
import unittest

import mock

from acts import utils
from acts import signals
from acts.controllers.adb_lib.error import AdbError
from acts.controllers.android_device import AndroidDevice
from acts.controllers.fuchsia_device import FuchsiaDevice
from acts.controllers.utils_lib.ssh.connection import SshConnection
from acts.libs.proc import job

PROVISIONED_STATE_GOOD = 1

MOCK_IP_ADDRESSES = """eno1 100.127.110.79
eno1 2401:fa00:480:7a00:8d4f:85ff:cc5c:787e
eno1 2401:fa00:480:7a00:459:b993:fcbf:1419
eno1 fe80::c66d:3c75:2cec:1d72
enx00e04c000d06 192.168.42.220
enx00e04c000d06 fe80::2c68:f1b7:eaaa:52e7"""

MOCK_IFCONFIG_OUTPUT = """eno1: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 100.127.110.79  netmask 255.255.255.0  broadcast 100.127.110.255
        inet6 2401:fa00:480:7a00:8d4f:85ff:cc5c:787e  prefixlen 64  scopeid 0x0<global>
        inet6 fe80::c66d:3c75:2cec:1d72  prefixlen 64  scopeid 0x20<link>
        inet6 2401:fa00:480:7a00:459:b993:fcbf:1419  prefixlen 64  scopeid 0x0<global>
        ether 54:b2:03:13:36:05  txqueuelen 1000  (Ethernet)
        RX packets 32943262  bytes 13324306863 (13.3 GB)
        RX errors 669  dropped 0  overruns 0  frame 669
        TX packets 4778580  bytes 3012041798 (3.0 GB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
        device interrupt 16  memory 0xdf200000-df220000

enx00e04c000d06: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 192.168.42.220  netmask 255.255.255.0  broadcast 192.168.42.255
        inet6 fe80::2c68:f1b7:eaaa:52e7  prefixlen 64  scopeid 0x20<link>
        ether 00:e0:4c:00:0d:06  txqueuelen 1000  (Ethernet)
        RX packets 10212416  bytes 3204008175 (3.2 GB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 9868425  bytes 5641667955 (5.6 GB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 65536
        inet 127.0.0.1  netmask 255.0.0.0
        inet6 ::1  prefixlen 128  scopeid 0x10<host>
        loop  txqueuelen 1000  (Local Loopback)
        RX packets 42779835  bytes 6144028882 (6.1 GB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 42779835  bytes 6144028882 (6.1 GB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

"""

FUCHSIA_INTERFACES = {
    'id':
    '1',
    'result': [{
        'features':
        4,
        'filepath':
        '[none]',
        'id':
        1,
        'ipv4_addresses': [[127, 0, 0, 1]],
        'ipv6_addresses': [[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1]],
        'is_administrative_status_enabled':
        True,
        'is_physical_status_up':
        True,
        'mac': [0, 0, 0, 0, 0, 0],
        'mtu':
        65536,
        'name':
        'lo',
        'topopath':
        'loopback'
    }, {
        'features':
        0,
        'filepath':
        '/dev/class/ethernet/000',
        'id':
        2,
        'ipv4_addresses': [[100, 127, 110, 79]],
        'ipv6_addresses':
        [[254, 128, 0, 0, 0, 0, 0, 0, 198, 109, 60, 117, 44, 236, 29, 114],
         [36, 1, 250, 0, 4, 128, 122, 0, 141, 79, 133, 255, 204, 92, 120, 126],
         [36, 1, 250, 0, 4, 128, 122, 0, 4, 89, 185, 147, 252, 191, 20, 25]],
        'is_administrative_status_enabled':
        True,
        'is_physical_status_up':
        True,
        'mac': [0, 224, 76, 5, 76, 229],
        'mtu':
        1514,
        'name':
        'eno1',
        'topopath':
        '@/dev/xhci/xhci/usb-bus/001/001/ifc-000/usb-cdc-ecm/ethernet'
    }, {
        'features':
        1,
        'filepath':
        '/dev/class/ethernet/001',
        'id':
        3,
        'ipv4_addresses': [],
        'ipv6_addresses':
        [[254, 128, 0, 0, 0, 0, 0, 0, 96, 255, 93, 96, 52, 253, 253, 243],
         [254, 128, 0, 0, 0, 0, 0, 0, 70, 7, 11, 255, 254, 118, 126, 192]],
        'is_administrative_status_enabled':
        False,
        'is_physical_status_up':
        False,
        'mac': [68, 7, 11, 118, 126, 192],
        'mtu':
        1500,
        'name':
        'wlanxc0',
        'topopath':
        '@/dev/wifi/wlanphy/wlanif-client/wlan-ethernet/ethernet'
    }],
    'error':
    None
}

CORRECT_FULL_IP_LIST = {
    'ipv4_private': [],
    'ipv4_public': ['100.127.110.79'],
    'ipv6_link_local': ['fe80::c66d:3c75:2cec:1d72'],
    'ipv6_private_local': [],
    'ipv6_public': [
        '2401:fa00:480:7a00:8d4f:85ff:cc5c:787e',
        '2401:fa00:480:7a00:459:b993:fcbf:1419'
    ]
}

CORRECT_EMPTY_IP_LIST = {
    'ipv4_private': [],
    'ipv4_public': [],
    'ipv6_link_local': [],
    'ipv6_private_local': [],
    'ipv6_public': []
}

FUCHSIA_INIT_SERVER = ('acts.controllers.fuchsia_device.FuchsiaDevice.'
                       'init_server_connection')
FUCHSIA_SET_CONTROL_PATH_CONFIG = ('acts.controllers.fuchsia_device.'
                                   'FuchsiaDevice._set_control_path_config')
FUCHSIA_START_SERVICES = ('acts.controllers.fuchsia_device.FuchsiaDevice.'
                          'start_services')
FUCHSIA_NETSTACK_LIST_INTERFACES = (
    'acts.controllers.'
    'fuchsia_lib.netstack.netstack_lib.'
    'FuchsiaNetstackLib.netstackListInterfaces')
FUCHSIA_INIT_NETSTACK = ('acts.controllers.fuchsia_lib.netstack.'
                         'netstack_lib.FuchsiaNetstackLib.init')


class ByPassSetupWizardTests(unittest.TestCase):
    """This test class for unit testing acts.utils.bypass_setup_wizard."""
    def test_start_standing_subproc(self):
        with self.assertRaisesRegex(utils.ActsUtilsError,
                                    'Process .* has terminated'):
            utils.start_standing_subprocess('sleep 0', check_health_delay=0.1)

    def test_stop_standing_subproc(self):
        p = utils.start_standing_subprocess('sleep 0')
        time.sleep(0.1)
        with self.assertRaisesRegex(utils.ActsUtilsError,
                                    'Process .* has terminated'):
            utils.stop_standing_subprocess(p)

    @mock.patch('time.sleep')
    def test_bypass_setup_wizard_no_complications(self, _):
        ad = mock.Mock()
        ad.adb.shell.side_effect = [
            # Return value for SetupWizardExitActivity
            BypassSetupWizardReturn.NO_COMPLICATIONS,
            # Return value for device_provisioned
            PROVISIONED_STATE_GOOD,
        ]
        ad.adb.return_state = BypassSetupWizardReturn.NO_COMPLICATIONS
        self.assertTrue(utils.bypass_setup_wizard(ad))
        self.assertFalse(
            ad.adb.root_adb.called,
            'The root command should not be called if there are no '
            'complications.')

    @mock.patch('time.sleep')
    def test_bypass_setup_wizard_unrecognized_error(self, _):
        ad = mock.Mock()
        ad.adb.shell.side_effect = [
            # Return value for SetupWizardExitActivity
            BypassSetupWizardReturn.UNRECOGNIZED_ERR,
            # Return value for device_provisioned
            PROVISIONED_STATE_GOOD,
        ]
        with self.assertRaises(AdbError):
            utils.bypass_setup_wizard(ad)
        self.assertFalse(
            ad.adb.root_adb.called,
            'The root command should not be called if we do not have a '
            'codepath for recovering from the failure.')

    @mock.patch('time.sleep')
    def test_bypass_setup_wizard_need_root_access(self, _):
        ad = mock.Mock()
        ad.adb.shell.side_effect = [
            # Return value for SetupWizardExitActivity
            BypassSetupWizardReturn.ROOT_ADB_NO_COMP,
            # Return value for rooting the device
            BypassSetupWizardReturn.NO_COMPLICATIONS,
            # Return value for device_provisioned
            PROVISIONED_STATE_GOOD
        ]

        utils.bypass_setup_wizard(ad)

        self.assertTrue(
            ad.adb.root_adb_called,
            'The command required root access, but the device was never '
            'rooted.')

    @mock.patch('time.sleep')
    def test_bypass_setup_wizard_need_root_already_skipped(self, _):
        ad = mock.Mock()
        ad.adb.shell.side_effect = [
            # Return value for SetupWizardExitActivity
            BypassSetupWizardReturn.ROOT_ADB_SKIPPED,
            # Return value for SetupWizardExitActivity after root
            BypassSetupWizardReturn.ALREADY_BYPASSED,
            # Return value for device_provisioned
            PROVISIONED_STATE_GOOD
        ]
        self.assertTrue(utils.bypass_setup_wizard(ad))
        self.assertTrue(ad.adb.root_adb_called)

    @mock.patch('time.sleep')
    def test_bypass_setup_wizard_root_access_still_fails(self, _):
        ad = mock.Mock()
        ad.adb.shell.side_effect = [
            # Return value for SetupWizardExitActivity
            BypassSetupWizardReturn.ROOT_ADB_FAILS,
            # Return value for SetupWizardExitActivity after root
            BypassSetupWizardReturn.UNRECOGNIZED_ERR,
            # Return value for device_provisioned
            PROVISIONED_STATE_GOOD
        ]

        with self.assertRaises(AdbError):
            utils.bypass_setup_wizard(ad)
        self.assertTrue(ad.adb.root_adb_called)


class BypassSetupWizardReturn:
    # No complications. Bypass works the first time without issues.
    NO_COMPLICATIONS = (
        'Starting: Intent { cmp=com.google.android.setupwizard/'
        '.SetupWizardExitActivity }')

    # Fail with doesn't need to be skipped/was skipped already.
    ALREADY_BYPASSED = AdbError('', 'ADB_CMD_OUTPUT:0', 'Error type 3\n'
                                'Error: Activity class', 1)
    # Fail with different error.
    UNRECOGNIZED_ERR = AdbError('', 'ADB_CMD_OUTPUT:0', 'Error type 4\n'
                                'Error: Activity class', 0)
    # Fail, get root access, then no complications arise.
    ROOT_ADB_NO_COMP = AdbError(
        '', 'ADB_CMD_OUTPUT:255', 'Security exception: Permission Denial: '
        'starting Intent { flg=0x10000000 '
        'cmp=com.google.android.setupwizard/'
        '.SetupWizardExitActivity } from null '
        '(pid=5045, uid=2000) not exported from uid '
        '10000', 0)
    # Even with root access, the bypass setup wizard doesn't need to be skipped.
    ROOT_ADB_SKIPPED = AdbError(
        '', 'ADB_CMD_OUTPUT:255', 'Security exception: Permission Denial: '
        'starting Intent { flg=0x10000000 '
        'cmp=com.google.android.setupwizard/'
        '.SetupWizardExitActivity } from null '
        '(pid=5045, uid=2000) not exported from '
        'uid 10000', 0)
    # Even with root access, the bypass setup wizard fails
    ROOT_ADB_FAILS = AdbError(
        '', 'ADB_CMD_OUTPUT:255',
        'Security exception: Permission Denial: starting Intent { '
        'flg=0x10000000 cmp=com.google.android.setupwizard/'
        '.SetupWizardExitActivity } from null (pid=5045, uid=2000) not '
        'exported from uid 10000', 0)


class ConcurrentActionsTest(unittest.TestCase):
    """Tests acts.utils.run_concurrent_actions and related functions."""
    @staticmethod
    def function_returns_passed_in_arg(arg):
        return arg

    @staticmethod
    def function_raises_passed_in_exception_type(exception_type):
        raise exception_type

    def test_run_concurrent_actions_no_raise_returns_proper_return_values(
        self):
        """Tests run_concurrent_actions_no_raise returns in the correct order.

        Each function passed into run_concurrent_actions_no_raise returns the
        values returned from each individual callable in the order passed in.
        """
        ret_values = utils.run_concurrent_actions_no_raise(
            lambda: self.function_returns_passed_in_arg('ARG1'),
            lambda: self.function_returns_passed_in_arg('ARG2'),
            lambda: self.function_returns_passed_in_arg('ARG3'))

        self.assertEqual(len(ret_values), 3)
        self.assertEqual(ret_values[0], 'ARG1')
        self.assertEqual(ret_values[1], 'ARG2')
        self.assertEqual(ret_values[2], 'ARG3')

    def test_run_concurrent_actions_no_raise_returns_raised_exceptions(self):
        """Tests run_concurrent_actions_no_raise returns raised exceptions.

        Instead of allowing raised exceptions to be raised in the main thread,
        this function should capture the exception and return them in the slot
        the return value should have been returned in.
        """
        ret_values = utils.run_concurrent_actions_no_raise(
            lambda: self.function_raises_passed_in_exception_type(IndexError),
            lambda: self.function_raises_passed_in_exception_type(KeyError))

        self.assertEqual(len(ret_values), 2)
        self.assertEqual(ret_values[0].__class__, IndexError)
        self.assertEqual(ret_values[1].__class__, KeyError)

    def test_run_concurrent_actions_returns_proper_return_values(self):
        """Tests run_concurrent_actions returns in the correct order.

        Each function passed into run_concurrent_actions returns the values
        returned from each individual callable in the order passed in.
        """

        ret_values = utils.run_concurrent_actions(
            lambda: self.function_returns_passed_in_arg('ARG1'),
            lambda: self.function_returns_passed_in_arg('ARG2'),
            lambda: self.function_returns_passed_in_arg('ARG3'))

        self.assertEqual(len(ret_values), 3)
        self.assertEqual(ret_values[0], 'ARG1')
        self.assertEqual(ret_values[1], 'ARG2')
        self.assertEqual(ret_values[2], 'ARG3')

    def test_run_concurrent_actions_raises_exceptions(self):
        """Tests run_concurrent_actions raises exceptions from given actions."""
        with self.assertRaises(KeyError):
            utils.run_concurrent_actions(
                lambda: self.function_returns_passed_in_arg('ARG1'), lambda:
                self.function_raises_passed_in_exception_type(KeyError))

    def test_test_concurrent_actions_raises_non_test_failure(self):
        """Tests test_concurrent_actions raises the given exception."""
        with self.assertRaises(KeyError):
            utils.test_concurrent_actions(
                lambda: self.function_raises_passed_in_exception_type(KeyError
                                                                      ),
                failure_exceptions=signals.TestFailure)

    def test_test_concurrent_actions_raises_test_failure(self):
        """Tests test_concurrent_actions raises the given exception."""
        with self.assertRaises(signals.TestFailure):
            utils.test_concurrent_actions(
                lambda: self.function_raises_passed_in_exception_type(KeyError
                                                                      ),
                failure_exceptions=KeyError)


class SuppressLogOutputTest(unittest.TestCase):
    """Tests SuppressLogOutput"""
    def test_suppress_log_output(self):
        """Tests that the SuppressLogOutput context manager removes handlers
        of the specified levels upon entry and re-adds handlers upon exit.
        """
        handlers = [
            logging.NullHandler(level=lvl)
            for lvl in (logging.DEBUG, logging.INFO, logging.ERROR)
        ]
        log = logging.getLogger('test_log')
        for handler in handlers:
            log.addHandler(handler)
        with utils.SuppressLogOutput(log, [logging.INFO, logging.ERROR]):
            self.assertTrue(
                any(handler.level == logging.DEBUG
                    for handler in log.handlers))
            self.assertFalse(
                any(handler.level in (logging.INFO, logging.ERROR)
                    for handler in log.handlers))
        self.assertCountEqual(handlers, log.handlers)


class IpAddressUtilTest(unittest.TestCase):
    def test_positive_ipv4_normal_address(self):
        ip_address = "192.168.1.123"
        self.assertTrue(utils.is_valid_ipv4_address(ip_address))

    def test_positive_ipv4_any_address(self):
        ip_address = "0.0.0.0"
        self.assertTrue(utils.is_valid_ipv4_address(ip_address))

    def test_positive_ipv4_broadcast(self):
        ip_address = "255.255.255.0"
        self.assertTrue(utils.is_valid_ipv4_address(ip_address))

    def test_negative_ipv4_with_ipv6_address(self):
        ip_address = "fe80::f693:9fff:fef4:1ac"
        self.assertFalse(utils.is_valid_ipv4_address(ip_address))

    def test_negative_ipv4_with_invalid_string(self):
        ip_address = "fdsafdsafdsafdsf"
        self.assertFalse(utils.is_valid_ipv4_address(ip_address))

    def test_negative_ipv4_with_invalid_number(self):
        ip_address = "192.168.500.123"
        self.assertFalse(utils.is_valid_ipv4_address(ip_address))

    def test_positive_ipv6(self):
        ip_address = 'fe80::f693:9fff:fef4:1ac'
        self.assertTrue(utils.is_valid_ipv6_address(ip_address))

    def test_positive_ipv6_link_local(self):
        ip_address = 'fe80::'
        self.assertTrue(utils.is_valid_ipv6_address(ip_address))

    def test_negative_ipv6_with_ipv4_address(self):
        ip_address = '192.168.1.123'
        self.assertFalse(utils.is_valid_ipv6_address(ip_address))

    def test_negative_ipv6_invalid_characters(self):
        ip_address = 'fe80:jkyr:f693:9fff:fef4:1ac'
        self.assertFalse(utils.is_valid_ipv6_address(ip_address))

    def test_negative_ipv6_invalid_string(self):
        ip_address = 'fdsafdsafdsafdsf'
        self.assertFalse(utils.is_valid_ipv6_address(ip_address))

    @mock.patch('acts.libs.proc.job.run')
    def test_local_get_interface_ip_addresses_full(self, job_mock):
        job_mock.side_effect = [
            job.Result(stdout=bytes(MOCK_IP_ADDRESSES, 'utf-8'),
                       encoding='utf-8'),
            job.Result(stdout=bytes(MOCK_IFCONFIG_OUTPUT, 'utf-8'),
                       encoding='utf-8')
        ]
        self.assertEqual(utils.get_interface_ip_addresses(job, 'eno1'),
                         CORRECT_FULL_IP_LIST)

    @mock.patch('acts.libs.proc.job.run')
    def test_local_get_interface_ip_addresses_empty(self, job_mock):
        job_mock.side_effect = [
            job.Result(stdout=bytes(MOCK_IP_ADDRESSES, 'utf-8'),
                       encoding='utf-8'),
            job.Result(stdout=bytes(MOCK_IFCONFIG_OUTPUT, 'utf-8'),
                       encoding='utf-8')
        ]
        self.assertEqual(utils.get_interface_ip_addresses(job, 'wlan1'),
                         CORRECT_EMPTY_IP_LIST)

    @mock.patch('acts.controllers.utils_lib.ssh.connection.SshConnection.run')
    def test_ssh_get_interface_ip_addresses_full(self, ssh_mock):
        ssh_mock.side_effect = [
            job.Result(stdout=bytes(MOCK_IP_ADDRESSES, 'utf-8'),
                       encoding='utf-8'),
            job.Result(stdout=bytes(MOCK_IFCONFIG_OUTPUT, 'utf-8'),
                       encoding='utf-8')
        ]
        self.assertEqual(
            utils.get_interface_ip_addresses(SshConnection('mock_settings'),
                                             'eno1'), CORRECT_FULL_IP_LIST)

    @mock.patch('acts.controllers.utils_lib.ssh.connection.SshConnection.run')
    def test_ssh_get_interface_ip_addresses_empty(self, ssh_mock):
        ssh_mock.side_effect = [
            job.Result(stdout=bytes(MOCK_IP_ADDRESSES, 'utf-8'),
                       encoding='utf-8'),
            job.Result(stdout=bytes(MOCK_IFCONFIG_OUTPUT, 'utf-8'),
                       encoding='utf-8')
        ]
        self.assertEqual(
            utils.get_interface_ip_addresses(SshConnection('mock_settings'),
                                             'wlan1'), CORRECT_EMPTY_IP_LIST)

    @mock.patch('acts.controllers.adb.AdbProxy')
    @mock.patch.object(AndroidDevice, 'is_bootloader', return_value=True)
    def test_android_get_interface_ip_addresses_full(self, is_bootloader,
                                                     adb_mock):
        adb_mock().shell.side_effect = [
            MOCK_IP_ADDRESSES, MOCK_IFCONFIG_OUTPUT
        ]
        self.assertEqual(
            utils.get_interface_ip_addresses(AndroidDevice(), 'eno1'),
            CORRECT_FULL_IP_LIST)

    @mock.patch('acts.controllers.adb.AdbProxy')
    @mock.patch.object(AndroidDevice, 'is_bootloader', return_value=True)
    def test_android_get_interface_ip_addresses_empty(self, is_bootloader,
                                                      adb_mock):
        adb_mock().shell.side_effect = [
            MOCK_IP_ADDRESSES, MOCK_IFCONFIG_OUTPUT
        ]
        self.assertEqual(
            utils.get_interface_ip_addresses(AndroidDevice(), 'wlan1'),
            CORRECT_EMPTY_IP_LIST)

    @mock.patch(FUCHSIA_INIT_SERVER)
    @mock.patch(FUCHSIA_SET_CONTROL_PATH_CONFIG)
    @mock.patch(FUCHSIA_START_SERVICES)
    @mock.patch(FUCHSIA_NETSTACK_LIST_INTERFACES)
    @mock.patch(FUCHSIA_INIT_NETSTACK)
    def test_fuchsia_get_interface_ip_addresses_full(self, init_mock,
                                                     list_interfaces_mock,
                                                     start_services_mock,
                                                     control_path_mock,
                                                     fuchsia_device_mock):
        init_mock.return_value = None
        list_interfaces_mock.return_value = FUCHSIA_INTERFACES
        fuchsia_device_mock.return_value = None
        self.assertEqual(
            utils.get_interface_ip_addresses(
                FuchsiaDevice({'ip': '192.168.1.1'}), 'eno1'),
            CORRECT_FULL_IP_LIST)

    @mock.patch(FUCHSIA_INIT_SERVER)
    @mock.patch(FUCHSIA_SET_CONTROL_PATH_CONFIG)
    @mock.patch(FUCHSIA_START_SERVICES)
    @mock.patch(FUCHSIA_NETSTACK_LIST_INTERFACES)
    @mock.patch(FUCHSIA_INIT_NETSTACK)
    def test_fuchsia_get_interface_ip_addresses_empty(self, init_mock,
                                                      list_interfaces_mock,
                                                      start_services_mock,
                                                      control_path_mock,
                                                      fuchsia_device_mock):
        init_mock.return_value = None
        list_interfaces_mock.return_value = FUCHSIA_INTERFACES
        fuchsia_device_mock.return_value = None
        self.assertEqual(
            utils.get_interface_ip_addresses(
                FuchsiaDevice({'ip': '192.168.1.1'}), 'wlan1'),
            CORRECT_EMPTY_IP_LIST)


if __name__ == '__main__':
    unittest.main()
