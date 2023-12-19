# Lint as: python2, python3
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import sys
import functools
import logging
import math
import time

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import hosts
from autotest_lib.client.common_lib import utils
from autotest_lib.server.cros.power import servo_charger
from autotest_lib.server.cros.servo import servo
from autotest_lib.server.hosts import cros_constants
from autotest_lib.server.hosts import repair_utils
from autotest_lib.server.hosts import servo_constants
from autotest_lib.server.cros.servo.topology import servo_topology
from autotest_lib.site_utils.admin_audit import servo_updater
import six

try:
    from chromite.lib import metrics
except ImportError:
    metrics = utils.metrics_mock


# TODO(gregorynisbet): will importing chromite always succeed in all contexts?
from chromite.lib import timeout_util


def ignore_exception_for_non_cros_host(func):
    """
    Decorator to ignore ControlUnavailableError if servo host is not cros host.
    When using test_that command on a workstation, this enables usage of
    additional servo devices such as servo micro and Sweetberry. This shall not
    change any lab behavior.
    """
    @functools.wraps(func)
    def wrapper(self, host):
        """
        Wrapper around func.
        """
        try:
            func(self, host)
        except servo.ControlUnavailableError as e:
            if host.is_cros_host():
                raise
            logging.warning("Servo host is not cros host, ignore %s: %s",
                            type(e).__name__, e)
    return wrapper


class _UpdateVerifier(hosts.Verifier):
    """
    Verifier to trigger a servo host update, if necessary.

    The operation doesn't wait for the update to complete and is
    considered a success whether or not the servo is currently
    up-to-date.
    """

    @timeout_util.TimeoutDecorator(cros_constants.LONG_VERIFY_TIMEOUT_SEC)
    def verify(self, host):
        # First, only run this verifier if the host is in the physical lab.
        # Secondly, skip if the test is being run by test_that, because subnet
        # restrictions can cause the update to fail.
        try:
            if host.is_labstation():
                logging.info("Skip update check because the host is a"
                             " labstation and labstation update is handled"
                             " by labstation AdminRepair task.")
                return
            if host.is_in_lab() and host.job and host.job.in_lab:
                if (
                        not host.get_dut_host_info() or
                        not host.get_dut_host_info().servo_cros_stable_version
                ):
                    logging.info('Servo stable version missed.'
                                 ' Skip update check action.')
                    return
                # We have seen cases that invalid GPT headers/entries block
                # v3s from been update, so always try to repair here.
                # See crbug.com/994396, crbug.com/1057302.
                host.run('cgpt repair /dev/mmcblk0', ignore_status=True)
                host.update_image()
        # We don't want failure from update block DUT repair action.
        # See crbug.com/1029950.
        except Exception as e:
            six.reraise(hosts.AutoservNonCriticalVerifyError, str(e),
                        sys.exc_info()[2])

    @property
    def description(self):
        return 'servo host software is up-to-date'


class _ConfigVerifier(hosts.Verifier):
    """
    Base verifier for the servo config file verifiers.
    """

    CONFIG_FILE = '/var/lib/servod/config'
    ATTR = ''

    @staticmethod
    def _get_config_val(host, config_file, attr):
        """
        Get the `attr` for `host` from `config_file`.

        @param host         Host to be checked for `config_file`.
        @param config_file  Path to the config file to be tested.
        @param attr         Attribute to get from config file.

        @return The attr val as set in the config file, or `None` if
                the file was absent.
        """
        getboard = ('CONFIG=%s ; [ -f $CONFIG ] && '
                    '. $CONFIG && echo $%s' % (config_file, attr))
        attr_val = host.run(getboard, ignore_status=True).stdout
        return attr_val.strip('\n') if attr_val else None

    @staticmethod
    def _validate_attr(host, val, expected_val, attr, config_file):
        """
        Check that the attr setting is valid for the host.

        This presupposes that a valid config file was found.  Raise an
        execption if:
          * There was no attr setting from the file (i.e. the setting
            is an empty string), or
          * The attr setting is valid, the attr is known,
            and the setting doesn't match the DUT.

        @param host         Host to be checked for `config_file`.
        @param val          Value to be tested.
        @param expected_val Expected value.
        @param attr         Attribute we're validating.
        @param config_file  Path to the config file to be tested.
        """
        if not val:
            raise hosts.AutoservVerifyError(
                    'config file %s exists, but %s '
                    'is not set' % (attr, config_file))
        if expected_val is not None and val != expected_val:
            raise hosts.AutoservVerifyError(
                    '%s is %s; it should be %s' % (attr, val, expected_val))


    def _get_config(self, host):
        """
        Return the config file to check.

        @param host     Host object.

        @return The config file to check.
        """
        return '%s_%d' % (self.CONFIG_FILE, host.servo_port)

    @property
    def description(self):
        return 'servo %s setting is correct' % self.ATTR


