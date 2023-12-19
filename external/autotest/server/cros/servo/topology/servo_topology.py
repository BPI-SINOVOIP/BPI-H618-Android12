#!/usr/bin/env python2
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import print_function
from __future__ import absolute_import
from __future__ import unicode_literals
from __future__ import division

import os
import copy
import json
import base64
import logging

import common
from autotest_lib.client.common_lib import hosts
from autotest_lib.server.cros.servo.topology import topology_constants as stc


class ServoTopologyError(Exception):
    """
    Generic Exception for failures from ServoTopology object.
    """
    pass


class MissingServoError(ServoTopologyError):
    """
    Exception to throw when child servo type is missing.
    """

    def __init__(self, message, servo_type):
        self._servo_type = servo_type
        self.message = message

    def __str__(self):
        return repr(self.message)


class ServoTopology(object):
    """Class to read, generate and validate servo topology in the lab.

    The class support detection of servo listed in ST_PRODUCT_TYPES.
    To save servo topology to host-info date passed two steps:
       - convert to the json
       - encode to base64
    """

    def __init__(self, servo_host):
        self._host = servo_host
        self._topology = None

    def read(self, host_info):
        """Reading servo-topology info."""
        logging.info('Reading servo topology info...')
        self._topology = None
        if not host_info:
            logging.info('The host_info not provided. Skip reading.')
            return
        b64_val = host_info.get_label_value(stc.SERVO_TOPOLOGY_LABEL_PREFIX)
        self._topology = _parse_string_as_topology(b64_val)
        logging.debug('Loaded servo topology: %s', self._topology)
        if self._topology:
            logging.info('Servo topology loaded successfully.')

    def save(self, host_info_store):
        """Saving servo-topology info."""
        if self.is_empty():
            logging.info('Topology is empty. Skip saving.')
            return
        if not host_info_store:
            logging.info('The host_info_store not provided. Skip saving.')
            return
        logging.info('Saving servo topology info...')
        data = _convert_topology_to_string(self._topology)
        if not data:
            logging.info('Servo topology fail to save data.'
                         ' Please file a bug.')
            return
        host_info = host_info_store.get()
        prev_value = host_info.get_label_value(stc.SERVO_TOPOLOGY_LABEL_PREFIX)
        if prev_value and prev_value == data:
            logging.info('Servo topology was not changed. Skip saving.')
            return
        logging.debug('Previous saved topology: %s', prev_value)
        host_info.set_version_label(stc.SERVO_TOPOLOGY_LABEL_PREFIX, data)
        host_info_store.commit(host_info)
        logging.info('Servo topology saved successfully.')

    def generate(self):
        """Read servo data and create topology."""
        try:
            self._topology = self._generate()
        except Exception as e:
            self._topology = None
            logging.debug('(Not critical) %s', e)
            logging.info('Fail to generate servo-topology')
        if not self.is_empty():
            logging.info('Servo topology successfully generated.')

    def is_empty(self):
        """If topology data was initialized."""
        return not bool(self._topology)

    def validate(self, raise_error=False, dual_set=False, compare=False):
        """Validate topology against expected topology.

        Validation against:
        - set-up expectation: min one child or 2 for DUAL_V4
        - last saved topology: check if any device missed

        @params raise_error: raise error if validate did not pass otherwise
                             return False.
        @params dual_set:    Check if servo expect DUAL_V4 setup.
        @params compare:     Validate against saved topology.
        """
        new_st = self._generate()
        if not new_st or not new_st.get(stc.ST_DEVICE_MAIN):
            message = 'Main device is not detected'
            return self._process_error(message, raise_error)
        children = new_st.get(stc.ST_DEVICE_CHILDREN)
        # basic setup has to have minimum one child.
        if not children or len(children) < 1:
            message = 'Each setup has at least one child'
            return self._process_error(message, raise_error)
        children_types = [c.get(stc.ST_DEVICE_TYPE) for c in children]
        # DUAL_V4 setup has to have cr50 and one more child.
        if dual_set:
            if stc.ST_CR50_TYPE not in children_types:
                return self._missing_servo_error(stc.ST_CR50_TYPE, raise_error)
            if len(children) < 2:
                message = 'Expected two children but have only one'
                return self._process_error(message, raise_error)
        if compare and not self.is_empty():
            main_device = new_st.get(stc.ST_DEVICE_MAIN)
            t = self._topology
            old_main = t.get(stc.ST_DEVICE_MAIN)
            old_children = t.get(stc.ST_DEVICE_CHILDREN)
            if not all([
                    old_children,
                    old_main,
                    old_main.get(stc.ST_DEVICE_HUB_PORT),
            ]):
                # Old data is invalid for comparasing
                return True
            if not self._equal_item(old_main, main_device):
                message = 'Main servo was changed'
                return self._process_error(message, raise_error)
            for child in old_children:
                old_type = child.get(stc.ST_DEVICE_TYPE)
                if old_type not in children_types:
                    return self._missing_servo_error(old_type, raise_error)
            if len(children) < len(old_children):
                message = 'Some child is missed'
                return self._process_error(message, raise_error)
        logging.info('Servo topology successfully verified.')
        return True

    def _process_error(self, message, raise_error):
        if not raise_error:
            logging.info('Validate servo topology failed with: %s', message)
            return False
        raise ServoTopologyError(message)

    def _missing_servo_error(self, servo_type, raise_error):
        message = 'Missed servo: %s!' % servo_type
        if not raise_error:
            logging.info('Validate servo topology failed with: %s', message)
            return False
        raise MissingServoError(message, servo_type)

    def _equal_item(self, old, new):
        """Servo was replugged to another port"""
        for field in stc.SERVO_TOPOLOGY_ITEM_COMPARE_FIELDS:
            if old.get(field) != new.get(field):
                return False
        return True

    def _generate(self):
        """Generate and return topology structure.

        Read and generate topology structure with out update the state.
        """
        logging.debug('Trying generate a servo-topology')
        core_servo_serial = self._host.servo_serial
        if not core_servo_serial:
            logging.info('Servo serial is not provided.')
            return None
        logging.debug('Getting topology for core servo: %s', core_servo_serial)
        # collect main device info
        cmd_hub = 'servodtool device -s %s usb-path' % core_servo_serial
        servo_path = self._read_line(cmd_hub)
        logging.debug('Device -%s path: %s', core_servo_serial, servo_path)
        if not servo_path:
            logging.info('Core servo not detected.')
            return None
        if not self._is_expected_type(servo_path):
            return None
        main_device = self._read_device_info(servo_path)
        if not main_device:
            logging.debug('Core device missed some data')
            return None
        # collect child device info
        children = []
        hub_path = servo_path[0:-2]
        logging.debug('Core hub path: %s', hub_path)
        devices_cmd = 'find %s/* -name serial' % hub_path
        devices = self._read_multilines(devices_cmd)
        core_device_port = main_device.get(stc.ST_DEVICE_HUB_PORT)
        for device in devices:
            logging.debug('Child device %s', device)
            device_dir = os.path.dirname(device)
            if not self._is_expected_type(device_dir):
                # skip not expected device type like USB or hubs
                continue
            child = self._read_device_info(device_dir)
            if not child:
                logging.debug('Child missed some data.')
                continue
            if core_device_port == child.get(stc.ST_DEVICE_HUB_PORT):
                logging.debug('Skip device if match with core device')
                continue
            children.append(child)
        topology = {
                stc.ST_DEVICE_MAIN: main_device,
                stc.ST_DEVICE_CHILDREN: children
        }
        logging.debug('Servo topology: %s', topology)
        return topology

    def _is_expected_type(self, path):
        """Check if device type is known servo type.

        Please update ST_PRODUCT_TYPES to extend more servo types.
        """
        product = self._read_file(path, 'product')
        if bool(stc.ST_PRODUCT_TYPES.get(product)):
            return True
        logging.info('Unknown product: %s', product)
        return False

    def _read_device_info(self, path):
        """Read device details for topology.

        @params path: Absolute path to the device in FS.
        """
        serial = self._read_file(path, 'serial')
        product = self._read_file(path, 'product')
        hub_path = self._read_file(path, 'devpath')
        stype = stc.ST_PRODUCT_TYPES.get(product)
        return self._create_item(serial, stype, product, hub_path)

    def _create_item(self, servo_serial, servo_type, product, hub_path):
        """Create topology item.

        Return created item only if all details provided.

        @params servo_serial:   Serial number of device.
        @params servo_type:     Product type code of the device.
        @params product:        Product name of the device.
        @params hub_path:       Device enumerated folder name. Show the
                                chain of used ports to connect the device.
        """
        item = {
                stc.ST_DEVICE_SERIAL: servo_serial,
                stc.ST_DEVICE_TYPE: servo_type,
                stc.ST_DEVICE_PRODUCT: product,
                stc.ST_DEVICE_HUB_PORT: hub_path
        }
        if not (servo_serial and servo_type and product and hub_path):
            logging.debug('Some data missing: %s', item)
            return None
        return item

    def _read_file(self, path, file_name):
        """Read context of the file and return result as one line.

        If execution finished with error result will be empty string.

        @params path:       Path to the folder where file located.
        @params file_name:  The file name to read.
        """
        if not path or not file_name:
            return ''
        f = os.path.join(path, file_name)
        return self._read_line('cat %s' % f)

    def _read_line(self, command):
        """Execute terminal command and return result as one line.

        If execution finished with error result will be empty string.

        @params command:    String to execute.
        """
        r = self._host.run(command, ignore_status=True, timeout=30)
        if r.exit_status == 0:
            return r.stdout.strip()
        return ''

    def _read_multilines(self, command):
        """Execute terminal command and return result as multi-line.

        If execution finished with error result will be an empty array.

        @params command:    String to execute.
        """
        r = self._host.run(command, ignore_status=True, timeout=30)
        if r.exit_status == 0:
            return r.stdout.splitlines()
        return []


