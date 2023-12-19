# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.server import test
from autotest_lib.server.hosts.drone_api_client.client import TLSClient


class infra_TLSExecDUTCommand(test.test):
    """
    Run a command on the host via the TLS API (ExecDutCommand) and ensure the
    output is as expected.

    """

    version = 1

    def run_once(self, host):
        """
        Run the test.

        @param host: A host object representing the DUT.

        """
        tlsclient = TLSClient(hostname=host.hostname)
        res = tlsclient.run_cmd("echo success")
        if not isinstance(res, utils.CmdResult):
            raise error.TestError(
                "Client returned type: '{}'. Expected type: 'utils.CmdResult'"
                .format(type(res)))
        if res.exit_status != 0:
            logging.info("STD_ERR of res {}".format(res.stderr))
            raise error.TestError("TLS CMD exit status was: '{}'. Expected: '0'"
                                  .format(res.exit_status))
        if res.stdout != "success\n":
            raise error.TestError("TLS returned: '{}'. Expected: '{}'"
                                  .format(res.stdout, "success\n"))
