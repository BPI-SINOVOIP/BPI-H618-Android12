#!/usr/bin/env python2
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import common
import base
import constants
import servo_updater
import time
import os
import re

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils as client_utils
from autotest_lib.server.cros.storage import storage_validate as storage
from autotest_lib.server.cros import servo_keyboard_utils
from autotest_lib.site_utils.admin_audit import rpm_validator

try:
    from chromite.lib import metrics
except ImportError:
    metrics = client_utils.metrics_mock

# Common status used for statistics.
STATUS_FAIL = 'fail'
STATUS_SUCCESS = 'success'
STATUS_SKIPPED = 'skipped'


class VerifyDutStorage(base._BaseDUTVerifier):
    """Verify the state of the storage on the DUT

    The process to determine the type of storage and read metrics
    of usage and EOL(end-of-life) information to determine the
    state.
    Supported storage types: MMS, NVME, SSD.
    Possible states are:
      UNKNOWN - not access to the DUT, not determine type of storage,
                not information to determine metrics
      NORMAL - the storage is in good shape and will work stable
                device will work stable. (supported for all types)
      ACCEPTABLE - the storage almost used all resources, device will
                work stable but it is better be ready for replacement
                device will work stable. (supported by MMS, NVME)
      NEED_REPLACEMENT - the storage broken or worn off the life limit
                device can work by not stable and can cause the
                flakiness on the tests. (supported by all types)
    """
    def __init__(self, dut_host):
        super(VerifyDutStorage, self).__init__(dut_host)
        self._state = None

    def _verify(self, set_label=True, run_badblocks=None):
        if not self.host_is_up():
            logging.info('Host is down; Skipping the verification')
            return
        try:
            validator = storage.StorageStateValidator(self.get_host())
            storage_type = validator.get_type()
            logging.debug('Detected storage type: %s', storage_type)
            storage_state = validator.get_state(run_badblocks=run_badblocks)
            logging.debug('Detected storage state: %s', storage_state)
            state = self.convert_state(storage_state)
            if state and set_label:
                self._set_host_info_state(constants.DUT_STORAGE_STATE_PREFIX,
                                          state)
                if state == constants.HW_STATE_NEED_REPLACEMENT:
                    self.get_host().set_device_needs_replacement(
                        resultdir=self.get_result_dir())
            self._state = state
        except Exception as e:
            raise base.AuditError('Exception during getting state of'
                                  ' storage %s' % str(e))

    def convert_state(self, state):
        """Mapping state from validator to verifier"""
        if state == storage.STORAGE_STATE_NORMAL:
            return constants.HW_STATE_NORMAL
        if state == storage.STORAGE_STATE_WARNING:
            return constants.HW_STATE_ACCEPTABLE
        if state == storage.STORAGE_STATE_CRITICAL:
            return constants.HW_STATE_NEED_REPLACEMENT
        return None

    def get_state(self):
        return self._state


