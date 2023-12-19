# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This test only checks whether the directory of the dependency package exists.
# It is expected that the necessary path configuration will be done in the test.

# IMPORTANT: This test should be updated whenever the dependencies specified change
# for any of the environments.

import logging
import os
import subprocess

from autotest_lib.client.bin import utils as client_utils
from autotest_lib.client.common_lib import error
from autotest_lib.server import test, utils


class cheets_LabDependencies(test.test):
    """Check basic dependency requirements for running CTS tests."""
    version = 1
    """
    Context: xTS P requires JDK 8 or 9, and xTS R requires JDK 9 or above.

    LXC (container for running server-side autotest)
    Runs xTS P and R. Uses JDK 8 as default, and JDK 9 on xTS R.
    JDK 9 path is hardcoded in cheets_CTS_R.

    moblab
    Only runs CTS P. Uses JDK 9 (single JDK) on CTS P.
    Currently this test does not run on moblab.
    Because is_moblab does not work, test ignores the moblab component.
    Need to move JDK's to /java. TODO(haddowk)
    Will run when new distribution is released.

    chroot (PFQ environment)
    Runs CTS P and R. Uses JDK 11 as default, and JDK 8 on CTS P.
    JDK 8 Path is hardcoded in cheets_CTS_P.
    """

    ENV_EXPECTED = {
            'LXC': {
                    'JDK': [
                            '/usr/lib/jvm/jdk-9.0.4',
                            '/usr/lib/jvm/java-8-openjdk-amd64',
                    ],
                    'DIR': '/usr/lib/jvm'
            },
            'moblab': {
                    'JDK': [
                            '/java/jdk-9.0.4',
                    ],
                    'DIR': '/java'
            },
            'chroot': {
                    'JDK': [
                            '/opt/icedtea-bin-3.4.0',
                            '/opt/openjdk-bin-11',
                    ],
                    'DIR': '/opt'
            }
    }

    def check_JDK(self, env):
        """Check whether required JDK directories exist for the environment."""
        for dep_path in self.ENV_EXPECTED[env]['JDK']:
            """Raise TestFail if specified JDK directories do not exist."""
            if not os.path.isdir(dep_path):
                java_path = subprocess.check_output([
                                'find', self.ENV_EXPECTED[env]['DIR'], '-path',
                                '*bin/java'], stderr=subprocess.STDOUT)
                if java_path:
                    java_path = ', '.join(java_path.split('\n')[:-1])
                raise error.TestFail(
                        'Missing required JDK dependency %s for %s. '
                        'Quick search shows currently installed versions are %s.'
                        % (dep_path, env, java_path))

    def run_once(self, host=None):
        """Check the type of environment, and see if all dependencies are satisfied."""
        if utils.is_in_container():
            logging.info('[ENV] Running inside the LXC container')
            env = 'LXC'
        # is_moblab does not work. Run in moblab when new distribution is released.
        elif client_utils.is_moblab():
            logging.info('[ENV] Running inside moblab')
            env = 'moblab'
        else:
            logging.info('[ENV] Running inside chroot environment')
            env = 'chroot'
        self.host = host
        self.check_JDK(env)
