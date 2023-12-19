# Copyright (c) 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.cfm.apollo import apollo_utils


class enterprise_CFM_ApolloPeripheralPowerCycle(test.test):
    """
    Disables then re-enables Power over Ethernet for the given IP port.

    NOTE: This test turns off the given peripheral, which could impact other Apollo tests.
    (autotests shouldn't assume anything about the device state.)
    """
    version = 1

    def run_once(self, port):
        apollo_utils.ectool_pse_disable(port)
        status = apollo_utils.ectool_pse_status(port)

        if status != 'disabled':
            raise error.TestError('IP port status not disabled')

        # TODO: Is there meaningful check we can add here?
        # (We may need to add a "peripheral" param as well)

        apollo_utils.ectool_pse_enable(port)
        status = apollo_utils.ectool_pse_status(port)

        if status not in ['enabled', 'powered']:
            raise error.TestError('IP port status not enabled')
