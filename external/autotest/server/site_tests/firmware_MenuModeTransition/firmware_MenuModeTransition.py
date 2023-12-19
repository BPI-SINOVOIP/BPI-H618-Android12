# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class BaseMenuNavigator:
    """Base class for menu navigator."""

    def __init__(self, test):
        self.test = test
        self.faft_config = self.test.faft_config
        self.servo = self.test.servo

    def menu_up(self):
        """Navigate up in the menu."""
        if self.faft_config.is_detachable:
            self.servo.set_nocheck('volume_up_hold', 100)
        else:
            self.servo.arrow_up()

    def menu_down(self):
        """Navigate down in the menu."""
        if self.faft_config.is_detachable:
            self.servo.set_nocheck('volume_down_hold', 100)
        else:
            self.servo.arrow_down()

    def menu_select(self, msg=None):
        """Select a menu item."""
        if msg:
            logging.info(msg)
        if self.faft_config.is_detachable:
            self.servo.power_short_press()
        else:
            self.servo.enter_key()


class LegacyMenuNavigator(BaseMenuNavigator):
    """Menu navigator for legacy menu UI.

    The "legacy menu UI" is an old menu-based UI, which has been replaced
    by the new one, called "menu UI".
    """

    def trigger_rec_to_dev(self):
        """Trigger to-dev transition."""
        self.test.switcher.trigger_rec_to_dev()

    def dev_boot_from_internal(self):
        """Boot from internal disk in developer mode.

        Menu items in developer warning screen:
            0. Developer Options
            1. Show Debug Info
            2. Enable OS Verification
           *3. Power Off
            4. Language

        (*) is the default selection.
        """
        self.test.wait_for('firmware_screen')
        for _ in range(3, 0, -1):
            self.menu_up()
            self.test.wait_for('confirm_screen')
        self.menu_select('Selecting "Developer Options"...')
        self.test.wait_for('confirm_screen')
        self.menu_select('Selecting "Boot From Internal Disk"...')

    def trigger_dev_to_normal(self):
        """Trigger dev-to-norm transition.

        Menu items in developer warning screen:
            0. Developer Options
            1. Show Debug Info
            2. Enable OS Verification
           *3. Power Off
            4. Language

        Menu items in to-norm confirmation screen:
           *0. Confirm Enabling OS Verification
            1. Cancel
            2. Power Off
            3. Language

        (*) is the default selection.
        """
        self.test.wait_for('firmware_screen')
        for _ in range(3, 2, -1):
            self.menu_up()
            self.test.wait_for('confirm_screen')
        self.menu_select('Selecting "Enable OS Verification"...')
        self.test.wait_for('confirm_screen')
        self.menu_select('Selecing "Confirm Enabling OS Verification"...')