class _SerialConfigVerifier(_ConfigVerifier):
    """
    Verifier for the servo SERIAL configuration.
    """

    ATTR = 'SERIAL'

    @timeout_util.TimeoutDecorator(cros_constants.SHORT_VERIFY_TIMEOUT_SEC)
    def verify(self, host):
        """
        Test whether the `host` has a `SERIAL` setting configured.

        This tests the config file names used by the `servod` upstart
        job for a valid setting of the `SERIAL` variable.  The following
        conditions raise errors:
          * The SERIAL setting doesn't match the DUT's entry in the AFE
            database.
          * There is no config file.
        """
        if not host.is_cros_host():
            return
        # Not all servo hosts will have a servo serial so don't verify if it's
        # not set.
        if host.servo_serial is None:
            return
        config = self._get_config(host)
        serialval = self._get_config_val(host, config, self.ATTR)
        if serialval is None:
            raise hosts.AutoservVerifyError(
                    'Servo serial is unconfigured; should be %s'
                    % host.servo_serial
            )

        self._validate_attr(host, serialval, host.servo_serial, self.ATTR,
                            config)



class _BoardConfigVerifier(_ConfigVerifier):
    """
    Verifier for the servo BOARD configuration.
    """

    ATTR = 'BOARD'

    @timeout_util.TimeoutDecorator(cros_constants.SHORT_VERIFY_TIMEOUT_SEC)
    def verify(self, host):
        """
        Test whether the `host` has a `BOARD` setting configured.

        This tests the config file names used by the `servod` upstart
        job for a valid setting of the `BOARD` variable.  The following
        conditions raise errors:
          * A config file exists, but the content contains no setting
            for BOARD.
          * The BOARD setting doesn't match the DUT's entry in the AFE
            database.
          * There is no config file.
        """
        if not host.is_cros_host():
            return
        config = self._get_config(host)
        boardval = self._get_config_val(host, config, self.ATTR)
        if boardval is None:
            msg = 'Servo board is unconfigured'
            if host.servo_board is not None:
                msg += '; should be %s' % host.servo_board
            raise hosts.AutoservVerifyError(msg)

        self._validate_attr(host, boardval, host.servo_board, self.ATTR,
                            config)


class _ServodJobVerifier(hosts.Verifier):
    """
    Verifier to check that the `servod` upstart job is running.
    """

    @timeout_util.TimeoutDecorator(cros_constants.SHORT_VERIFY_TIMEOUT_SEC)
    def verify(self, host):
        if not host.is_cros_host():
            return
        status_cmd = 'status servod PORT=%d' % host.servo_port
        job_status = host.run(status_cmd, ignore_status=True).stdout
        if 'start/running' not in job_status:
            raise hosts.AutoservVerifyError(
                    'servod not running on %s port %d' %
                    (host.hostname, host.servo_port))

    @property
    def description(self):
        return 'servod upstart job is running'


class _DiskSpaceVerifier(hosts.Verifier):
    """
    Verifier to make sure there is enough disk space left on servohost.
    """

    @timeout_util.TimeoutDecorator(cros_constants.SHORT_VERIFY_TIMEOUT_SEC)
    def verify(self, host):
        # Check available space of stateful is greater than threshold, in Gib.
        host.check_diskspace('/mnt/stateful_partition', 0.5)

    @property
    def description(self):
        return 'servohost has enough disk space.'


class _ServodConnectionVerifier(hosts.Verifier):
    """
    Verifier to check that we can connect to servod server.

    If this verifier failed, it most likely servod was crashed or in a
    crashing loop. For servo_v4 it's usually caused by not able to detect
    CCD or servo_micro.
    """

    @timeout_util.TimeoutDecorator(cros_constants.VERIFY_TIMEOUT_SEC)
    def verify(self, host):
        host.initilize_servo()

    @property
    def description(self):
        return 'servod service is taking calls'


class _ServodControlVerifier(hosts.Verifier):
    """
    Verifier to check basic servo control functionality.

    This tests the connection to the target servod service with a simple
    method call.  As a side-effect, all servo signals are initialized to
    default values.

    N.B. Initializing servo signals is necessary because the power
    button and lid switch verifiers both test against expected initial
    values.
    """

    @timeout_util.TimeoutDecorator(cros_constants.VERIFY_TIMEOUT_SEC)
    def verify(self, host):
        try:
            host.initialize_dut_for_servo()
        except Exception as e:
            six.reraise(hosts.AutoservNonCriticalVerifyError, str(e),
                        sys.exc_info()[2])

    @property
    def description(self):
        return 'Basic servod control is working'


