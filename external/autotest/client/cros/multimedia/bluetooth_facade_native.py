# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Facade to access the bluetooth-related functionality."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import base64
import collections
from datetime import datetime
import dbus
import dbus.mainloop.glib
import dbus.service
import glob
import gobject
import json
import logging
import logging.handlers
import os
import re
import subprocess
import functools
import time

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros.bluetooth import bluetooth_socket
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import dbus_util
from autotest_lib.client.cros.udev_helpers import UdevadmInfo, UdevadmTrigger
from autotest_lib.client.cros import xmlrpc_server
from autotest_lib.client.cros.audio import (audio_test_data as
                                            audio_test_data_module)
from autotest_lib.client.cros.audio import check_quality
from autotest_lib.client.cros.audio import cras_utils
from autotest_lib.client.cros.audio.sox_utils import (
        convert_format, convert_raw_file, get_file_length,
        trim_silence_from_wav_file)
from autotest_lib.client.cros.bluetooth import advertisement
from autotest_lib.client.cros.bluetooth import adv_monitor_helper
from autotest_lib.client.cros.bluetooth import output_recorder
from autotest_lib.client.cros.power import sys_power
import six
from six.moves import map
from six.moves import range

CheckQualityArgsClass = collections.namedtuple(
        'args_type', ['filename', 'rate', 'channel', 'bit_width'])


def _dbus_byte_array_to_b64_string(dbus_byte_array):
    """Base64 encodes a dbus byte array for use with the xml rpc proxy."""
    return base64.standard_b64encode(bytearray(dbus_byte_array))


def _b64_string_to_dbus_byte_array(b64_string):
    """Base64 decodes a dbus byte array for use with the xml rpc proxy."""
    dbus_array = dbus.Array([], signature=dbus.Signature('y'))
    bytes = bytearray(base64.standard_b64decode(b64_string))
    for byte in bytes:
        dbus_array.append(dbus.Byte(byte))
    return dbus_array


def dbus_print_error(default_return_value=False):
    """Catch all DBus exceptions and return the error.

    Wrap a function with a try block that catches DBus exceptions and
    returns the error with the specified return status. The exception is logged
    to aid in debugging.

    @param wrapped_function function to wrap.

    """

    def decorator(wrapped_function):
        """Call a function and catch DBus errors.

        @param wrapped_function function to call in dbus safe context.
        @return function return value or default_return_value on failure.

        """

        @functools.wraps(wrapped_function)
        def wrapper(*args, **kwargs):
            """Pass args and kwargs to a dbus safe function.

            @param args formal python arguments.
            @param kwargs keyword python arguments.
            @return function return value or default_return_value on failure.

            """
            logging.debug('%s()', wrapped_function.__name__)
            try:
                return wrapped_function(*args, **kwargs)

            except dbus.exceptions.DBusException as e:
                logging.debug(
                        'Exception while performing operation %s: %s: %s',
                        wrapped_function.__name__, e.get_dbus_name(),
                        e.get_dbus_message())
                return (default_return_value, str(e))

        return wrapper

    return decorator


class LogRecorder(object):
    """The LogRecorder class helps to collect logs without a listening thread"""

    class LoggingException(Exception):
        """A dummy exception class for LogRecorder class."""
        pass

    def __init__(self, log_path):
        """Initialize log recorder object

        @param log_path: string path to log file to record

        @raises: LogRecorder.LoggingException on non-existent log file
        """
        if not os.path.isfile(log_path):
            msg = 'Requested log file {} does not exist'.format(log_path)
            raise LogRecorder.LoggingException(msg)

        self.log_path = log_path

        self.initial_log_size = -1
        self.log_contents = []

    def StartRecording(self):
        """Mark initial log size for later comparison"""

        self.initial_log_size = os.path.getsize(self.log_path)
        self.log_contents = []

    def StopRecording(self):
        """Gather the logs since StartRecording was called

        @raises: LogRecorder.LoggingException if:
                - Log file disappeared since StartRecording was called
                - Log file is smaller than when logging began
                - StartRecording was never called
        """
        now_size = os.path.getsize(self.log_path)

        if not os.path.isfile(self.log_path):
            msg = 'File {} disappeared unexpectedly'.format(self.log_path)
            raise LogRecorder.LoggingException(msg)

        if now_size < self.initial_log_size:
            msg = 'Log became smaller unexpectedly'
            raise LogRecorder.LoggingException(msg)

        if self.initial_log_size < 0:
            msg = 'Recording stopped before it started'
            raise LogRecorder.LoggingException(msg)

        with open(self.log_path, 'r') as mf:
            # Skip to the point where we started recording
            mf.seek(self.initial_log_size)

            readsize = now_size - self.initial_log_size
            self.log_contents = mf.read(readsize).split('\n')

    def LogContains(self, search_str):
        """Performs simple string checking on each line from the collected log

        @param search_str: string to be located within log contents. This arg
                is expected to not span between lines in the logs

        @returns: True if search_str was located in the collected log contents,
                False otherwise
        """

        for line in self.log_contents:
            if search_str in line:
                return True

        return False


class InterleaveLogger(LogRecorder):
    """LogRecorder class that focus on interleave scan"""

    SYSLOG_PATH = '/var/log/messages'

    # Example bluetooth kernel log:
    # "2020-11-23T07:52:31.395941Z DEBUG kernel: [ 6469.811135] Bluetooth: "
    # "cancel_interleave_scan() hci0: hci0 cancelling interleave scan"
    KERNEL_LOG_PATTERN = ('([^ ]+) DEBUG kernel: \[.*\] Bluetooth: '
                          '{FUNCTION}\(\) hci0: {LOG_STR}')
    STATE_PATTERN = KERNEL_LOG_PATTERN.format(
            FUNCTION='add_le_interleave_adv_monitor_scan',
            LOG_STR='next state: (.+)')
    CANCEL_PATTERN = KERNEL_LOG_PATTERN.format(
            FUNCTION='cancel_interleave_scan',
            LOG_STR='hci0 cancelling interleave scan')
    SYSTIME_LENGTH = len('2020-12-18T00:11:22.345678')

    def __init__(self):
        """ Initialize object
        """
        self.reset()
        self.state_pattern = re.compile(self.STATE_PATTERN)
        self.cancel_pattern = re.compile(self.CANCEL_PATTERN)
        super(InterleaveLogger, self).__init__(self.SYSLOG_PATH)

    def reset(self):
        """ Clear data between each log collection attempt
        """
        self.records = []
        self.cancel_events = []

    def StartRecording(self):
        """ Reset the previous data and start recording.
        """
        self.reset()
        super(InterleaveLogger, self).StartRecording()

    def StopRecording(self):
        """ Stop recording and parse logs
            The following data will be set after this call

            - self.records: a dictionary where each item is a record of
                            interleave |state| and the |time| the state starts.
                            |state| could be {'no filter', 'allowlist'}
                            |time| is system time in sec

            - self.cancel_events: a list of |time| when a interleave cancel
                                  event log was found
                                  |time| is system time in sec

            @returns: True if StopRecording success, False otherwise

        """
        try:
            super(InterleaveLogger, self).StopRecording()
        except Exception as e:
            logging.error(e)
            return False

        success = True

        def sys_time_to_timestamp(time_str):
            """ Return timestamp of time_str """

            # This is to remove the suffix of time string, in some cases the
            # time string ends with an extra 'Z', in other cases, the string
            # ends with time zone (ex. '+08:00')
            time_str = time_str[:self.SYSTIME_LENGTH]

            try:
                dt = datetime.strptime(time_str, "%Y-%m-%dT%H:%M:%S.%f")
            except Exception as e:
                logging.error(e)
                success = False
                return 0

            return time.mktime(dt.timetuple()) + dt.microsecond * (10**-6)

        for line in self.log_contents:
            line = line.strip().replace('\\r\\n', '')
            state_pattern = self.state_pattern.search(line)
            cancel_pattern = self.cancel_pattern.search(line)

            if cancel_pattern:
                time_str = cancel_pattern.groups()[0]
                time_sec = sys_time_to_timestamp(time_str)
                self.cancel_events.append(time_sec)

            if state_pattern:
                time_str, state = state_pattern.groups()
                time_sec = sys_time_to_timestamp(time_str)
                self.records.append({'time': time_sec, 'state': state})

        return success


class PairingAgent(dbus.service.Object):
    """The agent handling the authentication process of bluetooth pairing.

    PairingAgent overrides RequestPinCode method to return a given pin code.
    User can use this agent to pair bluetooth device which has a known
    pin code.

    TODO (josephsih): more pairing modes other than pin code would be
    supported later.

    """

    def __init__(self, pin, *args, **kwargs):
        super(PairingAgent, self).__init__(*args, **kwargs)
        self._pin = pin

    @dbus.service.method('org.bluez.Agent1',
                         in_signature='o',
                         out_signature='s')
    def RequestPinCode(self, device_path):
        """Requests pin code for a device.

        Returns the known pin code for the request.

        @param device_path: The object path of the device.

        @returns: The known pin code.

        """
        logging.info('RequestPinCode for %s; return %s', device_path,
                     self._pin)
        return self._pin


