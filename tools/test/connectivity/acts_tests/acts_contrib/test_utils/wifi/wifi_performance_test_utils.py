#!/usr/bin/env python3.4
#
#   Copyright 2019 - The Android Open Source Project
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

import bokeh, bokeh.plotting, bokeh.io
import collections
import hashlib
import ipaddress
import itertools
import json
import logging
import math
import os
import re
import statistics
import time
from acts.controllers.android_device import AndroidDevice
from acts.controllers.utils_lib import ssh
from acts import asserts
from acts import utils
from acts_contrib.test_utils.wifi import wifi_test_utils as wutils
from concurrent.futures import ThreadPoolExecutor

SHORT_SLEEP = 1
MED_SLEEP = 6
TEST_TIMEOUT = 10
STATION_DUMP = 'iw wlan0 station dump'
SCAN = 'wpa_cli scan'
SCAN_RESULTS = 'wpa_cli scan_results'
SIGNAL_POLL = 'wpa_cli signal_poll'
WPA_CLI_STATUS = 'wpa_cli status'
DISCONNECTION_MESSAGE_BRCM = 'driver adapter not found'
CONST_3dB = 3.01029995664
RSSI_ERROR_VAL = float('nan')
RTT_REGEX = re.compile(r'^\[(?P<timestamp>\S+)\] .*? time=(?P<rtt>\S+)')
LOSS_REGEX = re.compile(r'(?P<loss>\S+)% packet loss')
FW_REGEX = re.compile(r'FW:(?P<firmware>\S+) HW:')


# Threading decorator
def nonblocking(f):
    """Creates a decorator transforming function calls to non-blocking"""
    def wrap(*args, **kwargs):
        executor = ThreadPoolExecutor(max_workers=1)
        thread_future = executor.submit(f, *args, **kwargs)
        # Ensure resources are freed up when executor ruturns or raises
        executor.shutdown(wait=False)
        return thread_future

    return wrap


# JSON serializer
def serialize_dict(input_dict):
    """Function to serialize dicts to enable JSON output"""
    output_dict = collections.OrderedDict()
    for key, value in input_dict.items():
        output_dict[_serialize_value(key)] = _serialize_value(value)
    return output_dict


def _serialize_value(value):
    """Function to recursively serialize dict entries to enable JSON output"""
    if isinstance(value, tuple):
        return str(value)
    if isinstance(value, list):
        return [_serialize_value(x) for x in value]
    elif isinstance(value, dict):
        return serialize_dict(value)
    else:
        return value


# Miscellaneous Wifi Utilities
def extract_sub_dict(full_dict, fields):
    sub_dict = collections.OrderedDict(
        (field, full_dict[field]) for field in fields)
    return sub_dict


def validate_network(dut, ssid):
    """Check that DUT has a valid internet connection through expected SSID

    Args:
        dut: android device of interest
        ssid: expected ssid
    """
    current_network = dut.droid.wifiGetConnectionInfo()
    try:
        connected = wutils.validate_connection(dut) is not None
    except:
        connected = False
    if connected and current_network['SSID'] == ssid:
        return True
    else:
        return False


def get_server_address(ssh_connection, dut_ip, subnet_mask):
    """Get server address on a specific subnet,

    This function retrieves the LAN or WAN IP of a remote machine used in
    testing. If subnet_mask is set to 'public' it returns a machines global ip,
    else it returns the ip belonging to the dut local network given the dut's
    ip and subnet mask.

    Args:
        ssh_connection: object representing server for which we want an ip
        dut_ip: string in ip address format, i.e., xxx.xxx.xxx.xxx
        subnet_mask: string representing subnet mask (public for global ip)
    """
    ifconfig_out = ssh_connection.run('ifconfig').stdout
    ip_list = re.findall('inet (?:addr:)?(\d+.\d+.\d+.\d+)', ifconfig_out)
    ip_list = [ipaddress.ip_address(ip) for ip in ip_list]

    if subnet_mask == 'public':
        for ip in ip_list:
            # is_global is not used to allow for CGNAT ips in 100.x.y.z range
            if not ip.is_private:
                return str(ip)
    else:
        dut_network = ipaddress.ip_network('{}/{}'.format(dut_ip, subnet_mask),
                                           strict=False)
        for ip in ip_list:
            if ip in dut_network:
                return str(ip)
    logging.error('No IP address found in requested subnet')