class _Cr50ConsoleVerifier(hosts.Verifier):
    """Verifier to check if cr50 console is present and working.

    Validating based by running commands and expect they will not fail.
    If any command fail then console is not working as expected.
    """

    COMMAND_TO_CHECK_CONSOLE = (
            'cr50_ccd_level',
            'cr50_testlab',
            'cr50_ccd_state_flags',
    )

    @ignore_exception_for_non_cros_host
    @timeout_util.TimeoutDecorator(cros_constants.VERIFY_TIMEOUT_SEC)
    def verify(self, host):
        try:
            for command in self.COMMAND_TO_CHECK_CONSOLE:
                if host.get_servo().has_control(command):
                    # Response of command is not important.
                    host.get_servo().get(command)
        except Exception as e:
            six.reraise(hosts.AutoservNonCriticalVerifyError, str(e),
                        sys.exc_info()[2])

    def _is_applicable(self, host):
        # Only when DUT is running through ccd.
        # TODO(coconutruben): replace with ccd API when available in servo.py
        return (host.get_servo()
                and host.get_servo().get_main_servo_device() == 'ccd_cr50')

    @property
    def description(self):
        return 'CR50 console is working'


class _CCDTestlabVerifier(hosts.Verifier):
    """
    Verifier to check that ccd testlab is enabled.

    All DUT connected by ccd has to supported cr50 with enabled testlab
    to allow manipulation by servo. The flag testlab is sticky and will
    stay enabled if was set up. The testlab can be enabled when ccd is
    open. (go/ccd-setup)
    """
    @ignore_exception_for_non_cros_host
    @timeout_util.TimeoutDecorator(cros_constants.VERIFY_TIMEOUT_SEC)
    def verify(self, host):
        if not host.get_servo().has_control('cr50_testlab'):
            raise hosts.AutoservVerifyError(
                'cr50 has to be supported when use servo with '
                'ccd_cr50/type-c connection')

        status = host.get_servo().get('cr50_testlab')
        # check by 'on' to fail when get unexpected value
        if status == 'on':
            # ccd testlab enabled
            return
        raise hosts.AutoservNonCriticalVerifyError(
            'The ccd testlab is disabled; DUT requires manual work '
            'to enable it (go/ccd-setup).')

    def _is_applicable(self, host):
        # Only when DUT is running through ccd.
        # TODO(coconutruben): replace with ccd API when available in servo.py
        return (host.get_servo()
                and host.get_servo().get_main_servo_device() == 'ccd_cr50')

    @property
    def description(self):
        return 'ccd testlab enabled'

class _CCDPowerDeliveryVerifier(hosts.Verifier):
    """Verifier to check and reset servo_v4_role for servos that support
    power delivery feature(a.k.a power pass through).

    There are currently two position of servo_v4_role, src and snk:
    src --  servo in power delivery mode and passes power to the DUT.
    snk --  servo in normal mode and not passes power to DUT.
    We want to ensure that servo_v4_role is set to src.

    TODO(xianuowang@) Convert it to verifier/repair action pair or remove it
    once we collected enough metrics.
    """
    # Change to use the  constant value in CrosHost if we move it to
    # verifier/repair pair.
    CHANGE_SERVO_ROLE_TIMEOUT = 180

    @timeout_util.TimeoutDecorator(cros_constants.VERIFY_TIMEOUT_SEC)
    def verify(self, host):
        if host.get_servo().get('servo_v4_role') == 'snk':
            logging.warning('The servo initlized with role snk while'
                            ' supporting power delivery, resetting role'
                            ' to src...')

            try:
                logging.info('setting power direction with retries')
                # do not pass host since host does not inherit from CrosHost.
                charge_manager = servo_charger.ServoV4ChargeManager(
                    host=None,
                    servo=host.get_servo(),
                )
                attempts = charge_manager.start_charging()
                logging.info('setting power direction took %d tries', attempts)
                # if control makes it here, we successfully changed the host
                # direction
                result = 'src'
            except Exception as e:
                logging.error(
                    'setting power direction with retries failed %s',
                    str(e),
                )
            finally:
                time.sleep(self.CHANGE_SERVO_ROLE_TIMEOUT)

            result = host.get_servo().get('servo_v4_role')
            logging.debug('Servo_v4 role after reset: %s', result)

            metrics_data = {
                'hostname': host.get_dut_hostname() or 'unknown',
                'status': 'success' if result == 'src' else 'failed',
                'board': host.servo_board or 'unknown',
                'model': host.servo_model or 'unknown'
            }
            metrics.Counter(
                'chromeos/autotest/repair/verifier/power_delivery3'
            ).increment(fields=metrics_data)

    def _is_applicable(self, host):
        return (host.is_in_lab() and
                host.get_servo().supports_built_in_pd_control())

    @property
    def description(self):
        return 'ensure applicable servo is in "src" mode for power delivery'


