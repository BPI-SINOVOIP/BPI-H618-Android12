# Copyright (c) 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.cros import upstart


class platform_Fingerprint(test.test):
  """Fingerprint client tests."""
  version = 1

  @staticmethod
  def _test_biod_is_running():
    """Verify biod is running."""
    upstart.ensure_running("biod")

  def run_once(self, to_test):
    """Run the test specified by to_test."""
    method_name = '_test_' + to_test
    method = getattr(self, method_name)
    method()
