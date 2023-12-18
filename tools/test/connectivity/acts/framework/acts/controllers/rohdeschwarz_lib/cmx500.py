#!/usr/bin/env python3
#
#   Copyright 2020 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#           http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from enum import Enum
from acts.controllers import abstract_inst

class BtsNumber(Enum):
    """Base station Identifiers."""
    BTS1 = 'PCC'
    BTS2 = 'SCC1'
    BTS3 = 'SCC2'
    BTS4 = 'SCC3'
    BTS5 = 'SCC4'
    BTS6 = 'SCC6'
    BTS7 = 'SCC7'

class Cmx500(abstract_inst.SocketInstrument):

    def __init__(self, ip_addr, port):
        """Init method to setup variables for controllers.

        Args:
              ip_addr: Controller's ip address.
              port: Port
        """
        super(Cmx500, self).__init__(ip_addr, port)

    def switch_lte_signalling(self, state):
        """ Turns LTE signalling ON/OFF.

        Args:
              state: an instance of LteState indicating the state to which LTE
                signal has to be set.
        """
        raise NotImplementedError()

    def enable_packet_switching(self):
        """Enable packet switching in call box."""
        raise NotImplementedError()

    def disable_packet_switching(self):
        """Disable packet switching in call box."""
        raise NotImplementedError()

    @property
    def use_carrier_specific(self):
        """Gets current status of carrier specific duplex configuration."""
        raise NotImplementedError()

    @use_carrier_specific.setter
    def use_carrier_specific(self, state):
        """Sets the carrier specific duplex configuration.

        Args:
            state: ON/OFF UCS configuration.
        """
        raise NotImplementedError()

    def send_and_recv(self, cmd):
        """Send and recv the status of the command.

        Args:
            cmd: Command to send.

        Returns:
            status: returns the status of the command sent.
        """
        raise NotImplementedError()

    def configure_mimo_settings(self, mimo):
        """Sets the mimo scenario for the test.

        Args:
            mimo: mimo scenario to set.
        """
        raise NotImplementedError()

    def wait_for_pswitched_state(self, timeout=10):
        """Wait until pswitched state.

        Args:
            timeout: timeout for lte pswitched state.

        Raises:
            CmxError on timeout.
        """
        raise NotImplementedError()

    def wait_for_attached_state(self, timeout=120):
        """Attach the controller with device.

        Args:
            timeout: timeout for phone to get attached.

        Raises:
            CmxError on time out.
        """
        raise NotImplementedError()

    def wait_for_rrc_state(self, state, timeout=120):
        """ Waits until a certain RRC state is set.

        Args:
            state: the RRC state that is being waited for.
            timeout: timeout for phone to be in connnected state.

        Raises:
            CmxError on time out.
        """
        raise NotImplementedError()

    def reset(self):
        """System level reset"""
        raise NotImplementedError()

    @property
    def get_instrument_id(self):
        """Gets instrument identification number"""
        raise NotImplementedError()

    def disconnect(self):
        """Disconnect controller from device and switch to local mode."""
        raise NotImplementedError()

    def close_remote_mode(self):
        """Exits remote mode to local mode."""
        raise NotImplementedError()

    def detach(self):
        """Detach callbox and controller."""
        raise NotImplementedError()

    @property
    def rrc_connection(self):
        """Gets the RRC connection state."""
        raise NotImplementedError()

    @rrc_connection.setter
    def rrc_connection(self, state):
        """Selects whether the RRC connection is kept or released after attach.

        Args:
            mode: RRC State ON/OFF.
        """
        raise NotImplementedError()

    @property
    def rrc_connection_timer(self):
        """Gets the inactivity timeout for disabled rrc connection."""
        raise NotImplementedError()

    @rrc_connection_timer.setter
    def rrc_connection_timer(self, time_in_secs):
        """Sets the inactivity timeout for disabled rrc connection. By default
        the timeout is set to 5.

        Args:
            time_in_secs: timeout of inactivity in rrc connection.
        """
        raise NotImplementedError()

    @property
    def dl_mac_padding(self):
        """Gets the state of mac padding."""
        raise NotImplementedError()

    @dl_mac_padding.setter
    def dl_mac_padding(self, state):
        """Enables/Disables downlink padding at the mac layer.

        Args:
            state: ON/OFF
        """
        raise NotImplementedError()

    @property
    def connection_type(self):
        """Gets the connection type applied in callbox."""
        raise NotImplementedError()

    @connection_type.setter
    def connection_type(self, ctype):
        """Sets the connection type to be applied.

        Args:
            ctype: Connection type.
        """
        raise NotImplementedError()

    def get_base_station(self, bts_num=BtsNumber.BTS1):
        """Gets the base station object based on bts num. By default
        bts_num set to PCC

        Args:
            bts_num: base station identifier

        Returns:
            base station object.
        """
        raise NotImplementedError()

    def init_lte_measurement(self):
        """Gets the class object for lte measurement which can be used to
        initiate measurements.

        Returns:
            lte measurement object.
        """
        raise NotImplementedError()