# Plotting Utilities
class BokehFigure():
    """Class enabling  simplified Bokeh plotting."""

    COLORS = [
        'black',
        'blue',
        'blueviolet',
        'brown',
        'burlywood',
        'cadetblue',
        'cornflowerblue',
        'crimson',
        'cyan',
        'darkblue',
        'darkgreen',
        'darkmagenta',
        'darkorange',
        'darkred',
        'deepskyblue',
        'goldenrod',
        'green',
        'grey',
        'indigo',
        'navy',
        'olive',
        'orange',
        'red',
        'salmon',
        'teal',
        'yellow',
    ]
    MARKERS = [
        'asterisk', 'circle', 'circle_cross', 'circle_x', 'cross', 'diamond',
        'diamond_cross', 'hex', 'inverted_triangle', 'square', 'square_x',
        'square_cross', 'triangle', 'x'
    ]

    TOOLS = ('box_zoom,box_select,pan,crosshair,redo,undo,reset,hover,save')
    TOOLTIPS = [
        ('index', '$index'),
        ('(x,y)', '($x, $y)'),
        ('info', '@hover_text'),
    ]

    def __init__(self,
                 title=None,
                 x_label=None,
                 primary_y_label=None,
                 secondary_y_label=None,
                 height=700,
                 width=1100,
                 title_size='15pt',
                 axis_label_size='12pt',
                 json_file=None):
        if json_file:
            self.load_from_json(json_file)
        else:
            self.figure_data = []
            self.fig_property = {
                'title': title,
                'x_label': x_label,
                'primary_y_label': primary_y_label,
                'secondary_y_label': secondary_y_label,
                'num_lines': 0,
                'height': height,
                'width': width,
                'title_size': title_size,
                'axis_label_size': axis_label_size
            }

    def init_plot(self):
        self.plot = bokeh.plotting.figure(
            sizing_mode='scale_both',
            plot_width=self.fig_property['width'],
            plot_height=self.fig_property['height'],
            title=self.fig_property['title'],
            tools=self.TOOLS,
            output_backend='webgl')
        self.plot.hover.tooltips = self.TOOLTIPS
        self.plot.add_tools(
            bokeh.models.tools.WheelZoomTool(dimensions='width'))
        self.plot.add_tools(
            bokeh.models.tools.WheelZoomTool(dimensions='height'))

    def _filter_line(self, x_data, y_data, hover_text=None):
        """Function to remove NaN points from bokeh plots."""
        x_data_filtered = []
        y_data_filtered = []
        hover_text_filtered = []
        for x, y, hover in itertools.zip_longest(x_data, y_data, hover_text):
            if not math.isnan(y):
                x_data_filtered.append(x)
                y_data_filtered.append(y)
                hover_text_filtered.append(hover)
        return x_data_filtered, y_data_filtered, hover_text_filtered

    def add_line(self,
                 x_data,
                 y_data,
                 legend,
                 hover_text=None,
                 color=None,
                 width=3,
                 style='solid',
                 marker=None,
                 marker_size=10,
                 shaded_region=None,
                 y_axis='default'):
        """Function to add line to existing BokehFigure.

        Args:
            x_data: list containing x-axis values for line
            y_data: list containing y_axis values for line
            legend: string containing line title
            hover_text: text to display when hovering over lines
            color: string describing line color
            width: integer line width
            style: string describing line style, e.g, solid or dashed
            marker: string specifying line marker, e.g., cross
            shaded region: data describing shaded region to plot
            y_axis: identifier for y-axis to plot line against
        """
        if y_axis not in ['default', 'secondary']:
            raise ValueError('y_axis must be default or secondary')
        if color == None:
            color = self.COLORS[self.fig_property['num_lines'] %
                                len(self.COLORS)]
        if style == 'dashed':
            style = [5, 5]
        if not hover_text:
            hover_text = ['y={}'.format(y) for y in y_data]
        x_data_filter, y_data_filter, hover_text_filter = self._filter_line(
            x_data, y_data, hover_text)
        self.figure_data.append({
            'x_data': x_data_filter,
            'y_data': y_data_filter,
            'legend': legend,
            'hover_text': hover_text_filter,
            'color': color,
            'width': width,
            'style': style,
            'marker': marker,
            'marker_size': marker_size,
            'shaded_region': shaded_region,
            'y_axis': y_axis
        })
        self.fig_property['num_lines'] += 1

    def add_scatter(self,
                    x_data,
                    y_data,
                    legend,
                    hover_text=None,
                    color=None,
                    marker=None,
                    marker_size=10,
                    y_axis='default'):
        """Function to add line to existing BokehFigure.

        Args:
            x_data: list containing x-axis values for line
            y_data: list containing y_axis values for line
            legend: string containing line title
            hover_text: text to display when hovering over lines
            color: string describing line color
            marker: string specifying marker, e.g., cross
            y_axis: identifier for y-axis to plot line against
        """
        if y_axis not in ['default', 'secondary']:
            raise ValueError('y_axis must be default or secondary')
        if color == None:
            color = self.COLORS[self.fig_property['num_lines'] %
                                len(self.COLORS)]
        if marker == None:
            marker = self.MARKERS[self.fig_property['num_lines'] %
                                  len(self.MARKERS)]
        if not hover_text:
            hover_text = ['y={}'.format(y) for y in y_data]
        self.figure_data.append({
            'x_data': x_data,
            'y_data': y_data,
            'legend': legend,
            'hover_text': hover_text,
            'color': color,
            'width': 0,
            'style': 'solid',
            'marker': marker,
            'marker_size': marker_size,
            'shaded_region': None,
            'y_axis': y_axis
        })
        self.fig_property['num_lines'] += 1

    def generate_figure(self, output_file=None, save_json=True):
        """Function to generate and save BokehFigure.

        Args:
            output_file: string specifying output file path
        """
        self.init_plot()
        two_axes = False
        for line in self.figure_data:
            source = bokeh.models.ColumnDataSource(
                data=dict(x=line['x_data'],
                          y=line['y_data'],
                          hover_text=line['hover_text']))
            if line['width'] > 0:
                self.plot.line(x='x',
                               y='y',
                               legend_label=line['legend'],
                               line_width=line['width'],
                               color=line['color'],
                               line_dash=line['style'],
                               name=line['y_axis'],
                               y_range_name=line['y_axis'],
                               source=source)
            if line['shaded_region']:
                band_x = line['shaded_region']['x_vector']
                band_x.extend(line['shaded_region']['x_vector'][::-1])
                band_y = line['shaded_region']['lower_limit']
                band_y.extend(line['shaded_region']['upper_limit'][::-1])
                self.plot.patch(band_x,
                                band_y,
                                color='#7570B3',
                                line_alpha=0.1,
                                fill_alpha=0.1)
            if line['marker'] in self.MARKERS:
                marker_func = getattr(self.plot, line['marker'])
                marker_func(x='x',
                            y='y',
                            size=line['marker_size'],
                            legend_label=line['legend'],
                            line_color=line['color'],
                            fill_color=line['color'],
                            name=line['y_axis'],
                            y_range_name=line['y_axis'],
                            source=source)
            if line['y_axis'] == 'secondary':
                two_axes = True

        #x-axis formatting
        self.plot.xaxis.axis_label = self.fig_property['x_label']
        self.plot.x_range.range_padding = 0
        self.plot.xaxis[0].axis_label_text_font_size = self.fig_property[
            'axis_label_size']
        #y-axis formatting
        self.plot.yaxis[0].axis_label = self.fig_property['primary_y_label']
        self.plot.yaxis[0].axis_label_text_font_size = self.fig_property[
            'axis_label_size']
        self.plot.y_range = bokeh.models.DataRange1d(names=['default'])
        if two_axes and 'secondary' not in self.plot.extra_y_ranges:
            self.plot.extra_y_ranges = {
                'secondary': bokeh.models.DataRange1d(names=['secondary'])
            }
            self.plot.add_layout(
                bokeh.models.LinearAxis(
                    y_range_name='secondary',
                    axis_label=self.fig_property['secondary_y_label'],
                    axis_label_text_font_size=self.
                    fig_property['axis_label_size']), 'right')
        # plot formatting
        self.plot.legend.location = 'top_right'
        self.plot.legend.click_policy = 'hide'
        self.plot.title.text_font_size = self.fig_property['title_size']

        if output_file is not None:
            self.save_figure(output_file, save_json)
        return self.plot

    def load_from_json(self, file_path):
        with open(file_path, 'r') as json_file:
            fig_dict = json.load(json_file)
        self.fig_property = fig_dict['fig_property']
        self.figure_data = fig_dict['figure_data']

    def _save_figure_json(self, output_file):
        """Function to save a json format of a figure"""
        figure_dict = collections.OrderedDict(fig_property=self.fig_property,
                                              figure_data=self.figure_data)
        output_file = output_file.replace('.html', '_plot_data.json')
        with open(output_file, 'w') as outfile:
            json.dump(figure_dict, outfile, indent=4)

    def save_figure(self, output_file, save_json=True):
        """Function to save BokehFigure.

        Args:
            output_file: string specifying output file path
            save_json: flag controlling json outputs
        """
        if save_json:
            self._save_figure_json(output_file)
        bokeh.io.output_file(output_file)
        bokeh.io.save(self.plot)

    @staticmethod
    def save_figures(figure_array, output_file_path, save_json=True):
        """Function to save list of BokehFigures in one file.

        Args:
            figure_array: list of BokehFigure object to be plotted
            output_file: string specifying output file path
        """
        for idx, figure in enumerate(figure_array):
            figure.generate_figure()
            if save_json:
                json_file_path = output_file_path.replace(
                    '.html', '{}-plot_data.json'.format(idx))
                figure._save_figure_json(json_file_path)
        plot_array = [figure.plot for figure in figure_array]
        all_plots = bokeh.layouts.column(children=plot_array,
                                         sizing_mode='scale_width')
        bokeh.plotting.output_file(output_file_path)
        bokeh.plotting.save(all_plots)


# Ping utilities
class PingResult(object):
    """An object that contains the results of running ping command.

    Attributes:
        connected: True if a connection was made. False otherwise.
        packet_loss_percentage: The total percentage of packets lost.
        transmission_times: The list of PingTransmissionTimes containing the
            timestamps gathered for transmitted packets.
        rtts: An list-like object enumerating all round-trip-times of
            transmitted packets.
        timestamps: A list-like object enumerating the beginning timestamps of
            each packet transmission.
        ping_interarrivals: A list-like object enumerating the amount of time
            between the beginning of each subsequent transmission.
    """
    def __init__(self, ping_output):
        self.packet_loss_percentage = 100
        self.transmission_times = []

        self.rtts = _ListWrap(self.transmission_times, lambda entry: entry.rtt)
        self.timestamps = _ListWrap(self.transmission_times,
                                    lambda entry: entry.timestamp)
        self.ping_interarrivals = _PingInterarrivals(self.transmission_times)

        self.start_time = 0
        for line in ping_output:
            if 'loss' in line:
                match = re.search(LOSS_REGEX, line)
                self.packet_loss_percentage = float(match.group('loss'))
            if 'time=' in line:
                match = re.search(RTT_REGEX, line)
                if self.start_time == 0:
                    self.start_time = float(match.group('timestamp'))
                self.transmission_times.append(
                    PingTransmissionTimes(
                        float(match.group('timestamp')) - self.start_time,
                        float(match.group('rtt'))))
        self.connected = len(
            ping_output) > 1 and self.packet_loss_percentage < 100

    def __getitem__(self, item):
        if item == 'rtt':
            return self.rtts
        if item == 'connected':
            return self.connected
        if item == 'packet_loss_percentage':
            return self.packet_loss_percentage
        raise ValueError('Invalid key. Please use an attribute instead.')

    def as_dict(self):
        return {
            'connected': 1 if self.connected else 0,
            'rtt': list(self.rtts),
            'time_stamp': list(self.timestamps),
            'ping_interarrivals': list(self.ping_interarrivals),
            'packet_loss_percentage': self.packet_loss_percentage
        }


