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
"""Tests for restart."""
import unittest

from unittest import mock

from acloud.internal.lib import driver_test_lib
from acloud.list import list as list_instances
from acloud.public import config
from acloud.restart import restart


class RestartTest(driver_test_lib.BaseDriverTest):
    """Test restart."""

    @mock.patch.object(restart, "RestartFromInstance")
    def testRun(self, mock_restart):
        """test Run."""
        cfg = mock.MagicMock()
        args = mock.MagicMock()
        instance_obj = mock.MagicMock()
        # Test case with provided instance name.
        args.instance_name = "instance_1"
        args.instance_id = 1
        args.powerwash = False
        self.Patch(config, "GetAcloudConfig", return_value=cfg)
        self.Patch(list_instances, "GetInstancesFromInstanceNames",
                   return_value=[instance_obj])
        restart.Run(args)
        mock_restart.assert_has_calls([
            mock.call(cfg, instance_obj, args.instance_id, args.powerwash)])

        # Test case for user select one instance to restart AVD.
        selected_instance = mock.MagicMock()
        self.Patch(list_instances, "ChooseOneRemoteInstance",
                   return_value=selected_instance)
        args.instance_name = None
        restart.Run(args)
        mock_restart.assert_has_calls([
            mock.call(cfg, selected_instance, args.instance_id, args.powerwash)])


if __name__ == '__main__':
    unittest.main()