class VerifyServoUsb(base._BaseServoVerifier):
    """Verify the state of the USB-drive on the Servo

    The process to determine by checking the USB-drive on having any
    bad sectors on it.
    Possible states are:
      UNKNOWN - not access to the device or servo, not available
                software on the servo.
      NORMAL - the device available for testing and not bad sectors.
                was found on it, device will work stable
      NEED_REPLACEMENT - the device available for testing and
                some bad sectors were found on it. The device can
                work but cause flakiness in the tests or repair process.

    badblocks errors:
    No such device or address while trying to determine device size
    """
    def _verify(self):
        if not self.servo_is_up():
            logging.info('Servo not initialized; Skipping the verification')
            return
        try:
            usb = self.get_host()._probe_and_validate_usb_dev()
            logging.debug('USB path: %s', usb)
        except Exception as e:
            usb = ''
            logging.debug('(Not critical) %s', e)
        if not usb:
            self._set_state(constants.HW_STATE_NOT_DETECTED)
            return
        # basic readonly check

        # path to USB if DUT is sshable
        logging.info('Starting verification of USB drive...')
        dut_usb = None
        if self.host_is_up():
            dut_usb = self._usb_path_on_dut()
        state = None
        try:
            if dut_usb:
                logging.info('Try run check on DUT side.')
                state = self._run_check_on_host(self._dut_host, dut_usb)
            else:
                logging.info('Try run check on ServoHost side.')
                servo = self.get_host().get_servo()
                servo_usb = servo.probe_host_usb_dev()
                state = self._run_check_on_host(self.get_host(), servo_usb)
        except Exception as e:
            if 'Timeout encountered:' in str(e):
                logging.info('Timeout during running action')
                metrics.Counter(
                    'chromeos/autotest/audit/servo/usb/timeout'
                    ).increment(fields={'host': self._dut_host.hostname})
            else:
                # badblocks generate errors when device not reachable or
                # cannot read system information to execute process
                state = constants.HW_STATE_NEED_REPLACEMENT
            logging.debug(str(e))

        self._set_state(state)
        logging.info('Finished verification of USB drive.')

        self._install_stable_image()

    def _usb_path_on_dut(self):
        """Return path to the USB detected on DUT side."""
        servo = self.get_host().get_servo()
        servo.switch_usbkey('dut')
        result = self._dut_host.run('ls /dev/sd[a-z]')
        for path in result.stdout.splitlines():
            cmd = ('. /usr/share/misc/chromeos-common.sh; get_device_type %s' %
                   path)
            check_run = self._dut_host.run(cmd, timeout=30, ignore_status=True)
            if check_run.stdout.strip() != 'USB':
                continue
            if self._quick_check_if_device_responsive(self._dut_host, path):
                logging.info('USB drive detected on DUT side as %s', path)
                return path
        return None

    def _quick_check_if_device_responsive(self, host, usb_path):
        """Verify that device """
        validate_cmd = 'fdisk -l %s' % usb_path
        try:
            resp = host.run(validate_cmd, ignore_status=True, timeout=30)
            if resp.exit_status == 0:
                return True
            logging.error('USB %s is not detected by fdisk!', usb_path)
        except error.AutoservRunError as e:
            if 'Timeout encountered' in str(e):
                logging.warning('Timeout encountered during fdisk run.')
            else:
                logging.error('(Not critical) fdisk check fail for %s; %s',
                              usb_path, str(e))
        return False

    def _run_check_on_host(self, host, usb):
        """Run badblocks on the provided host.

        @params host:   Host where USB drive mounted
        @params usb:    Path to USB drive. (e.g. /dev/sda)
        """
        command = 'badblocks -w -e 5 -b 4096 -t random %s' % usb
        logging.info('Running command: %s', command)
        # The response is the list of bad block on USB.
        # Extended time for 2 hour to run USB verification.
        # TODO (otabek@) (b:153661014#comment2) bring F3 to run
        # check faster if badblocks cannot finish in 2 hours.
        result = host.run(command, timeout=7200).stdout.strip()
        logging.info("Check result: '%s'", result)
        if result:
            # So has result is Bad and empty is Good.
            return constants.HW_STATE_NEED_REPLACEMENT
        return constants.HW_STATE_NORMAL

    def _install_stable_image(self):
        """Install stable image to the USB drive."""
        # install fresh image to the USB because badblocks formats it
        # https://crbug.com/1091406
        try:
            logging.debug('Started to install test image to USB-drive')
            _, image_path = self._dut_host.stage_image_for_servo()
            self.get_host().get_servo().image_to_servo_usb(image_path,
                                                           power_off_dut=False)
            logging.debug('Finished installing test image to USB-drive')
        except:
            # ignore any error which happined during install image
            # it not relative to the main goal
            logging.info('Fail to install test image to USB-drive')

    def _set_state(self, state):
        if state:
            self._set_host_info_state(constants.SERVO_USB_STATE_PREFIX, state)


class VerifyServoFw(base._BaseServoVerifier):
    """Force update Servo firmware if it not up-to-date.

    This is rarely case when servo firmware was not updated by labstation
    when servod started. This should ensure that the servo_v4 and
    servo_micro is up-to-date.
    """
    def _verify(self):
        if not self.servo_host_is_up():
            logging.info('Servo host is down; Skipping the verification')
            return
        servo_updater.update_servo_firmware(
            self.get_host(),
            force_update=True)


class VerifyRPMConfig(base._BaseDUTVerifier):
    """Check RPM config of the setup.

    This check run against RPM configs settings.
    """

    def _verify(self):
        if not self.host_is_up():
            logging.info('Host is down; Skipping the verification')
            return
        rpm_validator.verify_unsafe(self.get_host())