class PingTransmissionTimes(object):
    """A class that holds the timestamps for a packet sent via the ping command.

    Attributes:
        rtt: The round trip time for the packet sent.
        timestamp: The timestamp the packet started its trip.
    """
    def __init__(self, timestamp, rtt):
        self.rtt = rtt
        self.timestamp = timestamp


class _ListWrap(object):
    """A convenient helper class for treating list iterators as native lists."""
    def __init__(self, wrapped_list, func):
        self.__wrapped_list = wrapped_list
        self.__func = func

    def __getitem__(self, key):
        return self.__func(self.__wrapped_list[key])

    def __iter__(self):
        for item in self.__wrapped_list:
            yield self.__func(item)

    def __len__(self):
        return len(self.__wrapped_list)


class _PingInterarrivals(object):
    """A helper class for treating ping interarrivals as a native list."""
    def __init__(self, ping_entries):
        self.__ping_entries = ping_entries

    def __getitem__(self, key):
        return (self.__ping_entries[key + 1].timestamp -
                self.__ping_entries[key].timestamp)

    def __iter__(self):
        for index in range(len(self.__ping_entries) - 1):
            yield self[index]

    def __len__(self):
        return max(0, len(self.__ping_entries) - 1)


def get_ping_stats(src_device, dest_address, ping_duration, ping_interval,
                   ping_size):
    """Run ping to or from the DUT.

    The function computes either pings the DUT or pings a remote ip from
    DUT.

    Args:
        src_device: object representing device to ping from
        dest_address: ip address to ping
        ping_duration: timeout to set on the the ping process (in seconds)
        ping_interval: time between pings (in seconds)
        ping_size: size of ping packet payload
    Returns:
        ping_result: dict containing ping results and other meta data
    """
    ping_count = int(ping_duration / ping_interval)
    ping_deadline = int(ping_count * ping_interval) + 1
    ping_cmd_linux = 'ping -c {} -w {} -i {} -s {} -D'.format(
        ping_count,
        ping_deadline,
        ping_interval,
        ping_size,
    )

    ping_cmd_macos = 'ping -c {} -t {} -i {} -s {}'.format(
        ping_count,
        ping_deadline,
        ping_interval,
        ping_size,
    )

    if isinstance(src_device, AndroidDevice):
        ping_cmd = '{} {}'.format(ping_cmd_linux, dest_address)
        ping_output = src_device.adb.shell(ping_cmd,
                                           timeout=ping_deadline + SHORT_SLEEP,
                                           ignore_status=True)
    elif isinstance(src_device, ssh.connection.SshConnection):
        platform = src_device.run('uname').stdout
        if 'linux' in platform.lower():
            ping_cmd = 'sudo {} {}'.format(ping_cmd_linux, dest_address)
        elif 'darwin' in platform.lower():
            ping_cmd = "sudo {} {}| while IFS= read -r line; do printf '[%s] %s\n' \"$(gdate '+%s.%N')\" \"$line\"; done".format(
                ping_cmd_macos, dest_address)
        ping_output = src_device.run(ping_cmd,
                                     timeout=ping_deadline + SHORT_SLEEP,
                                     ignore_status=True).stdout
    else:
        raise TypeError('Unable to ping using src_device of type %s.' %
                        type(src_device))
    return PingResult(ping_output.splitlines())


@nonblocking
def get_ping_stats_nb(src_device, dest_address, ping_duration, ping_interval,
                      ping_size):
    return get_ping_stats(src_device, dest_address, ping_duration,
                          ping_interval, ping_size)


# Iperf utilities
@nonblocking
def start_iperf_client_nb(iperf_client, iperf_server_address, iperf_args, tag,
                          timeout):
    return iperf_client.start(iperf_server_address, iperf_args, tag, timeout)


def get_iperf_arg_string(duration,
                         reverse_direction,
                         interval=1,
                         traffic_type='TCP',
                         socket_size=None,
                         num_processes=1,
                         udp_throughput='1000M',
                         ipv6=False):
    """Function to format iperf client arguments.

    This function takes in iperf client parameters and returns a properly
    formatter iperf arg string to be used in throughput tests.

    Args:
        duration: iperf duration in seconds
        reverse_direction: boolean controlling the -R flag for iperf clients
        interval: iperf print interval
        traffic_type: string specifying TCP or UDP traffic
        socket_size: string specifying TCP window or socket buffer, e.g., 2M
        num_processes: int specifying number of iperf processes
        udp_throughput: string specifying TX throughput in UDP tests, e.g. 100M
        ipv6: boolean controlling the use of IP V6
    Returns:
        iperf_args: string of formatted iperf args
    """
    iperf_args = '-i {} -t {} -J '.format(interval, duration)
    if ipv6:
        iperf_args = iperf_args + '-6 '
    if traffic_type.upper() == 'UDP':
        iperf_args = iperf_args + '-u -b {} -l 1470 -P {} '.format(
            udp_throughput, num_processes)
    elif traffic_type.upper() == 'TCP':
        iperf_args = iperf_args + '-P {} '.format(num_processes)
    if socket_size:
        iperf_args = iperf_args + '-w {} '.format(socket_size)
    if reverse_direction:
        iperf_args = iperf_args + ' -R'
    return iperf_args


# Attenuator Utilities
def atten_by_label(atten_list, path_label, atten_level):
    """Attenuate signals according to their path label.

    Args:
        atten_list: list of attenuators to iterate over
        path_label: path label on which to set desired attenuation
        atten_level: attenuation desired on path
    """
    for atten in atten_list:
        if path_label in atten.path:
            atten.set_atten(atten_level)


def get_atten_for_target_rssi(target_rssi, attenuators, dut, ping_server):
    """Function to estimate attenuation to hit a target RSSI.

    This function estimates a constant attenuation setting on all atennuation
    ports to hit a target RSSI. The estimate is not meant to be exact or
    guaranteed.

    Args:
        target_rssi: rssi of interest
        attenuators: list of attenuator ports
        dut: android device object assumed connected to a wifi network.
        ping_server: ssh connection object to ping server
    Returns:
        target_atten: attenuation setting to achieve target_rssi
    """
    logging.info('Searching attenuation for RSSI = {}dB'.format(target_rssi))
    # Set attenuator to 0 dB
    for atten in attenuators:
        atten.set_atten(0, strict=False)
    # Start ping traffic
    dut_ip = dut.droid.connectivityGetIPv4Addresses('wlan0')[0]
    # Measure starting RSSI
    ping_future = get_ping_stats_nb(src_device=ping_server,
                                    dest_address=dut_ip,
                                    ping_duration=1.5,
                                    ping_interval=0.02,
                                    ping_size=64)
    current_rssi = get_connected_rssi(dut,
                                      num_measurements=4,
                                      polling_frequency=0.25,
                                      first_measurement_delay=0.5,
                                      disconnect_warning=1,
                                      ignore_samples=1)
    current_rssi = current_rssi['signal_poll_rssi']['mean']
    ping_future.result()
    target_atten = 0
    logging.debug('RSSI @ {0:.2f}dB attenuation = {1:.2f}'.format(
        target_atten, current_rssi))
    within_range = 0
    for idx in range(20):
        atten_delta = max(min(current_rssi - target_rssi, 20), -20)
        target_atten = int((target_atten + atten_delta) * 4) / 4
        if target_atten < 0:
            return 0
        if target_atten > attenuators[0].get_max_atten():
            return attenuators[0].get_max_atten()
        for atten in attenuators:
            atten.set_atten(target_atten, strict=False)
        ping_future = get_ping_stats_nb(src_device=ping_server,
                                        dest_address=dut_ip,
                                        ping_duration=1.5,
                                        ping_interval=0.02,
                                        ping_size=64)
        current_rssi = get_connected_rssi(dut,
                                          num_measurements=4,
                                          polling_frequency=0.25,
                                          first_measurement_delay=0.5,
                                          disconnect_warning=1,
                                          ignore_samples=1)
        current_rssi = current_rssi['signal_poll_rssi']['mean']
        ping_future.result()
        logging.info('RSSI @ {0:.2f}dB attenuation = {1:.2f}'.format(
            target_atten, current_rssi))
        if abs(current_rssi - target_rssi) < 1:
            if within_range:
                logging.info(
                    'Reached RSSI: {0:.2f}. Target RSSI: {1:.2f}.'
                    'Attenuation: {2:.2f}, Iterations = {3:.2f}'.format(
                        current_rssi, target_rssi, target_atten, idx))
                return target_atten
            else:
                within_range = True
        else:
            within_range = False
    return target_atten