class BluetoothFacadeNative(object):
    """Exposes DUT methods called remotely during Bluetooth autotests.

    All instance methods of this object without a preceding '_' are exposed via
    an XML-RPC server. This is not a stateless handler object, which means that
    if you store state inside the delegate, that state will remain around for
    future calls.
    """

    UPSTART_PATH = 'unix:abstract=/com/ubuntu/upstart'
    UPSTART_MANAGER_PATH = '/com/ubuntu/Upstart'
    UPSTART_MANAGER_IFACE = 'com.ubuntu.Upstart0_6'
    UPSTART_JOB_IFACE = 'com.ubuntu.Upstart0_6.Job'

    UPSTART_ERROR_UNKNOWNINSTANCE = \
            'com.ubuntu.Upstart0_6.Error.UnknownInstance'
    UPSTART_ERROR_ALREADYSTARTED = \
            'com.ubuntu.Upstart0_6.Error.AlreadyStarted'

    BLUETOOTHD_JOB = 'bluetoothd'

    DBUS_ERROR_SERVICEUNKNOWN = 'org.freedesktop.DBus.Error.ServiceUnknown'

    BLUETOOTH_SERVICE_NAME = 'org.chromium.Bluetooth'
    BLUEZ_SERVICE_NAME = 'org.bluez'
    BLUEZ_MANAGER_PATH = '/'
    BLUEZ_DEBUG_LOG_PATH = '/org/chromium/Bluetooth'
    BLUEZ_DEBUG_LOG_IFACE = 'org.chromium.Bluetooth.Debug'
    BLUEZ_MANAGER_IFACE = 'org.freedesktop.DBus.ObjectManager'
    BLUEZ_ADAPTER_IFACE = 'org.bluez.Adapter1'
    BLUEZ_BATTERY_IFACE = 'org.bluez.Battery1'
    BLUEZ_DEVICE_IFACE = 'org.bluez.Device1'
    BLUEZ_GATT_SERV_IFACE = 'org.bluez.GattService1'
    BLUEZ_GATT_CHAR_IFACE = 'org.bluez.GattCharacteristic1'
    BLUEZ_GATT_DESC_IFACE = 'org.bluez.GattDescriptor1'
    BLUEZ_LE_ADVERTISING_MANAGER_IFACE = 'org.bluez.LEAdvertisingManager1'
    BLUEZ_ADV_MONITOR_MANAGER_IFACE = 'org.bluez.AdvertisementMonitorManager1'
    BLUEZ_AGENT_MANAGER_PATH = '/org/bluez'
    BLUEZ_AGENT_MANAGER_IFACE = 'org.bluez.AgentManager1'
    BLUEZ_PROFILE_MANAGER_PATH = '/org/bluez'
    BLUEZ_PROFILE_MANAGER_IFACE = 'org.bluez.ProfileManager1'
    BLUEZ_ERROR_ALREADY_EXISTS = 'org.bluez.Error.AlreadyExists'
    BLUEZ_PLUGIN_DEVICE_IFACE = 'org.chromium.BluetoothDevice'
    DBUS_PROP_IFACE = 'org.freedesktop.DBus.Properties'
    AGENT_PATH = '/test/agent'

    BLUETOOTH_LIBDIR = '/var/lib/bluetooth'
    BTMON_STOP_DELAY_SECS = 3

    # Due to problems transferring a date object, we convert to stringtime first
    # This is the standard format that we will use.
    OUT_DATE_FORMAT = '%Y-%m-%d %H:%M:%S.%f'

    # Timeout for how long we'll wait for BlueZ and the Adapter to show up
    # after reset.
    ADAPTER_TIMEOUT = 30

    # How long to wait for hid device
    HID_TIMEOUT = 15
    HID_CHECK_SECS = 2

    # How long we should wait for property update signal before we cancel it
    PROPERTY_UPDATE_TIMEOUT_MILLI_SECS = 5000

    def __init__(self):
        # Open the Bluetooth Raw socket to the kernel which provides us direct,
        # raw, access to the HCI controller.
        self._raw = bluetooth_socket.BluetoothRawSocket()

        # Open the Bluetooth Control socket to the kernel which provides us
        # raw management access to the Bluetooth Host Subsystem. Read the list
        # of adapter indexes to determine whether or not this device has a
        # Bluetooth Adapter or not.
        self._control = bluetooth_socket.BluetoothControlSocket()
        self._has_adapter = len(self._control.read_index_list()) > 0

        # Create an Advertisement Monitor App Manager instance.
        # This needs to be created before making any dbus connections as
        # AdvMonitorAppMgr internally forks a new helper process and due to
        # a limitation of python, it is not possible to fork a new process
        # once any dbus connections are established.
        self.advmon_appmgr = adv_monitor_helper.AdvMonitorAppMgr()

        # Set up the connection to Upstart so we can start and stop services
        # and fetch the bluetoothd job.
        self._upstart_conn = dbus.connection.Connection(self.UPSTART_PATH)
        self._upstart = self._upstart_conn.get_object(
                None, self.UPSTART_MANAGER_PATH)

        bluetoothd_path = self._upstart.GetJobByName(
                self.BLUETOOTHD_JOB, dbus_interface=self.UPSTART_MANAGER_IFACE)
        self._bluetoothd = self._upstart_conn.get_object(None, bluetoothd_path)

        # Arrange for the GLib main loop to be the default.
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

        # Set up the connection to the D-Bus System Bus, get the object for
        # the Bluetooth Userspace Daemon (BlueZ) and that daemon's object for
        # the Bluetooth Adapter, and the advertising manager.
        self._system_bus = dbus.SystemBus()
        self._update_bluez()
        self._update_adapter()
        self._update_advertising()
        self._update_adv_monitor_manager()

        # The agent to handle pin code request, which will be
        # created when user calls pair_legacy_device method.
        self._pairing_agent = None
        # The default capability of the agent.
        self._capability = 'KeyboardDisplay'

        # Initailize a btmon object to record bluetoothd's activity.
        self.btmon = output_recorder.OutputRecorder(
                'btmon', stop_delay_secs=self.BTMON_STOP_DELAY_SECS)

        # Initialize a messages object to record general logging.
        self.messages = LogRecorder('/var/log/messages')

        self._cras_test_client = cras_utils.CrasTestClient()

        self.advertisements = []
        self.advmon_interleave_logger = InterleaveLogger()
        self._chrc_property = None
        self._timeout_id = 0
        self._signal_watch = None
        self._dbus_mainloop = gobject.MainLoop()

    @xmlrpc_server.dbus_safe(False)
    def set_debug_log_levels(self, dispatcher_vb, newblue_vb, bluez_vb,
                             kernel_vb):
        """Enable or disable the debug logs of bluetooth

        @param dispatcher_vb: verbosity of btdispatcher debug log, either 0 or 1
        @param newblue_vb: verbosity of newblued debug log, either 0 or 1
        @param bluez_vb: verbosity of bluez debug log, either 0 or 1
        @param kernel_vb: verbosity of kernel debug log, either 0 or 1

        """

        # TODO(b/145163508, b/145749798): update when debug logs is migrated to
        #                                 bluez.
        debug_object = self._system_bus.get_object(self.BLUETOOTH_SERVICE_NAME,
                                                   self.BLUEZ_DEBUG_LOG_PATH)
        debug_object.SetLevels(dbus.Byte(dispatcher_vb),
                               dbus.Byte(newblue_vb),
                               dbus.Byte(bluez_vb),
                               dbus.Byte(kernel_vb),
                               dbus_interface=self.BLUEZ_DEBUG_LOG_IFACE)
        return

    def log_message(self, msg):
        """ log a message to /var/log/messages."""
        try:
            cmd = ['logger', msg]
            subprocess.call(cmd)
        except Exception as e:
            logging.error("log_message %s failed with %s", cmd, str(e))

    def is_wrt_supported(self):
        """Check if Bluetooth adapter support WRT logs

        WRT is supported on Intel adapters other than (StP2 and WP2)

        @returns : True if adapter is Intel made.
        """
        # Dict of Intel Adapters that support WRT and vid:pid
        vid_pid_dict = {
                'HrP2': '8086:02f0',
                'ThP2': '8086:2526',
                'JfP2': '8086:31dc',
                'JfP2-2': '8086:9df0'
        }  # On Sarien/Arcada

        def _get_lspci_vid_pid(output):
            """ parse output of lspci -knn and get the vid:pid

            output is of the form '01:00.0 Network controller [0280]:
            \Intel Corporation Device [8086:2526] (rev 29)\n'

            @returns : 'vid:pid' or None
            """
            try:
                for i in output.split(b'\n'):
                    if 'Network controller' in i.decode('utf-8'):
                        logging.debug('Got line %s', i)
                        if 'Intel Corporation' in i.decode('utf-8'):
                            return i.split(b'[')[2].split(b']')[0]
                return None
            except Exception as e:
                logging.debug('Exception in _get_lspci_vidpid %s', str(e))
                return None

        try:
            cmd = ['lspci', '-knn']
            output = subprocess.check_output(cmd)
            vid_pid = _get_lspci_vid_pid(output)
            logging.debug("got vid_pid %s", vid_pid)
            if vid_pid is not None:
                if vid_pid in list(vid_pid_dict.values()):
                    return True
        except Exception as e:
            logging.error('is_intel_adapter  failed with %s', cmd, str(e))
            return False

    def enable_wrt_logs(self):
        """ Enable WRT logs for Intel Bluetooth adapters.

            This is applicable only to Intel adapters.
            Execute a series of custom hciconfig commands to
            setup WRT log collection

            Precondition :
                1) Check if the DUT has Intel controller other than StP2
                2) Make sure the controller is powered on
        """
        fw_trace_cmd = (
                'hcitool cmd 3f 7c 01 10 00 00 00 FE 81 02 80 04 00 00'
                ' 00 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00'
                ' 00 00 00 00 00 00 00')
        ddc_read_cmd = 'hcitool cmd 3f 8c 28 01'
        ddc_write_cmd_prefix = 'hcitool cmd 3f 8b 03 28 01'
        hw_trace_cmd = (
                'hcitool cmd 3f 6f 01 08 00 00 00 00 00 00 00 00 01 00'
                ' 00 03 01 03 03 03 10 03 6A 0A 6A 0A 6A 0A 6A 0A 00 00'
                ' 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00'
                ' 00 00 00 00 00 00')
        multi_comm_trace_str = ('000000F600000000005002000000003F3F3F3'
                                'F3F003F000000000000000001000000000000000000'
                                '000000000000000000000000000000000000000000'
                                '00000000000000000000000000000000000000000'
                                '00000000000000000')
        multi_comm_trace_file = ('/sys/kernel/debug/ieee80211'
                                 '/phy0/iwlwifi/iwlmvm/send_hcmd')

        def _execute_cmd(cmd_str, msg=''):
            """Wrapper around subprocess.check_output.

            @params cmd: Command to be executed as a string
            @params msg: Optional description of the command

            @returns: (True, output) if execution succeeded
                  (False, None) if execution failed

            """
            try:
                logging.info('Executing %s cmd', msg)
                cmd = cmd_str.split(' ')
                logging.debug('command is "%s"', cmd)
                output = subprocess.check_output(cmd)
                logging.info('%s cmd successfully executed', msg)
                logging.debug('output is %s', output)
                return (True, output)
            except Exception as e:
                logging.error('Exception %s while executing %s command',
                              str(e), msg)
                return (False, None)

        def _get_ddc_write_cmd(ddc_read_result, ddc_write_cmd_prefix):
            """ Create ddc_write_cmd from read command

           This function performs the following
           1) Take the output of ddc_read_cmd which is in following form
              '< HCI Command: ogf 0x3f, ocf 0x008c, plen 1\n
               01 \n>
               HCI Event: 0x0e plen 6\n  01 8C FC 12 00 18 \n'
           2) Take the last value of the output
              01 8C FC 12 00 ===>> 18 <====
           3) Bitwise or with 0x40
              0x18 | 0x40 = 0x58
           4) Add it to the end of the ddc_write_cmd
              'hcitool 01 8C FC 00 28 01 ===> 58 <===='

           """
            last_line = [
                    i for i in ddc_read_result.strip().split(b'\n') if i != ''
            ][-1]
            last_byte = [i for i in last_line.split(b' ') if i != ''][-1]
            processed_byte = hex(int(last_byte, 16) | 0x40).split('0x')[1]
            cmd = ddc_write_cmd_prefix + ' ' + processed_byte
            logging.debug('ddc_write_cmd is %s', cmd)
            return cmd

        try:
            logging.info('Enabling WRT logs')
            status, _ = _execute_cmd(fw_trace_cmd, 'FW trace cmd')
            if not status:
                logging.info('FW trace command execution failed')
                return False

            status, ddc_read_result = _execute_cmd(ddc_read_cmd, 'DDC Read')
            if not status:
                logging.info('DDC Read command  execution failed')
                return False

            ddc_write_cmd = _get_ddc_write_cmd(ddc_read_result,
                                               ddc_write_cmd_prefix)
            logging.debug('DDC Write command  is %s', ddc_write_cmd)
            status, _ = _execute_cmd(ddc_write_cmd, 'DDC Write')
            if not status:
                logging.info('DDC Write commanad execution failed')
                return False

            status, hw_trace_result = _execute_cmd(hw_trace_cmd, 'HW trace')
            if not status:
                logging.info('HW Trace command  execution failed')
                return False

            logging.debug('Executing the multi_comm_trace cmd %s to file %s',
                          multi_comm_trace_str, multi_comm_trace_file)
            with open(multi_comm_trace_file, 'w') as f:
                f.write(multi_comm_trace_str + '\n')
                f.flush()

            logging.info('WRT Logs enabled')
            return True
        except Exception as e:
            logging.error('Exception %s while enabling WRT logs', str(e))
            return False

    def collect_wrt_logs(self):
        """Collect the WRT logs for Intel Bluetooth adapters

           This is applicable only to Intel adapters.
           Execute following command to collect WRT log. The logs are
           copied to /var/spool/crash/

           'echo 1 > sudo tee /sys/kernel/debug/ieee80211/phy0'
                           '/iwlwifi/iwlmvm/fw_dbg_collect'
           This is to be called only after enable_wrt_logs is called


           Precondition:
                 1) enable_wrt_logs has been called
        """

        def _collect_logs():
            """Execute command to collect wrt logs."""
            try:
                with open(
                        '/sys/kernel/debug/ieee80211/phy0/iwlwifi/'
                        'iwlmvm/fw_dbg_collect', 'w') as f:
                    f.write('1')
                    f.flush()
                # There is some flakiness in log collection. This sleep
                # is due to the flakiness
                time.sleep(10)
                return True
            except Exception as e:
                logging.error('Exception %s in _collect logs ', str(e))
                return False

        def _get_num_log_files():
            """Return number of WRT log files."""
            try:
                return len(glob.glob('/var/spool/crash/devcoredump_iwlwifi*'))
            except Exception as e:
                logging.debug('Exception %s raised in _get_num_log_files',
                              str(e))
                return 0

        try:
            logging.info('Collecting WRT logs')
            #
            # The command to trigger the logs does seems to work always.
            # As a workaround for this flakiness, execute it multiple times
            # until a new log is created
            #
            num_logs_present = _get_num_log_files()
            logging.debug('%s logs present', num_logs_present)
            for i in range(10):
                time.sleep(1)
                logging.debug('Executing command to collect WRT logs ')
                if _collect_logs():
                    logging.debug('Command to collect WRT logs executed')
                else:
                    logging.debug('Command to collect WRT logs failed')
                    continue

                if _get_num_log_files() > num_logs_present:
                    logging.info('Successfully collected WRT logs ')
                    return True
                else:
                    logging.debug('Log file not written. Trying again')

            logging.info('Unable to collect WRT logs')
            return False
        except Exception as e:
            logging.error('Exception %s while collecting WRT logs', str(e))
            return False

    @xmlrpc_server.dbus_safe(False)
    def start_bluetoothd(self):
        """start bluetoothd.

        This includes powering up the adapter.

        @returns: True if bluetoothd is started correctly.
                  False otherwise.

        """
        try:
            self._bluetoothd.Start(dbus.Array(signature='s'),
                                   True,
                                   dbus_interface=self.UPSTART_JOB_IFACE)
        except dbus.exceptions.DBusException as e:
            # if bluetoothd was already started, the exception looks like
            #     dbus.exceptions.DBusException:
            #     com.ubuntu.Upstart0_6.Error.AlreadyStarted: Job is already
            #     running: bluetoothd
            if e.get_dbus_name() != self.UPSTART_ERROR_ALREADYSTARTED:
                logging.error('Error starting bluetoothd: %s', e)
                return False

        logging.debug('waiting for bluez start')
        try:
            utils.poll_for_condition(condition=self._update_bluez,
                                     desc='Bluetooth Daemon has started.',
                                     timeout=self.ADAPTER_TIMEOUT)
        except Exception as e:
            logging.error('timeout: error starting bluetoothd: %s', e)
            return False

        # Waiting for the self._adapter object.
        # This does not mean that the adapter is powered on.
        logging.debug('waiting for bluez to obtain adapter information')
        try:
            utils.poll_for_condition(
                    condition=self._update_adapter,
                    desc='Bluetooth Daemon has adapter information.',
                    timeout=self.ADAPTER_TIMEOUT)
        except Exception as e:
            logging.error('timeout: error starting adapter: %s', e)
            return False

        # Waiting for the self._advertising interface object.
        logging.debug('waiting for bluez to obtain interface manager.')
        try:
            utils.poll_for_condition(
                    condition=self._update_advertising,
                    desc='Bluetooth Daemon has advertising interface.',
                    timeout=self.ADAPTER_TIMEOUT)
        except utils.TimeoutError:
            logging.error('timeout: error getting advertising interface')
            return False

        return True

    @xmlrpc_server.dbus_safe(False)
    def stop_bluetoothd(self):
        """stop bluetoothd.

        @returns: True if bluetoothd is stopped correctly.
                  False otherwise.

        """

        def bluez_stopped():
            """Checks the bluetooth daemon status.

            @returns: True if bluez is stopped. False otherwise.

            """
            return not self._update_bluez()

        try:
            self._bluetoothd.Stop(dbus.Array(signature='s'),
                                  True,
                                  dbus_interface=self.UPSTART_JOB_IFACE)
        except dbus.exceptions.DBusException as e:
            # If bluetoothd was stopped already, the exception looks like
            #    dbus.exceptions.DBusException:
            #    com.ubuntu.Upstart0_6.Error.UnknownInstance: Unknown instance:
            if e.get_dbus_name() != self.UPSTART_ERROR_UNKNOWNINSTANCE:
                logging.error('Error stopping bluetoothd!')
                return False

        logging.debug('waiting for bluez stop')
        try:
            utils.poll_for_condition(condition=bluez_stopped,
                                     desc='Bluetooth Daemon has stopped.',
                                     timeout=self.ADAPTER_TIMEOUT)
            bluetoothd_stopped = True
        except Exception as e:
            logging.error('timeout: error stopping bluetoothd: %s', e)
            bluetoothd_stopped = False

        return bluetoothd_stopped

    def is_bluetoothd_running(self):
        """Is bluetoothd running?

        @returns: True if bluetoothd is running

        """
        return bool(self._get_dbus_proxy_for_bluetoothd())

    def is_bluetoothd_proxy_valid(self):
        """Checks whether the proxy object for bluetoothd is ok.

        The dbus proxy object (self._bluez) can become unusable if bluetoothd
        crashes or restarts for any reason. This method checks whether this has
        happened by attempting to use the object proxy. If bluetoothd has
        restarted (or is not available), then the session will no longer be
        valid and this will result in a dbus exception.

        Returns:
            True if the bluez proxy is still usable. False otherwise.
        """

        try:
            _ = self._bluez.GetManagedObjects(
                    dbus_interface=self.BLUEZ_MANAGER_IFACE)
        except dbus.exceptions.DBusException:
            return False

        return True

    def _update_bluez(self):
        """Store a D-Bus proxy for the Bluetooth daemon in self._bluez.

        This may be called in a loop until it returns True to wait for the
        daemon to be ready after it has been started.

        @return True on success, False otherwise.

        """
        self._bluez = self._get_dbus_proxy_for_bluetoothd()
        return bool(self._bluez)

    @xmlrpc_server.dbus_safe(False)
    def _get_dbus_proxy_for_bluetoothd(self):
        """Get the D-Bus proxy for the Bluetooth daemon.

        @return True on success, False otherwise.

        """
        bluez = None
        try:
            bluez = self._system_bus.get_object(self.BLUEZ_SERVICE_NAME,
                                                self.BLUEZ_MANAGER_PATH)
            logging.debug('bluetoothd is running')
        except dbus.exceptions.DBusException as e:
            # When bluetoothd is not running, the exception looks like
            #     dbus.exceptions.DBusException:
            #     org.freedesktop.DBus.Error.ServiceUnknown: The name org.bluez
            #     was not provided by any .service files
            if e.get_dbus_name() == self.DBUS_ERROR_SERVICEUNKNOWN:
                logging.debug('bluetoothd is not running')
            else:
                logging.error('Error getting dbus proxy for Bluez: %s', e)
        return bluez

    def _update_adapter(self):
        """Store a D-Bus proxy for the local adapter in self._adapter.

        This may be called in a loop until it returns True to wait for the
        daemon to be ready, and have obtained the adapter information itself,
        after it has been started.

        Since not all devices will have adapters, this will also return True
        in the case where we have obtained an empty adapter index list from the
        kernel.

        Note that this method does not power on the adapter.

        @return True on success, including if there is no local adapter,
            False otherwise.

        """
        self._adapter = None
        if self._bluez is None:
            logging.warning('Bluez not found!')
            return False
        if not self._has_adapter:
            logging.debug('Device has no adapter; returning')
            return True
        self._adapter = self._get_adapter()
        return bool(self._adapter)

    def _update_advertising(self):
        """Store a D-Bus proxy for the local advertising interface manager.

        This may be called repeatedly in a loop until True is returned;
        otherwise we wait for bluetoothd to start. After bluetoothd starts, we
        check the existence of a local adapter and proceed to get the
        advertisement interface manager.

        Since not all devices will have adapters, this will also return True
        in the case where there is no adapter.

        @return True on success, including if there is no local adapter,
                False otherwise.

        """
        self._advertising = None
        if self._bluez is None:
            logging.warning('Bluez not found!')
            return False
        if not self._has_adapter:
            logging.debug('Device has no adapter; returning')
            return True
        self._advertising = self._get_advertising()
        return bool(self._advertising)

    def _update_adv_monitor_manager(self):
        """Store a D-Bus proxy for the local advertisement monitor manager.

        This may be called repeatedly in a loop until True is returned;
        otherwise we wait for bluetoothd to start. After bluetoothd starts, we
        check the existence of a local adapter and proceed to get the
        advertisement monitor manager interface.

        Since not all devices will have adapters, this will also return True
        in the case where there is no adapter.

        @return True on success, including if there is no local adapter,
                False otherwise.

        """
        self._adv_monitor_manager = None
        if self._bluez is None:
            logging.warning('Bluez not found!')
            return False
        if not self._has_adapter:
            logging.debug('Device has no adapter; returning without '
                          'advertisement monitor manager')
            return True
        self._adv_monitor_manager = self._get_adv_monitor_manager()
        return bool(self._adv_monitor_manager)

    @xmlrpc_server.dbus_safe(False)
    def _get_adapter(self):
        """Get the D-Bus proxy for the local adapter.

        @return the adapter on success. None otherwise.

        """
        objects = self._bluez.GetManagedObjects(
                dbus_interface=self.BLUEZ_MANAGER_IFACE)
        for path, ifaces in six.iteritems(objects):
            logging.debug('%s -> %r', path, list(ifaces.keys()))
            if self.BLUEZ_ADAPTER_IFACE in ifaces:
                logging.debug('using adapter %s', path)
                adapter = self._system_bus.get_object(self.BLUEZ_SERVICE_NAME,
                                                      path)
                return adapter
        else:
            logging.warning('No adapter found in interface!')
            return None

    @xmlrpc_server.dbus_safe(False)
    def _get_advertising(self):
        """Get the D-Bus proxy for the local advertising interface.

        @return the advertising interface object.

        """
        return dbus.Interface(self._adapter,
                              self.BLUEZ_LE_ADVERTISING_MANAGER_IFACE)

    @xmlrpc_server.dbus_safe(False)
    def _get_adv_monitor_manager(self):
        """Get the D-Bus proxy for the local advertisement monitor manager.

        @return the advertisement monitor manager interface object.

        """
        return dbus.Interface(self._adapter,
                              self.BLUEZ_ADV_MONITOR_MANAGER_IFACE)

    @xmlrpc_server.dbus_safe(False)
    def reset_on(self):
        """Reset the adapter and settings and power up the adapter.

        @return True on success, False otherwise.

        """
        return self._reset(set_power=True)

    @xmlrpc_server.dbus_safe(False)
    def reset_off(self):
        """Reset the adapter and settings, leave the adapter powered off.

        @return True on success, False otherwise.

        """
        return self._reset(set_power=False)

    def has_adapter(self):
        """Return if an adapter is present.

        This will only return True if we have determined both that there is
        a Bluetooth adapter on this device (kernel adapter index list is not
        empty) and that the Bluetooth daemon has exported an object for it.

        @return True if an adapter is present, False if not.

        """
        return self._has_adapter and self._adapter is not None

    def is_wake_enabled(self):
        """Checks whether the bluetooth adapter has wake enabled.

        This will walk through all parents of the hci0 sysfs path and try to
        find one with a 'power/wakeup' entry and returns whether its value is
        'enabled'.

        @return True if 'power/wakeup' of an hci0 parent is 'enabled'
        """
        enabled = self._is_wake_enabled()
        return enabled

    def set_wake_enabled(self, value):
        """Sets wake enabled to the value if path exists.

        This will walk through all parents of the hci0 sysfs path and write the
        value to the first one it finds.

        Args:
            value: Sets power/wakeup to "enabled" if value is true, else
                   "disabled"

        @return True if it wrote value to a power/wakeup, False otherwise
        """
        return self._set_wake_enabled(value)

    def wait_for_hid_device(self, device_address):
        """Waits for hid device with given device address.

        Args:
            device_address: Peripheral address
        """

        def match_hid_to_device(hidpath, device_address):
            """Check if given hid syspath is for the given device address """
            # If the syspath has a uniq property that matches the peripheral
            # device's address, then it has matched
            props = UdevadmInfo.GetProperties(hidpath)
            if props.get('uniq', '').lower() == device_address.lower():
                logging.info('Found hid device for address {} at {}'.format(
                        device_address, hidpath))
                return True
            else:
                logging.info('Path {} is not right device.'.format(hidpath))

            return False

        start = datetime.now()

        # Keep scanning udev for correct hid device
        while (datetime.now() - start).seconds <= self.HID_TIMEOUT:
            existing_inputs = UdevadmTrigger(
                    subsystem_match=['input']).DryRun()
            for entry in existing_inputs:
                bt_hid = any([t in entry for t in ['uhid', 'hci']])
                logging.info('udevadm trigger entry is {}: {}'.format(
                        bt_hid, entry))

                if bt_hid and match_hid_to_device(entry, device_address):
                    return True

            time.sleep(self.HID_CHECK_SECS)

        return False

    def _reset(self, set_power=False):
        """Remove remote devices and set adapter to set_power state.

        Do not restart bluetoothd as this may incur a side effect.
        The unhappy chrome may disable the adapter randomly.

        @param set_power: adapter power state to set (True or False).

        @return True on success, False otherwise.

        """
        logging.debug('_reset')

        if not self._adapter:
            logging.warning('Adapter not found!')
            return False

        objects = self._bluez.GetManagedObjects(
                dbus_interface=self.BLUEZ_MANAGER_IFACE, byte_arrays=True)

        devices = []
        for path, ifaces in six.iteritems(objects):
            if self.BLUEZ_DEVICE_IFACE in ifaces:
                devices.append(objects[path][self.BLUEZ_DEVICE_IFACE])

        # Turn on the adapter in order to remove all remote devices.
        if not self._is_powered_on():
            if not self._set_powered(True):
                logging.warning('Unable to power on the adapter')
                return False

        for device in devices:
            logging.debug('removing %s', device.get('Address'))
            self.remove_device_object(device.get('Address'))

        # Toggle power to the adapter.
        if not self._set_powered(False):
            logging.warning('Unable to power off adapter')
            return False
        if set_power and not self._set_powered(True):
            logging.warning('Unable to power on adapter')
            return False

        return True

    @xmlrpc_server.dbus_safe(False)
    def set_powered(self, powered):
        """Set the adapter power state.

        @param powered: adapter power state to set (True or False).

        @return True on success, False otherwise.

        """
        if not self._adapter:
            if not powered:
                # Return success if we are trying to power off an adapter that's
                # missing or gone away, since the expected result has happened.
                return True
            else:
                logging.warning('Adapter not found!')
                return False
        return self._set_powered(powered)

    @xmlrpc_server.dbus_safe(False)
    def _set_powered(self, powered):
        """Set the adapter power state.

        @param powered: adapter power state to set (True or False).

        """
        logging.debug('_set_powered %r', powered)
        self._adapter.Set(self.BLUEZ_ADAPTER_IFACE,
                          'Powered',
                          dbus.Boolean(powered, variant_level=1),
                          dbus_interface=dbus.PROPERTIES_IFACE)
        return True

    @xmlrpc_server.dbus_safe(False)
    def set_discoverable(self, discoverable):
        """Set the adapter discoverable state.

        @param discoverable: adapter discoverable state to set (True or False).

        @return True on success, False otherwise.

        """
        if not discoverable and not self._adapter:
            # Return success if we are trying to make an adapter that's
            # missing or gone away, undiscoverable, since the expected result
            # has happened.
            return True
        self._adapter.Set(self.BLUEZ_ADAPTER_IFACE,
                          'Discoverable',
                          dbus.Boolean(discoverable, variant_level=1),
                          dbus_interface=dbus.PROPERTIES_IFACE)
        return True

    @xmlrpc_server.dbus_safe(False)
    def get_discoverable_timeout(self):
        """Get the adapter discoverable_timeout.

        @return True on success, False otherwise.

        """
        return int(
                self._adapter.Get(self.BLUEZ_ADAPTER_IFACE,
                                  'DiscoverableTimeout',
                                  dbus_interface=dbus.PROPERTIES_IFACE))

    @xmlrpc_server.dbus_safe(False)
    def set_discoverable_timeout(self, discoverable_timeout):
        """Set the adapter discoverable_timeout property.

        @param discoverable_timeout: adapter discoverable_timeout value
               in seconds to set (Integer).

        @return True on success, False otherwise.

        """
        self._adapter.Set(self.BLUEZ_ADAPTER_IFACE,
                          'DiscoverableTimeout',
                          dbus.UInt32(discoverable_timeout, variant_level=1),
                          dbus_interface=dbus.PROPERTIES_IFACE)
        return True

    @xmlrpc_server.dbus_safe(False)
    def get_pairable_timeout(self):
        """Get the adapter pairable_timeout.

        @return True on success, False otherwise.

        """
        return int(
                self._adapter.Get(self.BLUEZ_ADAPTER_IFACE,
                                  'PairableTimeout',
                                  dbus_interface=dbus.PROPERTIES_IFACE))

    @xmlrpc_server.dbus_safe(False)
    def set_pairable_timeout(self, pairable_timeout):
        """Set the adapter pairable_timeout property.

        @param pairable_timeout: adapter pairable_timeout value
               in seconds to set (Integer).

        @return True on success, False otherwise.

        """
        self._adapter.Set(self.BLUEZ_ADAPTER_IFACE,
                          'PairableTimeout',
                          dbus.UInt32(pairable_timeout, variant_level=1),
                          dbus_interface=dbus.PROPERTIES_IFACE)
        return True

    @xmlrpc_server.dbus_safe(False)
    def set_pairable(self, pairable):
        """Set the adapter pairable state.

        @param pairable: adapter pairable state to set (True or False).

        @return True on success, False otherwise.

        """
        self._adapter.Set(self.BLUEZ_ADAPTER_IFACE,
                          'Pairable',
                          dbus.Boolean(pairable, variant_level=1),
                          dbus_interface=dbus.PROPERTIES_IFACE)
        return True

    @xmlrpc_server.dbus_safe(False)
    def set_adapter_alias(self, alias):
        """Set the adapter alias.

        @param alias: adapter alias to set with type String

        @return True on success, False otherwise.
        """
        self._adapter.Set(self.BLUEZ_ADAPTER_IFACE,
                          'Alias',
                          dbus.String(alias),
                          dbus_interface=dbus.PROPERTIES_IFACE)
        return True

    @xmlrpc_server.dbus_safe(False)
    def _get_adapter_properties(self):
        """Read the adapter properties from the Bluetooth Daemon.

        @return the properties as a JSON-encoded dictionary on success,
            the value False otherwise.

        """
        if self._bluez and self._adapter:
            objects = self._bluez.GetManagedObjects(
                    dbus_interface=self.BLUEZ_MANAGER_IFACE)
            props = objects[self._adapter.object_path][
                    self.BLUEZ_ADAPTER_IFACE]
        else:
            props = {}
        logging.debug('get_adapter_properties')
        for i in props.items():
            logging.debug(i)
        return props

    def get_adapter_properties(self):
        return json.dumps(self._get_adapter_properties())

    def _is_powered_on(self):
        return bool(self._get_adapter_properties().get(u'Powered'))

    def _get_wake_enabled_path(self):
        # Walk up the parents from hci0 sysfs path and find the first one with
        # a power/wakeup property. Return that path (including power/wakeup).

        # Resolve hci path to get full device path (i.e. w/ usb or uart)
        search_at = os.path.realpath('/sys/class/bluetooth/hci0')

        # Exit early if path doesn't exist
        if not os.path.exists(search_at):
            return None

        # Walk up parents and try to find one with 'power/wakeup'
        for _ in range(search_at.count('/') - 1):
            search_at = os.path.normpath(os.path.join(search_at, '..'))
            try:
                path = os.path.join(search_at, 'power', 'wakeup')
                with open(path, 'r') as f:
                    return path
            except IOError:
                # No power wakeup at the given location so keep going
                continue

        return None

    def _is_wake_enabled(self):
        search_at = self._get_wake_enabled_path()

        if search_at is not None:
            try:
                with open(search_at, 'r') as f:
                    value = f.read()
                    logging.info('Power/wakeup found at {}: {}'.format(
                            search_at, value))
                    return 'enabled' in value
            except IOError:
                # Path was not readable
                return False

        logging.debug('No power/wakeup path found')
        return False

    def _set_wake_enabled(self, value):
        path = self._get_wake_enabled_path()
        if path is not None:
            try:
                with open(path, 'w') as f:
                    f.write('enabled' if value else 'disabled')
                    return True
            except IOError:
                # Path was not writeable
                return False

        return False

    def read_version(self):
        """Read the version of the management interface from the Kernel.

        @return the information as a JSON-encoded tuple of:
          ( version, revision )

        """
        #TODO(howardchung): resolve 'cannot allocate memory' error when
        #                   BluetoothControlSocket idle too long(about 3 secs)
        #                   (b:137603211)
        _control = bluetooth_socket.BluetoothControlSocket()
        return json.dumps(_control.read_version())

    def read_supported_commands(self):
        """Read the set of supported commands from the Kernel.

        @return the information as a JSON-encoded tuple of:
          ( commands, events )

        """
        #TODO(howardchung): resolve 'cannot allocate memory' error when
        #                   BluetoothControlSocket idle too long(about 3 secs)
        #                   (b:137603211)
        _control = bluetooth_socket.BluetoothControlSocket()
        return json.dumps(_control.read_supported_commands())

    def read_index_list(self):
        """Read the list of currently known controllers from the Kernel.

        @return the information as a JSON-encoded array of controller indexes.

        """
        #TODO(howardchung): resolve 'cannot allocate memory' error when
        #                   BluetoothControlSocket idle too long(about 3 secs)
        #                   (b:137603211)
        _control = bluetooth_socket.BluetoothControlSocket()
        return json.dumps(_control.read_index_list())

    def read_info(self):
        """Read the adapter information from the Kernel.

        @return the information as a JSON-encoded tuple of:
          ( address, bluetooth_version, manufacturer_id,
            supported_settings, current_settings, class_of_device,
            name, short_name )

        """
        #TODO(howardchung): resolve 'cannot allocate memory' error when
        #                   BluetoothControlSocket idle too long(about 3 secs)
        #                   (b:137603211)
        _control = bluetooth_socket.BluetoothControlSocket()
        return json.dumps(_control.read_info(0))

    def add_device(self, address, address_type, action):
        """Add a device to the Kernel action list.

        @param address: Address of the device to add.
        @param address_type: Type of device in @address.
        @param action: Action to take.

        @return on success, a JSON-encoded typle of:
          ( address, address_type ), None on failure.

        """
        #TODO(howardchung): resolve 'cannot allocate memory' error when
        #                   BluetoothControlSocket idle too long(about 3 secs)
        #                   (b:137603211)
        _control = bluetooth_socket.BluetoothControlSocket()
        return json.dumps(_control.add_device(0, address, address_type,
                                              action))

    def remove_device(self, address, address_type):
        """Remove a device from the Kernel action list.

        @param address: Address of the device to remove.
        @param address_type: Type of device in @address.

        @return on success, a JSON-encoded typle of:
          ( address, address_type ), None on failure.

        """
        #TODO(howardchung): resolve 'cannot allocate memory' error when
        #                   BluetoothControlSocket idle too long(about 3 secs)
        #                   (b:137603211)
        _control = bluetooth_socket.BluetoothControlSocket()
        return json.dumps(_control.remove_device(0, address, address_type))

    @xmlrpc_server.dbus_safe(False)
    def _get_devices(self):
        """Read information about remote devices known to the adapter.

        @return the properties of each device in a list

        """
        objects = self._bluez.GetManagedObjects(
                dbus_interface=self.BLUEZ_MANAGER_IFACE, byte_arrays=True)
        devices = []
        for path, ifaces in six.iteritems(objects):
            if self.BLUEZ_DEVICE_IFACE in ifaces:
                devices.append(objects[path][self.BLUEZ_DEVICE_IFACE])
        return devices

    def _encode_base64_json(self, data):
        """Base64 encode and json encode the data.
        Required to handle non-ascii data

        @param data: data to be base64 and JSON encoded

        @return: base64 and JSON encoded data

        """
        logging.debug('_encode_base64_json raw data is %s', data)
        b64_encoded = utils.base64_recursive_encode(data)
        logging.debug('base64 encoded data is %s', b64_encoded)
        json_encoded = json.dumps(b64_encoded)
        logging.debug('JSON encoded data is %s', json_encoded)
        return json_encoded

    def get_devices(self):
        """Read information about remote devices known to the adapter.

        @return the properties of each device as a JSON-encoded array of
            dictionaries on success, the value False otherwise.

        """
        devices = self._get_devices()
        return self._encode_base64_json(devices)

    @xmlrpc_server.dbus_safe(None)
    def get_device_property(self, address, prop_name):
        """Read a property of BT device by directly querying device dbus object

        @param address: Address of the device to query
        @param prop_name: Property to be queried

        @return Base 64 JSON repr of property if device is found and has
                property, otherwise None on failure. JSON is a recursive
                converter, automatically converting dbus types to python natives
                and base64 allows us to pass special characters over xmlrpc.
                Decode is done in bluetooth_device.py
        """

        prop_val = None

        # Grab dbus object, _find_device will catch any thrown dbus error
        device_obj = self._find_device(address)

        if device_obj:
            # Query dbus object for property
            prop_val = device_obj.Get(self.BLUEZ_DEVICE_IFACE,
                                      prop_name,
                                      dbus_interface=dbus.PROPERTIES_IFACE)

        return self._encode_base64_json(prop_val)

    @xmlrpc_server.dbus_safe(None)
    def get_battery_property(self, address, prop_name):
        """Read a property from Battery1 interface.

        @param address: Address of the device to query
        @param prop_name: Property to be queried

        @return The battery percentage value, or None if does not exist.
        """

        prop_val = None

        # Grab dbus object, _find_battery will catch any thrown dbus error
        battery_obj = self._find_battery(address)

        if battery_obj:
            # Query dbus object for property
            prop_val = battery_obj.Get(self.BLUEZ_BATTERY_IFACE,
                                       prop_name,
                                       dbus_interface=dbus.PROPERTIES_IFACE)

        return dbus_util.dbus2primitive(prop_val)

    @xmlrpc_server.dbus_safe(False)
    def set_discovery_filter(self, filter):
        """Set the discovery filter.

        @param filter: The discovery filter to set.

        @return True on success, False otherwise.

        """
        if not self._adapter:
            return False
        self._adapter.SetDiscoveryFilter(
                filter, dbus_interface=self.BLUEZ_ADAPTER_IFACE)
        return True

    @xmlrpc_server.dbus_safe(False)
    @dbus_print_error()
    def start_discovery(self):
        """Start discovery of remote devices.

        Obtain the discovered device information using get_devices(), called
        stop_discovery() when done.

        @return True on success, False otherwise.

        """
        if not self._adapter:
            return (False, "Adapter Not Found")
        self._adapter.StartDiscovery(dbus_interface=self.BLUEZ_ADAPTER_IFACE)
        return (True, None)

    @dbus_print_error()
    def stop_discovery(self):
        """Stop discovery of remote devices.

        @return True on success, False otherwise.

        """
        if not self._adapter:
            return (False, "Adapter Not Found")
        self._adapter.StopDiscovery(dbus_interface=self.BLUEZ_ADAPTER_IFACE)
        return (True, None)

    def get_dev_info(self):
        """Read raw HCI device information.

        @return JSON-encoded tuple of:
                (index, name, address, flags, device_type, bus_type,
                       features, pkt_type, link_policy, link_mode,
                       acl_mtu, acl_pkts, sco_mtu, sco_pkts,
                       err_rx, err_tx, cmd_tx, evt_rx, acl_tx, acl_rx,
                       sco_tx, sco_rx, byte_rx, byte_tx) on success,
                None on failure.

        """
        return json.dumps(self._raw.get_dev_info(0))

    @dbus_print_error(None)
    def get_supported_capabilities(self):
        """ Get supported capabilities of the adapter

        @returns (capabilities, None) on Success. (None, <error>) on failure
        """
        value = self._adapter.GetSupportedCapabilities(
                dbus_interface=self.BLUEZ_ADAPTER_IFACE)
        return (json.dumps(value), None)

    @xmlrpc_server.dbus_safe(False)
    def register_profile(self, path, uuid, options):
        """Register new profile (service).

        @param path: Path to the profile object.
        @param uuid: Service Class ID of the service as string.
        @param options: Dictionary of options for the new service, compliant
                        with BlueZ D-Bus Profile API standard.

        @return True on success, False otherwise.

        """
        profile_manager = dbus.Interface(
                self._system_bus.get_object(self.BLUEZ_SERVICE_NAME,
                                            self.BLUEZ_PROFILE_MANAGER_PATH),
                self.BLUEZ_PROFILE_MANAGER_IFACE)
        dbus_object = self._system_bus.get_object(self.BLUEZ_SERVICE_NAME,
                                                  path)
        profile_manager.RegisterProfile(
                dbus_object, uuid, dbus.Dictionary(options, signature='sv'))
        return True

    def has_device(self, address):
        """Checks if the device with a given address exists.

        @param address: Address of the device.

        @returns: True if there is an interface object with that address.
                  False if the device is not found.

        @raises: Exception if a D-Bus error is encountered.

        """
        result = self._find_device(address)
        logging.debug('has_device result: %s', str(result))

        # The result being False indicates that there is a D-Bus error.
        if result is False:
            raise Exception('dbus.Interface error')

        # Return True if the result is not None, e.g. a D-Bus interface object;
        # False otherwise.
        return bool(result)

    @xmlrpc_server.dbus_safe(False)
    def _find_device(self, address):
        """Finds the device with a given address.

        Find the device with a given address and returns the
        device interface.

        @param address: Address of the device.

        @returns: An 'org.bluez.Device1' interface to the device.
                  None if device can not be found.
        """
        path = self._get_device_path(address)
        if path:
            obj = self._system_bus.get_object(self.BLUEZ_SERVICE_NAME, path)
            return dbus.Interface(obj, self.BLUEZ_DEVICE_IFACE)
        logging.info('Device not found')
        return None

    @xmlrpc_server.dbus_safe(None)
    def _find_battery(self, address):
        """Finds the battery with a given address.

        Find the battery with a given address and returns the
        battery interface.

        @param address: Address of the device.

        @returns: An 'org.bluez.Battery1' interface to the device.
                  None if device can not be found.
        """
        path = self._get_device_path(address)
        if path:
            obj = self._system_bus.get_object(self.BLUEZ_SERVICE_NAME, path)
            return dbus.Interface(obj, self.BLUEZ_BATTERY_IFACE)
        logging.info('Battery not found')
        return None

    @xmlrpc_server.dbus_safe(False)
    def _get_device_path(self, address):
        """Gets the path for a device with a given address.

        Find the device with a given address and returns the
        the path for the device.

        @param address: Address of the device.

        @returns: The path to the address of the device, or None if device is
            not found in the object tree.

        """

        # Create device path, i.e. '/org/bluez/hci0/dev_AA_BB_CC_DD_EE_FF' based
        # on path assignment scheme used in bluez
        address_up = address.replace(':', '_')
        device_path = '{}/dev_{}'.format(self._adapter.object_path, address_up)

        # Verify the Address property agrees to confirm we have the device
        try:
            device = self._system_bus.get_object(self.BLUEZ_SERVICE_NAME,
                                                 device_path)
            found_addr = device.Get(self.BLUEZ_DEVICE_IFACE,
                                    'Address',
                                    dbus_interface=dbus.PROPERTIES_IFACE)

            if found_addr == address:
                logging.info('Device found at {}'.format(device_path))
                return device_path

        except dbus.exceptions.DBusException as e:
            log_msg = 'Couldn\'t reach device: {}'.format(str(e))
            logging.debug(log_msg)

        logging.debug('No device found at {}'.format(device_path))
        return None

    @xmlrpc_server.dbus_safe(False)
    def _setup_pairing_agent(self, pin):
        """Initializes and resiters a PairingAgent to handle authentication.

        @param pin: The pin code this agent will answer.

        """
        if self._pairing_agent:
            logging.info(
                    'Removing the old agent before initializing a new one')
            self._pairing_agent.remove_from_connection()
            self._pairing_agent = None
        self._pairing_agent = PairingAgent(pin, self._system_bus,
                                           self.AGENT_PATH)
        agent_manager = dbus.Interface(
                self._system_bus.get_object(self.BLUEZ_SERVICE_NAME,
                                            self.BLUEZ_AGENT_MANAGER_PATH),
                self.BLUEZ_AGENT_MANAGER_IFACE)
        try:
            agent_obj = self._system_bus.get_object(self.BLUEZ_SERVICE_NAME,
                                                    self.AGENT_PATH)
            agent_manager.RegisterAgent(agent_obj,
                                        dbus.String(self._capability))
        except dbus.exceptions.DBusException as e:
            if e.get_dbus_name() == self.BLUEZ_ERROR_ALREADY_EXISTS:
                logging.info('Unregistering old agent and registering the new')
                agent_manager.UnregisterAgent(agent_obj)
                agent_manager.RegisterAgent(agent_obj,
                                            dbus.String(self._capability))
            else:
                logging.error('Error setting up pin agent: %s', e)
                raise
        logging.info('Agent registered: %s', self.AGENT_PATH)

    @xmlrpc_server.dbus_safe(False)
    def _is_paired(self, device):
        """Checks if a device is paired.

        @param device: An 'org.bluez.Device1' interface to the device.

        @returns: True if device is paired. False otherwise.

        """
        props = dbus.Interface(device, dbus.PROPERTIES_IFACE)
        paired = props.Get(self.BLUEZ_DEVICE_IFACE, 'Paired')
        return bool(paired)

    @xmlrpc_server.dbus_safe(False)
    def device_is_paired(self, address):
        """Checks if a device is paired.

        @param address: address of the device.

        @returns: True if device is paired. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False
        return self._is_paired(device)

    @xmlrpc_server.dbus_safe(False)
    def _is_connected(self, device):
        """Checks if a device is connected.

        @param device: An 'org.bluez.Device1' interface to the device.

        @returns: True if device is connected. False otherwise.

        """
        props = dbus.Interface(device, dbus.PROPERTIES_IFACE)
        connected = props.Get(self.BLUEZ_DEVICE_IFACE, 'Connected')
        logging.info('Got connected = %r', connected)
        return bool(connected)

    @xmlrpc_server.dbus_safe(False)
    def _set_trusted_by_device(self, device, trusted=True):
        """Set the device trusted by device object.

        @param device: the device object to set trusted.
        @param trusted: True or False indicating whether to set trusted or not.

        @returns: True if successful. False otherwise.

        """
        try:
            properties = dbus.Interface(device, self.DBUS_PROP_IFACE)
            properties.Set(self.BLUEZ_DEVICE_IFACE, 'Trusted',
                           dbus.Boolean(trusted, variant_level=1))
            return True
        except Exception as e:
            logging.error('_set_trusted_by_device: %s', e)
        except:
            logging.error('_set_trusted_by_device: unexpected error')
        return False

    @xmlrpc_server.dbus_safe(False)
    def _set_trusted_by_path(self, device_path, trusted=True):
        """Set the device trusted by the device path.

        @param device_path: the object path of the device.
        @param trusted: True or False indicating whether to set trusted or not.

        @returns: True if successful. False otherwise.

        """
        try:
            device = self._system_bus.get_object(self.BLUEZ_SERVICE_NAME,
                                                 device_path)
            return self._set_trusted_by_device(device, trusted)
        except Exception as e:
            logging.error('_set_trusted_by_path: %s', e)
        except:
            logging.error('_set_trusted_by_path: unexpected error')
        return False

    @xmlrpc_server.dbus_safe(False)
    def set_trusted(self, address, trusted=True):
        """Set the device trusted by address.

        @param address: The bluetooth address of the device.
        @param trusted: True or False indicating whether to set trusted or not.

        @returns: True if successful. False otherwise.

        """
        try:
            device = self._find_device(address)
            return self._set_trusted_by_device(device, trusted)
        except Exception as e:
            logging.error('set_trusted: %s', e)
        except:
            logging.error('set_trusted: unexpected error')
        return False

    @xmlrpc_server.dbus_safe(False)
    def pair_legacy_device(self, address, pin, trusted, timeout=60):
        """Pairs a device with a given pin code.

        Registers a agent who handles pin code request and
        pairs a device with known pin code. After pairing, this function will
        automatically connect to the device as well (prevents timing issues
        between pairing and connect and reduces overall test execution time).

        @param address: Address of the device to pair.
        @param pin: The pin code of the device to pair.
        @param trusted: indicating whether to set the device trusted.
        @param timeout: The timeout in seconds for pairing.

        @returns: True on success. False otherwise.

        """

        def connect_reply():
            """Handler when connect succeeded."""
            logging.info('Device connected: %s', device_path)
            mainloop.quit()

        def connect_error(error):
            """Handler when connect failed.

            @param error: one of the errors defined in org.bluez.Error
            representing the error in connect.
            """
            logging.error('Connect device failed: %s', error)
            mainloop.quit()

        def pair_reply():
            """Handler when pairing succeeded."""
            logging.info('Device paired: %s', device_path)
            if trusted:
                self._set_trusted_by_path(device_path, trusted=True)
                logging.info('Device trusted: %s', device_path)

            # On finishing pairing, also connect; let connect result exit
            # mainloop instead
            device.Connect(reply_handler=connect_reply,
                           error_handler=connect_error,
                           timeout=timeout * 1000)

        def pair_error(error):
            """Handler when pairing failed.

            @param error: one of errors defined in org.bluez.Error representing
                          the error in pairing.

            """
            try:
                error_name = error.get_dbus_name()
                if error_name == 'org.freedesktop.DBus.Error.NoReply':
                    logging.error('Timed out after %d ms. Cancelling pairing.',
                                  timeout)
                    device.CancelPairing()
                else:
                    logging.error('Pairing device failed: %s', error)
            finally:
                mainloop.quit()

        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False

        device_path = device.object_path
        logging.info('Device %s is found.', device.object_path)

        self._setup_pairing_agent(pin)
        mainloop = gobject.MainLoop()

        try:
            if not self._is_paired(device):
                logging.info('Device is not paired. Pair and Connect.')
                device.Pair(reply_handler=pair_reply,
                            error_handler=pair_error,
                            timeout=timeout * 1000)
                mainloop.run()
            elif not self._is_connected(device):
                logging.info('Device is already paired. Connect.')
                device.Connect(reply_handler=connect_reply,
                               error_handler=connect_error,
                               timeout=timeout * 1000)
                mainloop.run()
        except Exception as e:
            logging.error('Exception %s in pair_legacy_device', e)
            return False

        return self._is_paired(device) and self._is_connected(device)

    @xmlrpc_server.dbus_safe(False)
    def remove_device_object(self, address):
        """Removes a device object and the pairing information.

        Calls RemoveDevice method to remove remote device
        object and the pairing information.

        @param address: Address of the device to unpair.

        @returns: True on success. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False
        self._adapter.RemoveDevice(device.object_path,
                                   dbus_interface=self.BLUEZ_ADAPTER_IFACE)
        return True

    @xmlrpc_server.dbus_safe(False)
    def connect_device(self, address):
        """Connects a device.

        Connects a device if it is not connected.

        @param address: Address of the device to connect.

        @returns: True on success. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False
        if self._is_connected(device):
            logging.info('Device is already connected')
            return True
        device.Connect()
        return self._is_connected(device)

    @xmlrpc_server.dbus_safe(False)
    def device_is_connected(self, address):
        """Checks if a device is connected.

        @param address: Address of the device to connect.

        @returns: True if device is connected. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False
        return self._is_connected(device)

    @xmlrpc_server.dbus_safe(False)
    def disconnect_device(self, address):
        """Disconnects a device.

        Disconnects a device if it is connected.

        @param address: Address of the device to disconnect.

        @returns: True on success. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False
        if not self._is_connected(device):
            logging.info('Device is not connected')
            return True
        device.Disconnect()
        return not self._is_connected(device)

    @xmlrpc_server.dbus_safe(False)
    def _device_services_resolved(self, device):
        """Checks if services are resolved.

        @param device: An 'org.bluez.Device1' interface to the device.

        @returns: True if device is connected. False otherwise.

        """
        logging.info('device for services resolved: %s', device)
        props = dbus.Interface(device, dbus.PROPERTIES_IFACE)
        resolved = props.Get(self.BLUEZ_DEVICE_IFACE, 'ServicesResolved')
        logging.info('Services resolved = %r', resolved)
        return bool(resolved)

    @xmlrpc_server.dbus_safe(False)
    def device_services_resolved(self, address):
        """Checks if service discovery is complete on a device.

        Checks whether service discovery has been completed..

        @param address: Address of the remote device.

        @returns: True on success. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False

        if not self._is_connected(device):
            logging.info('Device is not connected')
            return False

        return self._device_services_resolved(device)

    def btmon_start(self):
        """Start btmon monitoring."""
        self.btmon.start()

    def btmon_stop(self):
        """Stop btmon monitoring."""
        self.btmon.stop()

    def btmon_get(self, search_str, start_str):
        """Get btmon output contents.

        @param search_str: only lines with search_str would be kept.
        @param start_str: all lines before the occurrence of start_str would be
                filtered.

        @returns: the recorded btmon output.

        """
        return self.btmon.get_contents(search_str=search_str,
                                       start_str=start_str)

    def btmon_find(self, pattern_str):
        """Find if a pattern string exists in btmon output.

        @param pattern_str: the pattern string to find.

        @returns: True on success. False otherwise.

        """
        return self.btmon.find(pattern_str)

    def messages_start(self):
        """Start messages monitoring.

        @returns: True if logging started successfully, else False
        """

        try:
            self.messages.StartRecording()
            return True

        except Exception as e:
            logging.error('Failed to start log recording with error: %s', e)

        return False

    def messages_stop(self):
        """Stop messages monitoring.

        @returns: True if logs were successfully gathered since logging started,
                else False
        """
        try:
            self.messages.StopRecording()
            return True

        except Exception as e:
            logging.error('Failed to stop log recording with error: %s', e)

        return False

    def messages_find(self, pattern_str):
        """Find if a pattern string exists in messages output.

        @param pattern_str: the pattern string to find.

        @returns: True on success. False otherwise.

        """
        return self.messages.LogContains(pattern_str)

    @xmlrpc_server.dbus_safe(False)
    def dbus_async_method(self, dbus_method, reply_handler, error_handler,
                          *args):
        """Run an async dbus method.

        @param dbus_method: the dbus async method to invoke.
        @param reply_handler: the reply handler for the dbus method.
        @param error_handler: the error handler for the dbus method.
        @param *args: additional arguments for the dbus method.

        @returns: an empty string '' on success;
                  None if there is no _advertising interface manager; and
                  an error string if the dbus method fails or exception occurs

        """

        def successful_cb():
            """Called when the dbus_method completed successfully."""
            reply_handler()
            self.dbus_cb_msg = ''
            self._dbus_mainloop.quit()

        def error_cb(error):
            """Called when the dbus_method failed."""
            error_handler(error)
            self.dbus_cb_msg = str(error)
            self._dbus_mainloop.quit()

        # Call dbus_method with handlers.
        try:
            dbus_method(*args,
                        reply_handler=successful_cb,
                        error_handler=error_cb)
        except Exception as e:
            logging.error('Exception %s in dbus_async_method ', e)
            return str(e)

        self._dbus_mainloop.run()

        return self.dbus_cb_msg


    def advmon_check_manager_interface_exist(self):
        """Check if AdvertisementMonitorManager1 interface is available.

        @returns: True if Manager interface is available, False otherwise.

        """
        objects = self._bluez.GetManagedObjects(
                dbus_interface=self.BLUEZ_MANAGER_IFACE)
        for _, ifaces in six.iteritems(objects):
            if self.BLUEZ_ADV_MONITOR_MANAGER_IFACE in ifaces:
                return True

        return False


    def advmon_read_supported_types(self):
        """Read the Advertisement Monitor supported monitor types.

        Reads the value of 'SupportedMonitorTypes' property of the
        AdvertisementMonitorManager1 interface on the adapter.

        @returns: the list of the supported monitor types.

        """
        types = self._adapter.Get(self.BLUEZ_ADV_MONITOR_MANAGER_IFACE,
                                  'SupportedMonitorTypes',
                                  dbus_interface=self.DBUS_PROP_IFACE)
        return dbus_util.dbus2primitive(types)

    def advmon_read_supported_features(self):
        """Read the Advertisement Monitor supported features.

        Reads the value of 'SupportedFeatures' property of the
        AdvertisementMonitorManager1 interface on the adapter.

        @returns: the list of the supported features.

        """
        features = self._adapter.Get(self.BLUEZ_ADV_MONITOR_MANAGER_IFACE,
                                     'SupportedFeatures',
                                     dbus_interface=self.DBUS_PROP_IFACE)
        return dbus_util.dbus2primitive(features)

    def advmon_create_app(self):
        """Create an advertisement monitor app.

        @returns: app id, once the app is created.

        """
        return self.advmon_appmgr.create_app()

    def advmon_exit_app(self, app_id):
        """Exit an advertisement monitor app.

        @param app_id: the app id.

        @returns: True on success, False otherwise.

        """
        return self.advmon_appmgr.exit_app(app_id)

    def advmon_kill_app(self, app_id):
        """Kill an advertisement monitor app by sending SIGKILL.

        @param app_id: the app id.

        @returns: True on success, False otherwise.

        """
        return self.advmon_appmgr.kill_app(app_id)

    def advmon_register_app(self, app_id):
        """Register an advertisement monitor app.

        @param app_id: the app id.

        @returns: True on success, False otherwise.

        """
        return self.advmon_appmgr.register_app(app_id)

    def advmon_unregister_app(self, app_id):
        """Unregister an advertisement monitor app.

        @param app_id: the app id.

        @returns: True on success, False otherwise.

        """
        return self.advmon_appmgr.unregister_app(app_id)

    def advmon_add_monitor(self, app_id, monitor_data):
        """Create an Advertisement Monitor object.

        @param app_id: the app id.
        @param monitor_data: the list containing monitor type, RSSI filter
                             values and patterns.

        @returns: monitor id, once the monitor is created, None otherwise.

        """
        return self.advmon_appmgr.add_monitor(app_id, monitor_data)

    def advmon_remove_monitor(self, app_id, monitor_id):
        """Remove the Advertisement Monitor object.

        @param app_id: the app id.
        @param monitor_id: the monitor id.

        @returns: True on success, False otherwise.

        """
        return self.advmon_appmgr.remove_monitor(app_id, monitor_id)

    def advmon_get_event_count(self, app_id, monitor_id, event):
        """Read the count of a particular event on the given monitor.

        @param app_id: the app id.
        @param monitor_id: the monitor id.
        @param event: name of the specific event or 'All' for all events.

        @returns: count of the specific event or dict of counts of all events.

        """
        return self.advmon_appmgr.get_event_count(app_id, monitor_id, event)

    def advmon_reset_event_count(self, app_id, monitor_id, event):
        """Reset the count of a particular event on the given monitor.

        @param app_id: the app id.
        @param monitor_id: the monitor id.
        @param event: name of the specific event or 'All' for all events.

        @returns: True on success, False otherwise.

        """
        return self.advmon_appmgr.reset_event_count(app_id, monitor_id, event)

    def advmon_interleave_scan_logger_start(self):
        """ Start interleave logger recording
        """
        self.advmon_interleave_logger.StartRecording()

    def advmon_interleave_scan_logger_stop(self):
        """ Stop interleave logger recording

        @returns: True if logs were successfully collected,
                  False otherwise.

        """
        return self.advmon_interleave_logger.StopRecording()

    def advmon_interleave_scan_logger_get_records(self):
        """ Get records in previous log collections

        @returns: a list of records, where each item is a record of
                  interleave |state| and the |time| the state starts.
                  |state| could be {'no filter', 'allowlist'}
                  |time| is system time in sec

        """
        return self.advmon_interleave_logger.records

    def advmon_interleave_scan_logger_get_cancel_events(self):
        """ Get cancel events in previous log collections

        @returns: a list of cancel |time| when a interleave cancel event log
                  was found.
                  |time| is system time in sec

        """
        return self.advmon_interleave_logger.cancel_events

    def register_advertisement(self, advertisement_data):
        """Register an advertisement.

        Note that rpc supports only conformable types. Hence, a
        dict about the advertisement is passed as a parameter such
        that the advertisement object could be constructed on the host.

        @param advertisement_data: a dict of the advertisement to register.

        @returns: True on success. False otherwise.

        """
        adv = advertisement.Advertisement(self._system_bus, advertisement_data)
        self.advertisements.append(adv)
        return self.dbus_async_method(
                self._advertising.RegisterAdvertisement,
                # reply handler
                lambda: logging.info('register_advertisement: succeeded.'),
                # error handler
                lambda error: logging.error(
                        'register_advertisement: failed: %s', str(error)),
                # other arguments
                adv.get_path(),
                dbus.Dictionary({}, signature='sv'))

    def unregister_advertisement(self, advertisement_data):
        """Unregister an advertisement.

        Note that to unregister an advertisement, it is required to use
        the same self._advertising interface manager. This is because
        bluez only allows the same sender to invoke UnregisterAdvertisement
        method. Hence, watch out that the bluetoothd is not restarted or
        self.start_bluetoothd() is not executed between the time span that
        an advertisement is registered and unregistered.

        @param advertisement_data: a dict of the advertisements to unregister.

        @returns: True on success. False otherwise.

        """
        path = advertisement_data.get('Path')
        for index, adv in enumerate(self.advertisements):
            if adv.get_path() == path:
                break
        else:
            logging.error('Fail to find the advertisement under the path: %s',
                          path)
            return False

        result = self.dbus_async_method(
                self._advertising.UnregisterAdvertisement,
                # reply handler
                lambda: logging.info('unregister_advertisement: succeeded.'),
                # error handler
                lambda error: logging.error(
                        'unregister_advertisement: failed: %s', str(error)),
                # other arguments
                adv.get_path())

        # Call remove_from_connection() so that the same path could be reused.
        adv.remove_from_connection()
        del self.advertisements[index]

        return result

    def set_advertising_intervals(self, min_adv_interval_ms,
                                  max_adv_interval_ms):
        """Set advertising intervals.

        @param min_adv_interval_ms: the min advertising interval in ms.
        @param max_adv_interval_ms: the max advertising interval in ms.

        @returns: True on success. False otherwise.

        """
        return self.dbus_async_method(
                self._advertising.SetAdvertisingIntervals,
                # reply handler
                lambda: logging.info('set_advertising_intervals: succeeded.'),
                # error handler
                lambda error: logging.error(
                        'set_advertising_intervals: failed: %s', str(error)),
                # other arguments
                dbus.UInt16(min_adv_interval_ms),
                dbus.UInt16(max_adv_interval_ms))

    def reset_advertising(self):
        """Reset advertising.

        This includes un-registering all advertisements, reset advertising
        intervals, and disable advertising.

        @returns: True on success. False otherwise.

        """
        # It is required to execute remove_from_connection() to unregister the
        # object-path handler of each advertisement. In this way, we could
        # register an advertisement with the same path repeatedly.
        for adv in self.advertisements:
            adv.remove_from_connection()
        del self.advertisements[:]

        return self.dbus_async_method(
                self._advertising.ResetAdvertising,
                # reply handler
                lambda: logging.info('reset_advertising: succeeded.'),
                # error handler
                lambda error: logging.error('reset_advertising: failed: %s',
                                            str(error)))

    def create_audio_record_directory(self, audio_record_dir):
        """Create the audio recording directory.

        @param audio_record_dir: the audio recording directory

        @returns: True on success. False otherwise.
        """
        try:
            if not os.path.exists(audio_record_dir):
                os.makedirs(audio_record_dir)
            return True
        except Exception as e:
            logging.error('Failed to create %s on the DUT: %s',
                          audio_record_dir, e)
            return False

    def start_capturing_audio_subprocess(self, audio_data, recording_device):
        """Start capturing audio in a subprocess.

        @param audio_data: the audio test data
        @param recording_device: which device recorded the audio,
                possible values are 'recorded_by_dut' or 'recorded_by_peer'

        @returns: True on success. False otherwise.
        """
        audio_data = json.loads(audio_data)
        return self._cras_test_client.start_capturing_subprocess(
                audio_data[recording_device],
                sample_format=audio_data['format'],
                channels=audio_data['channels'],
                rate=audio_data['rate'],
                duration=audio_data['duration'])

    def stop_capturing_audio_subprocess(self):
        """Stop capturing audio.

        @returns: True on success. False otherwise.
        """
        return self._cras_test_client.stop_capturing_subprocess()

    def _generate_playback_file(self, audio_data):
        """Generate the playback file if it does not exist yet.

        Some audio test files may be large. Generate them on the fly
        to save the storage of the source tree.

        @param audio_data: the audio test data
        """
        if not os.path.exists(audio_data['file']):
            data_format = dict(file_type='raw',
                               sample_format='S16_LE',
                               channel=audio_data['channels'],
                               rate=audio_data['rate'])

            # Make the audio file a bit longer to handle any delay
            # issue in capturing.
            duration = audio_data['duration'] + 3
            audio_test_data_module.GenerateAudioTestData(
                    data_format=data_format,
                    path=audio_data['file'],
                    duration_secs=duration,
                    frequencies=audio_data['frequencies'])
            logging.debug("Raw file generated: %s", audio_data['file'])

    def start_playing_audio_subprocess(self, audio_data):
        """Start playing audio in a subprocess.

        @param audio_data: the audio test data

        @returns: True on success. False otherwise.
        """
        audio_data = json.loads(audio_data)
        self._generate_playback_file(audio_data)
        try:
            return self._cras_test_client.start_playing_subprocess(
                    audio_data['file'],
                    channels=audio_data['channels'],
                    rate=audio_data['rate'],
                    duration=audio_data['duration'])
        except Exception as e:
            logging.error("start_playing_subprocess() failed: %s", str(e))
            return False

    def stop_playing_audio_subprocess(self):
        """Stop playing audio in the subprocess.

        @returns: True on success. False otherwise.
        """
        return self._cras_test_client.stop_playing_subprocess()

    def play_audio(self, audio_data):
        """Play audio.

        It blocks until it has completed playing back the audio.

        @param audio_data: the audio test data

        @returns: True on success. False otherwise.
        """
        audio_data = json.loads(audio_data)
        self._generate_playback_file(audio_data)
        return self._cras_test_client.play(audio_data['file'],
                                           channels=audio_data['channels'],
                                           rate=audio_data['rate'],
                                           duration=audio_data['duration'])

    def check_audio_frames_legitimacy(self, audio_test_data, recording_device,
                                      recorded_file):
        """Get the number of frames in the recorded audio file.

        @param audio_test_data: the audio test data
        @param recording_device: which device recorded the audio,
                possible values are 'recorded_by_dut' or 'recorded_by_peer'
        @param recorded_file: the recorded file name

        @returns: True if audio frames are legitimate.
        """
        if bool(recorded_file):
            recorded_filename = recorded_file
        else:
            audio_test_data = json.loads(audio_test_data)
            recorded_filename = audio_test_data[recording_device]

        if recorded_filename.endswith('.raw'):
            # Make sure that the recorded file does not contain all zeros.
            filesize = os.path.getsize(recorded_filename)
            cmd_str = 'cmp -s -n %d %s /dev/zero' % (filesize,
                                                     recorded_filename)
            try:
                result = subprocess.call(cmd_str.split())
                return result != 0
            except Exception as e:
                logging.error("Failed: %s (%s)", cmd_str, str(e))
                return False
        else:
            # The recorded wav file should not be empty.
            wav_file = check_quality.WaveFile(recorded_filename)
            return wav_file.get_number_frames() > 0

    def convert_audio_sample_rate(self, input_file, out_file, test_data,
                                  new_rate):
        """Convert audio file to new sample rate.

        @param input_file: Path to file to upsample.
        @param out_file: Path to create upsampled file.
        @param test_data: Dictionary with information about file.
        @param new_rate: New rate to upsample file to.

        @returns: True if upsampling succeeded, False otherwise.
        """
        test_data = json.loads(test_data)
        logging.debug('Resampling file {} to new rate {}'.format(
                input_file, new_rate))

        convert_format(input_file,
                       test_data['channels'],
                       test_data['bit_width'],
                       test_data['rate'],
                       out_file,
                       test_data['channels'],
                       test_data['bit_width'],
                       new_rate,
                       1.0,
                       use_src_header=True,
                       use_dst_header=True)

        return os.path.isfile(out_file)

    def trim_wav_file(self,
                      in_file,
                      out_file,
                      new_duration,
                      test_data,
                      tolerance=0.1):
        """Trim long file to desired length.

        Trims audio file to length by cutting out silence from beginning and
        end.

        @param in_file: Path to audio file to be trimmed.
        @param out_file: Path to trimmed audio file to create.
        @param new_duration: A float representing the desired duration of
                the resulting trimmed file.
        @param test_data: Dictionary containing information about the test file.
        @param tolerance: (optional) A float representing the allowable
                difference between trimmed file length and desired duration

        @returns: True if file was trimmed successfully, False otherwise.
        """
        test_data = json.loads(test_data)
        trim_silence_from_wav_file(in_file, out_file, new_duration)
        measured_length = get_file_length(out_file, test_data['channels'],
                                          test_data['bit_width'],
                                          test_data['rate'])
        return abs(measured_length - new_duration) <= tolerance

    def unzip_audio_test_data(self, tar_path, data_dir):
        """Unzip audio test data files.

        @param tar_path: Path to audio test data tarball on DUT.
        @oaram data_dir: Path to directory where to extract test data directory.

        @returns: True if audio test data folder exists, False otherwise.
        """
        logging.debug('Downloading audio test data on DUT')
        # creates path to dir to extract test data to by taking name of the
        # tarball without the extension eg. <dir>/file.ext to data_dir/file/
        audio_test_dir = os.path.join(
                data_dir,
                os.path.split(tar_path)[1].split('.', 1)[0])

        unzip_cmd = 'tar -xf {0} -C {1}'.format(tar_path, data_dir)

        unzip_proc = subprocess.Popen(unzip_cmd.split(),
                                      stdout=subprocess.PIPE,
                                      stderr=subprocess.PIPE)
        _, stderr = unzip_proc.communicate()

        if stderr:
            logging.error('Error occurred in unzipping audio data: {}'.format(
                    str(stderr)))
            return False

        return unzip_proc.returncode == 0 and os.path.isdir(audio_test_dir)

    def convert_raw_to_wav(self, input_file, output_file, test_data):
        """Convert raw audio file to wav file.

        @oaram input_file: the location of the raw file
        @param output_file: the location to place the resulting wav file
        @param test_data: the data for the file being converted

        @returns: True if conversion was successful otherwise false
        """
        test_data = json.loads(test_data)
        convert_raw_file(input_file, test_data['channels'],
                         test_data['bit_width'], test_data['rate'],
                         output_file)

        return os.path.isfile(output_file)

    def get_primary_frequencies(self, audio_test_data, recording_device,
                                recorded_file):
        """Get primary frequencies of the audio test file.

        @param audio_test_data: the audio test data
        @param recording_device: which device recorded the audio,
                possible values are 'recorded_by_dut' or 'recorded_by_peer'
        @param recorded_file: the recorded file name

        @returns: a list of primary frequencies of channels in the audio file
        """
        audio_test_data = json.loads(audio_test_data)

        if bool(recorded_file):
            recorded_filename = recorded_file
        else:
            recorded_filename = audio_test_data[recording_device]

        args = CheckQualityArgsClass(filename=recorded_filename,
                                     rate=audio_test_data['rate'],
                                     channel=audio_test_data['channels'],
                                     bit_width=16)
        raw_data, rate = check_quality.read_audio_file(args)
        checker = check_quality.QualityChecker(raw_data, rate)
        # The highest frequency recorded would be near 24 Khz
        # as the max sample rate is 48000 in our tests.
        # So let's set ignore_high_freq to be 48000.
        checker.do_spectral_analysis(ignore_high_freq=48000,
                                     check_quality=False,
                                     quality_params=None)
        spectra = checker._spectrals
        primary_freq = [
                float(spectra[i][0][0]) if spectra[i] else 0
                for i in range(len(spectra))
        ]
        primary_freq.sort()
        return primary_freq

    def enable_wbs(self, value):
        """Enable or disable wideband speech (wbs) per the value.

        @param value: True to enable wbs.

        @returns: True if the operation succeeds.
        """
        return self._cras_test_client.enable_wbs(value)

    def set_player_playback_status(self, status):
        """Set playback status for the registered media player.

        @param status: playback status in string.

        """
        return self._cras_test_client.set_player_playback_status(status)

    def set_player_position(self, position):
        """Set media position for the registered media player.

        @param position: position in micro seconds.

        """
        return self._cras_test_client.set_player_position(position)

    def set_player_metadata(self, metadata):
        """Set metadata for the registered media player.

        @param metadata: dictionary of media metadata.

        """
        return self._cras_test_client.set_player_metadata(metadata)

    def set_player_length(self, length):
        """Set media length for the registered media player.

        Media length is a part of metadata information. However, without
        specify its type to int64. dbus-python will guess the variant type to
        be int32 by default. Separate it from the metadata function to help
        prepare the data differently.

        @param length: length in micro seconds.

        """
        length_variant = dbus.types.Int64(length, variant_level=1)
        length_dict = dbus.Dictionary({'length': length_variant},
                                      signature='sv')
        return self._cras_test_client.set_player_length(length_dict)

    def select_input_device(self, device_name):
        """Select the audio input device.

        @param device_name: the name of the Bluetooth peer device

        @returns: True if the operation succeeds.
        """
        return self._cras_test_client.select_input_device(device_name)

    @xmlrpc_server.dbus_safe(None)
    def select_output_node(self, node_type):
        """Select the audio output node.

        @param node_type: the node type of the Bluetooth peer device

        @returns: True if the operation succeeds.
        """
        return cras_utils.set_single_selected_output_node(node_type)

    @xmlrpc_server.dbus_safe(None)
    def get_selected_output_device_type(self):
        """Get the selected audio output node type.

        @returns: the node type of the selected output device.
        """
        # Note: should convert the dbus.String to the regular string.
        return str(cras_utils.get_selected_output_device_type())

    def get_gatt_attributes_map(self, address):
        """Return a JSON formatted string of the GATT attributes of a device,
        keyed by UUID
        @param address: a string of the MAC address of the device

        @return: JSON formated string, stored the nested structure of the
        attributes. Each attribute has 'path' and
        ['characteristics' | 'descriptors'], which store their object path and
        children respectively.

        """
        attribute_map = dict()

        device_object_path = self._get_device_path(address)
        objects = self._bluez.GetManagedObjects(
                dbus_interface=self.BLUEZ_MANAGER_IFACE, byte_arrays=False)
        service_map = self._get_service_map(device_object_path, objects)

        servs = dict()
        attribute_map['services'] = servs

        for uuid, path in service_map.items():

            servs[uuid] = dict()
            serv = servs[uuid]

            serv['path'] = path
            serv['characteristics'] = dict()
            chrcs = serv['characteristics']

            chrcs_map = self._get_characteristic_map(path, objects)
            for uuid, path in chrcs_map.items():
                chrcs[uuid] = dict()
                chrc = chrcs[uuid]

                chrc['path'] = path
                chrc['descriptors'] = dict()
                descs = chrc['descriptors']

                descs_map = self._get_descriptor_map(path, objects)

                for uuid, path in descs_map.items():
                    descs[uuid] = dict()
                    desc = descs[uuid]

                    desc['path'] = path

        return json.dumps(attribute_map)

    def _get_gatt_interface(self, uuid, object_path, interface):
        """Get dbus interface by uuid
        @param uuid: a string of uuid
        @param object_path: a string of the object path of the service

        @return: a dbus interface
        """

        return dbus.Interface(
                self._system_bus.get_object(self.BLUEZ_SERVICE_NAME,
                                            object_path), interface)

    def get_gatt_service_property(self, object_path, property_name):
        """Get property from a service attribute
        @param object_path: a string of the object path of the service
        @param property_name: a string of a property, ex: 'Value', 'UUID'

        @return: the property if success,
                 none otherwise

        """
        return self.get_gatt_attribute_property(object_path,
                                                self.BLUEZ_GATT_SERV_IFACE,
                                                property_name)

    def get_gatt_characteristic_property(self, object_path, property_name):
        """Get property from a characteristic attribute
        @param object_path: a string of the object path of the characteristic
        @param property_name: a string of a property, ex: 'Value', 'UUID'

        @return: the property if success,
                 none otherwise

        """
        return self.get_gatt_attribute_property(object_path,
                                                self.BLUEZ_GATT_CHAR_IFACE,
                                                property_name)

    def get_gatt_descriptor_property(self, object_path, property_name):
        """Get property from descriptor attribute
        @param object_path: a string of the object path of the descriptor
        @param property_name: a string of a property, ex: 'Value', 'UUID'

        @return: the property if success,
                 none otherwise

        """
        return self.get_gatt_attribute_property(object_path,
                                                self.BLUEZ_GATT_DESC_IFACE,
                                                property_name)

    @xmlrpc_server.dbus_safe(None)
    def get_gatt_attribute_property(self, object_path, interface,
                                    property_name):
        """Get property from attribute
        @param object_path: a string of the bject path
        @param property_name: a string of a property, ex: 'Value', 'UUID'

        @return: the property if success,
                 none otherwise

        """
        gatt_object = self._system_bus.get_object(self.BLUEZ_SERVICE_NAME,
                                                  object_path)
        prop = self._get_dbus_object_property(gatt_object, interface,
                                              property_name)
        logging.info(prop)
        if isinstance(prop, dbus.ByteArray):
            return _dbus_byte_array_to_b64_string(prop)
        if isinstance(prop, dbus.Boolean):
            return bool(prop)
        if isinstance(prop, dbus.String):
            return str(prop)
        if isinstance(prop, dbus.ObjectPath):
            return str(prop)
        if isinstance(prop, dbus.Array):
            return list(map(str, prop))
        return prop

    @xmlrpc_server.dbus_safe(None)
    def gatt_characteristic_read_value(self, uuid, object_path):
        """Perform method ReadValue on a characteristic attribute
        @param uuid: a string of uuid
        @param object_path: a string of the object path of the characteristic

        @return: base64 string of dbus bytearray
        """

        dbus_interface = self._get_gatt_interface(uuid, object_path,
                                                  self.BLUEZ_GATT_CHAR_IFACE)
        value = dbus_interface.ReadValue(dbus.Dictionary({}, signature='sv'))
        return _dbus_byte_array_to_b64_string(value)

    @xmlrpc_server.dbus_safe(None)
    def gatt_descriptor_read_value(self, uuid, object_path):
        """Perform method ReadValue on a descriptor attribute
        @param uuid: a string of uuid
        @param object_path: a string of the object path of the descriptor

        @return: base64 string of dbus bytearray
        """

        dbus_interface = self._get_gatt_interface(uuid, object_path,
                                                  self.BLUEZ_GATT_DESC_IFACE)
        value = dbus_interface.ReadValue(dbus.Dictionary({}, signature='sv'))
        return _dbus_byte_array_to_b64_string(value)

    @xmlrpc_server.dbus_safe(False)
    def _get_attribute_map(self, object_path, dbus_interface, objects):
        """Gets a map of object paths under an object path.

        Walks the object tree, and returns a map of UUIDs to object paths for
        all resolved gatt object.

        @param object_path: The object path of the attribute to retrieve
            gatt  UUIDs and paths from.
        @param objects: The managed objects.

        @returns: A dictionary of object paths, keyed by UUID.

        """
        attr_map = {}

        if object_path:
            for path, ifaces in six.iteritems(objects):
                if (dbus_interface in ifaces and path.startswith(object_path)):
                    uuid = ifaces[dbus_interface]['UUID'].lower()
                    attr_map[uuid] = path

        else:
            logging.warning('object_path %s is not valid', object_path)

        return attr_map

    def _get_service_map(self, device_path, objects):
        """Gets a map of service paths for a device.

        @param device_path: the object path of the device.
        @param objects: The managed objects.
        """
        return self._get_attribute_map(device_path, self.BLUEZ_GATT_SERV_IFACE,
                                       objects)

    def _get_characteristic_map(self, serv_path, objects):
        """Gets a map of characteristic paths for a service.

        @param serv_path: the object path of the service.
        @param objects: The managed objects.
        """
        return self._get_attribute_map(serv_path, self.BLUEZ_GATT_CHAR_IFACE,
                                       objects)

    def _get_descriptor_map(self, chrc_path, objects):
        """Gets a map of descriptor paths for a characteristic.

        @param chrc_path: the object path of the characteristic.
        @param objects: The managed objects.
        """
        return self._get_attribute_map(chrc_path, self.BLUEZ_GATT_DESC_IFACE,
                                       objects)

    @xmlrpc_server.dbus_safe(None)
    def _get_dbus_object_property(self, dbus_object, dbus_interface,
                                  dbus_property):
        """Get the property in an object.

        @param dbus_object: a dbus object
        @param dbus_property: a dbus property of the dbus object, as a string

        @return: dbus type object if it success, e.g. dbus.Boolean, dbus.String,
                 none otherwise

        """
        return dbus_object.Get(dbus_interface,
                               dbus_property,
                               dbus_interface=dbus.PROPERTIES_IFACE)

    @xmlrpc_server.dbus_safe(False)
    def get_characteristic_map(self, address):
        """Gets a map of characteristic paths for a device.

        Walks the object tree, and returns a map of uuids to object paths for
        all resolved gatt characteristics.

        @param address: The MAC address of the device to retrieve
            gatt characteristic uuids and paths from.

        @returns: A dictionary of characteristic paths, keyed by uuid.

        """
        device_path = self._get_device_path(address)
        char_map = {}

        if device_path:
            objects = self._bluez.GetManagedObjects(
                    dbus_interface=self.BLUEZ_MANAGER_IFACE, byte_arrays=False)

            for path, ifaces in six.iteritems(objects):
                if (self.BLUEZ_GATT_CHAR_IFACE in ifaces
                            and path.startswith(device_path)):
                    uuid = ifaces[self.BLUEZ_GATT_CHAR_IFACE]['UUID'].lower()
                    char_map[uuid] = path
        else:
            logging.warning('Device %s not in object tree.', address)

        return char_map

    @xmlrpc_server.dbus_safe(None)
    def _get_char_object(self, uuid, address):
        """Gets a characteristic object.

        Gets a characteristic object for a given UUID and address.

        @param uuid: The UUID of the characteristic, as a string.
        @param address: The MAC address of the remote device.

        @returns: A dbus interface for the characteristic if the uuid/address
                      is in the object tree.
                  None if the address/uuid is not found in the object tree.

        """
        path = self.get_characteristic_map(address).get(uuid)
        if not path:
            logging.error("path not found: %s %s", uuid, address)
            return None
        return dbus.Interface(
                self._system_bus.get_object(self.BLUEZ_SERVICE_NAME, path),
                self.BLUEZ_GATT_CHAR_IFACE)

    @xmlrpc_server.dbus_safe(None)
    def read_characteristic(self, uuid, address):
        """Reads the value of a gatt characteristic.

        Reads the current value of a gatt characteristic. Base64 endcoding is
        used for compatibility with the XML RPC interface.

        @param uuid: The uuid of the characteristic to read, as a string.
        @param address: The MAC address of the remote device.

        @returns: A b64 encoded version of a byte array containing the value
                      if the uuid/address is in the object tree.
                  None if the uuid/address was not found in the object tree, or
                      if a DBus exception was raised by the read operation.

        """
        char_obj = self._get_char_object(uuid, address)
        if char_obj is None:
            return None
        value = char_obj.ReadValue(dbus.Dictionary({}, signature='sv'))
        return _dbus_byte_array_to_b64_string(value)

    @xmlrpc_server.dbus_safe(None)
    def write_characteristic(self, uuid, address, value):
        """Performs a write operation on a gatt characteristic.

        Writes to a GATT characteristic on a remote device. Base64 endcoding is
        used for compatibility with the XML RPC interface.

        @param uuid: The uuid of the characteristic to write to, as a string.
        @param address: The MAC address of the remote device, as a string.
        @param value: A byte array containing the data to write.

        @returns: True if the write operation does not raise an exception.
                  None if the uuid/address was not found in the object tree, or
                      if a DBus exception was raised by the write operation.

        """
        char_obj = self._get_char_object(uuid, address)
        if char_obj is None:
            return None
        dbus_value = _b64_string_to_dbus_byte_array(value)
        char_obj.WriteValue(dbus_value, dbus.Dictionary({}, signature='sv'))
        return True

    @xmlrpc_server.dbus_safe(None)
    def exchange_messages(self, tx_object_path, rx_object_path, value):
        """Performs a write operation on a gatt characteristic and wait for
        the response on another characteristic.

        @param tx_object_path: the object path of the characteristic to write.
        @param rx_object_path: the object path of the characteristic ti read.
        @param value: A byte array containing the data to write.

        @returns: The value of the characteristic to read from.
                  None if the uuid/address was not found in the object tree, or
                      if a DBus exception was raised by the write operation.

        """
        tx_obj = self._get_gatt_characteristic_object(tx_object_path)

        if tx_obj is None:
            return None

        self._chrc_property = None

        self._signal_watch = self._system_bus.add_signal_receiver(
                self._property_changed,
                signal_name='PropertiesChanged',
                path=rx_object_path)

        self._timeout_id = gobject.timeout_add(
                self.PROPERTY_UPDATE_TIMEOUT_MILLI_SECS,
                self._property_wait_timeout)

        write_value = _b64_string_to_dbus_byte_array(value)
        tx_obj.WriteValue(write_value, dbus.Dictionary({}, signature='sv'))

        self._dbus_mainloop.run()

        return _dbus_byte_array_to_b64_string(self._chrc_property)

    def _property_changed(self, *args, **kwargs):
        """Handler for properties changed signal."""
        gobject.source_remove(self._timeout_id)
        self._signal_watch.remove()
        changed_prop = args

        logging.info(changed_prop)
        prop_dict = changed_prop[1]
        self._chrc_property = prop_dict['Value']
        if self._dbus_mainloop.is_running():
            self._dbus_mainloop.quit()

    def _property_wait_timeout(self):
        """Timeout handler when waiting for properties update signal."""
        self._signal_watch.remove()
        if self._dbus_mainloop.is_running():
            logging.warn("quit main loop due to timeout")
            self._dbus_mainloop.quit()
        # Return false so that this method will not be called again.
        return False

    @xmlrpc_server.dbus_safe(False)
    def _get_gatt_characteristic_object(self, object_path):
        return dbus.Interface(
                self._system_bus.get_object(self.BLUEZ_SERVICE_NAME,
                                            object_path),
                self.BLUEZ_GATT_CHAR_IFACE)

    @xmlrpc_server.dbus_safe(False)
    def start_notify(self, object_path, cccd_value):
        """Starts the notification session on the gatt characteristic.

        @param object_path: the object path of the characteristic.
        @param cccd_value: Possible CCCD values include
               0x00 - inferred from the remote characteristic's properties
               0x01 - notification
               0x02 - indication

        @returns: True if the operation succeeds.
                  False if the characteristic is not found, or
                      if a DBus exception was raised by the operation.

        """
        char_obj = self._get_gatt_characteristic_object(object_path)
        if char_obj is None:
            logging.error("characteristic not found: %s %s", object_path)
            return False

        try:
            char_obj.StartNotify(dbus.Byte(cccd_value))
            return True
        except Exception as e:
            logging.error('start_notify: %s', e)
        except:
            logging.error('start_notify: unexpected error')
        return False

    @xmlrpc_server.dbus_safe(False)
    def stop_notify(self, object_path):
        """Stops the notification session on the gatt characteristic.

        @param object_path: the object path of the characteristic.

        @returns: True if the operation succeeds.
                  False if the characteristic is not found, or
                      if a DBus exception was raised by the operation.

        """
        char_obj = self._get_gatt_characteristic_object(object_path)
        if char_obj is None:
            logging.error("characteristic not found: %s %s", object_path)
            return False

        try:
            char_obj.StopNotify()
            return True
        except Exception as e:
            logging.error('stop_notify: %s', e)
        except:
            logging.error('stop_notify: unexpected error')
        return False

    @xmlrpc_server.dbus_safe(False)
    def is_notifying(self, object_path):
        """Is the GATT characteristic in a notifying session?

        @param object_path: the object path of the characteristic.

        @return True if it is in a notification session. False otherwise.

        """

        return self.get_gatt_characteristic_property(object_path, 'Notifying')

    @xmlrpc_server.dbus_safe(False)
    def is_characteristic_path_resolved(self, uuid, address):
        """Checks whether a characteristic is in the object tree.

        Checks whether a characteristic is curently found in the object tree.

        @param uuid: The uuid of the characteristic to search for.
        @param address: The MAC address of the device on which to search for
            the characteristic.

        @returns: True if the characteristic is found.
                  False if the characteristic path is not found.

        """
        return bool(self.get_characteristic_map(address).get(uuid))

    @xmlrpc_server.dbus_safe(False)
    def get_connection_info(self, address):
        """Get device connection info.

        @param address: The MAC address of the device.

        @returns: On success, a JSON-encoded tuple of:
                      ( RSSI, transmit_power, max_transmit_power )
                  None otherwise.

        """
        plugin_device = self._get_plugin_device_interface(address)
        if plugin_device is None:
            return None

        try:
            connection_info = plugin_device.GetConnInfo()
            return json.dumps(connection_info)
        except Exception as e:
            logging.error('get_connection_info: %s', e)
        except:
            logging.error('get_connection_info: unexpected error')
        return None

    @xmlrpc_server.dbus_safe(False)
    def set_le_connection_parameters(self, address, parameters):
        """Set the LE connection parameters.

        @param address: The MAC address of the device.
        @param parameters: The LE connection parameters to set.

        @return: True on success. False otherwise.

        """
        plugin_device = self._get_plugin_device_interface(address)
        if plugin_device is None:
            return False

        return not self.dbus_async_method(
                plugin_device.SetLEConnectionParameters,
                # reply handler
                lambda: logging.info('set_le_connection_parameters: succeeded.'
                                     ),
                # error handler
                lambda error: logging.
                error('set_le_connection_parameters: failed: %s', str(error)),
                # other arguments
                parameters)

    @xmlrpc_server.dbus_safe(False)
    def _get_plugin_device_interface(self, address):
        """Get the BlueZ Chromium device plugin interface.

        This interface can be used to issue dbus requests such as
        GetConnInfo and SetLEConnectionParameters.

        @param address: The MAC address of the device.

        @return: On success, the BlueZ Chromium device plugin interface
                 None otherwise.

        """
        path = self._get_device_path(address)
        if path is None:
            return None

        return dbus.Interface(
                self._system_bus.get_object(self.BLUEZ_SERVICE_NAME, path),
                self.BLUEZ_PLUGIN_DEVICE_IFACE)

    def _powerd_last_resume_details(self, before=5, after=0):
        """ Look at powerd logs for last suspend/resume attempt.

        Note that logs are in reverse order (chronologically). Keep that in mind
        for the 'before' and 'after' parameters.

        @param before: Number of context lines before search item to show.
        @param after: Number of context lines after search item to show.

        @return Most recent lines containing suspend resume details or ''.
        """
        event_file = '/var/log/power_manager/powerd.LATEST'

        # Each powerd_suspend wakeup has a log "powerd_suspend returned 0",
        # with the return code of the suspend. We search for the last
        # occurrence in the log, and then find the collocated event_count log,
        # indicating the wakeup cause. -B option for grep will actually grab the
        # *next* 5 logs in time, since we are piping the powerd file backwards
        # with tac command
        resume_indicator = 'powerd_suspend returned'
        cmd = 'tac {} | grep -A {} -B {} -m1 "{}"'.format(
                event_file, after, before, resume_indicator)

        try:
            return utils.run(cmd).stdout
        except error.CmdError:
            logging.error('Could not locate recent suspend')

        return ''

    def bt_caused_last_resume(self):
        """Checks if last resume from suspend was caused by bluetooth

        @return: True if BT wake path was cause of resume, False otherwise
        """

        # When the resume cause is printed to powerd log, it omits the
        # /power/wakeup portion of wake path
        bt_wake_path = self._get_wake_enabled_path()

        # If bluetooth does not have a valid wake path, it could not have caused
        # the resume
        if not bt_wake_path:
            return False

        bt_wake_path = bt_wake_path.replace('/power/wakeup', '')

        last_resume_details = self._powerd_last_resume_details()

        # If BT caused wake, there will be a line describing the bt wake
        # path's event_count before and after the resume
        for line in last_resume_details.split('\n'):
            if 'event_count' in line:
                logging.info('Checking wake event: {}'.format(line))
                if bt_wake_path in line:
                    return True

        return False

    def find_last_suspend_via_powerd_logs(self):
        """ Finds the last suspend attempt via powerd logs.

        Finds the last suspend attempt using powerd logs by searching backwards
        through the logs to find the latest entries with 'powerd_suspend'. If we
        can't find a suspend attempt, we return None.

        @return: Tuple (suspend start time, suspend end time, suspend result) or
                None if we can't find a suspend attempt
        """
        # Logs look like this:
        # [1102/202036.973853:INFO:daemon.cc(704)] powerd_suspend returned 0
        # ... stuff in between ...
        # [1102/202025.785372:INFO:suspender.cc(574)] Starting suspend

        # Date format for strptime and strftime
        date_format = '%m%d/%H%M%S.%f'
        date_group_re = '(?P<date>[0-9]+/[0-9]+[.][0-9]+)'

        finish_suspend_re = re.compile(
                '^\\[{date_regex}'
                '.*daemon.*powerd_suspend returned '
                '(?P<exitcode>[0-9]+)'.format(date_regex=date_group_re))
        start_suspend_re = re.compile(
                '^\\[{date_regex}.*suspender.*'
                'Starting suspend'.format(date_regex=date_group_re))

        now = datetime.now()
        last_resume_details = self._powerd_last_resume_details(before=0,
                                                               after=8)
        if last_resume_details:
            start_time, end_time, ret = None, None, None
            try:
                for line in last_resume_details.split('\n'):
                    logging.debug('Last suspend search: %s', line)
                    m = finish_suspend_re.match(line)
                    if m:
                        logging.debug('Found suspend end: date(%s) ret(%s)',
                                      m.group('date'), m.group('exitcode'))
                        end_time = datetime.strptime(
                                m.group('date'),
                                date_format).replace(year=now.year)
                        ret = int(m.group('exitcode'))

                    m = start_suspend_re.match(line)
                    if m:
                        logging.debug('Found suspend start: date(%s)',
                                      m.group('date'))
                        start_time = datetime.strptime(
                                m.group('date'),
                                date_format).replace(year=now.year)
                        break

                if all([x is not None for x in [start_time, end_time, ret]]):
                    # Return dates in string format due to inconsistency between
                    # python2/3 usage on host and dut
                    return (start_time.strftime(self.OUT_DATE_FORMAT),
                            end_time.strftime(self.OUT_DATE_FORMAT), ret)
                else:
                    logging.error(
                            'Failed to parse details from last suspend. %s %s %s',
                            str(start_time), str(end_time), str(ret))
            except Exception as e:
                logging.error('Failed to parse last suspend: %s', str(e))
        else:
            logging.error('No powerd_suspend attempt found')

        return None

    def do_suspend(self, seconds, expect_bt_wake):
        """Suspend DUT using the power manager.

        @param seconds: The number of seconds to suspend the device.
        @param expect_bt_wake: Whether we expect bluetooth to wake us from
            suspend. If true, we expect this resume will occur early

        @throws: SuspendFailure on resume with unexpected timing or wake source.
            The raised exception will be handled as a non-zero retcode over the
            RPC, signalling for the test to fail.
        """
        early_wake = False
        try:
            sys_power.do_suspend(seconds)

        except sys_power.SpuriousWakeupError:
            logging.info('Early resume detected...')
            early_wake = True

        # Handle error conditions based on test expectations, whether resume
        # was early, and cause of the resume
        bt_caused_wake = self.bt_caused_last_resume()
        logging.info('Cause for resume: {}'.format(
                'BT' if bt_caused_wake else 'Not BT'))

        if not expect_bt_wake and bt_caused_wake:
            raise sys_power.SuspendFailure('BT woke us unexpectedly')

        # TODO(b/160803597) - Uncomment when BT wake reason is correctly
        # captured in powerd log.
        #
        # if expect_bt_wake and not bt_caused_wake:
        #   raise sys_power.SuspendFailure('BT should have woken us')
        #
        # if bt_caused_wake and not early_wake:
        #   raise sys_power.SuspendFailure('BT wake did not come early')

        return True

    def get_wlan_vid_pid(self):
        """ Return vendor id and product id of the wlan chip on BT/WiFi module

        @returns: (vid,pid) on success; (None,None) on failure
        """
        vid = None
        pid = None
        path_template = '/sys/class/net/%s/device/'
        for dev_name in ['wlan0', 'mlan0']:
            if os.path.exists(path_template % dev_name):
                path_v = path_template % dev_name + 'vendor'
                path_d = path_template % dev_name + 'device'
                logging.debug('Paths are %s %s', path_v, path_d)
                try:
                    vid = open(path_v).read().strip('\n')
                    pid = open(path_d).read().strip('\n')
                    break
                except Exception as e:
                    logging.error('Exception %s while reading vid/pid', str(e))
        logging.debug('returning vid:%s pid:%s', vid, pid)
        return (vid, pid)

    def get_bt_module_name(self):
        """ Return bluetooth module name for non-USB devices

        @returns '' on failure. On success return chipset name, if found in
                 dict.Otherwise it returns the raw string read.
        """
        # map the string read from device to chipset name
        chipset_string_dict = {'qcom,wcn3991-bt\x00': 'WCN3991'}

        hci_device = '/sys/class/bluetooth/hci0'
        real_path = os.path.realpath(hci_device)

        logging.debug('real path is %s', real_path)
        if 'usb' in real_path:
            return ''

        device_path = os.path.join(real_path, 'device', 'of_node',
                                   'compatible')
        try:
            chipset_string = open(device_path).read()
            logging.debug('read string %s from %s', chipset_string,
                          device_path)
        except Exception as e:
            logging.error('Exception %s while reading from file', str(e),
                          device_path)
            return ''

        if chipset_string in chipset_string_dict:
            return chipset_string_dict[chipset_string]
        else:
            logging.debug("Chipset not known. Returning %s", chipset_string)
            return chipset_string

    def get_device_time(self):
        """ Get the current device time. """
        return datetime.now().strftime(self.OUT_DATE_FORMAT)

    def cleanup(self):
        """Cleanup before exiting the client xmlrpc process."""

        self.advmon_appmgr.destroy()
