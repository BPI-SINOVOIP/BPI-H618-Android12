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

from acts.controllers.android_lib.tel import tel_utils
from acts.controllers.cellular_lib import BaseCellularDut


class AndroidCellularDut(BaseCellularDut.BaseCellularDut):
    """ Android implementation of the cellular DUT class."""
    def __init__(self, ad, logger):
        """ Keeps a handler to the android device.

        Args:
           ad: Android device handler
           logger: a handler to the logger object
        """
        self.ad = ad
        self.log = logger

    def toggle_airplane_mode(self, new_state=True):
        """ Turns airplane mode on / off.

        Args:
          new_state: True if airplane mode needs to be enabled.
        """
        tel_utils.toggle_airplane_mode(self.log, self.ad, new_state)

    def toggle_data_roaming(self, new_state=True):
        """ Enables or disables cellular data roaming.

        Args:
          new_state: True if data roaming needs to be enabled.
        """
        tel_utils.toggle_cell_data_roaming(self.ad, new_state)

    def get_rx_tx_power_levels(self):
        """ Obtains Rx and Tx power levels measured from the DUT.

        Returns:
            A tuple where the first element is an array with the RSRP value
            in each Rx chain, and the second element is the Tx power in dBm.
            Values for invalid or disabled Rx / Tx chains are set to None.
        """
        return tel_utils.get_rx_tx_power_levels(self.log, self.ad)

    def set_apn(self, name, apn, type='default'):
        """ Sets the Access Point Name.

        Args:
          name: the APN name
          apn: the APN
          type: the APN type
        """
        self.ad.droid.telephonySetAPN(name, apn, type)

    def set_preferred_network_type(self, type):
        """ Sets the preferred RAT.

        Args:
          type: an instance of class PreferredNetworkType
        """
        if type == BaseCellularDut.PreferredNetworkType.LTE_ONLY:
            formatted_type = tel_utils.NETWORK_MODE_LTE_ONLY
        elif type == BaseCellularDut.PreferredNetworkType.WCDMA_ONLY:
            formatted_type = tel_utils.NETWORK_MODE_WCDMA_ONLY
        elif type == BaseCellularDut.PreferredNetworkType.GSM_ONLY:
            formatted_type = tel_utils.NETWORK_MODE_GSM_ONLY
        else:
            raise ValueError('Invalid RAT type.')

        if not self.ad.droid.telephonySetPreferredNetworkTypesForSubscription(
                formatted_type, self.ad.droid.subscriptionGetDefaultSubId()):
            self.log.error("Could not set preferred network type.")
        else:
            self.log.info("Preferred network type set.")

    def get_telephony_signal_strength(self):
        """ Wrapper for the method with the same name in tel_utils.

        Will be deprecated and replaced by get_rx_tx_power_levels. """
        tel_utils.get_telephony_signal_strength(self.ad)