def get_current_atten_dut_chain_map(attenuators,
                                    dut,
                                    ping_server,
                                    ping_from_dut=False):
    """Function to detect mapping between attenuator ports and DUT chains.

    This function detects the mapping between attenuator ports and DUT chains
    in cases where DUT chains are connected to only one attenuator port. The
    function assumes the DUT is already connected to a wifi network. The
    function starts by measuring per chain RSSI at 0 attenuation, then
    attenuates one port at a time looking for the chain that reports a lower
    RSSI.

    Args:
        attenuators: list of attenuator ports
        dut: android device object assumed connected to a wifi network.
        ping_server: ssh connection object to ping server
        ping_from_dut: boolean controlling whether to ping from or to dut
    Returns:
        chain_map: list of dut chains, one entry per attenuator port
    """
    # Set attenuator to 0 dB
    for atten in attenuators:
        atten.set_atten(0, strict=False)
    # Start ping traffic
    dut_ip = dut.droid.connectivityGetIPv4Addresses('wlan0')[0]
    if ping_from_dut:
        ping_future = get_ping_stats_nb(dut, ping_server._settings.hostname,
                                        11, 0.02, 64)
    else:
        ping_future = get_ping_stats_nb(ping_server, dut_ip, 11, 0.02, 64)
    # Measure starting RSSI
    base_rssi = get_connected_rssi(dut, 4, 0.25, 1)
    chain0_base_rssi = base_rssi['chain_0_rssi']['mean']
    chain1_base_rssi = base_rssi['chain_1_rssi']['mean']
    if chain0_base_rssi < -70 or chain1_base_rssi < -70:
        logging.warning('RSSI might be too low to get reliable chain map.')
    # Compile chain map by attenuating one path at a time and seeing which
    # chain's RSSI degrades
    chain_map = []
    for test_atten in attenuators:
        # Set one attenuator to 30 dB down
        test_atten.set_atten(30, strict=False)
        # Get new RSSI
        test_rssi = get_connected_rssi(dut, 4, 0.25, 1)
        # Assign attenuator to path that has lower RSSI
        if chain0_base_rssi > -70 and chain0_base_rssi - test_rssi[
                'chain_0_rssi']['mean'] > 10:
            chain_map.append('DUT-Chain-0')
        elif chain1_base_rssi > -70 and chain1_base_rssi - test_rssi[
                'chain_1_rssi']['mean'] > 10:
            chain_map.append('DUT-Chain-1')
        else:
            chain_map.append(None)
        # Reset attenuator to 0
        test_atten.set_atten(0, strict=False)
    ping_future.result()
    logging.debug('Chain Map: {}'.format(chain_map))
    return chain_map


def get_full_rf_connection_map(attenuators,
                               dut,
                               ping_server,
                               networks,
                               ping_from_dut=False):
    """Function to detect per-network connections between attenuator and DUT.

    This function detects the mapping between attenuator ports and DUT chains
    on all networks in its arguments. The function connects the DUT to each
    network then calls get_current_atten_dut_chain_map to get the connection
    map on the current network. The function outputs the results in two formats
    to enable easy access when users are interested in indexing by network or
    attenuator port.

    Args:
        attenuators: list of attenuator ports
        dut: android device object assumed connected to a wifi network.
        ping_server: ssh connection object to ping server
        networks: dict of network IDs and configs
    Returns:
        rf_map_by_network: dict of RF connections indexed by network.
        rf_map_by_atten: list of RF connections indexed by attenuator
    """
    for atten in attenuators:
        atten.set_atten(0, strict=False)

    rf_map_by_network = collections.OrderedDict()
    rf_map_by_atten = [[] for atten in attenuators]
    for net_id, net_config in networks.items():
        wutils.reset_wifi(dut)
        wutils.wifi_connect(dut,
                            net_config,
                            num_of_tries=1,
                            assert_on_fail=False,
                            check_connectivity=False)
        rf_map_by_network[net_id] = get_current_atten_dut_chain_map(
            attenuators, dut, ping_server, ping_from_dut)
        for idx, chain in enumerate(rf_map_by_network[net_id]):
            if chain:
                rf_map_by_atten[idx].append({
                    'network': net_id,
                    'dut_chain': chain
                })
    logging.debug('RF Map (by Network): {}'.format(rf_map_by_network))
    logging.debug('RF Map (by Atten): {}'.format(rf_map_by_atten))

    return rf_map_by_network, rf_map_by_atten


# Generic device utils
def get_dut_temperature(dut):
    """Function to get dut temperature.

    The function fetches and returns the reading from the temperature sensor
    used for skin temperature and thermal throttling.

    Args:
        dut: AndroidDevice of interest
    Returns:
        temperature: device temperature. 0 if temperature could not be read
    """
    candidate_zones = [
        '/sys/devices/virtual/thermal/tz-by-name/skin-therm/temp',
        '/sys/devices/virtual/thermal/tz-by-name/sdm-therm-monitor/temp',
        '/sys/devices/virtual/thermal/tz-by-name/sdm-therm-adc/temp',
        '/sys/devices/virtual/thermal/tz-by-name/back_therm/temp',
        '/dev/thermal/tz-by-name/quiet_therm/temp'
    ]
    for zone in candidate_zones:
        try:
            temperature = int(dut.adb.shell('cat {}'.format(zone)))
            break
        except:
            temperature = 0
    if temperature == 0:
        logging.debug('Could not check DUT temperature.')
    elif temperature > 100:
        temperature = temperature / 1000
    return temperature


def wait_for_dut_cooldown(dut, target_temp=50, timeout=300):
    """Function to wait for a DUT to cool down.

    Args:
        dut: AndroidDevice of interest
        target_temp: target cooldown temperature
        timeout: maxt time to wait for cooldown
    """
    start_time = time.time()
    while time.time() - start_time < timeout:
        temperature = get_dut_temperature(dut)
        if temperature < target_temp:
            break
        time.sleep(SHORT_SLEEP)
    elapsed_time = time.time() - start_time
    logging.debug('DUT Final Temperature: {}C. Cooldown duration: {}'.format(
        temperature, elapsed_time))


