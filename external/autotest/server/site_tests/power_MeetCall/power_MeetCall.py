# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server import autotest
from autotest_lib.server import test
from autotest_lib.server.cros.cfm.utils import bond_http_api


class power_MeetCall(test.test):
    """Wrapper test to create meet bot and call power_MeetClient."""
    version = 1

    # 5 minutes for client test autotests overhead.
    AUTOTESTS_OVERHEAD = 300

    def run_once(self, host, args):
        """Create meetbot and call client test."""
        bond_api = bond_http_api.BondHttpApi()
        meet_code = bond_api.CreateConference()
        logging.info('meet_code: %s', meet_code)

        num_bots = args.get('num_bots', 4)
        duration = args.get('duration', 180) + self.AUTOTESTS_OVERHEAD

        bots = bond_api.AddBotsRequest(meet_code, num_bots, duration)
        if len(bots) < num_bots:
            bond_api.ExecuteScript('@all leave', meet_code)
            raise error.TestNAError('Can not add meet bots.')

        args['meet_code'] = meet_code
        if not args['pdash_note']:
            args['pdash_note'] = meet_code
        autotest_client = autotest.Autotest(host)
        autotest_client.run_test('power_MeetClient', **args)
