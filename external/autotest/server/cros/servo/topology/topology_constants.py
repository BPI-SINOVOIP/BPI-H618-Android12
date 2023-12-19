#!/usr/bin/env python2
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Constants that will be used as key name in device health profile.
SERVO_TOPOLOGY_LABEL_PREFIX = 'servo_topology'
ST_DEVICE_MAIN = 'main'
ST_DEVICE_CHILDREN = 'children'

# Fields of servo topology item
ST_DEVICE_SERIAL = 'serial'
ST_DEVICE_TYPE = 'type'
ST_DEVICE_PRODUCT = 'sysfs_product'
ST_DEVICE_HUB_PORT = 'usb_hub_port'

ST_CR50_TYPE = 'ccd_cr50'

# Mapping between product names and types.
ST_PRODUCT_TYPES = {
        'Servo V4': 'servo_v4',
        'Servo V4p1': 'servo_v4p1',
        'Cr50': ST_CR50_TYPE,
        'Servo Micro': 'servo_micro',
        'C2D2': 'c2d2',
        'Sweetberry': 'sweetberry'
}

# List unchangeable fields per device.
SERVO_TOPOLOGY_ITEM_COMPARE_FIELDS = (
        ST_DEVICE_SERIAL,
        ST_DEVICE_TYPE,
        ST_DEVICE_PRODUCT,
)