def health_check(dut, batt_thresh=5, temp_threshold=53, cooldown=1):
    """Function to check health status of a DUT.

    The function checks both battery levels and temperature to avoid DUT
    powering off during the test.

    Args:
        dut: AndroidDevice of interest
        batt_thresh: battery level threshold
        temp_threshold: temperature threshold
        cooldown: flag to wait for DUT to cool down when overheating
    Returns:
        health_check: boolean confirming device is healthy
    """
    health_check = True
    battery_level = utils.get_battery_level(dut)
    if battery_level < batt_thresh:
        logging.warning('Battery level low ({}%)'.format(battery_level))
        health_check = False
    else:
        logging.debug('Battery level = {}%'.format(battery_level))

    temperature = get_dut_temperature(dut)
    if temperature > temp_threshold:
        if cooldown:
            logging.warning(
                'Waiting for DUT to cooldown. ({} C)'.format(temperature))
            wait_for_dut_cooldown(dut, target_temp=temp_threshold - 5)
        else:
            logging.warning('DUT Overheating ({} C)'.format(temperature))
            health_check = False
    else:
        logging.debug('DUT Temperature = {} C'.format(temperature))
    return health_check


# Wifi Device utils
def detect_wifi_platform(dut):
    ini_check = len(dut.get_file_names('/vendor/firmware/wlan/qca_cld/'))
    if ini_check:
        wifi_platform = 'qcom'
    else:
        wifi_platform = 'brcm'
    return wifi_platform


def detect_wifi_decorator(f):
    def wrap(*args, **kwargs):
        if 'dut' in kwargs:
            dut = kwargs['dut']
        else:
            dut = next(arg for arg in args if type(arg) == AndroidDevice)
        f_decorated = '{}_{}'.format(f.__name__, detect_wifi_platform(dut))
        f_decorated = globals()[f_decorated]
        return (f_decorated(*args, **kwargs))

    return wrap


# Rssi Utilities
def empty_rssi_result():
    return collections.OrderedDict([('data', []), ('mean', None),
                                    ('stdev', None)])


@detect_wifi_decorator
def get_connected_rssi(dut,
                       num_measurements=1,
                       polling_frequency=SHORT_SLEEP,
                       first_measurement_delay=0,
                       disconnect_warning=True,
                       ignore_samples=0,
                       interface=None):
    """Gets all RSSI values reported for the connected access point/BSSID.

    Args:
        dut: android device object from which to get RSSI
        num_measurements: number of scans done, and RSSIs collected
        polling_frequency: time to wait between RSSI measurements
        disconnect_warning: boolean controlling disconnection logging messages
        ignore_samples: number of leading samples to ignore
    Returns:
        connected_rssi: dict containing the measurements results for
        all reported RSSI values (signal_poll, per chain, etc.) and their
        statistics
    """
    pass


@nonblocking
def get_connected_rssi_nb(dut,
                          num_measurements=1,
                          polling_frequency=SHORT_SLEEP,
                          first_measurement_delay=0,
                          disconnect_warning=True,
                          ignore_samples=0,
                          interface=None):
    return get_connected_rssi(dut, num_measurements, polling_frequency,
                              first_measurement_delay, disconnect_warning,
                              ignore_samples, interface)


def get_connected_rssi_qcom(dut,
                            num_measurements=1,
                            polling_frequency=SHORT_SLEEP,
                            first_measurement_delay=0,
                            disconnect_warning=True,
                            ignore_samples=0,
                            interface=None):
    # yapf: disable
    connected_rssi = collections.OrderedDict(
        [('time_stamp', []),
         ('bssid', []), ('ssid', []), ('frequency', []),
         ('signal_poll_rssi', empty_rssi_result()),
         ('signal_poll_avg_rssi', empty_rssi_result()),
         ('chain_0_rssi', empty_rssi_result()),
         ('chain_1_rssi', empty_rssi_result())])
    # yapf: enable
    previous_bssid = 'disconnected'
    t0 = time.time()
    time.sleep(first_measurement_delay)
    for idx in range(num_measurements):
        measurement_start_time = time.time()
        connected_rssi['time_stamp'].append(measurement_start_time - t0)
        # Get signal poll RSSI
        try:
            if interface is None:
                status_output = dut.adb.shell(WPA_CLI_STATUS)
            else:
                status_output = dut.adb.shell(
                    'wpa_cli -i {} status'.format(interface))
        except:
            status_output = ''
        match = re.search('bssid=.*', status_output)
        if match:
            current_bssid = match.group(0).split('=')[1]
            connected_rssi['bssid'].append(current_bssid)
        else:
            current_bssid = 'disconnected'
            connected_rssi['bssid'].append(current_bssid)
            if disconnect_warning and previous_bssid != 'disconnected':
                logging.warning('WIFI DISCONNECT DETECTED!')
        previous_bssid = current_bssid
        match = re.search('\s+ssid=.*', status_output)
        if match:
            ssid = match.group(0).split('=')[1]
            connected_rssi['ssid'].append(ssid)
        else:
            connected_rssi['ssid'].append('disconnected')
        try:
            if interface is None:
                signal_poll_output = dut.adb.shell(SIGNAL_POLL)
            else:
                signal_poll_output = dut.adb.shell(
                    'wpa_cli -i {} signal_poll'.format(interface))
        except:
            signal_poll_output = ''
        match = re.search('FREQUENCY=.*', signal_poll_output)
        if match:
            frequency = int(match.group(0).split('=')[1])
            connected_rssi['frequency'].append(frequency)
        else:
            connected_rssi['frequency'].append(RSSI_ERROR_VAL)
        match = re.search('RSSI=.*', signal_poll_output)
        if match:
            temp_rssi = int(match.group(0).split('=')[1])
            if temp_rssi == -9999 or temp_rssi == 0:
                connected_rssi['signal_poll_rssi']['data'].append(
                    RSSI_ERROR_VAL)
            else:
                connected_rssi['signal_poll_rssi']['data'].append(temp_rssi)
        else:
            connected_rssi['signal_poll_rssi']['data'].append(RSSI_ERROR_VAL)
        match = re.search('AVG_RSSI=.*', signal_poll_output)
        if match:
            connected_rssi['signal_poll_avg_rssi']['data'].append(
                int(match.group(0).split('=')[1]))
        else:
            connected_rssi['signal_poll_avg_rssi']['data'].append(
                RSSI_ERROR_VAL)

        # Get per chain RSSI
        try:
            if interface is None:
                per_chain_rssi = dut.adb.shell(STATION_DUMP)
            else:
                per_chain_rssi = ''
        except:
            per_chain_rssi = ''
        match = re.search('.*signal avg:.*', per_chain_rssi)
        if match:
            per_chain_rssi = per_chain_rssi[per_chain_rssi.find('[') +
                                            1:per_chain_rssi.find(']')]
            per_chain_rssi = per_chain_rssi.split(', ')
            connected_rssi['chain_0_rssi']['data'].append(
                int(per_chain_rssi[0]))
            connected_rssi['chain_1_rssi']['data'].append(
                int(per_chain_rssi[1]))
        else:
            connected_rssi['chain_0_rssi']['data'].append(RSSI_ERROR_VAL)
            connected_rssi['chain_1_rssi']['data'].append(RSSI_ERROR_VAL)
        measurement_elapsed_time = time.time() - measurement_start_time
        time.sleep(max(0, polling_frequency - measurement_elapsed_time))

    # Compute mean RSSIs. Only average valid readings.
    # Output RSSI_ERROR_VAL if no valid connected readings found.
    for key, val in connected_rssi.copy().items():
        if 'data' not in val:
            continue
        filtered_rssi_values = [x for x in val['data'] if not math.isnan(x)]
        if len(filtered_rssi_values) > ignore_samples:
            filtered_rssi_values = filtered_rssi_values[ignore_samples:]
        if filtered_rssi_values:
            connected_rssi[key]['mean'] = statistics.mean(filtered_rssi_values)
            if len(filtered_rssi_values) > 1:
                connected_rssi[key]['stdev'] = statistics.stdev(
                    filtered_rssi_values)
            else:
                connected_rssi[key]['stdev'] = 0
        else:
            connected_rssi[key]['mean'] = RSSI_ERROR_VAL
            connected_rssi[key]['stdev'] = RSSI_ERROR_VAL
    return connected_rssi


