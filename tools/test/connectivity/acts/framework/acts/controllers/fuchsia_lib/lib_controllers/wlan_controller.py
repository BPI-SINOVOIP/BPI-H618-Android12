#!/usr/bin/env python3
#
#   Copyright 2021 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import time

from acts import logger
from acts import signals
from acts import utils

TIME_TO_SLEEP_BETWEEN_RETRIES = 1
TIME_TO_WAIT_FOR_COUNTRY_CODE = 10


class WlanControllerError(signals.ControllerError):
    pass


class WlanController:
    """Contains methods related to wlan core, to be used in FuchsiaDevice object"""
    def __init__(self, fuchsia_device):
        self.device = fuchsia_device
        self.log = logger.create_tagged_trace_logger(
            'WlanController for FuchsiaDevice | %s' % self.device.ip)

    # TODO(70501): Wrap wlan_lib functions and setup from FuchsiaDevice here
    # (similar to how WlanPolicyController does it) to prevent FuchsiaDevice
    # from growing too large.
    def _configure_wlan(self):
        pass

    def _deconfigure_wlan(self):
        pass

    def get_wlan_client_interface_id(self):
        """ Returns the wlan interface id of the first found wlan client
        interface.
        """
        # Retrieve wlan ifaces
        response = self.device.wlan_lib.wlanGetIfaceIdList()
        if response.get('error'):
            raise WlanControllerError('Failed to get WLAN iface ids: %s' %
                                      response['error'])

        # If iface has role 'client', retunr id
        iface_ids = response.get('result', [])
        for id in iface_ids:
            query_response = self.device.wlan_lib.wlanQueryInterface(id)
            if query_response.get('error'):
                raise WlanControllerError(
                    'Failed to query wlan iface id %s: %s' %
                    (id, query_response['error']))

            if query_response['result'].get('role').lower() == 'client':
                return id

        return None

    def get_wlan_interface_mac_addr_from_id(self, iface_id):
        """ Retrieves the mac address of a wlan iface, using the wlan iface
        id.

        Args:
            iface_id: int, wlan iface id

        Returns:
            string, mac address of wlan iface
        """
        query_response = self.device.wlan_lib.wlanQueryInterface(iface_id)
        if query_response.get('error'):
            raise WlanControllerError('Failed to query wlan iface id %s: %s' %
                                      (iface_id, query_response['error']))
        return utils.mac_address_list_to_str(
            query_response['result'].get('mac_addr'))

    def get_wlan_interface_name(self, mac_addr=None):
        """ Retrieves name (netstack) of wlan interface using the mac address. If
        mac address is not provided, returns the name of the first found wlan
        client (as opposed to AP) interface.

        Args:
            mac_addr: optional, string or list of decimal octets representing
                the mac addr of the wlan interface. e.g. "44:07:0b:50:c1:ef" or
                [68, 7, 11, 80, 193, 239]

        Returns:
            string, name of wlan interface
        """
        # Default to first found client wlan interface
        if not mac_addr:
            client_iface_id = self.get_wlan_client_interface_id()
            mac_addr = self.get_wlan_interface_mac_addr_from_id(
                client_iface_id)

        # Convert mac addr to list, for comparison
        if type(mac_addr) == str:
            mac_addr = utils.mac_address_str_to_list(mac_addr)

        err = self.device.netstack_lib.init().get('error')
        if err:
            raise WlanControllerError('Failed to init netstack_lib: %s' % err)

        # Retrieve net ifaces
        response = self.device.netstack_lib.netstackListInterfaces()
        if response.get('error'):
            raise WlanControllerError(
                'Failed to get network interfaces list: %s' %
                response['error'])

        # Find iface with matching mac addr, and return name
        for iface_info in response['result']:
            if iface_info['mac'] == mac_addr:
                return iface_info['name']
        return None

    def set_country_code(self, country_code):
        """Sets country code through the regulatory region service and waits
        for the code to be applied to WLAN PHY.

        Args:
            country_code: string, the 2 character country code to set

        Raises:
            EnvironmentError - failure to get/set regulatory region
            ConnectionError - failure to query PHYs
        """
        self.log.info('Setting DUT country code to %s' % country_code)
        country_code_response = self.device.regulatory_region_lib.setRegion(
            country_code)
        if country_code_response.get('error'):
            raise EnvironmentError(
                'Failed to set country code (%s) on DUT. Error: %s' %
                (country_code, country_code_response['error']))

        self.log.info('Verifying DUT country code was correctly set to %s.' %
                      country_code)
        phy_ids_response = self.device.wlan_lib.wlanPhyIdList()
        if phy_ids_response.get('error'):
            raise ConnectionError('Failed to get phy ids from DUT. Error: %s' %
                                  (country_code, phy_ids_response['error']))

        end_time = time.time() + TIME_TO_WAIT_FOR_COUNTRY_CODE
        while time.time() < end_time:
            for id in phy_ids_response['result']:
                get_country_response = self.device.wlan_lib.wlanGetCountry(id)
                if get_country_response.get('error'):
                    raise ConnectionError(
                        'Failed to query PHY ID (%s) for country. Error: %s' %
                        (id, get_country_response['error']))

                set_code = ''.join([
                    chr(ascii_char)
                    for ascii_char in get_country_response['result']
                ])
                if set_code != country_code:
                    self.log.debug(
                        'PHY (id: %s) has incorrect country code set. '
                        'Expected: %s, Got: %s' % (id, country_code, set_code))
                    break
            else:
                self.log.info('All PHYs have expected country code (%s)' %
                              country_code)
                break
            time.sleep(TIME_TO_SLEEP_BETWEEN_RETRIES)
        else:
            raise EnvironmentError('Failed to set DUT country code to %s.' %
                                   country_code)