class _BaseDUTConnectionVerifier(hosts.Verifier):
    """Verifier to check connection between DUT and servo."""

    # Bus voltage on ppdut5. Value can be:
    # - less than 500 - DUT is likely not connected
    # - between 500 and 4000 - unexpected value
    # - more than 4000 - DUT is likely connected
    MAX_PPDUT5_MV_WHEN_NOT_CONNECTED = 500
    MIN_PPDUT5_MV_WHEN_CONNECTED = 4000

    def _is_usb_hub_connected(self, host):
        """Checking bus voltage on ppdut5.

        Supported only on servo_v4 boards.
        If voltage value is lower than 500 then device is not connected.
        When value higher 4000 means the device is connected. If value
        between 500 and 4000 is not expected and will be marked as connected
        and collected information which DUT has this exception.

        @returns: bool
        """
        logging.debug('Started check by ppdut5_mv:on')
        try:
            val = host.get_servo().get('ppdut5_mv')
            if val < self.MAX_PPDUT5_MV_WHEN_NOT_CONNECTED:
                # servo is not connected to the DUT
                return False
            if val < self.MIN_PPDUT5_MV_WHEN_CONNECTED:
                # is unexpected value.
                # collecting metrics to look case by case
                # TODO(otabek) for analysis b:163845694
                data = host._get_host_metrics_data()
                metrics.Counter('chromeos/autotest/repair/ppdut5_mv_case'
                                ).increment(fields=data)
            # else:
            # servo is physical connected to the DUT
        except Exception as e:
            logging.debug('(Not critical) %s', e)
        return True

    def _is_ribbon_cable_connected(self, host):
        """Check if ribbon cable is connected to the DUT.

        The servo_micro/flex - can be checked by `cold_reset` signal.
        When `cold_reset` is `on` it commonly indicates that the DUT
        is disconnected. To avoid mistake of real signal we try
        switch it off and if is cannot then servo is not connected.

        @returns: bool
        """
        logging.debug('Started check by cold_reset:on')
        try:
            if host.get_servo().get('cold_reset') == 'on':
                # If cold_reset has is on can be right signal
                # or caused by missing connection between servo_micro and DUT.
                # if we can switch it to the off then it was signal.
                host.get_servo().set('cold_reset', 'off')
        except error.TestFail:
            logging.debug('Ribbon cable is not connected to the DUT.')
            return False
        except Exception as e:
            logging.debug('(Not critical) %s', e)
        return True

    def _is_dut_power_on(self, host):
        # DUT is running in normal state.
        # if EC not supported by board then we expect error
        try:
            return host.get_servo().get('ec_system_powerstate') == 'S0'
        except Exception as e:
            logging.debug('(Not critical) %s', e)
        return False

    def _is_servo_v4_type_a(self, host):
        return (host.is_labstation()
                and host.get_servo().has_control('servo_v4_type')
                and host.get_servo().get('servo_v4_type') == 'type-a')

    def _is_servo_v4_type_c(self, host):
        return (host.is_labstation()
                and host.get_servo().has_control('servo_v4_type')
                and host.get_servo().get('servo_v4_type') == 'type-c')

    def _is_servo_v3(self, host):
        return not host.is_labstation()


class _DUTConnectionVerifier(_BaseDUTConnectionVerifier):
    """Verifier to check connection Servo to the DUT.

    Servo_v4 type-a connected to the DUT by:
        1) servo_micro - checked by `cold_reset`.
    Servo_v4 type-c connected to the DUT by:
        1) ccd - checked by ppdut5_mv.
    Servo_v3 connected to the DUT by:
        1) legacy servo header - can be checked by `cold_reset`.
    """

    @ignore_exception_for_non_cros_host
    @timeout_util.TimeoutDecorator(cros_constants.VERIFY_TIMEOUT_SEC)
    def verify(self, host):
        if self._is_servo_v4_type_a(host):
            if not self._is_ribbon_cable_connected(host):
                raise hosts.AutoservVerifyError(
                        'Servo_micro is likely not connected to the DUT.')
        elif self._is_servo_v4_type_c(host):
            logging.info('Skip check for type-c till confirm it in the lab')
            # TODO(otabek@) block check till verify on the lab
            # if not self._is_usb_hub_connected(host):
            #     raise hosts.AutoservVerifyError(
            #             'Servo_v4 is likely not connected to the DUT.')
        elif self._is_servo_v3(host):
            if not self._is_ribbon_cable_connected(host):
                raise hosts.AutoservVerifyError(
                        'Servo_v3 is likely not connected to the DUT.')
        else:
            logging.warn('Unsupported servo type!')

    def _is_applicable(self, host):
        if host.is_ec_supported():
            return True
        logging.info('DUT is not support EC.')
        return False

    @property
    def description(self):
        return 'Ensure the Servo connected to the DUT.'


