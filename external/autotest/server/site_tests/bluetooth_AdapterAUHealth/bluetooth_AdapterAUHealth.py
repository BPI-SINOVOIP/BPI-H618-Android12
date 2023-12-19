# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A Batch of Bluetooth AUdio Health tests"""

import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.bluetooth.bluetooth_audio_test_data import (
        A2DP, A2DP_LONG, AVRCP, HFP_WBS, HFP_NBS)
from autotest_lib.server.cros.bluetooth.bluetooth_adapter_audio_tests import (
        BluetoothAdapterAudioTests)
from autotest_lib.server.cros.bluetooth.bluetooth_adapter_quick_tests import (
        BluetoothAdapterQuickTests)


class bluetooth_AdapterAUHealth(BluetoothAdapterQuickTests,
                                BluetoothAdapterAudioTests):
    """A Batch of Bluetooth audio health tests."""

    test_wrapper = BluetoothAdapterQuickTests.quick_test_test_decorator
    batch_wrapper = BluetoothAdapterQuickTests.quick_test_batch_decorator


    def au_run_method(self, device, test_method, test_profile):
        """audio procedure of running a specified test method.

        @param device: the bt peer device
        @param test_method: the audio test method to run
        @param test_profile: which test profile is used,
                             A2DP, HFP_WBS or HFP_NBS
        """
        self.test_reset_on_adapter()
        self.test_bluetoothd_running()
        self.initialize_bluetooth_audio(device, test_profile)
        self.test_device_set_discoverable(device, True)
        self.test_discover_device(device.address)
        self.test_pairing(device.address, device.pin, trusted=True)
        self.test_connection_by_adapter(device.address)
        test_method()
        self.test_disconnection_by_adapter(device.address)
        self.cleanup_bluetooth_audio(device, test_profile)


    def _au_a2dp_test(self, test_profile, duration=0):
        """A2DP test with sinewaves on the two channels.

        @param test_profile: which test profile is used, A2DP or A2DP_LONG.
        @param duration: the duration to test a2dp. The unit is in seconds.
                if duration is 0, use the default duration in test_profile.
        """
        device = self.devices['BLUETOOTH_AUDIO'][0]
        self.au_run_method(device,
                           lambda: self.test_a2dp_sinewaves(
                                   device, test_profile, duration),
                           test_profile)


    @test_wrapper('A2DP sinewave test', devices={'BLUETOOTH_AUDIO':1})
    def au_a2dp_test(self):
        """A2DP test with sinewaves on the two channels."""
        self._au_a2dp_test(A2DP)

    # The A2DP long test is a stress test. Exclude it from the AVL.
    @test_wrapper('A2DP sinewave long test', devices={'BLUETOOTH_AUDIO':1},
                  flags=['Quick Health'])
    def au_a2dp_long_test(self, duration=600):
        """A2DP long test with sinewaves on the two channels.

        @param duration: the duration to test a2dp. The unit is in seconds.
        """
        self._au_a2dp_test(A2DP_LONG, duration=duration)


    def check_wbs_capability(self):
        """Check if the DUT supports WBS capability.

        @raises: TestNAError if the dut does not support wbs.
        """
        capabilities, err = self.bluetooth_facade.get_supported_capabilities()
        return err is None and bool(capabilities.get('wide band speech'))


    def au_hfp_run_method(self, device, test_method, test_profile):
        """Run an HFP test with the specified test method.

        @param device: the bt peer device
        @param test_method: the specific HFP WBS test method
        @param test_profile: which test profile is used, HFP_WBS or HFP_NBS
        """
        if self.check_wbs_capability():
            if test_profile == HFP_WBS:
                # Restart cras to ensure that cras goes back to the default
                # selection of either WBS or NBS.
                # Any board that supports WBS should use WBS by default, unless
                # it's overridden by CRAS' config.
                # Do not enable WBS explicitly in the test so we can catch if
                # the default selection goes wrong.
                self.restart_cras()
                # The audio team suggests a simple 2-second sleep.
                time.sleep(2)
            elif test_profile == HFP_NBS:
                # Cras may be in either WBS or NBS mode. Disable WBS explicitly.
                if not self.bluetooth_facade.enable_wbs(False):
                    raise error.TestError('failed to disable wbs')
        else:
            if test_profile == HFP_WBS:
                # Skip the WBS test on a board that does not support WBS.
                raise error.TestNAError(
                        'The DUT does not support WBS. Skip the test.')
            elif test_profile == HFP_NBS:
                # Restart cras to ensure that cras goes back to the default
                # selection of either WBS or NBS.
                # Any board that does not support WBS should use NBS by default.
                # Do not enable NBS explicitly in the test so we can catch if
                # the default selection goes wrong.
                self.restart_cras()
                # The audio team suggests a simple 2-second sleep.
                time.sleep(2)

        self.au_run_method(
                device, lambda: test_method(device, test_profile), test_profile)


    # TODO(b/163284498) Realtek not ready for WBS yet pending on cras patches.
    @test_wrapper('HFP WBS sinewave test with dut as source',
                  skip_chipsets=['Realtek-RTL8822C-USB'],
                  devices={'BLUETOOTH_AUDIO':1})
    def au_hfp_wbs_dut_as_source_test(self):
        """HFP WBS test with sinewave streaming from dut to peer."""
        device = self.devices['BLUETOOTH_AUDIO'][0]
        self.au_hfp_run_method(device, self.test_hfp_dut_as_source, HFP_WBS)


    # TODO(b/163284498) Realtek not ready for WBS yet pending on cras patches.
    @test_wrapper('HFP WBS sinewave test with dut as sink',
                  skip_chipsets=['Realtek-RTL8822C-USB'],
                  devices={'BLUETOOTH_AUDIO':1})
    def au_hfp_wbs_dut_as_sink_test(self):
        """HFP WBS test with sinewave streaming from peer to dut."""
        device = self.devices['BLUETOOTH_AUDIO'][0]
        self.au_hfp_run_method(device, self.test_hfp_dut_as_sink, HFP_WBS)


    @test_wrapper('HFP NBS sinewave test with dut as source',
                  devices={'BLUETOOTH_AUDIO':1})
    def au_hfp_nbs_dut_as_source_test(self):
        """HFP NBS test with sinewave streaming from dut to peer."""
        device = self.devices['BLUETOOTH_AUDIO'][0]
        self.au_hfp_run_method(device, self.test_hfp_dut_as_source, HFP_NBS)


    @test_wrapper('HFP NBS sinewave test with dut as sink',
                  devices={'BLUETOOTH_AUDIO':1})
    def au_hfp_nbs_dut_as_sink_test(self):
        """HFP NBS test with sinewave streaming from peer to dut."""
        device = self.devices['BLUETOOTH_AUDIO'][0]
        self.au_hfp_run_method(device, self.test_hfp_dut_as_sink, HFP_NBS)


    @test_wrapper('HFP WBS VISQOL test with dut as sink',
                  devices={'BLUETOOTH_AUDIO':1})
    def au_hfp_wbs_dut_as_sink_visqol_test(self):
        """HFP WBS VISQOL test with audio streaming from peer to dut"""
        device = self.devices['BLUETOOTH_AUDIO'][0]
        self.au_hfp_run_method(device, self.test_hfp_dut_as_sink_visqol_score,
                               HFP_WBS)


    @test_wrapper('HFP WBS VISQOL test with dut as source',
                  devices={'BLUETOOTH_AUDIO':1})
    def au_hfp_wbs_dut_as_source_visqol_test(self):
        """HFP WBS VISQOL test with audio streaming from dut to peer"""
        device = self.devices['BLUETOOTH_AUDIO'][0]
        self.au_hfp_run_method(device, self.test_hfp_dut_as_source_visqol_score,
                               HFP_WBS)

    @test_wrapper('HFP NBS VISQOL test with dut as sink',
                  devices={'BLUETOOTH_AUDIO':1})
    def au_hfp_nbs_dut_as_sink_visqol_test(self):
        """HFP NBS VISQOL test with audio streaming from peer to dut"""
        device = self.devices['BLUETOOTH_AUDIO'][0]
        self.au_hfp_run_method(device, self.test_hfp_dut_as_sink_visqol_score,
                               HFP_NBS)


    @test_wrapper('HFP NBS VISQOL test with dut as source',
                  devices={'BLUETOOTH_AUDIO':1})
    def au_hfp_nbs_dut_as_source_visqol_test(self):
        """HFP NBS VISQOL test with audio streaming from dut to peer"""
        device = self.devices['BLUETOOTH_AUDIO'][0]
        self.au_hfp_run_method(device, self.test_hfp_dut_as_source_visqol_score,
                               HFP_NBS)


    def au_run_avrcp_method(self, device, test_method):
        """avrcp procedure of running a specified test method.

        @param device: the bt peer device
        @param test_method: the avrcp test method to run
        """
        def wrapped_test_method(device):
            """A wrapper method to initialize and cleanup avrcp tests.

            @param device: the bt peer device
            """
            self.initialize_bluetooth_player(device)
            test_method(device)
            self.cleanup_bluetooth_player(device)

        self.au_run_method(
                device, lambda: wrapped_test_method(device), AVRCP)


    @test_wrapper('avrcp command test', devices={'BLUETOOTH_AUDIO':1})
    def au_avrcp_command_test(self):
        """AVRCP test to examine commands reception."""
        device = self.devices['BLUETOOTH_AUDIO'][0]
        self.au_run_avrcp_method(device, self.test_avrcp_commands)


    # Add 'Quick Health' to flags to exclude the test from AVL.
    # When this test is stable enough later, remove the flags here.
    @test_wrapper('avrcp media info test', devices={'BLUETOOTH_AUDIO':1},
                  flags=['Quick Health'])
    def au_avrcp_media_info_test(self):
        """AVRCP test to examine metadata propgation."""
        device = self.devices['BLUETOOTH_AUDIO'][0]
        self.au_run_avrcp_method(device, self.test_avrcp_media_info)


    @batch_wrapper('Bluetooth Audio Batch Health Tests')
    def au_health_batch_run(self, num_iterations=1, test_name=None):
        """Run the bluetooth audio health test batch or a specific given test.

        @param num_iterations: how many iterations to run
        @param test_name: specific test to run otherwise None to run the
                whole batch
        """
        self.au_a2dp_test()
        self.au_a2dp_long_test()
        self.au_hfp_nbs_dut_as_source_test()
        self.au_hfp_nbs_dut_as_sink_test()
        self.au_hfp_wbs_dut_as_source_test()
        self.au_hfp_wbs_dut_as_sink_test()
        self.au_hfp_wbs_dut_as_source_visqol_test()
        self.au_hfp_wbs_dut_as_sink_visqol_test()
        self.au_hfp_nbs_dut_as_source_visqol_test()
        self.au_hfp_nbs_dut_as_sink_visqol_test()
        self.au_avrcp_command_test()
        self.au_avrcp_media_info_test()


    def run_once(self,
                 host,
                 num_iterations=1,
                 args_dict=None,
                 test_name=None,
                 flag='Quick Health'):
        """Run the batch of Bluetooth stand health tests

        @param host: the DUT, usually a chromebook
        @param num_iterations: the number of rounds to execute the test
        @param test_name: the test to run, or None for all tests
        """
        self.host = host

        self.quick_test_init(host,
                             use_btpeer=True,
                             flag=flag,
                             args_dict=args_dict)
        self.au_health_batch_run(num_iterations, test_name)
        self.quick_test_cleanup()