def _convert_topology_to_string(topology):
    """Convert topology to the string respresentation.

    Convert topology to json and encode by Base64 for host-info file.

    @params topology: Servo topology data
    @returns: topology representation in Base64 string
    """
    if not topology:
        return ''
    try:
        # generate json similar to golang to avoid extra updates
        json_string = json.dumps(topology, separators=(',', ':'))
        logging.debug('Servo topology (json): %s', json_string)
    except Exception as e:
        logging.debug('(Not critical) %s', e)
        logging.info('Failed to convert topology to json')
        return ''
    try:
        # recommended to convert to the bytes for python 3
        b64_string = base64.b64encode(json_string.encode("utf-8"))
        logging.debug('Servo topology (b64): %s', b64_string)
        return b64_string
    except Exception as e:
        logging.debug('(Not critical) %s', e)
        logging.info('Failed to convert topology to base64')
    return ''


def _parse_string_as_topology(src):
    """Parse and load servo topology from string.

    Decode Base64 and load as json of servo-topology data.

    @params src: topology representation in Base64 string
    @returns: servo topology data
    """
    if not src:
        logging.debug('Servo topology data not present in host-info.')
        return None
    try:
        json_string = base64.b64decode(src)
        logging.debug('Servo topology (json) from host-info: %s', json_string)
        return json.loads(json_string)
    except Exception as e:
        logging.debug('(Not critical) %s', e)
        logging.info('Fail to read servo-topology from host-info.')
    return None