class _ServoHubConnectionVerifier(_BaseDUTConnectionVerifier):
    """Verifier to check connection ServoHub to DUT.

    Servo_v4 type-a connected to the DUT by:
        1) USB hub - checked by ppdut5_mv.
    """

    @ignore_exception_for_non_cros_host
    @timeout_util.TimeoutDecorator(cros_constants.VERIFY_TIMEOUT_SEC)
    def verify(self, host):
        if self._is_servo_v4_type_a(host):
            if (self._is_dut_power_on(host)
                        and not self._is_usb_hub_connected(host)):
                raise hosts.AutoservVerifyError(
                        'Servo USB hub is likely not connected to the DUT.')

    def _is_applicable(self, host):
        if host.is_ec_supported():
            return True
        logging.info('DUT is not support EC.')
        return False

    @property
    def description(self):
        return 'Ensure the Servo HUB connected to the DUT.'


class _TopologyVerifier(hosts.Verifier):
    """Verifier that all servo component is presented."""

    @ignore_exception_for_non_cros_host
    @timeout_util.TimeoutDecorator(cros_constants.VERIFY_TIMEOUT_SEC)
    def verify(self, host):
        topology = servo_topology.ServoTopology(host)
        topology.read(host.get_dut_host_info())
        try:
            # Linux takes 1 second to detect and enumerate USB device since
            # 2010 year. We take 10 seconds to be sure as old standard was
            # 5 seconds.
            time.sleep(10)
            topology.validate(raise_error=True,
                              dual_set=host.is_dual_setup(),
                              compare=True)
        except servo_topology.ServoTopologyError as e:
            six.reraise(hosts.AutoservVerifyError, str(e), sys.exc_info()[2])

    def _is_applicable(self, host):
        if host.is_localhost():
            logging.info('Target servo is not in a lab,'
                         ' action is not applicable.')
            return False
        if not host.is_servo_topology_supported():
            logging.info('Target servo-topology is not supported,'
                         ' action is not applicable.')
            return False
        return True

    @property
    def description(self):
        return 'Ensure all Servo component present.'


class _PowerButtonVerifier(hosts.Verifier):
    """
    Verifier to check sanity of the `pwr_button` signal.

    Tests that the `pwr_button` signal shows the power button has been
    released.  When `pwr_button` is stuck at `press`, it commonly
    indicates that the ribbon cable is disconnected.
    """
    # TODO (crbug.com/646593) - Remove list below once servo has been updated
    # with a dummy pwr_button signal.
    _BOARDS_WO_PWR_BUTTON = ['arkham', 'gale', 'mistral', 'storm', 'whirlwind']

    @ignore_exception_for_non_cros_host
    @timeout_util.TimeoutDecorator(cros_constants.SHORT_VERIFY_TIMEOUT_SEC)
    def verify(self, host):
        if host.servo_board in self._BOARDS_WO_PWR_BUTTON:
            return
        try:
            button = host.get_servo().get('pwr_button')
        except Exception as e:
            six.reraise(hosts.AutoservNonCriticalVerifyError, str(e),
                        sys.exc_info()[2])

        if button != 'release':
            raise hosts.AutoservNonCriticalVerifyError(
                'Check ribbon cable: \'pwr_button\' is stuck')

    def _is_applicable(self, host):
        return (host.get_servo() and host.get_servo().main_device_is_flex())

    @property
    def description(self):
        return 'pwr_button control is normal'


