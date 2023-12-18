#!/usr/bin/env python
#
# Copyright 2018 - The Android Open Source Project
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
r"""RemoteImageRemoteInstance class.

Create class that is responsible for creating a remote instance AVD with a
remote image.
"""

import logging
import time

from acloud.create import base_avd_create
from acloud.internal import constants
from acloud.internal.lib import engprod_client
from acloud.internal.lib import utils
from acloud.public.actions import common_operations
from acloud.public.actions import remote_instance_cf_device_factory
from acloud.public import report


logger = logging.getLogger(__name__)
_DEVICE = "device"
_DEVICE_KEY_MAPPING = {"serverUrl": "ip", "sessionId": "instance_name"}
_LAUNCH_CVD_TIME = "launch_cvd_time"


class RemoteImageRemoteInstance(base_avd_create.BaseAVDCreate):
    """Create class for a remote image remote instance AVD."""

    @utils.TimeExecute(function_description="Total time: ",
                       print_before_call=False, print_status=False)
    def _CreateAVD(self, avd_spec, no_prompts):
        """Create the AVD.

        Args:
            avd_spec: AVDSpec object that tells us what we're going to create.
            no_prompts: Boolean, True to skip all prompts.

        Returns:
            A Report instance.
        """
        if avd_spec.oxygen:
            return self._LeaseOxygenAVD(avd_spec)
        device_factory = remote_instance_cf_device_factory.RemoteInstanceDeviceFactory(
            avd_spec)
        create_report = common_operations.CreateDevices(
            "create_cf", avd_spec.cfg, device_factory, avd_spec.num,
            report_internal_ip=avd_spec.report_internal_ip,
            autoconnect=avd_spec.autoconnect,
            avd_type=constants.TYPE_CF,
            boot_timeout_secs=avd_spec.boot_timeout_secs,
            unlock_screen=avd_spec.unlock_screen,
            wait_for_boot=False,
            connect_webrtc=avd_spec.connect_webrtc,
            client_adb_port=avd_spec.client_adb_port)
        # Launch vnc client if we're auto-connecting.
        if avd_spec.connect_vnc:
            utils.LaunchVNCFromReport(create_report, avd_spec, no_prompts)
        if avd_spec.connect_webrtc:
            utils.LaunchBrowserFromReport(create_report)

        return create_report

    def _LeaseOxygenAVD(self, avd_spec):
        """Lease the AVD from the AVD pool.

        Args:
            avd_spec: AVDSpec object that tells us what we're going to create.

        Returns:
            A Report instance.
        """
        timestart = time.time()
        response = engprod_client.EngProdClient.LeaseDevice(
            avd_spec.remote_image[constants.BUILD_TARGET],
            avd_spec.remote_image[constants.BUILD_ID],
            avd_spec.cfg.api_key,
            avd_spec.cfg.api_url)
        execution_time = round(time.time() - timestart, 2)
        reporter = report.Report(command="create_cf")
        if _DEVICE in response:
            reporter.SetStatus(report.Status.SUCCESS)
            device_data = response[_DEVICE]
            device_data[_LAUNCH_CVD_TIME] = execution_time
            self._ReplaceDeviceDataKeys(device_data)
            reporter.UpdateData(response)
        else:
            reporter.SetStatus(report.Status.FAIL)
            reporter.AddError(response.get("errorMessage"))

        return reporter

    @staticmethod
    def _ReplaceDeviceDataKeys(device_data):
        """Replace keys of device data from oxygen response.

        To keep the device data using the same keys in Acloud report. Before
        writing data to report, it needs to update the keys.

        Values:
            device_data: Dict of device data. e.g. {'sessionId': 'b01ead68',
                                                    'serverUrl': '10.1.1.1'}
        """
        for key, val in _DEVICE_KEY_MAPPING.items():
            if key in device_data:
                device_data[val] = device_data[key]
                del device_data[key]
            else:
                logger.debug("There is no '%s' data in response.", key)