def get_connected_rssi_brcm(dut,
                            num_measurements=1,
                            polling_frequency=SHORT_SLEEP,
                            first_measurement_delay=0,
                            disconnect_warning=True,
                            ignore_samples=0,
                            interface=None):
    # yapf: disable
    connected_rssi = collections.OrderedDict(
        [('time_stamp', []),
         ('bssid', []), ('ssid', []), ('frequency', []),
         ('signal_poll_rssi', empty_rssi_result()),
         ('signal_poll_avg_rssi', empty_rssi_result()),
         ('chain_0_rssi', empty_rssi_result()),
         ('chain_1_rssi', empty_rssi_result())])

    # yapf: enable
    previous_bssid = 'disconnected'
    t0 = time.time()
    time.sleep(first_measurement_delay)
    for idx in range(num_measurements):
        measurement_start_time = time.time()
        connected_rssi['time_stamp'].append(measurement_start_time - t0)
        # Get signal poll RSSI
        status_output = dut.adb.shell('wl assoc')
        match = re.search('BSSID:.*', status_output)

        if match:
            current_bssid = match.group(0).split('\t')[0]
            current_bssid = current_bssid.split(' ')[1]
            connected_rssi['bssid'].append(current_bssid)

        else:
            current_bssid = 'disconnected'
            connected_rssi['bssid'].append(current_bssid)
            if disconnect_warning and previous_bssid != 'disconnected':
                logging.warning('WIFI DISCONNECT DETECTED!')

        previous_bssid = current_bssid
        match = re.search('SSID:.*', status_output)
        if match:
            ssid = match.group(0).split(': ')[1]
            connected_rssi['ssid'].append(ssid)
        else:
            connected_rssi['ssid'].append('disconnected')

        #TODO: SEARCH MAP ; PICK CENTER CHANNEL
        match = re.search('Primary channel:.*', status_output)
        if match:
            frequency = int(match.group(0).split(':')[1])
            connected_rssi['frequency'].append(frequency)
        else:
            connected_rssi['frequency'].append(RSSI_ERROR_VAL)

        try:
            per_chain_rssi = dut.adb.shell('wl phy_rssi_ant')
        except:
            per_chain_rssi = DISCONNECTION_MESSAGE_BRCM
        if DISCONNECTION_MESSAGE_BRCM not in per_chain_rssi:
            per_chain_rssi = per_chain_rssi.split(' ')
            chain_0_rssi = int(per_chain_rssi[1])
            chain_1_rssi = int(per_chain_rssi[4])
            connected_rssi['chain_0_rssi']['data'].append(chain_0_rssi)
            connected_rssi['chain_1_rssi']['data'].append(chain_1_rssi)
            combined_rssi = math.pow(10, chain_0_rssi / 10) + math.pow(
                10, chain_1_rssi / 10)
            combined_rssi = 10 * math.log10(combined_rssi)
            connected_rssi['signal_poll_rssi']['data'].append(combined_rssi)
            connected_rssi['signal_poll_avg_rssi']['data'].append(
                combined_rssi)
        else:
            connected_rssi['chain_0_rssi']['data'].append(RSSI_ERROR_VAL)
            connected_rssi['chain_1_rssi']['data'].append(RSSI_ERROR_VAL)
            connected_rssi['signal_poll_rssi']['data'].append(RSSI_ERROR_VAL)
            connected_rssi['signal_poll_avg_rssi']['data'].append(
                RSSI_ERROR_VAL)
        measurement_elapsed_time = time.time() - measurement_start_time
        time.sleep(max(0, polling_frequency - measurement_elapsed_time))

    # Statistics, Statistics
    for key, val in connected_rssi.copy().items():
        if 'data' not in val:
            continue
        filtered_rssi_values = [x for x in val['data'] if not math.isnan(x)]
        if len(filtered_rssi_values) > ignore_samples:
            filtered_rssi_values = filtered_rssi_values[ignore_samples:]
        if filtered_rssi_values:
            connected_rssi[key]['mean'] = statistics.mean(filtered_rssi_values)
            if len(filtered_rssi_values) > 1:
                connected_rssi[key]['stdev'] = statistics.stdev(
                    filtered_rssi_values)
            else:
                connected_rssi[key]['stdev'] = 0
        else:
            connected_rssi[key]['mean'] = RSSI_ERROR_VAL
            connected_rssi[key]['stdev'] = RSSI_ERROR_VAL

    return connected_rssi


@detect_wifi_decorator
def get_scan_rssi(dut, tracked_bssids, num_measurements=1):
    """Gets scan RSSI for specified BSSIDs.

    Args:
        dut: android device object from which to get RSSI
        tracked_bssids: array of BSSIDs to gather RSSI data for
        num_measurements: number of scans done, and RSSIs collected
    Returns:
        scan_rssi: dict containing the measurement results as well as the
        statistics of the scan RSSI for all BSSIDs in tracked_bssids
    """
    pass


@nonblocking
def get_scan_rssi_nb(dut, tracked_bssids, num_measurements=1):
    return get_scan_rssi(dut, tracked_bssids, num_measurements)


def get_scan_rssi_qcom(dut, tracked_bssids, num_measurements=1):
    scan_rssi = collections.OrderedDict()
    for bssid in tracked_bssids:
        scan_rssi[bssid] = empty_rssi_result()
    for idx in range(num_measurements):
        scan_output = dut.adb.shell(SCAN)
        time.sleep(MED_SLEEP)
        scan_output = dut.adb.shell(SCAN_RESULTS)
        for bssid in tracked_bssids:
            bssid_result = re.search(bssid + '.*',
                                     scan_output,
                                     flags=re.IGNORECASE)
            if bssid_result:
                bssid_result = bssid_result.group(0).split('\t')
                scan_rssi[bssid]['data'].append(int(bssid_result[2]))
            else:
                scan_rssi[bssid]['data'].append(RSSI_ERROR_VAL)
    # Compute mean RSSIs. Only average valid readings.
    # Output RSSI_ERROR_VAL if no readings found.
    for key, val in scan_rssi.items():
        filtered_rssi_values = [x for x in val['data'] if not math.isnan(x)]
        if filtered_rssi_values:
            scan_rssi[key]['mean'] = statistics.mean(filtered_rssi_values)
            if len(filtered_rssi_values) > 1:
                scan_rssi[key]['stdev'] = statistics.stdev(
                    filtered_rssi_values)
            else:
                scan_rssi[key]['stdev'] = 0
        else:
            scan_rssi[key]['mean'] = RSSI_ERROR_VAL
            scan_rssi[key]['stdev'] = RSSI_ERROR_VAL
    return scan_rssi


def get_scan_rssi_brcm(dut, tracked_bssids, num_measurements=1):
    scan_rssi = collections.OrderedDict()
    for bssid in tracked_bssids:
        scan_rssi[bssid] = empty_rssi_result()
    for idx in range(num_measurements):
        scan_output = dut.adb.shell('cmd wifi start-scan')
        time.sleep(MED_SLEEP)
        scan_output = dut.adb.shell('cmd wifi list-scan-results')
        for bssid in tracked_bssids:
            bssid_result = re.search(bssid + '.*',
                                     scan_output,
                                     flags=re.IGNORECASE)
            if bssid_result:
                bssid_result = bssid_result.group(0).split()
                print(bssid_result)
                scan_rssi[bssid]['data'].append(int(bssid_result[2]))
            else:
                scan_rssi[bssid]['data'].append(RSSI_ERROR_VAL)
    # Compute mean RSSIs. Only average valid readings.
    # Output RSSI_ERROR_VAL if no readings found.
    for key, val in scan_rssi.items():
        filtered_rssi_values = [x for x in val['data'] if not math.isnan(x)]
        if filtered_rssi_values:
            scan_rssi[key]['mean'] = statistics.mean(filtered_rssi_values)
            if len(filtered_rssi_values) > 1:
                scan_rssi[key]['stdev'] = statistics.stdev(
                    filtered_rssi_values)
            else:
                scan_rssi[key]['stdev'] = 0
        else:
            scan_rssi[key]['mean'] = RSSI_ERROR_VAL
            scan_rssi[key]['stdev'] = RSSI_ERROR_VAL
    return scan_rssi