class _BatteryVerifier(hosts.Verifier):
    """Collect battery info for analysis."""

    @ignore_exception_for_non_cros_host
    @timeout_util.TimeoutDecorator(cros_constants.VERIFY_TIMEOUT_SEC)
    def verify(self, host):
        try:
            servo = host.get_servo()
            charging = False
            if servo.has_control('battery_is_charging'):
                charging = servo.get('battery_is_charging')
            level = -1
            if servo.has_control('battery_charge_percent'):
                level = servo.get('battery_charge_percent')
            design_mah = servo.get('battery_full_design_mah')
            charge_mah = servo.get('battery_full_charge_mah')
            logging.info('Charging: %s', charging)
            logging.info('Percentage: %s', level)
            logging.info('Full charge max: %s', charge_mah)
            logging.info('Full design max: %s', design_mah)
            # based on analysis of ratio we can find out what is
            # the level when we can say that battery is dead
            ratio = int(math.floor(charge_mah / design_mah * 100.0))
            logging.info('Ratio: %s', ratio)
            data = {
                    'board': host.servo_board or 'unknown',
                    'model': host.servo_model or 'unknown',
                    'ratio': ratio
            }
            metrics.Counter('chromeos/autotest/battery/ratio').increment(
                    fields=data)
        except Exception as e:
            # Keeping it with info level because we do not expect it.
            logging.info('(Not critical) %s', e)

    def _is_applicable(self, host):
        if not host.is_ec_supported():
            logging.info('The board not support EC')
            return False
        dut_info = host.get_dut_host_info()
        if dut_info:
            host_info = host.get_dut_host_info()
            if host_info.get_label_value('power') != 'battery':
                logging.info('The board does not have battery')
                return False
        servo = host.get_servo()
        if (not servo.has_control('battery_full_design_mah')
                    or not servo.has_control('battery_full_charge_mah')):
            logging.info('The board is not supported battery controls...')
            return False
        return True

    @property
    def description(self):
        return 'Logs battery levels'


class _LidVerifier(hosts.Verifier):
    """
    Verifier to check sanity of the `lid_open` signal.
    """

    @ignore_exception_for_non_cros_host
    @timeout_util.TimeoutDecorator(cros_constants.SHORT_VERIFY_TIMEOUT_SEC)
    def verify(self, host):
        try:
            lid_open = host.get_servo().get('lid_open')
        except Exception as e:
            six.reraise(hosts.AutoservNonCriticalVerifyError, str(e),
                        sys.exc_info()[2])

        if lid_open != 'yes' and lid_open != 'not_applicable':
            raise hosts.AutoservNonCriticalVerifyError(
                'Check lid switch: lid_open is %s' % lid_open)

    @property
    def description(self):
        return 'lid_open control is normal'


class _EcBoardVerifier(hosts.Verifier):
    """
    Verifier response from the 'ec_board' control.
    """

    @ignore_exception_for_non_cros_host
    @timeout_util.TimeoutDecorator(cros_constants.SHORT_VERIFY_TIMEOUT_SEC)
    def verify(self, host):
        if host.is_ec_supported():
            ec_board_name = ''
            try:
                ec_board_name = host.get_servo().get_ec_board()
                logging.debug('EC board: %s', ec_board_name)
            except Exception as e:
                raise hosts.AutoservNonCriticalVerifyError(
                        '`ec_board` control is not responding; '
                        'may be caused of broken EC firmware')
        else:
            logging.info('The board not support EC')

    @property
    def description(self):
        return 'Check EC by get `ec_board` control'


class _RestartServod(hosts.RepairAction):
    """Restart `servod` with the proper BOARD setting."""

    @timeout_util.TimeoutDecorator(cros_constants.REPAIR_TIMEOUT_SEC)
    def repair(self, host):
        if not host.is_cros_host():
            raise hosts.AutoservRepairError(
                    'Can\'t restart servod: not running '
                    'embedded Chrome OS.',
                    'servo_not_applicable_to_non_cros_host')
        host.restart_servod()

    @property
    def description(self):
        return 'Start servod with the proper config settings.'


class _ServoRebootRepair(repair_utils.RebootRepair):
    """Try repair servo by reboot servohost.

    This is the same as the standard `RebootRepair`, for servo_v3 it will
    reboot the beaglebone board immidiately while for labstation it will
    request a reboot by touch a flag file on its labstation, then
    labstation reboot will be handled by labstation AdminRepair task as
    labstation host multiple servos and need do an synchronized reboot.
    """

    @timeout_util.TimeoutDecorator(cros_constants.REPAIR_TIMEOUT_SEC)
    def repair(self, host):
        super(_ServoRebootRepair, self).repair(host)
        # restart servod for v3 after reboot.
        host.restart_servod()

    def _is_applicable(self, host):
        if host.is_localhost() or not host.is_cros_host():
            logging.info('Target servo is not in a lab, the reboot repair'
                         ' action is not applicable.')
            return False

        if host.is_labstation():
            host.request_reboot()
            logging.info('Reboot labstation requested, it will be handled'
                         ' by labstation AdminRepair task.')
            return False
        return True

    @property
    def description(self):
        return 'Reboot the servo host.'


