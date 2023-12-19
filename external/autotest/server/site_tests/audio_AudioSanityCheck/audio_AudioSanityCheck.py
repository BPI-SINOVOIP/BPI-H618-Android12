# Copyright 2019 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This is a server side audio sanity test testing assumptions other audio tests
rely on.
"""

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import audio_test_utils
from autotest_lib.server.cros.audio import audio_test

class audio_AudioSanityCheck(audio_test.AudioTest):
    """
    This test talks to a Cros device to verify if some basic functions that
    other audio tests rely on still work after a suspension.
    """
    version = 1

    def verify_chrome_audio(self):
        """"Verify if chrome.audio API is available"""
        if not self.facade.get_chrome_audio_availablity():
            raise error.TestFail("chrome.audio API is not available")

    def verify_suspend(self):
        """"Verify and trigger a suspension"""
        audio_test_utils.suspend_resume_and_verify(self.host, self.factory)

    def run_once(self, suspend_only=False):
        """Runs Audio sanity test to make sure chrome api works. """

        # The suspend_only flag is for crbug:978593, which causes sanity check
        # to always fail. however, we still want to check the suspend operation
        # as it also potentially fails the audio tests. This should be removed
        # once the blocker is fixed
        if suspend_only:
            self.verify_suspend()
            return

        # Check if the chrome.audio API is available
        self.verify_chrome_audio()
        # chrome.audio API should remain available after a suspension
        self.verify_suspend()
        self.verify_chrome_audio()