class BaseStation(object):
    """Class to interact with different base stations"""

    def __init__(self, cmx, bts_num):
        if not isinstance(bts_num, BtsNumber):
            raise ValueError('bts_num should be an instance of BtsNumber.')
        self._bts = bts_num.value
        self._cmx = cmx

    @property
    def duplex_mode(self):
        """Gets current duplex of cell."""
        raise NotImplementedError()

    @duplex_mode.setter
    def duplex_mode(self, mode):
        """Sets the Duplex mode of cell.

        Args:
            mode: String indicating FDD or TDD.
        """
        raise NotImplementedError()

    @property
    def band(self):
        """Gets the current band of cell."""
        raise NotImplementedError()

    @band.setter
    def band(self, band):
        """Sets the Band of cell.

        Args:
            band: band of cell.
        """
        raise NotImplementedError()

    @property
    def dl_channel(self):
        """Gets the downlink channel of cell."""
        raise NotImplementedError()

    @dl_channel.setter
    def dl_channel(self, channel):
        """Sets the downlink channel number of cell.

        Args:
            channel: downlink channel number of cell.
        """
        raise NotImplementedError()

    @property
    def ul_channel(self):
        """Gets the uplink channel of cell."""
        raise NotImplementedError()

    @ul_channel.setter
    def ul_channel(self, channel):
        """Sets the up link channel number of cell.

        Args:
            channel: up link channel number of cell.
        """
        raise NotImplementedError()

    @property
    def bandwidth(self):
        """Get the channel bandwidth of the cell."""
        raise NotImplementedError()

    @bandwidth.setter
    def bandwidth(self, bandwidth):
        """Sets the channel bandwidth of the cell.

        Args:
            bandwidth: channel bandwidth of cell.
        """
        raise NotImplementedError()

    @property
    def ul_frequency(self):
        """Get the uplink frequency of the cell."""
        raise NotImplementedError()

    @ul_frequency.setter
    def ul_frequency(self, freq):
        """Get the uplink frequency of the cell.

        Args:
            freq: uplink frequency of the cell.
        """
        raise NotImplementedError()

    @property
    def dl_frequency(self):
        """Get the downlink frequency of the cell"""
        raise NotImplementedError()

    @dl_frequency.setter
    def dl_frequency(self, freq):
        """Get the downlink frequency of the cell.

        Args:
            freq: downlink frequency of the cell.
        """
        raise NotImplementedError()

    @property
    def transmode(self):
        """Gets the TM of cell."""
        raise NotImplementedError()

    @transmode.setter
    def transmode(self, tm_mode):
        """Sets the TM of cell.

        Args:
            tm_mode: TM of cell.
        """
        raise NotImplementedError()

    @property
    def downlink_power_level(self):
        """Gets RSPRE level."""
        raise NotImplementedError()

    @downlink_power_level.setter
    def downlink_power_level(self, pwlevel):
        """Modifies RSPRE level.

        Args:
            pwlevel: power level in dBm.
        """
        raise NotImplementedError()

    @property
    def uplink_power_control(self):
        """Gets open loop nominal power directly."""
        raise NotImplementedError()

    @uplink_power_control.setter
    def uplink_power_control(self, ul_power):
        """Sets open loop nominal power directly.

        Args:
            ul_power: uplink power level.
        """
        raise NotImplementedError()

    @property
    def uldl_configuration(self):
        """Gets uldl configuration of the cell."""
        raise NotImplementedError()

    @uldl_configuration.setter
    def uldl_configuration(self, uldl):
        """Sets the ul-dl configuration.

        Args:
            uldl: Configuration value ranging from 0 to 6.
        """
        raise NotImplementedError()

    @property
    def tdd_special_subframe(self):
        """Gets special subframe of the cell."""
        raise NotImplementedError()

    @tdd_special_subframe.setter
    def tdd_special_subframe(self, sframe):
        """Sets the tdd special subframe of the cell.

        Args:
            sframe: Integer value ranging from 1 to 9.
        """
        raise NotImplementedError()

    @property
    def scheduling_mode(self):
        """Gets the current scheduling mode."""
        raise NotImplementedError()

    @scheduling_mode.setter
    def scheduling_mode(self, mode):
        """Sets the scheduling type for the cell.

        Args:
            mode: Selects the channel mode to be scheduled.
        """
        raise NotImplementedError()

    @property
    def rb_configuration_dl(self):
        """Gets rmc's rb configuration for down link. This function returns
        Number of Resource blocks, Resource block position and Modulation type.
        """
        raise NotImplementedError()

    @rb_configuration_dl.setter
    def rb_configuration_dl(self, rb_config):
        """Sets the rb configuration for down link for scheduling type.

        Args:
            rb_config: Tuple containing Number of resource blocks, resource
            block position and modulation type.

        Raises:
            ValueError: If tuple unpacking fails.
        """
        raise NotImplementedError()

    @property
    def rb_configuration_ul(self):
        """Gets rb configuration for up link. This function returns
        Number of Resource blocks, Resource block position and Modulation type.
        """
        raise NotImplementedError()

    @rb_configuration_ul.setter
    def rb_configuration_ul(self, rb_config):
        """Sets the rb configuration for down link for scheduling mode.

        Args:
            rb_config: Tuple containing Number of resource blocks, resource
            block position and modulation type.

        Raises:
            ValueError: If tuple unpacking fails.
        """
        raise NotImplementedError()

    def validate_rb(self, rb):
        """Validates if rb is within the limits for bandwidth set.

        Args:
            rb: No. of resource blocks.

        Raises:
            ValueError if rb out of range.
        """
        raise NotImplementedError()

    @property
    def rb_position_dl(self):
        """Gets the position of the allocated down link resource blocks within
        the channel band-width.
        """
        raise NotImplementedError()

    @rb_position_dl.setter
    def rb_position_dl(self, rbpos):
        """Selects the position of the allocated down link resource blocks
        within the channel band-width

        Args:
            rbpos: position of resource blocks.
        """
        raise NotImplementedError()

    @property
    def rb_position_ul(self):
        """Gets the position of the allocated up link resource blocks within
        the channel band-width.
        """
        raise NotImplementedError()

    @rb_position_ul.setter
    def rb_position_ul(self, rbpos):
        """Selects the position of the allocated up link resource blocks
        within the channel band-width.

        Args:
            rbpos: position of resource blocks.
        """
        raise NotImplementedError()

    @property
    def dci_format(self):
        """Gets the downlink control information (DCI) format."""
        raise NotImplementedError()

    @dci_format.setter
    def dci_format(self, dci_format):
        """Selects the downlink control information (DCI) format.

        Args:
            dci_format: supported dci.
        """
        raise NotImplementedError()

    @property
    def dl_antenna(self):
        """Gets dl antenna count of cell."""
        raise NotImplementedError()

    @dl_antenna.setter
    def dl_antenna(self, num_antenna):
        """Sets the dl antenna count of cell.

        Args:
            num_antenna: Count of number of dl antennas to use.
        """
        raise NotImplementedError()

    @property
    def reduced_pdcch(self):
        """Gets the reduction of PDCCH resources state."""
        raise NotImplementedError()

    @reduced_pdcch.setter
    def reduced_pdcch(self, state):
        """Sets the reduction of PDCCH resources state.

        Args:
            state: ON/OFF.
        """
        raise NotImplementedError()

    def tpc_power_control(self, set_type):
        """Set and execute the Up Link Power Control via TPC.

        Args:
            set_type: Type of tpc power control.
        """
        raise NotImplementedError()

    @property
    def tpc_closed_loop_target_power(self):
        """Gets the target powers for power control with the TPC setup."""
        raise NotImplementedError()

    @tpc_closed_loop_target_power.setter
    def tpc_closed_loop_target_power(self, cltpower):
        """Sets the target powers for power control with the TPC setup.

        Args:
            tpower: Target power.
        """
        raise NotImplementedError()

    @property
    def drx_connected_mode(self):
        """ Gets the Connected DRX LTE cell parameter

        Args:
            None

        Returns:
            DRX connected mode (OFF, AUTO, MANUAL)
        """
        raise NotImplementedError()

    @drx_connected_mode.setter
    def drx_connected_mode(self, mode):
        """  Sets the Connected DRX LTE cell parameter

        Args:
            mode: DRX Connected mode

        Returns:
            None
        """
        raise NotImplementedError()

    @property
    def drx_on_duration_timer(self):
        """ Gets the amount of PDCCH subframes to wait for data after
            waking up from a DRX cycle

        Args:
            None

        Returns:
            DRX mode duration timer
        """
        raise NotImplementedError()

    @drx_on_duration_timer.setter
    def drx_on_duration_timer(self, time):
        """ Sets the amount of PDCCH subframes to wait for data after
            waking up from a DRX cycle

        Args:
            timer: Length of interval to wait for user data to be transmitted

        Returns:
            None
        """
        raise NotImplementedError()

    @property
    def drx_inactivity_timer(self):
        """ Gets the number of PDCCH subframes to wait before entering DRX mode

        Args:
            None

        Returns:
            DRX mode inactivity timer
        """
        raise NotImplementedError()

    @drx_inactivity_timer.setter
    def drx_inactivity_timer(self, time):
        """ Sets the number of PDCCH subframes to wait before entering DRX mode

        Args:
            timer: Length of the interval to wait

        Returns:
            None
        """
        raise NotImplementedError()

    @property
    def drx_retransmission_timer(self):
        """ Gets the number of consecutive PDCCH subframes to wait
        for retransmission

        Args:
            None

        Returns:
            Number of PDCCH subframes to wait for retransmission
        """
        raise NotImplementedError()

    @drx_retransmission_timer.setter
    def drx_retransmission_timer(self, time):
        """ Sets the number of consecutive PDCCH subframes to wait
        for retransmission

        Args:
            time: Number of PDCCH subframes to wait
            for retransmission

        Returns:
            None
        """
        raise NotImplementedError()

    @property
    def drx_long_cycle(self):
        """ Gets the amount of subframes representing a DRX long cycle

        Args:
            None

        Returns:
            The amount of subframes representing one long DRX cycle.
            One cycle consists of DRX sleep + DRX on duration
        """
        raise NotImplementedError()

    @drx_long_cycle.setter
    def drx_long_cycle(self, time):
        """ Sets the amount of subframes representing a DRX long cycle

        Args:
            long_cycle: The amount of subframes representing one long DRX cycle.
                One cycle consists of DRX sleep + DRX on duration

        Returns:
            None
        """
        raise NotImplementedError()

    @property
    def drx_long_cycle_offset(self):
        """ Gets the offset used to determine long cycle starting
        subframe

        Args:
            None

        Returns:
            Long cycle offset
        """
        raise NotImplementedError()

    @drx_long_cycle_offset.setter
    def drx_long_cycle_offset(self, offset):
        """ Sets the offset used to determine long cycle starting
        subframe

        Args:
            offset: Number in range 0...(long cycle - 1)
        """
        raise NotImplementedError()


class CmxError(Exception):
    """Class to raise exceptions related to cmx."""
