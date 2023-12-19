# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import absolute_import

import base64
import functools
import json
import logging
import threading
from datetime import datetime

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.cros import constants
from autotest_lib.server import autotest

def proxy_thread_safe(method):
    """A decorator enabling thread-safe XmlRpc calls"""

    @functools.wraps(method)
    def wrapper(self, *args, **kwargs):
        """A wrapper of the decorated method"""
        with self._proxy_lock:
            return method(self, *args, **kwargs)

    return wrapper


class BluetoothDevice(object):
    """BluetoothDevice is a thin layer of logic over a remote DUT.

    The Autotest host object representing the remote DUT, passed to this
    class on initialization, can be accessed from its host property.

    """

    XMLRPC_BRINGUP_TIMEOUT_SECONDS = 60
    XMLRPC_LOG_PATH = '/var/log/bluetooth_xmlrpc_device.log'
    XMLRPC_REQUEST_TIMEOUT_SECONDS = 180

    # We currently get dates back in string format due to some inconsistencies
    # between python2 and python3. This is the standard date format we use.
    NATIVE_DATE_FORMAT = '%Y-%m-%d %H:%M:%S.%f'

    def __init__(self, device_host, remote_facade_proxy=None):
        """Construct a BluetoothDevice.

        @param device_host: host object representing a remote host.

        """
        self.host = device_host
        self._remote_proxy = remote_facade_proxy

        # Make sure the client library is on the device so that the proxy code
        # is there when we try to call it.
        client_at = autotest.Autotest(self.host)
        client_at.install()
        self._proxy_lock = threading.Lock()

        # If remote facade wasn't already created, connect directly here
        if not self._remote_proxy:
            self._connect_xmlrpc_directly()

        # Get some static information about the bluetooth adapter.
        properties = self.get_adapter_properties()
        self.bluez_version = properties.get('Name')
        self.address = properties.get('Address')
        self.bluetooth_class = properties.get('Class')
        self.UUIDs = properties.get('UUIDs')

    def _connect_xmlrpc_directly(self):
        """Connects to the bluetooth native facade directly via xmlrpc."""
        proxy = self.host.rpc_server_tracker.xmlrpc_connect(
                constants.BLUETOOTH_DEVICE_XMLRPC_SERVER_COMMAND,
                constants.BLUETOOTH_DEVICE_XMLRPC_SERVER_PORT,
                command_name=constants.
                BLUETOOTH_DEVICE_XMLRPC_SERVER_CLEANUP_PATTERN,
                ready_test_name=constants.
                BLUETOOTH_DEVICE_XMLRPC_SERVER_READY_METHOD,
                timeout_seconds=self.XMLRPC_BRINGUP_TIMEOUT_SECONDS,
                logfile=self.XMLRPC_LOG_PATH,
                request_timeout_seconds=self.XMLRPC_REQUEST_TIMEOUT_SECONDS)

        self._bt_direct_proxy = proxy
        return proxy

    @property
    def _proxy(self):
        """Gets the proxy to the DUT bluetooth facade.

        @return XML RPC proxy to DUT bluetooth facade.

        """
        # When the xmlrpc server is already created (using the
        # RemoteFacadeFactory), we will use the BluetoothNativeFacade inside the
        # remote proxy. Otherwise, we will use the xmlrpc server started from
        # this class. Currently, there are a few users outside of the Bluetooth
        # autotests that use this and this can be removed once those users
        # migrate to using the RemoteFacadeFactory to generate the xmlrpc
        # connection.
        if self._remote_proxy:
            return self._remote_proxy.bluetooth
        else:
            return self._bt_direct_proxy

    @proxy_thread_safe
    def set_debug_log_levels(self, dispatcher_vb, newblue_vb, bluez_vb,
                             kernel_vb):
        """Enable or disable the debug logs of bluetooth

        @param dispatcher_vb: verbosity of btdispatcher debug log, either 0 or 1
        @param newblue_vb: verbosity of newblued debug log, either 0 or 1
        @param bluez_vb: verbosity of bluez debug log, either 0 or 1
        @param kernel_vb: verbosity of kernel debug log, either 0 or 1

        """
        return self._proxy.set_debug_log_levels(dispatcher_vb, newblue_vb,
                                                bluez_vb, kernel_vb)

    @proxy_thread_safe
    def log_message(self, msg, dut=True, peer=True):
        """ Log a message in DUT log and peer logs with timestamp.

        @param msg: message to be logged.
        @param dut: log message on DUT
        @param peer: log message on peer devices
        """
        try:
            # TODO(b/146671469) Implement logging to tester

            date =  datetime.strftime(datetime.now(),"%Y:%m:%d %H:%M:%S:%f")
            msg = "bluetooth autotest --- %s : %s ---" % (date, msg)
            logging.debug("Broadcasting '%s'",msg)

            if dut:
                self._proxy.log_message(msg)

            if peer:
                for btpeer in self.host.btpeer_list:
                    btpeer.log_message(msg)
        except Exception as e:
            logging.error("Exception '%s' in log_message '%s'", str(e), msg)


    @proxy_thread_safe
    def is_wrt_supported(self):
        """ Check if Bluetooth adapter support WRT logs.

        Intel adapter support WRT (except of WP2 and StP2)

        @returns: True if adapter support WRT logs
        """
        return self._proxy.is_wrt_supported()


    @proxy_thread_safe
    def enable_wrt_logs(self):
        """Enable wrt logs on Intel adapters."""
        return self._proxy.enable_wrt_logs()


    @proxy_thread_safe
    def collect_wrt_logs(self):
        """Collect wrt logs on Intel adapters."""
        return self._proxy.collect_wrt_logs()


    @proxy_thread_safe
    def start_bluetoothd(self):
        """start bluetoothd.

        @returns: True if bluetoothd is started correctly.
                  False otherwise.

        """
        return self._proxy.start_bluetoothd()


    @proxy_thread_safe
    def stop_bluetoothd(self):
        """stop bluetoothd.

        @returns: True if bluetoothd is stopped correctly.
                  False otherwise.

        """
        return self._proxy.stop_bluetoothd()


    @proxy_thread_safe
    def is_bluetoothd_running(self):
        """Is bluetoothd running?

        @returns: True if bluetoothd is running

        """
        return self._proxy.is_bluetoothd_running()


    @proxy_thread_safe
    def is_bluetoothd_valid(self):
        """Checks whether the current bluetoothd session is ok.

        Returns:
            True if the current bluetoothd session is ok. False if bluetoothd is
            not running or it is a new session.
        """
        return self._proxy.is_bluetoothd_proxy_valid()


    @proxy_thread_safe
    def reset_on(self):
        """Reset the adapter and settings and power up the adapter.

        @return True on success, False otherwise.

        """
        return self._proxy.reset_on()


    @proxy_thread_safe
    def reset_off(self):
        """Reset the adapter and settings, leave the adapter powered off.

        @return True on success, False otherwise.

        """
        return self._proxy.reset_off()


    @proxy_thread_safe
    def has_adapter(self):
        """@return True if an adapter is present, False if not."""
        return self._proxy.has_adapter()


    @proxy_thread_safe
    def is_wake_enabled(self):
        """@return True if adapter is wake enabled, False if not."""
        return self._proxy.is_wake_enabled()


    @proxy_thread_safe
    def set_wake_enabled(self, value):
        """ Sets the power/wakeup value for the adapter.

        Args:
            value: Whether the adapter can wake from suspend

        @return True if able to set it to value, False if not."""
        return self._proxy.set_wake_enabled(value)


    @proxy_thread_safe
    def set_powered(self, powered):
        """Set the adapter power state.

        @param powered: adapter power state to set (True or False).

        @return True on success, False otherwise.

        """
        return self._proxy.set_powered(powered)


    def is_powered_on(self):
        """Is the adapter powered on?

        @returns: True if the adapter is powered on

        """
        properties = self.get_adapter_properties()
        return bool(properties.get(u'Powered'))


    def get_hci(self):
        """Get hci of the adapter; normally, it is 'hci0'.

        @returns: the hci name of the adapter.

        """
        dev_info = self.get_dev_info()
        hci = (dev_info[1] if isinstance(dev_info, list) and
               len(dev_info) > 1 else None)
        return hci


    def get_address(self):
        """Get the bluetooth address of the adapter.

        An example of the bluetooth address of the adapter: '6C:29:95:1A:D4:6F'

        @returns: the bluetooth address of the adapter.

        """
        return self.address


    def get_bluez_version(self):
        """Get bluez version.

        An exmaple of bluez version: 'BlueZ 5.39'

        @returns: the bluez version

        """
        return self.bluez_version


    def get_bluetooth_class(self):
        """Get the bluetooth class of the adapter.

        An example of the bluetooth class of a chromebook: 4718852

        @returns: the bluetooth class.

        """
        return self.bluetooth_class


    def get_UUIDs(self):
        """Get the UUIDs.

        An example of UUIDs:
            [u'00001112-0000-1000-8000-00805f9b34fb',
             u'00001801-0000-1000-8000-00805f9b34fb',
             u'0000110a-0000-1000-8000-00805f9b34fb',
             u'0000111f-0000-1000-8000-00805f9b34fb',
             u'00001200-0000-1000-8000-00805f9b34fb',
             u'00001800-0000-1000-8000-00805f9b34fb']

        @returns: the list of the UUIDs.

        """
        return self.UUIDs


    @proxy_thread_safe
    def set_discoverable(self, discoverable):
        """Set the adapter discoverable state.

        @param discoverable: adapter discoverable state to set (True or False).

        @return True on success, False otherwise.

        """
        return self._proxy.set_discoverable(discoverable)


    def is_discoverable(self):
        """Is the adapter in the discoverable state?

        @return True if discoverable. False otherwise.

        """
        properties = self.get_adapter_properties()
        return properties.get('Discoverable') == 1


    @proxy_thread_safe
    def set_discoverable_timeout(self, discoverable_timeout):
        """Set the adapter DiscoverableTimeout.

        @param discoverable_timeout: adapter DiscoverableTimeout
                value to set in seconds (Integer).

        @return True on success, False otherwise.

        """
        return self._proxy.set_discoverable_timeout(discoverable_timeout)


    @proxy_thread_safe
    def get_discoverable_timeout(self):
        """Get the adapter DiscoverableTimeout.

        @return Value of property DiscoverableTimeout in seconds (Integer).

        """
        return self._proxy.get_discoverable_timeout()


    @proxy_thread_safe
    def set_pairable_timeout(self, pairable_timeout):
        """Set the adapter PairableTimeout.

        @param pairable_timeout: adapter PairableTimeout
                value to set in seconds (Integer).

        @return True on success, False otherwise.

        """
        return self._proxy.set_pairable_timeout(pairable_timeout)


    @proxy_thread_safe
    def get_pairable_timeout(self):
        """Get the adapter PairableTimeout.

        @return Value of property PairableTimeout in seconds (Integer).

        """
        return self._proxy.get_pairable_timeout()


    @proxy_thread_safe
    def set_pairable(self, pairable):
        """Set the adapter pairable state.

        @param pairable: adapter pairable state to set (True or False).

        @return True on success, False otherwise.

        """
        return self._proxy.set_pairable(pairable)


    def is_pairable(self):
        """Is the adapter in the pairable state?

        @return True if pairable. False otherwise.

        """
        properties = self.get_adapter_properties()
        return properties.get('Pairable') == 1

    @proxy_thread_safe
    def set_adapter_alias(self, alias):
        """Set the adapter alias.

        A note on Alias property - providing an empty string ('') will reset the
        Alias property to the system default

        @param alias: adapter alias to set with type String

        @return True on success, False otherwise.
        """

        return self._proxy.set_adapter_alias(alias)

    @proxy_thread_safe
    def get_adapter_properties(self):
        """Read the adapter properties from the Bluetooth Daemon.

        An example of the adapter properties looks like
        {u'Name': u'BlueZ 5.35',
         u'Alias': u'Chromebook',
         u'Modalias': u'bluetooth:v00E0p2436d0400',
         u'Powered': 1,
         u'DiscoverableTimeout': 180,
         u'PairableTimeout': 0,
         u'Discoverable': 0,
         u'Address': u'6C:29:95:1A:D4:6F',
         u'Discovering': 0,
         u'Pairable': 1,
         u'Class': 4718852,
         u'UUIDs': [u'00001112-0000-1000-8000-00805f9b34fb',
                    u'00001801-0000-1000-8000-00805f9b34fb',
                    u'0000110a-0000-1000-8000-00805f9b34fb',
                    u'0000111f-0000-1000-8000-00805f9b34fb',
                    u'00001200-0000-1000-8000-00805f9b34fb',
                    u'00001800-0000-1000-8000-00805f9b34fb']}

        @return the properties as a dictionary on success,
            the value False otherwise.

        """
        return json.loads(self._proxy.get_adapter_properties())


    @proxy_thread_safe
    def read_version(self):
        """Read the version of the management interface from the Kernel.

        @return the version as a tuple of:
          ( version, revision )

        """
        return json.loads(self._proxy.read_version())


    @proxy_thread_safe
    def read_supported_commands(self):
        """Read the set of supported commands from the Kernel.

        @return set of supported commands as arrays in a tuple of:
          ( commands, events )

        """
        return json.loads(self._proxy.read_supported_commands())


    @proxy_thread_safe
    def read_index_list(self):
        """Read the list of currently known controllers from the Kernel.

        @return array of controller indexes.

        """
        return json.loads(self._proxy.read_index_list())


    @proxy_thread_safe
    def read_info(self):
        """Read the adapter information from the Kernel.

        An example of the adapter information looks like
        [u'6C:29:95:1A:D4:6F', 6, 2, 65535, 2769, 4718852, u'Chromebook', u'']

        @return the information as a tuple of:
          ( address, bluetooth_version, manufacturer_id,
            supported_settings, current_settings, class_of_device,
            name, short_name )

        """
        return json.loads(self._proxy.read_info())


    @proxy_thread_safe
    def add_device(self, address, address_type, action):
        """Add a device to the Kernel action list.

        @param address: Address of the device to add.
        @param address_type: Type of device in @address.
        @param action: Action to take.

        @return tuple of ( address, address_type ) on success,
          None on failure.

        """
        return json.loads(self._proxy.add_device(address, address_type, action))


    @proxy_thread_safe
    def remove_device(self, address, address_type):
        """Remove a device from the Kernel action list.

        @param address: Address of the device to remove.
        @param address_type: Type of device in @address.

        @return tuple of ( address, address_type ) on success,
          None on failure.

        """
        return json.loads(self._proxy.remove_device(address, address_type))

    def _decode_json_base64(self, data):
        """Load serialized JSON and then base64 decode it

        Required to handle non-ascii data
        @param data: data to be JSON and base64 decode

        @return : JSON and base64 decoded date


        """
        logging.debug("_decode_json_base64 raw data is %s", data)
        json_encoded = json.loads(data)
        logging.debug("JSON encoded data is %s", json_encoded)
        base64_decoded = utils.base64_recursive_decode(json_encoded)
        logging.debug("base64 decoded data is %s", base64_decoded)
        return base64_decoded


    @proxy_thread_safe
    def get_devices(self):
        """Read information about remote devices known to the adapter.

        An example of the device information of RN-42 looks like
        [{u'Name': u'RNBT-A96F',
          u'Alias': u'RNBT-A96F',
          u'Adapter': u'/org/bluez/hci0',
          u'LegacyPairing': 0,
          u'Paired': 1,
          u'Connected': 0,
          u'UUIDs': [u'00001124-0000-1000-8000-00805f9b34fb'],
          u'Address': u'00:06:66:75:A9:6F',
          u'Icon': u'input-mouse',
          u'Class': 1408,
          u'Trusted': 1,
          u'Blocked': 0}]

        @return the properties of each device as an array of
            dictionaries on success, the value False otherwise.

        """
        encoded_devices = self._proxy.get_devices()
        return self._decode_json_base64(encoded_devices)


    @proxy_thread_safe
    def get_device_property(self, address, prop_name):
        """Read a property of BT device by directly querying device dbus object

        @param address: Address of the device to query
        @param prop_name: Property to be queried

        @return The property if device is found and has property, None otherwise
        """

        prop_val = self._proxy.get_device_property(address, prop_name)

        # Handle dbus error case returned by xmlrpc_server.dbus_safe decorator
        if prop_val is None:
            return prop_val

        # Decode and return property value
        return self._decode_json_base64(prop_val)


    @proxy_thread_safe
    def get_battery_property(self, address, prop_name):
        """Read a property of battery by directly querying the dbus object

        @param address: Address of the device to query
        @param prop_name: Property to be queried

        @return The property if battery is found and has property,
          None otherwise
        """

        return self._proxy.get_battery_property(address, prop_name)

    @proxy_thread_safe
    def start_discovery(self):
        """Start discovery of remote devices.

        Obtain the discovered device information using get_devices(), called
        stop_discovery() when done.

        @return (True, None) on success, (False, <error>) otherwise.

        """
        return self._proxy.start_discovery()


    @proxy_thread_safe
    def stop_discovery(self):
        """Stop discovery of remote devices.

        @return (True, None) on success, (False, <error>) otherwise.

        """
        return self._proxy.stop_discovery()


    def is_discovering(self):
        """Is it discovering?

        @return True if it is discovering. False otherwise.

        """
        return self.get_adapter_properties().get('Discovering') == 1


    @proxy_thread_safe
    def get_dev_info(self):
        """Read raw HCI device information.

        An example of the device information looks like:
        [0, u'hci0', u'6C:29:95:1A:D4:6F', 13, 0, 1, 581900950526, 52472, 7,
         32768, 1021, 5, 96, 6, 0, 0, 151, 151, 0, 0, 0, 0, 1968, 12507]

        @return tuple of (index, name, address, flags, device_type, bus_type,
                       features, pkt_type, link_policy, link_mode,
                       acl_mtu, acl_pkts, sco_mtu, sco_pkts,
                       err_rx, err_tx, cmd_tx, evt_rx, acl_tx, acl_rx,
                       sco_tx, sco_rx, byte_rx, byte_tx) on success,
                None on failure.

        """
        return json.loads(self._proxy.get_dev_info())


    @proxy_thread_safe
    def get_supported_capabilities(self):
        """ Get the supported_capabilities of the adapter
        @returns (capabilities,None) on success (None, <error>) on failure
        """
        capabilities, error = self._proxy.get_supported_capabilities()
        return (json.loads(capabilities), error)


    @proxy_thread_safe
    def register_profile(self, path, uuid, options):
        """Register new profile (service).

        @param path: Path to the profile object.
        @param uuid: Service Class ID of the service as string.
        @param options: Dictionary of options for the new service, compliant
                        with BlueZ D-Bus Profile API standard.

        @return True on success, False otherwise.

        """
        return self._proxy.register_profile(path, uuid, options)


    @proxy_thread_safe
    def has_device(self, address):
        """Checks if the device with a given address exists.

        @param address: Address of the device.

        @returns: True if there is a device with that address.
                  False otherwise.

        """
        return self._proxy.has_device(address)


    @proxy_thread_safe
    def device_is_paired(self, address):
        """Checks if a device is paired.

        @param address: address of the device.

        @returns: True if device is paired. False otherwise.

        """
        return self._proxy.device_is_paired(address)


    @proxy_thread_safe
    def device_services_resolved(self, address):
        """Checks if services are resolved for a device.

        @param address: address of the device.

        @returns: True if services are resolved. False otherwise.

        """
        return self._proxy.device_services_resolved(address)


    @proxy_thread_safe
    def set_trusted(self, address, trusted=True):
        """Set the device trusted.

        @param address: The bluetooth address of the device.
        @param trusted: True or False indicating whether to set trusted or not.

        @returns: True if successful. False otherwise.

        """
        return self._proxy.set_trusted(address, trusted)


    @proxy_thread_safe
    def pair_legacy_device(self, address, pin, trusted, timeout):
        """Pairs a device with a given pin code.

        Registers an agent who handles pin code request and
        pairs a device with known pin code.

        @param address: Address of the device to pair.
        @param pin: The pin code of the device to pair.
        @param trusted: indicating whether to set the device trusted.
        @param timeout: The timeout in seconds for pairing.

        @returns: True on success. False otherwise.

        """
        return self._proxy.pair_legacy_device(address, pin, trusted, timeout)


    @proxy_thread_safe
    def remove_device_object(self, address):
        """Removes a device object and the pairing information.

        Calls RemoveDevice method to remove remote device
        object and the pairing information.

        @param address: address of the device to unpair.

        @returns: True on success. False otherwise.

        """
        return self._proxy.remove_device_object(address)


    @proxy_thread_safe
    def connect_device(self, address):
        """Connects a device.

        Connects a device if it is not connected.

        @param address: Address of the device to connect.

        @returns: True on success. False otherwise.

        """
        return self._proxy.connect_device(address)


    @proxy_thread_safe
    def device_is_connected(self, address):
        """Checks if a device is connected.

        @param address: Address of the device to check if it is connected.

        @returns: True if device is connected. False otherwise.

        """
        return self._proxy.device_is_connected(address)


    @proxy_thread_safe
    def disconnect_device(self, address):
        """Disconnects a device.

        Disconnects a device if it is connected.

        @param address: Address of the device to disconnect.

        @returns: True on success. False otherwise.

        """
        return self._proxy.disconnect_device(address)


    @proxy_thread_safe
    def btmon_start(self):
        """Start btmon monitoring."""
        self._proxy.btmon_start()


    @proxy_thread_safe
    def btmon_stop(self):
        """Stop btmon monitoring."""
        self._proxy.btmon_stop()


    @proxy_thread_safe
    def btmon_get(self, search_str='', start_str=''):
        """Get btmon output contents.

        @param search_str: only lines with search_str would be kept.
        @param start_str: all lines before the occurrence of start_str would be
                filtered.

        @returns: the recorded btmon output.

        """
        return self._proxy.btmon_get(search_str, start_str)


    @proxy_thread_safe
    def btmon_find(self, pattern_str):
        """Find if a pattern string exists in btmon output.

        @param pattern_str: the pattern string to find.

        @returns: True on success. False otherwise.

        """
        return self._proxy.btmon_find(pattern_str)


    @proxy_thread_safe
    def advmon_check_manager_interface_exist(self):
        """Check if AdvertisementMonitorManager1 interface is available.

        @returns: True if Manager interface is available, False otherwise.

        """
        return self._proxy.advmon_check_manager_interface_exist()


    @proxy_thread_safe
    def advmon_read_supported_types(self):
        """Read the Advertisement Monitor supported monitor types.

        @returns: List of supported advertisement monitor types.

        """
        return self._proxy.advmon_read_supported_types()


    @proxy_thread_safe
    def advmon_read_supported_features(self):
        """Read the Advertisement Monitor supported features.

        @returns: List of supported advertisement monitor features.

        """
        return self._proxy.advmon_read_supported_features()


    @proxy_thread_safe
    def advmon_create_app(self):
        """Create an advertisement monitor app.

        @returns: app id, once the app is created.

        """
        return self._proxy.advmon_create_app()


    @proxy_thread_safe
    def advmon_exit_app(self, app_id):
        """Exit an advertisement monitor app.

        @param app_id: the app id.

        @returns: True on success, False otherwise.

        """
        return self._proxy.advmon_exit_app(app_id)


    @proxy_thread_safe
    def advmon_kill_app(self, app_id):
        """Kill an advertisement monitor app by sending SIGKILL.

        @param app_id: the app id.

        @returns: True on success, False otherwise.

        """
        return self._proxy.advmon_kill_app(app_id)


    @proxy_thread_safe
    def advmon_register_app(self, app_id):
        """Register an advertisement monitor app.

        @param app_id: the app id.

        @returns: True on success, False otherwise.

        """
        return self._proxy.advmon_register_app(app_id)


    @proxy_thread_safe
    def advmon_unregister_app(self, app_id):
        """Unregister an advertisement monitor app.

        @param app_id: the app id.

        @returns: True on success, False otherwise.

        """
        return self._proxy.advmon_unregister_app(app_id)


    @proxy_thread_safe
    def advmon_add_monitor(self, app_id, monitor_data):
        """Create an Advertisement Monitor object.

        @param app_id: the app id.
        @param monitor_data: the list containing monitor type, RSSI filter
                             values and patterns.

        @returns: monitor id, once the monitor is created, None otherwise.

        """
        return self._proxy.advmon_add_monitor(app_id, monitor_data)


    @proxy_thread_safe
    def advmon_remove_monitor(self, app_id, monitor_id):
        """Remove the Advertisement Monitor object.

        @param app_id: the app id.
        @param monitor_id: the monitor id.

        @returns: True on success, False otherwise.

        """
        return self._proxy.advmon_remove_monitor(app_id, monitor_id)


    @proxy_thread_safe
    def advmon_get_event_count(self, app_id, monitor_id, event):
        """Read the count of a particular event on the given monitor.

        @param app_id: the app id.
        @param monitor_id: the monitor id.
        @param event: name of the specific event or 'All' for all events.

        @returns: count of the specific event or dict of counts of all events.

        """
        return self._proxy.advmon_get_event_count(app_id, monitor_id, event)


    @proxy_thread_safe
    def advmon_reset_event_count(self, app_id, monitor_id, event):
        """Reset the count of a particular event on the given monitor.

        @param app_id: the app id.
        @param monitor_id: the monitor id.
        @param event: name of the specific event or 'All' for all events.

        @returns: True on success, False otherwise.

        """
        return self._proxy.advmon_reset_event_count(app_id, monitor_id, event)

    @proxy_thread_safe
    def advmon_interleave_scan_logger_start(self):
        """ Start interleave logger recording
        """
        self._proxy.advmon_interleave_scan_logger_start()

    @proxy_thread_safe
    def advmon_interleave_scan_logger_stop(self):
        """ Stop interleave logger recording

        @returns: True if logs were successfully collected,
                  False otherwise.

        """
        return self._proxy.advmon_interleave_scan_logger_stop()

    @proxy_thread_safe
    def advmon_interleave_scan_logger_get_records(self):
        """ Get records in previous log collections

        @returns: a list of records, where each item is a record of
                  interleave |state| and the |time| the state starts.
                  |state| could be {'no filter', 'allowlist'}
                  |time| is system time in sec

        """
        return self._proxy.advmon_interleave_scan_logger_get_records()

    @proxy_thread_safe
    def advmon_interleave_scan_logger_get_cancel_events(self):
        """ Get cancel events in previous log collections

        @returns: a list of cancel |time| when a interleave cancel event log
                  was found.
                  |time| is system time in sec

        """
        return self._proxy.advmon_interleave_scan_logger_get_cancel_events()

    @proxy_thread_safe
    def messages_start(self):
        """Start messages monitoring."""
        self._proxy.messages_start()

    @proxy_thread_safe
    def messages_stop(self):
        """Stop messages monitoring.

        @returns: True if logs were successfully gathered since logging started,
                else False
        """
        return self._proxy.messages_stop()

    @proxy_thread_safe
    def messages_find(self, pattern_str):
        """Find if a pattern string exists in messages output.

        @param pattern_str: the pattern string to find.

        @returns: True on success. False otherwise.

        """
        return self._proxy.messages_find(pattern_str)

    @proxy_thread_safe
    def register_advertisement(self, advertisement_data):
        """Register an advertisement.

        Note that rpc supports only conformable types. Hence, a
        dict about the advertisement is passed as a parameter such
        that the advertisement object could be contructed on the host.

        @param advertisement_data: a dict of the advertisement for
                                   the adapter to register.

        @returns: True on success. False otherwise.

        """
        return self._proxy.register_advertisement(advertisement_data)


    @proxy_thread_safe
    def unregister_advertisement(self, advertisement_data):
        """Unregister an advertisement.

        @param advertisement_data: a dict of the advertisement to unregister.

        @returns: True on success. False otherwise.

        """
        return self._proxy.unregister_advertisement(advertisement_data)


    @proxy_thread_safe
    def set_advertising_intervals(self, min_adv_interval_ms,
                                  max_adv_interval_ms):
        """Set advertising intervals.

        @param min_adv_interval_ms: the min advertising interval in ms.
        @param max_adv_interval_ms: the max advertising interval in ms.

        @returns: True on success. False otherwise.

        """
        return self._proxy.set_advertising_intervals(min_adv_interval_ms,
                                                     max_adv_interval_ms)


    @proxy_thread_safe
    def reset_advertising(self):
        """Reset advertising.

        This includes unregister all advertisements, reset advertising
        intervals, and disable advertising.

        @returns: True on success. False otherwise.

        """
        return self._proxy.reset_advertising()


    @proxy_thread_safe
    def create_audio_record_directory(self, audio_record_dir):
        """Create the audio recording directory.

        @param audio_record_dir: the audio recording directory

        @returns: True on success. False otherwise.
        """
        return self._proxy.create_audio_record_directory(audio_record_dir)


    @proxy_thread_safe
    def start_capturing_audio_subprocess(self, audio_data, recording_device):
        """Start capturing audio in a subprocess.

        @param audio_data: the audio test data
        @param recording_device: which device recorded the audio,
                possible values are 'recorded_by_dut' or 'recorded_by_peer'

        @returns: True on success. False otherwise.
        """
        return self._proxy.start_capturing_audio_subprocess(
                json.dumps(audio_data), recording_device)


    @proxy_thread_safe
    def stop_capturing_audio_subprocess(self):
        """Stop capturing audio.

        @returns: True on success. False otherwise.
        """
        return self._proxy.stop_capturing_audio_subprocess()


    @proxy_thread_safe
    def start_playing_audio_subprocess(self, audio_data):
        """Start playing audio in a subprocess.

        @param audio_data: the audio test data

        @returns: True on success. False otherwise.
        """
        audio_data = json.dumps(audio_data)
        return self._proxy.start_playing_audio_subprocess(audio_data)


    @proxy_thread_safe
    def stop_playing_audio_subprocess(self):
        """Stop playing audio in the subprocess.

        @returns: True on success. False otherwise.
        """
        return self._proxy.stop_playing_audio_subprocess()


    @proxy_thread_safe
    def play_audio(self, audio_data):
        """Play audio.

        It blocks until it has completed playing back the audio.

        @param audio_data: the audio test data

        @returns: True on success. False otherwise.
        """
        return self._proxy.play_audio(json.dumps(audio_data))


    @proxy_thread_safe
    def check_audio_frames_legitimacy(self, audio_test_data, recording_device,
                                      recorded_file):
        """Get the number of frames in the recorded audio file.
        @param audio_test_data: the audio test data
        @param recording_device: which device recorded the audio,
                possible values are 'recorded_by_dut' or 'recorded_by_peer'
        @param recorded_file: the recorded file name

        @returns: True if audio frames are legitimate.
        """
        return self._proxy.check_audio_frames_legitimacy(
                json.dumps(audio_test_data), recording_device, recorded_file)


    @proxy_thread_safe
    def convert_audio_sample_rate(self, input_file, out_file, test_data,
                                  new_rate):
        """Convert audio file to new sample rate.

        @param input_file: Path to file to upsample.
        @param out_file: Path to create upsampled file.
        @param test_data: Dictionary with information about file.
        @param new_rate: New rate to upsample file to.

        @returns: True if upsampling succeeded, False otherwise.
        """
        return self._proxy.convert_audio_sample_rate(input_file, out_file,
                                                     json.dumps(test_data),
                                                     new_rate)


    @proxy_thread_safe
    def trim_wav_file(self, in_file, out_file, new_duration, test_data,
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
        return self._proxy.trim_wav_file(in_file, out_file, new_duration,
                                         json.dumps(test_data), tolerance)


    @proxy_thread_safe
    def unzip_audio_test_data(self, tar_path, data_dir):
        """Unzip audio test data files.

        @param tar_path: Path to audio test data tarball on DUT.
        @oaram data_dir: Path to directory where to extract test data directory.

        @returns: True if audio test data folder exists, False otherwise.
        """
        return self._proxy.unzip_audio_test_data(tar_path, data_dir)


    @proxy_thread_safe
    def convert_raw_to_wav(self, input_file, output_file, test_data):
        """Convert raw audio file to wav file.

        @oaram input_file: The location of the raw file.
        @param output_file: The location to place the resulting wav file.
        @param test_data: The data for the file being converted.

        @returns: True if conversion was successful, otherwise false.
        """
        return self._proxy.convert_raw_to_wav(input_file, output_file,
                                              json.dumps(test_data))


    @proxy_thread_safe
    def get_primary_frequencies(self, audio_test_data, recording_device,
                                recorded_file):
        """Get primary frequencies of the audio test file.

        @param audio_test_data: the audio test data
        @param recording_device: which device recorded the audio,
                possible values are 'recorded_by_dut' or 'recorded_by_peer'
        @param recorded_file: the recorded file name

        @returns: a list of primary frequencies of channels in the audio file
        """
        return self._proxy.get_primary_frequencies(
                json.dumps(audio_test_data), recording_device, recorded_file)


    @proxy_thread_safe
    def enable_wbs(self, value):
        """Enable or disable wideband speech (wbs) per the value.

        @param value: True to enable wbs.

        @returns: True if the operation succeeds.
        """
        logging.debug('%s wbs', 'enable' if value else 'disable')
        return self._proxy.enable_wbs(value)


    @proxy_thread_safe
    def set_player_playback_status(self, status):
        """Set playback status for the registered media player.

        @param status: playback status in string.

        """
        logging.debug('Set media player playback status to %s', status)
        return self._proxy.set_player_playback_status(status)


    @proxy_thread_safe
    def set_player_position(self, position):
        """Set media position for the registered media player.

        @param position: position in micro seconds.

        """
        logging.debug('Set media player position to %d', position)
        return self._proxy.set_player_position(position)


    @proxy_thread_safe
    def set_player_metadata(self, metadata):
        """Set metadata for the registered media player.

        @param metadata: dictionary of media metadata.

        """
        logging.debug('Set media player album:%s artist:%s title:%s',
                      metadata.get("album"), metadata.get("artist"),
                      metadata.get("title"))
        return self._proxy.set_player_metadata(metadata)


    @proxy_thread_safe
    def set_player_length(self, length):
        """Set media length for the registered media player.

        @param length: length in micro seconds.

        """
        logging.debug('Set media player length to %d', length)
        return self._proxy.set_player_length(length)


    @proxy_thread_safe
    def select_input_device(self, device_name):
        """Select the audio input device.

        @param device_name: the name of the Bluetooth peer device

        @returns: True if the operation succeeds.
        """
        return self._proxy.select_input_device(device_name)


    @proxy_thread_safe
    def select_output_node(self, node_type):
        """Select the audio output node.

        @param node_type: the node type of the Bluetooth peer device

        @returns: True if the operation succeeds.
        """
        return self._proxy.select_output_node(node_type)


    @proxy_thread_safe
    def get_selected_output_device_type(self):
        """Get the selected audio output node type.

        @returns: the node type of the selected output device.
        """
        return self._proxy.get_selected_output_device_type()


    @proxy_thread_safe
    def read_characteristic(self, uuid, address):
        """Reads the value of a gatt characteristic.

        Reads the current value of a gatt characteristic.

        @param uuid: The uuid of the characteristic to read, as a string.
        @param address: The MAC address of the remote device.

        @returns: A byte array containing the value of the if the uuid/address
                      was found in the object tree.
                  None if the uuid/address was not found in the object tree, or
                      if a DBus exception was raised by the read operation.

        """
        value = self._proxy.read_characteristic(uuid, address)
        if value is None:
            return None
        return bytearray(base64.standard_b64decode(value))


    @proxy_thread_safe
    def write_characteristic(self, uuid, address, bytes_to_write):
        """Performs a write operation on a gatt characteristic.

        Writes to a GATT characteristic on a remote device.

        @param uuid: The uuid of the characteristic to write to, as a string.
        @param address: The MAC address of the remote device, as a string.
        @param bytes_to_write: A byte array containing the data to write.

        @returns: True if the write operation does not raise an exception.
                  None if the uuid/address was not found in the object tree, or
                      if a DBus exception was raised by the write operation.

        """
        return self._proxy.write_characteristic(
            uuid, address, base64.standard_b64encode(bytes_to_write))


    @proxy_thread_safe
    def exchange_messages(self, tx_object_path, rx_object_path, bytes_to_write):
        """Performs a write operation on a gatt characteristic and wait for
        the response on another characteristic.

        @param tx_object_path: the object path of the characteristic to write.
        @param rx_object_path: the object path of the characteristic to read.
        @param value: A byte array containing the data to write.

        @returns: The value of the characteristic to read from.
                  None if the uuid/address was not found in the object tree, or
                      if a DBus exception was raised by the write operation.

        """
        return self._proxy.exchange_messages(
            tx_object_path, rx_object_path,
            base64.standard_b64encode(bytes_to_write))


    @proxy_thread_safe
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
        return self._proxy.start_notify(object_path, cccd_value)


    @proxy_thread_safe
    def stop_notify(self, object_path):
        """Stops the notification session on the gatt characteristic.

        @param object_path: the object path of the characteristic.

        @returns: True if the operation succeeds.
                  False if the characteristic is not found, or
                      if a DBus exception was raised by the operation.

        """
        return self._proxy.stop_notify(object_path)


    @proxy_thread_safe
    def is_notifying(self, object_path):
        """Is the GATT characteristic in a notifying session?

        @param object_path: the object path of the characteristic.

        @return True if it is in a notification session. False otherwise.

        """
        return self._proxy.is_notifying(object_path)


    @proxy_thread_safe
    def is_characteristic_path_resolved(self, uuid, address):
        """Checks whether a characteristic is in the object tree.

        Checks whether a characteristic is curently found in the object tree.

        @param uuid: The uuid of the characteristic to search for.
        @param address: The MAC address of the device on which to search for
            the characteristic.

        @returns: True if the characteristic is found, False otherwise.

        """
        return self._proxy.is_characteristic_path_resolved(uuid, address)


    @proxy_thread_safe
    def get_gatt_attributes_map(self, address):
        """Return a JSON formated string of the GATT attributes of a device,
        keyed by UUID
        @param address: a string of the MAC address of the device

        @return: JSON formated string, stored the nested structure of the
        attributes. Each attribute has 'path' and ['chrcs' | 'descs'], which
        store their object path and children respectively.
        """
        return self._proxy.get_gatt_attributes_map(address)


    @proxy_thread_safe
    def get_gatt_service_property(self, object_path, property_name):
        """Get property from a service attribute
        @param object_path: a string of the object path of the service
        @param property_name: a string of a property, ex: 'Value', 'UUID'

        @return: the property if success,
                 None otherwise
        """
        return self._proxy.get_gatt_service_property(object_path, property_name)


    @proxy_thread_safe
    def get_gatt_characteristic_property(self, object_path, property_name):
        """Get property from a characteristic attribute
        @param object_path: a string of the object path of the characteristic
        @param property_name: a string of a property, ex: 'Value', 'UUID'

        @return: the property if success,
                 None otherwise
        """
        return self._proxy.get_gatt_characteristic_property(object_path,
                                                            property_name)


    @proxy_thread_safe
    def get_gatt_descriptor_property(self, object_path, property_name):
        """Get property from a descriptor attribute
        @param object_path: a string of the object path of the descriptor
        @param property_name: a string of a property, ex: 'Value', 'UUID'

        @return: the property if success,
                 None otherwise
        """
        return self._proxy.get_gatt_descriptor_property(object_path,
                                                        property_name)


    @proxy_thread_safe
    def gatt_characteristic_read_value(self, uuid, object_path):
        """Perform method ReadValue on a characteristic attribute
        @param uuid: a string of uuid
        @param object_path: a string of the object path of the characteristic

        @return: base64 string of dbus bytearray
        """
        return self._proxy.gatt_characteristic_read_value(uuid, object_path)


    @proxy_thread_safe
    def gatt_descriptor_read_value(self, uuid, object_path):
        """Perform method ReadValue on a descriptor attribute
        @param uuid: a string of uuid
        @param object_path: a string of the object path of the descriptor

        @return: base64 string of dbus bytearray
        """
        return self._proxy.gatt_descriptor_read_value(uuid, object_path)


    @proxy_thread_safe
    def get_gatt_object_path(self, address, uuid):
        """Get property from a characteristic attribute

        @param address: The MAC address of the remote device.
        @param uuid: The uuid of the attribute.

        @return: the object path of the attribute if success,
                 none otherwise

        """
        return self._proxy.get_gatt_object_path(address, uuid)


    def copy_logs(self, destination):
        """Copy the logs generated by this device to a given location.

        @param destination: destination directory for the logs.

        """
        self.host.collect_logs(self.XMLRPC_LOG_PATH, destination)


    @proxy_thread_safe
    def get_connection_info(self, address):
        """Get device connection info.

        @param address: The MAC address of the device.

        @returns: On success, a tuple of:
                      ( RSSI, transmit_power, max_transmit_power )
                  None otherwise.

        """
        return self._proxy.get_connection_info(address)


    @proxy_thread_safe
    def set_discovery_filter(self, filter):
        """Set the discovery filter.

        @param filter: The discovery filter to set.

        @return True on success, False otherwise.

        """
        return self._proxy.set_discovery_filter(filter)


    @proxy_thread_safe
    def set_le_connection_parameters(self, address, parameters):
        """Set the LE connection parameters.

        @param address: The MAC address of the device.
        @param parameters: The LE connection parameters to set.

        @return: True on success. False otherwise.

        """
        return self._proxy.set_le_connection_parameters(address, parameters)


    @proxy_thread_safe
    def wait_for_hid_device(self, device_address):
        """Wait for hid device with given device address.

        Args:
            device_address: Peripheral Address

        Returns:
            True if hid device is found.
        """
        return self._proxy.wait_for_hid_device(device_address)


    @proxy_thread_safe
    def bt_caused_last_resume(self):
        """Checks if last resume from suspend was caused by bluetooth

        @return: True if BT wake path was cause of resume, False otherwise
        """

        return self._proxy.bt_caused_last_resume()

    @proxy_thread_safe
    def find_last_suspend_via_powerd_logs(self):
        """Finds the last suspend attempt via powerd logs.

        @return: Tuple (suspend start time, suspend end time, suspend result) or
                 None
        """
        info = self._proxy.find_last_suspend_via_powerd_logs()

        # Currently, we get the date back in string format due to python2/3
        # inconsistencies. We can get rid of this once everything is running
        # python3 (hopefully)
        # TODO - Revisit converting date to string and back in this method
        if info:
            start_date = datetime.strptime(info[0], self.NATIVE_DATE_FORMAT)
            end_date = datetime.strptime(info[1], self.NATIVE_DATE_FORMAT)
            ret = info[2]

            return (start_date, end_date, ret)

        return None

    @proxy_thread_safe
    def do_suspend(self, seconds, expect_bt_wake):
        """Suspend DUT using the power manager.

        @param seconds: The number of seconds to suspend the device.
        @param expect_bt_wake: Whether we expect bluetooth to wake us from
            suspend. If true, we expect this resume will occur early
        """

        # Do not retry this RPC if it fails or times out
        return self._proxy.do_suspend(seconds, expect_bt_wake, __no_retry=True)


    @proxy_thread_safe
    def get_wlan_vid_pid(self):
        """ Return vendor id and product id of the wlan chip on BT/WiFi module

        @returns: (vid,pid) on success; (None,None) on failure
        """
        return self._proxy.get_wlan_vid_pid()

    @proxy_thread_safe
    def get_bt_module_name(self):
        """ Return bluetooth module name for non-USB devices

        @returns: Name of the Bluetooth module (or string read from device on
                  success); '' on failure
        """
        return self._proxy.get_bt_module_name()

    @proxy_thread_safe
    def get_device_time(self):
        """ Get the current device time. """
        return datetime.strptime(self._proxy.get_device_time(),
                                 self.NATIVE_DATE_FORMAT)

    @proxy_thread_safe
    def close(self, close_host=True):
        """Tear down state associated with the client.

        @param close_host: If True, shut down the xml rpc server by closing the
            underlying host object (which also shuts down all other xml rpc
            servers running on the DUT). Otherwise, only shut down the
            bluetooth device xml rpc server, which can be desirable if the host
            object and/or other xml rpc servers need to be used afterwards.
        """
        # Turn off the discoverable flag since it may affect future tests.
        self._proxy.set_discoverable(False)
        # Reset the adapter and leave it on.
        self._proxy.reset_on()
        # This kills the RPC server.
        if close_host:
            self.host.close()
        elif self._bt_direct_proxy:
            self.host.rpc_server_tracker.disconnect(
                    constants.BLUETOOTH_DEVICE_XMLRPC_SERVER_PORT)
