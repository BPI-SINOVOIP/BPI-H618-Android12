#!/usr/bin/env python3
#
#   Copyright 2020 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from acts import logger
from acts.controllers.fuchsia_lib.base_lib import BaseLib

COMMAND_SUGGEST_AP_MAC_ADDR = 'wlan_deprecated.suggest_ap_mac'


class FuchsiaWlanDeprecatedConfigurationLib(BaseLib):
    def __init__(self, addr, tc, client_id):
        self.address = addr
        self.test_counter = tc
        self.client_id = client_id
        self.log = logger.create_tagged_trace_logger(str(addr))

    def wlanSuggestAccessPointMacAddress(self, addr):
        """ Suggests a mac address to soft AP interface, to support
        cast legacy behavior.

        Args:
            addr: string of mac address to suggest (e.g. '12:34:56:78:9a:bc')
        """
        test_cmd = COMMAND_SUGGEST_AP_MAC_ADDR
        test_args = {'mac': addr}
        test_id = self.build_id(self.test_counter)
        self.test_counter += 1

        return self.send_command(test_id, test_cmd, test_args)