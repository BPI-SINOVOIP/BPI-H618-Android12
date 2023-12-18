#!/usr/bin/env python3
#
#   Copyright 2020 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the 'License');
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an 'AS IS' BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from datetime import datetime
import unittest

from acts.controllers.bits_lib import bits_client
from acts.controllers.bits_lib import bits_service_config
import mock

CONTROLLER_CONFIG_WITH_MONSOON = {
    'Monsoon': {'serial_num': 1234, 'monsoon_voltage': 4.2}
}

MONSOONED_CONFIG = bits_service_config.BitsServiceConfig(
    CONTROLLER_CONFIG_WITH_MONSOON, lvpm_monsoon_bin='lvpm.par')

CONTROLLER_CONFIG_WITHOUT_MONSOON = {}

NON_MONSOONED_CONFIG = bits_service_config.BitsServiceConfig(
    CONTROLLER_CONFIG_WITHOUT_MONSOON)


class BitsClientTest(unittest.TestCase):

    def setUp(self):
        super().setUp()
        self.mock_service = mock.Mock()
        self.mock_service.port = '42'
        self.mock_active_collection = mock.Mock()
        self.mock_active_collection.name = 'my_active_collection'
        self.mock_active_collection.markers_buffer = []

    @mock.patch('acts.libs.proc.job.run')
    def test_start_collection__without_monsoon__does_not_disconnect_monsoon(
        self,
        mock_run):
        client = bits_client.BitsClient('bits.par', self.mock_service,
                                        service_config=NON_MONSOONED_CONFIG)

        client.start_collection()

        mock_run.assert_called()
        args_list = mock_run.call_args_list
        non_expected_call = list(
            filter(lambda call: 'usb_disconnect' in call.args[0],
                   args_list))
        self.assertEqual(len(non_expected_call), 0,
                         'did not expect call with usb_disconnect')

    @mock.patch('acts.context.get_current_context')
    @mock.patch('acts.libs.proc.job.run')
    def test_stop_collection__usb_not_automanaged__does_not_connect_monsoon(
        self,
        mock_run,
        mock_context):
        output_path = mock.MagicMock(return_value='out')
        mock_context.side_effect = lambda: output_path
        client = bits_client.BitsClient('bits.par', self.mock_service,
                                        service_config=MONSOONED_CONFIG)
        client._active_collection = self.mock_active_collection

        client.stop_collection()

        mock_run.assert_called()
        args_list = mock_run.call_args_list
        non_expected_call = list(
            filter(lambda call: 'usb_connect' in call.args[0], args_list))
        self.assertEquals(len(non_expected_call), 0,
                          'did not expect call with usb_connect')

    @mock.patch('acts.context.get_current_context')
    @mock.patch('acts.libs.proc.job.run')
    def test_stop_collection__triggers_export(self, mock_run, mock_context):
        output_path = mock.MagicMock(return_value='out')
        mock_context.side_effect = lambda: output_path
        client = bits_client.BitsClient('bits.par', self.mock_service,
                                        service_config=MONSOONED_CONFIG)
        client._active_collection = self.mock_active_collection

        client.stop_collection()

        mock_run.assert_called()
        args_list = mock_run.call_args_list
        expected_call = list(
            filter(lambda call: '--export' in call.args[0], args_list))
        self.assertEqual(len(expected_call), 1,
                         'expected a call with --export')

    @mock.patch('acts.context.get_current_context')
    @mock.patch('acts.libs.proc.job.run')
    def test__export_ignores_dataseries_gaps(self, mock_run, mock_context):
        output_path = mock.MagicMock(return_value='out')
        mock_context.side_effect = lambda: output_path
        client = bits_client.BitsClient('bits.par', self.mock_service,
                                        service_config=MONSOONED_CONFIG)
        client._active_collection = self.mock_active_collection

        client._export()

        mock_run.assert_called()
        args_list = mock_run.call_args_list
        expected_call = list(
            filter(
                lambda call: '--ignore_gaps' in call.args[0] and '--export' in
                             call.args[0], args_list))
        self.assertEqual(len(expected_call), 1,
                         'expected a call with --ignore_gaps and --export')
        self.assertIn('--ignore_gaps', expected_call[0].args[0])

    @mock.patch('acts.libs.proc.job.run')
    def test_add_marker(self, _):
        client = bits_client.BitsClient('bits.par', self.mock_service,
                                        service_config=MONSOONED_CONFIG)
        client._active_collection = self.mock_active_collection

        client.add_marker(7133, 'my marker')

        client._active_collection.add_marker.assert_called_with(7133,
                                                                'my marker')

    @mock.patch('acts.context.get_current_context')
    @mock.patch('acts.libs.proc.job.run')
    def test_stop_collection__flushes_buffered_markers(self, mock_run,
                                                       mock_context):
        output_path = mock.MagicMock(return_value='out')
        mock_context.side_effect = lambda: output_path
        client = bits_client.BitsClient('bits.par', self.mock_service,
                                        service_config=MONSOONED_CONFIG)
        self.mock_active_collection.markers_buffer.append((3, 'tres'))
        self.mock_active_collection.markers_buffer.append((1, 'uno'))
        self.mock_active_collection.markers_buffer.append((2, 'dos'))
        client._active_collection = self.mock_active_collection

        client.stop_collection()

        mock_run.assert_called()
        args_list = mock_run.call_args_list
        expected_calls = list(
            filter(lambda call: '--log' in call.args[0], args_list))
        self.assertEqual(len(expected_calls), 3,
                         'expected 3 calls with --log')
        self.assertIn('--log_ts', expected_calls[0][0][0])
        self.assertIn('1', expected_calls[0][0][0])
        self.assertIn('uno', expected_calls[0][0][0])
        self.assertIn('--log_ts', expected_calls[1][0][0])
        self.assertIn('2', expected_calls[1][0][0])
        self.assertIn('dos', expected_calls[1][0][0])
        self.assertIn('--log_ts', expected_calls[2][0][0])
        self.assertIn('3', expected_calls[2][0][0])
        self.assertIn('tres', expected_calls[2][0][0])
        self.mock_active_collection.clear_markers_buffer.assert_called()

    @mock.patch('acts.context.get_current_context')
    @mock.patch('acts.libs.proc.job.run')
    def test_stop_collection__flushes_buffered_datetime_markers(self,
                                                                mock_run,
                                                                mock_context):
        output_path = mock.MagicMock(return_value='out')
        mock_context.side_effect = lambda: output_path
        client = bits_client.BitsClient('bits.par', self.mock_service,
                                        service_config=MONSOONED_CONFIG)
        self.mock_active_collection.markers_buffer.append(
            (datetime.utcfromtimestamp(3), 'tres'))
        self.mock_active_collection.markers_buffer.append(
            (datetime.utcfromtimestamp(1), 'uno'))
        self.mock_active_collection.markers_buffer.append(
            (datetime.utcfromtimestamp(2), 'dos'))
        client._active_collection = self.mock_active_collection

        client.stop_collection()

        mock_run.assert_called()
        args_list = mock_run.call_args_list
        expected_calls = list(
            filter(lambda call: '--log' in call.args[0], args_list))
        self.assertEqual(len(expected_calls), 3,
                         'expected 3 calls with --log')
        self.assertIn('--log_ts', expected_calls[0][0][0])
        self.assertIn(str(int(1e9)), expected_calls[0][0][0])
        self.assertIn('uno', expected_calls[0][0][0])
        self.assertIn('--log_ts', expected_calls[1][0][0])
        self.assertIn(str(int(2e9)), expected_calls[1][0][0])
        self.assertIn('dos', expected_calls[1][0][0])
        self.assertIn('--log_ts', expected_calls[2][0][0])
        self.assertIn(str(int(3e9)), expected_calls[2][0][0])
        self.assertIn('tres', expected_calls[2][0][0])
        self.mock_active_collection.clear_markers_buffer.assert_called()

    @mock.patch('acts.libs.proc.job.run')
    def test_get_metrics(self, mock_run):
        client = bits_client.BitsClient('bits.par', self.mock_service,
                                        service_config=MONSOONED_CONFIG)
        client._active_collection = self.mock_active_collection

        client.get_metrics(8888, 9999)

        mock_run.assert_called()
        args_list = mock_run.call_args_list
        expected_call = list(
            filter(lambda call: '--aggregates_yaml_path' in call.args[0],
                   args_list))
        self.assertEqual(len(expected_call), 1,
                         'expected a call with --aggregates_yaml_path')
        self.assertIn('8888', expected_call[0][0][0])
        self.assertIn('--ignore_gaps', expected_call[0][0][0])
        self.assertIn('--abs_stop_time', expected_call[0][0][0])
        self.assertIn('9999', expected_call[0][0][0])

    @mock.patch('acts.libs.proc.job.run')
    def test_get_metrics_with_datetime_markers(self, mock_run):
        client = bits_client.BitsClient('bits.par', self.mock_service,
                                        service_config=MONSOONED_CONFIG)
        client._active_collection = self.mock_active_collection

        client.get_metrics(datetime.utcfromtimestamp(1),
                           datetime.utcfromtimestamp(2))

        mock_run.assert_called()
        args_list = mock_run.call_args_list
        expected_call = list(
            filter(lambda call: '--aggregates_yaml_path' in call.args[0],
                   args_list))
        self.assertEqual(len(expected_call), 1,
                         'expected a call with --aggregates_yaml_path')
        self.assertIn(str(int(1e9)), expected_call[0][0][0])
        self.assertIn('--ignore_gaps', expected_call[0][0][0])
        self.assertIn('--abs_stop_time', expected_call[0][0][0])
        self.assertIn(str(int(2e9)), expected_call[0][0][0])

    @mock.patch('acts.libs.proc.job.run')
    def test_get_metrics_with_virtual_metrics_file(self, mock_run):
        service_config = mock.Mock()
        service_config.has_virtual_metrics_file = True
        client = bits_client.BitsClient('bits.par', self.mock_service,
                                        service_config=service_config)
        client._active_collection = self.mock_active_collection

        client.get_metrics(8888, 9999)

        mock_run.assert_called()
        args_list = mock_run.call_args_list
        expected_call = list(
            filter(lambda call: '--aggregates_yaml_path' in call.args[0],
                   args_list))
        self.assertEqual(len(expected_call), 1,
                         'expected a call with --aggregates_yaml_path')
        self.assertIn('--vm_file', expected_call[0][0][0])
        self.assertIn('default', expected_call[0][0][0])


if __name__ == '__main__':
    unittest.main()