class MenuNavigator(BaseMenuNavigator):
    """Menu navigator for menu UI.

    The "menu UI" aims to replace both "legacy clamshell UI" and "legacy
    menu UI". See chromium:1033815 for the discussion about the naming.
    """

    def _confirm_to_dev(self):
        if self.faft_config.rec_button_dev_switch:
            logging.info('Confirm to-dev by RECOVERY button')
            self.servo.toggle_recovery_switch()
        elif self.faft_config.power_button_dev_switch:
            logging.info('Confirm to-dev by POWER button')
            self.servo.power_normal_press()
        else:
            self.menu_select('Confirm to-dev by menu selection')

    def trigger_rec_to_dev(self):
        """Trigger to-dev transition.

        Menu items in recovery select screen:
            0. Language
            1. Recovery using phone
            2. Recovery using external disk
            3. Launch diagnostics
            4. Advanced options
            5. Power off

        Menu items in advanced options screen:
            0. Language
           *1. Enable developer mode
            2. Back
            3. Power off

        Menu items in to-dev screen:
            0. Language
           *1. Confirm
            2. Cancel
            3. Power off

        (*) is the default selection.
        """
        self.test.wait_for('firmware_screen')
        # Since the default selection is unknown, navigate to item 5 first
        for _ in range(0, 5):
            self.menu_down()
            self.test.wait_for('confirm_screen')
        # Navigate to "Advanced options"
        self.menu_up()
        self.test.wait_for('confirm_screen')
        self.menu_select('Selecting "Advanced options"...')
        self.test.wait_for('confirm_screen')
        self.menu_select('Selecting "Enable developer mode"...')
        self.test.wait_for('confirm_screen')
        # Confirm to-dev transition
        self._confirm_to_dev()

    def dev_boot_from_internal(self):
        """Boot from internal disk in developer mode.

        Menu items in developer mode screen:
            0. Language
            1. Return to secure mode
            2. Boot from internal disk
            3. Boot from external disk
            4. Advanced options
            5. Power off
        """
        self.test.wait_for('firmware_screen')
        # Since the default selection is unknown, navigate to item 0 first
        for _ in range(5, 0, -1):
            self.menu_up()
            self.test.wait_for('confirm_screen')
        # Navigate to "Boot from internal disk"
        for _ in range(0, 2):
            self.menu_down()
            self.test.wait_for('confirm_screen')
        self.menu_select('Selecting "Boot from internal disk"...')

    def trigger_dev_to_normal(self):
        """Trigger dev-to-norm transition.

        Menu items in developer mode screen:
            0. Language
            1. Return to secure mode
            2. Boot from internal disk
            3. Boot from external disk
            4. Advanced options
            5. Power off

        Menu items in to-norm screen:
            0. Language
           *1. Confirm
            2. Cancel
            3. Power off

        (*) is the default selection.
        """
        self.test.wait_for('firmware_screen')
        # Since the default selection is unknown, navigate to item 0 first
        for _ in range(5, 0, -1):
            self.menu_up()
            self.test.wait_for('confirm_screen')
        # Navigate to "Return to secure mode"
        self.menu_down()
        self.test.wait_for('confirm_screen')
        self.menu_select('Selecting "Return to secure mode"...')
        self.test.wait_for('confirm_screen')
        self.menu_select('Selecing "Confirm"...')


class firmware_MenuModeTransition(FirmwareTest):
    """
    Servo based test for manual mode transitions through the UI menu.
    """
    version = 1

    def initialize(self, host, cmdline_args, ec_wp=None):
        super(firmware_MenuModeTransition, self).initialize(
                host, cmdline_args, ec_wp=ec_wp)
        self.switcher.setup_mode('normal')
        self.setup_usbkey(usbkey=False)

    def run_once(self):
        """Method which actually runs the test."""
        self.check_state((self.checkers.mode_checker, 'normal'))

        if self.faft_config.mode_switcher_type == 'menu_switcher':
            navigator = MenuNavigator(self)
        elif (self.faft_config.mode_switcher_type ==
              'tablet_detachable_switcher'):
            navigator = LegacyMenuNavigator(self)
        else:
            raise error.TestNAError('Test skipped for menuless UI')

        # Trigger to-dev by menu navigation
        logging.info('Trigger to-dev by menu navigation.')
        self.switcher.enable_rec_mode_and_reboot(usb_state='host')
        self.switcher.wait_for_client_offline()
        navigator.trigger_rec_to_dev()

        # Now the device should be in dev mode screen
        navigator.dev_boot_from_internal()
        self.switcher.wait_for_client()

        logging.info('Expected dev mode boot.')
        self.check_state((self.checkers.mode_checker, 'dev'))

        # Trigger to-norm by menu navigation
        logging.info('Trigger to-norm by menu navigation.')
        self.switcher.disable_rec_mode_and_reboot()
        self.switcher.wait_for_client_offline()
        navigator.trigger_dev_to_normal()
        self.switcher.wait_for_client()

        logging.info('Expected normal mode boot, done.')
        self.check_state((self.checkers.mode_checker, 'normal'))