class _ToggleCCLineRepair(hosts.RepairAction):
    """Try repair servod by toggle cc.

    When cr50 is not enumerated we can try to recover it by toggle cc line.
    Repair action running from servohost.
    We using usb_console temporally witch required stop servod.

    TODO(otabek@) review the logic when b/159755652 implemented
    """

    @timeout_util.TimeoutDecorator(cros_constants.REPAIR_TIMEOUT_SEC)
    def repair(self, host):
        host.stop_servod()
        self._reset_usbc_pigtail_connection(host)
        host.restart_servod()

    def _is_applicable(self, host):
        if host.is_localhost() or not host.is_labstation():
            return False
        if not host.servo_serial:
            return False
        return self._is_type_c(host)

    def _is_type_c(self, host):
        if host.get_dut_host_info():
            servo_type = host.get_dut_host_info().get_label_value(
                    servo_constants.SERVO_TYPE_LABEL_PREFIX)
            return 'ccd_cr50' in servo_type
        return False

    def _reset_usbc_pigtail_connection(self, host):
        """Reset USBC pigtail connection on servo board.

        To reset need to run 'cc off' and then 'cc srcdts' in usb_console.
        """
        logging.debug('Starting reset USBC pigtail connection.')

        def _run_command(cc_command):
            """Run configuration channel commands.

            @returns: True if pas successful and False if fail.
            """
            try:
                cmd = (r"echo 'cc %s' | usb_console -d 18d1:501b -s %s" %
                       (cc_command, host.servo_serial))
                resp = host.run(cmd, timeout=host.DEFAULT_TERMINAL_TIMEOUT)
                return True
            except Exception as e:
                logging.info('(Non-critical) %s.', e)
            return False

        logging.info('Turn off configuration channel. And wait 5 seconds.')
        if _run_command('off'):
            # wait till command will be effected
            time.sleep(5)
            logging.info('Turn on configuration channel. '
                         'And wait 15 seconds.')
            if _run_command('srcdts'):
                # wait till command will be effected
                time.sleep(15)

    @property
    def description(self):
        return 'Toggle cc lines'


class _ECRebootRepair(hosts.RepairAction):
    """
    Reboot EC on DUT from servo.
    """

    def _is_applicable(self, host):
        return (not host.is_localhost()) and host.is_ec_supported()

    @timeout_util.TimeoutDecorator(cros_constants.REPAIR_TIMEOUT_SEC)
    def repair(self, host):
        host.get_servo().ec_reboot()

    @property
    def description(self):
        return 'Reboot EC'


class _DutRebootRepair(hosts.RepairAction):
    """
    Reboot DUT to recover some servo controls depending on EC console.

    Some servo controls, like lid_open, requires communicating with DUT through
    EC UART console. Failure of this kinds of controls can be recovered by
    rebooting the DUT.
    """

    @timeout_util.TimeoutDecorator(cros_constants.REPAIR_TIMEOUT_SEC)
    def repair(self, host):
        host.get_servo().get_power_state_controller().reset()
        # Get the lid_open value which requires EC console.
        lid_open = host.get_servo().get('lid_open')
        if lid_open != 'yes' and lid_open != 'not_applicable':
            raise hosts.AutoservVerifyError(
                    'Still fail to contact EC console after rebooting DUT')

    @property
    def description(self):
        return 'Reset the DUT via servo'


class _DiskCleanupRepair(hosts.RepairAction):
    """
    Remove old logs/metrics/crash_dumps on servohost to free up disk space.
    """
    KEEP_LOGS_MAX_DAYS = 5

    FILE_TO_REMOVE = [
            '/var/lib/metrics/uma-events', '/var/spool/crash/*',
            '/var/log/chrome/*', '/var/log/ui/*',
            '/home/chronos/BrowserMetrics/*'
    ]

    @timeout_util.TimeoutDecorator(cros_constants.SHORT_REPAIR_TIMEOUT_SEC)
    def repair(self, host):
        if host.is_localhost():
            # we don't want to remove anything from local testing.
            return

        # Remove old servod logs.
        host.run('/usr/bin/find /var/log/servod_* -mtime +%d -print -delete'
                 % self.KEEP_LOGS_MAX_DAYS, ignore_status=True)

        # Remove pre-defined metrics and crash dumps.
        for path in self.FILE_TO_REMOVE:
            host.run('rm %s' % path, ignore_status=True)

    @property
    def description(self):
        return 'Clean up old logs/metrics on servohost to free up disk space.'


