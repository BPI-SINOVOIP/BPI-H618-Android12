# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.chameleon import chameleon
from autotest_lib.client.common_lib import ui_utils_helpers

_CHECK_TIMEOUT = 20
_PRINTER_NAME = "HP OfficeJet g55"
_PRINTING_COMPLETE_NOTIF = "Printing complete"
_PRINTING_NOTIF = "Printing"
_STEPS_BETWEEN_CONTROLS = 4
_USB_PRINTER_CONNECTED_NOTIF = "USB printer connected"

_SHORT_WAIT = 2

class platform_PrintJob(test.test):
    """
    E2E test - Chrome is brought up, local pdf file open, print dialog open,
    chameleon gadget driver printer selected and print job executed. The test
    verifies the print job finished successfully.
    """
    version = 1

    def cleanup(self):
        if hasattr(self, 'browser'):
            self.browser.close()
        if self.printer_capture_started:
            self.usb_printer.StopCapturingPrinterData()
        if self.printer_connected:
            self.usb_printer.Unplug()

    def check_notification(self, notification):
        """Polls for successful print job notification"""

        def find_notification(title=None):
            notifications = self.cr.get_visible_notifications()
            if notifications is None:
                return False
            for n in notifications:
                if title in n['title']:
                    return True
            return False

        utils.poll_for_condition(
                condition=lambda: find_notification(notification),
                desc='Notification %s NOT found' % notification,
                timeout=_CHECK_TIMEOUT, sleep_interval=_SHORT_WAIT)

    def navigate_to_pdf(self):
        """Navigate to the pdf page to print"""
        self.cr.browser.platform.SetHTTPServerDirectories(self.bindir)
        tab = self.cr.browser.tabs.New()
        pdf_path = os.path.join(self.bindir, 'to_print.pdf')
        tab.Navigate(self.cr.browser.platform.http_server.UrlOf(pdf_path))
        tab.WaitForDocumentReadyStateToBeInteractiveOrBetter(
                timeout=_CHECK_TIMEOUT);
        time.sleep(_SHORT_WAIT)

    def run_once(self, host, args):
        """Run the test."""
        # Set these to know if the usb_printer needs to be handled post test.
        self.printer_capture_started = False
        self.printer_connected = False

        # Make chameleon host known to the DUT host crbug.com/862646
        chameleon_args = 'chameleon_host=' + host.hostname + '-chameleon'
        args.append(chameleon_args)

        chameleon_board = chameleon.create_chameleon_board(host.hostname, args)
        chameleon_board.setup_and_reset(self.outputdir)
        self.usb_printer = chameleon_board.get_usb_printer()
        self.usb_printer.SetPrinterModel(1008, 17, _PRINTER_NAME)

        with chrome.Chrome(autotest_ext=True,
                           init_network_controller=True) as self.cr:
            self.usb_printer.Plug()
            self.printer_connected = True
            self.check_notification(_USB_PRINTER_CONNECTED_NOTIF)
            logging.info('Chameleon printer connected!')
            self.navigate_to_pdf()
            time.sleep(_SHORT_WAIT)
            logging.info('PDF file opened in browser!')
            self.ui_helper = ui_utils_helpers.UIPrinterHelper(chrome=self.cr)
            self.ui_helper.print_to_custom_printer("Chameleon " + _PRINTER_NAME)
            self.usb_printer.StartCapturingPrinterData()
            self.printer_capture_started = True
            self.check_notification(_PRINTING_NOTIF)
            self.check_notification(_PRINTING_COMPLETE_NOTIF)
