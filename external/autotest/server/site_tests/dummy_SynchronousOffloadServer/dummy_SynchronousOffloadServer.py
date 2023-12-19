# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.common_lib import error
from autotest_lib.server import test

class dummy_SynchronousOffloadServer(test.test):
  version = 1

  def run_once(self):
    DIR = os.getenv('SYNCHRONOUS_OFFLOAD_DIR', "")
    if DIR == "":
      raise error.TestFail("Did not find value for SYNCHRONOUS_OFFLOAD_DIR")
    if not os.path.isdir(DIR):
      raise error.TestFail("$SYNCHRONOUS_OFFLOAD_DIR=%s, which is not "
                           "a valid directory." % DIR)
    logging.debug("Writing to directory %s", DIR)
    with open(os.path.join(DIR,"test_file"), "w") as f:
      f.write("Test string which should be offloaded")
      logging.debug("Wrote string to test file.")