@detect_wifi_decorator
def get_sw_signature(dut):
    """Function that checks the signature for wifi firmware and config files.

    Returns:
        bdf_signature: signature consisting of last three digits of bdf cksums
        fw_signature: floating point firmware version, i.e., major.minor
    """
    pass


def get_sw_signature_qcom(dut):
    bdf_output = dut.adb.shell('cksum /vendor/firmware/bdwlan*')
    logging.debug('BDF Checksum output: {}'.format(bdf_output))
    bdf_signature = sum(
        [int(line.split(' ')[0]) for line in bdf_output.splitlines()]) % 1000

    fw_output = dut.adb.shell('halutil -logger -get fw')
    logging.debug('Firmware version output: {}'.format(fw_output))
    fw_version = re.search(FW_REGEX, fw_output).group('firmware')
    fw_signature = fw_version.split('.')[-3:-1]
    fw_signature = float('.'.join(fw_signature))
    serial_hash = int(hashlib.md5(dut.serial.encode()).hexdigest(), 16) % 1000
    return {
        'config_signature': bdf_signature,
        'fw_signature': fw_signature,
        'serial_hash': serial_hash
    }


def get_sw_signature_brcm(dut):
    bdf_output = dut.adb.shell('cksum /vendor/etc/wifi/bcmdhd*')
    logging.debug('BDF Checksum output: {}'.format(bdf_output))
    bdf_signature = sum(
        [int(line.split(' ')[0]) for line in bdf_output.splitlines()]) % 1000

    fw_output = dut.adb.shell('getprop vendor.wlan.firmware.version')
    logging.debug('Firmware version output: {}'.format(fw_output))
    fw_version = fw_output.split('.')[-1]
    driver_output = dut.adb.shell('getprop vendor.wlan.driver.version')
    driver_version = driver_output.split('.')[-1]
    fw_signature = float('{}.{}'.format(fw_version, driver_version))
    serial_hash = int(hashlib.md5(dut.serial.encode()).hexdigest(), 16) % 1000
    return {
        'config_signature': bdf_signature,
        'fw_signature': fw_signature,
        'serial_hash': serial_hash
    }


@detect_wifi_decorator
def push_config(dut, config_file):
    """Function to push Wifi BDF files

    This function checks for existing wifi bdf files and over writes them all,
    for simplicity, with the bdf file provided in the arguments. The dut is
    rebooted for the bdf file to take effect

    Args:
        dut: dut to push bdf file to
        config_file: path to bdf_file to push
    """
    pass


def push_config_qcom(dut, config_file):
    config_files_list = dut.adb.shell(
        'ls /vendor/firmware/bdwlan*').splitlines()
    for dst_file in config_files_list:
        dut.push_system_file(config_file, dst_file)
    dut.reboot()


def push_config_brcm(dut, config_file):
    config_files_list = dut.adb.shell('ls /vendor/etc/*.cal').splitlines()
    for dst_file in config_files_list:
        dut.push_system_file(config_file, dst_file)
    dut.reboot()


def push_firmware(dut, firmware_files):
    """Function to push Wifi firmware files

    Args:
        dut: dut to push bdf file to
        firmware_files: path to wlanmdsp.mbn file
        datamsc_file: path to Data.msc file
    """
    for file in firmware_files:
        dut.push_system_file(file, '/vendor/firmware/')
    dut.reboot()


@detect_wifi_decorator
def start_wifi_logging(dut):
    """Function to start collecting wifi-related logs"""
    pass


def start_wifi_logging_qcom(dut):
    dut.droid.wifiEnableVerboseLogging(1)
    msg = "Failed to enable WiFi verbose logging."
    asserts.assert_equal(dut.droid.wifiGetVerboseLoggingLevel(), 1, msg)
    logging.info('Starting CNSS logs')
    dut.adb.shell("find /data/vendor/wifi/wlan_logs/ -type f -delete",
                  ignore_status=True)
    dut.adb.shell_nb('cnss_diag -f -s')


def start_wifi_logging_brcm(dut):
    pass


@detect_wifi_decorator
def stop_wifi_logging(dut):
    """Function to start collecting wifi-related logs"""
    pass


def stop_wifi_logging_qcom(dut):
    logging.info('Stopping CNSS logs')
    dut.adb.shell('killall cnss_diag')
    logs = dut.get_file_names("/data/vendor/wifi/wlan_logs/")
    if logs:
        dut.log.info("Pulling cnss_diag logs %s", logs)
        log_path = os.path.join(dut.device_log_path,
                                "CNSS_DIAG_%s" % dut.serial)
        os.makedirs(log_path, exist_ok=True)
        dut.pull_files(logs, log_path)


def stop_wifi_logging_brcm(dut):
    pass


def _set_ini_fields(ini_file_path, ini_field_dict):
    template_regex = r'^{}=[0-9,.x-]+'
    with open(ini_file_path, 'r') as f:
        ini_lines = f.read().splitlines()
        for idx, line in enumerate(ini_lines):
            for field_name, field_value in ini_field_dict.items():
                line_regex = re.compile(template_regex.format(field_name))
                if re.match(line_regex, line):
                    ini_lines[idx] = '{}={}'.format(field_name, field_value)
                    print(ini_lines[idx])
    with open(ini_file_path, 'w') as f:
        f.write('\n'.join(ini_lines) + '\n')


def _edit_dut_ini(dut, ini_fields):
    """Function to edit Wifi ini files."""
    dut_ini_path = '/vendor/firmware/wlan/qca_cld/WCNSS_qcom_cfg.ini'
    local_ini_path = os.path.expanduser('~/WCNSS_qcom_cfg.ini')
    dut.pull_files(dut_ini_path, local_ini_path)

    _set_ini_fields(local_ini_path, ini_fields)

    dut.push_system_file(local_ini_path, dut_ini_path)
    dut.reboot()


def set_ini_single_chain_mode(dut, chain):
    ini_fields = {
        'gEnable2x2': 0,
        'gSetTxChainmask1x1': chain + 1,
        'gSetRxChainmask1x1': chain + 1,
        'gDualMacFeatureDisable': 1,
        'gDot11Mode': 0
    }
    _edit_dut_ini(dut, ini_fields)


def set_ini_two_chain_mode(dut):
    ini_fields = {
        'gEnable2x2': 2,
        'gSetTxChainmask1x1': 1,
        'gSetRxChainmask1x1': 1,
        'gDualMacFeatureDisable': 6,
        'gDot11Mode': 0
    }
    _edit_dut_ini(dut, ini_fields)


def set_ini_tx_mode(dut, mode):
    TX_MODE_DICT = {
        'Auto': 0,
        '11n': 4,
        '11ac': 9,
        '11abg': 1,
        '11b': 2,
        '11': 3,
        '11g only': 5,
        '11n only': 6,
        '11b only': 7,
        '11ac only': 8
    }

    ini_fields = {
        'gEnable2x2': 2,
        'gSetTxChainmask1x1': 1,
        'gSetRxChainmask1x1': 1,
        'gDualMacFeatureDisable': 6,
        'gDot11Mode': TX_MODE_DICT[mode]
    }
    _edit_dut_ini(dut, ini_fields)


