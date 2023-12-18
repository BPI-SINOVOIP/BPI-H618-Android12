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
"""Tests for RemoteImageRemoteInstance."""

import unittest

from unittest import mock

from acloud.create import remote_image_remote_instance
from acloud.internal import constants
from acloud.internal.lib import driver_test_lib
from acloud.internal.lib import engprod_client
from acloud.public import report
from acloud.public.actions import common_operations
from acloud.public.actions import remote_instance_cf_device_factory


class RemoteImageRemoteInstanceTest(driver_test_lib.BaseDriverTest):
    """Test RemoteImageRemoteInstance method."""

    def setUp(self):
        """Initialize new RemoteImageRemoteInstance."""
        super().setUp()
        self.remote_image_remote_instance = remote_image_remote_instance.RemoteImageRemoteInstance()

    # pylint: disable=protected-access
    @mock.patch.object(remote_image_remote_instance.RemoteImageRemoteInstance,
                       "_LeaseOxygenAVD")
    @mock.patch.object(common_operations, "CreateDevices")
    @mock.patch.object(remote_instance_cf_device_factory,
                       "RemoteInstanceDeviceFactory")
    def testCreateAVD(self, mock_factory, mock_create_device, mock_lease):
        """test CreateAVD."""
        avd_spec = mock.Mock()
        avd_spec.oxygen = False
        self.remote_image_remote_instance._CreateAVD(
            avd_spec, no_prompts=True)
        mock_factory.assert_called_once()
        mock_create_device.assert_called_once()

        avd_spec.oxygen = True
        self.remote_image_remote_instance._CreateAVD(
            avd_spec, no_prompts=True)
        mock_lease.assert_called_once()

    def testLeaseOxygenAVD(self):
        """test LeaseOxygenAVD."""
        avd_spec = mock.Mock()
        avd_spec.oxygen = True
        avd_spec.remote_image = {constants.BUILD_TARGET: "fake_target",
                                 constants.BUILD_ID: "fake_id"}
        response_success = {"device": {"sessionId": "fake_device",
                                       "serverUrl": "10.1.1.1"}}
        response_fail = {"errorMessage": "Lease device fail."}
        self.Patch(engprod_client.EngProdClient, "LeaseDevice",
                   side_effect=[response_success, response_fail])
        expected_status = report.Status.SUCCESS
        reporter = self.remote_image_remote_instance._LeaseOxygenAVD(avd_spec)
        self.assertEqual(reporter.status, expected_status)

        expected_status = report.Status.FAIL
        reporter = self.remote_image_remote_instance._LeaseOxygenAVD(avd_spec)
        self.assertEqual(reporter.status, expected_status)


    def testReplaceDeviceDataKeys(self):
        """test ReplaceDeviceDataKeys."""
        device_data = {"sessionId": "fake_device", "serverUrl": "10.1.1.1"}
        expected_result = {"instance_name": "fake_device", "ip": "10.1.1.1"}
        self.remote_image_remote_instance._ReplaceDeviceDataKeys(device_data)
        self.assertEqual(device_data, expected_result)


if __name__ == '__main__':
    unittest.main()