class FlashServoKeyboardMapVerifier(base._BaseDUTVerifier):
    """Flash the keyboard map on servo."""

    _ATMEGA_RESET_DELAY = 0.2
    _USB_PRESENT_DELAY = 1

    # Command to detect LUFA Keyboard Demo by VID.
    LSUSB_CMD = 'lsusb -d %s:' % servo_keyboard_utils.ATMEL_USB_VENDOR_ID

    def _verify(self):
        if not self.host_is_up():
            logging.info('Host is down; Skipping the action')
            return
        if not self.servo_is_up():
            logging.info('Servo not initialized; Skipping the action')
            return

        host = self.get_host()
        servo = host.servo
        try:
            logging.info('Starting flashing the keyboard map.')
            status = self._flash_keyboard_map(host, servo)
            logging.info('Set status: %s', status)
            if status == STATUS_FAIL:
                self._send_metrics()
        except Exception as e:
            # The possible errors is timeout of commands.
            logging.debug('Failed to flash servo keyboard map; %s', e)
            self._send_metrics()
        finally:
            # Restore the default settings.
            # Select the chip on the USB mux unless using Servo V4
            if 'servo_v4' not in servo.get_servo_version():
                servo.set('usb_mux_sel4', 'on')

    def _flash_keyboard_map(self, host, servo):
        if host.run('hash dfu-programmer', ignore_status=True).exit_status:
            logging.info(
                'The image is too old that does not have dfu-programmer.')
            return STATUS_SKIPPED

        servo.set_nocheck('init_usb_keyboard', 'on')

        if self._is_keyboard_present(host):
            logging.info('Already using the new keyboard map.')
            return STATUS_SUCCESS

        # Boot AVR into DFU mode by enabling the HardWareBoot mode
        # strapping and reset.
        servo.set_get_all(['at_hwb:on',
                            'atmega_rst:on',
                            'sleep:%f' % self._ATMEGA_RESET_DELAY,
                            'atmega_rst:off',
                            'sleep:%f' % self._ATMEGA_RESET_DELAY,
                            'at_hwb:off'])

        result = host.run(self.LSUSB_CMD, timeout=30).stdout.strip()
        if not 'Atmel Corp. atmega32u4 DFU bootloader' in result:
            logging.info('Not an expected chip: %s', result)
            return STATUS_FAIL

        # Update the keyboard map.
        bindir = os.path.dirname(os.path.realpath(__file__))
        local_path = os.path.join(bindir, 'data', 'keyboard.hex')
        host.send_file(local_path, '/tmp')
        logging.info('Updating the keyboard map...')
        host.run('dfu-programmer atmega32u4 erase --force', timeout=120)
        host.run('dfu-programmer atmega32u4 flash /tmp/keyboard.hex',
                 timeout=120)

        # Reset the chip.
        servo.set_get_all(['atmega_rst:on',
                            'sleep:%f' % self._ATMEGA_RESET_DELAY,
                            'atmega_rst:off'])
        if self._is_keyboard_present(host):
            logging.info('Update successfully!')
            return STATUS_SUCCESS

        logging.info('Update failed!')
        return STATUS_FAIL

    def _is_keyboard_present(self, host):
        # Check the result of lsusb.
        time.sleep(self._USB_PRESENT_DELAY)
        result = host.run(self.LSUSB_CMD, timeout=30).stdout.strip()
        logging.info('got the result: %s', result)
        if ('LUFA Keyboard Demo' in result and
            servo_keyboard_utils.is_servo_usb_wake_capable(host)):
            return True
        return False

    def _send_metrics(self):
        host = self.get_host()
        data = {'host': host.hostname, 'status': STATUS_FAIL}
        metrics.Counter(
            'chromeos/autotest/audit/servo_keyboard').increment(fields=data)


