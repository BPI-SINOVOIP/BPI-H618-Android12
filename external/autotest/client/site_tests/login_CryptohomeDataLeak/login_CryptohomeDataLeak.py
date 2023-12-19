# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import cryptohome


class login_CryptohomeDataLeak(test.test):
    """Verify decrypted user data is cleared after end of session.
    """
    version = 1

    _CHAPS_LOCK_DIR = '/run/lock/power_override'
    _CHAPS_LOCK_PREFIX = 'chapsd_token_init_slot_'
    _CHAPS_INIT_TIMEOUT = 30

    def run_once(self):
        """Entry point of test"""
        username = ''
        test_file = ''

        with chrome.Chrome() as cr:
            username = cr.username
            if not cryptohome.is_permanent_vault_mounted(username):
                raise error.TestError('Expected to find a mounted vault.')

            test_file =  '/home/.shadow/%s/mount/hello' \
                         % cryptohome.get_user_hash(username)

            logging.info("Test file: %s", test_file)
            open(test_file, 'w').close()

            # Check until chaps lock file disappear.
            for _ in xrange(self._CHAPS_INIT_TIMEOUT):
                time.sleep(1)
                has_lock = False
                for lock in os.listdir(self._CHAPS_LOCK_DIR):
                    if lock.startswith(self._CHAPS_LOCK_PREFIX):
                        has_lock = True
                        break
                if not has_lock:
                    break
            else:
                raise error.TestError(
                        'Expected chaps finished all load events.')

        if cryptohome.is_vault_mounted(user=username, allow_fail=True):
            raise error.TestError('Expected to not find a mounted vault.')

        # At this point, the session is not active and the file name is expected
        # to be encrypted again.

        if os.path.isfile(test_file):
            raise error.TestFail('File still visible after end of session.')

        cryptohome.remove_vault(username)