# Link layer stats utilities
class LinkLayerStats():
    def __new__(self, dut, llstats_enabled=True):
        if detect_wifi_platform(dut) == 'qcom':
            return LinkLayerStatsQcom(dut, llstats_enabled=True)
        else:
            return LinkLayerStatsBrcm(dut, llstats_enabled=True)


class LinkLayerStatsQcom():

    LLSTATS_CMD = 'cat /d/wlan0/ll_stats'
    PEER_REGEX = 'LL_STATS_PEER_ALL'
    MCS_REGEX = re.compile(
        r'preamble: (?P<mode>\S+), nss: (?P<num_streams>\S+), bw: (?P<bw>\S+), '
        'mcs: (?P<mcs>\S+), bitrate: (?P<rate>\S+), txmpdu: (?P<txmpdu>\S+), '
        'rxmpdu: (?P<rxmpdu>\S+), mpdu_lost: (?P<mpdu_lost>\S+), '
        'retries: (?P<retries>\S+), retries_short: (?P<retries_short>\S+), '
        'retries_long: (?P<retries_long>\S+)')
    MCS_ID = collections.namedtuple(
        'mcs_id', ['mode', 'num_streams', 'bandwidth', 'mcs', 'rate'])
    MODE_MAP = {'0': '11a/g', '1': '11b', '2': '11n', '3': '11ac'}
    BW_MAP = {'0': 20, '1': 40, '2': 80}

    def __init__(self, dut, llstats_enabled=True):
        self.dut = dut
        self.llstats_enabled = llstats_enabled
        self.llstats_cumulative = self._empty_llstats()
        self.llstats_incremental = self._empty_llstats()

    def update_stats(self):
        if self.llstats_enabled:
            try:
                llstats_output = self.dut.adb.shell(self.LLSTATS_CMD,
                                                    timeout=0.1)
            except:
                llstats_output = ''
        else:
            llstats_output = ''
        self._update_stats(llstats_output)

    def reset_stats(self):
        self.llstats_cumulative = self._empty_llstats()
        self.llstats_incremental = self._empty_llstats()

    def _empty_llstats(self):
        return collections.OrderedDict(mcs_stats=collections.OrderedDict(),
                                       summary=collections.OrderedDict())

    def _empty_mcs_stat(self):
        return collections.OrderedDict(txmpdu=0,
                                       rxmpdu=0,
                                       mpdu_lost=0,
                                       retries=0,
                                       retries_short=0,
                                       retries_long=0)

    def _mcs_id_to_string(self, mcs_id):
        mcs_string = '{} {}MHz Nss{} MCS{} {}Mbps'.format(
            mcs_id.mode, mcs_id.bandwidth, mcs_id.num_streams, mcs_id.mcs,
            mcs_id.rate)
        return mcs_string

    def _parse_mcs_stats(self, llstats_output):
        llstats_dict = {}
        # Look for per-peer stats
        match = re.search(self.PEER_REGEX, llstats_output)
        if not match:
            self.reset_stats()
            return collections.OrderedDict()
        # Find and process all matches for per stream stats
        match_iter = re.finditer(self.MCS_REGEX, llstats_output)
        for match in match_iter:
            current_mcs = self.MCS_ID(self.MODE_MAP[match.group('mode')],
                                      int(match.group('num_streams')) + 1,
                                      self.BW_MAP[match.group('bw')],
                                      int(match.group('mcs')),
                                      int(match.group('rate'), 16) / 1000)
            current_stats = collections.OrderedDict(
                txmpdu=int(match.group('txmpdu')),
                rxmpdu=int(match.group('rxmpdu')),
                mpdu_lost=int(match.group('mpdu_lost')),
                retries=int(match.group('retries')),
                retries_short=int(match.group('retries_short')),
                retries_long=int(match.group('retries_long')))
            llstats_dict[self._mcs_id_to_string(current_mcs)] = current_stats
        return llstats_dict

    def _diff_mcs_stats(self, new_stats, old_stats):
        stats_diff = collections.OrderedDict()
        for stat_key in new_stats.keys():
            stats_diff[stat_key] = new_stats[stat_key] - old_stats[stat_key]
        return stats_diff

    def _generate_stats_summary(self, llstats_dict):
        llstats_summary = collections.OrderedDict(common_tx_mcs=None,
                                                  common_tx_mcs_count=0,
                                                  common_tx_mcs_freq=0,
                                                  common_rx_mcs=None,
                                                  common_rx_mcs_count=0,
                                                  common_rx_mcs_freq=0)
        txmpdu_count = 0
        rxmpdu_count = 0
        for mcs_id, mcs_stats in llstats_dict['mcs_stats'].items():
            if mcs_stats['txmpdu'] > llstats_summary['common_tx_mcs_count']:
                llstats_summary['common_tx_mcs'] = mcs_id
                llstats_summary['common_tx_mcs_count'] = mcs_stats['txmpdu']
            if mcs_stats['rxmpdu'] > llstats_summary['common_rx_mcs_count']:
                llstats_summary['common_rx_mcs'] = mcs_id
                llstats_summary['common_rx_mcs_count'] = mcs_stats['rxmpdu']
            txmpdu_count += mcs_stats['txmpdu']
            rxmpdu_count += mcs_stats['rxmpdu']
        if txmpdu_count:
            llstats_summary['common_tx_mcs_freq'] = (
                llstats_summary['common_tx_mcs_count'] / txmpdu_count)
        if rxmpdu_count:
            llstats_summary['common_rx_mcs_freq'] = (
                llstats_summary['common_rx_mcs_count'] / rxmpdu_count)
        return llstats_summary

    def _update_stats(self, llstats_output):
        # Parse stats
        new_llstats = self._empty_llstats()
        new_llstats['mcs_stats'] = self._parse_mcs_stats(llstats_output)
        # Save old stats and set new cumulative stats
        old_llstats = self.llstats_cumulative.copy()
        self.llstats_cumulative = new_llstats.copy()
        # Compute difference between new and old stats
        self.llstats_incremental = self._empty_llstats()
        for mcs_id, new_mcs_stats in new_llstats['mcs_stats'].items():
            old_mcs_stats = old_llstats['mcs_stats'].get(
                mcs_id, self._empty_mcs_stat())
            self.llstats_incremental['mcs_stats'][
                mcs_id] = self._diff_mcs_stats(new_mcs_stats, old_mcs_stats)
        # Generate llstats summary
        self.llstats_incremental['summary'] = self._generate_stats_summary(
            self.llstats_incremental)
        self.llstats_cumulative['summary'] = self._generate_stats_summary(
            self.llstats_cumulative)


class LinkLayerStatsBrcm():
    def __init__(self, dut, llstats_enabled=True):
        self.dut = dut
        self.llstats_enabled = llstats_enabled
        self.llstats_incremental = self._empty_llstats()
        self.llstats_cumulative = self.llstats_incremental

    def _empty_llstats(self):
        return collections.OrderedDict(mcs_stats=collections.OrderedDict(),
                                       summary=collections.OrderedDict())

    def update_stats(self):
        self.llstats_incremental = self._empty_llstats()
        self.llstats_incremental['summary'] = collections.OrderedDict(
            common_tx_mcs=None,
            common_tx_mcs_count=1,
            common_tx_mcs_freq=1,
            common_rx_mcs=None,
            common_rx_mcs_count=1,
            common_rx_mcs_freq=1)
        if self.llstats_enabled:
            try:
                rate_info = self.dut.adb.shell('wl rate_info', timeout=0.1)
                self.llstats_incremental['summary'][
                    'common_tx_mcs'] = '{} Mbps'.format(
                        re.findall('\[Tx\]:'
                                   ' (\d+[.]*\d* Mbps)', rate_info))
                self.llstats_incremental['summary'][
                    'common_rx_mcs'] = '{} Mbps'.format(
                        re.findall('\[Rx\]:'
                                   ' (\d+[.]*\d* Mbps)', rate_info))
            except:
                pass
