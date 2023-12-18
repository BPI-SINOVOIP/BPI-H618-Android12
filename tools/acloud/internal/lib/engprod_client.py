# Copyright 2021 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""A client that talks to EngProd APIs."""

import json
import subprocess

from urllib.parse import urljoin


class EngProdClient():
    """Client that manages EngProd api."""

    @staticmethod
    def LeaseDevice(build_target, build_id, api_key, api_url):
        """Lease one cuttlefish device.

        Args:
            build_target: Target name, e.g. "aosp_cf_x86_phone-userdebug"
            build_id: Build ID, a string, e.g. "2263051", "P2804227"
            api_key: String of api key.
            api_url: String of api url.

        Returns:
            The response of curl command.
        """
        request_data = "{\"target\": \"%s\", \"build_id\": \"%s\"}" % (
            build_target, build_id)
        lease_url = urljoin(api_url, "lease?key=%s" % api_key)
        response = subprocess.check_output([
            "curl", "--request", "POST", lease_url, "-H",
            "Accept: application/json", "-H", "Content-Type: application/json",
            "-d", request_data
        ])
        return json.loads(response)
