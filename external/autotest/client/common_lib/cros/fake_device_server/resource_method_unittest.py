#!/usr/bin/python3

# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for resource_method.py."""

import unittest
import mock

import common
from fake_device_server import common_util
from fake_device_server import resource_method
from fake_device_server import resource_delegate
from fake_device_server import server_errors


class ResourceMethodTest(unittest.TestCase):
    """Tests for the ResourceMethod class."""

    def setUp(self):
        """Sets up resource_method object and dict of resources."""
        self.resources = {}
        self.resource_method = resource_method.ResourceMethod(
                resource_delegate.ResourceDelegate(self.resources))


    def testPatch(self):
        """Tests that we correctly patch a resource."""
        expected_resource = dict(id=1234, blah='hi')
        update_resource = dict(blah='hi')
        self.resources[(1234, None)] = dict(id=1234)

        common_util.parse_serialized_json = mock.MagicMock(
            return_value=update_resource)

        returned_json = self.resource_method.PATCH(1234)
        self.assertEquals(expected_resource, returned_json)
        common_util.parse_serialized_json.assert_called_once()


    def testPut(self):
        """Tests that we correctly replace a resource."""
        update_resource = dict(id=12345, blah='hi')
        self.resources[(12345, None)] = dict(id=12345)

        common_util.parse_serialized_json = mock.MagicMock(
            return_value=update_resource)

        returned_json = self.resource_method.PUT(12345)
        self.assertEquals(update_resource, returned_json)
        common_util.parse_serialized_json.assert_called_once()

        common_util.parse_serialized_json.reset_mock()

        # Ticket id doesn't match.
        update_resource = dict(id=12346, blah='hi')
        common_util.parse_serialized_json = mock.MagicMock(
            return_value=update_resource)

        self.assertRaises(server_errors.HTTPError,
                          self.resource_method.PUT, 12345)
        common_util.parse_serialized_json.assert_called_once()


if __name__ == '__main__':
    unittest.main()
