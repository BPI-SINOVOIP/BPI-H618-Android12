#!/usr/bin/env python3
#
# Copyright (C) 2019 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
import os
import pandas as pd
import acts_contrib.test_utils.bt.bt_test_utils as btutils
import acts_contrib.test_utils.wifi.wifi_performance_test_utils as wifi_utils
from acts import asserts
from acts_contrib.test_utils.bt import bt_constants
from acts_contrib.test_utils.bt import BtEnum
from acts_contrib.test_utils.bt.A2dpBaseTest import A2dpBaseTest
from acts_contrib.test_utils.bt.loggers import bluetooth_metric_logger as log
from acts.test_utils.power.PowerBTBaseTest import ramp_attenuation
from acts.signals import TestPass


class BtA2dpRangeTest(A2dpBaseTest):
    def __init__(self, configs):
        super().__init__(configs)
        self.bt_logger = log.BluetoothMetricLogger.for_test_case()
        req_params = ['attenuation_vector', 'codecs']
        #'attenuation_vector' is a dict containing: start, stop and step of
        #attenuation changes
        #'codecs' is a list containing all codecs required in the tests
        self.unpack_userparams(req_params)
        for codec_config in self.codecs:
            self.generate_test_case(codec_config)

    def setup_class(self):
        super().setup_class()
        # Enable BQR on all android devices
        btutils.enable_bqr(self.android_devices)

    def generate_test_case(self, codec_config):
        def test_case_fn():
            self.run_a2dp_to_max_range(codec_config)

        test_case_name = 'test_bt_a2dp_range_codec_{}'.format(
            codec_config['codec_type'])
        setattr(self, test_case_name, test_case_fn)

    def generate_proto(self, data_points, codec_type, sample_rate,
                       bits_per_sample, channel_mode):
        """Generate a results protobuf.

        Args:
            data_points: list of dicts representing info to go into
              AudioTestDataPoint protobuffer message.
            codec_type: The codec type config to store in the proto.
            sample_rate: The sample rate config to store in the proto.
            bits_per_sample: The bits per sample config to store in the proto.
            channel_mode: The channel mode config to store in the proto.
        Returns:
             dict: Dictionary with key 'proto' mapping to serialized protobuf,
               'proto_ascii' mapping to human readable protobuf info, and 'test'
               mapping to the test class name that generated the results.
        """

        # Populate protobuf
        test_case_proto = self.bt_logger.proto_module.BluetoothAudioTestResult(
        )

        for data_point in data_points:
            audio_data_proto = test_case_proto.data_points.add()
            log.recursive_assign(audio_data_proto, data_point)

        codec_proto = test_case_proto.a2dp_codec_config
        codec_proto.codec_type = bt_constants.codec_types[codec_type]
        codec_proto.sample_rate = int(sample_rate)
        codec_proto.bits_per_sample = int(bits_per_sample)
        codec_proto.channel_mode = bt_constants.channel_modes[channel_mode]

        self.bt_logger.add_config_data_to_proto(test_case_proto, self.dut,
                                                self.bt_device)

        self.bt_logger.add_proto_to_results(test_case_proto,
                                            self.__class__.__name__)

        proto_dict = self.bt_logger.get_proto_dict(self.__class__.__name__,
                                                   test_case_proto)
        del proto_dict["proto_ascii"]
        return proto_dict

    def plot_graph(self, df):
        """ Plotting A2DP DUT RSSI, remote RSSI and TX Power with Attenuation.

        Args:
            df: Summary of results contains attenuation, DUT RSSI, remote RSSI and Tx Power
        """
        self.plot = wifi_utils.BokehFigure(title='{}'.format(
            self.current_test_name),
                                           x_label='Pathloss (dBm)',
                                           primary_y_label='RSSI (dBm)',
                                           secondary_y_label='TX Power (dBm)',
                                           axis_label_size='16pt')
        self.plot.add_line(df.index,
                           df['rssi_primary'],
                           legend='DUT RSSI (dBm)',
                           marker='circle_x')
        self.plot.add_line(df.index,
                           df['rssi_secondary'],
                           legend='Remote RSSI (dBm)',
                           marker='square_x')
        self.plot.add_line(df.index,
                           df['tx_power_level_master'],
                           legend='DUT TX Power (dBm)',
                           marker='hex',
                           y_axis='secondary')

        results_file_path = os.path.join(
            self.log_path, '{}.html'.format(self.current_test_name))
        self.plot.generate_figure()
        wifi_utils.BokehFigure.save_figures([self.plot], results_file_path)

    def run_a2dp_to_max_range(self, codec_config):
        attenuation_range = range(self.attenuation_vector['start'],
                                  self.attenuation_vector['stop'] + 1,
                                  self.attenuation_vector['step'])

        data_points = []
        self.file_output = os.path.join(
            self.log_path, '{}.csv'.format(self.current_test_name))

        # Set Codec if needed
        current_codec = self.dut.droid.bluetoothA2dpGetCurrentCodecConfig()
        current_codec_type = BtEnum.BluetoothA2dpCodecType(
            current_codec['codecType']).name
        if current_codec_type != codec_config['codec_type']:
            codec_set = btutils.set_bluetooth_codec(self.dut, **codec_config)
            asserts.assert_true(codec_set, 'Codec configuration failed.')
        else:
            self.log.info('Current codec is {}, no need to change'.format(
                current_codec_type))

        #loop RSSI with the same codec setting
        for atten in attenuation_range:
            ramp_attenuation(self.attenuator, atten)
            self.log.info('Set attenuation to %d dB', atten)

            tag = 'codec_{}_attenuation_{}dB_'.format(
                codec_config['codec_type'], atten)
            recorded_file = self.play_and_record_audio(
                self.audio_params['duration'])
            [rssi_master, pwl_master, rssi_slave] = self._get_bt_link_metrics()
            thdns = self.run_thdn_analysis(recorded_file, tag)
            # Collect Metrics for dashboard
            data_point = {
                'attenuation_db': int(self.attenuator.get_atten()),
                'rssi_primary': rssi_master[self.dut.serial],
                'tx_power_level_master': pwl_master[self.dut.serial],
                'rssi_secondary': rssi_slave[self.bt_device_controller.serial],
                'total_harmonic_distortion_plus_noise_percent': thdns[0] * 100
            }
            data_points.append(data_point)
            self.log.info(data_point)
            A2dpRange_df = pd.DataFrame(data_points)

            # Check thdn for glitches, stop if max range reached
            for thdn in thdns:
                if thdn >= self.audio_params['thdn_threshold']:
                    self.log.info(
                        'Max range at attenuation {} dB'.format(atten))
                    self.log.info('DUT rssi {} dBm, DUT tx power level {}, '
                                  'Remote rssi {} dBm'.format(
                                      rssi_master, pwl_master, rssi_slave))
                    proto_dict = self.generate_proto(data_points,
                                                     **codec_config)
                    A2dpRange_df.to_csv(self.file_output, index=False)
                    self.plot_graph(A2dpRange_df)
                    raise TestPass('Max range reached and move to next codec',
                                   extras=proto_dict)
        # Save Data points to csv
        A2dpRange_df.to_csv(self.file_output, index=False)
        # Plot graph
        self.plot_graph(A2dpRange_df)
        proto_dict = self.generate_proto(data_points, **codec_config)
        raise TestPass('Could not reach max range, need extra attenuation.',
                       extras=proto_dict)