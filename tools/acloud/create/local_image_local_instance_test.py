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
"""Tests for LocalImageLocalInstance."""

import os
import subprocess
import tempfile
import unittest

from unittest import mock

from acloud import errors
from acloud.create import local_image_local_instance
from acloud.list import instance
from acloud.list import list as list_instance
from acloud.internal import constants
from acloud.internal.lib import driver_test_lib
from acloud.internal.lib import utils


class LocalImageLocalInstanceTest(driver_test_lib.BaseDriverTest):
    """Test LocalImageLocalInstance method."""

    LAUNCH_CVD_CMD_WITH_DISK = """sg group1 <<EOF
sg group2
launch_cvd -daemon -config=phone -run_adb_connector=true -system_image_dir fake_image_dir -instance_dir fake_cvd_dir -undefok=report_anonymous_usage_stats,enable_sandbox,config -report_anonymous_usage_stats=y -enable_sandbox=false -cpus fake -x_res fake -y_res fake -dpi fake -memory_mb fake -blank_data_image_mb fake -data_policy always_create -start_vnc_server=true
EOF"""

    LAUNCH_CVD_CMD_NO_DISK = """sg group1 <<EOF
sg group2
launch_cvd -daemon -config=phone -run_adb_connector=true -system_image_dir fake_image_dir -instance_dir fake_cvd_dir -undefok=report_anonymous_usage_stats,enable_sandbox,config -report_anonymous_usage_stats=y -enable_sandbox=false -cpus fake -x_res fake -y_res fake -dpi fake -memory_mb fake -start_vnc_server=true
EOF"""

    LAUNCH_CVD_CMD_NO_DISK_WITH_GPU = """sg group1 <<EOF
sg group2
launch_cvd -daemon -config=phone -run_adb_connector=true -system_image_dir fake_image_dir -instance_dir fake_cvd_dir -undefok=report_anonymous_usage_stats,enable_sandbox,config -report_anonymous_usage_stats=y -enable_sandbox=false -cpus fake -x_res fake -y_res fake -dpi fake -memory_mb fake -start_vnc_server=true
EOF"""

    LAUNCH_CVD_CMD_WITH_WEBRTC = """sg group1 <<EOF
sg group2
launch_cvd -daemon -config=auto -run_adb_connector=true -system_image_dir fake_image_dir -instance_dir fake_cvd_dir -undefok=report_anonymous_usage_stats,enable_sandbox,config -report_anonymous_usage_stats=y -enable_sandbox=false -guest_enforce_security=false -vm_manager=crosvm -start_webrtc=true -webrtc_public_ip=0.0.0.0
EOF"""

    LAUNCH_CVD_CMD_WITH_MIXED_IMAGES = """sg group1 <<EOF
sg group2
launch_cvd -daemon -config=phone -run_adb_connector=true -system_image_dir fake_image_dir -instance_dir fake_cvd_dir -undefok=report_anonymous_usage_stats,enable_sandbox,config -report_anonymous_usage_stats=y -enable_sandbox=false -start_vnc_server=true -super_image=fake_super_image -boot_image=fake_boot_image
EOF"""

    LAUNCH_CVD_CMD_WITH_ARGS = """sg group1 <<EOF
sg group2
launch_cvd -daemon -config=phone -run_adb_connector=true -system_image_dir fake_image_dir -instance_dir fake_cvd_dir -undefok=report_anonymous_usage_stats,enable_sandbox,config -report_anonymous_usage_stats=y -enable_sandbox=false -start_vnc_server=true -setupwizard_mode=REQUIRED
EOF"""

    _EXPECTED_DEVICES_IN_REPORT = [
        {
            "instance_name": "local-instance-1",
            "ip": "0.0.0.0:6520",
            "adb_port": 6520,
            "vnc_port": 6444,
            "webrtc_port": 8443
        }
    ]

    _EXPECTED_DEVICES_IN_FAILED_REPORT = [
        {
            "instance_name": "local-instance-1",
            "ip": "0.0.0.0"
        }
    ]

    def setUp(self):
        """Initialize new LocalImageLocalInstance."""
        super().setUp()
        self.local_image_local_instance = local_image_local_instance.LocalImageLocalInstance()

    # pylint: disable=protected-access
    @mock.patch("acloud.create.local_image_local_instance.utils")
    @mock.patch.object(local_image_local_instance.LocalImageLocalInstance,
                       "GetImageArtifactsPath")
    @mock.patch.object(local_image_local_instance.LocalImageLocalInstance,
                       "_SelectAndLockInstance")
    @mock.patch.object(local_image_local_instance.LocalImageLocalInstance,
                       "_CheckRunningCvd")
    @mock.patch.object(local_image_local_instance.LocalImageLocalInstance,
                       "_CreateInstance")
    def testCreateAVD(self, mock_create, mock_check_running_cvd,
                      mock_lock_instance, mock_get_image, mock_utils):
        """Test _CreateAVD."""
        mock_utils.IsSupportedPlatform.return_value = True
        mock_get_image.return_value = local_image_local_instance.ArtifactPaths(
            "/image/path", "/host/bin/path", None, None, None, None)
        mock_check_running_cvd.return_value = True
        mock_avd_spec = mock.Mock()
        mock_lock = mock.Mock()
        mock_lock.Unlock.return_value = False
        mock_lock_instance.return_value = (1, mock_lock)

        # Success
        mock_create.return_value = mock.Mock()
        self.local_image_local_instance._CreateAVD(
            mock_avd_spec, no_prompts=True)
        mock_lock_instance.assert_called_once()
        mock_lock.SetInUse.assert_called_once_with(True)
        mock_lock.Unlock.assert_called_once()

        mock_lock_instance.reset_mock()
        mock_lock.SetInUse.reset_mock()
        mock_lock.Unlock.reset_mock()

        # Failure with no report
        mock_create.side_effect = ValueError("unit test")
        with self.assertRaises(ValueError):
            self.local_image_local_instance._CreateAVD(
                mock_avd_spec, no_prompts=True)
        mock_lock_instance.assert_called_once()
        mock_lock.SetInUse.assert_not_called()
        mock_lock.Unlock.assert_called_once()

        # Failure with report
        mock_lock_instance.side_effect = errors.CreateError("unit test")
        report = self.local_image_local_instance._CreateAVD(
            mock_avd_spec, no_prompts=True)
        self.assertEqual(report.errors, ["unit test"])

    def testSelectAndLockInstance(self):
        """test _SelectAndLockInstance."""
        mock_avd_spec = mock.Mock(local_instance_id=0)
        mock_lock = mock.Mock()
        mock_lock.Lock.return_value = True
        mock_lock.LockIfNotInUse.side_effect = (False, True)
        self.Patch(instance, "GetLocalInstanceLock",
                   return_value=mock_lock)

        ins_id, _ = self.local_image_local_instance._SelectAndLockInstance(
            mock_avd_spec)
        self.assertEqual(2, ins_id)
        mock_lock.Lock.assert_not_called()
        self.assertEqual(2, mock_lock.LockIfNotInUse.call_count)

        mock_lock.LockIfNotInUse.reset_mock()

        mock_avd_spec.local_instance_id = 1
        ins_id, _ = self.local_image_local_instance._SelectAndLockInstance(
            mock_avd_spec)
        self.assertEqual(1, ins_id)
        mock_lock.Lock.assert_called_once()
        mock_lock.LockIfNotInUse.assert_not_called()

    @mock.patch("acloud.create.local_image_local_instance.utils")
    @mock.patch("acloud.create.local_image_local_instance.ota_tools")
    @mock.patch("acloud.create.local_image_local_instance.create_common")
    @mock.patch.object(local_image_local_instance.LocalImageLocalInstance,
                       "_LaunchCvd")
    @mock.patch.object(local_image_local_instance.LocalImageLocalInstance,
                       "PrepareLaunchCVDCmd")
    @mock.patch.object(instance, "GetLocalInstanceRuntimeDir")
    @mock.patch.object(instance, "GetLocalInstanceHomeDir")
    def testCreateInstance(self, mock_home_dir, _mock_runtime_dir,
                           _mock_prepare_cmd, mock_launch_cvd,
                           _mock_create_common, mock_ota_tools, _mock_utils):
        """Test the report returned by _CreateInstance."""
        self.Patch(instance, "GetLocalInstanceName",
                   return_value="local-instance-1")
        mock_home_dir.return_value = "/local-instance-1"
        artifact_paths = local_image_local_instance.ArtifactPaths(
            "/image/path", "/host/bin/path", "/misc/info/path",
            "/ota/tools/dir", "/system/image/path", "/boot/image/path")
        mock_ota_tools_object = mock.Mock()
        mock_ota_tools.OtaTools.return_value = mock_ota_tools_object
        mock_avd_spec = mock.Mock(unlock_screen=False)
        local_ins = mock.Mock(
            adb_port=6520,
            vnc_port=6444
        )
        local_ins.CvdStatus.return_value = True
        self.Patch(instance, "LocalInstance",
                   return_value=local_ins)
        self.Patch(list_instance, "GetActiveCVD",
                   return_value=local_ins)
        self.Patch(os, "symlink")

        # Success
        report = self.local_image_local_instance._CreateInstance(
            1, artifact_paths, mock_avd_spec, no_prompts=True)

        self.assertEqual(report.data.get("devices"),
                         self._EXPECTED_DEVICES_IN_REPORT)
        mock_ota_tools.OtaTools.assert_called_with("/ota/tools/dir")
        mock_ota_tools_object.BuildSuperImage.assert_called_with(
            "/local-instance-1/mixed_super.img", "/misc/info/path", mock.ANY)

        # Failure
        mock_launch_cvd.side_effect = errors.LaunchCVDFail("unit test")

        report = self.local_image_local_instance._CreateInstance(
            1, artifact_paths, mock_avd_spec, no_prompts=True)

        self.assertEqual(report.data.get("devices_failing_boot"),
                         self._EXPECTED_DEVICES_IN_FAILED_REPORT)
        self.assertIn("unit test", report.errors[0])

    # pylint: disable=protected-access
    @mock.patch("acloud.create.local_image_local_instance.os.path.isfile")
    def testFindCvdHostBinaries(self, mock_isfile):
        """Test FindCvdHostBinaries."""
        cvd_host_dir = "/unit/test"
        mock_isfile.return_value = None

        with mock.patch.dict("acloud.internal.lib.ota_tools.os.environ",
                             {"ANDROID_HOST_OUT": cvd_host_dir,
                              "ANDROID_SOONG_HOST_OUT": cvd_host_dir}, clear=True):
            with self.assertRaises(errors.GetCvdLocalHostPackageError):
                self.local_image_local_instance._FindCvdHostBinaries(
                    [cvd_host_dir])

        mock_isfile.side_effect = (
            lambda path: path == "/unit/test/bin/launch_cvd")

        with mock.patch.dict("acloud.internal.lib.ota_tools.os.environ",
                             {"ANDROID_HOST_OUT": cvd_host_dir,
                              "ANDROID_SOONG_HOST_OUT": cvd_host_dir}, clear=True):
            path = self.local_image_local_instance._FindCvdHostBinaries([])
            self.assertEqual(path, cvd_host_dir)

        with mock.patch.dict("acloud.internal.lib.ota_tools.os.environ",
                             dict(), clear=True):
            path = self.local_image_local_instance._FindCvdHostBinaries(
                [cvd_host_dir])
            self.assertEqual(path, cvd_host_dir)

    @staticmethod
    def _CreateEmptyFile(path):
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w"):
            pass

    @mock.patch("acloud.create.local_image_local_instance.ota_tools")
    def testGetImageArtifactsPath(self, mock_ota_tools):
        """Test GetImageArtifactsPath without system image dir."""
        with tempfile.TemporaryDirectory() as temp_dir:
            image_dir = "/unit/test"
            cvd_dir = os.path.join(temp_dir, "cvd-host_package")
            self._CreateEmptyFile(os.path.join(cvd_dir, "bin", "launch_cvd"))

            mock_avd_spec = mock.Mock(
                local_image_dir=image_dir,
                local_kernel_image=None,
                local_system_image=None,
                local_tool_dirs=[cvd_dir])

            paths = self.local_image_local_instance.GetImageArtifactsPath(
                mock_avd_spec)

        mock_ota_tools.FindOtaTools.assert_not_called()
        self.assertEqual(paths, (image_dir, cvd_dir, None, None, None, None))

    @mock.patch("acloud.create.local_image_local_instance.ota_tools")
    def testGetImageFromBuildEnvironment(self, mock_ota_tools):
        """Test GetImageArtifactsPath with files in build environment."""
        with tempfile.TemporaryDirectory() as temp_dir:
            image_dir = os.path.join(temp_dir, "image")
            cvd_dir = os.path.join(temp_dir, "cvd-host_package")
            mock_ota_tools.FindOtaTools.return_value = cvd_dir
            extra_image_dir = os.path.join(temp_dir, "extra_image")
            system_image_path = os.path.join(extra_image_dir, "system.img")
            boot_image_path = os.path.join(extra_image_dir, "boot.img")
            misc_info_path = os.path.join(image_dir, "misc_info.txt")
            self._CreateEmptyFile(os.path.join(image_dir, "vbmeta.img"))
            self._CreateEmptyFile(os.path.join(cvd_dir, "bin", "launch_cvd"))
            self._CreateEmptyFile(system_image_path)
            self._CreateEmptyFile(boot_image_path)
            self._CreateEmptyFile(os.path.join(extra_image_dir,
                                               "boot-debug.img"))
            self._CreateEmptyFile(misc_info_path)

            mock_avd_spec = mock.Mock(
                local_image_dir=image_dir,
                local_kernel_image=extra_image_dir,
                local_system_image=extra_image_dir,
                local_tool_dirs=[])

            with mock.patch.dict("acloud.create.local_image_local_instance."
                                 "os.environ",
                                 {"ANDROID_SOONG_HOST_OUT": cvd_dir},
                                 clear=True):
                paths = self.local_image_local_instance.GetImageArtifactsPath(
                    mock_avd_spec)

        mock_ota_tools.FindOtaTools.assert_called_once()
        self.assertEqual(paths,
                         (image_dir, cvd_dir, misc_info_path, cvd_dir,
                          system_image_path, boot_image_path))

    @mock.patch("acloud.create.local_image_local_instance.ota_tools")
    def testGetImageFromTargetFiles(self, mock_ota_tools):
        """Test GetImageArtifactsPath with extracted target files."""
        ota_tools_dir = "/mock_ota_tools"
        mock_ota_tools.FindOtaTools.return_value = ota_tools_dir

        with tempfile.TemporaryDirectory() as temp_dir:
            image_dir = os.path.join(temp_dir, "image")
            cvd_dir = os.path.join(temp_dir, "cvd-host_package")
            system_image_path = os.path.join(temp_dir, "system", "test.img")
            misc_info_path = os.path.join(image_dir, "META", "misc_info.txt")
            boot_image_path = os.path.join(temp_dir, "boot", "test.img")
            self._CreateEmptyFile(os.path.join(image_dir, "IMAGES",
                                               "vbmeta.img"))
            self._CreateEmptyFile(os.path.join(cvd_dir, "bin", "launch_cvd"))
            self._CreateEmptyFile(system_image_path)
            self._CreateEmptyFile(misc_info_path)
            self._CreateEmptyFile(boot_image_path)

            mock_avd_spec = mock.Mock(
                local_image_dir=image_dir,
                local_kernel_image=boot_image_path,
                local_system_image=system_image_path,
                local_tool_dirs=[ota_tools_dir, cvd_dir])

            paths = self.local_image_local_instance.GetImageArtifactsPath(
                mock_avd_spec)

        mock_ota_tools.FindOtaTools.assert_called_once()
        self.assertEqual(paths,
                         (os.path.join(image_dir, "IMAGES"), cvd_dir,
                          misc_info_path, ota_tools_dir, system_image_path,
                          boot_image_path))

    @mock.patch.object(utils, "CheckUserInGroups")
    def testPrepareLaunchCVDCmd(self, mock_usergroups):
        """test PrepareLaunchCVDCmd."""
        mock_usergroups.return_value = False
        hw_property = {"cpu": "fake", "x_res": "fake", "y_res": "fake",
                       "dpi":"fake", "memory": "fake", "disk": "fake"}
        constants.LIST_CF_USER_GROUPS = ["group1", "group2"]

        launch_cmd = self.local_image_local_instance.PrepareLaunchCVDCmd(
            constants.CMD_LAUNCH_CVD, hw_property, True, "fake_image_dir",
            "fake_cvd_dir", False, True, None, None, None, "phone")
        self.assertEqual(launch_cmd, self.LAUNCH_CVD_CMD_WITH_DISK)

        # "disk" doesn't exist in hw_property.
        hw_property = {"cpu": "fake", "x_res": "fake", "y_res": "fake",
                       "dpi": "fake", "memory": "fake"}
        launch_cmd = self.local_image_local_instance.PrepareLaunchCVDCmd(
            constants.CMD_LAUNCH_CVD, hw_property, True, "fake_image_dir",
            "fake_cvd_dir", False, True, None, None, None, "phone")
        self.assertEqual(launch_cmd, self.LAUNCH_CVD_CMD_NO_DISK)

        # "gpu" is enabled with "default"
        launch_cmd = self.local_image_local_instance.PrepareLaunchCVDCmd(
            constants.CMD_LAUNCH_CVD, hw_property, True, "fake_image_dir",
            "fake_cvd_dir", False, True, None, None, None, "phone")
        self.assertEqual(launch_cmd, self.LAUNCH_CVD_CMD_NO_DISK_WITH_GPU)

        # Following test with hw_property is None.
        launch_cmd = self.local_image_local_instance.PrepareLaunchCVDCmd(
            constants.CMD_LAUNCH_CVD, None, True, "fake_image_dir",
            "fake_cvd_dir", True, False, None, None, None, "auto")
        self.assertEqual(launch_cmd, self.LAUNCH_CVD_CMD_WITH_WEBRTC)

        launch_cmd = self.local_image_local_instance.PrepareLaunchCVDCmd(
            constants.CMD_LAUNCH_CVD, None, True, "fake_image_dir",
            "fake_cvd_dir", False, True, "fake_super_image", "fake_boot_image",
            None, "phone")
        self.assertEqual(launch_cmd, self.LAUNCH_CVD_CMD_WITH_MIXED_IMAGES)

        # Add args into launch command with "-setupwizard_mode=REQUIRED"
        launch_cmd = self.local_image_local_instance.PrepareLaunchCVDCmd(
            constants.CMD_LAUNCH_CVD, None, True, "fake_image_dir",
            "fake_cvd_dir", False, True, None, None,
            "-setupwizard_mode=REQUIRED", "phone")
        self.assertEqual(launch_cmd, self.LAUNCH_CVD_CMD_WITH_ARGS)

    @mock.patch.object(utils, "GetUserAnswerYes")
    @mock.patch.object(list_instance, "GetActiveCVD")
    def testCheckRunningCvd(self, mock_cvd_running, mock_get_answer):
        """test _CheckRunningCvd."""
        local_instance_id = 3

        # Test that launch_cvd is running.
        mock_cvd_running.return_value = True
        mock_get_answer.return_value = False
        answer = self.local_image_local_instance._CheckRunningCvd(
            local_instance_id)
        self.assertFalse(answer)

        # Test that launch_cvd is not running.
        mock_cvd_running.return_value = False
        answer = self.local_image_local_instance._CheckRunningCvd(
            local_instance_id)
        self.assertTrue(answer)

    # pylint: disable=protected-access
    @mock.patch("acloud.create.local_image_local_instance.subprocess."
                "check_call")
    @mock.patch.dict("os.environ", clear=True)
    def testLaunchCVD(self, mock_check_call):
        """test _LaunchCvd should call subprocess.check_call with the env."""
        local_instance_id = 3
        launch_cvd_cmd = "launch_cvd"
        host_bins_path = "host_bins_path"
        cvd_home_dir = "fake_home"
        timeout = 100
        cvd_env = {}
        cvd_env[constants.ENV_CVD_HOME] = cvd_home_dir
        cvd_env[constants.ENV_CUTTLEFISH_INSTANCE] = str(local_instance_id)
        cvd_env[constants.ENV_ANDROID_SOONG_HOST_OUT] = host_bins_path
        cvd_env[constants.ENV_ANDROID_HOST_OUT] = host_bins_path

        self.local_image_local_instance._LaunchCvd(launch_cvd_cmd,
                                                   local_instance_id,
                                                   host_bins_path,
                                                   cvd_home_dir,
                                                   timeout)

        mock_check_call.assert_called_once_with(launch_cvd_cmd,
                                                shell=True,
                                                stderr=subprocess.STDOUT,
                                                env=cvd_env,
                                                timeout=timeout)

    @mock.patch("acloud.create.local_image_local_instance.subprocess."
                "check_call")
    def testLaunchCVDTimeout(self, mock_check_call):
        """test _LaunchCvd with subprocess errors."""
        mock_check_call.side_effect = subprocess.TimeoutExpired(
            cmd="launch_cvd", timeout=100)
        with self.assertRaises(errors.LaunchCVDFail):
            self.local_image_local_instance._LaunchCvd("launch_cvd",
                                                       3,
                                                       "host_bins_path",
                                                       "cvd_home_dir",
                                                       100)

        mock_check_call.side_effect = subprocess.CalledProcessError(
            cmd="launch_cvd", returncode=1)
        with self.assertRaises(errors.LaunchCVDFail):
            self.local_image_local_instance._LaunchCvd("launch_cvd",
                                                       3,
                                                       "host_bins_path",
                                                       "cvd_home_dir",
                                                       100)

    def testGetWebrtcSigServerPort(self):
        """test GetWebrtcSigServerPort."""
        instance_id = 3
        expected_port = 8445
        self.assertEqual(
            self.local_image_local_instance.GetWebrtcSigServerPort(instance_id),
            expected_port)


if __name__ == "__main__":
    unittest.main()
