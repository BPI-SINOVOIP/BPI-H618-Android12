# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, re
import os
import xmlrpclib

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.server.cros.faft.firmware_test import FirmwareTest


class firmware_CsmeFwUpdate(FirmwareTest):
    """
    This tests csme rw firmware update feature by changing the me_rw
    image in firmware main regions with a different version

    Accepted --args names:
    old_bios = specify this argument to use a different bios
                than shellball default for downgrade

    """
    version = 1
    ORIGINAL_BIOS = "/usr/local/tmp/bios_original.bin"
    DOWNGRADE_BIOS = "/usr/local/tmp/bios_downgrade.bin"
    # Region to use for flashrom wp-region commands
    WP_REGION = 'WP_RO'
    MODE = 'recovery'

    def initialize(self, host, cmdline_args, dev_mode = False):
        # Parse arguments from command line
        dict_args = utils.args_to_dict(cmdline_args)
        super(firmware_CsmeFwUpdate, self).initialize(host, cmdline_args)

        self.bios_input = None
        self.restore_required = False
        self.downgrade_bios = None
        self.spi_bios = None
        self._orig_sw_wp = None
        self._original_hw_wp = None
        arg_name = "old_bios"
        arg_value = dict_args.get(arg_name)
        if arg_value:
            logging.info('%s=%s', arg_name, arg_value)
            image_path = os.path.expanduser(arg_value)
            if not os.path.isfile(image_path):
                raise error.TestError(
                        "Specified file does not exist: %s=%s"
                        % (arg_name, image_path))
            self.bios_input = image_path
        else:
            logging.info("No bios specified. Using default " \
                        "shellball bios for downgrade")

        self.backup_firmware()
        self.switcher.setup_mode('dev' if dev_mode else 'normal')

        # Save write protect configuration and enable it
        logging.info("Enabling Write protection")
        self._orig_sw_wp = self.faft_client.bios.get_write_protect_status()
        self._original_hw_wp = 'on' in self.servo.get('fw_wp_state')
        self.set_ap_write_protect_and_reboot(False)
        self.faft_client.bios.set_write_protect_region(self.WP_REGION, True)
        self.set_ap_write_protect_and_reboot(True)

        # Make sure that the shellball is retained over subsequent power cycles
        self.blocking_sync()

    def cleanup(self):
        """
        Flash the backed up firmware at the end of test

        """
        self.faft_client.system.remove_file(self.ORIGINAL_BIOS)
        self.faft_client.system.remove_file(self.DOWNGRADE_BIOS)
        self.set_ap_write_protect_and_reboot(False)

        try:
            if self.is_firmware_saved() and self.restore_required:
                logging.info("Restoring Original Image")
                self.restore_firmware()
        except (EnvironmentError, xmlrpclib.Fault,
                error.AutoservError, error.TestBaseException):
            logging.error("Problem restoring firmware:", exc_info=True)

        try:
            # Restore the old write-protection value at the end of the test.
            logging.info("Restoring write protection configuration")
            if self._orig_sw_wp:
                self.faft_client.bios.set_write_protect_range(
                        self._orig_sw_wp['start'],
                        self._orig_sw_wp['length'],
                        self._orig_sw_wp['enabled'])
        except (EnvironmentError, xmlrpclib.Fault,
                error.AutoservError, error.TestBaseException):
            logging.error("Problem restoring software write-protect:",
                          exc_info = True)

        if self._original_hw_wp is not None:
            self.set_ap_write_protect_and_reboot(self._original_hw_wp)

        self.switcher.mode_aware_reboot(reboot_type = 'cold')
        super(firmware_CsmeFwUpdate, self).cleanup()

    def read_current_bios_and_save(self):
        """
        Dumps current bios from spi to two file.(working copy and backup)

        @returns the working copy file path

        """
        # Dump the current spi bios to file
        self.spi_bios = self.ORIGINAL_BIOS
        logging.info("Copying current bios image to %s for upgrade " \
                     "test" % self.spi_bios)
        self.faft_client.bios.dump_whole(self.spi_bios)

        # Get the downgrade bios image from user or from shellball
        self.downgrade_bios = self.DOWNGRADE_BIOS
        if self.bios_input:
            logging.info("Copying user given bios image to %s for downgrade " \
                    "test" % self.downgrade_bios)
            self._client.send_file(self.bios_input, self.downgrade_bios)
        else:
            logging.info("Copying bios image from update shellball to %s " \
                    "for downgrade test" % self.downgrade_bios)
            self.faft_client.updater.extract_shellball()
            cbfs_work_dir = self.faft_client.updater.cbfs_setup_work_dir()
            shellball_bios = os.path.join(cbfs_work_dir,
                    self.faft_client.updater.get_bios_relative_path())
            command = "cp %s %s" % (shellball_bios, self.downgrade_bios)
            self.faft_client.system.run_shell_command(command)

    def check_fmap_format(self, image_path):
        """
        Checks FMAP format used by the Image for CSME update

        @param image_path: path of the image
        @returns the fmap format string

        """
        # Check if ME_RW_A is present in the image
        logging.info("Checking if seperate CBFS is used for CSE RW in " \
                     "image : %s" % image_path)
        command = "futility dump_fmap -F %s | grep ME_RW_A" % image_path
        output = self.faft_client.system.run_shell_command_get_output(
                    command, True)
        if output:
            logging.info("Image uses seperate CBFS for CSE RW")
            return "CSE_RW_SEPARATE_CBFS"
        else:
            return "DEFAULT"

    def check_if_me_blob_exist_in_image(self, image_path):
        """
        Checks if me_blob exists in FW MAIN section of an image

        @param image_path: path of the image
        @returns True if present else False

        """
        # Check if me_rw.metadata present FW_MAIN region
        logging.info("Checking if me_rw.metadata file " \
                     "present in image : %s" % image_path )
        command = "cbfstool %s print -r FW_MAIN_A " \
                            "| grep me_rw.metadata" % image_path
        output = self.faft_client.system.run_shell_command_get_output(
                    command, True)
        if output:
            available = True
            logging.info("me_rw.metadata present in image")
        else:
            available = False
            logging.info("me_rw.metadata not present in image")

        return available

    def extract_me_rw_version_from_bin(self, me_blob, version_offset = 0):
        """
        Extract me_rw version from given me_rw blob. Version is first 8
        bytes in the blob

        @param me_blob: me_rw blob (old fmap) or me_rw_metadata blob
        @param version_offset: version filed offset in the blob
        @returns the CSME RW version string

        """
        ver_res = ""
        logging.info("Extracting version field from ME blob")
        command = ("hexdump -n 8 -s %s %s | cut -c 9- |sed 's/ //g' |" \
                   "sed 's/.\{4\}/&./g;s/ $//' | head -c19" % ( \
                    str(int(version_offset)), me_blob))
        output = self.faft_client.system.run_shell_command_get_output(
                    command, True)
        for each_word in output[0].split("."):
            version = (int(each_word, 16))
            ver_res = "".join((ver_res, "".join((str(version),"."))))
        ver_res = ver_res[:-1]
        logging.info("Version : %s" % ver_res)
        return ver_res

    def get_image_fwmain_me_rw_version(self,
                                       bios,
                                       region = "FW_MAIN_A"):
        """
        Extract CSME RW version of the me_rw blob of the given
        region in the given bios

        @param bios: Bios path
        @param region: region which contains me_rw blob
        @returns the CSME RW version string

        """
        # Extract me_rw.metadata and check version.
        cbfs_name = "me_rw.metadata"
        temp_dir = self.faft_client.system.create_temp_dir()
        me_blob = os.path.join(temp_dir, cbfs_name)

        cmd_status = self.faft_client.updater.cbfs_extract(cbfs_name,
                                                       '',(region, ),
                                                   me_blob,'x86',bios)

        if cmd_status is None:
            self.faft_client.system.remove_dir(temp_dir)
            raise error.TestError("Failed to extract ME blob from " \
                                    "the given bios : %s" % bios)

        version = self.extract_me_rw_version_from_bin(me_blob)
        self.faft_client.system.remove_dir(temp_dir)
        return version

    def get_current_me_rw_version(self):
        """
        Reads the current active CSME RW Version from coreboot logs

        @returns the CSME RW version string

        """
        logging.info("Extracting cselite version info from coreboot logs")
        command = "cbmem -1 | grep 'cse_lite:'"
        output = self.faft_client.system.run_shell_command_get_output(
                    command, True)
        logging.info(output)
        # Offset of rw portion in ME region
        me_cse_rw_info = re.search(r"(cse_lite: RW version = )" \
                    "([0-9]*\.[0-9]*\.[0-9]*\.[0-9]*)","".join(output))

        if me_cse_rw_info:
            me_version = me_cse_rw_info.group(2)
        else:
            raise error.TestError("cse_lite RW info not"
                                  " found in coreboot logs!")
        return me_version

    def verify_me_version(self, expected_version, expected_slot):
        """
        Reads the current active CSME RW Version from coreboot logs
        and compares with expected version

        @param expected_version: Expected CSME RW Version string
        @returns True is matching else False

        """
        me_version = self.get_current_me_rw_version()
        command = "crossystem mainfw_act"
        output = self.faft_client.system.run_shell_command_get_output(
                    command, True)
        main_fw_act = output[0]

        logging.info("Expected mainfw_act    : %s\n" \
                     "Current mainfw_act     : %s\n" \
                     "Expected ME RW Version : %s\n" \
                     "Current ME RW Version  : %s\n" % (
                          expected_slot, main_fw_act,
                          expected_version, me_version))

        if (expected_version not in me_version) or \
                 (expected_slot not in main_fw_act):
            return False
        else:
            return True

    def prepare_shellball(self, bios_image, append = None):
        """Prepare a shellball with the given bios image.

        @param bios_image: bios image with shellball to be created
        @param append: string to be updated with shellball name
        """
        logging.info("Preparing shellball with %s" % bios_image)
        self.faft_client.updater.reset_shellball()
        # Copy the given bois to shellball
        extract_dir = self.faft_client.updater.get_work_path()
        bios_rel = self.faft_client.updater.get_bios_relative_path()
        bios_shell = os.path.join(extract_dir, bios_rel)
        command = "cp %s %s" % (bios_image, bios_shell)
        output = self.faft_client.system.run_shell_command_get_output(
                    command, True)
        if output:
            raise error.TestError("File not found!: %s" % bios_image)
        # Reload and repack the shellball
        self.faft_client.updater.reload_images()
        self.faft_client.updater.repack_shellball(append)

    def run_shellball(self, append):
        """Run chromeos-firmwareupdate

        @param append: additional piece to add to shellball name
        """

        # make sure we restore firmware after the test, if it tried to flash.
        self.restore_required = True

        # Update only host firmware
        options = ['--host_only', '--wp=1']
        logging.info("Updating RW firmware using " \
                     "chromeos_firmwareupdate")
        logging.info("Update command : chromeos_firmwareupdate-%s --mode=%s "
                     " %s" % (append,self.MODE,' '.join(options)))
        result = self.run_chromeos_firmwareupdate(
                self.MODE, append, options, ignore_status = True)

        if result.exit_status == 255:
            raise error.TestError("DUT network dropped during update.")
        elif result.exit_status != 0:
            if ('Good. It seems nothing was changed.' in result.stdout):
                logging.info("DUT already matched the image; updater aborted.")
            else:
                raise error.TestError("Firmware updater unexpectedly" \
                                      "failed (rc=%s)" % result.exit_status)

    def run_once(self):
        if not self.faft_config.intel_cse_lite:
            raise error.TestNAError("CSELite feature not supported " \
                                    "on this device. Test Skipped")

        # Read current bios from SPI and create a backup copy
        self.read_current_bios_and_save()

        # Check fmap scheme of the bios read from SPI
        spi_bios_fmap_ver = self.check_fmap_format(self.spi_bios)

        if not self.check_if_me_blob_exist_in_image(self.spi_bios):
            raise error.TestError("Test setup issue : me_rw blob is not " \
                                "present in the current bios.!")

        # Check fmap scheme of the default bios in shellball
        downgrade_bios_fmap = self.check_fmap_format(self.downgrade_bios)

        # Check if me_rw blob is present in FW_MAIN
        if not self.check_if_me_blob_exist_in_image(self.downgrade_bios):
            raise error.TestError("Test setup issue : me_rw blob is not " \
                                    "present in downgrade bios.")

        # Check if both of the bios versions use same fmap structure for me_rw
        if downgrade_bios_fmap not in spi_bios_fmap_ver:
            raise error.TestError("Test setup issue : FMAP format is " \
                            "different in current and downgrade bios.")

        # Get the version of me_rw in the downgrade bios
        downgrade_me_version = self.get_image_fwmain_me_rw_version( \
                                    self.downgrade_bios)

        # Get the version of me_rw in the spi bios
        spi_me_version = self.get_image_fwmain_me_rw_version(self.spi_bios)

        # Get active CSME RW version from cbmem -1
        active_csme_rw_version = self.get_current_me_rw_version()

        logging.info("Active CSME RW Version                 : %s\n" \
                     "FW main CSME RW Version SPI Image      : %s\n" \
                     "FW main CSME RW Version downgrade Image: %s\n" % (
                     active_csme_rw_version, spi_me_version,
                     downgrade_me_version ))

        # Abort if downgrade me_rw version is same as spi me_rw version
        if (spi_me_version in downgrade_me_version):
            raise error.TestError("Test setup issue : CSME RW version is " \
                                    "same in both of the images.")

        for slot in ["A", "B"]:
            operation = "downgrade"
            # Create a shellball with downgrade bios
            self.prepare_shellball(self.downgrade_bios, operation)

            logging.info("Downgrading RW section. Downgrade ME " \
                        "Version: %s" % downgrade_me_version)
            # Run firmware updater downgrade the bios RW
            self.run_shellball(operation)

            # Set fw_try_next to slot and reboot to trigger csme update
            logging.info("Setting fw_try_next to %s: " % slot)
            self.faft_client.system.set_fw_try_next(slot)
            self.switcher.mode_aware_reboot(reboot_type = 'cold')

            # Check if the Active CSME RW version changed to downgrade version
            if not self.verify_me_version(downgrade_me_version, slot):
                raise error.TestError("CSME RW Downgrade using "
                                    "FW_MAIN_%s is Failed!" % slot)
            logging.info("CSME RW Downgrade using FW_MAIN_%s is "
                        "successful" % slot)

            operation = "upgrade"
            # Create a shellball with the original spi bios
            self.prepare_shellball(self.spi_bios, operation)

            logging.info("Upgrading RW Section. Upgrade ME " \
                        "Version: %s" % spi_me_version)
            # Run firmware updater and update RW section with shellball
            self.run_shellball(operation)

            # Set fw_try_next to slot and reboot to trigger csme update
            logging.info("Setting fw_try_next to %s: " % slot)
            self.faft_client.system.set_fw_try_next(slot)
            self.switcher.mode_aware_reboot(reboot_type = 'cold')

            # Check if the Active CSME RW version changed to original version
            if not self.verify_me_version(spi_me_version, slot):
                raise error.TestError("CSME RW Upgrade using "
                                    "FW_MAIN_%s is Failed!" % slot)
            logging.info("CSME RW Upgrade using FW_MAIN_%s is "
                        "successful" % slot)