class VerifyDUTMacAddress(base._BaseDUTVerifier):
    """Verify and update cached NIC mac address on servo.

    Servo_v4 plugged to the DUT and providing NIC for that. We caching mac
    address on servod side to better debugging.
    """

    # HUB and NIC VID/PID.
    # Values presented as the string of the hex without 0x to match
    # representation in sysfs (idVendor/idProduct).
    HUB_VID = '04b4'
    HUB_PID = '6502'
    NIC_VID = '0bda'
    NIC_PID = '8153'

    # Regex to check mac address format.
    # eg: f4:f5:e8:50:e9:45
    RE_MACADDR = re.compile('^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$')

    def _verify(self):
        if not self.host_is_up():
            logging.info('Host is down; Skipping the action')
            return
        if not self.servo_is_up():
            logging.info('Servo host is down; Skipping the action')
            return
        host = self.get_host()
        servo = host.servo
        if not host._servo_host.is_labstation():
            logging.info('Only servo_v4 has NIC; '
                         'Skipping the action')
            return
        if not servo.has_control('macaddr'):
            logging.info('"macaddr" control not supported;'
                         'Skipping the action')
            return

        # Path to the NIC has to be located in the HUB.
        # eg.
        # HUB: /sys/bus/usb/devices/1-1
        # NIC: /sys/bus/usb/devices/1-1.1
        hub_path = self._get_device_path(None, self.HUB_VID, self.HUB_PID)
        if not hub_path or hub_path == '.':
            logging.info('The servo_v4 HUB not detected from DUT')
            self._send_metrics()
            return
        logging.info('Path to the servo_v4 HUB device: %s', hub_path)
        nic_path = self._get_device_path(hub_path, self.NIC_VID, self.NIC_PID)
        if not nic_path or nic_path == '.':
            logging.info('The servo_v4 NIC not detected in HUB folder')
            self._send_metrics()
            return
        logging.info('Path to the servo_v4 NIC device: %s', nic_path)
        if hub_path == nic_path or not nic_path.startswith(hub_path):
            logging.info('The servo_v4 NIC was detect out of servo_v4 HUB;'
                         ' Skipping the action.')
            self._send_metrics()
            return

        macaddr = self._get_mac_address(host, nic_path)
        if not macaddr:
            self._send_metrics()
            return

        cached_mac = self._get_cached_mac_address()
        if not cached_mac or macaddr != cached_mac:
            try:
                servo.set('macaddr', macaddr)
                logging.info('Successfully updated the servo "macaddr"!')
            except error.TestFail as e:
                logging.debug('Fail to update macaddr value; %s', e)
                logging.info('Fail to update the "macaddr" value!')
                self._send_metrics()
        else:
            logging.info('The servo "macaddr" doe not need update.')

    def _get_cached_mac_address(self):
        try:
            return self.get_host().servo.get('macaddr')
        except error.TestFail as e:
            logging.error('(Non-critical) Fail to get macaddr: %s', e)
            return None

    def _get_mac_address(self, host, nic_path):
        cmd = r'find %s/ | grep /net/ | grep /address' % nic_path
        res = host.run(cmd,
                       timeout=30,
                       ignore_status=True,
                       ignore_timeout=True)
        if not res:
            logging.info('Timeout during retriving NIC address files.')
            return None
        addrs = res.stdout.splitlines()
        if not addrs or len(addrs) == 0:
            logging.info('No NIC address file found.')
            return None
        if len(addrs) > 1:
            logging.info('More than one NIC address file found.')
            return None
        logging.info('Found NIC address file: %s', addrs[0])
        cmd = r'cat %s' % addrs[0]
        res = host.run(cmd,
                       timeout=30,
                       ignore_status=True,
                       ignore_timeout=True)
        if not res:
            logging.info('Timeout during attemp read NIC address file: %s',
                         addrs[0])
            return None
        mac_addr = res.stdout.strip()
        if not self.RE_MACADDR.match(mac_addr):
            logging.info('incorrect format of the mac address: %s', mac_addr)
            return None
        logging.info('Servo_v4 NIC mac address from DUT side: %s', mac_addr)
        return mac_addr

    def _get_device_path(self, base_path, vid, pid):
        """Find a device by VID/PID under particular path.

        1) Get path to the unique idVendor file with VID
        2) Get path to the unique idProduct file with PID
        3) Get directions of both file and compare them

        @param base_path:   Path to the directory where to look for the device.
        @param vid:         Vendor ID of the looking device.
        @param pid:         Product ID of the looking device.

        @returns: path to the folder of the device
        """
        host = self.get_host()
        def _run(cmd):
            res = host.run(cmd, timeout=30,
                           ignore_status=True,
                           ignore_timeout=True)
            l = res.stdout.splitlines()
            if not l or len(l) != 1:
                return None
            return l[0]

        if not base_path:
            base_path = '/sys/bus/usb/devices/*/'
        else:
            base_path += '*/'
        cmd_template = 'grep -l %s $(find %s -maxdepth 1 -name %s)'
        vid_path = _run(cmd_template % (vid, base_path, 'idVendor'))
        if not vid_path:
            return None

        pid_path = _run(cmd_template % (pid, base_path, 'idProduct'))
        if not pid_path:
            return None

        # check if both files locates in the same folder
        return _run('LC_ALL=C comm -12 <(dirname %s) <(dirname %s)' %
                    (vid_path, pid_path))

    def _send_metrics(self):
        host = self.get_host()
        data = {'host': host.hostname, 'status': STATUS_FAIL}
        metrics.Counter(
            'chromeos/autotest/audit/servo_macaddr').increment(fields=data)
