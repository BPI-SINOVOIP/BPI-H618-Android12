# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""The autotest performing FW update, both EC and AP in CCD mode."""
import logging
import re

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_ECRestoreFW(FirmwareTest):
    """A test that restores a machine from an incrrect FW to backup."""

    version = 1

    # A set of fake board candidates per ec type.
    FAKE_BOARD_DICT = {'npcx':['coral', 'reef'],
                       'stm32':['samus', 'nami'],
                       'it83':['dragonegg', 'waddledee']}

    def initialize(self, host, cmdline_args, full_args):
        """Initialize the test and pick a fake board to use for corruption. """
        super(firmware_ECRestoreFW, self).initialize(host, cmdline_args,
                                                   full_args)

        # Don't bother if there is no Chrome EC.
        if not self.check_ec_capability():
            raise error.TestNAError('Nothing needs to be tested on this device')

        self.board_as = None
        # find if "board_as" was given in the command line arguments.
        for arg in cmdline_args:
            match = re.search(r'^board_as=(.+)', arg)
            if match:
                self.board_as = match.group(1)
                break
        else:
            # if "board_as" was not given, then pick one from FAKE_BOARD_DICT.
            ec_chip = self.servo.get('ec_chip')
            if 'stm32' in ec_chip:
                ec_type = 'stm32'
            elif 'it83' in ec_chip:
                ec_type = 'it83'
            else:
                ec_type = 'npcx'

            for board in self.FAKE_BOARD_DICT[ec_type]:
                if board not in self.faft_config.platform:
                    self.board_as = board
                    break

        if not self.board_as:
            raise error.TestError('fake board is not selected.')

        logging.info('A fake board to use for corruption: %s', self.board_as)

        self.backup_firmware()

    def cleanup(self):
        """The method called by the control file to start the test.

        Raises:
          TestFail: if the firmware restoration fails.
        """
        try:
            if self.is_firmware_saved():
                self.restore_firmware()
        except Exception as e:
            raise error.TestFail('FW Restoration failed: %s' % str(e))
        finally:
            super(firmware_ECRestoreFW, self).cleanup()

    def run_once(self, host):
        """The method called by the control file to start the test.

        Args:
          host:  a CrosHost object of the machine to update.
        """

        logging.info('Downloading a firmware of %s', self.board_as)
        value = host.get_latest_release_version(self.board_as)
        if not value:
            raise error.TestError('Cannot locate the latest release for %s' %
                                  self.board_as)

        try:
            host.firmware_install(build=value, dest=self.resultsdir,
                                  install_ec=True, install_bios=False,
                                  board_as=self.board_as)
        except error.TestError as e:
            # It failed before the test attempts to install firmware.
            # It could be either devserver timeout or servo device error.
            # Let this test fail in those cases.
            raise e
        except Exception as e:
            # Nothing can be guaranteed with the firmware corruption with wrong
            # firmware. Let's not this test fail for that.
            logging.info('Caught an exception: %s', e)
