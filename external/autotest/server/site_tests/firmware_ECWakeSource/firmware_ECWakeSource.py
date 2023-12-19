# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest
from autotest_lib.server.cros.faft.firmware_test import ConnectionError
from autotest_lib.server.cros.servo import servo


class firmware_ECWakeSource(FirmwareTest):
    """
    Servo based EC wake source test.
    """
    version = 1

    # After suspending the device, wait this long before waking it again.
    SUSPEND_WAIT_TIME_SECONDS = 5
    # Retries allowed for reaching S0ix|S3 after suspend and S0 after wake.
    POWER_STATE_RETRY_COUNT = 10
    # The timeout (in seconds) to confirm the device is woken up from
    # suspend mode.
    RESUME_TIMEOUT = 60
    # Delay before the USB keyboard is seen by DUT after initialization
    USB_PRESENT_DELAY = 1

    def initialize(self, host, cmdline_args):
        super(firmware_ECWakeSource, self).initialize(host, cmdline_args)
        # Only run in normal mode
        self.switcher.setup_mode('normal')
        self.has_internal_display = host.has_internal_display()

    def cleanup(self):
        # Restore the lid_open switch in case the test failed in the middle.
        if self.check_ec_capability(['lid']):
            self.servo.set('lid_open', 'yes')
        super(firmware_ECWakeSource, self).cleanup()

    def hibernate_and_wake_by_power_button(self, host):
        """Shutdown to G3/S5, hibernate EC, and then wake by power button."""
        is_ac = host.is_ac_connected()
        self.run_shutdown_cmd()
        if not self.wait_power_state(self.POWER_STATE_G3,
                                     self.POWER_STATE_RETRY_COUNT):
            raise error.TestFail('Platform failed to reach G3 state.')

        self.ec.send_command('hibernate')
        time.sleep(self.WAKE_DELAY)

        # If AC is plugged during the test, the DUT would wake up right after
        # entering hibernate mode. So skip the verification for EC console
        # responsiveness.
        if is_ac != True and self.is_ec_console_responsive():
            raise error.TestFail('The DUT is not in hibernate mode.')
        self.servo.power_short_press()
        self.switcher.wait_for_client()

    def is_ec_console_responsive(self):
        """Test if EC console is responsive."""
        try:
            self.ec.send_command_get_output('help', ['.*>'])
            return True
        except servo.UnresponsiveConsoleError:
            return False

    def wake_by_lid_switch(self):
        """Wake up the device by lid switch."""
        self.servo.set('lid_open', 'no')
        time.sleep(self.LID_DELAY)
        self.servo.set('lid_open', 'yes')

    def suspend_and_wake(self, suspend_func, wake_func):
        """Suspend and then wake up the device.

        Args:
            suspend_func: The method used to suspend the device
            wake_func: The method used to resume the device
        """
        suspend_func()
        self.switcher.wait_for_client_offline()
        if not self.wait_power_state(self.POWER_STATE_SUSPEND,
                                     self.POWER_STATE_RETRY_COUNT):
            raise error.TestFail('Platform failed to reach S0ix or S3 state.')
        time.sleep(self.SUSPEND_WAIT_TIME_SECONDS);
        wake_func()
        if not self.wait_power_state(self.POWER_STATE_S0,
                                     self.POWER_STATE_RETRY_COUNT):
            raise error.TestFail('Platform failed to reach S0 state.')
        self.switcher.wait_for_client(timeout=self.RESUME_TIMEOUT)

    def suspend_and_dont_wake(self, suspend_func, wake_func):
        """Suspend and then check the the device doesn't wake up.

        Args:
            suspend_func: The method used to suspend the device
            wake_func: The method used to wake the device
        """
        suspend_func()
        self.switcher.wait_for_client_offline()
        if not self.wait_power_state(self.POWER_STATE_SUSPEND,
                                     self.POWER_STATE_RETRY_COUNT):
            raise error.TestFail('Platform failed to reach S0ix or S3 state.')
        time.sleep(self.SUSPEND_WAIT_TIME_SECONDS)
        wake_func()
        if self.wait_power_state(self.POWER_STATE_S0,
                                 self.POWER_STATE_RETRY_COUNT):
            raise error.TestFail('Platform woke up unexpectedly.')
        else:
            self.servo.power_normal_press()
            if not self.wait_power_state(self.POWER_STATE_S0,
                                         self.POWER_STATE_RETRY_COUNT):
                raise error.TestFail('Platform failed to reach S0 state.')
        self.switcher.wait_for_client(timeout=self.RESUME_TIMEOUT)

    def run_once(self, host):
        """Runs a single iteration of the test."""
        # Login as a normal user and stay there, such that closing lid triggers
        # suspend, instead of shutdown.
        autotest_client = autotest.Autotest(host)
        autotest_client.run_test('desktopui_SimpleLogin',
                                 exit_without_logout=True)
        original_boot_id = host.get_boot_id()

        # With no display connected, pressing the power button in suspend mode
        # would lead to shutdown.
        if self.has_internal_display:
            logging.info('Suspend and wake by power button.')
            self.suspend_and_wake(self.suspend, self.servo.power_normal_press)

        if not self.check_ec_capability(['keyboard']):
            logging.info('The device has no internal keyboard. '
                         'Skip testing suspend/resume by internal keyboard.')
        elif not self.ec.has_command('ksstate'):
            logging.info('The device does not support the ksstate command. '
                         'Skip testing suspend/resume by internal keyboard.')
        else:
            result = self.ec.send_command_get_output(
                    'ksstate',
                    ['Keyboard scan disable mask: 0x([0-9a-fA-F]{8})'])
            kb_scan_disable_mask = int(result[0][1], 16)
            if kb_scan_disable_mask == 0:
                logging.info('Suspend and wake by internal key press.')
                self.suspend_and_wake(self.suspend,
                                      lambda: self.ec.key_press('<enter>'))
            else:
                logging.info('Tablet mode enabled; suspend and check device '
                             'does not wake by internal key press.')
                self.suspend_and_dont_wake(
                        self.suspend, lambda: self.ec.key_press('<enter>'))

        if not self.faft_config.usb_hid_wake_enabled:
            logging.info('Device does not support wake by USB HID. '
                         'Skip suspend and wake by USB HID key press.')
        else:
            logging.info('Suspend and wake by USB HID key press.')

            logging.debug('Initializing HID keyboard emulator.')
            self.servo.set_nocheck('init_usb_keyboard', 'on')
            time.sleep(self.USB_PRESENT_DELAY)

            try:
                self.suspend_and_wake(self.suspend,
                        lambda:self.servo.set_nocheck('usb_keyboard_enter_key',
                                                      'press'))
            except ConnectionError:
                raise error.TestFail(
                        'USB HID suspend/resume fails. Maybe try to '
                        'update firmware for Atmel USB KB emulator by running '
                        'firmware_FlashServoKeyboardMap test and then try again?'
                )

            logging.debug('Turning off HID keyboard emulator.')
            self.servo.set_nocheck('init_usb_keyboard', 'off')

        if not self.check_ec_capability(['lid']):
            logging.info('The device has no lid. '
                         'Skip testing suspend/resume by lid switch.')
        else:
            logging.info('Suspend and wake by lid switch.')
            self.suspend_and_wake(self.suspend, self.wake_by_lid_switch)
            logging.info('Close lid to suspend and wake by lid switch.')
            self.suspend_and_wake(lambda:self.servo.set('lid_open', 'no'),
                                  self.wake_by_lid_switch)

        boot_id = host.get_boot_id()
        if boot_id != original_boot_id:
            raise error.TestFail('Different boot_id. Unexpected reboot.')

        if self.servo.main_device_is_ccd():
            logging.info('With CCD, we can\'t wake up the DUT from hibernate '
                         'by power button. Skip hibernate test.')
        elif not self.faft_config.ec_has_hibernate_cmd:
            logging.info('EC does not support hibernate. Skip hibernate test.')
        elif not self.has_internal_display:
            logging.info('For the form factors without internal display, '
                         'hibernate is not useful. Skip hibernate test.')
        else:
            logging.info('EC hibernate and wake by power button.')
            self.hibernate_and_wake_by_power_button(host)
