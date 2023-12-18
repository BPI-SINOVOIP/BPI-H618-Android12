#!/usr/bin/env python3
#
# Copyright 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import logging
import time
import statistics
from queue import Empty
from concurrent.futures import ThreadPoolExecutor

from acts_contrib.test_utils.bt.bt_gatt_utils import close_gatt_client
from acts_contrib.test_utils.bt.bt_coc_test_utils import do_multi_connection_throughput
from acts_contrib.test_utils.bt.bt_gatt_utils import disconnect_gatt_connection
from acts_contrib.test_utils.bt.bt_constants import gatt_cb_err
from acts_contrib.test_utils.bt.bt_constants import gatt_cb_strings
from acts_contrib.test_utils.bt.bt_constants import l2cap_coc_header_size
from acts_contrib.test_utils.bt.bt_gatt_utils import GattTestUtilsError
from acts_contrib.test_utils.bt.bt_coc_test_utils import orchestrate_coc_connection
from acts_contrib.test_utils.bt.bt_gatt_utils import orchestrate_gatt_connection

default_event_timeout = 10
rssi_read_duration = 25


def establish_ble_connection(client_ad, server_ad):
    """Function to establish BLE connection between two BLE devices.

    Args:
        client_ad: the Android device performing the connection.
        server_ad: the Android device accepting the connection.
    Returns:
        bluetooth_gatt: GATT object
        gatt_callback: Gatt callback object
        adv_callback: advertisement callback object
        gatt_server: the gatt server
    """
    gatt_server_cb = server_ad.droid.gattServerCreateGattServerCallback()
    gatt_server = server_ad.droid.gattServerOpenGattServer(gatt_server_cb)
    try:
        bluetooth_gatt, gatt_callback, adv_callback = (
            orchestrate_gatt_connection(client_ad, server_ad))
    except GattTestUtilsError as err:
        logging.error(err)
        return False
    return bluetooth_gatt, gatt_callback, adv_callback, gatt_server


def read_ble_rssi(client_ad, gatt_server, gatt_callback):
    """Function to Read BLE RSSI of the remote BLE device.
    Args:
        client_ad: the Android device performing the connection.
        gatt_server: the gatt server
        gatt_callback:the gatt connection call back object
    Returns:
      ble_rssi: RSSI value of the remote BLE device
    """
    AVG_RSSI = []
    end_time = time.time() + rssi_read_duration
    logging.info("Reading BLE RSSI for {} sec".format(rssi_read_duration))
    while time.time() < end_time:
        expected_event = gatt_cb_strings['rd_remote_rssi'].format(
            gatt_callback)
        read_rssi = client_ad.droid.gattClientReadRSSI(gatt_server)
        if read_rssi:
            try:
                event = client_ad.ed.pop_event(expected_event,
                                               default_event_timeout)
            except Empty:
                logging.error(
                    gatt_cb_err['rd_remote_rssi_err'].format(expected_event))
                return False
        rssi_value = event['data']['Rssi']
        AVG_RSSI.append(rssi_value)
    logging.debug("First & Last reading of RSSI :{:03d} & {:03d}".format(
        AVG_RSSI[0], AVG_RSSI[-1]))
    ble_rssi = statistics.mean(AVG_RSSI)
    ble_rssi = round(ble_rssi, 2)

    return ble_rssi


def ble_coc_connection(client_ad, server_ad):
    """Sets up the CoC connection between two Android devices.

    Args:
        client_ad: the Android device performing the connection.
        server_ad: the Android device accepting the connection.

    Returns:
        True if connection was successful or false if unsuccessful,
        gatt_callback: GATT callback object
        client connection ID: Client connection ID
        and server connection ID : server connection ID
    """
    #secured_conn: True if using secured connection
    #le_connection_interval: LE Connection interval. 0 means use default.
    #buffer_size : is the number of bytes per L2CAP data buffer
    #le_tx_data_length: LE Data Length used by BT Controller to transmit.
    is_secured = False
    le_connection_interval = 30
    buffer_size = 240
    le_tx_data_length = buffer_size + l2cap_coc_header_size
    gatt_server_cb = server_ad.droid.gattServerCreateGattServerCallback()
    gatt_server = server_ad.droid.gattServerOpenGattServer(gatt_server_cb)

    logging.info(
        "orchestrate_ble_coc_connection. is_secured={}, Connection Interval={}msec, "
        "buffer_size={}bytes".format(is_secured, le_connection_interval,
                                     buffer_size))
    try:
        status, client_conn_id, server_conn_id, bluetooth_gatt, gatt_callback = orchestrate_coc_connection(
            client_ad,
            server_ad,
            True,
            is_secured,
            le_connection_interval,
            le_tx_data_length,
            gatt_disconnection=False)
    except Exception as err:
        logging.info("Failed to esatablish COC connection".format(err))
        return 0
    return True, gatt_callback, gatt_server, bluetooth_gatt, client_conn_id


def run_ble_throughput(server_ad, client_conn_id, client_ad,
                       num_iterations=30):
    """Function to measure Throughput from one client to one-or-many servers

    Args:
        server_ad: the Android device accepting the connection.
        client_conn_id: the client connection ID.
        client_ad: the Android device performing the connection.
        num_iterations: The num_iterations is that number of repetitions of each
        set of buffers r/w.
    Returns:
      data_rate: Throughput in terms of bytes per second, 0 if test failed.
    """
    # number_buffers is the total number of data buffers to transmit per
    # set of buffers r/w.
    # buffer_size is the number of bytes per L2CAP data buffer.
    number_buffers = 100
    buffer_size = 240
    list_server_ad = [server_ad]
    list_client_conn_id = [client_conn_id]
    data_rate = do_multi_connection_throughput(client_ad, list_server_ad,
                                               list_client_conn_id,
                                               num_iterations, number_buffers,
                                               buffer_size)
    if data_rate <= 0:
        return False
    data_rate = data_rate * 8
    logging.info(
        "run_ble_coc_connection_throughput: throughput=%d bites per sec",
        data_rate)
    return data_rate


def run_ble_throughput_and_read_rssi(client_ad, server_ad, client_conn_id,
                                     gatt_server, gatt_callback):
    """Function to measure ble rssi while sendinng data from client to server

    Args:
        client_ad: the Android device performing the connection.
        server_ad: the Android device accepting the connection.
        client_conn_id: the client connection ID.
        gatt_server: the gatt server
        gatt_callback: Gatt callback object
    Returns:
      ble_rssi: RSSI value of the remote BLE device.
    """
    executor = ThreadPoolExecutor(2)
    ble_throughput = executor.submit(run_ble_throughput, client_ad,
                                     client_conn_id, server_ad)
    ble_rssi = executor.submit(read_ble_rssi, server_ad, gatt_server,
                               gatt_callback)
    logging.info("BLE RSSI is:{} dBm with data rate={} bites per sec ".format(
        ble_rssi.result(), ble_throughput.result()))
    return ble_rssi.result()


def ble_gatt_disconnection(client_ad, bluetooth_gatt, gatt_callback):
    """Function to disconnect GATT connection between client and server.

    Args:
        client_ad: the Android device performing the connection.
        bluetooth_gatt: GATT object
        gatt_callback:the gatt connection call back object
    Returns:
      ble_rssi: RSSI value of the remote BLE device
    """
    logging.info("Disconnecting from peripheral device.")
    try:
        disconnect_gatt_connection(client_ad, bluetooth_gatt, gatt_callback)
        close_gatt_client(client_ad, bluetooth_gatt)
    except GattTestUtilsError as err:
        logging.error(err)
        return False
    return True