class _ServoMicroFlashRepair(hosts.RepairAction):
    """
    Remove old logs/metrics/crash_dumps on servohost to free up disk space.
    """
    _TARGET_SERVO = 'servo_micro'

    @timeout_util.TimeoutDecorator(cros_constants.REPAIR_TIMEOUT_SEC)
    def repair(self, host):
        if not host.is_cros_host():
            raise hosts.AutoservRepairError(
                    'Can\'t restart servod: not running '
                    'embedded Chrome OS.',
                    'servo_not_applicable_to_non_cros_host')
        servo = host.get_servo()
        if not servo or self._TARGET_SERVO not in servo.get_servo_type():
            logging.info("Servo-micro is not present on set-up")
            return

        try:
            servo_updater.update_servo_firmware(host,
                                                boards=(self._TARGET_SERVO, ),
                                                force_update=True,
                                                ignore_version=True)
        except Exception as e:
            logging.debug("(Not critical) Servo device update error: %s", e)
            raise hosts.AutoservVerifyError(
                    'Still fail to contact EC console after rebooting DUT')
        # Update time when we reflashed the fw on the device
        dhp = host.get_dut_health_profile()
        dhp.refresh_servo_miro_fw_update_run_time()
        host.restart_servod()

    def is_time_to_try(self, dhp):
        """Verify that it is time when we can try to re-flash fw on servo_micro.

        Re-flashing limited to once per 2 weeks to avoid over-flashing
        the servo device.
        """
        today_time = int(time.time())
        last_check = dhp.get_servo_micro_fw_update_time_epoch()
        can_run = today_time > (last_check + (14 * 24 * 60 * 60))
        if not can_run:
            logging.info("The servo_micro fw updated in las 2 weeks ago.")
        return can_run

    def _is_applicable(self, host):
        return (not host.is_localhost() and host.get_dut_health_profile()
                and self.is_time_to_try(host.get_dut_health_profile()))

    @property
    def description(self):
        return 'Re-flash servo_micro firmware.'


def create_servo_repair_strategy():
    """
    Return a `RepairStrategy` for a `ServoHost`.
    """
    config = ['brd_config', 'ser_config']
    verify_dag = [
            (repair_utils.SshVerifier, 'servo_ssh', []),
            (_DiskSpaceVerifier, 'disk_space', ['servo_ssh']),
            (_UpdateVerifier, 'update', ['servo_ssh']),
            (_BoardConfigVerifier, 'brd_config', ['servo_ssh']),
            (_SerialConfigVerifier, 'ser_config', ['servo_ssh']),
            (_ServodJobVerifier, 'servod_job', config + ['disk_space']),
            (_TopologyVerifier, 'servo_topology', ['servod_job']),
            (_ServodConnectionVerifier, 'servod_connection', ['servod_job']),
            (_ServodControlVerifier, 'servod_control', ['servod_connection']),
            (_DUTConnectionVerifier, 'dut_connected', ['servod_connection']),
            (_ServoHubConnectionVerifier, 'hub_connected', ['dut_connected']),
            (_PowerButtonVerifier, 'pwr_button', ['hub_connected']),
            (_BatteryVerifier, 'battery', ['hub_connected']),
            (_LidVerifier, 'lid_open', ['hub_connected']),
            (_EcBoardVerifier, 'ec_board', ['dut_connected']),
            (_Cr50ConsoleVerifier, 'cr50_console', ['dut_connected']),
            (_CCDTestlabVerifier, 'ccd_testlab', ['cr50_console']),
            (_CCDPowerDeliveryVerifier, 'power_delivery', ['dut_connected']),
    ]

    servod_deps = [
            'servod_job', 'servo_topology', 'servod_connection',
            'servod_control', 'dut_connected', 'hub_connected', 'pwr_button',
            'cr50_console'
    ]
    repair_actions = [
            (_DiskCleanupRepair, 'disk_cleanup', ['servo_ssh'], ['disk_space'
                                                                 ]),
            (_ServoMicroFlashRepair, 'servo_micro_flash',
             ['servo_ssh', 'servo_topology'], ['dut_connected']),
            (_RestartServod, 'restart', ['servo_ssh'], config + servod_deps),
            (_ServoRebootRepair, 'servo_reboot', ['servo_ssh'], servod_deps),
            (_ToggleCCLineRepair, 'servo_cc', ['servo_ssh'], servod_deps),
            (_DutRebootRepair, 'dut_reboot', ['servod_connection'],
             ['servod_control', 'lid_open', 'ec_board']),
            (_ECRebootRepair, 'ec_reboot', ['servod_connection'],
             ['servod_control', 'lid_open', 'ec_board']),
    ]
    return hosts.RepairStrategy(verify_dag, repair_actions, 'servo')
